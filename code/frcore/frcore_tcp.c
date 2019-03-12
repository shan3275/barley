#include "frcore_pkt.h"

#include "frcore_tcp.h"
#include "frcore_stat.h"

#include "frc_dma.h"
#include "frcore_chan.h"
#include "frcore_proto.h"
#include "frcore_ssn.h"
#include "frcore_ssn_priv.h"

CVMX_SHARED uint64_t rx_tcp_pkts = 0;
CVMX_SHARED uint32_t tcprec_lock = 0;

#if FRC_CONFIG_TCP
#if 0
int frcore_tcp_process(cvmx_wqe_t *work, uint8_t *eth_ptr, uint8_t *ip_ptr, uint8_t *tcp_ptr, uint16_t ip_paylen, uint64_t smac, uint64_t dmac)
{
    int rv;
    frc_dma_hdr_t hdr;
    frc_dma_pkt_info_t info;
    int eth_len;

    FRCORE_CYCLE_RECORDING();

    FRCORE_PKT("=============== rx_tcp_pkts %lld.\n", (ULL) ++rx_tcp_pkts);

    memset(&hdr, 0, sizeof(frc_dma_hdr_t));
    memset(&info, 0, sizeof(frc_dma_pkt_info_t));

    UP32(ip_ptr + 12, hdr.sip);

    UP32(ip_ptr + 16, hdr.dip);

    UP16(tcp_ptr, hdr.sport);
    UP16(tcp_ptr + 2, hdr.dport);

    eth_len          = work->len;

    hdr.protocol     = PROTO_TCP;
    hdr.hash         = work->tag & 0xffffff;
    hdr.info_offset  = FRC_DMA_PKT_INFO_OFFSET;
    hdr.pkt_num      = 1;
    hdr.total_paylen = eth_len;

    info.data_offset = 0;
    info.payload_len = eth_len;
    info.smac        = smac;
    info.dmac        = dmac;

    rv = frcore_forward_pkt_to_host(FRC_TCP_QUEUE, &hdr, &info, eth_ptr);
    if (rv)
    {
        FRCORE_TCP("frcore_forward_tcp_pkt_to_host return %d, eth_len %d.\n", rv, eth_len);
        FRCORE_STAT_INC(stat_dma_errors);
    }

    return FRCORE_ACT_FORWARD;
}
#else
int frcore_tcp_process(cvmx_wqe_t *work, uint8_t *eth_ptr, uint8_t *ip_ptr, uint8_t *tcp_ptr, uint16_t ip_paylen, uint64_t smac, uint64_t dmac)
{
    struct mpp_control *mpp=NULL;
    struct mpp_tuple tuple;
    struct mpp_ssn *ssn;
    struct tcphdr *th;
    uint8_t th_flags;
    uint32_t seq; /* first packet seq number */
    uint32_t ack_seq; /* first packet ack number */
    uint8_t direction;

    //FRCORE_CYCLE_RECORDING();
    mpp = (struct mpp_control*)(&work->packet_data[0]);
    mpp->packet = (uint8_t *)cvmx_phys_to_ptr(work->packet_ptr.s.addr);
    #define _IPH ((struct iphdr*)((uint8_t *)mpp->packet+work->word2.s.ip_offset))

    mpp->ssn = NULL;
    mpp->ssn_index = SSN_INDEX_INVALID;
    mpp->tag = 0;
    mpp->vlan_act = MPP_VLAN_DO_NOTHING;
    mpp->len_header = work->word2.s.ip_offset;

    th = (struct tcphdr *)(((uint8_t*)(_IPH))+(_IPH->ihl<<2));
    th_flags = *(((uint8_t *)(th)) + 13);
    /*innormal packet, flag all 0 or all 1 */
    if((th_flags & 0x3f) == 0x3f ||
       (th_flags & 0x3f) == 0x00) {
        FRCORE_STAT_INC(stat_tcp_malformed_packet);
        return FRCORE_ACT_DROP;
    }
    /* count malformed packet */
    if(((_IPH->ihl + th->doff) <<2) > (_IPH->tot_len)) {
        FRCORE_STAT_INC(stat_tcp_malformed_packet);
        return FRCORE_ACT_DROP;
    }
    seq = th->seq;
    ack_seq = th->ack_seq;

    /* count tcp option packet */
    if((th->doff << 2) > 20) {
        FRCORE_STAT_INC(stat_tcp_option);
    }
    /* count tcp payload 0 packet */
    if((_IPH->tot_len) - ((_IPH->ihl + th->doff) <<2) == 0) {
        FRCORE_STAT_INC(stat_tcp_payload_zero);
    }

    mpp->udph = (struct udphdr*)(((uint8_t*)(_IPH))+(_IPH->ihl<<2));
    //tuple.dip = _IPH->daddr;
    //tuple.sip = _IPH->saddr;
    tuple.data[0] = *((uint64_t*)(((char*)_IPH)+12));
    //tuple.dp = mpp->udph->dest;
    //tuple.sp = mpp->udph->source;
    tuple.data[1] = 0ul;
    tuple.data_32[2] = *(uint32_t*)(mpp->udph);
    tuple.proto = _IPH->protocol;
    /* ssn */

    {
        /* 5-tuple sniffer session */
        struct mpp_tuple tuple_sort;
        struct mpp_tuple tuple_positive; //read from ssn structure
        ssn_sort_tuple(&tuple, &tuple_sort);
        mpp->ssn_index = ssn_hash_by_tuple(tuple_sort.data, SSN_HASH_BUCKET_SIZE_MASK);
        ssn = &gsdata.ssn[mpp->ssn_index];
        uint32_t  *s_sniffer = &ssn->ssn_status;
        cvmx_spinlock_bit_lock(s_sniffer);

        mpp->ssn = ssn_lookup_by_tuple(mpp->ssn_index, &tuple_sort, MPP_SSN_HASH_MASK_SNIFFER);
        if(!mpp->ssn) {
            /* if session has not been created, create session */
            //FRCORE_CYCLE_RECORDING();
            mpp->ssn = ssn_prepare(mpp->ssn_index, tuple.data, MPP_SSN_HASH_MASK_DATA64, th_flags, seq, ack_seq);
            //FRCORE_CYCLE_RECORDING();
            if(cvmx_unlikely(!mpp->ssn)) {
                /* failed for creating session */
                //MC_PRINTF_ERR("-----------------------\n");
                FRCORE_STAT_INC(stat_ssn_fail);
                cvmx_spinlock_bit_unlock(s_sniffer);
                return FRCORE_ACT_DROP;
            }else{
                /* succeed for creating session */
                mpp->ssn->key.data_64[2] = 1;
                mpp->ssn->key.data_64[3] = work->len;//cvmx_wqe_get_len(work);//work->len;
                mpp->ssn->key.action = MPP_SSN_ACTION_TCPFLOWREC;

                /* for tcp flow recovery*/
                mpp->ssn->tcp_flow_positive_data.tsegqe_len = 0;
                mpp->ssn->tcp_flow_positive_data.rcv_nxt = 0;
                mpp->ssn->tcp_flow_positive_data.head.lh_first = NULL;
                mpp->ssn->tcp_flow_negative_data.tsegqe_len = 0;
                mpp->ssn->tcp_flow_negative_data.rcv_nxt = 0;
                mpp->ssn->tcp_flow_negative_data.head.lh_first = NULL;
                mpp->ssn->tcp_flow_positive_data.rev_flow_data = &mpp->ssn->tcp_flow_negative_data;
                mpp->ssn->tcp_flow_negative_data.rev_flow_data = &mpp->ssn->tcp_flow_positive_data;
                MC_PRINTF_INFO("\n");
                FRCORE_STAT_INC(stat_ssn_prepare);
                ssn_add(mpp->ssn);
                ssn_set_smac(mpp->ssn, smac);
                ssn_set_dmac(mpp->ssn, dmac);
                //md_dump_ssn(mpp->ssn);
            }
            /* after ssn has been created, fill in the data to rec queue */
        }

        /* if session has been created, begin tcp flow recovery */
        //FRCORE_CYCLE_RECORDING();
        tuple_positive.data[0] = mpp->ssn->key.data_64[0];
        tuple_positive.data[1] = mpp->ssn->key.data_64[1] & MPP_SSN_HASH_MASK_DATA64;
        direction = ssn_direction_tuple(&tuple, &tuple_positive);
        ssn_execute_packet(work, mpp->ssn, mpp, direction);
        //FRCORE_CYCLE_RECORDING();
        //md_dump_tcpflowrec(mpp->ssn);
        cvmx_spinlock_bit_unlock(s_sniffer);
        goto crslk;
    }

    crslk:
    return FRCORE_ACT_UNFREE;
#undef _IPH
}
#endif

#if FRC_CONFIG_AGE
 int frcore_ssn_age_process(cvmx_wqe_t *work)
{
    //FRCORE_CYCLE_RECORDER_INIT();
#if !FRC_CONFIG_SSN_ATOMIC
    if(work->grp/*cvmx_wqe_get_grp(work)*/ == GROUP_TO_DATA_PLANE_AGING){
#endif
        //FRCORE_CYCLE_RECORDING();
        struct mpp_ssn *ssn = (struct mpp_ssn*)work;
        struct mpp_ssn *ssn1=&gsdata.ssn[ssn->ssn_index];
        uint32_t *s = &ssn1->ssn_status;
        FRCORE_STAT_INC(stat_ssn_tim_fired);
        if(cvmx_atomic_fetch_and_add32(&ssn->ttl, -1)==0){
        /* when ssn is expired, send out all tcp fragments on its self direction */
            //FRCORE_CYCLE_RECORDING();
            cvmx_spinlock_bit_lock(s);
            if (ssn->key.tcpflowrec) {
                tcp_flow_flush_lock(ssn);
            }
            //FRCORE_CYCLE_RECORDING();
            MC_PRINTF_INFO("ssn purge ssn ssn->key.tcpflowrec: %u\n", ssn->key.tcpflowrec);
            ssn_purge_ssn(ssn);
            cvmx_spinlock_bit_unlock(s);
            //FRCORE_CYCLE_RECORDING();
            //FRCORE_CYCLE_RECORD_DUMP();
            return FRE_SUCCESS;
        }
        FRCORE_SSN("ssn ttl %d\n", ssn->ttl);
        if(ssn_add_timer(ssn, MC_SSN_TTL_UNIT)<0) {
            cvmx_spinlock_bit_lock(s);
            MC_PRINTF_ERR("Tim add failed!\n");
            if (ssn->key.tcpflowrec) {
                printf("ssn->key.tcpflowrec == %d\n", ssn->key.tcpflowrec);
                tcp_flow_flush_lock(ssn);
            }
            ssn_purge_ssn(ssn);
            cvmx_spinlock_bit_unlock(s);
        } else {
            FRCORE_SSN("ssn add timer == 0\n");/* dont do ll/sc on the same cache block after this! */
        }
#if !FRC_CONFIG_SSN_ATOMIC
    }
    else {
        MC_PRINTF_ERR("Unknow group %d %p\n", work->grp, work);
        return FRE_FAIL;
    }
#endif
    return FRE_SUCCESS;
}

#endif

#endif


/* End of file */

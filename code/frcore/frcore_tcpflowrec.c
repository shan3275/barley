
/*
 *  TCP flow recovery implementation for mpp
 */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <stddef.h>
#include <errno.h>

#include "cvmx-config.h"
#include "cvmx.h"
#include "cvmx-helper.h"
#include "cvmx-pko.h"
#include "cvmx-wqe.h"
#include "cvmx-swap.h"
#include "frcore_proto.h"
#include "frcore_ssn.h"
#include "frcore_tcpflowrec.h"
#include "cvm-driver-defs.h"
#include "cvm-pci.h"
#include "frcore_debug.h"
#include "frcore_proc.h"
#include "frcore_dma.h"
#include "frcore_stat.h"
#include "frc_util.h"
#include "frcore_pkt.h"
#include "frcore_chan.h"
//#include "cvmcs-cntq-test.h"
//#include "cvmcs-nic.h"

int        ddoq_pkt_count[2000];

CVMX_SHARED uint8_t disorder_depth = 5;
#if FRC_CONFIG_SSN_CHAN
CVMX_SHARED uint64_t   ddoq_index = 0;
CVMX_SHARED uint64_t   pkts_count = 0;
//(ddoq_index)%5000 + 36

extern CVMX_SHARED  uint64_t            cpu_freq;
uint8_t  ssn_block_exhaust[FRC_SIMPLE_PACKAGE_SIMPLE_CHAN_FIFO_NUM]  = {0};
uint64_t ssn_last_display[FRC_SIMPLE_PACKAGE_SIMPLE_CHAN_FIFO_NUM]   = {0};
int get_ssn_direction(struct mpp_ssn *ssn, uint16_t *direction)
{
    uint32_t index;
    struct mpp_ssn *ssn_positive;

    if (ssn == NULL) {
        return FRE_MEMORY;
    }

    index = ssn->ssn_index;
    ssn_positive = &gsdata.ssn[index];
    if (ssn_positive->key.sip == ssn->key.sip) {
        *direction = FRC_SSN_POSIVTE_FLOW;
    }else if(ssn_positive->key.sip == ssn->key.dip){
        *direction = FRC_SSN_NEGATIVE_FLOW;
    }else {
        return FRE_FAIL;
    }
    return FRE_SUCCESS;
}

int check_ssn_addr(struct frc_ssn_submit_addr *ssn_addr, uint32_t fifo_id)
{
    uint16_t flag;
    uint64_t block_addr;
    unsigned int core_num;
    if (ssn_addr == NULL) {
        return FRE_MEMORY;
    }

    core_num = cvmx_get_core_num();

    flag = ssn_addr->addr_flag;
    if (!flag)
    {
        if (ssn_block_exhaust[core_num]) {
            if (cvmx_get_cycle() - ssn_last_display[core_num]
                < (cpu_freq / DMA_CHANNEL_REST_DIV))
            {
                return FRE_NOSPACE;
            }
        }
        /* block addr no effective, so request a new block addr */
        /* get block addr */
        if (frcore_ssn_get_one_block_addr(&block_addr, fifo_id))
        {
            ssn_block_exhaust[core_num] = 1;
            ssn_last_display[core_num] = cvmx_get_cycle();
            FRCORE_ERROR("frcore_ssn_chan_avail_get failed!\n");
            return FRE_NOSPACE;
        }else {
            ssn_block_exhaust[core_num] = 0;
        }
        ssn_addr->block_addr  = block_addr;
        ssn_addr->head_offset = FRC_DMA_OFFSET;
        ssn_addr->payload_offset = FRC_DMA_OFFSET + sizeof(frc_dma_hdr_t);
        ssn_addr->info_offset = FRC_DMA_SSN_BLOCK_SIZE - sizeof(frc_dma_pkt_info_t);
        ssn_addr->payload_len = 0;
        ssn_addr->pkt_num = 0;
        ssn_addr->addr_flag = 1;
        CVMX_SYNCWS;
    }
    return FRE_SUCCESS;

}

void stat_ssn_block(uint64_t pkt_num)
{
    int index;
    if (pkt_num <= 5) {
        index = (int)(pkt_num - 1);
    }else {
        index = 5;
    }
    FRCORE_STAT_INC((stat_ssn_1packet_blocks + index));
    FRCORE_STAT_INC(stat_ssn_dma_blocks);
}

/* submit dma header to fifo */
int sbumit_dma_header_to_fifo(struct mpp_ssn *ssn)
{
    #if FRC_CONFIG_SSN_CLOSE_SUBMIT_DATA
    #else
    int rv;
    struct frc_ssn_submit_addr *ssn_addr=NULL;
    frc_dma_hdr_t hdr;

    /* get ssn submit addr structure */
    ssn_addr = &ssn->ssn_addr;

    if (ssn_addr->addr_flag ) {
        //printf("%s %d ssn_addr->addr_flag=0x%x\n", __func__, __LINE__, ssn_addr->addr_flag);
        /* blcok if full, so submit the block dma head */
        hdr.sip      = ssn->key.sip;
        hdr.dip      = ssn->key.dip;
        hdr.sport    = ssn->key.sp;
        hdr.dport    = ssn->key.dp;
        hdr.protocol = ssn->key.proto;
        hdr.stop_sec = 0;
        hdr.hash     = ssn->ssn_index;
        hdr.total_paylen = ssn_addr->payload_len;
        hdr.pkt_num  = ssn_addr->pkt_num;

        rv = frcore_forward_ssn_pkt_to_fifo(FRC_WORK_SSN, ssn->ssn_index, FRC_SSN_DMA_HEAD,
                                            &hdr, NULL, NULL, ssn_addr->block_addr,
                                            0, 0, NULL);
        if (rv) {
            FRCORE_STAT_INC(stat_ssn_dma_head_fail);
            printf("%s,%d,rv=%d\n", __FUNCTION__, __LINE__, rv);
            return rv;
        }
        stat_ssn_block(hdr.pkt_num);

        /* get new block */
        ssn_addr->addr_flag = 0;
        CVMX_SYNCWS;
    }
    #endif
    return FRE_SUCCESS;
}

#if 1
/* submit tcp rec data to fifo */
int sbumit_data_to_fifo(tseg_qent *current, struct mpp_ssn *ssn)
{
    #if FRC_CONFIG_SSN_CLOSE_SUBMIT_DATA
    cvmx_wqe_t *wqe = current->work;
    frcore_work_free(wqe);
    #else
    int rv;
    cvmx_wqe_t *wqe = current->work;
    uint32_t index;
    uint32_t fifo_id;
    struct frc_ssn_submit_addr *ssn_addr=NULL;
    frc_dma_hdr_t hdr;
    frc_dma_pkt_info_t info;
    uint64_t data_len;
    uint8_t *payload;
    /* get hash or index */
    index = ssn->ssn_index;
    fifo_id = index % FRC_SSN_FIFO_NUM;

    /* get ssn submit addr structure */
    ssn_addr = &ssn->ssn_addr;

    /* check block addr if effective or not, if not effective ,request a new block_addr */
    rv = check_ssn_addr(ssn_addr, fifo_id);
    if (rv) {
        FRCORE_ERROR("%s,%d,rv=%d\n", __FUNCTION__, __LINE__, rv);
        frcore_work_free(wqe);
        return rv;
    }

    //MC_PRINTF_INFO("ssn directon %d:%s\n", direction, direction ? "positive" : "reverse" );
    /*check current block if full*/
    data_len = current->data_len;
    if (ssn_addr->payload_offset + data_len > ssn_addr->info_offset ) {
        /* blcok if full, so submit the block dma head */
        hdr.sip      = ssn->key.sip;
        hdr.dip      = ssn->key.dip;
        hdr.sport    = ssn->key.sp;
        hdr.dport    = ssn->key.dp;
        hdr.protocol = ssn->key.proto;
        hdr.stop_sec = 0;
        hdr.hash     = ssn->ssn_index;
        hdr.total_paylen = ssn_addr->payload_len;
        hdr.pkt_num  = ssn_addr->pkt_num;

        rv = frcore_forward_ssn_pkt_to_fifo(FRC_WORK_SSN, ssn->ssn_index,FRC_SSN_DMA_HEAD,
                                             &hdr, NULL, NULL,ssn_addr->block_addr,
                                             0, 0, NULL);
        if (rv) {
            FRCORE_STAT_INC(stat_ssn_dma_head_fail);
            printf("%s,%d,rv=%d\n", __FUNCTION__, __LINE__, rv);
            frcore_work_free(wqe);
            return rv;
        }
        stat_ssn_block(hdr.pkt_num);

        /* get new block */
        ssn_addr->addr_flag = 0;
        rv = check_ssn_addr(ssn_addr, fifo_id);
        if (rv) {
            FRCORE_ERROR("%s,%d,rv=%d\n", __FUNCTION__, __LINE__, rv);
            frcore_work_free(wqe);
            return rv;
        }
    }

    /* go on and submit pkt and info_pkt */
    info.sequence = current->seq;
    info.ack_seq  = current->ack_num;
    info.direction = current->direction;
    info.payload_len = current->data_len;
    info.data_offset = ssn_addr->payload_len;
    if (info.direction == FRC_SSN_POSIVTE_FLOW) {
        info.smac = ssn->cont.data_64[1] & 0xffffffffffff;
    info.dmac = ssn->cont.data_64[2] & 0xffffffffffff;
    }else {
        info.smac = ssn->cont.data_64[2] & 0xffffffffffff;
        info.dmac = ssn->cont.data_64[1] & 0xffffffffffff;
    }
    payload = current->data;
    rv = frcore_forward_ssn_pkt_to_fifo(FRC_WORK_SSN, ssn->ssn_index,FRC_SSN_DMA_PAYLOAD,
                                        NULL, &info, payload, ssn_addr->block_addr,
                                        ssn_addr->info_offset, ssn_addr->payload_offset,
                                        wqe);
    MC_PRINTF_DEBUG("ssn_addr->block_addr:0x%llx, ssn_addr->info_offset:%d, ssn_addr->payload_offset:%d\n",
                    ssn_addr->block_addr, ssn_addr->info_offset, ssn_addr->payload_offset);
    MC_PRINTF_DEBUG("current->data_len:%d\n", current->data_len);
    if (rv) {
        FRCORE_ERROR("%s,%d,rv=%d\n", __FUNCTION__, __LINE__, rv);
        frcore_work_free(wqe);
        return rv;
    }
    /* update ssn_addr */
    ssn_addr->info_offset -= sizeof(frc_dma_pkt_info_t);
    ssn_addr->payload_offset += data_len;
    ssn_addr->payload_len += data_len;
    ssn_addr->pkt_num++;
    #endif
    return FRE_SUCCESS;
}
#else
#if !FRC_CONFIG_SSN_SIMPLE_PACKET_TEST
/* for test 1*/
/* submit tcp rec data to fifo */
int sbumit_data_to_fifo(tseg_qent *current, struct mpp_ssn *ssn)
{
    int rv;
    cvmx_wqe_t *wqe = current->work;
    uint16_t direction;
    uint32_t index;
    struct frc_ssn_submit_addr *ssn_addr=NULL;
    struct mpp_ssn *ssn_positive=NULL;
    frc_dma_hdr_t hdr;
    frc_dma_pkt_info_t info;
    uint64_t data_len;
    uint8_t *payload;
    /* get hash or index */
    index = ssn->ssn_index;

    /* get ssn submit addr structure */
    ssn_addr = &gsdata.ssn_addr[index];

    /* get positive ssn */
    ssn_positive = &gsdata.ssn[index];

    /* check block addr if effective or not, if not effective ,request a new block_addr */
    rv = check_ssn_addr(ssn_addr);
    if (rv) {
        return rv;
    }

    /* need know ssn positive or reverse */
    if (get_ssn_direction(ssn, &direction)) {
        return FRE_DIRECTION_FAIL;
    }
    MC_PRINTF_INFO("ssn directon %d:%s\n", direction, direction ? "positive" : "reverse" );

    /* go on and submit pkt and info_pkt */
    info.sequence = current->seq;
    info.ack_seq  = current->ack_num;
    info.direction = direction;
    info.payload_len = current->data_len;
    info.data_offset = ssn_addr->payload_len;
    info.smac = ssn->cont.data_64[1] & 0xffffffffffff;
    info.dmac = ssn->cont.data_64[2] & 0xffffffffffff;
    payload = current->data;
    data_len = current->data_len;
    rv = frcore_forward_ssn_pkt_to_fifo(FRC_WORK_SSN, FRC_SSN_DMA_PAYLOAD, NULL, &info, payload,
                        ssn_addr->block_addr, ssn_addr->info_offset, ssn_addr->payload_offset, wqe);
    MC_PRINTF_DEBUG("ssn_addr->block_addr:0x%llx, ssn_addr->info_offset:%d, ssn_addr->payload_offset:%d\n",
                    ssn_addr->block_addr, ssn_addr->info_offset, ssn_addr->payload_offset);
    MC_PRINTF_DEBUG("current->data_len:%d\n", current->data_len);
    if (rv) {
        printf("%s,%d,rv=%d\n", __FUNCTION__, __LINE__, rv);
        return rv;
    }
    /* update ssn_addr */
    ssn_addr->info_offset -= sizeof(frc_dma_pkt_info_t);
    ssn_addr->payload_offset += data_len;
    ssn_addr->payload_len += data_len;
    ssn_addr->pkt_num++;

    /*submit current packet header */
    hdr.sip      = ssn_positive->key.sip;
    hdr.dip      = ssn_positive->key.dip;
    hdr.sport    = ssn_positive->key.sp;
    hdr.dport    = ssn_positive->key.dp;
    hdr.protocol = ssn_positive->key.proto;
    hdr.hash     = ssn_positive->ssn_index;
    hdr.total_paylen = ssn_addr->payload_len;
    hdr.pkt_num  = ssn_addr->pkt_num;

    rv = frcore_forward_ssn_pkt_to_fifo(FRC_WORK_SSN, FRC_SSN_DMA_HEAD, &hdr, NULL, NULL,
                                        ssn_addr->block_addr, 0, 0, NULL);
    if (rv) {
        printf("%s,%d,rv=%d\n", __FUNCTION__, __LINE__, rv);
        return rv;
    }

    /* get new block */
    ssn_addr->addr_flag = 0;
    rv = check_ssn_addr(ssn_addr);
    if (rv) {
        return rv;
    }

    return FRE_SUCCESS;
}
#else
/* for test 2*/
/* submit tcp rec data to fifo as udp packet */
int sbumit_data_to_fifo(tseg_qent *current, struct mpp_ssn *ssn)
{
    int rv;
    cvmx_wqe_t *wqe = current->work;
    uint16_t direction=0;
    uint32_t index;
    struct frc_ssn_submit_addr *ssn_addr=NULL;
    struct mpp_ssn *ssn_positive=NULL;
    frc_dma_hdr_t hdr;
    frc_dma_pkt_info_t info;
    uint64_t data_len;
    uint8_t *payload;
    /* get hash or index */
    index = ssn->ssn_index;

    /* get positive ssn */
    ssn_positive = &gsdata.ssn[index];

    MC_PRINTF_INFO("ssn directon %d:%s\n", direction, direction ? "positive" : "reverse" );

    /* go on and submit pkt and info_pkt */
    info.sequence = current->seq;
    info.ack_seq  = current->ack_num;
    info.direction = direction;
    info.payload_len = current->data_len;
    info.data_offset = 0;
    info.smac = ssn->cont.data_64[1] & 0xffffffffffff;
    info.dmac = ssn->cont.data_64[2] & 0xffffffffffff;
    payload = current->data;
    data_len = current->data_len;

    /*submit current packet header */
    hdr.sip      = ssn_positive->key.sip;
    hdr.dip      = ssn_positive->key.dip;
    hdr.sport    = ssn_positive->key.sp;
    hdr.dport    = ssn_positive->key.dp;
    hdr.protocol = ssn_positive->key.proto;
    hdr.hash     = ssn_positive->ssn_index;
    hdr.total_paylen = data_len;
    hdr.pkt_num  = 1;

    rv = frcore_forward_simple_pkt_to_fifo(FRC_WORK_UDP, &hdr, &info, payload);
    FRCORE_UDP("frcore_forward_simple_pkt_to_fifo return %d.\n", rv);
    if (rv)
    {
        FRCORE_STAT_INC(stat_dma_errors);
    }
    frcore_work_free(wqe);
    return FRE_SUCCESS;
}
#endif
#endif

int
cvmcs_nic_find_idx(int port)
{
    int i;

    for(i = 0; i < octnic->nports; i++) {
        if(octnic->port[i].linfo.gmxport == port)
            return i;
    }

    return -1;
}

int __send_packet(struct tseg_qent *q, tcp_flow_data *flow_data, struct mpp_ssn *ssn)
{
       static int i=0;
       int rv;
      //MC_PRINTF_ERR("__send_packet\n");
      //MC_PRINTF_ERR("q->seq == 0x%lx\n", q->seq);
      /* Remove from list */
      LIST_REMOVE(q, tqe_q);

      if (flow_data->tsegqe_len == 0) {
          MC_PRINTF_ERR("tsegqe_len == %u tsegqe_len - 1 == %u q->seq == 0x%x\n",
                        flow_data->tsegqe_len, (flow_data->tsegqe_len - 1), q->seq);
          printf("tsegqe_len == %u tsegqe_len - 1 == %u q->seq == 0x%x i=%d\n",
                 flow_data->tsegqe_len, (flow_data->tsegqe_len - 1), q->seq, i++);
      }

      flow_data->tsegqe_len--;
      CVMX_SYNCWS;
      /* Send as a UDP packet */
       //FRCORE_CYCLE_RECORDING();
      rv = sbumit_data_to_fifo(q, ssn);
       //FRCORE_CYCLE_RECORDING();
      if (rv) {
          FRCORE_ERROR("%s %d rv=%d, i=%d\n", __func__, __LINE__,rv, i++);
      }
      //send_udp_packet(q, ssn);
      cvmx_fpa_free(q, CVMX_FPA_TCPFLOWREC_POOL, 0);

      return 0;
}


/* flush tcp flow queue without tag sw lock */
int tcp_flow_flush(tcp_flow_data *flow_data, struct mpp_ssn *ssn)
{
    struct tseg_qent *q;

    while ((q = LIST_FIRST(&flow_data->head))) {
        //MC_PRINTF_ERR("q->seq == 0x%lx\n", q->seq);
        __send_packet(q, flow_data, ssn);
    }

    if (flow_data->tsegqe_len != 0) {
        MC_PRINTF_ERR("End  : flow_data->tsegqe_len == %u\n", flow_data->tsegqe_len);
    }

    /* init data */
    flow_data->rcv_nxt = 0;
    flow_data->tsegqe_len = 0;
    LIST_INIT(&flow_data->head);
    CVMX_SYNCWS;

    return 0;
}

/* flush tcp flow queue with tag sw lock */
int tcp_flow_flush_lock(struct mpp_ssn *ssn)
{
    struct tseg_qent *q;
    tcp_flow_data *flow_data = &ssn->tcp_flow_positive_data;

    MC_PRINTF_INFO("tcp_flow_flush_lock queue len == %u\n", flow_data->tsegqe_len);

    /*cvmx_pow_tag_sw_nocheck(staging_tag(ssn->ssn_index,SESSION_STAGING),
                            CVMX_POW_TAG_TYPE_ATOMIC);*/
    //cvmx_pow_tag_sw_wait();

    while ((q = LIST_FIRST(&flow_data->head))) {
        __send_packet(q, flow_data, ssn);
    }

    /* ssn field */
    //ssn->tcp_flow_data = NULL;
    /* init data */
    flow_data->enable = 1;
    flow_data->rcv_nxt = 0;
    flow_data->tsegqe_len = 0;
    LIST_INIT(&flow_data->head);
    flow_data->rev_flow_data = NULL;

    /* negative */
    flow_data = &ssn->tcp_flow_negative_data;
    while ((q = LIST_FIRST(&flow_data->head))) {
        __send_packet(q, flow_data, ssn);
    }

    /* ssn field */
    //ssn->tcp_flow_data = NULL;
    /* init data */
    flow_data->enable = 1;
    flow_data->rcv_nxt = 0;
    flow_data->tsegqe_len = 0;
    LIST_INIT(&flow_data->head);
    flow_data->rev_flow_data = NULL;
    CVMX_SYNCWS;

    /*cvmx_pow_tag_sw_nocheck(staging_tag(ssn->ssn_index,SESSION_STAGING),
                            CVMX_POW_TAG_TYPE_ORDERED);*/
    return 0;
}

/* ack reverse queue */
int send_reverse_tcpflow(tcp_flow_data *flow_data, struct mpp_ssn *ssn, uint64_t ack_num)
{
    struct tseg_qent *q;

    //MC_PRINTF_ERR("Reverse: ack num == 0x%lx  rev_flow_data->rcv_nxt == 0x%lx\n", ack_num, flow_data->rcv_nxt);
    int i = ack_num - flow_data->rcv_nxt;

    if (i == 0) {
        while ((q = LIST_FIRST(&flow_data->head))) {
            if (q->nxt_seq <= flow_data->rcv_nxt) {
                //MC_PRINTF_ERR("Reverse: q->seq == 0x%lx\n", q->seq);
                __send_packet(q, flow_data, ssn);
            } else {
                flow_data->rcv_nxt = q->seq;
                CVMX_SYNCWS;
                break;
            }
        }
    } else {
        tcp_flow_flush(flow_data, ssn);
        return 0;
    }

    /* If list is empty, return */
    if (LIST_EMPTY(&flow_data->head)) {
        /* Just make sure, not need to init data */
        flow_data->rcv_nxt = 0;
        flow_data->tsegqe_len = 0;
        LIST_INIT(&flow_data->head);
        CVMX_SYNCWS;
        return 0;
    }

    /* If list isn't empty, continue update rcv_nxt value */
    LIST_FOREACH(q, &flow_data->head, tqe_q) {
        if (q->seq == flow_data->rcv_nxt) {
            if (SEQ_GT(q->nxt_seq, flow_data->rcv_nxt)) {
                flow_data->rcv_nxt = q->nxt_seq;
                CVMX_SYNCWS;
            }
        }
    }

    return 0;
}

int frcore_remove_chain_by_timer(struct mpp_ssn *ssn)
{
    #if 0
    struct mpp_ssn *ssn1 = &gsdata.ssn[ssn->ssn_index];

    /* positive ssn */
    if (ssn1 == ssn ) {
        /* remove  positive chain by timer */
        if (cvmx_atomic_get32(&ssn->ttl)) {
            cvmx_atomic_set32(&ssn->ttl, 0);
        }
        /* remove reverse chain by timer */
        if (ssn->next_sc) {
            if (cvmx_atomic_get32(&ssn->next_sc->ttl)) {
                cvmx_atomic_set32(&ssn->next_sc->ttl, 0);
            }
        }
    } else {
        /* remove reverse chain by timer */
        if (cvmx_atomic_get32(&ssn->ttl)) {
            cvmx_atomic_set32(&ssn->ttl, 0);
        }
        /* remove  positive chain by timer */
        if (cvmx_atomic_get32(&ssn1->ttl)) {
            cvmx_atomic_set32(&ssn1->ttl, 0);
        }
    }
#else
        if (cvmx_atomic_get32(&ssn->ttl)) {
            cvmx_atomic_set32(&ssn->ttl, 0);
        }
#endif
    return FRE_SUCCESS;
}


int
reass_tcp(cvmx_wqe_t *work,
          struct mpp_ssn *ssn, struct mpp_control *mpp,
          uint64_t sequence, uint64_t ack_num,
          uint8_t *data, uint64_t data_length,
          uint8_t th_flags, uint8_t direction)
{
    struct tseg_qent *q;
    struct tseg_qent *p = NULL;
    struct tseg_qent *nq;
    struct tseg_qent *te = NULL;
    tcp_flow_data *fwd_flow_data, *rev_flow_data;
    uint32_t ssn_seq;
    //FRCORE_CYCLE_RECORDING();
    /* for debug */
    //MC_PRINTF_ERR("Forward: sequence == 0x%lx ack_num == 0x%lx\n", sequence, ack_num);

    /*if packet seq is smaller than first packet seq ,drop it*/
    if (direction == FRC_SSN_POSIVTE_FLOW) {
        ssn_seq = ssn->seq;
    }else {
        ssn_seq = ssn->ack_seq;
    }

    if (sequence < ssn_seq) {
        frcore_work_free(work);
        FRCORE_STAT_INC(stat_ssn_drop);
        goto out;
    }

    if (direction == FRC_SSN_POSIVTE_FLOW) {
        fwd_flow_data = &ssn->tcp_flow_positive_data;
    }else {
        fwd_flow_data = &ssn->tcp_flow_negative_data;
    }
    rev_flow_data = fwd_flow_data->rev_flow_data;
    /* ACK and send reverse's pakcet received*/
    if (rev_flow_data && LIST_FIRST(&rev_flow_data->head)) {
        MC_PRINTF_INFO("send reverse tcp flow\n");
        //printf("send reverse tcp flow\n");
        send_reverse_tcpflow(rev_flow_data, ssn, ack_num);
    }

    /* purge ssn */
    if (th_flags & (TH_FIN|TH_RST)) {
        frcore_work_free(work);
        FRCORE_STAT_INC(stat_ssn_drop);
        tcp_flow_flush(fwd_flow_data, ssn);

        /* remove the chain by the timer */
        frcore_remove_chain_by_timer(ssn);
        goto out;
    }

    /* for debug */
    if (fwd_flow_data->tsegqe_len >= disorder_depth) {
        frcore_work_free(work);
        FRCORE_STAT_INC(stat_tcpflowrec_queue_overflow);
        MC_PRINTF_ERR("tcp reassemble queue len %u overflows\n", fwd_flow_data->tsegqe_len);
        return 0;
    }

    /*
     * Allocate a new queue entry. If we can't, or hit the FPA limit
     * just drop the pkt.
     *
     */
    te = (struct tseg_qent *)cvmx_fpa_alloc(CVMX_FPA_TCPFLOWREC_POOL);
    if (te == NULL) {
        MC_PRINTF_ERR("tcp reasseble alloc failed\n");
        frcore_work_free(work);
        FRCORE_STAT_INC(stat_tcpflowrec_mem_failed);
        return 0;
    } else {
        te->data_64[0] = te->data_64[1] = 0;
        te->data_64[2] = te->data_64[3] = 0;
        te->data_64[4] = te->data_64[5] = 0;
        te->data_64[6] = te->data_64[7] = 0;
        //memset(te, 0, sizeof(struct tseg_qent));
    }

    /*
     * Find a segment which begins after this one does.
     */
    LIST_FOREACH(q, &fwd_flow_data->head, tqe_q) {
       /* printf("%s %d ssn=%p fwd_flow_data->head.lh_first=%x, q->sequence=0x%x\n", __func__, __LINE__, ssn, fwd_flow_data->head.lh_first
               ,q->seq);
        printf("sip=0x%x, dip=0x%x, sp=%d, sp=%d, proto=%d\n",ssn->key.sip,
               ssn->key.dip, ssn->key.sp, ssn->key.dp, ssn->key.proto);
        printf("fwd_flow_data->tsegqe_len=%d\n", fwd_flow_data->tsegqe_len);*/
        if (SEQ_GT(q->seq, sequence))
            break;
        p = q;
    }

    /* If current packet sequence is greater than p->seq more than 64K, drop it */
    if (p != NULL) {
        if (sequence - p->seq > 64*K) {
            frcore_work_free(work);
            cvmx_fpa_free(te, CVMX_FPA_TCPFLOWREC_POOL, 0);
            FRCORE_STAT_INC(stat_ssn_drop);
            goto out;
        }
    }

    /*
     * If there is a preceding segment, it may provide some of
     * our data already.  If so, drop the data from the incoming
     * segment.  If it provides all of our data, drop us.
     */
    if (p != NULL) {
        int i;
        /* conversion to int (in i) handles seq wraparound */
        i = p->seq + p->data_len - sequence;
        if (i > 0) {
            if (i > ((int)data_length)) {
                frcore_work_free(work);
                cvmx_fpa_free(te, CVMX_FPA_TCPFLOWREC_POOL, 0);
                FRCORE_STAT_INC(stat_tcpflowrec_dup_remove);
                //md_fau_atomic_add64(CNTER_XMIT_PKTS_TO_PKO, 1);
#if 0
                int  ifidx = cvmcs_nic_find_idx(work->ipprt);
                                octnic_port_info_t  *nicport = &octnic->port[ifidx];
                                cvmx_atomic_add64(&nicport->stats.fromwire.total_tcprecdup, 1);
#endif
                return 0;
            }else if (i == ((int)data_length)) {
                cvmx_fpa_free(te, CVMX_FPA_TCPFLOWREC_POOL, 0);
                frcore_work_free(p->work);
                p->work = work;
                p->seq = sequence;
                p->ack_num = ack_num;
                p->data = data;
                p->data_len = data_length;
                FRCORE_STAT_INC(stat_tcpflowrec_retrans);
                FRCORE_STAT_INC(stat_tcpflowrec_retrans_drop);
                return 0;
            }
            data += i;
            sequence += i;
            data_length -= i;
            FRCORE_STAT_INC(stat_tcpflowrec_part_retrans);
        }
    }

    /*
     * While we overlap succeeding segments trim them or,
     * if they are completely covered, dequeue them.
     */
    while (q) {
        int i = (sequence + data_length) - q->seq;
        if (i <= 0) {
            //FRCORE_STAT_INC(stat_tcpflowrec_retrans);
            break;
        }
        if (i < ((int)q->data_len)) {
            q->seq += i;
            q->data += i;
            q->data_len -= i;
            FRCORE_STAT_INC(stat_tcpflowrec_part_retrans);
            break;
        }

        nq = LIST_NEXT(q, tqe_q);
        LIST_REMOVE(q, tqe_q);
        frcore_work_free(q->work);
        cvmx_fpa_free(q, CVMX_FPA_TCPFLOWREC_POOL, 0);
        fwd_flow_data->tsegqe_len--;
        CVMX_SYNCWS;
#if 0
        int  ifidx = cvmcs_nic_find_idx(work->ipprt);
        octnic_port_info_t  *nicport = &octnic->port[ifidx];
        cvmx_atomic_add64(&nicport->stats.fromwire.total_tcprecdup, 1);
#endif
        q = nq;

        FRCORE_STAT_INC(stat_tcpflowrec_dup_remove);
    }

    //printf("p=%p, q=%p\n", p, q);
    /* Check th's flags and update seq number */
    uint64_t nxt_seq = sequence + data_length;
    if (th_flags & TH_SYN)
        nxt_seq++;
    /*
    * When list is empty, rewrite the list's rcv_nxt
    * value.
    */
    if (LIST_EMPTY(&fwd_flow_data->head))
        fwd_flow_data->rcv_nxt = nxt_seq;

    /* if SYN, drop it */
    if (th_flags & (TH_SYN)) {
        frcore_work_free(work);
        cvmx_fpa_free(te, CVMX_FPA_TCPFLOWREC_POOL, 0);
        FRCORE_STAT_INC(stat_ssn_drop);
        goto out;
    }

    /* Insert the new segment queue entry into place. */
    te->work = work;
    te->seq = sequence;
    te->ack_num = ack_num;
    te->nxt_seq = nxt_seq;
    te->data = data;
    te->data_len = data_length;
    te->direction = direction;

    if (p == NULL) {
        LIST_INSERT_HEAD(&fwd_flow_data->head, te, tqe_q);
    } else {
        LIST_INSERT_AFTER(p, te, tqe_q);
    }
    fwd_flow_data->tsegqe_len++;
    CVMX_SYNCWS;/* must sync */

    //printf("fwd_flow_data->tsegqe_len = %d\n", fwd_flow_data->tsegqe_len);
    /*
    * If tcp segment queue overflows or received fin/rst packet,
    * flush our queue
    */
    if (fwd_flow_data->tsegqe_len >= disorder_depth ||
        (th_flags & (TH_FIN|TH_RST))) {
        if (fwd_flow_data->tsegqe_len > disorder_depth) {
            MC_PRINTF_ERR("tcp segment queue len == %u\n", fwd_flow_data->tsegqe_len);
        }
        //MC_PRINTF_ERR("tcp squeue len:%u seq:%lu nxt_seq:%lu\n",
        //              fwd_flow_data->tsegqe_len, sequence, nxt_seq);
        tcp_flow_flush(fwd_flow_data, ssn);
        goto out;
    }

    /* Inorder right packet, update rcv_nxt value */
    if (sequence == fwd_flow_data->rcv_nxt) {
        if (SEQ_GT(nxt_seq, fwd_flow_data->rcv_nxt))
            fwd_flow_data->rcv_nxt = nxt_seq;
        /*
         * Search throught all the follow frags collected to
         * update rcv_nxt field value.
         */
        q = LIST_NEXT(te, tqe_q);
        while (q) {
            if (q->seq == fwd_flow_data->rcv_nxt) {
                if (SEQ_GT(q->nxt_seq, fwd_flow_data->rcv_nxt)) {
                    fwd_flow_data->rcv_nxt = q->nxt_seq;
                    CVMX_SYNCWS;
                }
            }else{
                break;
            }
            q = LIST_NEXT(q, tqe_q);
        }
    }
 //printf("fwd_flow_data->tsegqe_len = %d\n", fwd_flow_data->tsegqe_len);
    //FRCORE_CYCLE_RECORDING();
out:
    return 0;
}

/* For tcp flow recovery */
void tcp_flow_recovery(cvmx_wqe_t *work,struct mpp_ssn *ssn,
                       struct mpp_control *mpp, uint8_t direction)
{
    uint8_t th_flags;
    struct tcphdr *th;
    struct iphdr *_iph = (struct iphdr*)(&mpp->packet[mpp->len_header]);

    //MC_PRINTF_ERR("============ tcp_flow_recovery =============\n");
    #if 0
    int  ifidx = cvmcs_nic_find_idx(work->ipprt);
    octnic_port_info_t  *nicport = &octnic->port[ifidx];
    cvmx_atomic_add64(&nicport->stats.fromwire.total_tcprec, 1);
    #endif

    /* TCP packet */
    if (_iph->protocol == 0x06) {
        th = (struct tcphdr *)(((uint8_t*)(_iph))+(_iph->ihl<<2));
        th_flags = *(((uint8_t *)(th)) + 13);
        /* add statistics */
        ssn->pkts++;
        ssn->bytes += work->len;
        /*
         * Drop the pkt whose payload eq zero and flag != (TH_SYN|TH_FIN|TH_RST)
         */
        int data_len = _iph->tot_len - ((_iph->ihl + th->doff) << 2);
        //MC_PRINTF_DEBUG("data_len:%d, th_flags:0x%x\n", data_len, th_flags);
        //printf("data_len:%d, th_flags:0x%x\n", data_len, th_flags);
        if (data_len == 0 && !(th_flags & (TH_SYN | TH_FIN | TH_RST))) {
            frcore_work_free(work);
            //md_fau_atomic_add64(CNTER_XMIT_PKTS_TO_PKO, 1);
            FRCORE_STAT_INC(stat_ssn_drop);
            return;
        }else {
            uint64_t seq = th->seq;
            uint64_t ack_num = th->ack_seq;
            uint8_t *data = ((uint8_t *)(th)) + (th->doff << 2);

            //cvmx_atomic_add64(&nicport->stats.fromwire.total_test, 1);

            reass_tcp(work, ssn, mpp, seq, ack_num,
                          data, data_len, th_flags, direction);
            return;
        }
    } else {
        MC_PRINTF_ERR("_iph->protocol == %u\n", _iph->protocol);
        frcore_work_free(work);
        //md_fau_atomic_add64(CNTER_XMIT_PKTS_TO_PKO, 1);
        FRCORE_STAT_INC(stat_ssn_drop);
        return;
    }

    return;
}

int md_dump_tcpflowrec(struct mpp_ssn *ssn)
{
    int i = 0;
    struct tseg_qent *q;
//    struct tseg_qent *p = NULL;
//    struct tseg_qent *nq;
    tcp_flow_data *fwd_flow_data;

    fwd_flow_data = &ssn->tcp_flow_positive_data;

    /*
     * Find a segment which begins after this one does.
     */
    LIST_FOREACH(q, &fwd_flow_data->head, tqe_q) {
        i++;
        printf(" index   seq  ack_num   nxt_seq   work   data   data_len\n"
               " =======================================================\n"
               " %.6d 0x%x 0x%x 0x%x %p %p %.5d\n", i, q->seq, q->ack_num, (int)q->nxt_seq,
               q->work, q->data, (int)q->data_len);
        frc_dump_buff(q->data_len, q->data);
    }

    fwd_flow_data = &ssn->tcp_flow_negative_data;

    /*
     * Find a segment which begins after this one does.
     */
    LIST_FOREACH(q, &fwd_flow_data->head, tqe_q) {
        i++;
        printf(" index   seq  ack_num   nxt_seq   work   data   data_len\n"
               " =======================================================\n"
               " %.6d 0x%x 0x%x 0x%x %p %p %.5d\n", i, q->seq, q->ack_num, (int)q->nxt_seq,
               q->work, q->data, (int)q->data_len);
        frc_dump_buff(q->data_len, q->data);
    }

    return 0;
}

#endif

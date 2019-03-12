
/***********************license start************************************
 * OCTEON SDK
 *
 * Copyright (c) 2003-2005 Cavium Networks. All rights reserved.
 *
 * This file, which is part of the OCTEON SDK from Cavium Networks,
 * contains proprietary and confidential information of Cavium Networks and
 * its suppliers.
 *
 * Any licensed reproduction, distribution, modification, or other use of
 * this file or the confidential information or patented inventions
 * embodied in this file is subject to your license agreement with Cavium
 * Networks. Unless you and Cavium Networks have agreed otherwise in
 * writing, the applicable license terms can be found at:
 * licenses/cavium-license-type2.txt
 *
 * All other use and disclosure is prohibited.
 *
 * Contact Cavium Networks at info@caviumnetworks.com for more information.
 **********************license end**************************************/

/*
 * File version info: $Id: passthrough.c 39253 2008-12-05 18:22:15Z cchavva $
 *
 */

#include <stdio.h>
#include <string.h>
#include "frcore_pkt.h"


#include "frcore_ip.h"

#include "frcore_stat.h"
#include "frcore_config.h"
#include "frc_util.h"
#include "frcore_dma.h"
#include "frcore_chan.h"
#include "frcore_tcp.h"
#include "frcore_proto.h"
#include "frcore_vlan_check.h"
#include "frcore_mac_statistics.h"
//static CVMX_SHARED uint64_t start_cycle;
//static CVMX_SHARED uint64_t stop_cycle;
#if 0
static CVMX_SHARED uint64_t now_cycle;
static CVMX_SHARED uint64_t max_cycle = 0;
#endif

extern void frcore_forward_packet_to_host(cvmx_wqe_t   *wqe);
//static int use_ipd_no_wptr = 0;
extern     uint32_t    core_id;
extern CVMX_SHARED uint8_t work_mode_oct0; /* 0 for flow rec; 1 for nic */
extern CVMX_SHARED uint8_t work_mode_oct1; /* 0 for flow rec; 1 for nic */

#if FRC_CONFIG_VLAN_CHECK
extern CVMX_SHARED int64_t vlan_id_stat[stat_vlan_id_max];
#endif
#if FRC_CONFIG_TIMESTAMP_CHECK
extern CVMX_SHARED int64_t timestamp_stat[stat_timestamp_max];
#endif
/* Note: The dump_packet routine that used to be here has been moved to
    cvmx_helper_dump_packet. */
#define FRCORE_INIT_DUMP(_func) \
{ \
    printf("%-20s ...... %s.\n", #_func, _func() ? "FAIL" : "DONE"); \
}


void frcore_cmd_core_init()
{
    unsigned int core_num = cvmx_get_core_num();
    uint64_t mask = 0;

    mask = 1 << FRC_CMD_GRP;

    cvmx_pow_set_group_mask(core_num, mask);

    FRCORE_DUMP("CMD CORE %d: MASK 0x%.16llx\n", core_num, (unsigned long long) mask);
}

void frcore_pkt_core_init()
{
    unsigned int core_num = cvmx_get_core_num();
    uint64_t mask = 0;

    mask = 1 << FRC_PKT_GRP;

    cvmx_pow_set_group_mask(core_num, mask);

    FRCORE_DUMP("PKT CORE %d: MASK 0x%.16llx\n", core_num, (unsigned long long) mask);
}

void frcore_dat_core_init()
{
    unsigned int core_num = cvmx_get_core_num();

    uint64_t mask = 0;

    mask = (1 << (core_num + 1)) | (1 << FRC_DAT_GRP) | (1 << GROUP_TO_DATA_PLANE_AGING);

    cvmx_pow_set_group_mask(core_num, mask);

    FRCORE_DUMP("DATA CORE %d: MASK 0x%.16llx\n", core_num, (unsigned long long) mask);
}

int
frcore_tim_init()
{
    if(cvmx_tim_setup(FRCORE_TIM_TICKS, FRCORE_TIM_TICKS_PSEC*3)) {
        return SPTE_FAIL;
    }

    cvmx_tim_start();

    return SPTE_SUCCESS;
}

void frcore_pkt_init()
{
    //frcore_console_init();

    FRCORE_DUMP("\n\nOcteon spt\n");

    FRCORE_DUMP("Initializing...\n");

    FRCORE_INIT_DUMP(frcore_stat_init);

    FRCORE_INIT_DUMP(frcore_tim_init);

    FRCORE_DUMP("Initialization Completed.\n");
}

void frcore_work_free(cvmx_wqe_t *work)
{
    FRCORE_STAT_INC(stat_work_free);
#if FRCORE_CFG_FREE_WORK

    cvm_free_host_instr(work);
#else
#error "Undefine FRCORE_CFG_FREE_WORK"
#endif
}


void kinds_of_pkt_len_stat(cvmx_wqe_t  *work)
{
    uint64_t len = work->len;
    if(len < 60) {
        FRCORE_STAT_INC(stat_pkts_below_64);
        FRCORE_STAT_INC(stat_very_short_pkts);
    }else if (len>=60 && len<124) {
        FRCORE_STAT_INC(stat_pkts_between_64_128);
    }else if (len>=124 && len<252) {
        FRCORE_STAT_INC(stat_pkts_between_128_256);
    }else if (len>=252 && len<508) {
        FRCORE_STAT_INC(stat_pkts_between_256_512);
    }else if (len>=508 && len<1020) {
        FRCORE_STAT_INC(stat_pkts_between_512_1024);
    }else if (len>=1020 && len<1496) {
        FRCORE_STAT_INC(stat_pkts_between_1024_1500);
    }else if (len>=1496 && len<=1596) {
        FRCORE_STAT_INC(stat_pkts_between_1500_1600);
    }else if (len>1596) {
        FRCORE_STAT_INC(stat_pkts_above_1600);
        FRCORE_STAT_INC(stat_very_long_pkts);
    }

    if (len>=60 && len<=1596) {
        FRCORE_STAT_INC(stat_normal_pkts);
    }

}
static int
frcore_send_to_pko(cvmx_wqe_t  *wqe, int port, int offload)
{
    int rv;
	cvmx_buf_ptr_t  packet_ptr;
    cvmx_pko_command_word0_t    pko_command;
	const int use_ipd_no_wptr = octeon_has_feature(OCTEON_FEATURE_NO_WPTR);

    CVMX_SYNCWS;

    /* Prepare to send a packet to PKO. */
    cvmx_pko_send_packet_prepare(port, cvmx_pko_get_base_queue(port), 1);

    /* Build a PKO pointer to this packet */


    if (wqe->word2.s.bufs == 0)
    {
    /* Packet is entirely in the WQE. Give the WQE to PKO and have it
	free it */
	    pko_command.s.total_bytes = wqe->len;
	    pko_command.s.segs = 1;
	    packet_ptr.u64 = 0;
	    if (use_ipd_no_wptr)
	    {
		packet_ptr.s.pool = CVMX_FPA_PACKET_POOL;
		packet_ptr.s.size = CVMX_FPA_PACKET_POOL_SIZE;
	    }
	    else
	    {
		packet_ptr.s.pool = CVMX_FPA_WQE_POOL;
		packet_ptr.s.size = CVMX_FPA_WQE_POOL_SIZE;
	    }
	    packet_ptr.s.addr = cvmx_ptr_to_phys(wqe->packet_data);
	    if (cvmx_likely(!wqe->word2.s.not_IP))
	    {
		/* The beginning of the packet moves for IP packets */
		if (wqe->word2.s.is_v6)
		    packet_ptr.s.addr += 2;
		else
		    packet_ptr.s.addr += 6;
	    }
    }
    else
    {
	    pko_command.s.total_bytes = wqe->len;
	    pko_command.s.segs = wqe->word2.s.bufs;
	    packet_ptr = wqe->packet_ptr;
	    if (!use_ipd_no_wptr)
		cvmx_fpa_free(wqe, CVMX_FPA_WQE_POOL, 0);
    }

    /* Send a packet to PKO. */
    rv = cvmx_pko_send_packet_finish(port, cvmx_pko_get_base_queue(port),
                                pko_command, packet_ptr, 1);

    if (rv) {
	    cvmx_helper_free_packet_data(wqe);
        FRCORE_ERROR("RV %d.\n", rv);
    }

    //cvmx_fpa_free(wqe, CVMX_FPA_WQE_POOL, 0);

    return rv;
}

void mac_print(uint8_t *ether_ptr)
{
	uint64_t dst = 0;
	uint64_t src = 0;
	dst = *(uint64_t *)ether_ptr;
	dst = dst >> 16;
	src = *(uint64_t *)(ether_ptr + 6);
	src = src >> 16;
	printf("+++++++++++ dst = 0x%lx  src = 0x%lx +++++++++++\n", dst, src);
}

static inline void swap_mac_addr(uint64_t pkt_ptr)

{

    uint16_t s;

	uint32_t w;

    /* assuming an IP/IPV6 pkt i.e. L2 header is 2 byte aligned, 4 byte non-aligned */

	s = *(uint16_t*)pkt_ptr;
	w = *(uint32_t*)(pkt_ptr+2);
	*(uint16_t*)pkt_ptr = *(uint16_t*)(pkt_ptr+6);
	*(uint32_t*)(pkt_ptr+2) = *(uint32_t*)(pkt_ptr+8);
	*(uint16_t*)(pkt_ptr+6) = s;
	*(uint32_t*)(pkt_ptr+8) = w;
}
#if FRC_CONFIG_MAC_STATISTICS
int frcore_pkt_sendback(cvmx_wqe_t *work)
{
    uint8_t port_num = work->ipprt;
    uint8_t *ether_ptr;
	const int use_ipd_no_wptr = octeon_has_feature(OCTEON_FEATURE_NO_WPTR);
	
    if ((work->grp == FRC_PKT_GRP) || (work->grp == FRC_CMD_GRP))
	{
		cvmx_helper_free_packet_data(work);
        if (use_ipd_no_wptr)
            cvmx_fpa_free(work, CVMX_FPA_PACKET_POOL, 0);
        else
            cvmx_fpa_free(work, CVMX_FPA_WQE_POOL, 0);
	}

    if (work->word2.s.bufs == 0)
    {
        ether_ptr = work->packet_data;
    } else {
        ether_ptr = cvmx_phys_to_ptr(work->packet_ptr.s.addr);
    }
    //check if heart beat packet
    if ((*(uint16_t *)(ether_ptr + 12)) != 0x88a8)
    {
        //pkt_dump(ether_ptr, work->len);
    
        frcore_mac_statistics_counter_inc(ether_ptr);
   
        swap_mac_addr((uint64_t)ether_ptr);
        frcore_forward_packet_to_wire(work, port_num, 0);
	FRCORE_STAT_INC(stat_swap_mac_send_pkts); 
        return FRCORE_ACT_UNFREE;
    }
    else
    {
    	FRCORE_STAT_INC(stat_heart_beat_pkt);
        if (heart_beat == HEART_BEAT_ON_WITH_DROP)
        {
            //printf("macccccccccccccccccc ====== %02x:%02x:%02x:%02x:%02x:%02x\n", ether_ptr[0],ether_ptr[1],ether_ptr[2],ether_ptr[3],ether_ptr[4], ether_ptr[5]);
            if (((*(ether_ptr + 5)) % 64 != 0))
            {
    		FRCORE_STAT_INC(stat_heart_beat_send_back_pkt);
               // printf("on......... %02x:%02x:%02x:%02x:%02x:%02x\n", ether_ptr[0],ether_ptr[1],ether_ptr[2],ether_ptr[3],ether_ptr[4], ether_ptr[5]);
                swap_mac_addr((uint64_t)ether_ptr);
                frcore_forward_packet_to_wire(work, port_num, 0);
                return FRCORE_ACT_UNFREE;
            }
            else
            {
    		FRCORE_STAT_INC(stat_heart_beat_on_drop_pkt);
                //printf("drop....... %02x:%02x:%02x:%02x:%02x:%02x\n", ether_ptr[0],ether_ptr[1],ether_ptr[2],ether_ptr[3],ether_ptr[4], ether_ptr[5]);
                return FRCORE_ACT_DROP;
            }
        }
        else if (heart_beat == HEART_BEAT_ON)
        {
    		FRCORE_STAT_INC(stat_heart_beat_send_back_pkt);
                //printf("on......... %02x:%02x:%02x:%02x:%02x:%02x\n", ether_ptr[0],ether_ptr[1],ether_ptr[2],ether_ptr[3],ether_ptr[4], ether_ptr[5]);
            swap_mac_addr((uint64_t)ether_ptr);
            frcore_forward_packet_to_wire(work, port_num, 0);
            return FRCORE_ACT_UNFREE;
        }
        else
        {
    		FRCORE_STAT_INC(stat_heart_beat_drop_pkt);
                //printf("drop .... %02x:%02x:%02x:%02x:%02x:%02x\n", ether_ptr[0],ether_ptr[1],ether_ptr[2],ether_ptr[3],ether_ptr[4], ether_ptr[5]);
            return FRCORE_ACT_DROP;
        }
    }
}
#endif

CVMX_SHARED uint64_t rx_pkts = 0;
inline int frcore_pkt_work_process(cvmx_wqe_t *work)
{
    uint8_t *ip_ptr, *ether_ptr;
    int rv;
    uint64_t smac = 0, dmac = 0;
    uint8_t *p;
    uint16_t type = 0;

    kinds_of_pkt_len_stat(work);

    if(!octnic->port[work->ipprt].rx_on) {
       FRCORE_DROP(stat_drop_port_off);
    }

    FRCORE_PKT("=============== rx_pkts %lld.\n", (ULL) ++rx_pkts);

    //FRCORE_CYCLE_RECORDING();

    if (work->word2.s.rcv_error) {
        FRCORE_STAT_INC(stat_rx_errs);
        FRCORE_DROP(stat_drop_rx_error);
    }

    FRCORE_STAT_INC(stat_rx_pkts);
    FRCORE_STAT_ADD(stat_rx_bytes, (work->len + 4));

    #if FRC_CONFIG_VLAN_CHECK
    FRCORE_PORT_VLAN_STAT_INC(work->ipprt, xe0_stat_rxx_pkts);
    FRCORE_PORT_VLAN_STAT_ADD(work->ipprt, xe0_stat_rxx_bytes, (work->len + 4));
    #endif
#if FRC_CONFIG_MAC_STATISTICS
    return frcore_pkt_sendback(work);
#endif


    if (work->word2.s.bufs == 0)
    {
        printf("%s work->word2.s.bufs == 0\n", __func__);
        ip_ptr = work->packet_data + work->word2.s.ip_offset + 6;
        ether_ptr = work->packet_data;
    } else {
        ether_ptr = cvmx_phys_to_ptr(work->packet_ptr.s.addr);
        ip_ptr = cvmx_phys_to_ptr(work->packet_ptr.s.addr + work->word2.s.ip_offset);
    }

    /* Drop not ip packet. */
    if (work->word2.s.not_IP) {
        FRCORE_STAT_INC(stat_not_ip);
        /* mpls packet */
        /* Drop mpls packet */
        UP16(ether_ptr + 12, type);
        if (type == ETH_P_MPLS_UC || type == ETH_P_MPLS_MC) {
            FRCORE_STAT_INC(stat_mpls);
        }
        /* Drop pppoe packet */
        if (type == ETH_P_PPP_DISC || type == ETH_P_PPP_SES) {
            FRCORE_STAT_INC(stat_pppoe);
        }
        FRCORE_DROP(stat_drop_not_ip);
    }



    p = ether_ptr;
    //frc_dump_buff(12, p);
    //FRCORE_CYCLE_RECORDING();
    memcpy(&dmac, ether_ptr + 0, 6);
    memcpy(&smac, ether_ptr + 6, 6);
    swap_buff(1, &dmac);
    swap_buff(1, &smac);
    //FRCORE_CYCLE_RECORDING();
    FRCORE_PKT("smac 0x%llx, dmac 0x%llx\n", (ULL) smac, (ULL) dmac);

    //FRCORE_CYCLE_RECORDING();
    /* Get ip header offset ponter. */
    //ip_ptr = cvmx_phys_to_ptr(buffer_ptr.s.addr + work->word2.s.ip_offset);
#if FRC_DEBUG_PKT_LEN
#error "Defined FRC_DEBUG_PKT_LEN"
    if (work->len != 1514)
    {
        FRC_DEBUG(PKT_LEN, "work->word2.s.bufs %d\n", work->word2.s.bufs);
        frc_dump_buff(work->len, ether_ptr);
    }
#endif
    rv = frcore_ip_process(work, ether_ptr, ip_ptr, smac, dmac);

    return rv;
}
#if FRCORE_CFG_DUMP_CYCLE
frcore_cycle_rec_t frcore_cycle_recoder;
#endif

inline int frcore_dat_work_process(cvmx_wqe_t *work)
{
    int rv;
#if FRC_CONFIG_IPC
    rv = frcore_ipc_work_process(work);
#else
    rv = frcore_pkt_work_process(work);
#endif
    return rv;
}

void frcore_work_process(cvmx_wqe_t *work)
{
    int rv;
    //FRCORE_STAT_INC(stat_rx_work);
    //FRCORE_WQE("work->unused = %d, work->ipprt = %d.\n", work->unused, work->ipprt);

    switch (work->unused) {
    case FRCORE_WORK_PKT:
        {
            //FRCORE_CYCLE_RECORDER_INIT();
            octnic_port_info_t  *nicport = &octnic->port[work->ipprt];
            FRCORE_STAT_INC((stat_core0_wqe + core_id));
            cvmx_atomic_add64(&nicport->stats.fromwire.total_rcvd, 1);
            cvmx_atomic_add64(&nicport->stats.fromwire.total_bytes, work->len);
            FRCORE_STAT_INC(stat_work_data);
     
            if (work->ipprt == 0) {
                FRCORE_STAT_INC(stat_p0_rx_pkts);
                if (!work_mode_oct0) {
                    rv = frcore_dat_work_process(work);
                }else {
                #ifdef ENABLE_NIC_PEER_TO_PEER
                    frcore_forward_pkt_to_ep(work);
                #else
                    frcore_forward_packet_to_host(work);
                #endif
                    return;
                }
            }else {
                FRCORE_STAT_INC(stat_p1_rx_pkts);
                if (!work_mode_oct1) {
                    rv = frcore_dat_work_process(work);
                }else {
                #ifdef ENABLE_NIC_PEER_TO_PEER
                    frcore_forward_pkt_to_ep(work);
                #else
                    frcore_forward_packet_to_host(work);
                #endif
                    return;
                }
            }
            FRCORE_WQE("frcore_pkt_work_process return %d.\n", rv);
            //FRCORE_CYCLE_RECORDING();
            //FRCORE_CYCLE_RECORD_DUMP();
            break;
        }
#if FRCORE_CONFIG_STAT_WQE
    case FRCORE_WORK_STAT:
        FRCORE_STAT_INC(stat_work_stat);
        rv = frcore_stat_work_process(work);
        FRCORE_WQE("frcore_stat_work_process return %d.\n", rv);
        break;
#endif

#if FRC_CONFIG_DMA_TEST
    case FRCORE_WORK_DMA_LOOP:
        rv = frcore_dma_loop(work);
        FRCORE_WQE("frcore_dma_loop return %d.\n", rv);
        break;
#endif
#if FRC_CONFIG_SIMPLE_PACKAGE || FRC_CONFIG_SSN_CHAN
    case FRCORE_WORK_CHAN:
        FRCORE_STAT_INC(stat_chan_timer);
        rv = frcore_chan_process(work);
        FRCORE_WQE("frcore_chan_process return %d.\n", rv);
        break;
#endif
#if FRC_CONFIG_SIMPLE_PACKET_TEST || FRC_CONFIG_SSN_CHAN_TEST
    case FRCORE_WORK_CHAN_TEST:
        rv = frcore_chan_test_process(work);
        FRCORE_WQE("frcore_chan_process return %d.\n", rv);
        break;
#endif

    case FRCORE_WORK_SSN_AGE:
#if FRC_CONFIG_AGE
        FRCORE_STAT_INC(stat_ssn_age_wqe);
        rv = frcore_ssn_age_process(work);
#else
        rv = FRCORE_ACT_UNFREE;
#endif
        FRCORE_WQE("frcore_chan_process return %d.\n", rv);
        break;

    default:
        FRCORE_STAT_INC(stat_work_unkown);
        rv = FRCORE_ACT_DROP;
        FRCORE_WQE("work_unkown return %d.\n", rv);
        break;
    }

    switch (rv) {
    case FRCORE_ACT_DEBUG:
        FRCORE_STAT_INC(stat_act_debug);
        break;
    case FRCORE_ACT_FREE:
        FRCORE_STAT_INC(stat_act_free);
        frcore_work_free(work);
        break;
    case FRCORE_ACT_DROP:
        FRCORE_STAT_INC(stat_act_drop);
    #if FRCORE_CONFIG_TX_DROP
    #error "defined FRCORE_CONFIG_TX_DROP"
        frcore_tx_work(work);
    #else
        frcore_work_free(work);
    #endif
        break;
    case FRCORE_ACT_FORWARD:
        FRCORE_STAT_INC(stat_act_forward);
        //frcore_tx_work(work);
        frcore_work_free(work);
        FRCORE_TEST("\n");
        break;
    case FRCORE_ACT_DELAY:
        FRCORE_STAT_INC(stat_act_delay);
        break;
    case FRCORE_ACT_UNFREE:
        FRCORE_STAT_INC(stat_act_unfree);
        break;
    default:
        FRCORE_STAT_INC(stat_act_unkown);
        frcore_work_free(work);
        break;
    }

}

typedef struct
{
    cvmx_pip_port_status_t  input_statistics;
    cvmx_helper_interface_mode_t imode;
    int                     display;
    uint64_t                link_state;
} rx_port_state_t;


/**
 * This is the main reptitive update function. It must be
 * called at least once per second for stuff to work. It can
 * be called more often without any ill effects. The more
 * often, the better.
 */

#define FRCORE_RESET_TIME  10000


void frcore_state_dump()
{
    uint64_t next_update     = 0;
    uint64_t update_cycle    = 0;
    uint64_t next_dump       = 0;
    uint64_t dump_cycle      = 0;
    uint64_t cycle;
    int dump_cnt = FRCORE_RESET_TIME;
    uint64_t cpu_clock_hz = 0;

    if (cvmx_sysinfo_get()->board_type == CVMX_BOARD_TYPE_SIM) {
        cpu_clock_hz = FRCORE_SIM_CLOCK_HZ;
        update_cycle = cpu_clock_hz * 5;
        dump_cycle   = cpu_clock_hz;
    } else {
        cpu_clock_hz = cvmx_sysinfo_get()->cpu_clock_hz;
        update_cycle = cpu_clock_hz;
        dump_cycle   = cpu_clock_hz * 5;
    }

    cycle = cvmx_get_cycle();
    next_update += cycle + update_cycle;
    next_dump   += cycle + dump_cycle;

    FRCORE_WQE("cycle %lld next_update %lld next_dump %lld\n",
                  (unsigned long long) cycle,
                  (unsigned long long) next_update,
                  (unsigned long long) next_dump);

    while (dump_cnt) {
        cycle = cvmx_get_cycle();
        if (cycle >= next_update)
        {
            //FRCORE_DEBUG("cycle %lld next_update %lld\n",
            //             (unsigned long long) cycle, (unsigned long long) next_update);
            next_update = cycle + update_cycle;
            //update_rgmii_speed();
        }

        if (cycle >= next_dump)
        {
            //FRCORE_DEBUG("cycle %lld next_dump %lld\n",
            //             (unsigned long long) cycle, (unsigned long long) next_dump);
            FRCORE_DUMP("UPDATE CNT %d:\n", dump_cnt--);
            frcore_stat_dump();
            next_dump = cvmx_get_cycle() + dump_cycle;
        }
    }
    printf("=== System reset now ===\n");

    cvmx_reset_octeon();
}

void ip_string(uint32_t ip, char str[IP_STR_LEN])
{
    uint8_t *p = (uint8_t *)&ip;
    memset(str, 0, IP_STR_LEN);
    sprintf(str, "%u.%u.%u.%u", p[0], p[1], p[2], p[3]);
    str[IP_STR_LEN - 1] = 0;
}




/* Note: The dump_packet routine that used to be here has been moved to
    cvmx_helper_dump_packet. */
//#define DUMP_PACKETS 1
#define SWAP_MAC_ADDR
#define TCP_UDP_ONLY
#define DUMP_PKT_INFO


/**
 * Main entry point
 *
 * @return exit code
 */
int frcore_pkt_main()
{
    cvmx_sysinfo_t *sysinfo;
    unsigned int coremask_frcore;
    int result = 0;
    unsigned int core_num;
    sysinfo = cvmx_sysinfo_get();
    coremask_frcore = sysinfo->core_mask;
    /*
     * elect a core to perform boot initializations, as only one core needs to
     * perform this function.
     *
     */
    if (cvmx_coremask_first_core(coremask_frcore)) {
        printf("Version: %s\n", cvmx_helper_get_version());
        frcore_pkt_init();
    }
    cvmx_coremask_barrier_sync(coremask_frcore);

    core_num = cvmx_get_core_num();
    if (core_num < FRC_DAT_CORE_MAX) {
        frcore_dat_core_init();
    } else if (core_num < FRC_PKT_CORE_MAX){
        frcore_pkt_core_init();
    } else {
        frcore_cmd_core_init();
    }

    cvmx_coremask_barrier_sync(coremask_frcore);
    if (cvmx_coremask_first_core(coremask_frcore)) {
        //use_ipd_no_wptr = octeon_has_feature(OCTEON_FEATURE_NO_WPTR);
        //frcore_config_dump();
        //frcore_memory_dump();
    }
    cvmx_coremask_barrier_sync(coremask_frcore);

    return result;
}



/* End of file */

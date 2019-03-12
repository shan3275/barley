/*
 *
 * OCTEON SDK
 *
 * Copyright (c) 2010 Cavium Networks. All rights reserved.
 *
 * This file, which is part of the OCTEON SDK which also includes the
 * OCTEON SDK Package from Cavium Networks, contains proprietary and
 * confidential information of Cavium Networks and in some cases its
 * suppliers.
 *
 * Any licensed reproduction, distribution, modification, or other use of
 * this file or the confidential information or patented inventions
 * embodied in this file is subject to your license agreement with Cavium
 * Networks. Unless you and Cavium Networks have agreed otherwise in
 * writing, the applicable license terms "OCTEON SDK License Type 5" can be found
 * under the directory: $OCTEON_ROOT/components/driver/licenses/
 *
 * All other use and disclosure is prohibited.
 *
 * Contact Cavium Networks at info@caviumnetworks.com for more information.
 *
 */

#include "frcore.h"
#include "frcore_cmd.h"
#include "frcore_ipc.h"
#include "frcore_misc.h"
#include "frcore_phy.h"
#include "frcore_dma.h"
#include "frcore_chan.h"
#include "frcore_pkt.h"
#include "frcore_stat.h"
#include "frcore_fr.h"
#include "frcore_bmm.h"
#include "frcore_rule.h"
#include "frcore_init.h"

/* Enable this flag if you want to test peer-to-peer communication.
   Packets received on the GMX ports will be forwarded to the peer to be sent
   out from the same GMX port (e.g. pkt arriving from port 16 on a 56xx will
   be forwarded to the peer which will send it out on its port 16).
*/
//#define  ENABLE_NIC_PEER_TO_PEER


#ifdef ENABLE_NIC_PEER_TO_PEER
#include "cn56xx_ep_comm.h"
#endif


extern uint32_t  core_id;

CVMX_SHARED uint32_t wqe_count[MAX_CORES] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};

CVMX_SHARED octnic_dev_t  *octnic;

extern CVMX_SHARED  uint64_t            cpu_freq;
extern CVMX_SHARED uint8_t work_mode_oct0; /* 0 for flow rec; 1 for nic */
extern CVMX_SHARED uint8_t work_mode_oct1; /* 0 for flow rec; 1 for nic */
#ifdef __linux__
void (*prev_sig_handler)(int);
#endif

//#define FRCORE_PAKCETS     ((cvmx_fau_reg_64_t)(CVMX_FAU_REG_AVAIL_BASE + 0))   /**< Fetch and add for counting packets processed */
//#define FRCORE_ERRORS      ((cvmx_fau_reg_64_t)(CVMX_FAU_REG_AVAIL_BASE + 8))   /**< Fetch and add for counting detected errors */
//#define FRCORE_OUTSTANDING ((cvmx_fau_reg_64_t)(CVMX_FAU_REG_AVAIL_BASE + 16))  /**< Fetch and add for counting outstanding packets */

void
cvmcs_print_compile_options()
{
    printf("Application compiled with: ");
#ifdef  CVMCS_DUTY_CYCLE
    printf("[ DUTY CYCLE ] ");
#endif
#ifdef ENABLE_NIC_PEER_TO_PEER
    printf("[ PEER TO PEER ] ");
#endif
}

static inline void
frcore_print_stats(void)
{
    int i;
    oct_link_stats_t  *st;
    uint64_t packet_num;
    uint64_t bytes_num;
    uint64_t ass; /* average segment size */

    printf("-- RGMII driver stats --\n");
    printf(" (Rx pkts from wire; Tx pkts from host)\n");
    for(i = 0; i < MAX_OCTEON_ETH_PORTS; i++) {
        if(!frcore_port_active(i))
            continue;

        printf("Port%d: ", i);
        st = &octnic->port[i].stats;
        packet_num = st->fromwire.total_rcvd - st->fromwire.last_total_rcvd;
        bytes_num = st->fromwire.total_bytes - st->fromwire.last_total_bytes;
        if(packet_num) {
            ass = bytes_num / packet_num;
        }else {
            ass = 0;
        }

        printf(" Rx : %llu ", cast64(st->fromwire.total_rcvd));
        printf("(bps %llu) ", cast64((bytes_num * 8)) );
        printf("(pps %llu) ", cast64(packet_num) );
        printf("(ass %llu) ", cast64(ass));
        if(st->fromwire.err_pko)
            printf("(%llu PKO Err) ", cast64(st->fromwire.err_pko));
        if(st->fromwire.err_link)
            printf("(%llu Link Err) ", cast64(st->fromwire.err_link));
        if(st->fromwire.err_drop)
            printf("(%llu Drops) ", cast64(st->fromwire.err_drop));

        packet_num = st->fromhost.total_rcvd - st->fromhost.last_total_rcvd;
        bytes_num = st->fromhost.total_bytes - st->fromhost.last_total_bytes;
        if(packet_num) {
            ass = bytes_num / packet_num;
        }else {
            ass = 0;
        }
        printf(" Tx : %llu ", cast64(st->fromhost.total_rcvd));
        printf("(bps %llu) ", cast64((bytes_num * 8)));
        printf("(pps %llu) ", cast64(packet_num));
        printf("(ass %llu) ", cast64(ass));
        if(st->fromhost.err_pko)
            printf("(%llu PKO Err) ", cast64(st->fromhost.err_pko));
        if(st->fromhost.err_link)
            printf("(%llu Link Err) ", cast64(st->fromhost.err_link));
        if(st->fromhost.err_drop)
            printf("(%llu Drops) ", cast64(st->fromhost.err_drop));
        printf("\n");
    }
}

/** Setup the FPA pools. The Octeon hardware, simple executive and
  * PCI core driver use  WQE and Packet pool. OQ pool is used to
  * allocate command buffers for Output queue by simple exec.
  * Test pool is used by this application.
  */
int
frcore_setup_memory()
{

   /* cvmx_fpa_enable();

    if( cvmcs_app_mem_alloc("Packet Buffers", CVMX_FPA_PACKET_POOL,
                             CVMX_FPA_PACKET_POOL_SIZE, FPA_PACKET_POOL_COUNT))
        return 1;

    if( cvmcs_app_mem_alloc("Work Queue Entries", CVMX_FPA_WQE_POOL,
                             CVMX_FPA_WQE_POOL_SIZE, FPA_WQE_POOL_COUNT))
        return 1;


    if( cvmcs_app_mem_alloc("PKO Command Buffers",CVMX_FPA_OUTPUT_BUFFER_POOL,
                         CVMX_FPA_OUTPUT_BUFFER_POOL_SIZE, FPA_OQ_POOL_COUNT))
        return 1;

    if( cvmcs_app_mem_alloc("TIMER buffer",CVMX_FPA_TIMER_POOL,
                         CVMX_FPA_TIMER_POOL_SIZE, FPA_TIMER_POOL_COUNT))
        return 1;*/
    if(cvmx_helper_initialize_fpa(FPA_PACKET_POOL_COUNT, FPA_WQE_POOL_COUNT,
                                  FPA_OQ_POOL_COUNT, FPA_TIMER_POOL_COUNT, 0)) {
        return 1;
    }
    printf("FPA_PACKET_POOL_COUNT=0x%x\n", FPA_PACKET_POOL_COUNT);
    cvm_common_fpa_display_all_pool_info();
    return 0;
}






/** Global initialization. Performed by the boot core only. */
int
frcore_init_global()
{
    if(frcore_setup_memory())
        return 1;

    if(cvmcs_app_init_global())
        return 1;

    cvmx_helper_ipd_and_packet_input_enable();

    /* Enable IPD only after sending the START indication packet to host. */
    cvmx_ipd_disable();

    return frcore_setup_interfaces();

}





/** Local initialization. Performed by all cores. */
int
frcore_init_local()
{
    if(cvmcs_app_init_local())
        return 1;

    CVMX_SYNCW;
    return 0;
}





#ifdef ENABLE_NIC_PEER_TO_PEER
int
frcore_forward_pkt_to_ep(cvmx_wqe_t  *wqe)
{
    cn56xx_ep_packet_t  pkt;
    int                 retval;

    cvmx_atomic_add64(&octnic->port[wqe->ipprt].stats.fromwire.total_rcvd, 1);
    cvmx_atomic_add64(&octnic->port[wqe->ipprt].stats.fromwire.total_bytes, wqe->len);

    memset(&pkt, 0, sizeof(cn56xx_ep_packet_t));

    pkt.bufcount = 1;

    pkt.buf[0].s.addr = wqe->packet_ptr.s.addr;
    pkt.buf[0].s.size = wqe->len;
    pkt.buf[0].s.pool = CVMX_FPA_PACKET_POOL;
    pkt.buf[0].s.i    = 1;

    pkt.tag     = 0x11001100;
    pkt.tagtype = CVMX_POW_TAG_TYPE_ORDERED;
    pkt.param   = wqe->ipprt;
    pkt.opcode  = EP_TO_EP_OP;

    DBG("Sending test packet with opcode: %x param: %x\n", pkt.opcode, pkt.param);


    retval = cn56xx_send_ep_packet(&pkt);

    /* If packet was sent successfully, the packet buffers would be freed by the
       core driver EP communication code. Else we need to free it here. */
    if(retval == 0) {
        wqe->word2.s.bufs   = 0;
        wqe->packet_ptr.u64 = 0;
        cvmx_atomic_add64(&octnic->port[wqe->ipprt].stats.fromwire.total_fwd, 1);
    } else {
        cvmx_atomic_add64(&octnic->port[wqe->ipprt].stats.fromwire.err_drop, 1);
    }

    cvm_free_host_instr(wqe);

    return retval;
}
#endif






static int
frcore_send_to_pko(cvmx_wqe_t  *wqe, int port, int offload)
{
    int rv;
    cvmx_pko_command_word0_t    pko_command;
    CVMX_SYNCWS;
    /* Prepare to send a packet to PKO. */
    cvmx_pko_send_packet_prepare(port, cvmx_pko_get_base_queue(port), 1);

    /* Build a PKO pointer to this packet */
    pko_command.u64           = 0;
    /* Setting II = 0 and DF = 1 will free all buffers whose I bit is set. */
    pko_command.s.ignore_i    = 0;
    pko_command.s.dontfree    = 0;
    pko_command.s.segs        = wqe->word2.s.bufs;
    pko_command.s.total_bytes = wqe->len;
    if(offload)
        pko_command.s.ipoffp1 = offload;

    DBG_PRINT(DBG_FLOW,"pko cmd: %016llx lptr: %016llx PORT: %d Q: %d\n",
              cast64(pko_command.u64), cast64(wqe->packet_ptr.u64), port,
              cvmx_pko_get_base_queue(port));

    /* Send a packet to PKO. */
    rv = cvmx_pko_send_packet_finish(port, cvmx_pko_get_base_queue(port),
                                pko_command, wqe->packet_ptr, 1);
    if (rv) {
        FRCORE_ERROR("RV %d.\n", rv);
    }

    return rv;
}

void
frcore_forward_packet_to_host(cvmx_wqe_t   *wqe)
{
    cvmx_resp_hdr_t     *resp_ptr;
    cvmx_buf_ptr_t      *lptr;
    octnic_port_info_t  *nicport = &octnic->port[wqe->ipprt];

    //frcore_dump_ptrs(&wqe->packet_ptr, wqe->len);
    cvmx_atomic_add64(&nicport->stats.fromwire.total_rcvd, 1);
    cvmx_atomic_add64(&nicport->stats.fromwire.total_bytes, wqe->len);

    if(!octnic->port[wqe->ipprt].rx_on) {
        cvm_free_wqe_and_pkt_bufs(wqe);
        //FRCORE_STAT_INC(stat_p1_rx_dropped);
        return;
    }

    wqe->word2.s.bufs += 1;
    wqe->len          += CVMX_RESP_HDR_SIZE;

    /* We use the space in WQE starting at byte offset 40 for the response
       header information. This allows the WQE to be used for data and let
       PKO free the WQE buffer. */
    lptr      = (cvmx_buf_ptr_t *)&wqe->packet_data[40];
    lptr->u64 = wqe->packet_ptr.u64;

    resp_ptr = (cvmx_resp_hdr_t  *)&wqe->packet_data[48];
    resp_ptr->u64         = 0;
    resp_ptr->s.opcode    = CORE_NW_DATA_OP;
    resp_ptr->s.destqport = nicport->ifidx;

    /* lptr is reused */
    lptr         = &wqe->packet_ptr;
    lptr->u64    = 0;
    lptr->s.addr = CVM_DRV_GET_PHYS(resp_ptr);
    lptr->s.size = 8;
    lptr->s.pool = CVMX_FPA_WQE_POOL;

    FRCORE_PKT("wqe @ %p wqe bufs: %d len: %d pkt_ptr @ %llx\n", wqe,
            wqe->word2.s.bufs, wqe->len, (ULL) wqe->packet_ptr.s.addr);

    if(!frcore_send_to_pko(wqe, nicport->linfo.pciport, 0)) {
        //FRCORE_STAT_INC(stat_p1_rx_pko);
        cvmx_atomic_add64(&nicport->stats.fromwire.total_fwd, 1);
        return;
    }

    //FRCORE_STAT_INC(stat_p1_rx_dropped);
    cvmx_atomic_add64(&nicport->stats.fromwire.err_pko, 1);
    cvm_free_wqe_and_pkt_bufs(wqe);
    return;
}


void
frcore_forward_packet_to_wire(cvmx_wqe_t   *wqe, int p_num, int offload)
{
    uint64_t             nextptr;
    octnic_port_info_t  *nicport = &octnic->port[p_num];

    cvmx_atomic_add64(&nicport->stats.fromhost.total_rcvd, 1);
    cvmx_atomic_add64(&nicport->stats.fromhost.total_bytes, wqe->len);

    if(cvmx_unlikely(nicport->linfo.link.s.status == 0) ) {
        cvmx_atomic_add64(&nicport->stats.fromhost.err_link, 1);
        cvm_free_wqe_and_pkt_bufs(wqe);
        return;
    }

//	cvmx_helper_dump_packet(wqe);
#ifndef FRC_CONFIG_MAC_STATISTICS
    nextptr = *((uint64_t *)CVM_DRV_GET_PTR(wqe->packet_ptr.s.addr - 8));
    wqe->packet_ptr.s.addr += CVM_RAW_FRONT_SIZE;
    wqe->packet_ptr.s.size -= CVM_RAW_FRONT_SIZE;
    wqe->len               -= CVM_RAW_FRONT_SIZE;
    *((uint64_t *)CVM_DRV_GET_PTR(wqe->packet_ptr.s.addr - 8)) = nextptr;
#endif

    if(!frcore_send_to_pko(wqe, p_num, offload)) {
        if (p_num == 0)
        {
            FRCORE_STAT_INC(stat_p0_tx_pkts);
        }
        else
        {
            FRCORE_STAT_INC(stat_p1_tx_pkts);
        }
        cvmx_atomic_add64(&nicport->stats.fromhost.total_fwd, 1);
//	cvmx_helper_dump_packet(wqe);
        cvm_drv_fpa_free(wqe, CVMX_FPA_WQE_POOL, 0);
        return;
    }
    cvmx_atomic_add64(&nicport->stats.fromwire.err_pko, 1);
    cvm_free_wqe_and_pkt_bufs(wqe);
}






/** All work received by the application is forwarded to this routine.
  * All RAW packets with opcode=0x1234 and param=0x10 are test instructions
  * and handle by the application. All other RAW packets with opcode in
  * the range 0x1000-0x1FFF is given to the core driver. All other packets
  * are dropped.
  */
static inline int
frcore_process_wqe(cvmx_wqe_t  *wqe)
{
    cvmx_raw_inst_front_t     *front;
    uint32_t                   opcode;
    //octnic_port_info_t  *nicport = &octnic->port[wqe->ipprt];

    front = (cvmx_raw_inst_front_t *)wqe->packet_data;
/*
    DBG("Received WQE @ %p ipprt: %d bufs: %d len: %d opcode: %x qe->word2.s.software: %d.\n",
           wqe, wqe->ipprt, wqe->word2.s.bufs, wqe->len, front->irh.s.opcode, wqe->word2.s.software);
*/


    if(wqe->grp == GROUP_TO_DATA_PLANE_AGING) {
        if(frcore_port_active(wqe->ipprt)) {
            frcore_work_process(wqe);
        }
    }else {
        if(wqe->word2.s.software)   {

            front = (cvmx_raw_inst_front_t *)wqe->packet_data;
            opcode = front->irh.s.opcode;

            switch(opcode) {

    #ifdef ENABLE_NIC_PEER_TO_PEER
            case EP_TO_EP_OP:
                frcore_forward_packet_to_wire(wqe, front->irh.s.param);
                break;
    #endif

            case OCT_NW_PKT_OP:
                FRCORE_STAT_INC(stat_work_pkt);
                frcore_forward_packet_to_wire(wqe, front->irh.s.param,
                                                 front->irh.s.rlenssz);
                break;
            case OCT_NW_CMD_OP:
                FRCORE_STAT_INC(stat_work_cmd);
                frcore_process_nic_cmd(wqe);
                break;
            case HOST_NW_INFO_OP:
                FRCORE_STAT_INC(stat_work_cmd);
                frcore_send_link_info(wqe);
                break;
            case FRC_CMD_REQUEST_OP:
                FRCORE_STAT_INC(stat_work_cmd);
                frcore_process_frc_cmd(wqe);
                break;
            default:
                FRCORE_STAT_INC(stat_work_instr);
                if(opcode >= DRIVER_OP_START && opcode <= DRIVER_OP_END) {
                    cvm_drv_process_instr(wqe);
                } else {
                    cvm_free_host_instr(wqe);
                }
            } /* switch */

        } else  {

            if(frcore_port_active(wqe->ipprt)) {

                if(cvmx_likely(!wqe->word2.snoip.rcv_error)) 
                {
                    frcore_work_process(wqe);
                }
                else 
                {

                    octnic->port[wqe->ipprt].stats.fromwire.err_drop++;

                    DBG("L2/L1 error from port %d. Error code=%x\n",
                        wqe->ipprt, wqe->word2.snoip.err_code);
                    if (wqe->ipprt == 0)
                    {
                        FRCORE_STAT_INC(stat_p0_rx_errs);
                        switch (wqe->word2.snoip.err_code)
                        {
                        case CVMX_PIP_PARTIAL_ERR:
                            FRCORE_STAT_INC(stat_p0_PARTIAL_ERR);
                            break;
                        case CVMX_PIP_JABBER_ERR:
                            FRCORE_STAT_INC(stat_p0_JABBER_ERR);
                            break;
                        case CVMX_PIP_OVER_FCS_ERR:
                            FRCORE_STAT_INC(stat_p0_OVER_FCS_ERR);
                            break;
                        case CVMX_PIP_OVER_ERR:
                            FRCORE_STAT_INC(stat_p0_OVER_ERR);
                            FRCORE_STAT_INC(stat_pkts_above_1600);
                            FRCORE_STAT_INC(stat_very_long_pkts);
                            break;
                        case CVMX_PIP_ALIGN_ERR:
                            FRCORE_STAT_INC(stat_p0_ALIGN_ERR);
                            break;
                        case CVMX_PIP_UNDER_FCS_ERR:
                            FRCORE_STAT_INC(stat_p0_UNDER_FCS_ERR);
                            break;
                        case CVMX_PIP_GMX_FCS_ERR:
                            FRCORE_STAT_INC(stat_p0_GMX_FCS_ERR);
                            break;
                        case CVMX_PIP_UNDER_ERR:
                            FRCORE_STAT_INC(stat_p0_UNDER_ERR);
                            FRCORE_STAT_INC(stat_pkts_below_64);
                            FRCORE_STAT_INC(stat_very_short_pkts);
                            break;
                        case CVMX_PIP_EXTEND_ERR:
                            FRCORE_STAT_INC(stat_p0_EXTEND_ERR);
                            break;
                        case CVMX_PIP_LENGTH_ERR:
                            FRCORE_STAT_INC(stat_p0_LENGTH_ERR);
                            break;
                        case CVMX_PIP_DAT_ERR:
                            FRCORE_STAT_INC(stat_p0_DAT_ERR);
                            break;
                        case CVMX_PIP_SKIP_ERR:
                            FRCORE_STAT_INC(stat_p0_SKIP_ERR);
                            break;
                        case CVMX_PIP_NIBBLE_ERR:
                            FRCORE_STAT_INC(stat_p0_NIBBLE_ERR);
                            break;
                        case CVMX_PIP_PIP_FCS:
                            FRCORE_STAT_INC(stat_p0_PIP_FCS);
                            break;
                        case CVMX_PIP_PIP_SKIP_ERR:
                            FRCORE_STAT_INC(stat_p0_PIP_SKIP_ERR);
                            break;
                        case CVMX_PIP_PIP_L2_MAL_HDR:
                            FRCORE_STAT_INC(stat_p0_PIP_L2_MAL_HDR);
                            break;
                        case CVMX_PIP_PUNY_ERR:
                            FRCORE_STAT_INC(stat_p0_PUNY_ERR);
                            break;
                        default:
                            FRCORE_STAT_INC(stat_p0_UNKNOWN_ERR);
                            break;
                        }
                    }
                    else
                    {
                        FRCORE_STAT_INC(stat_p1_rx_errs);
                        switch (wqe->word2.snoip.err_code)
                        {
                        case CVMX_PIP_PARTIAL_ERR:
                            FRCORE_STAT_INC(stat_p1_PARTIAL_ERR);
                            break;
                        case CVMX_PIP_JABBER_ERR:
                            FRCORE_STAT_INC(stat_p1_JABBER_ERR);
                            break;
                        case CVMX_PIP_OVER_FCS_ERR:
                            FRCORE_STAT_INC(stat_p1_OVER_FCS_ERR);
                            break;
                        case CVMX_PIP_OVER_ERR:
                            FRCORE_STAT_INC(stat_p1_OVER_ERR);
                            break;
                        case CVMX_PIP_ALIGN_ERR:
                            FRCORE_STAT_INC(stat_p1_ALIGN_ERR);
                            break;
                        case CVMX_PIP_UNDER_FCS_ERR:
                            FRCORE_STAT_INC(stat_p1_UNDER_FCS_ERR);
                            break;
                        case CVMX_PIP_GMX_FCS_ERR:
                            FRCORE_STAT_INC(stat_p1_GMX_FCS_ERR);
                            break;
                        case CVMX_PIP_UNDER_ERR:
                            FRCORE_STAT_INC(stat_p1_UNDER_ERR);
                            break;
                        case CVMX_PIP_EXTEND_ERR:
                            FRCORE_STAT_INC(stat_p1_EXTEND_ERR);
                            break;
                        case CVMX_PIP_LENGTH_ERR:
                            FRCORE_STAT_INC(stat_p1_LENGTH_ERR);
                            break;
                        case CVMX_PIP_DAT_ERR:
                            FRCORE_STAT_INC(stat_p1_DAT_ERR);
                            break;
                        case CVMX_PIP_SKIP_ERR:
                            FRCORE_STAT_INC(stat_p1_SKIP_ERR);
                            break;
                        case CVMX_PIP_NIBBLE_ERR:
                            FRCORE_STAT_INC(stat_p1_NIBBLE_ERR);
                            break;
                        case CVMX_PIP_PIP_FCS:
                            FRCORE_STAT_INC(stat_p1_PIP_FCS);
                            break;
                        case CVMX_PIP_PIP_SKIP_ERR:
                            FRCORE_STAT_INC(stat_p1_PIP_SKIP_ERR);
                            break;
                        case CVMX_PIP_PIP_L2_MAL_HDR:
                            FRCORE_STAT_INC(stat_p1_PIP_L2_MAL_HDR);
                            break;
                        case CVMX_PIP_PUNY_ERR:
                            FRCORE_STAT_INC(stat_p1_PUNY_ERR);
                            break;
                        default:
                            FRCORE_STAT_INC(stat_p1_UNKNOWN_ERR);
                            break;
                        }
                    }

                    cvm_free_host_instr(wqe);
                }
                return 0;
            }

            printf("Unsupported WQE format @ %p\n", wqe);
            cvm_drv_print_data(wqe, 64);
            cvm_free_host_instr(wqe);

        }
    }

    return 0;
}

/** This loop is run by all cores running the application.
  * If any work is available it is processed. If there is no work
  * and
  * If CVMCS_DUTY_CYCLE is enabled, it prints useful statistics
  * at intervals determined by DISPLAY_INTERVAL.
  * If CVMCS_TEST_PKO is enabled, then packets are sent to PKO
  * port 32 at intervals determined by "cvmcs_run_freq".
  * If CVMCS_FIXED_SIZE_TEST is enabled, packets of fixed size
  * are sent. Else packet size can be of range 1-CVM_MAX_DATA
  * where CVM_MAX_DATA cannot be > 65520 bytes.
  */
int
frcore_data_loop()
{
    cvmx_wqe_t    *wqe;
    uint64_t       last_display=0, last_link_check=0;
    uint64_t       idle_counter = 0, pko_test_counter=0;

    printf("# cvmcs: Data loop started on core[%d]\n", core_id);
    do {

#if !defined(DISABLE_PCIE14425_ERRATAFIX)
        if(is_display_core(core_id)) {
            cvmcs_sw_iq_bp();
        }
#endif

        if (cvmx_sysinfo_get()->board_type == CVMX_BOARD_TYPE_SIM)
        {
           wqe = cvmx_pow_work_request_sync(CVMX_POW_NO_WAIT);
           if (wqe == NULL) {

              // if (cvmx_fau_fetch_and_add64(FRCORE_PAKCETS, 0) == 8)
                 //  break;
               continue;
           }
        }
        else
        {
            wqe = cvmcs_app_get_work_sync(0);
        }
        if (wqe) {
            wqe_count[core_id]++;
            frcore_process_wqe(wqe);
            continue;
        }

        idle_counter++;
        pko_test_counter++;

        if (cvmx_sysinfo_get()->board_type == CVMX_BOARD_TYPE_SIM)
        {
            /* Increment the total packet counts */
            //cvmx_fau_atomic_add64(FRCORE_PAKCETS, 1);
            //cvmx_fau_atomic_add64(FRCORE_OUTSTANDING, 1);
        }

    /* There is no definition of model PASS2_0 in SDK 1.8.0; check for
       model CN56XX_PASS2 returns true for CN56XX Pass2.0 parts only.
       In SDK 1.8.1, check for model CN56XX_PASS2 returns true
       for all CN56XX Pass 2.X parts, so use model CN56XX_PASS2_0 instead.
    */
#if (OCTEON_SDK_VERSION_NUM < 108010000ULL)
        if(OCTEON_IS_MODEL(OCTEON_CN56XX_PASS2))
#else
        if(OCTEON_IS_MODEL(OCTEON_CN56XX_PASS2_0))
#endif
            cvm_56xx_pass2_update_pcie_req_num();


        if(is_display_core(core_id)) {

            if( (cvmx_get_cycle() - last_link_check)
                 >= (cpu_freq * LINK_CHECK_INTERVAL)) {
                frcore_check_link_status();
                frcore_cal_oct_speed();
                last_link_check = cvmx_get_cycle();
            }

#ifdef  CVMCS_DUTY_CYCLE
            if( (cvmx_get_cycle() - last_display)
                 >= (cpu_freq * DISPLAY_INTERVAL)) {

                //cvmcs_app_duty_cycle_actions();
                frcore_core_print_pow();
                frcore_core_print_pip_pko_stat();
                frcore_print_pool_count_stats();
                frcore_print_block_usage();
                frcore_print_stats();
                frcore_stat_dump();
                last_display = cvmx_get_cycle();
            }
#endif
        }
    } while(1);
    printf("# cvmcs: Core %d Exited from data loop\n", core_id);


    return 0;
}

#ifdef __linux__
void signal_handler(int x)
{
        printf("# cvmcs: Received signal %d, quitting now!!\n", x);
        signal(SIGINT, prev_sig_handler );
        cvmcs_app_shutdown();
        exit(0);
}
#endif

static void
modify_ipd_settings(void)
{
    cvmx_ipd_ctl_status_t ipd_reg;
    ipd_reg.u64 = cvmx_read_csr(CVMX_IPD_CTL_STATUS);
    ipd_reg.s.len_m8 = 1;
    cvmx_write_csr(CVMX_IPD_CTL_STATUS, ipd_reg.u64);
}

int frcore_init_frc()
{
    frcore_cmd_init();
    #if FRC_CONFIG_GET_SYSINFO
    frcore_misc_init();
    #endif
#if FRC_CONFIG_DMA_TEST
    frcore_dma_init();
#else
#endif
#if FRC_CONFIG_SIMPLE_PACKAGE
    frcore_simple_package_chan_init();
#endif
#if FRC_CONFIG_SSN_CHAN
    frcore_ssn_chan_init();
#endif
#if FRC_CONFIG_SSN
    if(frcore_ssn_main_prepare()) {
        return -1;
    }
#endif
    //frcore_misc_init();
#if FRC_CONFIG_SSN
    frcore_fr_init();
#endif

    frcore_bmm_init();

    frcore_phy_init();

#if FRC_CONFIG_RULE
    frcore_rule_init();
#endif

#if FRC_CONFIG_TWO_TUPLE
    frcore_acl_init();
#endif

#if FRC_CONFIG_UDP
    frcore_udp_init();
#endif

#if FRC_CONFIG_VLAN_CHECK
    frcore_vlan_check_init();
#endif

#if FRC_CONFIG_MAC_STATISTICS
    frcore_mac_statistics_init();
#endif

#if FRC_CONFIG_TIMESTAMP_CHECK
    frcore_timestamp_check_init();
#endif

#if FRC_CONFIG_IPC
    frcore_ipc_init();
#endif
    return 0;
}

void sim_init()
{
    /* Initialize the FAU registers. */
    //cvmx_fau_fetch_and_add64(FRCORE_ERRORS, 0);
    if (cvmx_sysinfo_get()->board_type == CVMX_BOARD_TYPE_SIM)
    {
        //cvmx_fau_fetch_and_add64(FRCORE_PAKCETS, 0);
        //cvmx_fau_fetch_and_add64(FRCORE_OUTSTANDING, 0);
    }
}

#define MTU_IPC_MAX 4096

void frcore_mtu_init(int new_mtu)
{
    frcore_change_mtu(0, new_mtu);
    frcore_change_mtu(16, new_mtu);
}

/** MAIN */
int main()
{
    unsigned int packet_termination_num = 8;
    if(cvmcs_app_bringup())
        return 1;

    if(is_boot_core(core_id)) {

        printf("SDK Build Number: %s\n", SDKVER);
        cvmcs_print_compile_options();
        printf("\n# frcore: Starting global init on core %d\n", core_id);

        if (cvmx_sysinfo_get()->board_type == CVMX_BOARD_TYPE_SIM)
        {
            sim_init();
        }

        if(frcore_init_global())
            return 1;

        if (frcore_init_frc())
        {
            return 1;
        }

        printf("# frcore: Global initialization completed\n\n");
    }

    cvmcs_app_barrier();

    /* Initialization local to each core */
    frcore_init_local();

    if(is_boot_core(core_id)) {

#ifdef __linux__
        prev_sig_handler = signal(SIGINT, signal_handler);
#endif

        cvmx_helper_setup_red(RED_HIGH_WMARK, RED_LOW_WMARK);

        cvm_drv_setup_app_mode(FRC_DRV_APP);
        cvm_drv_start();

        /* Modify default IPD settings */
        modify_ipd_settings();

        /* Enable IPD only after sending the START indication packet to host. */
        cvmx_ipd_enable();

        print_pool_count_stats();
    }

    cvmcs_app_barrier();

    frcore_pkt_main();

    cvmcs_app_barrier();

    if (cvmx_sysinfo_get()->board_type == CVMX_BOARD_TYPE_SIM)
    {
        cvmx_pow_iq_com_cnt_t pow_iq_com_cnt;
        printf("Waiting to give packet input (~1Gbps) time to read the packets...\n");
        do
        {
            pow_iq_com_cnt.u64 = cvmx_read_csr(CVMX_POW_IQ_COM_CNT);
        } while (pow_iq_com_cnt.s.iq_cnt < packet_termination_num);
        printf("Done waiting\n");
    }
    cvmcs_app_barrier();

#ifdef FRC_CONFIG_IPC
    frcore_mtu_init(MTU_IPC_MAX);
#endif
    /* Start the data processing loop on each core. */
    frcore_data_loop();

    cvmcs_app_barrier();

    if(is_boot_core(core_id))
        cvmcs_app_shutdown();

    return 0;
}


/* $Id: frcore-main.c 52873 2010-09-09 22:35:57Z panicker $ */

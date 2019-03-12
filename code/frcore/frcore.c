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




#include <stdio.h>
#include <string.h>

#include "frcore.h"
#include "cvmx-helper.h"
#include "cvm-pci-loadstore.h"





extern     uint32_t    core_id;

CVMX_SHARED uint32_t            cvm_first_buf_size_after_skip;
CVMX_SHARED uint32_t            cvm_subs_buf_size_after_skip;






int
frcore_port_active(int p_num)
{
        if(p_num < MAX_OCTEON_ETH_PORTS)
                return (octnic->port[p_num].state & CVM_NIC_IF_STATE_ACTIVE);

        return 0;
}






void
frcore_print_link_status(oct_link_status_t  *st, int port)
{
        printf("Port %d: %d Mbps %s duplex %s\n", port, st->s.speed,
                (st->s.duplex)?"Full":"Half", (st->s.status)?"UP":"DOWN");
}








uint64_t
frcore_update_link_status(int port)
{
        cvmx_helper_link_info_t link = cvmx_helper_link_autoconf(port);
        oct_link_status_t  st;

        st.s.duplex = link.s.full_duplex;
        st.s.status = link.s.link_up;
        st.s.speed  = link.s.speed;

        return st.u64;
}







int
frcore_change_multicast_list(int port)
{
        cvmx_gmxx_prtx_cfg_t gmx_cfg;
        int                  interface = INTERFACE(port);
        int                  index = INDEX(port);
        octnet_ifflags_t     flags = octnic->port[port].ifflags;

        printf("%s port: %d flags: 0x%x\n", __FUNCTION__, port, flags);

        if ((interface < 2)
             && (cvmx_helper_interface_get_mode(interface) != CVMX_HELPER_INTERFACE_MODE_SPI)) {

                cvmx_gmxx_rxx_adr_ctl_t control;
                control.u64    = 0;
                control.s.bcst = 1;     /* Allow broadcast MAC addresses */

                /* Force accept multicast packets. This is required to support IPv6. */
                control.s.mcst = 2;

                if (flags & OCTNET_IFFLAG_PROMISC)
                        control.s.cam_mode = 0; /* Reject matches if promisc. Since CAM is shut off, should accept everything */
                else
                        control.s.cam_mode = 1; /* Filter packets based on the CAM */

                gmx_cfg.u64 = cvmx_read_csr(CVMX_GMXX_PRTX_CFG(index, interface));
                cvmx_write_csr(CVMX_GMXX_PRTX_CFG(index, interface), gmx_cfg.u64 & ~1ull);

                cvmx_write_csr(CVMX_GMXX_RXX_ADR_CTL(index, interface), control.u64);

                if (flags & OCTNET_IFFLAG_PROMISC)
                        cvmx_write_csr(CVMX_GMXX_RXX_ADR_CAM_EN(index, interface), 0);
                else
                        cvmx_write_csr(CVMX_GMXX_RXX_ADR_CAM_EN(index, interface), 1);

                cvmx_write_csr(CVMX_GMXX_PRTX_CFG(index, interface), gmx_cfg.u64);

        } else {

                return 1;

        }

        return 0;
}





int
frcore_change_mac_address(uint8_t *addr, int port)
{
        uint8_t             *ptr = (uint8_t *)addr + 2;
        uint64_t             mac = 0;
        cvmx_gmxx_prtx_cfg_t gmx_cfg;
        int                  i;
        int                  interface = INTERFACE(port);
        int                  index = INDEX(port);


        for (i=0; i < 6; i++) {
                mac = (mac<<8) | (uint64_t)(ptr[i]);
                printf(" %02x ", ptr[i]);
        }
        printf("\n");

        gmx_cfg.u64 = cvmx_read_csr(CVMX_GMXX_PRTX_CFG(index, interface));

        cvmx_write_csr(CVMX_GMXX_PRTX_CFG(index, interface), gmx_cfg.u64 & ~1ull);
        cvmx_write_csr(CVMX_GMXX_SMACX(index, interface), mac);
        cvmx_write_csr(CVMX_GMXX_RXX_ADR_CAM0(index, interface), ptr[0]);
        cvmx_write_csr(CVMX_GMXX_RXX_ADR_CAM1(index, interface), ptr[1]);
        cvmx_write_csr(CVMX_GMXX_RXX_ADR_CAM2(index, interface), ptr[2]);
        cvmx_write_csr(CVMX_GMXX_RXX_ADR_CAM3(index, interface), ptr[3]);
        cvmx_write_csr(CVMX_GMXX_RXX_ADR_CAM4(index, interface), ptr[4]);
        cvmx_write_csr(CVMX_GMXX_RXX_ADR_CAM5(index, interface), ptr[5]);
        cvmx_write_csr(CVMX_GMXX_PRTX_CFG(index, interface), gmx_cfg.u64);

        return 0;
}








int
frcore_change_mtu(int port, int new_mtu)
{
        int max_frm_size = new_mtu + 18;
        int interface = INTERFACE(port);
        int index = INDEX(port);

        /* Limit the MTU to make sure the ethernet packets are between 64 bytes
           and 65535 bytes */
        if ( (max_frm_size < OCTNET_MIN_FRM_SIZE)
                 || (max_frm_size > OCTNET_MAX_FRM_SIZE))  {
                printf("MTU must be between %d and %d.\n", OCTNET_MIN_FRM_SIZE - 18,
                       OCTNET_MAX_FRM_SIZE - 18);
                return 1;
        }

        if ((interface < 2) && (cvmx_helper_interface_get_mode(interface) != CVMX_HELPER_INTERFACE_MODE_SPI)) {

                if (OCTEON_IS_MODEL(OCTEON_CN3XXX) || OCTEON_IS_MODEL(OCTEON_CN58XX)) {
                        /* Signal errors on packets larger than the MTU */
                        cvmx_write_csr(CVMX_GMXX_RXX_FRM_MAX(index, interface), max_frm_size);
                }
                /* Set the hardware to truncate packets larger than the MTU. The
                    jabber register must be set to a multiple of 8 bytes, so round up */
                cvmx_write_csr(CVMX_GMXX_RXX_JABBER(index, interface), (max_frm_size + 7) & ~7u);
                printf("MTU for port %d changed to %d\n\n", port, new_mtu);
        }

        return 0;
}












static int
frcore_prepare_link_info_pkt(uint64_t  *buf)
{
        int                i, size = 0;
        oct_link_info_t   *linfo;


        if(octnic->nports == 0)
                return 0;

        /* 64-bit link info field tells host driver the number of links
           that the core has information for. */
        buf[0] = octnic->nports;

        linfo = (oct_link_info_t *)&buf[1];

        for(i = 0; i < MAX_OCTEON_ETH_PORTS; i++) {
                if(frcore_port_active(i)) {
                        linfo->link.u64 = frcore_update_link_status(i);
                        linfo->gmxport  = i;
                        linfo->pciport  = octnic->port[i].linfo.pciport;
                        linfo->hw_addr  = octnic->port[i].linfo.hw_addr;
                        linfo++;
                }
        }

        size = (OCT_LINK_INFO_SIZE * octnic->nports) + 8;

        CVMX_SYNCWS;
        return size;
}








int
frcore_send_link_info(cvmx_wqe_t  *wqe)
{
        cvm_pci_dma_cmd_t      cmd;
        cvmx_buf_ptr_t         lptr;
        cvm_dma_remote_ptr_t   rptr;
        cvmx_raw_inst_front_t  f;
        uint64_t              *buf;

        cmd.u64  = 0;
        lptr.u64 = 0;


        memcpy(&f, wqe->packet_data, sizeof(cvmx_raw_inst_front_t) );

        rptr.s.addr = f.rptr;
        rptr.s.size = f.irh.s.rlenssz;

        if(cvmx_unlikely(rptr.s.size > CVMX_FPA_WQE_POOL_SIZE)) {
                printf("[ DRV ] Cannot use WQE pool buf for sending link info\n");
                return 1;
        }

        lptr.s.size = rptr.s.size;

        /* Free the packet buffers for this WQE. */
        cvm_free_wqe_packet_bufs(wqe);

        /* Re-use the WQE buffer to send the link info to host. */
        buf = (uint64_t *)wqe;

        /* Reset all bytes so that unused fields don't have any value. */
        memset(buf, 0, rptr.s.size);

        lptr.s.addr = CVM_DRV_GET_PHYS(buf);

        /* First 8 bytes is response header. No information in it.
           The link data starts from byte offset 8. */
        if(frcore_prepare_link_info_pkt(&buf[1]) == 0) {
                printf("[ DRV ] prepare link info pkt failed\n");
                return 1;
        }

        lptr.s.i    = 1;
        lptr.s.pool = CVMX_FPA_WQE_POOL;

        cmd.s.nl = cmd.s.nr = 1;

        return cvm_pci_dma_send_data(&cmd, &lptr, &rptr);
}

void
frcore_check_link_status(void)
{
        int                 i;
        oct_link_status_t   link;

        for(i = 0 ; i < MAX_OCTEON_ETH_PORTS; i++) {

                if(!frcore_port_active(i))
                        continue;

                link.u64 = frcore_update_link_status(i);

                if(link.u64 != octnic->port[i].linfo.link.u64) {
                        /* If link status has changed, update status info first. */
                        octnic->port[i].linfo.link.u64 = link.u64;
                        CVMX_SYNCW;
                        /* Print the updated status. */
                        frcore_print_link_status(&link, i);
                }
        }
}

void
frcore_cal_oct_speed(void)
{
        int                 i;

        for(i = 0 ; i < MAX_OCTEON_ETH_PORTS; i++) {
           if(!frcore_port_active(i))
              continue;
           octnic->port[i].stats.fromhost.last_total_rcvd =
              octnic->port[i].stats.fromhost.total_rcvd;
           octnic->port[i].stats.fromhost.last_total_bytes =
              octnic->port[i].stats.fromhost.total_bytes;
           octnic->port[i].stats.fromwire.last_total_rcvd =
              octnic->port[i].stats.fromwire.total_rcvd;
           octnic->port[i].stats.fromwire.last_total_bytes =
              octnic->port[i].stats.fromwire.total_bytes;
        }
}

void
frcore_process_nic_cmd(cvmx_wqe_t    *wqe)
{
        cvmx_raw_inst_front_t  *f = (cvmx_raw_inst_front_t *)wqe->packet_data;
        octnet_cmd_t           *ncmd;
        uint64_t                retaddr = 0, ret = -1ULL;

        ncmd = (octnet_cmd_t  *) ((uint8_t *)f + CVM_RAW_FRONT_SIZE);

        DBG("NW Command packet received (0x%016lx)\n", ncmd->u64);

        switch(ncmd->s.cmd) {
                case OCTNET_CMD_RX_CTL:
                        printf("Command for RX Control: (Port %d Command: %s)\n",
                                ncmd->s.param1, (ncmd->s.param2?"Start":"Stop"));
                        if(frcore_port_active(ncmd->s.param1))
                                octnic->port[ncmd->s.param1].rx_on = ncmd->s.param2;
                        CVMX_SYNCWS;
                        break;

                case OCTNET_CMD_CHANGE_MTU:
                        printf("Command to change MTU (port %d new_mtu %d)\n",
                                ncmd->s.param1, ncmd->s.param2);
                        ret = frcore_change_mtu(ncmd->s.param1, ncmd->s.param2);
                        if(f->rptr)
                                retaddr = f->rptr + 8;
                        break;

                case OCTNET_CMD_CHANGE_MACADDR:
                        {
                                uint64_t   *macaddr;

                                macaddr = (uint64_t *)((uint8_t *)ncmd + sizeof(octnet_cmd_t));
                                printf("Command to change MAC Addr (port %d MAC 0x%lx)\n",
                                        ncmd->s.param1, *macaddr);
                                ret = frcore_change_mac_address((uint8_t *)macaddr, ncmd->s.param1);
                                if(f->rptr)
                                        retaddr = f->rptr + 8;
                                break;
                        }

                case OCTNET_CMD_CHANGE_DEVFLAGS:
                        {
                                octnet_ifflags_t    flags;
                                int                 port;

                                flags = (octnet_ifflags_t)
                                        *((uint64_t *)((uint8_t *)ncmd + sizeof(octnet_cmd_t)));
                                port  = ncmd->s.param1;
                                if (port == 0) {
                                   flags |= OCTNET_IFFLAG_PROMISC;
                                }
                                printf("Command to change Flags (port %d Flags 0x%lx)\n",
                                        port, (unsigned long)flags);
                                ret = 0;
                                if(frcore_port_active(port)) {
                                        if(octnic->port[port].ifflags != flags) {
                                                octnic->port[port].ifflags = flags;
                                                ret = frcore_change_multicast_list(port);
                                        }
                                }
                                if(f->rptr)
                                        retaddr = f->rptr + 8;
                                break;
                        }

                default:
                        printf("Unknown NIC Command 0x%08x\n", ncmd->s.cmd);
                        retaddr = 0;
                        break;

        }

        if(retaddr)
                cvm_pci_mem_writell(retaddr, ret);

        cvm_free_host_instr(wqe);
}

uint64_t frcore_gettimeofday(void)
{
    uint64_t cycle;
    cycle = cvmx_get_cycle();

    return cycle;
}

void frcore_sleep(uint64_t cycle)
{
    uint64_t start_cycle, cur_cycle;

    start_cycle = cvmx_get_cycle();
    while (1)
    {
        cur_cycle = cvmx_get_cycle();
        if ((cur_cycle - start_cycle) >= cycle)
        {
            break;
        }
    }
}

void
frcore_dump_ptrs(cvmx_buf_ptr_t  *ptr, int numbufs)
{
    int              i, total=0;
    cvmx_buf_ptr_t  *next;

    for(i = 0; i < numbufs; i++) {
        next = (cvmx_buf_ptr_t *)CVM_DRV_GET_PTR(ptr->s.addr - 8);
        printf("ptr[%d]: 0x%016llx  size: %d pool %d\n", i, CAST64(ptr->s.addr),
               ptr->s.size, ptr->s.pool);
        total += ptr->s.size;
        ptr = next;
    }

    printf("Total Bytes: %d\n", total);
}

/* $Id: frcore.c 52830 2010-09-09 03:03:13Z panicker $ */

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


#ifndef __FRCORE_H__
#define __FRCORE_H__

#include <stdio.h>
#include <string.h>

#include "cvmx-config.h"
#include "cvmx.h"
#include "cvmx-swap.h"
#include "cvmx-sysinfo.h"

#include "cvm-driver-defs.h"
#include "cvm-drv.h"
#include "cvm-drv-debug.h"
#include "octeon-nic-common.h"
#include "octeon-opcodes.h"

#include "frc.h"

#include "frcore_defs.h"
#include "frcore_debug.h"
#include "frcore_config.h"



#define  MAX_OCTEON_ETH_PORTS    32

#define  CVM_NIC_IF_STATE_INACTIVE     0
#define  CVM_NIC_IF_STATE_ACTIVE       1


/* Enable this flag if you want the NIC app to send packets across multiple
   PCI output queues. This is required if you enable multi-core Rx processing
   or NAPI in the host driver. */
//#define  USE_MULTIPLE_OQ





/** Stats for each NIC port in a single direction. */
struct nic_stats_t {

    int64_t        total_wqe;

    int64_t        total_rcvd; /* packet number statistics */
    int64_t        total_bytes; /* packet bytes number statistics */
    int64_t        last_total_rcvd;   /*bit per second */
    int64_t        last_total_bytes;   /* packet per second */
    int64_t        total_test;

    int64_t        total_fwd;

    int64_t        err_pko;

    int64_t        err_link;

    int64_t        err_drop;
};






typedef struct {

    struct nic_stats_t  fromwire;
    struct nic_stats_t  fromhost;

} oct_link_stats_t;

#define OCT_LINK_STATS_SIZE   (sizeof(oct_link_stats_t))




typedef struct {
    uint64_t            state:8;
    uint64_t            ifidx:8;
    uint64_t            present:8;
    uint64_t            rx_on:8;
    uint64_t            ifflags:16;
    uint64_t            rsvd:16;

    oct_link_info_t     linfo;

    oct_link_stats_t    stats;

} octnic_port_info_t;




typedef struct {

    uint64_t            board_type:16;
    uint64_t            nports:16;
    uint64_t            numpciqs:8;
    uint64_t            rsvd:24;
    uint64_t            macaddrbase;

    octnic_port_info_t  port[MAX_OCTEON_ETH_PORTS];


}  octnic_dev_t;



extern CVMX_SHARED octnic_dev_t  *octnic;


#define   COREMASK_BOOT   cvmx_coremask_core(0) /*core 0 to perform boot init*/
#define   INTERFACE(port) (port >> 4)
/* Ports 0-15 are interface 0, 16-31 are interface 1 */
#define   INDEX(port)     (port & 0xf)


static inline uint64_t
frcore_mac_to_64bit(uint8_t  *mac)
{
    uint64_t  macaddr=0;
    int i;
    for(i = 0; i < 6; i++)
        macaddr = (macaddr << 8) | mac[i];
    return macaddr;
}


/* Number of (non-pci) output queues. */
#define NUMBER_OUTPUT_QUEUES 8


/* Macro to get the queue number for a given non-PCI port */
#define GET_QUEUE(a) ((a < 4) ? a : a-12)


int
frcore_port_active(int p_num);



int
frcore_send_link_info(cvmx_wqe_t  *wqe);

void
frcore_check_link_status(void);

void
frcore_cal_oct_speed(void);

int
frcore_change_multicast_list(int port);


int
frcore_change_mac_address(uint8_t *addr, int port);


void
frcore_process_nic_cmd(cvmx_wqe_t    *wqe);


int
frcore_setup_interfaces(void);




void
frcore_process_frc_cmd(cvmx_wqe_t    *wqe);


uint64_t frcore_gettimeofday(void);

#define frcore_memcpy   memcpy

void frcore_sleep(uint64_t cycle);

#define PREPROC_STAGING             ((uint32_t)(0x01<<28))
#define SESSION_STAGING             ((uint32_t)(0x02<<28))
#define CONTROL_STAGING             ((uint32_t)(0x03<<28))

#define STAGING_MASK                ((uint32_t)(0xff<<28))

#define staging_tag(x, y)      ((x)|(y))
#define tag_is_staging(x, y)   (((x)&STAGING_MASK) == (y))


struct mpp_tuple{
    union{
        struct{
            uint32_t sip, dip;
            uint16_t sp, dp;
            uint8_t  proto;
            uint8_t  session_type:3;
            uint8_t  ipif:5;
            uint16_t  rsv;
        };
        uint64_t data[2];
        uint32_t data_32[4];
    };
};

/*
 *  Intermedia control block, should limit to fit the
 *  packet_data in wqe
 */
struct mpp_control{
    /* 0  */
    uint8_t *packet;
    /* 1 */
    uint8_t  ipif;
    uint8_t  len_header;
    int16_t  rev0;
#define SSN_INDEX_INVALID 0xffffffff
    uint32_t ssn_index;

    /* 2 */
    struct mpp_ssn *ssn;

    /* 3 */
    struct udphdr *udph;

    /* 4 */
    uint8_t  session_type;
    uint8_t  vlan_act;            /* 3:5 */
#define     MPP_VLAN_DO_NOTHING   0xe0
#define     MPP_VLAN_ADD          0xc0
#define     MPP_VLAN_DEL          0xa0
#define     MPP_VLAN_TAG_MASK     0x1f
    uint8_t  mirrif;
    uint8_t  ilif;             /* ingress logical interface  */
    int32_t  dupl_num;

    /* 5 */
    /* for syncookie */
    uint32_t svr_seq;
    uint32_t syn_cookie;

    /* 6 */
    uint32_t  tag;
    int32_t  rev;
};
#endif

void frcore_forward_packet_to_wire(cvmx_wqe_t   *wqe, int p_num, int offload);
void frcore_dump_ptrs(cvmx_buf_ptr_t  *ptr, int numbufs);

/* $Id: frcore.h 53241 2010-09-23 01:17:08Z panicker $ */

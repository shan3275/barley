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


#ifndef   __FRCORE_DEFS_H__
#define   __FRCORE_DEFS_H__


#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

#include "cvmx.h"
#include "cvmx-wqe.h"

#include "frc.h"

/* Enable this define to let the application print information at regular
   intervals */
#define CVMCS_DUTY_CYCLE

/* dma channel rest intervals */
#define DMA_CHANNEL_REST_DIV    10

/* Interval in seconds after which the duty cycle prints information. */
#define DISPLAY_INTERVAL         15

#define LINK_CHECK_INTERVAL      1


/* Maximum number of cores the application supports. */
#define MAX_CORES                16





#define INIT_PACKET_COUNT        (1 * 1024)

/* Size of each FPA pool */
/* The packet and WQE pool is used by hardware and PCI core driver. */
#define FPA_PACKET_POOL_COUNT    (1467 * INIT_PACKET_COUNT)
//#define FPA_PACKET_POOL_COUNT    (146 * INIT_PACKET_COUNT)
#define FPA_WQE_POOL_COUNT       (1467 * INIT_PACKET_COUNT)

/* PKO queue command buffers are allocated by simple exec from this pool. */
#define FPA_OQ_POOL_COUNT        (4 * INIT_PACKET_COUNT)
#define FPA_TIMER_POOL_COUNT     (301024)


/* If OCTEON_DEBUG_LEVEL is defined all
   application debug messages are enabled. */
#ifdef OCTEON_DEBUG_LEVEL
#define DBG                      printf
#else
#define DBG(format, args...)     do{ }while(0)
#endif



#endif



/* $Id: cvmcs-nic-defs.h 47531 2010-02-23 23:54:35Z panicker $ */

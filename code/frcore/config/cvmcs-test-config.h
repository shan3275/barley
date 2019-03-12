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


  
#ifndef __CVMCS_TEST_CONFIG_H__
#define __CVMCS_TEST_CONFIG_H__
 

/* Content below this point is only used by the cvmx-config tool, and is
** not used by any C files as CAVIUM_COMPONENT_REQUIREMENT is never
defined.
*/
#ifdef CAVIUM_COMPONENT_REQUIREMENT

        /* global resource requirement */
        cvmxconfig
        {
                fpa CVM_FPA_TEST_POOL
                        size        = 16
                        priority    = 1
                        description = "Test buffer pool";

                scratch CVMCS_TEST_BUF_PTR
                        size = 8
                        iobdma = true
                        permanent = true
                        description = "Scratch pad location for test buffer";

	}
#endif
 
#endif  /* __GLOBAL_CONFIG_H__ */


/* $Id: cvmcs-test-config.h 42872 2009-05-20 00:06:26Z panicker $ */

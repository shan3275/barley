#ifndef _MPP_PROC_H
#define _MPP_PROC_H

#include "cvmx-config.h"
#include "cvmx.h"
#include "cvmx-wqe.h"
#include "cvmx-pko.h"
#include "cvmx-helper.h"
#include "cvmx-atomic.h"
#include "cvmx-tim.h"
#include "cvmx-pip.h"

#include "frcore.h"
#include "frcore_config.h"
#include "frcore_debug.h"
//#include "mpp_init.h"
//#include "mpp_util.h"
#include "frcore_proto.h"
//#include "atomic_q.h"
//#include "mpp_ip_mac.h"
//#include "mpp_cpser.h"

/*
 * free WQE and Packet Data Buffer
 */
static inline void mpp_free_wqe_packet_data(cvmx_wqe_t *work)
{
    cvmx_helper_free_packet_data(work);
    cvmx_fpa_free(work, CVMX_FPA_WQE_POOL, 0);
    //md_fau_atomic_add64(CNTER_FREE_PKTS, 1);

    //cvmx_pow_work_request_async(MC_SCR_SCRATCH_16, _cvmx_pow_wait_mode);
}

static inline void mpp_free_wqe_packet_data_ngetwork(cvmx_wqe_t *work)
{
    cvmx_helper_free_packet_data(work);
    cvmx_fpa_free(work, CVMX_FPA_WQE_POOL, 0);
    //md_fau_atomic_add64(CNTER_FREE_PKTS, 1);
}

#endif /* _MPP_PROC_H */

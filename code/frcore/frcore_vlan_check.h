#ifndef _FRCORE_VLAN_CHEKC_H_
#define _FRCORE_VLAN_CHECK_H_
#include "frcore_pkt.h"
#include "frcore_stat.h"
#include "frc_dma.h"
#include "frcore_proto.h"
#include "frcore_ssn.h"
#include "frcore_ssn_priv.h"
#include "frcore.h"
#include "frcore_cmd.h"
#include "frcore_misc.h"
#include "frc_pack.h"
#include "frcore_proto.h"
#include "frcore_init.h"
#include "frc_types.h"
#include "frcore_stat.h"
#include "frcore_alg.h"

#if FRC_CONFIG_VLAN_CHECK

#define FRCORE_VLAN_STAT_INC(_cnt)         cvmx_atomic_add64((int64_t *)(&vlan_id_stat[_cnt]), 1)
#define FRCORE_VLAN_STAT_ADD(_cnt, _val)   cvmx_atomic_add64((int64_t *)(&vlan_id_stat[_cnt]), _val)
#define FRCORE_VLAN_STAT_VAL(_cnt)         cvmx_atomic_get64((int64_t *)(&vlan_id_stat[_cnt]))
#define FRCORE_VLAN_STAT_CLEAR(_cnt)       cvmx_atomic_set64((int64_t *)(&vlan_id_stat[_cnt]), 0)

#define FRCORE_PORT_VLAN_STAT_INC(_port, _cnt)   \
    if (0 == _port)  \
    { \
        FRCORE_VLAN_STAT_INC(_cnt); \
    } \
    else \
    { \
        FRCORE_VLAN_STAT_INC(_cnt + (stat_vlan_id_max / 2)); \
    } 

#define FRCORE_PORT_VLAN_STAT_ADD(_port, _cnt, _val)   \
    if (0 == _port)  \
    { \
        FRCORE_VLAN_STAT_ADD(_cnt, _val); \
    } \
    else \
    { \
        FRCORE_VLAN_STAT_ADD((_cnt + (stat_vlan_id_max / 2)), _val); \
    } 


int frcore_vlan_check_v4(cvmx_wqe_t *work, uint8_t *eth_ptr, uint8_t *ip_ptr, uint64_t smac, uint64_t dmac);
int frcore_vlan_check_v6(cvmx_wqe_t *work, uint8_t *eth_ptr, uint8_t *ip_ptr, uint64_t smac, uint64_t dmac);
int frcore_vlan_check_init();
#endif /* end of FRC_CONFIG_VLAN_CHECK */
#endif /* !_FRCORE_VLAN_CHECK_H_ */

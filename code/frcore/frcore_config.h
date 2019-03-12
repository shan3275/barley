#ifndef __FRCORE_CONFIG_H__
#define __FRCORE_CONFIG_H__

#define FRCORE_CFG_DUMP_CYCLE     0
#define FRCORE_CFG_FREE_WORK      1

#define MC_HW_TIMER_BUCKET_TICK         1000       //1000      //microsecond
#define MC_SSN_TTL_UNIT                 (10000000/MC_HW_TIMER_BUCKET_TICK)  /* 1s */
//#define MC_SSN_TTL_UNIT                 (100)  /* 10ms */

#define FRCORE_SSN_AGE_MIN      1
#define FRCORE_SSN_AGE_MAX      250


#define MPP_MAX_ACSM_NUM_BITS       0
#define MPP_MAX_ACSM_NUM  (1<<MPP_MAX_ACSM_NUM_BITS)
#define MAX_ACL_IP_NUM      0x40            //64
#define MAX_ACL_IP_NUM_MASK     0x3f
#define MAX_ACL_IP_HASH_BUCKET_LEN      4

#define MPP_MAX_ACL_NUM                 1
#define MPP_MAX_FILTERS_BITS        11
#define MPP_MAX_FILTERS (1<< MPP_MAX_FILTERS_BITS)  // maximum amount of acl filters
#define MPP_ACL_IP_START    (MPP_MAX_FILTERS * MPP_MAX_ACSM_NUM)
#define MPP_MAX_CRSLK_ENTRY     (MPP_ACL_IP_START+ MAX_ACL_IP_NUM) //maximum counter of tags

#endif /* !__FRCORE_CONFIG_H__ */

#ifndef __FRCORE_DEBUG_H__
#define __FRCORE_DEBUG_H__

#include "frc_debug.h"

#   define FRCORE_DUMP(_fmt, _args...)  printf("%s.%d : " _fmt, __func__, __LINE__, ##_args)

#if FRC_DEBUG_CORE

#   define FRCORE_WARN(_fmt, _args...)  printf("[WARN] %s.%d : " _fmt, __func__, __LINE__, ##_args)
#   define FRCORE_ERROR(_fmt, _args...) printf("[ERROR] %s.%d : " _fmt, __func__, __LINE__, ##_args)
#   define FRCORE_INFO(_fmt, _args...)  printf("[INFO] %s.%d : " _fmt, __func__, __LINE__, ##_args)
#   define MC_PRINTF_DEBUG(fmt, arg...) printf("[DEBUG] %12s[%18s][%4d]:" fmt , __FILE__, __FUNCTION__, __LINE__, ##arg)
#   define MC_PRINTF_ERR(fmt, arg...)   printf("[ERR] %12s[%18s][%4d]:" fmt , __FILE__, __FUNCTION__, __LINE__, ##arg)
#   define MC_PRINTF_INFO(fmt, arg...)  printf("[INFO] %12s[%18s][%4d]:" fmt , __FILE__, __FUNCTION__, __LINE__, ##arg)
#   define MC_PRINTF(fmt, arg...)       printf("[INFO] %12s[%18s][%4d]:" fmt , __FILE__, __FUNCTION__, __LINE__, ##arg)
#else
#   define FRCORE_WARN(_fmt, _args...)  {}
#   define FRCORE_ERROR(_fmt, _args...) {}
#   define FRCORE_INFO(_fmt, _args...)  {}
#   define MC_PRINTF_DEBUG(fmt, arg...) {}
#   define MC_PRINTF_ERR(fmt, arg...)   printf("[ERR] %12s[%18s][%4d]:" fmt , __FILE__, __FUNCTION__, __LINE__, ##arg)
#   define MC_PRINTF_INFO(fmt, arg...)  {}
#   define MC_PRINTF(fmt, arg...)       {}
#endif

#define FRCORE_DEBUG_NONE           0x00000
#define FRCORE_DEBUG_INIT           0x00001
#define FRCORE_DEBUG_TEST           0x00002
#define FRCORE_DEBUG_CMD            0x00004
#define FRCORE_DEBUG_DMA            0x00008
#define FRCORE_DEBUG_TCP            0x00010
#define FRCORE_DEBUG_UDP            0x00020
#define FRCORE_DEBUG_QUEUE          0x00080
#define FRCORE_DEBUG_RING           0x00100
#define FRCORE_DEBUG_LOOP           0x00200
#define FRCORE_DEBUG_WQE            0x00400
#define FRCORE_DEBUG_PKT            0x00800
#define FRCORE_DEBUG_PHY            0x01000
#define FRCORE_DEBUG_LEN            0x02000
#define FRCORE_DEBUG_CHAN           0x04000
#define FRCORE_DEBUG_SSN            0x08000
#define FRCORE_DEBUG_RULE           0x10000
#define FRCORE_DEBUG_TwoTuple       0x20000
#define FRCORE_DEBUG_VLAN_CHECK     0x40000
#define FRCORE_DEBUG_TIMESTAMP_CHECK 0x80000
//#define FRCORE_DEBUG_FLAGS          0x00000
#define FRCORE_DEBUG_FLAGS          0x80000

#if FRCORE_CONFIG_FRCORE_DEBUG
#   define FRCORE_DBUF(_buf, _len) frcore_buf_dump(_buf, _len)
#else

#   define FRCORE_DBUF(_buf, _len)
#endif


#if (FRC_DEBUG_CORE)

#   define FRCORE_DEBUG(_level, _fmt, _args...) \
    { \
        if (FRCORE_DEBUG_FLAGS & _level) \
        { \
            printf("[%s] %12s[%18s][%4d]: " _fmt, #_level, __FILE__, __func__, __LINE__, ##_args); \
        } \
    }

#else

#define FRCORE_DEBUG(_level, _fmt, _args...)

#endif

#define FRCORE_INIT(_fmt, _args...)       FRCORE_DEBUG(FRCORE_DEBUG_INIT,  _fmt, ##_args)
#define FRCORE_TEST(_fmt, _args...)       FRCORE_DEBUG(FRCORE_DEBUG_TEST,  _fmt, ##_args)
#define FRCORE_CMD(_fmt, _args...)        FRCORE_DEBUG(FRCORE_DEBUG_CMD,   _fmt, ##_args)
#define FRCORE_DMA(_fmt, _args...)        FRCORE_DEBUG(FRCORE_DEBUG_DMA,   _fmt, ##_args)
#define FRCORE_TCP(_fmt, _args...)        FRCORE_DEBUG(FRCORE_DEBUG_TCP,   _fmt, ##_args)
#define FRCORE_UDP(_fmt, _args...)        FRCORE_DEBUG(FRCORE_DEBUG_UDP,   _fmt, ##_args)
#define FRCORE_QUEUE(_fmt, _args...)      FRCORE_DEBUG(FRCORE_DEBUG_QUEUE, _fmt, ##_args)
#define FRCORE_RING(_fmt, _args...)       FRCORE_DEBUG(FRCORE_DEBUG_RING,  _fmt, ##_args)
#define FRCORE_LOOP(_fmt, _args...)       FRCORE_DEBUG(FRCORE_DEBUG_LOOP,  _fmt, ##_args)
#define FRCORE_WQE(_fmt, _args...)        FRCORE_DEBUG(FRCORE_DEBUG_WQE,   _fmt, ##_args)
#define FRCORE_PKT(_fmt, _args...)        FRCORE_DEBUG(FRCORE_DEBUG_PKT,   _fmt, ##_args)
#define FRCORE_PHY(_fmt, _args...)        FRCORE_DEBUG(FRCORE_DEBUG_PHY,   _fmt, ##_args)
#define FRCORE_LEN(_fmt, _args...)        FRCORE_DEBUG(FRCORE_DEBUG_LEN,   _fmt, ##_args)
#define FRCORE_CHAN(_fmt, _args...)       FRCORE_DEBUG(FRCORE_DEBUG_CHAN,  _fmt, ##_args)
#define FRCORE_SSN(_fmt, _args...)        FRCORE_DEBUG(FRCORE_DEBUG_SSN,   _fmt, ##_args)
#define FRCORE_RULE(_fmt, _args...)       FRCORE_DEBUG(FRCORE_DEBUG_RULE,   _fmt, ##_args)
#define FRCORE_TwoTuple(_fmt, _args...)   FRCORE_DEBUG(FRCORE_DEBUG_TwoTuple,   _fmt, ##_args)
#define FRCORE_VLAN_CHECK(_fmt, _args...) FRCORE_DEBUG(FRCORE_DEBUG_VLAN_CHECK,   _fmt, ##_args)
#define FRCORE_TIMESTAMP_CHECK(_fmt, _args...) FRCORE_DEBUG(FRCORE_DEBUG_TIMESTAMP_CHECK,   _fmt, ##_args)
#endif

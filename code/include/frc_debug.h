#ifndef __FRC_DEBUG_H__
#define __FRC_DEBUG_H__

#define FRC_DEBUG_DRV       0
#define FRC_DEBUG_CORE      0
#define FRC_DEBUG_FRLOOPER  0
#define FRC_DEBUG_API       0
#define FRC_DEBUG_TWEAK     0
#define FRC_DEBUG_RING_BUFF 0
#define FRC_DEBUG_PKT_LEN   0

#define FRC_DEBUG(_level, _fmt, _args...) \
    printf("[%s]%s.%d: " _fmt,  #_level, __func__, __LINE__, ##_args)

#endif /* !__FRC_DEBUG_H__ */

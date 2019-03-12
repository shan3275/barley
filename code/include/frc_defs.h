#ifndef __FRC_DEFS_H__
#define __FRC_DEFS_H__

#if  defined(MAKE_KERNEL) // X86 linux kernel space

    #define __FRC_BYTE_ORDER            __CAVIUM_BYTE_ORDER
    #define __FRC_BIG_ENDIAN            __CAVIUM_BIG_ENDIAN
    #define __FRC_LITTLE_ENDIAN         __CAVIUM_LITTLE_ENDIAN

    #define frc_spinlock_t              spinlock_t    
    #define frc_spinlock_lock(lock)     cvmx_spinlock_lock(lock)
    #define frc_spinlock_unlock(lock)   cvmx_spinlock_unlock(lock)
    #define frc_print                   printk
#elif defined(MAKE_APP) // X86 linux user space

    #define __FRC_BYTE_ORDER            __BYTE_ORDER
    #define __FRC_BIG_ENDIAN            __BIG_ENDIAN
    #define __FRC_LITTLE_ENDIAN         __LITTLE_ENDIAN

    #define frc_spinlock_t              int    
    #define frc_spinlock_lock(lock)     
    #define frc_spinlock_unlock(lock)   
    #define frc_print                   printf
#elif defined(MAKE_CORE) // Cavium Semple Executetion

    #define __FRC_BYTE_ORDER            __BYTE_ORDER
    #define __FRC_BIG_ENDIAN            __BIG_ENDIAN
    #define __FRC_LITTLE_ENDIAN         __LITTLE_ENDIAN

    #include "cvmx-spinlock.h"

    #define frc_spinlock_t              cvmx_spinlock_t    
    #define frc_spinlock_lock(lock)     cvmx_spinlock_lock(lock)
    #define frc_spinlock_unlock(lock)   cvmx_spinlock_unlock(lock)
    #define frc_print                   printf
#else

    #error "Unkown MAKE_TYPE!, build exit now."
#endif

/* Check ENDIAN Macro
#if __FRC_BYTE_ORDER == __FRC_BIG_ENDIAN
#error "__FRC_BYTE_ORDER == __FRC_BIG_ENDIAN"
#endif

#if __FRC_BYTE_ORDER == __FRC_LITTLE_ENDIAN
#error "__FRC_BYTE_ORDER == __FRC_LITTLE_ENDIAN"
#endif
*/
#endif /* !__FRC_DEFS_H__ */

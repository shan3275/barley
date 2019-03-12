#ifndef __FRCDRV_H__
#define __FRCDRV_H__

#include "octeon_device.h"
#include "cavium_sysdep.h"
#include "frc.h"

#define MAJOR_VERSION   1
#define MINOR_VERSION   1
#define BUILD_VERSION   1



/*   
 *
 */

#if FRC_DEBUG_DRV
#   define FRCDRV_ERROR(_fmt, _args...)  printk("[ERROR] %s.%d:" _fmt, __func__, __LINE__, ##_args)
#   define FRCDRV_DEBUG(_fmt, _args...)  printk("[DEBUG] %s.%d:" _fmt, __func__, __LINE__, ##_args)
#else
#   define FRCDRV_ERROR(_fmt, _args...)
#   define FRCDRV_DEBUG(_fmt, _args...) 
#endif
#define frcdrv_malloc(size) kmalloc((size), GFP_ATOMIC)
#define frcdrv_free(pbuf) kfree(pbuf) 
/*  Some list macro
 *
 */
#define FRC_LIST_NEW(list) \
{ \
    list = frcdrv_malloc(sizeof(list_t)); \
    if (list == NULL) \
    { \
        return FRE_MEMORY; \
    } \
    memset(list, 0, sizeof(list_t)); \
}

#endif /* !__FRCDRV_H__ */

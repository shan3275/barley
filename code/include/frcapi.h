#ifndef __FRCAPI_H__
#define __FRCAPI_H__

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/ioctl.h>

#include "frc.h"
#include "frc_cmd.h"
#include "frc_ioctl.h"

#ifdef FRC_DEBUG_API
#   define FRCAPI_ERROR(_fmt, _args...)  printf("[ERROR] %s.%d:" _fmt, __func__, __LINE__, ##_args)
#   define FRCAPI_DEBUG(_fmt, _args...)  printf("[DEBUG] %s.%d:" _fmt, __func__, __LINE__, ##_args)
#else
#   define FRCAPI_ERROR(_fmt, _args...)  
#   define FRCAPI_DEBUG(_fmt, _args...) 
#endif


static inline
int frcapi(uint16_t type, uint16_t cmd, uint16_t ilen, void *input, uint16_t *olen, void *output)
{
    frc_ioctl_t arg;
    int fd, rv;

    fd = open(FRC_DEVICE_PATH, O_RDWR);
    if(fd < 0) {
        FRCAPI_ERROR("Can't open %s.\n", FRC_DEVICE_PATH);
        return FRE_OPEN;
    }

    memset(&arg, 0, sizeof(frc_ioctl_t));
    
    arg.type   = type;
    arg.cmd    = cmd;
    arg.ilen   = ilen;
    arg.input  = input;
    if (olen != NULL && output != NULL)
    {
        arg.olen   = *olen;
        arg.output = output;
    }

    switch (arg.type)
    {
    case CMD_TYPE_DRV:
        rv = ioctl(fd, IOCTL_FRCDRV_REQUEST, &arg);
        break;
    case CMD_TYPE_TEST:
    case CMD_TYPE_CTRL:
    case CMD_TYPE_USER:
        rv = ioctl(fd, IOCTL_FRCORE_REQUEST, &arg);
        break;
    default:
        rv = FRE_UNSUPPORT;
        break;
    }

    close(fd);

    if (rv != FRE_SUCCESS) {
        FRCAPI_ERROR("Ioctl fail: rv = %d!\n", rv);
        return FRE_IOCTL;
    }

    if (arg.rv != FRE_SUCCESS)
    {
        return arg.rv;
    }

    if (olen != NULL)
    {
        *olen = arg.olen;
    }
    
    return FRE_SUCCESS;
}

#endif /* !__FRCAPI_H__ */

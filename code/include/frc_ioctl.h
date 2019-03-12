#ifndef  __FRC_IOCTL_H__
#define  __FRC_IOCTL_H__

#define FRC_DEVICE_NAME    "frc"
#define FRC_DEVICE_PATH    "/dev/"FRC_DEVICE_NAME

#define  FRC_DEVICE_MAJOR   250

typedef struct frc_ioctl {
    uint32_t did;
    uint32_t rv;
    uint16_t type;
    uint16_t cmd;
    uint16_t ilen;
    uint16_t olen;
    void *input;
    void *output;
} frc_ioctl_t;

#if defined(_WIN32)

#define _IOWR(_A, _B, _C) CTL_CODE(FILE_DEVICE_UNKNOWN, 0x8000 | _B, METHOD_NEITHER, \
				   FILE_READ_DATA | FILE_WRITE_DATA)
#endif

#define FRC_MAGIC   0xC2

#define FRCDRV_REQUEST_CODE           1
#define FRCORE_REQUEST_CODE           2


#define IOCTL_FRCDRV_REQUEST   \
        _IOWR(FRC_MAGIC, FRCDRV_REQUEST_CODE, frc_ioctl_t)

#define IOCTL_FRCORE_REQUEST   \
        _IOWR(FRC_MAGIC, FRCORE_REQUEST_CODE, frc_ioctl_t)



#endif  /*__FRC_IOCTL_H__ */


/* $Id: octeon_ioctl.h 43790 2009-07-20 19:28:21Z panicker $ */

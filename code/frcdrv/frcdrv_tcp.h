#ifndef __FRCDRV_TCP_H__
#define __FRCDRV_TCP_H__

#include "frc.h"
#include "frcdrv.h"
#include "frc_list.h"
#include "frc_ioctl.h"

#if FRC_CONFIG_SSN_CHAN
#if FRC_CONFIG_SSN
int frcdrv_ssn_init();
int frcdrv_ssn_start(octeon_device_t *oct);
#endif
#endif
#endif /* !__FRCDRV_TCP_H__ */

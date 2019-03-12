#ifndef __FRCDRV_RULE_H__
#define __FRCDRV_RULE_H__

#include "frc.h"
#include "frcdrv.h"
#include "frc_list.h"
#include "frc_ioctl.h"

#if FRC_CONFIG_RULE
int frcdrv_rule_chan_init(void);
int frcdrv_rule_chan_start(octeon_device_t *octeon_dev);
#endif
#endif /* !__FRCDRV_UDP_H__ */

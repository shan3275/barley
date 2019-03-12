#ifndef __FRC_QUEUE_H__
#define __FRC_QUEUE_H__

#include "frc_dma.h"
#include "frc_ioctl.h"

#if FRC_CONFIG_SIMPLE_PACKAGE
void frcdrv_simple_package_chan_destroy(void);
int frcdrv_simple_package_chan_init(void);
frc_dma_simple_package_chan_t *frcdrv_simple_pakcage_chan_get(uint32_t chan_id);
int frcdrv_simple_package_chan_setup(frc_dma_simple_package_chan_t *chan, uint32_t type, uint64_t pool_num);
int frcdrv_simple_package_chan_set_core(octeon_device_t *oct, frc_dma_simple_package_chan_t *chan);
#endif
int frcdrv_cmd_pool_and_ring_addr_get(frc_ioctl_t *ioctl_arg);
#if FRC_CONFIG_SSN_CHAN
void frcdrv_ssn_chan_destroy(void);
int frcdrv_ssn_chan_init(void);
frc_dma_ssn_chan_t *frcdrv_ssn_chan_get(uint32_t chan_id);
int frcdrv_ssn_chan_setup(frc_dma_ssn_chan_t *chan, uint32_t type, uint64_t pool_num);
int frcdrv_ssn_chan_set_core(octeon_device_t *oct, frc_dma_ssn_chan_t *chan);
#endif

#endif /* !__FRC_QUEUE_H__ */

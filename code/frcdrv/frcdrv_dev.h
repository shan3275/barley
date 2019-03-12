#ifndef __FRCDRV_DEV_H__
#define __FRCDRV_DEV_H__

int  frcdrv_start(int octeon_id, octeon_device_t *octeon_dev);
int  frcdrv_reset(int octeon_id, void *octeon_dev);
int  frcdrv_stop(int octeon_id, void *octeon_dev);

int  frcdrv_base_init_module(void);
void frcdrv_base_exit_module(void);

#endif /* !__FRCDRV_DEV_H__ */

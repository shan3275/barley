#ifndef __FRCDRV_DMA_H__
#define __FRCDRV_DMA_H__

void  frcdrv_dma_block_free(void *block_addr);
void* frcdrv_dma_block_alloc(void);
void  frcdrv_dma_pool_free(void *block_addr);
void* frcdrv_dma_pool_alloc(void);
int   frcdrv_dma_start(octeon_device_t *oct);
int   frcdrv_dma_init(void);
void  frcdrv_dma_destroy(void);
int frcdrv_cmd_dma_loop_buff_get(frc_ioctl_t *ioctl_arg);

#endif /* !__FRC_DMA_H__ */

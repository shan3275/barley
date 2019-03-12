#include <asm/page.h>
#include <linux/slab.h>
#include <linux/proc_fs.h>

#include "frcdrv.h"

#include "frc_dma.h"
#include "frc_ioctl.h"
#include "frc_cmd.h"
#include "frcdrv_dma.h"
#include "frcdrv_cmd.h"

void
frcdrv_dma_block_free(void *block_addr)
{
    int pages = 0;
    char *addr;
    addr = (char *) block_addr;
    while (pages <= (FRC_DMA_BUFF_PAGES -1))
    {
        ClearPageReserved(virt_to_page(addr));

        addr = addr + PAGE_SIZE;
        pages++;
    }
    free_pages((unsigned long) block_addr, FRC_DMA_BUFF_PAGES_ORDER);
}

void*
frcdrv_dma_block_alloc(void)
{
    int pages = 0;
    char *addr;
    void *block_addr;

    block_addr = (void *) __get_free_pages(GFP_KERNEL,FRC_DMA_BUFF_PAGES_ORDER);

    if (block_addr == 0)
    {
        return NULL;
    }

    addr = (char *)block_addr;
    while (pages <= (FRC_DMA_BUFF_PAGES - 1))
    {
        SetPageReserved(virt_to_page(addr));
        addr = addr + PAGE_SIZE;
        pages++;
    }

    return block_addr;
}


void
frcdrv_dma_pool_free(void *pool_addr)
{
    int pages = 0;
    char *addr;
    addr = (char *) pool_addr;
    while (pages <= (FRC_DMA_BUFF_PAGES -1))
    {
        ClearPageReserved(virt_to_page(addr));

        addr = addr + PAGE_SIZE;
        pages++;
    }
    free_pages((unsigned long) pool_addr, FRC_DMA_BUFF_PAGES_ORDER);
}

void*
frcdrv_dma_pool_alloc(void)
{
    int pages = 0;
    char *addr;
    void *block_addr;

    block_addr = (void *) __get_free_pages(GFP_KERNEL,FRC_DMA_BUFF_PAGES_ORDER);

    if (block_addr == 0)
    {
        return NULL;
    }

    addr = (char *)block_addr;
    while (pages <= (FRC_DMA_BUFF_PAGES - 1))
    {
        SetPageReserved(virt_to_page(addr));
        addr = addr + PAGE_SIZE;
        pages++;
    }

    return block_addr;
}


#if FRC_CONFIG_DMA_TEST
static frc_dma_loop_buff_t *dma_loop_rx_buff[FRC_DAT_CORE_NUM];
static frc_dma_loop_buff_t *dma_loop_tx_buff[FRC_DAT_CORE_NUM];

int frcdrv_cmd_dma_loop_buff_get(frc_ioctl_t *ioctl_arg)
{
    int i;

    frc_dma_loop_arg_t *loop_arg = (frc_dma_loop_arg_t *) ioctl_arg->output;

    for (i = 0; i < FRC_DAT_CORE_NUM; i++)
    {
        loop_arg->rx_addr[i] = __pa(dma_loop_rx_buff[i]);
        loop_arg->tx_addr[i] = __pa(dma_loop_tx_buff[i]);
    }

    ioctl_arg->olen = sizeof(frc_dma_loop_arg_t);

    return FRE_SUCCESS;
}
#endif

int
frcdrv_dma_start(octeon_device_t *oct)
{
#if FRC_CONFIG_DMA_TEST
    int i, rv;
    frc_dma_loop_buff_addr_t buff_addr;


    for (i = 0; i < FRC_DAT_CORE_NUM; i++)
    {
        buff_addr.core_num = i;
        buff_addr.rx_addr = (uint64_t)octeon_pci_map_single(oct->pci_dev, dma_loop_rx_buff[i], FRC_DMA_BUFF_SIZE, CAVIUM_PCI_DMA_TODEVICE);
        buff_addr.tx_addr = (uint64_t)octeon_pci_map_single(oct->pci_dev, dma_loop_tx_buff[i], FRC_DMA_BUFF_SIZE, CAVIUM_PCI_DMA_TODEVICE);
        FRCDRV_DEBUG("%d: rx_addr %llx, tx_addr %llx\n", i, buff_addr.rx_addr, buff_addr.tx_addr);

        rv = frcore_set(oct, CMD_TYPE_CTRL, CTRL_CMD_SETUP_DMA_LOOP_BUFF, sizeof(frc_dma_loop_buff_addr_t), &buff_addr);
        if (rv != FRE_SUCCESS) {
            FRCDRV_ERROR("CTRL_CMD_SETUP_DMA_LOOP_BUFF  execute fail: rv = %d.\n", rv);
            return rv;
        }
    }
#endif
    return FRE_SUCCESS;
}

void frcdrv_dma_destroy()
{
#if FRC_CONFIG_DMA_TEST
    int i;
    for (i = 0; i < FRC_DAT_CORE_NUM; i++)
    {

        FRCDRV_DEBUG("%d: dma_loop_rx_buff %p, dma_loop_tx_buff %p\n", i, dma_loop_rx_buff[i], dma_loop_tx_buff[i]);
        if (dma_loop_rx_buff[i])
        {
            frcdrv_dma_block_free(dma_loop_rx_buff[i]);
            dma_loop_rx_buff[i] = NULL;
        }

        if (dma_loop_tx_buff[i])
        {
            frcdrv_dma_block_free(dma_loop_tx_buff[i]);
            dma_loop_tx_buff[i] = NULL;
        }
        //FRCDRV_DEBUG("%d: dma_loop_rx_buff %p, dma_loop_tx_buff %p\n", i, dma_loop_rx_buff[i], dma_loop_tx_buff[i]);

    }
#endif
}

int frcdrv_dma_init()
{
#if FRC_CONFIG_DMA_TEST
    int i;
    for (i = 0; i < FRC_DAT_CORE_NUM; i++)
    {
        dma_loop_rx_buff[i] = frcdrv_dma_block_alloc();
        if (dma_loop_rx_buff[i] == NULL)
        {
            FRCDRV_ERROR("Alloc dma loop rx buff %d fail!\n", i);
            return FRE_MEMORY;
        }
        memset(dma_loop_rx_buff[i], 0, FRC_DMA_BUFF_SIZE);
        dma_loop_tx_buff[i] = frcdrv_dma_block_alloc();
        if (dma_loop_tx_buff[i] == NULL)
        {
            FRCDRV_ERROR("Alloc dma loop tx buff %d fail!\n", i);
            return FRE_MEMORY;
        }
        memset(dma_loop_tx_buff[i], 0, FRC_DMA_BUFF_SIZE);
        FRCDRV_DEBUG("%d: dma_loop_rx_buff %p, dma_loop_tx_buff %p\n", i, dma_loop_rx_buff[i], dma_loop_tx_buff[i]);
    }
#endif
    return FRE_SUCCESS;
}

/* $Id: frc_test.c 53432 2010-09-29 00:49:02Z panicker $ */

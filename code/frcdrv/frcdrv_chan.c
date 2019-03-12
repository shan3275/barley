#include "frcdrv.h"
#include "frcdrv_chan.h"
#include "frcdrv_dma.h"
#include "frcdrv_cmd.h"

#if FRC_CONFIG_SIMPLE_PACKAGE
frc_dma_simple_package_chan_t *frcdrv_dma_simple_package_chans = NULL;

frc_dma_simple_package_chan_t *
frcdrv_simple_pakcage_chan_get(uint32_t chan_id)
{
    if (chan_id >= FRC_DMA_SIMPLE_PACKAGE_CHAN_NUM)
    {
        return NULL;
    }

    if (frcdrv_dma_simple_package_chans == NULL)
    {
        return NULL;
    }

    return &frcdrv_dma_simple_package_chans[chan_id];
}

void frcdrv_simple_package_chans_free(frc_dma_simple_package_chan_t *chan)
{
    int i;
    frc_dma_simple_package_pool_t *pool = chan->pool;
    if (pool)
    {
        for (i = 0; i < pool->pool_num; i++)
        {
            frcdrv_dma_pool_free(__va(pool->pool_addr[i]));
        }
        frcdrv_dma_pool_free(pool);
        chan->pool = NULL;
    }

    if (chan->dma_ctrl)
    {
        frcdrv_dma_pool_free(chan->dma_ctrl);
    }
    chan->dma_ctrl = NULL;
}

int
frcdrv_simple_package_chan_avail_init(frc_dma_simple_package_chan_t *chan)
{
    uint64_t block_addr, pool_id, block_offset;
    frc_dma_simple_package_pool_t *pool = NULL;

    pool = chan->pool;
    if (pool == NULL)
    {
        FRCDRV_ERROR("pool is NULL.\n");
        return FRE_INIT;
    }

    for (pool_id = 0; pool_id < pool->pool_num; pool_id++)
    {
        for (block_offset = 0; block_offset < FRC_DMA_POOL_SIZE; block_offset+=FRC_DMA_SIMPLE_PACKAGE_BLOCK_SIZE)
        {
            block_addr  = pool->pool_addr[pool_id] + block_offset;
            if (!SIMPLE_PACKAGE_AVAIL_RING_PUT(*chan->dma_ctrl, block_addr))
            {
                FRCDRV_ERROR("Available ring put fail!\n");
                return FRE_INIT;
            }
            //FRCDRV_DEBUG("chan[%llx]: ring_buff[%llx] = 0x%llx\n", (ULL)chan->desc.type, (ULL) (SWAP_8_BYTE(chan->dma_ctrl->available_ring.widx)-1)%FRC_SIMPLE_RING_BUFF_SIZE,
            //             (ULL) chan->dma_ctrl->available_ring.ring_buff[(SWAP_8_BYTE(chan->dma_ctrl->available_ring.widx)-1)%FRC_SIMPLE_RING_BUFF_SIZE]);
        }
    }
            FRCDRV_DEBUG("chan[%llx]: available_ring.widx = 0x%llx\n", (ULL)chan->desc.type, (ULL) chan->dma_ctrl->available_ring.widx);
            FRCDRV_DEBUG("chan[%llx]: available_ring.ridx = 0x%llx\n", (ULL)chan->desc.type, (ULL) chan->dma_ctrl->available_ring.ridx);
            FRCDRV_DEBUG("chan[%llx]: little endian available_ring.widx = 0x%llx\n", (ULL)chan->desc.type, (ULL) SWAP_8_BYTE(chan->dma_ctrl->available_ring.widx));
            FRCDRV_DEBUG("chan[%llx]: little endian available_ring.ridx = 0x%llx\n", (ULL)chan->desc.type, (ULL) SWAP_8_BYTE(chan->dma_ctrl->available_ring.ridx));
    return FRE_SUCCESS;
}

int
frcdrv_simple_package_chan_dma_ctrl_init(frc_dma_simple_package_chan_t *chan)
{
    uint64_t ctrl_addr;
    if (chan->pool == NULL)
    {
        FRCDRV_ERROR("chan->pool is NULL!\n");
        return FRE_INIT;
    }

    chan->dma_ctrl = frcdrv_dma_pool_alloc();
    if (chan->dma_ctrl == NULL)
    {
        FRCDRV_ERROR("Malloc dma ctrl fail!\n");
        return FRE_MEMORY;
    }
    memset(chan->dma_ctrl, 0, sizeof(frc_dma_simple_package_chan_ctrl_t));
    chan->desc.ctrl_size = sizeof(frc_dma_simple_package_chan_ctrl_t);
    ctrl_addr = __pa(chan->dma_ctrl);
    chan->desc.ctrl_addr = ctrl_addr;

    FRCDRV_DEBUG("chan[%llx]: chan->dma_ctrl = %p\n", (ULL)chan->desc.type, chan->dma_ctrl);
    FRCDRV_DEBUG("chan[%llx]: chan->desc.ctrl_addr = 0x%llx\n", (ULL)chan->desc.type, (ULL) chan->desc.ctrl_addr);
    FRCDRV_DEBUG("chan[%llx]: chan->desc.ctrl_size = 0x%llx\n", (ULL)chan->desc.type, (ULL) chan->desc.ctrl_size);
    FRCDRV_DEBUG("chan[%llx]: sizeof(frc_dma_simple_package_chan_ctrl_t) = 0x%llx\n", (ULL)chan->desc.type,
                 (ULL)sizeof(frc_dma_simple_package_chan_ctrl_t));
    FRCDRV_DEBUG("chan[%llx]: available_ring.widx = 0x%llx\n", (ULL)chan->desc.type, (ULL) chan->dma_ctrl->available_ring.widx);
    FRCDRV_DEBUG("chan[%llx]: available_ring.ridx = 0x%llx\n", (ULL)chan->desc.type, (ULL) chan->dma_ctrl->available_ring.ridx);
    FRCDRV_DEBUG("chan[%llx]: completed_ring.widx = 0x%llx\n", (ULL)chan->desc.type, (ULL) chan->dma_ctrl->completed_ring.widx);
    FRCDRV_DEBUG("chan[%llx]: completed_ring.ridx = 0x%llx\n", (ULL)chan->desc.type, (ULL) chan->dma_ctrl->completed_ring.ridx);

    return FRE_SUCCESS;
}

int
frcdrv_simple_package_chan_pool_init(frc_dma_simple_package_chan_t *chan, uint64_t pool_num)
{
    int i;
    void *buff = NULL;
    frc_dma_simple_package_pool_t *pool;
    uint64_t addr_min, addr_max;

    pool = frcdrv_dma_pool_alloc();
    if (pool == NULL)
    {
        FRCDRV_ERROR("Malloc buf_pool fail!\n");
        return FRE_MEMORY;
    }
    memset(pool, 0, sizeof(frc_dma_simple_package_pool_t));

    chan->pool = pool;
    pool->pool_num = pool_num;

    FRCDRV_DEBUG("chan->pool=%p,chan->pool->pool_num=%llu\n",
                 chan->pool, chan->pool->pool_num);

    for (i = 0; i < pool->pool_num ; i++)
    {
        buff = frcdrv_dma_pool_alloc();
        if (buff  == NULL)
        {
            FRCDRV_ERROR("Malloc dma buffer %d fail!\n", i);
            return FRE_MEMORY;
        }
        pool->pool_addr[i] = __pa(buff);
        FRCDRV_DEBUG("buff=%p,pool->pool_addr[%d]=%llx\n",buff, i, (ULL)pool->pool_addr[i]);
    }
    /* find max addr and min addr */
    addr_min = pool->pool_addr[0];
    addr_max = pool->pool_addr[0];
    for (i = 1; i < pool->pool_num; i++)
    {
        if (addr_min > pool->pool_addr[i])
        {
            addr_min = pool->pool_addr[i];
        }
        if (addr_max < pool->pool_addr[i])
        {
            addr_max = pool->pool_addr[i];
        }
    }
    chan->desc.pool_addr = addr_min;
    chan->desc.pool_size = addr_max - addr_min + FRC_DMA_POOL_SIZE;
    FRCDRV_DEBUG("pool_addr_min=%llx\n", (ULL)addr_min);
    FRCDRV_DEBUG("pool_addr_max=%llx\n", (ULL)addr_max);
    FRCDRV_DEBUG("chan->desc.pool_addr=%llx, chan->desc.pool_size=%llx\n", chan->desc.pool_addr, chan->desc.pool_size);
    return FRE_SUCCESS;
}

int
frcdrv_simple_package_chan_setup(frc_dma_simple_package_chan_t *chan, uint32_t type, uint64_t pool_num)
{
    int rv;
    if (pool_num > FRC_DMA_SIMPLE_PACKAGE_POOL_NUM_MAX)
    {
        return FRE_PARAM;
    }

    rv = frcdrv_simple_package_chan_pool_init(chan, pool_num);
    if (rv)
    {
        FRCDRV_ERROR("frcdrv_queue_buff_pool_init fail: %d.\n", rv);
        return FRE_FAIL;
    }

    rv = frcdrv_simple_package_chan_dma_ctrl_init(chan);
    if (rv)
    {
        FRCDRV_ERROR("frcdrv_queue_dma_ctrl_init fail: %d.\n", rv);
        return FRE_FAIL;
    }

    rv = frcdrv_simple_package_chan_avail_init(chan);
    if (rv)
    {
        FRCDRV_ERROR("frcdrv_queue_avail_init fail: %d.\n", rv);
        return FRE_FAIL;
    }

    chan->desc.type = type;

    return FRE_SUCCESS;
}

int
frcdrv_simple_package_chan_set_core(octeon_device_t *oct, frc_dma_simple_package_chan_t *chan)
{
    int rv;
    rv = frcore_set(oct, CMD_TYPE_CTRL, CTRL_CMD_SETUP_SIMPLE_PACKAGE_CHAN, sizeof(frc_dma_chan_desc_t), &chan->desc);
    FRCDRV_DEBUG("chan %lld setup: ctrl_size = %llx, app_addr 0x%llx , rv = %d.\n",
                 (ULL) chan->desc.type, (ULL) chan->desc.ctrl_size,
                 (ULL) chan->desc.ctrl_addr, rv);
    if (rv != FRE_SUCCESS) {
        FRCDRV_ERROR("CTRL_CMD_SET_SIMPLE_PACKAGE_CHAN  execute fail: rv = %d.\n", rv);
        return rv;
    }

    return FRE_SUCCESS;
}

void
frcdrv_simple_package_chan_destroy(void)
{
    int i;
    if (frcdrv_dma_simple_package_chans)
    {
        for (i = 0; i < FRC_DMA_SIMPLE_PACKAGE_CHAN_NUM; i++)
        {
            frcdrv_simple_package_chans_free(&frcdrv_dma_simple_package_chans[i]);
        }
        kfree(frcdrv_dma_simple_package_chans);
        frcdrv_dma_simple_package_chans = NULL;
    }
}

int
frcdrv_simple_package_chan_init(void)
{
    frcdrv_dma_simple_package_chans = kmalloc(sizeof(frc_dma_simple_package_chan_t) * FRC_DMA_SIMPLE_PACKAGE_CHAN_NUM, GFP_KERNEL);
    if (frcdrv_dma_simple_package_chans == NULL)
    {
        FRCDRV_ERROR("Malloc simple package dma ctrl fail!\n");
        return FRE_MEMORY;
    }
    memset(frcdrv_dma_simple_package_chans, 0, sizeof(frc_dma_simple_package_chan_t) * FRC_DMA_SIMPLE_PACKAGE_CHAN_NUM);

    frcdrv_dma_simple_package_chans[0].desc.type = FRC_WORK_UDP;
    frcdrv_dma_simple_package_chans[1].desc.type = FRC_WORK_RULE;


    FRCDRV_DEBUG("frcdrv_dma_simple_package_chans:%p,"
                 "sizeof(frc_dma_simple_package_chan_t):%lld, sizeof(frc_dma_simple_package_chans):%lld\n",
                 frcdrv_dma_simple_package_chans, (ULL)sizeof(frc_dma_simple_package_chan_t),
                 (ULL)sizeof(frcdrv_dma_simple_package_chans));
    FRCDRV_DEBUG("frcdrv_dma_simple_package_chans[0].desc.type=%llx, "
                 "frcdrv_dma_simple_package_chans[1].desc.type=%llx\n",
                 frcdrv_dma_simple_package_chans[0].desc.type, frcdrv_dma_simple_package_chans[1].desc.type);

    return FRE_SUCCESS;
}

#endif


#if FRC_CONFIG_SSN_CHAN
frc_dma_ssn_chan_t *frcdrv_dma_ssn_chans = NULL;

frc_dma_ssn_chan_t *
frcdrv_ssn_chan_get(uint32_t chan_id)
{
    if (chan_id > FRC_WORK_MAX - 1)
    {
        return NULL;
    }

    if (frcdrv_dma_ssn_chans == NULL)
    {
        return NULL;
    }

    return &frcdrv_dma_ssn_chans[chan_id - FRC_WORK_SSN];
}

void frcdrv_ssn_chans_free(frc_dma_ssn_chan_t *chan)
{
    int i;
    frc_dma_ssn_pool_t *pool = chan->pool;
    if (pool)
    {
        for (i = 0; i < pool->pool_num; i++)
        {
            frcdrv_dma_pool_free(__va(pool->pool_addr[i]));
        }
        frcdrv_dma_pool_free(pool);
        chan->pool = NULL;
    }

    if (chan->available_ring)
    {
        frcdrv_dma_pool_free(chan->available_ring);
    }
    chan->available_ring = NULL;

    if (chan->completed_ring)
    {
        frcdrv_dma_pool_free(chan->completed_ring);
    }
    chan->completed_ring = NULL;
}

int
frcdrv_ssn_chan_avail_init(frc_dma_ssn_chan_t *chan)
{
    uint64_t block_addr, pool_id, block_offset;
    frc_dma_ssn_pool_t *pool = NULL;

    pool = chan->pool;
    if (pool == NULL)
    {
        FRCDRV_ERROR("pool is NULL.\n");
        return FRE_INIT;
    }

    for (pool_id = 0; pool_id < pool->pool_num; pool_id++)
    {
        for (block_offset = 0; block_offset < FRC_DMA_POOL_SIZE; block_offset+=FRC_DMA_SSN_BLOCK_SIZE)
        {
            block_addr  = pool->pool_addr[pool_id] + block_offset;
            if (!SSN_AVAIL_RING_PUT(*chan, block_addr))
            {
                FRCDRV_ERROR("Available ring put fail!\n");
                return FRE_INIT;
            }
            //FRCDRV_DEBUG("chan[%llx]: ring_buff[%llx] = 0x%llx\n", (ULL)chan->desc.type, (ULL) (SWAP_8_BYTE(chan->available_ring->widx)-1)%FRC_DMA_SSN_RING_BUFF_SIZE,
              //           (ULL) chan->available_ring->buff[(SWAP_8_BYTE(chan->available_ring->widx)-1)%FRC_DMA_SSN_RING_BUFF_SIZE]);
        }
    }

    FRCDRV_DEBUG("chan[%llx]: available_ring.widx = 0x%llx\n", (ULL)chan->desc.type, (ULL) chan->available_ring->widx);
    FRCDRV_DEBUG("chan[%llx]: available_ring.ridx = 0x%llx\n", (ULL)chan->desc.type, (ULL) chan->available_ring->ridx);
    FRCDRV_DEBUG("chan[%llx]: little endian available_ring.widx = 0x%llx\n", (ULL)chan->desc.type, (ULL) SWAP_8_BYTE(chan->available_ring->widx));
    FRCDRV_DEBUG("chan[%llx]: little endian available_ring.ridx = 0x%llx\n", (ULL)chan->desc.type, (ULL) SWAP_8_BYTE(chan->available_ring->ridx));
    return FRE_SUCCESS;
}

int
frcdrv_ssn_chan_dma_ctrl_init(frc_dma_ssn_chan_t *chan)
{
    if (chan->pool == NULL)
    {
        FRCDRV_ERROR("chan->pool is NULL!\n");
        return FRE_INIT;
    }

    /* init avail ring */
    chan->available_ring = frcdrv_dma_pool_alloc();
    if (chan->available_ring == NULL)
    {
        FRCDRV_ERROR("Malloc dma available_ring fail!\n");
        return FRE_MEMORY;
    }
    memset(chan->available_ring, 0, sizeof(frc_ssn_ring_buff_t));
    chan->desc.avail_size = sizeof(frc_ssn_ring_buff_t);
    chan->desc.avail_addr = __pa(chan->available_ring);

    /* init compl ring */
    chan->completed_ring = frcdrv_dma_pool_alloc();
    if (chan->completed_ring == NULL)
    {
        FRCDRV_ERROR("Malloc dma completed_ring fail!\n");
        return FRE_MEMORY;
    }
    memset(chan->completed_ring, 0, sizeof(frc_ssn_ring_buff_t));
    chan->desc.compl_size = sizeof(frc_ssn_ring_buff_t);
    chan->desc.compl_addr = __pa(chan->completed_ring);

    FRCDRV_DEBUG("chan[%llx]: chan->available_ring = %p\n", (ULL)chan->desc.type, chan->available_ring);
    FRCDRV_DEBUG("chan[%llx]: chan->desc.avail_addr = 0x%llx\n", (ULL)chan->desc.type, (ULL) chan->desc.avail_addr);
    FRCDRV_DEBUG("chan[%llx]: chan->desc.avail_size = 0x%llx\n", (ULL)chan->desc.type, (ULL) chan->desc.avail_size);
    FRCDRV_DEBUG("chan[%llx]: chan->completed_ring = %p\n", (ULL)chan->desc.type, chan->completed_ring);
    FRCDRV_DEBUG("chan[%llx]: chan->desc.compl_addr = 0x%llx\n", (ULL)chan->desc.type, (ULL) chan->desc.compl_addr);
    FRCDRV_DEBUG("chan[%llx]: chan->desc.compl_size = 0x%llx\n", (ULL)chan->desc.type, (ULL) chan->desc.compl_size);
    FRCDRV_DEBUG("chan[%llx]: sizeof(frc_ssn_ring_buff_t) = 0x%llx\n", (ULL)chan->desc.type,
                 (ULL)sizeof(frc_ssn_ring_buff_t));
    FRCDRV_DEBUG("chan[%llx]: available_ring.widx = 0x%llx\n", (ULL)chan->desc.type, (ULL) chan->available_ring->widx);
    FRCDRV_DEBUG("chan[%llx]: available_ring.ridx = 0x%llx\n", (ULL)chan->desc.type, (ULL) chan->available_ring->ridx);
    FRCDRV_DEBUG("chan[%llx]: completed_ring.widx = 0x%llx\n", (ULL)chan->desc.type, (ULL) chan->completed_ring->widx);
    FRCDRV_DEBUG("chan[%llx]: completed_ring.ridx = 0x%llx\n", (ULL)chan->desc.type, (ULL) chan->completed_ring->ridx);

    return FRE_SUCCESS;
}

int
frcdrv_ssn_chan_pool_init(frc_dma_ssn_chan_t *chan, uint64_t pool_num)
{
    int i;
    void *buff = NULL;
    frc_dma_ssn_pool_t *pool;
    uint64_t addr_min, addr_max;

    pool = frcdrv_dma_pool_alloc();
    if (pool == NULL)
    {
        FRCDRV_ERROR("Malloc buf_pool fail!\n");
        return FRE_MEMORY;
    }
    memset(pool, 0, sizeof(frc_dma_ssn_pool_t));

    chan->pool = pool;
    pool->pool_num = pool_num;

    FRCDRV_DEBUG("chan->pool=%p,chan->pool->pool_num=%llu\n",
                 chan->pool, chan->pool->pool_num);

    for (i = 0; i < pool->pool_num ; i++)
    {
        buff = frcdrv_dma_pool_alloc();
        if (buff  == NULL)
        {
            FRCDRV_ERROR("Malloc dma buffer %d fail!\n", i);
            return FRE_MEMORY;
        }
        pool->pool_addr[i] = __pa(buff);
        FRCDRV_DEBUG("buff=%p,pool->pool_addr[%d]=%llx\n",buff, i, (ULL)pool->pool_addr[i]);
    }
    /* find max addr and min addr */
    addr_min = pool->pool_addr[0];
    addr_max = pool->pool_addr[0];
    for (i = 1; i < pool->pool_num; i++)
    {
        if (addr_min > pool->pool_addr[i])
        {
            addr_min = pool->pool_addr[i];
        }
        if (addr_max < pool->pool_addr[i])
        {
            addr_max = pool->pool_addr[i];
        }
    }
    chan->desc.pool_addr = addr_min;
    chan->desc.pool_size = addr_max - addr_min + FRC_DMA_POOL_SIZE;
    FRCDRV_DEBUG("pool_addr_min=%llx\n", (ULL)addr_min);
    FRCDRV_DEBUG("pool_addr_max=%llx\n", (ULL)addr_max);
    FRCDRV_DEBUG("chan->desc.pool_addr=%llx, chan->desc.pool_size=%llx\n", chan->desc.pool_addr, chan->desc.pool_size);
    return FRE_SUCCESS;
}

int
frcdrv_ssn_chan_setup(frc_dma_ssn_chan_t *chan, uint32_t type, uint64_t pool_num)
{
    int rv;
    if (pool_num > FRC_DMA_SSN_POOL_NUM_MAX)
    {
        return FRE_PARAM;
    }

    rv = frcdrv_ssn_chan_pool_init(chan, pool_num);
    if (rv)
    {
        FRCDRV_ERROR("frcdrv_ssn_chan_pool_init fail: %d.\n", rv);
        return FRE_FAIL;
    }

    rv = frcdrv_ssn_chan_dma_ctrl_init(chan);
    if (rv)
    {
        FRCDRV_ERROR("frcdrv_ssn_chan_dma_ctrl_init fail: %d.\n", rv);
        return FRE_FAIL;
    }

    rv = frcdrv_ssn_chan_avail_init(chan);
    if (rv)
    {
        FRCDRV_ERROR("frcdrv_ssn_chan_avail_init fail: %d.\n", rv);
        return FRE_FAIL;
    }

    chan->desc.type = type;

    return FRE_SUCCESS;
}

int
frcdrv_ssn_chan_set_core(octeon_device_t *oct, frc_dma_ssn_chan_t *chan)
{
    int rv;
    rv = frcore_set(oct, CMD_TYPE_CTRL, CTRL_CMD_SETUP_SSN_CHAN, sizeof(frc_dma_ssn_chan_desc_t), &chan->desc);
    FRCDRV_DEBUG("chan %lld setup: ctrl_size = %llx, avail_addr 0x%llx, compl_size = %llx, compl_addr 0x%llx, rv = %d.\n",
                 (ULL) chan->desc.type,       (ULL) chan->desc.avail_size,
                 (ULL) chan->desc.avail_addr, (ULL) chan->desc.compl_size,
                 (ULL) chan->desc.compl_addr, rv);
    if (rv != FRE_SUCCESS) {
        FRCDRV_ERROR("CTRL_CMD_SETUP_SSN_CHAN  execute fail: rv = %d.\n", rv);
        return rv;
    }

    return FRE_SUCCESS;
}


void
frcdrv_ssn_chan_destroy(void)
{
    int i;
    if (frcdrv_dma_ssn_chans)
    {
        for (i = 0; i < FRC_DMA_SSN_CHAN_NUM; i++)
        {
            frcdrv_ssn_chans_free(&frcdrv_dma_ssn_chans[i]);
        }
        kfree(frcdrv_dma_ssn_chans);
        frcdrv_dma_ssn_chans = NULL;
    }
}

int
frcdrv_ssn_chan_init(void)
{
    frcdrv_dma_ssn_chans = kmalloc(sizeof(frc_dma_ssn_chan_t) * FRC_DMA_SSN_CHAN_NUM, GFP_KERNEL);
    if (frcdrv_dma_ssn_chans == NULL)
    {
        FRCDRV_ERROR("Malloc ssn dma ctrl fail!\n");
        return FRE_MEMORY;
    }
    memset(frcdrv_dma_ssn_chans, 0, sizeof(frc_dma_ssn_chan_t) * FRC_DMA_SSN_CHAN_NUM);

    frcdrv_dma_ssn_chans[0].desc.type = FRC_WORK_SSN;

    FRCDRV_DEBUG("frcdrv_dma_ssn_chans:%p,"
                 "sizeof(frc_dma_ssn_chan_t):%lld, sizeof(frc_dma_ssn_chans):%lld\n",
                 frcdrv_dma_ssn_chans, (ULL)sizeof(frc_dma_ssn_chan_t),
                 (ULL)sizeof(frcdrv_dma_ssn_chans));
    FRCDRV_DEBUG("frcdrv_dma_ssn_chans[0].desc.type=%llx\n",
                 frcdrv_dma_ssn_chans[0].desc.type);

    return FRE_SUCCESS;
}

#endif

int
frcdrv_cmd_pool_and_ring_addr_get(frc_ioctl_t *ioctl_arg)
{
    uint64_t type;
    if (ioctl_arg->olen != sizeof(frc_dma_chan_desc_t) &&
        ioctl_arg->olen != sizeof(frc_dma_ssn_chan_desc_t) )
    {
        return FRE_PARAM;
    }


    type = *((uint64_t *) ioctl_arg->input);

    if (type > FRC_WORK_MAX)
    {
        return FRE_PARAM;
    }

    if (type == FRC_WORK_UDP || type == FRC_WORK_RULE)
    {
        #if FRC_CONFIG_SIMPLE_PACKAGE
        if (frcdrv_dma_simple_package_chans == NULL)
        {
            return FRE_INIT;
        }
        cavium_memcpy(ioctl_arg->output, &frcdrv_dma_simple_package_chans[type].desc, sizeof(frc_dma_chan_desc_t));
        ioctl_arg->olen = sizeof(frc_dma_chan_desc_t);
        #endif
    }else if (type == FRC_WORK_SSN)
    {
        #if FRC_CONFIG_SSN_CHAN
        if (frcdrv_dma_ssn_chans == NULL)
        {
            return FRE_INIT;
        }
        cavium_memcpy(ioctl_arg->output, &frcdrv_dma_ssn_chans[type - FRC_WORK_SSN].desc, sizeof(frc_dma_ssn_chan_desc_t));
        ioctl_arg->olen = sizeof(frc_dma_ssn_chan_desc_t);
        #endif
    }

    return FRE_SUCCESS;
}

/* End of file */

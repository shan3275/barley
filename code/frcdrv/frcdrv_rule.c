//#include <linux/wrapper.h>
#include <asm/page.h>
#include <linux/slab.h>
#include <linux/proc_fs.h>
#include "frcdrv.h"
#include "frcdrv_udp.h"
#include "frc_pack.h"
#include "frc_dma.h"
#include "frcdrv_cmd.h"
#include "frcdrv_network.h"
#include "frcdrv_dma.h"
#include "frcdrv_chan.h"

#if FRC_CONFIG_RULE
int frcdrv_rule_chan_init()
{
    frc_dma_simple_package_chan_t *chan;
    chan = frcdrv_simple_pakcage_chan_get(FRC_WORK_RULE);

    if (chan == NULL)
    {
        FRCDRV_ERROR("RULE chan init fail!\n");
        return FRE_FAIL;
    }

    if(frcdrv_simple_package_chan_setup(chan, FRC_WORK_RULE, FRC_DMA_SIMPLE_PACKAGE_POOL_NUM_MAX))
    {
        FRCDRV_ERROR("RULE chan init fail!\n");
        return FRE_FAIL;
    }

    return FRE_SUCCESS;
}

int frcdrv_rule_chan_start(octeon_device_t *oct)
{
    frc_dma_simple_package_chan_t *chan;

    /* Setup udp dma chan */
    chan = frcdrv_simple_pakcage_chan_get(FRC_WORK_RULE);
    if (chan == NULL)
    {
        FRCDRV_ERROR("RULE chan set core fail!\n");
        return FRE_FAIL;
    }
    if (frcdrv_simple_package_chan_set_core(oct, chan)) {
        FRCDRV_ERROR("RULE chan set core fail!\n");
        return FRE_FAIL;
    }

    return FRE_SUCCESS;
}

#endif
/* End of file */

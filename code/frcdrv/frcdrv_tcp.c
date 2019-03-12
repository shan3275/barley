//#include <linux/wrapper.h>
#include <asm/page.h>
#include <linux/slab.h>
#include <linux/proc_fs.h>
#include "frcdrv.h"
#include "frcdrv_tcp.h"


#include "frc_pack.h"
#include "frc_dma.h"

#include "frcdrv_cmd.h"
#include "frcdrv_network.h"
#include "frcdrv_dma.h"
#include "frcdrv_chan.h"

#if FRC_CONFIG_SSN
int frcdrv_ssn_init()
{
    frc_dma_ssn_chan_t *chan;
    chan = frcdrv_ssn_chan_get(FRC_WORK_SSN);

    if (chan == NULL)
    {
        FRCDRV_ERROR("SSN chan init fail!\n");
        return FRE_FAIL;
    }

    if(frcdrv_ssn_chan_setup(chan, FRC_WORK_SSN, FRC_DMA_SSN_POOL_NUM_MAX))
    {
        FRCDRV_ERROR("SSN chan init fail!\n");
        return FRE_FAIL;
    }

    return FRE_SUCCESS;
}

int frcdrv_ssn_start(octeon_device_t *oct)
{
    frc_dma_ssn_chan_t *chan;

    /* Setup udp dma chan */
    chan = frcdrv_ssn_chan_get(FRC_WORK_SSN);

    if (chan == NULL)
    {
        FRCDRV_ERROR("SSN chan set core fail!\n");
        return FRE_FAIL;
    }
    if (frcdrv_ssn_chan_set_core(oct, chan)) {
        FRCDRV_ERROR("SSN chan set core fail!\n");
        return FRE_FAIL;
    }

    return FRE_SUCCESS;
}

#endif
/* End of file */

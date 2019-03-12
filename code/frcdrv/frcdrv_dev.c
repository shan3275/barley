#include "frcdrv.h"
#include "frc_ioctl.h"
#include "frcdrv_dev.h"
#include "frcdrv_dma.h"

#include "frcdrv_tcp.h"
#include "frcdrv_udp.h"
#include "frcdrv_rule.h"
#include "frcdrv_cmd.h"
#include "frcdrv_chan.h"

int frc_device_major = 0;
extern void frcdrv_version_get(void *version);

int frcdrv_port_start(octeon_device_t *oct)
{
    int rv = frcore_set(oct, CMD_TYPE_CTRL, CTRL_CMD_PORT_ENABLE, 0, NULL);
    if (rv != FRE_SUCCESS) {
        FRCDRV_ERROR("CTRL_CMD_PORT_ENABLE  execute fail: rv = %d.\n", rv);
        return rv;
    }

    return FRE_SUCCESS;
}

int frcdrv_start(int octeon_id, octeon_device_t *octeon_dev)
{
    FRCDRV_DEBUG("octeon_id = %d, octeon_dev = %p\n", octeon_id, octeon_dev);

    if (frcdrv_cmd_init(octeon_id))
    {
        FRCDRV_ERROR("CMD init fail!\n");
        return 1;
    }
#if FRC_CONFIG_DMA_TEST
    if (frcdrv_dma_start(octeon_dev))
    {
        FRCDRV_ERROR("start ssn fail!\n");
        return 1;
    }
#else

#endif
#if FRC_CONFIG_SIMPLE_PACKAGE
#if FRC_CONFIG_UDP
    if (frcdrv_udp_chan_start(octeon_dev))
    {
        FRCDRV_ERROR("start udp fail!\n");
        return 1;
    }
#endif
#if FRC_CONFIG_RULE
    if (frcdrv_rule_chan_start(octeon_dev))
    {
        FRCDRV_ERROR("start rule fail!\n");
        return 1;
    }
#endif
#endif
#if FRC_CONFIG_SSN_CHAN
#if FRC_CONFIG_SSN
    if (frcdrv_ssn_start(octeon_dev))
    {
        FRCDRV_ERROR("start ssn fail!\n");
        return 1;
    }
#endif
#endif
    if (frcdrv_port_start(octeon_dev))
    {
        FRCDRV_ERROR("start port fail!\n");
        return 1;
    }
    return 0;
}

int frcdrv_reset(int octeon_id, void *octeon_dev)
{
    FRCDRV_DEBUG("octeon_id = %d, octeon_dev = %p\n", octeon_id, octeon_dev);

    return 0;
}

int frcdrv_stop(int octeon_id, void *octeon_dev)
{
    FRCDRV_DEBUG("octeon_id = %d, octeon_dev = %p\n", octeon_id, octeon_dev);

    frcdrv_cmd_destroy(octeon_id);
    return 0;
}








/* The open() entry point for the octeon driver. This routine is called
   every time the /dev/octeon_device file is opened. */
int
frcdrv_open (struct inode *inode, struct file *file)
{
    CVM_MOD_INC_USE_COUNT;
    return 0;
}


int
frcdrv_cmd_get_version(frc_ioctl_t *ioctl_arg)
{
    frc_version_t version;

    if (ioctl_arg->olen != sizeof(version))
    {
        return FRE_PARAM;
    }
    frcdrv_version_get(&version);

    cavium_memcpy(ioctl_arg->output, &version, sizeof(version));
    ioctl_arg->olen = sizeof(version);

    return FRE_SUCCESS;
}

int
frcdrv_cmd_get_feature(frc_ioctl_t *ioctl_arg)
{
    uint64_t feature = 0;


    if (ioctl_arg->olen != sizeof(uint64_t))
    {
        return FRE_PARAM;
    }

#define FEATURE_SUPPORT(_val, _feature) \
    _val |= (1 << _feature)

    FEATURE_SUPPORT(feature, FEATURE_TCP_SINGLE);
    FEATURE_SUPPORT(feature, FEATURE_OCT1_NETNIC);
#if FRC_CONFIG_LOCK_NET_XMIT
    FEATURE_SUPPORT(feature, REATURE_LOCK_XMIT);
#endif
    cavium_memcpy(ioctl_arg->output, &feature, sizeof(uint64_t));
    ioctl_arg->olen = sizeof(uint64_t);

    return FRE_SUCCESS;
}

int
frcdrv_ioctl_frcdrv(void *arg)
{
    int ret = 0;

    frc_ioctl_t *ioctl_arg = (frc_ioctl_t *) arg;
    FRCDRV_DEBUG("arg = %p\n", arg);
    switch (ioctl_arg->cmd)
    {
    case DRV_CMD_GET_VERSION:
        ret = frcdrv_cmd_get_version(ioctl_arg);
        break;
#if FRC_CONFIG_DMA_TEST
    case DRV_CMD_DMA_LOOP_BUFF_GET:
        ret = frcdrv_cmd_dma_loop_buff_get(ioctl_arg);
        break;
#endif
    case DRV_CMD_GET_FEATURE:
        ret = frcdrv_cmd_get_feature(ioctl_arg);
        break;

    case DRV_CMD_GET_POOL_AND_RING_ADDR:
        ret = frcdrv_cmd_pool_and_ring_addr_get(ioctl_arg);
        break;

    default:
        FRCDRV_ERROR("Unknown ioctl type\n");
        ret = -ENOTTY;
        break;
    }

    ioctl_arg->rv = ret;

    return 0;
}

int
frcdrv_ioctl_frcore(frc_ioctl_t *arg)
{
    octeon_device_t *oct;

    FRCDRV_DEBUG("did %d, type %d, cmd %d, ilen %d, olen %d, input %p, output %p\n",
                 arg->did, arg->type, arg->cmd, arg->ilen, arg->olen, arg->input, arg->output);

    oct = get_octeon_device_ptr(arg->did);

    if (oct == NULL)
    {
        return -ENOTTY;
    }

    arg->rv = frcdrv_core_cmd(oct, arg->type, arg->cmd, arg->ilen, arg->input, &arg->olen, arg->output);

    return 0;
}

/*
 *  Standard ioctl() entry point.
 */
int frcdrv_ioctl (struct inode   *inode,
                  struct file    *file,
                  unsigned int   cmd,
                  unsigned long  arg)
{
   int retval=0;

   FRCDRV_DEBUG("cmd = 0x%x.\n", cmd);
   switch(cmd)  {
      case IOCTL_FRCDRV_REQUEST:
           retval = frcdrv_ioctl_frcdrv((void *)arg);
           break;
      case IOCTL_FRCORE_REQUEST:
           retval = frcdrv_ioctl_frcore((void *)arg);
           break;
      default:
           FRCDRV_ERROR("Unknown ioctl command\n");
           retval = -ENOTTY;
           break;
   }  /* switch */

   return retval;
}




/* The close() entry point for the octeon driver. This routine is called
   every time the /dev/octeon_device file is closed. */
int
frcdrv_release(struct inode *inode, struct file *file)
{
    CVM_MOD_DEC_USE_COUNT;
    return 0;
}

long frc_compat_ioctl (struct file *f, unsigned int cmd, unsigned long  arg)
{
    return (long) frcdrv_ioctl (NULL, f, cmd, arg);
}



long frc_unlocked_ioctl (struct file *f,
                         unsigned int   cmd,
                         unsigned long  arg)
{
    return (long) frcdrv_ioctl (NULL, f, cmd, arg);
}


static struct file_operations frcdrv_fops =
{
   open:      frcdrv_open,
   release:   frcdrv_release,
   read:      NULL,
   write:     NULL,
   ioctl:     frcdrv_ioctl,
#if LINUX_VERSION_CODE > KERNEL_VERSION(2,6,11)
   unlocked_ioctl: frc_unlocked_ioctl,
   compat_ioctl:  frc_compat_ioctl,
#endif
   mmap:      NULL
};

int frcdrv_base_init_module(void)
{
    int ret = 0;

    ret = register_chrdev(frc_device_major, FRC_DEVICE_NAME, &frcdrv_fops);
    if(ret < 0)  {
        FRCDRV_ERROR("Device Registration failed, error:%d\n", ret);
        /* This is the error value returned by register_chrdev() */
        return ret;
    }
    if (frc_device_major == 0)
    {
        frc_device_major = ret;
    }

#if FRC_CONFIG_DMA_TEST
    if (frcdrv_dma_init())
    {
        FRCDRV_ERROR("DMA init fail!\n");
        goto frcdrv_base_init_module_err;
    }
#else

#endif
#if FRC_CONFIG_SIMPLE_PACKAGE
    if (frcdrv_simple_package_chan_init())
    {
        FRCDRV_ERROR("TCP init fail!\n");
        goto frcdrv_base_init_module_err;
    }
#if FRC_CONFIG_UDP
   if (frcdrv_udp_chan_init())
    {
        FRCDRV_ERROR("UDP init fail!\n");
        ret = 1;
        goto frcdrv_base_init_module_err;
    }
#endif
#if FRC_CONFIG_RULE
   if (frcdrv_rule_chan_init())
    {
        FRCDRV_ERROR("UDP init fail!\n");
        ret = 1;
        goto frcdrv_base_init_module_err;
    }
#endif
#endif

#if FRC_CONFIG_SSN_CHAN
    if (frcdrv_ssn_chan_init())
    {
        FRCDRV_ERROR("SSN chan init fail!\n");
        goto frcdrv_base_init_module_err;
    }
#if FRC_CONFIG_SSN
   if (frcdrv_ssn_init())
    {
        FRCDRV_ERROR("ssn init fail!\n");
        ret = 1;
        goto frcdrv_base_init_module_err;
    }
#endif
#endif
    FRCDRV_DEBUG("init success.\n");
    return 0;

frcdrv_base_init_module_err:
    unregister_chrdev(frc_device_major, FRC_DEVICE_NAME);
    return 1;
}

void frcdrv_base_exit_module(void)
{
    unregister_chrdev(frc_device_major, FRC_DEVICE_NAME);

#if FRC_CONFIG_DMA_TEST
    frcdrv_dma_destroy();
#else
#endif
#if FRC_CONFIG_SIMPLE_PACKAGE
    frcdrv_simple_package_chan_destroy();
#endif
#if FRC_CONFIG_SSN_CHAN
    frcdrv_ssn_chan_destroy();
#endif
    FRCDRV_DEBUG("device unregister..done\n");
}

/* End of file */




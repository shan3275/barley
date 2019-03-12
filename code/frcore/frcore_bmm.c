#include "frcore.h"
#include "frcore_cmd.h"
#include "frcore_misc.h"
#include "frcore_ssn.h"

#include "cvmx-mdio.h"
#include "cvmx-twsi.h"

extern unsigned int frcore_version_get();

//uint8_t loopback_mode = 1;
uint8_t dma_block_size = 8;
uint8_t loopback_oct0 = 0;
uint8_t loopback_oct1 = 0;
CVMX_SHARED uint8_t work_mode_oct0 = 0; /* 0 for flow rec; 1 for nic */
CVMX_SHARED uint8_t work_mode_oct1 = 0; /* 0 for flow rec; 1 for nic */

int loopback_process(uint8_t port, uint8_t mode)
{
    int rv;
    uint16_t phyid = 0 ,value;
    if (port == 0)
    {
        phyid = 0;
    }
    else if (port == 16)
    {
        phyid = 1;
    }
    switch(mode)
    {
        case 1:
            value = cvmx_mdio_45_read(0, phyid, 1, 0xc80a);
            value |= 1;
            rv = cvmx_mdio_45_write(0, phyid, 1, 0xc80a, value);
            break;
        case 2:
            rv = FRE_SUCCESS;
            break;
        case 3:
            value = cvmx_mdio_45_read(0, phyid, 1, 0);
            value |= 1;
            rv = cvmx_mdio_45_write(0, phyid, 1, 0, value);
            break;
        case 4:
            rv = FRE_SUCCESS;
            break;
        case 0:
            value = cvmx_mdio_45_read(0, phyid, 1, 0xc80a);
            value &= 0xfffe;
            rv = cvmx_mdio_45_write(0, phyid, 1, 0xc80a, value);
            value = cvmx_mdio_45_read(0, phyid, 1, 0);
            value &= 0xffe;
            rv = cvmx_mdio_45_write(0, phyid, 1, 0, value);
            break;
        default:
            rv = FRE_FAIL;
            break;
    }
    return rv;
}

void dma_mem_data_read(frc_bdd_dma_read_in_t *range, uint8_t *data)
{
    ;
}

int frcore_cmd_bdd_status_get(uint16_t plen, void *param, uint16_t *olen, void *outbuf)
{
    int rv;
    int value;
    uint64_t local, remote;
    frc_bdd_status_out_t bdd_status;
    memset(&bdd_status, 0 ,sizeof(frc_bdd_status_out_t));
#if 1
    rv = cvmx_twsix_read_ia(1, 0x4d, 0, 1, 1, &local);
    FRCORE_CMD("RV = %d.\n", rv);
    rv = cvmx_twsix_read_ia(1, 0x4d, 1, 1, 1, &remote);
    FRCORE_CMD("RV = %d.\n", rv);
#endif
    bdd_status.temp.local = local;
    bdd_status.temp.remote = remote;
#if 1

    value = cvmx_mdio_read(1, 11, 0x4);

    bdd_status.oct0.link = (value & 4)?1:0;;
    bdd_status.oct0.enable = 1;
    bdd_status.oct0.work_mode = work_mode_oct0;
    bdd_status.oct0.loopback_mode = loopback_oct0;
    bdd_status.oct0.optical_status = (value & 1)?1:0;;
    bdd_status.oct0.optical = 20;
    bdd_status.oct0.rx_pkts_rate = 10;
    bdd_status.oct0.rx_bytes_rate = 1000;
    bdd_status.oct0.tx_pkts_rate = 5;
    bdd_status.oct0.tx_bytes_rate = 500;

    bdd_status.oct1.link = (value & 8)?1:0;;
    bdd_status.oct1.enable = 1;
    bdd_status.oct1.work_mode = work_mode_oct1;
    bdd_status.oct1.loopback_mode = loopback_oct1;
    bdd_status.oct1.optical_status = (value & 2)?1:0;;
    bdd_status.oct1.optical = 10;
    bdd_status.oct1.rx_pkts_rate = 50;
    bdd_status.oct1.rx_bytes_rate = 5000;
    bdd_status.oct1.tx_pkts_rate = 10;
    bdd_status.oct1.tx_bytes_rate = 2000;
#endif
    //bdd_status.loopback = loopback_mode;
    bdd_status.dma_block_size = dma_block_size;
    #if FRC_CONFIG_RULE
    bdd_status.rule_max = RULE_MAX;
    #endif /* end of FRC_CONFIG_RULE */
    #if FRC_CONFIG_SSN
    bdd_status.ssn_max = SSN_TOTAL_NUM;
    #endif /* end of FRC_CONFGI_SSN */
    bdd_status.running_status = 1;

    memcpy(outbuf, &bdd_status, sizeof(frc_bdd_status_out_t));
    *olen = sizeof(frc_bdd_status_out_t);

//    for (i = 0; i < (int)sizeof(frc_bdd_status_out_t); i ++)
//    {
//        printf("0x%x\n", ((uint8_t *)outbuf)[i]);
//    }
//
    return FRE_SUCCESS;

}


int frcore_cmd_bdd_info_get(uint16_t plen, void *param, uint16_t *olen, void *outbuf)
{
    frc_bdd_info_out_t bdd_info;
    memset(&bdd_info, 0 ,sizeof(frc_bdd_info_out_t));

    frcore_version_get(&bdd_info.frcore_version);

    bdd_info.octdrv_version.major = 1;
    bdd_info.octdrv_version.minor = 5;
    bdd_info.octdrv_version.build = 100;

    bdd_info.octnic_version.major = 1;
    bdd_info.octnic_version.minor = 7;
    bdd_info.octnic_version.build = 101;

    bdd_info.cpld_version = 0x18;
    bdd_info.port_number = 2;

    memcpy(outbuf, &bdd_info, sizeof(frc_bdd_info_out_t));
    *olen = sizeof(frc_bdd_info_out_t);

    return FRE_SUCCESS;


}




int frcore_cmd_set_loopback_mode(uint16_t plen, void *param, uint16_t *olen, void *outbuf)
{
    frc_bdd_set_loopback_in_t *mode = (frc_bdd_set_loopback_in_t *) param;

    if (mode->port == 0)
    {
        loopback_oct0 = mode->loopback;
    }

    else if (mode->port == 16)
    {
        loopback_oct1 = mode->loopback;
    }

    else
    {
        return FRE_FAIL;
    }

    if (loopback_process(mode->port, mode->loopback))
    {
        return FRE_FAIL;
    }

    printf("$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$\n");
    printf("port = %d, loopback_mode = %d \n", mode->port, mode->loopback);
    printf("$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$\n");

    return FRE_SUCCESS;
}


int frcore_cmd_set_work_mode(uint16_t plen, void *param, uint16_t *olen, void *outbuf)
{
    frc_bdd_workmode_set_in_t *work_mode = (frc_bdd_workmode_set_in_t *)param;

    if (work_mode->oct == 0)
    {
        work_mode_oct0 = work_mode->mode;
    }

    else if (work_mode->oct == 16)
    {
        work_mode_oct1 = work_mode->mode;
    }

    else
    {
        return FRE_FAIL;
    }

    printf("$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$\n");
    printf("port = %d, mode = %d \n", work_mode->oct, work_mode->mode);
    printf("$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$\n");

    return FRE_SUCCESS;
}


int frcore_cmd_enable_port(uint16_t plen, void *param, uint16_t *olen, void *outbuf)
{
    frc_bdd_set_port_in_t *port_enable = (frc_bdd_set_port_in_t *)param;

    printf("$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$\n");
    printf("port = %d, enable = %d \n", port_enable->port, port_enable->enable);
    printf("$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$\n");
    if (port_enable->port == 0 || port_enable->port == 16)
    {
        octnic->port[port_enable->port].rx_on = port_enable->enable;
    }else {
        return FRE_FAIL;
    }

    return FRE_SUCCESS;

}


int frcore_cmd_force_link(uint16_t plen, void *param, uint16_t *olen, void *outbuf)
{
    frc_bdd_force_link_in_t *force_link = (frc_bdd_force_link_in_t *)param;

    printf("$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$\n");
    printf("port = %d, link = %d \n", force_link->oct, force_link->link);
    printf("$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$\n");

    return FRE_SUCCESS;
}


int frcore_cmd_log_set(uint16_t plen, void *param, uint16_t *olen, void *outbuf)
{
    frc_bdd_log_in_t *log_info = (frc_bdd_log_in_t *)param;

    printf("$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$\n");
    printf("log = %d, enable = %d \n", log_info->log, log_info->enable);
    printf("$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$\n");

    return FRE_SUCCESS;
}


int frcore_cmd_poweroff(uint16_t plen, void *param, uint16_t *olen, void *outbuf)
{
    int rv;
    rv = cvmx_mdio_write(1, 11, 0xd, 2);
    if (0 != rv)
    {
        FRCORE_ERROR("Poweroff the frc card fail!\n");
        return FRE_FAIL;
    }

    return FRE_SUCCESS;
}


int frcore_cmd_dma_block_set(uint16_t plen, void *param, uint16_t *olen, void *outbuf)
{
    uint8_t *size = (uint8_t *)param;

    dma_block_size = *size;

    return FRE_SUCCESS;
}


int frcore_cmd_dma_data_read(uint16_t plen, void *param, uint16_t *olen, void *outbuf)
{
    int i;
    frc_bdd_dma_read_in_t *range = (frc_bdd_dma_read_in_t *)param;
    uint8_t data[256];
    //dma_mem_data_read(range, data);

    printf("size = %d ^^^^^^^^^^^^^^^^^^^^^^^^^^^^\n", range->size);

    for (i = 0; i<(int)range->size;i++)
    {
        data[i] = i +1;
    }

    memcpy(outbuf, data, range->size);
    *olen = range->size;

    for (i = 0; i < (int)range->size; i++)
    {
        printf("0x%.2x\n", ((uint8_t *)outbuf)[i]);
    }

    return FRE_SUCCESS;
}


int
frcore_bmm_init()
{
    FRCORE_REGISTER_CMD(CMD_TYPE_USER, USER_CMD_GET_BDD_STATUS, frcore_cmd_bdd_status_get);
    FRCORE_REGISTER_CMD(CMD_TYPE_USER, USER_CMD_GET_BDD_INFO,  frcore_cmd_bdd_info_get);
    FRCORE_REGISTER_CMD(CMD_TYPE_USER, USER_CMD_SET_LOOPBACK_MODE,  frcore_cmd_set_loopback_mode);
    FRCORE_REGISTER_CMD(CMD_TYPE_USER, USER_CMD_SET_WORK_MODE,  frcore_cmd_set_work_mode);
    FRCORE_REGISTER_CMD(CMD_TYPE_USER, USER_CMD_BDD_PORT_ENABLE,  frcore_cmd_enable_port);
    FRCORE_REGISTER_CMD(CMD_TYPE_USER, USER_CMD_FORCE_LINK,  frcore_cmd_force_link);
    FRCORE_REGISTER_CMD(CMD_TYPE_USER, USER_CMD_CONFIG_LOG_SW,  frcore_cmd_log_set);
    FRCORE_REGISTER_CMD(CMD_TYPE_USER, USER_CMD_POWEROFF,  frcore_cmd_poweroff);
    FRCORE_REGISTER_CMD(CMD_TYPE_USER, USER_CMD_SET_DMA_BLOCK_SIZE,  frcore_cmd_dma_block_set);
    FRCORE_REGISTER_CMD(CMD_TYPE_USER, USER_CMD_READ_DMA,  frcore_cmd_dma_data_read);

    return 0;
}

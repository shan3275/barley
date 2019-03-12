#include <stdlib.h>
#include <string.h>

#include "frctweak.h"
#include "frc_pack.h"
#include "frc_dma.h"
#include "cvmx-swap.h"
#include "frc_api.h"
#include "frc_debug.h"

#define HSTRTU16(_str, _val) \
{ \
    char *e; \
    unsigned int _vali;\
    if (_str != NULL) { \
        _vali = strtoul(_str, &e, 16); \
        if (e == _str || *e != '\0') { \
            printf("Error: invail format!\n"); \
            return FRE_PARAM; \
        } \
        if (_vali > 0xFFFF) \
        { \
            return FRE_PARAM; \
        } \
        _val = _vali & 0xFFFF; \
    } \
}

#if 1
int parse_oct(char *str, uint8_t *value)
{
    if (!strcasecmp(str, "oct0"))
    {
        *value = 0;
        return 0;
    }

    if (!strcasecmp(str, "oct1"))
    {
        *value = 16;
        return 0;
    }

    return 1;

}
#endif

void debug_loopback_mode(uint8_t loopback)
{
    switch(loopback)
    {
        case 1:
            printf("line-phy\n");
            break;
        case 2:
            printf("line-core\n");
            break;
        case 3:
            printf("host-phy\n");
            break;
        case 4:
            printf("host-core\n");
            break;
        case 0:
            printf("normal\n");
            break;
        default:
            printf("Invilid loopback mode\n");
            break;

    }
}

int board_info_get(void)
{
    int rv;
    frc_bdd_info_out_t bdd_info;
    char buf[sizeof(frc_bdd_info_out_t)], *p;

    p = buf;

    memset(&bdd_info, 0, sizeof(frc_bdd_info_out_t));

    rv = frcapi_bdd_info_get((frc_bdd_info_out_t *)buf);
    if (rv != FRE_SUCCESS)
    {
        FRCTWEAK_ERROR("Get information of board fail: %d!\n", rv);
        return rv;
    }

    p = frc_bdd_info_unpack(p, &bdd_info);

    frctweak_version_get(&bdd_info.frctweak_version);

    rv = frcapi_frcdrv_version_get(&bdd_info.frcdrv_version);
    if (rv != FRE_SUCCESS)
    {
        FRCTWEAK_ERROR("Get version of frcdrv fail: %d!\n", rv);
        return rv;
    }

    printf("frcore version: V%d.%d.%d\n", bdd_info.frcore_version.major, bdd_info.frcore_version.minor, bdd_info.frcore_version.build);
    printf("frctweak version: V%d.%d.%d\n", bdd_info.frctweak_version.major, bdd_info.frctweak_version.minor, bdd_info.frctweak_version.build);
    printf("frcdrv version: V%d.%d.%d\n", bdd_info.frcdrv_version.major, bdd_info.frcdrv_version.minor, bdd_info.frcdrv_version.build);
    printf("octeon_drv version: V%d.%d.%d\n", bdd_info.octdrv_version.major, bdd_info.octdrv_version.minor, bdd_info.octdrv_version.build);
    printf("octnic version: V%d.%d.%d\n", bdd_info.octnic_version.major, bdd_info.octnic_version.minor, bdd_info.octnic_version.build);
    printf("cpld version: %d\n", bdd_info.cpld_version);
    printf("port number: %d\n", bdd_info.port_number);

    return FRE_SUCCESS;
}



int board_status_get(void)
{
    int rv;
    frc_bdd_status_out_t bdd_status;
    char buf[sizeof(frc_bdd_status_out_t)], *p;

    p = buf;

    memset(&bdd_status, 0 , sizeof(frc_bdd_status_out_t));
    rv = frcapi_bdd_status_get((frc_bdd_status_out_t *)buf);
    if (rv != FRE_SUCCESS)
    {
        FRCTWEAK_ERROR("Get status of board fail: %d!\n", rv);
        return rv;
    }


    p = frc_bdd_status_unpack(p, &bdd_status);
    //swap_buff(16 >> 3, &bdd_status.temp);
#if 0
    for (i = 0; i<sizeof(frc_bdd_status_out_t); i ++)
    {
        printf(" 0x%.2x\n", ((uint8_t *)(&bdd_status))[i]);
    }

    printf("\n");


    swap_buff(16 >> 3, &bdd_status.temp);
    swap_4_buff(1, &bdd_status.rule_max);


    for (i = 0; i<sizeof(frc_bdd_status_out_t); i ++)
    {
        printf(" 0x%.2x\n", ((uint8_t *)(&bdd_status))[i]);
    }
#endif
    printf("local temperature: %lld\n", (ULL) bdd_status.temp.local);
    printf("remote temperature: %lld\n", (ULL) bdd_status.temp.remote);

    printf("dma block size: %d kb\n", bdd_status.dma_block_size);
    printf("board running status: %s\n", bdd_status.running_status?"normal":"abnormal");
    printf("ssn max: %d\n", bdd_status.ssn_max);
    printf("rule max: %d\n", bdd_status.rule_max);
#if 1
    printf("oct0 link status: %s\n", bdd_status.oct0.link?"up":"down");
    printf("oct0 work mode: %s\n", bdd_status.oct0.work_mode?"NICMODE":"FRMODE");
    //printf("oct0 enable status: %s\n", bdd_status.oct0.enable?"ENABLED":"DISABLED");
    printf("oct0 loopback mode: ");
    debug_loopback_mode(bdd_status.oct0.loopback_mode);
    printf("oct0 optical transceiver status: %s\n", bdd_status.oct0.optical_status?"ON":"OFF");
    //printf("oct0 optical: %d\n", bdd_status.oct0.optical);
    //printf("oct0 rx pkts rate: %d pps\n", bdd_status.oct0.rx_pkts_rate);
    //printf("oct0 rx bytes rate: %d bps\n", bdd_status.oct0.rx_bytes_rate);
    //printf("oct0 tx pkts rate: %d pps\n", bdd_status.oct0.tx_pkts_rate);
    //printf("oct0 tx bytes rate: %d bps\n", bdd_status.oct0.tx_bytes_rate);

    printf("oct1 link status: %s\n", bdd_status.oct1.link?"up":"down");
    printf("oct1 work mode: %s\n", bdd_status.oct1.work_mode?"NICMODE":"FRMODE");
    //printf("oct1 enable status: %s\n", bdd_status.oct1.enable?"ENABLED":"DISABLED");
    printf("oct1 loopback mode: ");
    debug_loopback_mode(bdd_status.oct1.loopback_mode);
    printf("oct1 optical transceiver status: %s\n", bdd_status.oct1.optical_status?"ON":"OFF");
    //printf("oct1 optical: %d\n", bdd_status.oct1.optical);
    //printf("oct1 rx pkts rate: %d pps\n", bdd_status.oct1.rx_pkts_rate);
    //printf("oct1 rx bytes rate: %d bps\n", bdd_status.oct1.rx_bytes_rate);
    //printf("oct1 tx pkts rate: %d pps\n", bdd_status.oct1.tx_pkts_rate);
    //printf("oct1 tx bytes rate: %d bps\n", bdd_status.oct1.tx_bytes_rate);
#endif
    return FRE_SUCCESS;

}


int frctweak_sysinfo_get(void)
{
    int rv;
    frc_system_info_t sysinfo;
    rv =  frcapi_sysinfo_get(&sysinfo);
    if (rv != FRE_SUCCESS)
    {
        return rv;
    }

    swap_buff(sizeof(frc_system_info_t)>>3, &sysinfo);
    printf("core_num            : %d\n", (int)sysinfo.core_num);
    printf("core_type           : %d\n", 5650);
    printf("core_arch (bit)     : %d\n", 64);
    printf("core_freq (Mhz)     : %d\n", (int)sysinfo.core_freq/1000000);
    printf("mem_size  (GB)      : %d\n", (int)sysinfo.mem_size);
    printf("mem_freq  (Mhz)     : %d\n", (int)sysinfo.mem_freq/1000000);
    printf("hfa_freq  (Mhz)     : %d\n", 0);
    printf("hfa_size  (MB)      : %d\n", 1024);
    printf("port_speed (Gbps)   : %d\n", (int)sysinfo.port_speed);
    printf("port_num            : %d\n", (int)sysinfo.port_num);
    printf("pcie_lan            : %d\n", (int)sysinfo.pcie_lane);
    printf("pcie_version        : %d\n", (int)sysinfo.pcie_version);
    printf("pcie_oneway (Gbps)  : %d\n", 10);
    printf("board_type          : %d\n", (int)sysinfo.boot_type);
    printf("uboot_flag          : %d\n", 4);
    printf("soft_version        : %d\n", 756);
    printf("grp_all_number      : %d\n", 14);
    printf("grp_id              : %d, num_core: %d, %d~%d\n", 0, 10, 0, 9);
    printf("grp_id              : %d, num_core: %d, %d~%d\n", 1, 1, 0, 0);
    printf("grp_id              : %d, num_core: %d, %d~%d\n", 2, 1, 1, 1);
    printf("grp_id              : %d, num_core: %d, %d~%d\n", 3, 1, 2, 2);
    printf("grp_id              : %d, num_core: %d, %d~%d\n", 4, 1, 3, 3);
    printf("grp_id              : %d, num_core: %d, %d~%d\n", 5, 1, 4, 4);
    printf("grp_id              : %d, num_core: %d, %d~%d\n", 6, 1, 5, 5);
    printf("grp_id              : %d, num_core: %d, %d~%d\n", 7, 1, 6, 6);
    printf("grp_id              : %d, num_core: %d, %d~%d\n", 8, 1, 7, 7);
    printf("grp_id              : %d, num_core: %d, %d~%d\n", 9, 1, 8, 8);
    printf("grp_id              : %d, num_core: %d, %d~%d\n", 10, 1, 9, 9);
    printf("grp_id              : %d, num_core: %d, %d~%d\n", 11, 1, 10, 10);
    printf("grp_id              : %d, num_core: %d, %d~%d\n", 12, 1, 11, 10);
    printf("grp_id              : %d, num_core: %d, %d~%d\n", 13, 10, 0, 9);
    printf("oct0_ipd_port       : %d\n", 0);
    printf("oct1_ipd_port       : %d\n", 16);
    printf("pko_q_per_port_pci  : %d\n", 8);
    printf("pko_q_per_port_if0  : %d\n", 8);
    printf("pko_q_per_port_if1  : %d\n", 8);
    printf("pko_max_port_if0    : %d\n", 1);
    printf("pko_max_port_if1    : %d\n", 1);
    printf("muti_queue          : %d\n", 3);
    printf("core_drv_stat       : %d\n", 1);
    printf("pko_stat            : %d\n", 1);
    printf("dev_id              : %d\n", 0);
    printf("npi_if              : %d\n", 0);
    printf("dma_engines         : %d\n", 8);
    printf("max_local_ptr       : %d\n", 14);
    printf("max_remote_ptr      : %d\n", 14);
    printf("pcipko_active       : %d\n", 1);
    printf("pcipko_port         : %d\n", 32);
    printf("pcipko_queue        : %d\n", 16);
    printf("bar0_addr           : %d\n", 0);
    printf("bar1_addr           : %d\n", 0);
    printf("app_core            : %d\n", 1);
    printf("drv_napi            : %d\n", 1);
    printf("fast_path           : %d\n", 0);
    printf("drop_thread         : %d\n", 0);
    printf("baudrate            : %d\n", 115200);
    return FRE_SUCCESS;
}

void frctweak_cmd_board_get_usage()
{
    printf("USAGE:   %s board (status|info|sysinfo)\n", program);
    printf("  %s board status    --Get the status of the system\n", program);
    printf("  %s board info      --Get the version of the software\n", program);
    printf("  %s board sysinfo   --Get the information of the system\n", program);
}

int frctweak_cmd_board_get(int argc, char **argv)
{
    int rv;
    if (argc != 2)
    {
        frctweak_cmd_board_get_usage();
        exit(1);
    }


    if (!strcasecmp("status", argv[1]))
    {
        rv = board_status_get();
    }

    else if (!strcasecmp("info", argv[1]))
    {
        rv = board_info_get();
    }

    else if (!strcasecmp("sysinfo", argv[1]))
    {
        rv = frctweak_sysinfo_get();
    }
    else
    {
        return FRE_PARAM;
    }

    return rv;
}





void frctweak_cmd_phy_usage(void)
{
    printf("%s phy (oct0|oct1) devad addr [value]\n", program);
    printf("\nOPTIONS:\n");
    printf("  %-16s --%s", "oct0", "Get or set phy register of oct0\n");
    printf("  %-16s --%s", "oct1", "Get or set phy register of oct1\n");
    printf("  %-16s --%s", "devid", "1:PMD_PMA; 2: WIS; 3:PCS; 4:XGXS.\n");
    printf("  %-16s --%s", "addr", "Address of register.\n");
    printf("  %-16s --%s", "value", "Value to set.\n");

}


int frctweak_cmd_phy(int argc, char **argv)
{
    int rv;
    //uint16_t olen;
    frc_bdd_phy_t input;
    frc_phy_op_t output;
    //int op = 0;

    memset(&input, 0, sizeof(frc_bdd_phy_t));
    memset(&output, 0, sizeof(frc_phy_op_t));
    //memset(&output, 0, sizeof(frc_phy_op_t));

    if (argc < 4 || argc > 5)
    {
        frctweak_cmd_phy_usage();
        exit(1);
    }

    if (parse_oct(argv[1], (uint8_t *)&input.phy.port))
    {
        return FRE_PARAM;
    }

    HSTRTU16(argv[2], input.phy.devad);

    HSTRTU16(argv[3], input.phy.addr);

    if (argc == 5)
    {
        HSTRTU16(argv[4], input.phy.value);
        input.op = 1;
    }

    else
    {
        input.op = 0;
    }

    rv =frcapi_bdd_phy(&input, &output);
    if (rv != FRE_SUCCESS)
    {
        FRCTWEAK_ERROR("Get or set PHY of %s fail: %d!\n", argv[1], rv);
        return rv;
    }

    if (!input.op)
    {
        swap_buff(sizeof(frc_phy_op_t) >> 3, &output);
        //printf("PHY of %s: DEVAD %d REG 0x%x VALUE 0x%x\n", argv[1], output.devad, output.addr, output.value);
        printf("0x%x\n", output.value);
    }


    return FRE_SUCCESS;
}

void frctweak_cmd_cpld_usage(void)
{
    printf("%s cpld addr [value]\n", program);
    printf("\nOPTIONS:\n");
    printf("  %-16s --%s", "addr", "Address of register.\n");
    printf("  %-16s --%s", "value", "Value to set.\n");

}

int frctweak_cmd_cpld(int argc, char **argv)
{
    int rv;
    frc_bdd_cpld_t input;
    frc_cpld_op_t output;
    //uint16_t data;
    //int op = 0;

    memset(&output, 0, sizeof(frc_cpld_op_t));
    memset(&input, 0, sizeof(frc_bdd_cpld_t));


    if (argc < 2 || argc > 3)
    {
        frctweak_cmd_cpld_usage();
        exit(1);
    }


    HSTRTU16(argv[1], input.cpld.addr);
    if (3 == argc)
    {
        HSTRTU16(argv[2], input.cpld.data);
        input.op = 1;
    }
    else
    {
        input.op = 0;
    }


    rv = frcapi_bdd_cpld(&input, &output);
    if (rv != FRE_SUCCESS)
    {
        FRCTWEAK_ERROR("Get or set cpld fail: %d!\n", rv);
        return rv;
    }

    if (!input.op)
    {
        swap_buff(sizeof(frc_cpld_op_t) >> 3, &output);
        printf("REG 0x%x VALUE 0x%x\n", output.addr, output.data);
    }

    return FRE_SUCCESS;

}

void frctweak_cmd_poweroff_usage(void)
{
    printf("%s poweroff\n", program);
}

int frctweak_cmd_poweroff(int argc, char **argv)
{
    int rv;

    if (argc != 1)
    {
        frctweak_cmd_poweroff_usage();
        exit(1);
    }
#if 0
    if (strcasecmp("poweroff", argv[1]))
    {
        return FRE_PARAM;
    }
#endif
    rv = frcapi_bdd_poweroff();
    if (rv != FRE_SUCCESS)
    {
        FRCTWEAK_ERROR("Poweroff system fail: %d!\n", rv);
        return rv;
    }

    return FRE_SUCCESS;

}


void frctweak_cmd_port_enable_usage(void)
{
    printf("USAGE: %s port INTERFACE (enable/disable)\n", program);
}

int frctweak_cmd_port_enable(int argc, char **argv)
{
    int rv;
    frc_bdd_set_port_in_t input;
    //uint16_t ilen;

    if (argc != 3)
    {
        frctweak_cmd_port_enable_usage();
        exit(1);
    }

    if (parse_oct(argv[1], &input.port))
    {
        return FRE_PARAM;
    }

    if (parse_bool(argv[2], &input.enable))
    {
        return FRE_PARAM;
    }


    rv = frcapi_bdd_port_enable(&input);
    if (rv != FRE_SUCCESS)
    {
        FRCTWEAK_ERROR("Enable %s fail: %d!\n",argv[1], rv);
        return rv;
    }

    return FRE_SUCCESS;

}

void frctweak_cmd_port_loopback_set_usage(void)
{
    printf("USAGE: %s loopback INTERFACE MODE\n", program);
    printf("\n  OPTION:\n");
    printf("    ----      INTERFACE: oct0 or oct1\n");
    printf("    ----      MODE: line-phy\n"
           "                 line-core\n"
           "                 host-phy\n"
           "                 host-core\n"
           "                 normal      --Means no loopback\n");
}

int frctweak_cmd_port_loopback_set(int argc, char **argv)
{
    int rv;
    frc_bdd_set_loopback_in_t mode;

    if (argc != 3)
    {
        frctweak_cmd_port_loopback_set_usage();
        exit(1);
    }


    if (parse_oct(argv[1], &mode.port))
    {
        return FRE_PARAM;
    }

    if (parse_loopback(argv[2], &mode.loopback))
    {
        return FRE_PARAM;
    }

    //printf("port = %d \n", mode.port);

    rv = frcapi_bdd_port_loopback_set(&mode);
    if (rv != FRE_SUCCESS)
    {
        FRCTWEAK_ERROR("Set loopback mode fail: %d!\n", rv);
        return rv;
    }

    return FRE_SUCCESS;

}



void frctweak_cmd_set_work_mode_usage(void)
{
    printf("USAGE:   %s workmode INTERFACE MODE\n", program);
    printf("\n  OPTION:\n");
    printf("    INTERFACE   -- oct0,oct1\n");
    printf("    MODE        --NICMODE or FRMODE\n");
}

int frctweak_cmd_set_work_mode(int argc, char **argv)
{
    int rv;
    frc_bdd_workmode_set_in_t input;

    memset(&input, 0, sizeof(frc_bdd_workmode_set_in_t));

    if (argc != 3)
    {
        frctweak_cmd_set_work_mode_usage();
        return FRE_SUCCESS;
    }

    if (parse_oct(argv[1], &input.oct))
    {
        return FRE_PARAM;
    }

    if (parse_bool(argv[2], &input.mode))
    {
        return FRE_PARAM;
    }

    rv = frcapi_bdd_workmode_set(&input);
    if (rv != FRE_SUCCESS)
    {
        FRCTWEAK_ERROR("Set work_mode of %s fail: %d!\n", argv[1], rv);
        return rv;
    }

    return FRE_SUCCESS;

}

void frctweak_cmd_sysinfo_get_usage()
{
    printf("USAGE:   %s sysinfo\n", program);
    printf("  %s sysinfo --Get the system information of the board\n", program);
}

int frctweak_cmd_sysinfo_get(int argc, char **argv)
{
    int rv;
    rv = frctweak_sysinfo_get();
    if (rv != FRE_SUCCESS)
    {
        printf("Get sysinfo fail %d\n", rv);
    }
    return rv;
}



int frctweak_bmm_cmd_init(frctweak_cmd_t *cmd)
{
    frctweak_cmd_register(cmd, "board", "Get the information or status of the board", frctweak_cmd_board_get, frctweak_cmd_board_get_usage);
    frctweak_cmd_register(cmd, "phy", "Read or write the phy reg", frctweak_cmd_phy, frctweak_cmd_phy_usage);
    frctweak_cmd_register(cmd, "cpld", "Read or write the cpld reg", frctweak_cmd_cpld, frctweak_cmd_cpld_usage);
    frctweak_cmd_register(cmd, "poweroff", "Poweroff the board", frctweak_cmd_poweroff, frctweak_cmd_poweroff_usage);
    frctweak_cmd_register(cmd, "port", "Enable or disable the port", frctweak_cmd_port_enable, frctweak_cmd_port_enable_usage);
    frctweak_cmd_register(cmd, "loopback", "Set the loopbcak mode", frctweak_cmd_port_loopback_set, frctweak_cmd_port_loopback_set_usage);
    frctweak_cmd_register(cmd, "workmode", "Set work mode", frctweak_cmd_set_work_mode, frctweak_cmd_set_work_mode_usage);
    //frctweak_cmd_register(cmd, "sysinfo", "Get the system information of the board", frctweak_cmd_sysinfo_get, frctweak_cmd_sysinfo_get_usage);
    return 0;

}

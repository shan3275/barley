#include "frcore.h"
#include "frcore_cmd.h"
#include "frcore_misc.h"

#include "cvmx-twsi.h"
#include "frcore_ssn.h"

#if FRC_CONFIG_GET_SYSINFO
extern unsigned int frcore_version_get();

int
frcore_cmd_get_version(uint16_t plen, void *param, uint16_t *olen, void *outbuf)
{
    frc_version_t version;

    frcore_version_get(&version);
    *olen = sizeof(version);
    memcpy(outbuf, &version, sizeof(version));

    return FRE_SUCCESS;
}

int
frcore_cmd_get_system_info(uint16_t plen, void *param, uint16_t *olen, void *outbuf)
{
    frc_system_info_t sysinfo;

    memset(&sysinfo, 0, sizeof(frc_system_info_t));

    sysinfo.core_num       = 12;
    sysinfo.core_mask      = cvmx_sysinfo_get()->core_mask;
    sysinfo.data_core_num  = FRC_DAT_CORE_NUM;
    sysinfo.core_freq      = cvmx_sysinfo_get()->cpu_clock_hz;

    sysinfo.mem_size       = 4096;//cvmx_sysinfo_get()->system_dram_size;
    sysinfo.mem_freq       = cvmx_sysinfo_get()->dram_data_rate_hz;

    sysinfo.pcb_version    = 1;
    sysinfo.boot_type      = 1;

    sysinfo.pcie_version   = 11;
    sysinfo.pcie_lane      = 4;

    sysinfo.cpld_version   = 1;
    sysinfo.port_num       = 2;
    sysinfo.port_speed     = 10000;
    sysinfo.boot_cycle     = cvmx_get_cycle();
    memcpy(outbuf, &sysinfo, sizeof(frc_system_info_t));
    *olen = sizeof(frc_system_info_t);

    return FRE_SUCCESS;
}


int
frcore_cmd_get_temp(uint16_t plen, void *param, uint16_t *olen, void *outbuf)
{
    int rv;
    frc_temp_t temp;
    uint64_t local, remote;

    *olen = sizeof(frc_temp_t);

    rv = cvmx_twsix_read_ia(1, 0x4d, 0, 1, 1, &local);
    FRCORE_CMD("RV = %d.\n", rv);
    rv = cvmx_twsix_read_ia(1, 0x4d, 1, 1, 1, &remote);
    FRCORE_CMD("RV = %d.\n", rv);

    FRCORE_CMD("local %lld, remote %lld.\n", (ULL) local, (ULL) remote);
    temp.local = local;
    temp.remote = remote;

    memcpy(outbuf, &temp, sizeof(frc_temp_t));

    return FRE_SUCCESS;
}

int
frcore_cmd_get_ssn_age(uint16_t plen, void *param, uint16_t *olen, void *outbuf)
{
    int rv;
    uint64_t ssn_age;

    *olen = sizeof(uint64_t);

    rv = frcore_get_ssn_age(&ssn_age);

    memcpy(outbuf, &ssn_age, sizeof(uint64_t));

    return rv;
}

int
frcore_cmd_set_ssn_age(uint16_t plen, void *param, uint16_t *olen, void *outbuf)
{
    int rv;
    uint64_t *ssn_age = (uint64_t *)param;
    rv = frcore_set_ssn_age(*ssn_age);
    return rv;
}

int
frcore_misc_init()
{
    //FRCORE_REGISTER_CMD(CMD_TYPE_USER, USER_CMD_GET_VERSION, frcore_cmd_get_version);
    FRCORE_REGISTER_CMD(CMD_TYPE_USER, USER_CMD_GET_SYSINFO, frcore_cmd_get_system_info);
    //FRCORE_REGISTER_CMD(CMD_TYPE_USER, USER_CMD_GET_TEMP, frcore_cmd_get_temp);
    //FRCORE_REGISTER_CMD(CMD_TYPE_USER, USER_CMD_GET_SSN_AGE, frcore_cmd_get_ssn_age);
    //FRCORE_REGISTER_CMD(CMD_TYPE_USER, USER_CMD_SET_SSN_AGE, frcore_cmd_set_ssn_age);
    return 0;
}
#endif
/* End of file */

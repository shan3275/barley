#include "frcore.h"
#include "frcore_cmd.h"
#include "frcore_misc.h"

#include "cvmx-mdio.h"



int 
frcore_cpld_read(uint32_t addr, uint32_t *value)
{
    int v32;
    v32 = cvmx_mdio_read(1, 11, addr);
    printf("$$$$$$$$$$$$$$$$$$$$\n");
    printf("addr 0x%x v32 0x%x.\n", addr, v32);
    if (v32 < 0)
    {
        FRCORE_ERROR("Cpld read fail: addr 0x%x.\n", addr);
        return FRE_FAIL;
    }
    *value = v32;

    return FRE_SUCCESS;
}

int 
frcore_cpld_write(uint32_t addr, uint32_t value)
{
    int rv;
    rv = cvmx_mdio_write(1, 11, addr, value);
    printf("$$$$$$$$$$$$$$$$$$$$\n");
    printf("addr 0x%x value 0x%x rv %d.\n",addr, value, rv);
    if (0 != rv)
    {
        FRCORE_ERROR("Cpld write fail: addr 0x%x value 0x%8x.\n", addr, value);
        return FRE_FAIL;
    }

    return FRE_SUCCESS;
}

int
frcore_phy_read(uint16_t port, uint16_t devad, uint16_t addr, uint16_t *value)
{
    uint16_t phyid;
    int i32;

    if (port == 0)
    {
        phyid = 0;
    }
    else if (port == 16)
    {
        phyid = 1;
    }
    else
    {
        return FRE_PARAM;
    }
    i32 = cvmx_mdio_45_read(0, phyid, devad, addr);
    FRCORE_PHY("phyid %d devad %d addr 0x%.4x i32 0x%.8x.\n", phyid, devad, addr, i32);
    if (i32 < 0)
    {
        FRCORE_ERROR("PHY read fail: phyid 0x%x devad 0x%x addr 0x%x.\n",
                     phyid, devad, addr);
        return FRE_FAIL;
    }
    *value = i32 & 0xFFFF;

    return FRE_SUCCESS;
}

int
frcore_phy_write(uint16_t port, uint16_t devad, uint16_t addr, uint16_t value)
{
    int rv;
    uint16_t phyid;

    if (port == 0)
    {
        phyid = 0;
    }
    else if (port == 16)
    {
        phyid = 1;
    }
    else
    {
        return FRE_PARAM;
    }

    rv = cvmx_mdio_45_write(0, phyid, devad, addr, value);
    FRCORE_PHY("phyid %d devad %d addr 0x%.4x value 0x%.4x rv %d.\n", phyid, devad, addr, value, rv);
    if (0 != rv)
    {
        FRCORE_ERROR("PHY write fail: phyid 0x%x devad 0x%x addr 0x%x value 0x%x.\n", phyid, devad, addr, value);
        return FRE_FAIL;
    }

    return FRE_SUCCESS;
}


int
frcore_cmd_get_cpld(uint16_t plen, void *param, uint16_t *olen, void *outbuf)
{
    frc_cpld_op_t *arg = (frc_cpld_op_t *)param;
    *olen = sizeof(frc_cpld_op_t);
    
    printf("$$$$$$$$$$$$$$$$\n");
    printf("addr 0x%x value 0x%x\n", arg->addr, arg->data);


    if (frcore_cpld_read(arg->addr, &arg->data)) {
        return FRE_FAIL;
    }
    memcpy(outbuf, arg, sizeof(frc_cpld_op_t));

    return FRE_SUCCESS;

}


int
frcore_cmd_set_cpld(uint16_t plen, void *param, uint16_t *olen, void *outbuf)
{
    frc_cpld_op_t *arg = (frc_cpld_op_t *)param;
    *olen = 0;
    
    printf("$$$$$$$$$$$$$$$$\n");
    printf("addr 0x%x value 0x%x\n",arg->addr, arg->data);


    if (frcore_cpld_write(arg->addr, arg->data)) {
        return FRE_FAIL;
    }

    return FRE_SUCCESS;

}



int 
frcore_cmd_get_phy(uint16_t plen, void *param, uint16_t *olen, void *outbuf)
{

    frc_phy_op_t *arg = (frc_phy_op_t *) param;
    *olen = sizeof(frc_phy_op_t);

    FRCORE_CMD("port %d, devad %d addr 0x%.4x\n", arg->port, arg->devad, arg->addr);

    printf("######################################\n");
    printf("port = %d, devad = %d, addr = 0x%.4x\n", arg->port, arg->devad, arg->addr);

    if (frcore_phy_read(arg->port, arg->devad, arg->addr, &arg->value)) {
        return FRE_FAIL;
    }
    memcpy(outbuf, arg, sizeof(frc_phy_op_t));

    return FRE_SUCCESS;
}

int 
frcore_cmd_set_phy(uint16_t plen, void *param, uint16_t *olen, void *outbuf)
{

    frc_phy_op_t *arg = (frc_phy_op_t *) param;
    *olen = 0;

    FRCORE_CMD("port %d, devad %d addr 0x%.4x value 0x%.4x\n", arg->port, arg->devad, arg->addr, arg->value);
    printf("port %d, devad %d, addr 0x%.4x, vaule 0x%.4x\n", arg->port, arg->devad, arg->addr, arg->value);

    if (frcore_phy_write(arg->port, arg->devad, arg->addr, arg->value)) {
        return FRE_FAIL;
    }

    return FRE_SUCCESS;
}


int frcore_cmd_port_enable(uint16_t plen, void *param, uint16_t *olen, void *outbuf)
{
    FRCORE_CMD("\n");
    uint16_t v0 = 0, v1 = 0;
    if (frcore_phy_read(0, 1, 0, &v0))
    {
        FRCORE_ERROR("Read phy register of port0 fail!\n");
        return FRE_FAIL;
    }

    if (frcore_phy_read(16, 1, 0, &v1))
    {
        FRCORE_ERROR("Read phy register of port1 fail!\n");
        return FRE_FAIL;
    }

    printf("Port 0 , PHY CTRL reigster: 0x%.4x\n", v0);
    printf("Port 16, PHY CTRL reigster: 0x%.4x\n", v1);

    if (frcore_phy_write(0, 1, 0, 0x2040))
    {
        FRCORE_ERROR("Enable port 0 fail!\n");
        return FRE_FAIL;
    }

    if (frcore_phy_write(16, 1, 0, 0x2040))
    {
        FRCORE_ERROR("Enable port 16 fail!\n");
        return FRE_FAIL;
    }

    return FRE_SUCCESS;
}


int
frcore_phy_init()
{
    FRCORE_REGISTER_CMD(CMD_TYPE_CTRL, CTRL_CMD_PORT_ENABLE,  frcore_cmd_port_enable);
    FRCORE_REGISTER_CMD(CMD_TYPE_USER, USER_CMD_GET_PHY, frcore_cmd_get_phy);
    FRCORE_REGISTER_CMD(CMD_TYPE_USER, USER_CMD_SET_PHY, frcore_cmd_set_phy);
    FRCORE_REGISTER_CMD(CMD_TYPE_USER, USER_CMD_GET_CPLD, frcore_cmd_get_cpld);
    FRCORE_REGISTER_CMD(CMD_TYPE_USER, USER_CMD_SET_CPLD, frcore_cmd_set_cpld);

    /* SFP+ cable tunning */
    frcore_phy_write(0, 1, 0xCA02, 0xCC10);
    frcore_phy_write(0, 1, 0xCA00, 0x7008);
    frcore_phy_write(16, 1, 0xCA02, 0xCC10);
    frcore_phy_write(16, 1, 0xCA00, 0x7008);

    return 0;
}

/* End of file */

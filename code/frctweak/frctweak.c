#include <stdlib.h>
#include <string.h>

#include "frctweak.h"
#include "frc_debug.h"
#include "frc_api.h"
#include "frc_debug.h"
#include "frc_types.h"

frctweak_cmd_t *frctweak_cmd = NULL;
char *program;

int parse_loopback(char *str, uint8_t *value)
{
    if (!strcasecmp("line-phy", str))
    {
        *value = 1;
        return 0;
    }

    if (!strcasecmp("line-core", str))
    {
        *value = 2;
        return 0;
    }

    if (!strcasecmp("host-phy", str))
    {
        *value = 3;
        return 0;
    }

    if (!strcasecmp("host-core", str))
    {
        *value = 4;
        return 0;
    }

    if (!strcasecmp("normal", str))
    {
        *value = 0;
        return 0;
    }
    return 1;
}


int parse_log(char *str, uint8_t *value)
{
    if (!strcasecmp(str, "config"))
    {
        *value |= 0x1;
        return 0;
    }

    if (!strcasecmp(str, "event"))
    {
        *value |= 0x2;
        return 0;
    }

    if (!strcasecmp(str, "debug"))
    {
        *value |= 0x4;
        return 0;
    }

    if (!strcasecmp(str, "error"))
    {
        *value |= 0x8;
        return 0;
    }

    if (!strcasecmp(str, "warning"))
    {
        *value |= 0x10;
        return 0;
    }

    if (!strcasecmp(str, "alarm"))
    {
        *value |= 0x20;
        return 0;
    }

    if (!strcasecmp(str, "all"))
    {
        *value |= 0x3f;
        return 0;
    }

    return 1;
}


void fr_u8_string(uint8_t val, char str[18])
{
    memset(str, 0, 18);
    sprintf(str, "%d", val);
}

void fr_u16_string(uint16_t val, char str[18])
{
    memset(str, 0, 18);
    sprintf(str, "%d", val);
}

void fr_u32_string(uint32_t val, char str[18])
{
    memset(str, 0, 18);
    sprintf(str, "%d", val);
}

void fr_proto_string(uint16_t proto, char str[18])
{
    memset(str, 0, 18);
    if (0 == proto)
    {
        sprintf(str, "$");
    }

    else if (6 == proto)
    {
        sprintf(str, "TCP");
    }

    else if (17 == proto)
    {
        sprintf(str, "UDP");
    }

    else
    {
        sprintf(str, "?");
    }

}

void fr_port_string(uint16_t port, char str[18])
{
    memset(str, 0, 18);
    if (0 == port)
    {
        sprintf(str, "$");
    }

    else
    {
        sprintf(str, "%d", port);
    }

}

void fr_ipv4_string(uint32_t ip, char str[18])
{
    memset(str, 0, 18);

    //sprintf(str, "%d.%d.%d.%d",
            //ip & 0xff,
            //(ip >> 8) & 0xff,
            //(ip >> 16) & 0xff,
            //(ip >> 24) & 0xff);

    if (0 == ip)
    {
        sprintf(str, "$");
    }

    else
    {
        sprintf(str, "%d.%d.%d.%d",
                (ip >> 24) & 0xff,
                (ip >> 16) & 0xff,
                (ip >> 8) & 0xff,
                ip & 0xff);
    }
}

int u8_string(uint8_t val, char *str)
{
    if (NULL == str)
    {
        return FRE_PARAM;
    }

    sprintf(str, "0x%.2X", val);
    return 0;
}

int u16_string(uint16_t val, char *str)
{
    if (NULL == str)
    {
        return FRE_PARAM;
    }

    sprintf(str, "0x%.4X", val);
    return 0;
}

int u32_string(uint32_t val, char *str)
{
    if (NULL == str)
    {
        return FRE_PARAM;
    }

    sprintf(str, "0x%.8X", val);
    return 0;
}

int u64_string(uint64_t val, char *str)
{
    if (NULL == str)
    {
        return FRE_PARAM;
    }

    sprintf(str, "0x%llX", val);
    return 0;
}

int mac_string(uint64_t val, char *str)
{
    //printf("%s.%d:val = 0x%llx\n", __func__, __LINE__, val);
    char *u8p = (char *)&val;
    sprintf(str, "%.2X:%.2X:%.2X:%.2X:%.2X:%.2X", 
            u8p[0], u8p[1], u8p[2], u8p[3], u8p[4], u8p[5]);
    //printf("str = %s\n", str);
    return 0;
}

int parse_mac64(char *str, uint64_t *val)
{
    int rv = 0;
    int i;
    uint16_t u16[6] = {};
    uint64_t val1 = 0, val2 = 0;
    rv = sscanf(str, "%hX:%hX:%hX:%hX:%hX:%hX", &u16[0], &u16[1],&u16[2], &u16[3],&u16[4], &u16[5]);
    if (6 != rv)
    {
        return 1;
    }
    for (i = 0; i < 3; i++)
    {
        val1 |= ((u16[i] & 0xff) << ( 8 * i));
    }
    for (i = 3; i < 6; i++)
    {
        val2 |= ((u16[i] & 0xff) << ( 8 * (i-3)));
    }
    *val = (val2 << 24) | val1;
    *val &= 0xffffffffffff;
    //printf("val = 0x%llx, val1 = 0x%llx, val2 = 0x%llx\n", *val, val1, val2);
    return 0;
}

int parse_protocal(char *str, uint16_t *protocal)
{
    if (!strcasecmp(str, "TCP"))
    {
        *protocal = 6;
        return 0;
    }

    else if (!strcasecmp(str, "UDP"))
    {
        *protocal = 17;
        return 0;
    }
    else if (!strcasecmp(str, "ANY"))
    {
        *protocal = 0;
        return 0;
    }

    else if(!strcasecmp(str, "$"))
    {
        *protocal = 0;
        return 0;
    }

    return 1;

}

#if FRC_CONFIG_TWO_TUPLE
int parse_acl_type(char *str, uint16_t *acl_type)
{
    if (!strcasecmp(str, "SIP"))
    {
        *acl_type= FRC_ACL_SIP;
        return FRE_SUCCESS;
    }
    else if (!strcasecmp(str, "DIP"))
    {
        *acl_type = FRC_ACL_DIP;
        return FRE_SUCCESS;
    }else if (!strcasecmp(str, "SP"))
    {
        *acl_type = FRC_ACL_SP;
        return FRE_SUCCESS;
    }
    else if(!strcasecmp(str, "DP"))
    {
        *acl_type = FRC_ACL_DP;
        return FRE_SUCCESS;
    }
    *acl_type = FRC_ACL_UNKNOWN;
    return FRE_FAIL;

}
#endif

#if FRC_CONFIG_VLAN_CHECK
int parse_vlan_check_type(char *str, uint8_t *type)
{
    if (!strcasecmp(str, "SIP"))
    {
        *type= FRC_VLAN_CHECK_SIP;
        return FRE_SUCCESS;
    }
    else if (!strcasecmp(str, "DIP"))
    {
        *type = FRC_VLAN_CHECK_DIP;
        return FRE_SUCCESS;
    }else if (!strcasecmp(str, "SDIP"))
    {
        *type = FRC_VLAN_CHECK_SDIP;
        return FRE_SUCCESS;
    }
    *type = FRC_VLAN_CHECK_UNKNOWN;
    return FRE_FAIL;

}
#endif

int parse_bool(char *str, uint8_t *value)
{
    if (!strcasecmp(str, "enable"))
    {
        *value = 1;
        return 0;
    }
    else if (!strcasecmp(str, "disable"))
    {
        *value = 0;
        return 0;
    }
    else if (!strcasecmp(str, "linkup"))
    {
        *value = 1;
        return 0;
    }
    else if (!strcasecmp(str, "linkdown"))
    {
        *value = 0;
        return 0;
    }
    else if (!strcasecmp(str, "NICMODE"))
    {
        *value = 1;
        return 0;
    }
    else if (!strcasecmp(str, "FRMODE"))
    {
        *value = 0;
        return 0;
    }

    return 1;
}


int parse_u32(char *str, uint32_t *value)
{
    //char *e;
    if (!strncasecmp("0x", str, 2))
    {
        *value = strtoul(str, NULL, 16);
    }
    else
    {
        *value = strtoul(str, NULL, 10);
    }
#if 0
    if (e == (str) || *e != '\0' || *e != '\n') {
        
        return 1;
    }
#endif
    return 0;
}


int parse_u64(char *str, uint64_t *value)
{
    char *e;
    if (!strncasecmp("0x", str, 2))
    {
        *value = strtoull(str, &e, 16);
    }
    else
    {
        *value = strtoull(str, &e, 10);
    }

    if (e == (str) || *e != '\0') {
        return 1;
    }

    return 0;
}

int parse_u32_inc(char *str, uint32_t *value, uint32_t *inc)
{
    char *p, u16s[10];

    memset(u16s, 0, 10);
    if (!strcmp("$", str))
    {
        *value = 0;
        *inc = 1;
        return 0;
    }

    p = strchr(str, '+');
    if (p)
    {
        if (parse_u32((p + 1), inc))
        {
            return 1;
        }
        memcpy(u16s, str, p - str);
    }
    else
    {
        *inc = 1;
        memcpy(u16s, str, strlen(str));
    }

    if (parse_u32(u16s, value))
    {
        return 1;
    }


    return 0;
}


int parse_u16(char *str, uint16_t *value)
{
    int rv;

    uint32_t v32 = 0;

    rv = parse_u32(str, &v32);

    if (rv)
    {
        return 1;
    }

    if (v32 > 0xffff)
    {
        return 1;
    }
    *value = v32 & 0xffff;

    return 0;
}



int parse_u16_inc(char *str, uint16_t *value, uint16_t *inc)
{
    char *p, u16s[10];

    memset(u16s, 0, 10);
    if (!strcmp("$", str))
    {
        *value = 0;
        *inc = 1;
        return 0;
    }

    p = strchr(str, '+');
    if (p)
    {
        if (parse_u16((p + 1), inc))
        {
            return 1;
        }
        memcpy(u16s, str, p - str);
    }
    else
    {
        *inc = 1;
        memcpy(u16s, str, strlen(str));
    }

    if (parse_u16(u16s, value))
    {
        return 1;
    }


    return 0;
}


int parse_u8(char *str, uint8_t *value)
{
    
    int rv;

    uint32_t v32 = 0;

    rv = parse_u32(str, &v32);

    if (rv)
    {
        return 1;
    }

    if (v32 > 0xff)
    {
        return 1;
    }

    *value = v32 & 0xff;

    return 0;
}


int parse_ipv4(char *str, uint32_t *ip)
{
    int rv, i;
    uint32_t ipv[4];
    char buf[32];
    memset(buf, 0, 32);
    memcpy(buf, str, strlen(str));

    rv = sscanf(buf, "%d.%d.%d.%d", &ipv[0], &ipv[1], &ipv[2], &ipv[3]);
    if(rv != 4)
    {
        return 1;
    }

    for(i = 0; i < 4; i++)
    {
        if((ipv[i] < 0) || (ipv[i] > 255))
        {
            return 1;
        }
    }

    *ip = (uint32_t)ipv[3];
    *ip |= (uint32_t)ipv[2] << 8;
    *ip |= (uint32_t)ipv[1] << 16;
    *ip |= (uint32_t)ipv[0] << 24;

    return 0;
}

int parse_ipv4_inc(char *str, uint32_t *ip, uint32_t *inc)
{
    char ipstr[20], *p = NULL;

    memset(ipstr, 0, 20);
    if (!strcmp("$", str))
    {
        str = "0.0.0.0";
    }
    p = strchr(str, '+');
    if (p)
    {
        if (parse_u32((p + 1), inc))
        {
            return 1;
        }
        memcpy(ipstr, str, p - str);
    }
    else
    {
        *inc = 1;
        memcpy(ipstr, str, strlen(str));
    }

    if (parse_ipv4(ipstr, ip))
    {
        return 1;
    }

    return 0;
}

void frctweak_program_string(frctweak_cmd_t *cmd, char *pstr)
{
    frctweak_cmd_t *parent;
    char tstr[FRCTWEAK_PSTR_SZ];

    memset(tstr, 0, FRCTWEAK_PSTR_SZ);
    parent = cmd;
    do
    {
        sprintf(tstr, "%s", pstr);
        sprintf(pstr, "%s %s", parent->name, tstr);
        parent = parent->parent;
    } while (parent);
    pstr[FRCTWEAK_PSTR_SZ - 1] = 0;
}

int frctweak_cmd_usage(frctweak_cmd_t *cmd, int argc, char **argv)
{
    int rv = 0;
    frc_list_t *node;
    frctweak_cmd_t *child;

    frctweak_program_string(cmd, program);

    if ((argc > 1) && !(!strcmp(argv[1], "?") || !strcasecmp(argv[1], "-h") || !strcasecmp(argv[1], "help") || !strcasecmp(argv[1], "--help")))
    {
        printf("Unkown COMMAND: %s!\n", argv[1]);
        rv = 1;
    }



    printf("USAGE:\n");
    printf("%s (?|-h|-H|help|HELP|--help|--HELP)\n", program);
    if (cmd->usage)
    {
        cmd->usage();
    }
    printf("\nCOMMAND:\n");
    frc_list_for_each(node, cmd->head) {
        child = (frctweak_cmd_t *)node;
        printf("%s %s\n", program, child->name);
    }
    printf("\nSUMMARY:\n");
    frc_list_for_each(node, cmd->head) {
        child = (frctweak_cmd_t *)node;
        printf("  %-16s --%s\n", child->name, child->summary);
    }

    return rv;
}

frctweak_cmd_t*
frctweak_cmd_register(frctweak_cmd_t *parent, char *name, char *summary, frctweak_fn_t fn, frctweak_usage_fn_t usage)
{
    frctweak_cmd_t *cmd;

    cmd = malloc(sizeof(frctweak_cmd_t));
    if (cmd == NULL)
    {
        FRCTWEAK_ERROR("Register cmd fail: Can't malloc cmd!\n");
        exit(1);
    }

    memset(cmd, 0, sizeof(frctweak_cmd_t));

    strcpy(cmd->name, name);
    strcpy(cmd->summary, summary);
    cmd->fn = fn;
    cmd->usage = usage;

    if (parent)
    {
        if (!parent->head)
        {
            parent->head = malloc(sizeof(frc_list_t));
            FRC_INIT_LIST_HEAD(parent->head);
        }
        frc_list_add_tail(&cmd->node, parent->head);
        cmd->parent = parent;
    }
    else
    {
        cmd->parent = NULL;
    }

    return cmd;
}

int frctweak_main_cmd(int argc, char **argv)
{
    frc_version_t version;

    frctweak_version_get(&version);
    printf("  FRCTWEAK V%d.%.2d.%d\n", version.major, version.minor, version.build);
    return frctweak_cmd_usage(frctweak_cmd, argc, argv);
}

int frctweak_cmd_exec(frctweak_cmd_t *cmd, int argc, char **argv)
{
    int rv = FRE_PARAM;
    frc_list_t *node;
    frctweak_cmd_t *child;

    if (!strcmp(argv[0], "?") || !strcasecmp(argv[0], "-h") || !strcasecmp(argv[0], "help") || !strcasecmp(argv[0], "--help"))
    {
        return frctweak_cmd_usage(cmd, argc, argv);

    }

    if (argc > 1 && cmd->head)
    {
        frc_list_for_each(node, cmd->head) {
            child = (frctweak_cmd_t *)node;
            if (!strcmp(child->name, argv[1]))
            {
                return frctweak_cmd_exec(child, --argc, ++argv);
            }
        }
    }

    if (cmd->fn)
    {
        rv = cmd->fn(argc, argv);
    }


    if (rv == FRE_PARAM)
    {
        return frctweak_cmd_usage(cmd, argc, argv);
    }

    return 0;
}

int main(int argc, char **argv)
{
    int ret;

    program = malloc(FRCTWEAK_PSTR_SZ);
    memset(program, 0, FRCTWEAK_PSTR_SZ);
    memcpy(program, argv[0], 10);

    frctweak_cmd = frctweak_cmd_register(NULL, argv[0], "Tweak tool of frc", frctweak_main_cmd, NULL);
    //frctweak_misc_cmd_init(frctweak_cmd);
    //frctweak_port_cmd_init(frctweak_cmd);
    //frctweak_phy_cmd_init(frctweak_cmd);
    frctweak_test_cmd_init(frctweak_cmd);
    frctweak_fr_cmd_init(frctweak_cmd);
    frctweak_rule_cmd_init(frctweak_cmd);
    frctweak_bmm_cmd_init(frctweak_cmd);
    //frctweak_dma_cmd_init(frctweak_cmd);
    frctweak_stat_cmd_init(frctweak_cmd);
    frctweak_pr_cmd_init(frctweak_cmd);
    frctweak_chan_cmd_init(frctweak_cmd);
    frctweak_file_cmd_init(frctweak_cmd);
    #if FRC_CONFIG_TWO_TUPLE
    frctweak_acl_cmd_init(frctweak_cmd);
    #endif
    #if FRC_CONFIG_UDP
    frctweak_udp_cmd_init(frctweak_cmd);
    #endif
    #if FRC_CONFIG_VLAN_CHECK
    frctweak_vlan_check_cmd_init(frctweak_cmd);
    #endif
    #if FRC_CONFIG_MAC_STATISTICS
    frctweak_mac_stat_cmd_init(frctweak_cmd);
    #endif
    #if FRC_CONFIG_TIMESTAMP_CHECK
    frctweak_timestamp_check_cmd_init(frctweak_cmd);
    #endif
    #if FRC_CONFIG_IPC
    frctweak_ipc_cmd_init(frctweak_cmd);
    #endif
    ret = frctweak_cmd_exec(frctweak_cmd, argc, argv);

    if (ret != FRE_SUCCESS)
    {
        FRCTWEAK_DEBUG("COMMAND: \"%s\" execute fail: %d!\n", argv[0], ret);
        return 1;
    }
    else
    {
        //FRCTWEAK_DEBUG("COMMAND: \"%s\" execute success.\n", argv[0]);
        return 0;
    }
}
/* End of file */

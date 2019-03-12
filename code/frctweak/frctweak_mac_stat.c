#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include "frctweak.h"
#include "frc_pack.h"
#include "frc_dma.h"
#include "cvmx-swap.h"
#include "frc_api.h"
#include "frc_debug.h"
#if FRC_CONFIG_MAC_STATISTICS

#define MAX_MAC_NUM         512

#define HASH_SIP            0
#define HASH_DIP            1
#define HASH_SIP_DIP        2
#define HASH_FIVE_TUPLE     3

#define HEART_BEAT_OFF                  0
#define HEART_BEAT_ON                   1
#define HEART_BEAT_ON_WITH_DROP         2

#define IP_TOS              1
#define IP_TTL              2


void frctweak_cmd_mac_stat_usage()
{
    printf("Usage: %s mac_stat add mac <mac address>\n", program);
    printf("       %s mac_stat add mac inc <num> <step> <mac address>\n", program);
    printf("       %s mac_stat del mac <mac address>\n", program);
    printf("       %s mac_stat del all\n", program);
    printf("       %s mac_stat clear counter <mac address>\n", program);
    printf("       %s mac_stat clear all\n", program);
    printf("       %s mac_stat show counter <mac address>\n", program);
    printf("       %s mac_stat show all\n", program);
    printf("       %s mac_stat set_hash <sip | dip | sip_dip | five_tuple>\n", program);
    printf("       %s mac_stat heart_beat <on | off | on_with_drop>\n", program);
    printf("       %s mac_stat set_ip <tos | ttl> <num>\n", program);
    printf("Example: \n");
    printf("        %s mac_stat add mac 00:21:45:aa:bb:cc\n", program);
}

static uint64_t frctweak_endian_swap_64(uint64_t x)
{
    x = ((x << 8) & 0xFF00FF00FF00FF00ULL) | ((x >> 8) & 0x00FF00FF00FF00FFULL);
    x = ((x << 16) & 0xFFFF0000FFFF0000ULL) | ((x >> 16) & 0x0000FFFF0000FFFFULL);
    return (x>>32) | (x<<32);
}

int frctweak_mac_str2int(char *mac, uint64_t *mac_num)
{
    char *str = malloc(strlen(mac) + 1);
    if (str == NULL)
    {
        FRC_DEBUG(0, "malloc error\n");
        return FRE_NOSPACE;
    }

    memset(str, 0, strlen(mac) + 1);
    strcpy(str, mac);
    char m[6] = {0};
    char *tmp = str;
    int i;
    for (i = 0; i < 5; i++)
    {
        tmp = strstr(tmp, ":");
        if (tmp == NULL)
        {
            FRC_DEBUG(0, "mac format error\n");
            goto err;
        }
        else
        {
            *tmp = '\0';
            tmp++;
        }
    }

    tmp = str;

    for (i = 0; i < 6; i++)
    {
        m[i] = (char)(strtoul(tmp, NULL, 16));
        tmp += strlen(tmp) + 1;
    }

    uint64_t mac_tmp = 0;
    uint64_t t;
    for (i = 5; i >= 0; i--)
    {
        t = 0xff;
        t = m[5 - i] & t;
        mac_tmp = mac_tmp | (t << (i * 8));
    }
    *mac_num = mac_tmp;
    return FRE_SUCCESS;
err:
    free(str);
    return FRE_PARAM;
}

int frctweak_mac_int2str(uint64_t imac, unsigned char *mac)
{
    unsigned char cmac[6] = {0};
    uint64_t tmp;
    int i;
    for (i = 0; i < 6; i++)
    {
        tmp = imac & 0xff;
        cmac[i] = (char)tmp;
        imac = imac >> 8;
    }

    sprintf(mac, "%02x:%02x:%02x:%02x:%02x:%02x", cmac[5], cmac[4], cmac[3], cmac[2],  cmac[1], cmac[0]);
    return FRE_SUCCESS;
}

int frctweak_cmd_mac_stat_set_hash_mode(char *hash_mode)
{
    uint64_t hash;
    if (!strcmp(hash_mode, "sip"))
    {
        hash = HASH_SIP;
    }
    else if (!strcmp(hash_mode, "dip"))
    {
        hash = HASH_DIP;
    }
    else if (!strcmp(hash_mode, "sip_dip"))
    {
        hash = HASH_SIP_DIP;
    }
    else if (!strcmp(hash_mode, "five_tuple"))
    {
        hash = HASH_FIVE_TUPLE;
    }
    else
    {
        printf("hash mode arguments error!!!\n");
        return FRE_FAIL;
    }

    frc_mac_stat_in_t input;
    input.mac = hash;
    input.counter = 0;

    int rv = frcapi_mac_stat_set(&input, USER_CMD_MAC_STAT_SET_HASH_MODE);
    if (rv != FRE_SUCCESS)
    {
        printf("set hash mode error!\n");
        return FRE_FAIL;
    }
    return FRE_SUCCESS;
}

int frctweak_cmd_mac_stat_heart_beat(char *heart)
{
    frc_mac_stat_in_t input;
    if (!strcmp(heart, "on"))
    {
        input.mac = HEART_BEAT_ON;
    }
    else if (!strcmp(heart, "off"))
    {
        input.mac = HEART_BEAT_OFF;
    }
    else if (!strcmp(heart, "on_with_drop"))
    {
        input.mac = HEART_BEAT_ON_WITH_DROP;
    }
    else
    {
        printf("heart beat arguments error!\n");
        return FRE_FAIL;
    }

    input.counter = 0;
    int rv = frcapi_mac_stat_set(&input, USER_CMD_MAC_STAT_HEART_BEAT);
    if (rv != FRE_SUCCESS)
    {
        printf("set heart beat error!\n");
        return FRE_FAIL;
    }
    return FRE_SUCCESS;
}

int frctweak_cmd_mac_stat_set_ip(char *type, int value)
{
    frc_mac_stat_in_t input;
    if (!strcmp(type, "tos"))
    {
        input.mac = IP_TOS;
    }
    else if (!strcmp(type, "ttl"))
    {
        input.mac = IP_TTL;
    }
    else
    {
        printf("ip header arguments error!\n");
        return FRE_FAIL;
    }
    input.counter = value;

    int rv = frcapi_mac_stat_set(&input, USER_CMD_MAC_STAT_SET_IP);
    if (rv != FRE_SUCCESS)
    {
        printf("set ip header error!\n");
        return FRE_FAIL;
    }
    return rv;
}

int frctweak_cmd_mac_stat_add_mac(char *mac)
{
    uint64_t imac;
    int rv = frctweak_mac_str2int(mac, &imac);
    if (rv != FRE_SUCCESS)
    {
        FRC_DEBUG(0, "add mac error!\n");
        return FRE_FAIL;
    }

    frc_mac_stat_in_t input;
    input.mac = imac;
    input.counter = 0;
    rv = frcapi_mac_stat_set(&input, USER_CMD_MAC_STAT_ADD_MAC);
    if (rv == FRE_SUCCESS)
    {
        printf("add %s OK!\n", mac);
    }
    else
    {
        printf("add %s ERR!\n", mac);
    }

    return rv;
}

int frctweak_cmd_mac_stat_add_mac_inc(char *mac, unsigned int num, unsigned int step)
{
    uint64_t imac;
    frctweak_mac_str2int(mac, &imac);
    unsigned char mac_str[64];
    for (; num > 0; num--)
    {
        memset(mac_str, 0, 64);
        frctweak_mac_int2str(imac, &(mac_str[0]));
        frctweak_cmd_mac_stat_add_mac(&mac_str);
        imac = imac + step;
    }

    return FRE_SUCCESS;
}

int frctweak_cmd_mac_stat_del_mac(char *mac)
{
    uint64_t imac;
    int rv = frctweak_mac_str2int(mac, &imac);
    if (rv != FRE_SUCCESS)
    {
        printf("del mac error!\n");
        return FRE_FAIL;
    }

    frc_mac_stat_in_t input;
    input.mac = imac;
    input.counter = 0;

    rv = frcapi_mac_stat_set(&input, USER_CMD_MAC_STAT_DEL_MAC);
    if (rv == FRE_SUCCESS)
    {
        printf("del %s OK!\n", mac);
    }
    else
    {
        printf("del %s ERR!\n", mac);
    }

    return rv;
}

int frctweak_cmd_mac_stat_del_all()
{
    int rv = frcapi_mac_stat_set(NULL, USER_CMD_MAC_STAT_DEL_ALL);
    if (rv == FRE_SUCCESS)
    {
        printf("del all OK!\n");
    }
    else
    {
        printf("del all ERR!\n");
    }

    return rv;
}

int frctweak_cmd_mac_stat_clear_counter(char *mac)
{
    uint64_t imac;
    int rv = frctweak_mac_str2int(mac, &imac);
    if (rv != FRE_SUCCESS)
    {
        printf("clear counter error!\n");
        return FRE_FAIL;
    }

    frc_mac_stat_in_t input;
    input.mac = imac;
    input.counter = 0;

    rv = frcapi_mac_stat_set(&input, USER_CMD_MAC_STAT_CLEAR_COUNTER);
    if (rv == FRE_SUCCESS)
    {
        printf("clear %s counter OK!\n", mac);
    }
    else
    {
        printf("clear %s counter ERR!\n", mac);
    }

    return rv;
   
}

int frctweak_cmd_mac_stat_clear_all()
{
    int rv = frcapi_mac_stat_set(NULL, USER_CMD_MAC_STAT_CLEAR_ALL_COUNTER);
    if (rv == FRE_SUCCESS)
    {
        printf("clear all mac counter OK!\n");
    }
    else
    {
        printf("clear all mac counter ERR!\n");
    }

    return rv;
   
}

int frctweak_cmd_mac_stat_show_counter(char *mac)
{
    uint64_t imac;
    unsigned char mac_str[64];
    memset(mac_str, 0, 64);
    int rv = frctweak_mac_str2int(mac, &imac);
    if (rv != FRE_SUCCESS)
    {
        printf("show counter error!\n");
        return FRE_FAIL;
    }

    frc_mac_stat_in_t input;
    input.mac = imac;
    input.counter = 0;

    frc_mac_stat_out_t output;
    output.mac = 0;
    output.total = 0;
    output.errors = 0;
    rv = frcapi_mac_stat_get(&input, &output, USER_CMD_MAC_STAT_SHOW_COUNTER);
    if (rv == FRE_SUCCESS)
    {
        output.mac = frctweak_endian_swap_64(output.mac);
        output.total = frctweak_endian_swap_64(output.total);
        output.errors = frctweak_endian_swap_64(output.errors);
        frctweak_mac_int2str(output.mac, mac_str);
        printf("mac:  %s     \ttotal:  %lu\t\terrors:%lu\n", mac_str, output.total, output.errors);
    }
    else
    {
        printf("show %s counter ERR!\n", mac);
    }

    return rv;
   
}

int frctweak_cmd_mac_stat_show_all()
{
    int rv;
    frc_mac_stat_in_t input;
    uint8_t mac[64];
    frc_mac_stat_out_t output;
    uint64_t i;

    for (i = 0; i <= MAX_MAC_NUM; i++)
    {
        input.mac = i;
        input.counter = 0;
        memset(&output, 0, sizeof(frc_mac_stat_out_t));
        rv = frcapi_mac_stat_get_all(&input, &output, 3, USER_CMD_MAC_STAT_SHOW_ALL_COUNTER);
        if (rv == FRE_SUCCESS)
        {
                memset(mac, 0, 64);
                output.mac = frctweak_endian_swap_64(output.mac);
                output.total = frctweak_endian_swap_64(output.total);
                output.errors = frctweak_endian_swap_64(output.errors);
                frctweak_mac_int2str(output.mac, mac);
                printf("mac:  %s     \ttotal:  %lu\t\terrors:%lu\n", mac, output.total, output.errors);
        }
        else
        {
            break;
        }
    }

    return FRE_SUCCESS;
}

int frctweak_cmd_mac_stat(int argc, char **argv)
{
    if (argc < 3)
    {
        frctweak_cmd_mac_stat_usage();
        return FRE_SUCCESS;
    }
    if (!strcmp(argv[1], "add"))
    {
        if (!strcmp(argv[2], "mac"))
        {
            if (argc == 4)
            {
                return frctweak_cmd_mac_stat_add_mac(argv[3]);
            }
            else if (argc == 7)
            {
                if (!strcmp(argv[3], "inc"))
                {
                    return frctweak_cmd_mac_stat_add_mac_inc(argv[6], atoi(argv[4]), atoi(argv[5]));
                }
                else
                {
                    frctweak_cmd_mac_stat_usage();
                    return FRE_SUCCESS;
                }
            }
            else
            {
                frctweak_cmd_mac_stat_usage();
                return FRE_SUCCESS;
            }
        }
        else
        {
            frctweak_cmd_mac_stat_usage();
            return FRE_FAIL;
        }
    }
    else if (!strcmp(argv[1], "del"))
    {
        if (argc == 3)
        {
            if (!strcmp(argv[2], "all"))
            {
                return frctweak_cmd_mac_stat_del_all();

            }
            else
            {
                frctweak_cmd_mac_stat_usage();
                return FRE_SUCCESS;
            }
        }
        else if (argc ==4 && (!strcmp(argv[2], "mac")))
        {
            return frctweak_cmd_mac_stat_del_mac(argv[3]);
        }
        else
        {
            frctweak_cmd_mac_stat_usage();
            return FRE_SUCCESS;
        }
    }
    else if (!strcmp(argv[1], "clear"))
    {
        if (argc == 3)
        {
            if (!strcmp(argv[2], "all"))
            {
                return frctweak_cmd_mac_stat_clear_all();
            }
            else
            {
                frctweak_cmd_mac_stat_usage();
                return FRE_SUCCESS;
            }
        }
        else if (argc == 4)
        {
            if (!strcmp(argv[2], "counter"))
            {
                return frctweak_cmd_mac_stat_clear_counter(argv[3]);
            }
            else
            {
                frctweak_cmd_mac_stat_usage();
                return FRE_SUCCESS;
            }
        }
        else
        {
            frctweak_cmd_mac_stat_usage();
            return FRE_SUCCESS;
        }
    }
    else if (!strcmp(argv[1], "show"))
    {
        if (argc == 3)
        {
            if (!strcmp(argv[2], "all"))
            {
                return frctweak_cmd_mac_stat_show_all();
            }
            else
            {
                frctweak_cmd_mac_stat_usage();
                return FRE_SUCCESS;
            }
        }
        else if (argc == 4)
        {
            if (!strcmp(argv[2], "counter"))
            {
                return frctweak_cmd_mac_stat_show_counter(argv[3]);
            }
            else
            {
                return FRE_SUCCESS;
            }
        }
        else
        {
            frctweak_cmd_mac_stat_usage();
            return FRE_SUCCESS;
        }
    }
    else if (!strcmp(argv[1], "set_hash"))
    {
        if (argc != 3)
        {
            frctweak_cmd_mac_stat_usage();
            return FRE_SUCCESS;
        }
        return frctweak_cmd_mac_stat_set_hash_mode(argv[2]);
    }
    else if (!strcmp(argv[1], "heart_beat"))
    {
        if (argc != 3)
        {
            frctweak_cmd_mac_stat_usage();
            return FRE_FAIL;
        }
        return frctweak_cmd_mac_stat_heart_beat(argv[2]);
    }
    else if (!strcmp(argv[1], "set_ip"))
    {
        if ((!strcmp(argv[2], "tos")) || (!strcmp(argv[2], "ttl")))
        {
            return frctweak_cmd_mac_stat_set_ip(argv[2], atoi(argv[3]));
        }
        frctweak_cmd_mac_stat_usage();
        return FRE_FAIL;
    }
    {
        frctweak_cmd_mac_stat_usage();
        return FRE_SUCCESS;
    }
}

int frctweak_mac_stat_cmd_init(frctweak_cmd_t *cmd)
{
    frctweak_cmd_register(cmd, "mac_stat", "Add and Del mac pkts counter", frctweak_cmd_mac_stat, frctweak_cmd_mac_stat_usage);
    return 0;
}

#endif

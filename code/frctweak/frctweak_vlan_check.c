#include <stdlib.h>
#include <string.h>

#include "frctweak.h"
#include "frc_pack.h"
#include "frc_dma.h"
#include "cvmx-swap.h"
#include "frc_api.h"
#include "frc_debug.h"

#if FRC_CONFIG_VLAN_CHECK

typedef enum{
    FRCTWEAK_VLAN_CHECK_GET_PARAMETER_CMD,
    FRCTWEAK_VLAN_CHECK_SET_PARAMETER_CMD,
    FRCTWEAK_VLAN_CHECK_GET_STAT_CMD,
    FRCTWEAK_VLAN_CHECK_CLEAR_STAT_CMD,
}frctweak_vlan_check_t;

char *frctweak_vlan_check_type_str[FRC_VLAN_CHECK_MAX]= {
    "UNKNOWN",
    "SIP    ",
    "DIP    ",
    "SDIP   ",
};

void frctweak_cmd_vlan_check_usage()
{
    printf("Usage: %s vlan_check SUBCOMMAND OPTION\n",program);
    printf("       %s vlan_check status\n",program);
    printf("       %s vlan_check set          Type Start Num\n", program);
    printf("       %s vlan_check stat PORT\n", program);
    printf("       %s vlan_check clear\n", program);
    printf("\n  SUBCOMMAND:\n");
    printf("    status          -- Get vlan_check module parameter.\n");
    printf("    set             -- Set vlan_check parameter.\n");
    printf("    stat            -- Get vlan_check statistics.\n");
    printf("    clear           -- Clear vlan_check statistics.\n");
    printf("\n  OPTION:\n");
    printf("    Type            -- Type, (SIP | DIP | SDIP).\n");
    printf("    Start           -- Start ID\n");
    printf("    Num             -- Vlan id Number, <=256\n");
    printf("    PORT            -- Physic port, (xe0 | xe1).\n");
}

void frctweak_cmd_vlan_check_set_usage()
{
    printf("USAGE:       %s vlan_check set Type Start Num\n",program);
    printf("\n  OPTION:\n");
    printf("    Type            -- Type, (SIP | DIP | SDIP).\n");
    printf("    Start           -- Start ID\n");
    printf("    Num             -- Vlan id Number, <=256\n");
}

void frctweak_cmd_vlan_stat_usage()
{
    printf("USAGE:       %s vlan_check stat PORT\n",program);
    printf("\n  OPTION:\n");
    printf("    PORT            -- Physic port, (xe0 | xe1).\n");   
}

int frctweak_vlan_check_stat_get(char *port)
{
    #define STAT_GET_ONCE_SIZE 12
    int rv, i;
    int stat_max = stat_vlan_id_max/2;
    uint64_t stat[stat_max];  
    frc_vlan_check_para_t vlan_check_para = {};
    char stat_name[FRC_STAT_NAME_SIZE];
    frc_vlan_op_in_t input;
    memset(stat, 0, sizeof(stat));
    int num;
    stat_max = stat_vlan_id_max/2;
    num = stat_max / STAT_GET_ONCE_SIZE;

    if (NULL == port)
    {
        return FRE_FAIL;
    }

    if (!strcmp(port, "xe0"))
    {
        input.port = 0;
    }
    else if (!strcmp(port, "xe1"))
    {
        input.port = 16;
    }
    else
    {
        frctweak_cmd_vlan_stat_usage();
    }

    int raw_num = 0, vlan_offset = 0;
    if (stat_max % STAT_GET_ONCE_SIZE)
    {
        num += 1;
    }
    rv = frcapi_vlan_check_status_get(&vlan_check_para);
    if (rv != FRE_SUCCESS)
    {
        printf("Get status of vlan config fail: %d!\n", rv);
        return FRE_FAIL;
    }
    swap_buff(sizeof(frc_vlan_check_para_t)>>3, &vlan_check_para);

    printf("Start_id  : %d\n", vlan_check_para.start_id);

    vlan_offset = vlan_check_para.start_id;

    for (i = 0; i < num; i++)
    {
        if (i == num -1)
        {
            if (stat_max % STAT_GET_ONCE_SIZE)
            {
                input.num = stat_max % STAT_GET_ONCE_SIZE;
            } else {
                input.num = STAT_GET_ONCE_SIZE;
            }
        } else {
            input.num = STAT_GET_ONCE_SIZE;
        }

        input.index = i * STAT_GET_ONCE_SIZE;

        rv = frcapi_vlan_check_stat_get(&input, &stat[i * STAT_GET_ONCE_SIZE]);

        if (rv != FRE_SUCCESS)
        {
            printf("Get stat of frcore fail: %d!\n", rv);
            return FRE_FAIL;
        }

    }

    swap_buff(stat_max, stat);
    //stat[xe0_stat_vlan_id_total] -= stat[xe0_stat_vlan_id_error];

    for (i = 0; i < stat_max; i++)
    {
        if (stat[i] > 0)  {
            frc_vlan_stat_name_get(i, stat_name);
            rv = sscanf(stat_name, "vlan_id_%d", &raw_num);
            if (1 == rv)
            {
                sprintf(stat_name, "vlan_id_%d", raw_num+vlan_offset);
            }
            if (port)
            {               
                printf("%-20s: %lld\n", stat_name, (ULL)stat[i]);
            }
        }
    }

    return FRE_SUCCESS;
}

int frctweak_cmd_vlan_check(int argc, char **argv)
{
    int rv;
    int op;
    frc_vlan_check_para_t vlan_check_para;
    uint8_t  vlan_check_type;
    uint16_t start_id;
    uint16_t  number;

    memset(&vlan_check_para, 0, sizeof(frc_vlan_check_para_t));

    if (argc < 2)
    {
        frctweak_cmd_vlan_check_usage();
        return FRE_SUCCESS;
    }
    if (!(strcmp("status", argv[1])))
    {
        op = FRCTWEAK_VLAN_CHECK_GET_PARAMETER_CMD;
    }
    else if (!(strcmp("set", argv[1])))
    {
        op = FRCTWEAK_VLAN_CHECK_SET_PARAMETER_CMD;
    }
    else if (!(strcmp("stat", argv[1])))
    {
        op = FRCTWEAK_VLAN_CHECK_GET_STAT_CMD;
    }
    else if (!(strcmp("clear", argv[1])))
    {
        op = FRCTWEAK_VLAN_CHECK_CLEAR_STAT_CMD;
    }
    else
    {
        frctweak_cmd_vlan_check_usage();
        return 0;
    }

    switch (op)
    {
    case FRCTWEAK_VLAN_CHECK_GET_PARAMETER_CMD:
        rv = frcapi_vlan_check_status_get(&vlan_check_para);
        if (rv)
        {
            printf("Error rv=%d", rv);
            return FRE_FAIL;
        }
        swap_buff(sizeof(frc_vlan_check_para_t)>>3, &vlan_check_para);
        printf("VLAN_CHECK status:\n");
        printf("Type      : %s\n", frctweak_vlan_check_type_str[vlan_check_para.type]);
        printf("Start_id  : %d\n", vlan_check_para.start_id);
        printf("number    : %d\n", vlan_check_para.num);
        break;
    case FRCTWEAK_VLAN_CHECK_SET_PARAMETER_CMD:
        if (argc < 5)
        {
            frctweak_cmd_vlan_check_set_usage();
            return FRE_SUCCESS;
        }

        if (parse_vlan_check_type(argv[2], &vlan_check_type))
        {
            printf("Invalid vlan_check type!");
            return FRE_PARAM;
        }

        sscanf(argv[3], "%hu", &start_id);
        if (start_id < VLAN_CHECK_START_ID_MIN || start_id > VLAN_CHECK_START_ID_MAX)
        {
            printf("Invalid vlan_check start_id!");
            return FRE_PARAM;
        }

        if (parse_u16(argv[4], &number))
        {
            return FRE_PARAM;
        }
        if (number > 128)
        {
            printf("Invalid vlan_check start_id!");
            return FRE_PARAM;
        }

        if (start_id + number > VLAN_CHECK_ID_MAX)
        {
            printf(" stat_id + number > %d\n", VLAN_CHECK_ID_MAX);
            return FRE_PARAM;
        }

        vlan_check_para.type     = vlan_check_type;
        vlan_check_para.start_id = start_id;
        vlan_check_para.num      = number;
        rv = frcapi_vlan_check_set_parameter(&vlan_check_para);
        if (rv)
        {
               printf("Error rv=%d\n", rv);
               return FRE_FAIL;
        }
        break;
    case FRCTWEAK_VLAN_CHECK_GET_STAT_CMD:
        rv = frctweak_vlan_check_stat_get(argv[2]);
        if (rv)
        {
            printf("Error rv=%d\n", rv);
            return FRE_FAIL;
        }
        break;
    case FRCTWEAK_VLAN_CHECK_CLEAR_STAT_CMD:
        rv = frcapi_vlan_check_stat_clear();
        if (rv)
        {
            printf("Error rv=%d\n", rv);
            return FRE_FAIL;
        }
        break;
    default:
        return FRE_PARAM;
    }

    return rv;

}

int frctweak_vlan_check_cmd_init(frctweak_cmd_t *cmd)
{
    frctweak_cmd_register(cmd, "vlan_check", "Get or set for vlan_check module", frctweak_cmd_vlan_check, frctweak_cmd_vlan_check_usage);

    return 0;

}
#endif /* end of FRC_CONFIG_TWO_TUPLE*/

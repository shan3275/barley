#include <stdlib.h>
#include <string.h>

#include "frctweak.h"
#include "frc_pack.h"
#include "frc_dma.h"
#include "cvmx-swap.h"
#include "frc_api.h"
#include "frc_debug.h"

#if FRC_CONFIG_TIMESTAMP_CHECK

typedef enum{
    FRCTWEAK_TIMESTAMP_CHECK_GET_PARAMETER_CMD,
    FRCTWEAK_TIMESTAMP_CHECK_SET_PARAMETER_CMD,
    FRCTWEAK_TIMESTAMP_CHECK_GET_STAT_CMD,
    FRCTWEAK_TIMESTAMP_CHECK_CLEAR_STAT_CMD,
}frctweak_timestamp_check_t;


void frctweak_cmd_timestamp_check_usage()
{
    printf("Usage: %s timestamp_check SUBCOMMAND OPTION\n",program);
    printf("       %s timestamp_check stat\n", program);
    printf("       %s timestamp_check clear\n", program);
    printf("\n  SUBCOMMAND:\n");
    printf("    stat            -- Get timestamp_check statistics.\n");
    printf("    clear           -- Clear timestamp_check statistics.\n");
}


void frctweak_cmd_timestamp_stat_usage()
{
    printf("USAGE:       %s timestamp_check stat PORT\n",program);
}

int frctweak_timestamp_check_stat_get()
{
    #define STAT_GET_ONCE_SIZE 12
    int rv, i;
    uint64_t stat[stat_timestamp_max];
    char stat_name[42];
    frc_timestamp_op_in_t input;
    memset(stat, 0, sizeof(stat));
    int num;
    num = stat_timestamp_max / STAT_GET_ONCE_SIZE;
    if (stat_timestamp_max % STAT_GET_ONCE_SIZE)
    {
        num += 1;
    }
    for (i = 0; i < num; i++)
    {
        if (i == num -1)
        {
            if (stat_timestamp_max % STAT_GET_ONCE_SIZE)
            {
                input.num = stat_timestamp_max % STAT_GET_ONCE_SIZE;
            } else {
                input.num = STAT_GET_ONCE_SIZE;
            }
        } else {
            input.num = STAT_GET_ONCE_SIZE;
        }

        input.index = i * STAT_GET_ONCE_SIZE;

        rv = frcapi_timestamp_check_stat_get(&input, &stat[i * STAT_GET_ONCE_SIZE]);

        if (rv != FRE_SUCCESS)
        {
            printf("Get stat of frcore fail: %d!\n", rv);
            return FRE_FAIL;
        }

    }

    swap_buff(stat_timestamp_max, stat);
    for (i = 0; i < stat_timestamp_max; i++)
    {
        if (stat[i] > 0)  {
            frc_timestamp_stat_name_get(i, stat_name);
            printf("%-40s: %lld\n", stat_name, (ULL)stat[i]);
        }
    }

    return FRE_SUCCESS;
}

int frctweak_cmd_timestamp_check(int argc, char **argv)
{
    int rv;
    int op;
    uint8_t  timestamp_check_type;
    uint16_t start_id;
    uint16_t  number;
    if (argc < 2)
    {
        frctweak_cmd_timestamp_check_usage();
        return FRE_SUCCESS;
    }
    if (!(strcmp("stat", argv[1])))
    {
        op = FRCTWEAK_TIMESTAMP_CHECK_GET_STAT_CMD;
    }
    else if (!(strcmp("clear", argv[1])))
    {
        op = FRCTWEAK_TIMESTAMP_CHECK_CLEAR_STAT_CMD;
    }
    else
    {
        frctweak_cmd_timestamp_check_usage();
        return 0;
    }

    switch (op)
    {
    case FRCTWEAK_TIMESTAMP_CHECK_GET_STAT_CMD:
        rv = frctweak_timestamp_check_stat_get();
        if (rv)
        {
            printf("Error rv=%d\n", rv);
            return FRE_FAIL;
        }
        break;
    case FRCTWEAK_TIMESTAMP_CHECK_CLEAR_STAT_CMD:
        rv = frcapi_timestamp_check_stat_clear();
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

int frctweak_timestamp_check_cmd_init(frctweak_cmd_t *cmd)
{
    frctweak_cmd_register(cmd, "timestamp_check", "Get or set for timestamp_check module", frctweak_cmd_timestamp_check, frctweak_cmd_timestamp_check_usage);

    return 0;

}
#endif /* end of FRC_CONFIG_TWO_TUPLE*/

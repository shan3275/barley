#include <stdlib.h>
#include <string.h>

#include "frctweak.h"
#include "frc_pack.h"
#include "frc_dma.h"
#include "cvmx-swap.h"
#include "frc_api.h"
#include "frc_debug.h"

char *frc_cnt_name[stat_max] = {
    "current_ssn             ",
    "total_ssn               ",
    "total_submit_ssn        ",
    "fail_created_ssn        ",
    "drop_vlan               ",
    "drop_mpls               ",
    "drop_not_ip             ",
    "ip_pkts                 ",
    "ipv4_pkts               ",
    "drop_ipv6_pkts          ",
    "drop_ip_option          ",
    "drop_ip_frag            ",
    "drop_not_tcp_udp        ",
    "tcp_pkts                ",
    "udp_pkts                ",
    "drop_tcp_option         ",
    "tcp_payload_null        ",
    "pkts_below_64           ",
    "pkts_64_128             ",
    "pkts_128_256            ",
    "pkts_256_512            ",
    "pkts_512_1024           ",
    "pkts_1024_1500          ",
    "pkts_1500_1600          ",
    "pkts_above_1600         ",
    "vary_short_pkts         ",
    "vary_long_pkts          ",
    "normal_pkts             ",
    "retrans_pkts            ",
    "disorder_pkts           ",
    "tcp_submit_pkts         ",
    "tcp_sumbit_bytes        ",
    "udp_sumbit_pkts         ",
    "udp_sumbit_bytes        ",
    "drop_pkts_fifo_short    ",
    "drop_pkts_dma_full      ",
    "drop_ip_other           ",
    "rx_errs                 ",
    "rx_pkts                 ",
    "rx_bytes                ",
    "tx_errs                 ",
    "tx_pkts                 ",
    "tx_bytes                ",
    "act_debug               ",
    "act_drop                ",
    "act_forward             ",
    "act_unkown              ",
    "ssn_pkts                ",
    "ssn_error               ",
    "ssn_created             ",
    "ssn_create_fail         ",
    "ssn_free                ",
    "ssn_collision           ",
    "cnt_ssn_no_collision    ",
    "ssn_no_available        ",
    "ssn_collision_max       ",
    "ssn_forward_dir         ",
    "ssn_postitive_dir       ",
    "ssn_exist               ",
    "drop_rx_error           ",
    "work                    ",
    "pkt_work                ",
    "age_work                ",
    "cnt_work                ",
    "unknown_work            ",
};



void frctweak_cmd_stat_clear_usage(void)
{
    printf("%s clear stat\n", program);
}

int frctweak_cmd_stat_clear(int argc, char **argv)
{
    int rv;
    if (argc < 2)
    {
        frctweak_cmd_stat_clear_usage();
        return FRE_SUCCESS;
    }

    if (strcmp("stat", argv[1]))
    {
        return FRE_PARAM;
    }

    rv = frcapi_stat_clear();

    if (rv != FRE_SUCCESS)
    {
        FRCTWEAK_ERROR("Clear stat of frcore fail: %d!\n", rv);
        return rv;
    }

    return FRE_SUCCESS;

}

void frctweak_cmd_stat_get_usage(void)
{
    printf("%s stat\n", program);
}

int frctweak_cmd_stat_get(int argc,char **argv)
{
    #define STAT_GET_ONCE_SIZE 12
    int rv, i;
    uint64_t stat[stat_max];
    char stat_name[FRC_STAT_NAME_SIZE];
    frc_stat_op_in_t input;
    memset(stat, 0, sizeof(stat));
    int num;
    num = stat_max / STAT_GET_ONCE_SIZE;
    if (stat_max % STAT_GET_ONCE_SIZE)
    {
        num += 1;
    }
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

        rv = frcapi_pkt_stat_get(&input, &stat[i * STAT_GET_ONCE_SIZE]);

        if (rv != FRE_SUCCESS)
        {
            printf("Get stat of frcore fail: %d!\n", rv);
            return FRE_FAIL;
        }

    }

    swap_buff(stat_max, stat);
    for (i = 0; i < stat_max; i++)
    {
        if (stat[i] > 0)  {
            frc_stat_name_get(i, stat_name);
            printf("%-20s: %lld\n", stat_name, (ULL)stat[i]);
        }
    }

    return FRE_SUCCESS;
}


int frctweak_stat_cmd_init(frctweak_cmd_t *cmd)
{
    frctweak_cmd_register(cmd, "clear", "clear packets statistic", frctweak_cmd_stat_clear, frctweak_cmd_stat_clear_usage);
    frctweak_cmd_register(cmd, "stat", "Get statitics of all packets", frctweak_cmd_stat_get, frctweak_cmd_stat_get_usage);

    return 0;

}

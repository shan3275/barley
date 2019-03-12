#include <stdlib.h>
#include <string.h>

#include "frctweak.h"
#include "frc_pack.h"
#include "frc_dma.h"
#include "cvmx-swap.h"
#include "frc_api.h"
#include "frc_debug.h"


typedef enum{
    FRCTWEAK_RULE_STATUS_CMD,
    FRCTWEAK_RULE_ADD_CMD,
    FRCTWEAK_RULE_DEL_CMD,
    FRCTWEAK_RULE_STAT_CLEAR_CMD,
    FRCTWEAK_RULE_CLEAR_CMD,
    FRCTWEAK_RULE_GET_CMD,
    FRCTWEAK_RULE_ENABLE_CMD,
    FRCTWEAK_RULE_DISABLE_CMD,
    FRCTWEAK_RULE_CNT_CMD,
    FRCTWEAK_RULE_UPDATE_CMD,
    FRCTWEAK_RULE_NUM_GET_CMD,
    FRCTWEAK_RULE_MATCH_CMD,
}frctweak_rule_t;

void frctweak_cmd_rule_usage()
{
    printf("Usage: %s rule SUBCOMMAND OPTION\n",program);
    printf("       %s rule status\n",program);
    printf("       %s rule add          INDEX SIP DIP SP DP PROTO\n",program);
    printf("       %s rule del          INDEX NUM\n", program);
    printf("       %s rule update\n", program);
    printf("       %s rule get          INDEX NUM\n", program);
    printf("       %s rule stat_clear   INDEX NUM\n", program);
    printf("       %s rule num\n", program);
    printf("       %s rule match        SIP DIP SP DP PROTO\n", program);
    printf("       %s rule cnt          INDEX NUM\n", program);
    printf("       %s rule  clear                  ", program);
    printf("\n  SUBCOMMAND:\n");
    printf("    status          -- Get rule module status:(enable|disable).\n");
    printf("    add             -- Add one rule.\n");
    printf("    del             -- Delete rule.\n");
    printf("\n  OPTION:\n");
    printf("    INDEX           -- Index, from 1 to 2000.\n");
    printf("    SIP+            -- Source IP or source ip increment, like: 192.168.1.0/255.255.255.0 or 10.0.0.1+100\n");
    printf("    DIP+            -- Destination IP or source ip increment, like: 192.168.1.0/255.255.255.0 or 10.0.0.1+100\n");
    printf("    SP+             -- TCP/UDP source port & mask or source port increment, like: 80,80/0x00ff, 1024+10\n");
    printf("    DP+             -- TCP/UDP destination port & mask or destination port increment, like: 80,80/0x00ff, 1024+10\n");
    printf("    NUM             -- Numer to del.\n");
}

void frctweak_cmd_rule_add_usage()
{
    printf("USAGE:       %s rule add INDEX SIP DIP SP DP PROTO\n",program);
    printf("\n  OPTION:\n");
    printf("    INDEX           -- Index, from 1 to 2000.\n");
    printf("    SIP+            -- Source IP or source ip increment, like: 192.168.1.0/255.255.255.0 or 10.0.0.1+100\n");
    printf("    DIP+            -- Destination IP or source ip increment, like: 192.168.1.0/255.255.255.0 or 10.0.0.1+100\n");
    printf("    SP+             -- TCP/UDP source port & mask or source port increment, like: 80,80/0x00ff, 1024+10\n");
    printf("    DP+             -- TCP/UDP destination port & mask or destination port increment, like: 80,80/0x00ff, 1024+10\n");
    printf("    PROTO           -- Protocol, like: (TCP|UDP|ANY).\n");
}


void frctweak_cmd_rule_del_usage()
{
    printf("Usage: %s rule del INDEX NUM\n",program);
    printf("\n  OPTION:\n");
    printf("    INDEX           -- Index, from 1 to 2000.\n");
    printf("    NUM             -- Numer to del.\n");
}

void frctweak_cmd_rule_get_usage()
{
    printf("Usage: %s rule get INDEX NUM\n",program);
    printf("\n  OPTION:\n");
    printf("    INDEX           -- Index, from 1 to 2000.\n");
    printf("    NUM             -- Numer to del.\n");
}

void frctweak_cmd_rule_stat_clear_usage()
{
    printf("Usage: %s rule stat_clear {INDEX NUM, all}\n",program);
    printf("\n  OPTION:\n");
    printf("    INDEX           -- Index, from 1 to 2000.\n");
    printf("    NUM             -- Numer to del.\n");
}


void frctweak_cmd_rule_cnt_usage()
{
    printf("Usage: %s rule cnt INDEX NUM\n",program);
    printf("\n  OPTION:\n");
    printf("    INDEX           -- Index, from 1 to 2000.\n");
    printf("    NUM             -- Numer to del.\n");
}

void frctweak_cmd_rule_match_usage()
{
    printf("USAGE:       %s rule match SIP DIP SP DP PROTO\n",program);
    printf("\n  OPTION:\n");
    printf("    INDEX           -- Index, from 1 to 2000.\n");
    printf("    SIP+            -- Source IP or source ip increment, like: 192.168.1.0/255.255.255.0 or 10.0.0.1+100\n");
    printf("    DIP+            -- Destination IP or source ip increment, like: 192.168.1.0/255.255.255.0 or 10.0.0.1+100\n");
    printf("    SP+             -- TCP/UDP source port & mask or source port increment, like: 80,80/0x00ff, 1024+10\n");
    printf("    DP+             -- TCP/UDP destination port & mask or destination port increment, like: 80,80/0x00ff, 1024+10\n");
    printf("    PROTO           -- Protocol, like: (TCP|UDP|ANY).\n");
}


int frctweak_cmd_rule(int argc, char **argv)
{
    int rv;
    int op;
    uint8_t enable;
    //uint64_t rule_enable;
    frc_rule_t rule[RULE_MAX];
    frc_rule_stat_t rule_stat[RULE_MAX];
    frc_rule_op_in_t input;
    frc_rule_pkt_stat_t cnt;
    frc_rule_stat_out_t stat_out;

    int i, j, k;

    uint32_t sip, sips, sipi, sipe, dip, dips, dipi, dipe;
    uint16_t sp, sps, spi, spe, dp, dps, dpi, dpe;
    uint32_t total = 0, cur = 0;
    uint8_t rule_source = 0, rule_type = 0, action = 0, rule_op = 0;
    uint16_t index, proto;
    char str_sip[18], str_dip[18], str_sp[18], str_dp[18], str_proto[18];
    uint16_t clear_num, rule_num, match_num;
    uint16_t num = 0;
    //char sip[18];
    //char dip[18];
    char buf[sizeof(frc_rule_stat_out_t)], *p;

    memset(rule, 0, sizeof(frc_rule_t));
    memset(&rule_stat, 0 , sizeof(frc_rule_stat_t));
    memset(&input, 0, sizeof(frc_rule_op_in_t));
    memset(&cnt, 0, sizeof(frc_rule_pkt_stat_t));
    memset(&stat_out, 0, sizeof(frc_rule_stat_out_t));

    //printf("argc  = %d , argv[0] = %s ,argv[1]= %s\n", argc, argv[0], argv[1]);

    if (argc < 2)
    {
        frctweak_cmd_rule_usage();
        return FRE_SUCCESS;
    }

    if (!(strcmp("status", argv[1])))
    {
        op = FRCTWEAK_RULE_STATUS_CMD;
    }

    else if (!(strcmp("add", argv[1])))
    {
        op = FRCTWEAK_RULE_ADD_CMD;
    }

    else if (!(strcmp("del", argv[1])))
    {
        op = FRCTWEAK_RULE_DEL_CMD;
    }

    else if (!(strcmp("stat_clear", argv[1])))
    {
        op = FRCTWEAK_RULE_STAT_CLEAR_CMD;
    }

    else if (!(strcmp("clear", argv[1])))
    {
        op = FRCTWEAK_RULE_CLEAR_CMD;
    }

    else if (!(strcmp("get", argv[1])))
    {
        op = FRCTWEAK_RULE_GET_CMD;
    }

    else if (!strcmp("enable", argv[1]))
    {
        op = FRCTWEAK_RULE_ENABLE_CMD;
    }

    else if (!strcmp("disable", argv[1]))
    {
        op = FRCTWEAK_RULE_DISABLE_CMD;
    }
    else if (!strcmp("cnt", argv[1]))
    {
        op = FRCTWEAK_RULE_CNT_CMD;
    }
    else if (!strcmp("update", argv[1]))
    {
        op = FRCTWEAK_RULE_UPDATE_CMD;
    }
    else if (!strcmp("num", argv[1]))
    {
        op = FRCTWEAK_RULE_NUM_GET_CMD;
    }
    else if (!strcmp("match", argv[1]))
    {
        op = FRCTWEAK_RULE_MATCH_CMD;
    }
    else
    {
        frctweak_cmd_rule_usage();
        return 0;
    }

    switch (op)
    {
    case FRCTWEAK_RULE_STATUS_CMD:
        rv = frcapi_rule_status_get(&enable);
        printf("RULE function status: %s\n", enable?"enable":"disable");
        break;
    case FRCTWEAK_RULE_ADD_CMD:
        if ((argc < 8) || (argc > 11))
        {
            frctweak_cmd_rule_add_usage();
            return FRE_SUCCESS;
        }

        sscanf(argv[2], "%hu", &index);
        if (index <= 0 || index > RULE_MAX)
        {
            FRCTWEAK_ERROR("Invalid rule index!");
            return FRE_PARAM;
        }

        if (parse_ipv4_inc(argv[3], &sips, &sipi))
        {
            return FRE_PARAM;
        }

        if (parse_ipv4_inc(argv[4], &dips, &dipi))
        {
            return FRE_PARAM;
        }

        if (parse_u16_inc(argv[5], &sps, &spi))
        {
            return FRE_PARAM;
        }

        if (parse_u16_inc(argv[6], &dps, &dpi))
        {
            return FRE_PARAM;
        }

        if (parse_protocal(argv[7], &proto))
        {
            return FRE_PARAM;
        }

        if (11 == argc)
        {
            if (parse_u8(argv[8], &rule_source))
            {
                return FRE_PARAM;
            }

            if (parse_u8(argv[9], &rule_type))
            {
                return FRE_PARAM;
            }

            if (parse_u8(argv[10], &rule_op))
            {
                return FRE_PARAM;
            }
        }

        total = sipi * dipi * spi * dpi;

        if (total > RULE_MAX)
        {
            printf("Total %d rules, out of bond.\n", total);
            return FRE_PARAM;
        }

        sipe = sips + sipi;
        dipe = dips + dipi;
        spe = sps + spi;
        dpe = dps + dpi;

        for (sip = sips; sip < sipe; sip++)
        {
            for (dip = dips; dip < dipe; dip++)
            {
                for (sp = sps; sp < spe; sp++)
                {
                    for (dp = dps; dp < dpe; dp++)
                    {

                       rule[cur].sip    = sip;
                       rule[cur].dip    = dip;
                       rule[cur].sp     = sp;
                       rule[cur].dp     = dp;
                       rule[cur].proto  = proto;
                       rule[cur].rule_source = rule_source;
                       rule[cur].rule_type = rule_type;
                       rule[cur].action = action;
                       rule[cur].op = rule_op;
                       rule[cur].index = index;

                       rv = frcapi_rule_add(&rule[cur]);

                       cur ++;
                       index ++;
                    }
                }
            }
        }

        break;
    case FRCTWEAK_RULE_DEL_CMD:
        if (argc < 4)
        {
            frctweak_cmd_rule_del_usage();
            return FRE_SUCCESS;
        }
        sscanf(argv[2], "%d", &input.index);
        if (input.index <= 0 || input.index > RULE_MAX)
        {
            FRCTWEAK_ERROR("Invalid rule index!");
            return FRE_PARAM;
        }

        sscanf(argv[3], "%d", &input.num);
        if (input.num < 1 || input.num > RULE_MAX)
        {
            FRCTWEAK_ERROR("Invalid rule number!");
            return FRE_PARAM;
        }

        if (input.index + input.num > RULE_MAX + 1)
        {
            return FRE_PARAM;
        }
        rv = frcapi_rule_del(&input, (uint16_t *)&input.num);
        break;
    case FRCTWEAK_RULE_STAT_CLEAR_CMD:
        if ((argc < 3) || (argc > 4))
        {
            frctweak_cmd_rule_stat_clear_usage();
            return FRE_SUCCESS;
        }
        if (4 == argc)
        {
            sscanf(argv[2], "%d", &input.index);
            if (input.index < 1 || input.index > (RULE_MAX))
            {
                FRCTWEAK_ERROR("Invalid rule index!");
                return FRE_PARAM;
            }
            if (argc < 4)
            {
                frctweak_cmd_rule_usage();
                return FRE_PARAM;
            }
            sscanf(argv[3], "%d", &input.num);
            if (input.num < 1 || input.num > RULE_MAX)
            {
                FRCTWEAK_ERROR("Invalid rule number!");
                return FRE_PARAM;
            }

            if (input.index + input.num > RULE_MAX+1)
            {
                return FRE_PARAM;
            }
        }

        if (3 == argc)
        {
            if (strcmp("all", argv[2]))
            {
                return FRE_PARAM;
            }

            input.index = 1;
            input.num = RULE_MAX;
        }

        rv  = frcapi_rule_stat_clear(&input, &clear_num);
        if (rv != FRE_SUCCESS)
        {
            FRCTWEAK_ERROR("Clearing up the statistics of rules failed!\n");
        }
        break;
    case FRCTWEAK_RULE_CLEAR_CMD:
        input.index = 1;
        input.num = RULE_MAX;
        rv = frcapi_rule_clear(&input, &clear_num);
        if (rv != FRE_SUCCESS)
        {
            FRCTWEAK_ERROR("Clearing up rules failed\n");
        }
        break;
    case FRCTWEAK_RULE_GET_CMD:
        if (argc < 4)
        {
            frctweak_cmd_rule_get_usage();
            return FRE_SUCCESS;
        }
        sscanf(argv[2], "%d", &input.index);
        if (input.index < 1 || input.index > RULE_MAX)
        {
            FRCTWEAK_ERROR("Invalid rule index!");
            return FRE_PARAM;
        }
        sscanf(argv[3], "%d", &input.num);
        if (input.num < 1 || input.num > RULE_MAX)
        {
            FRCTWEAK_ERROR("Invalid rule number!");
            return FRE_PARAM;
        }

        if (input.index + input.num > (RULE_MAX+1))
        {
            return FRE_PARAM;
        }
        index   = input.index;
        rule_num = input.num;
        num     = input.num / MAX_RULE;
        if (input.num%MAX_RULE)
        {
            num +=1;
        }
        printf("%-6s %-18s %-18s %-6s %-6s %-6s %-8s %-8s %-8s %-6s %-8s %-4s\n",
               "INDEX","SIP", "DIP", "SP", "DP", "PROTO", "PKTS", "BYTES", "SOURCE", "TYPE", "ACTION", "OP");
        for (j = 0; j < num; j++)
        {
            p = buf;
            if (j == num-1)
            {
                if (rule_num%MAX_RULE)
                {
                    input.num = rule_num % MAX_RULE;
                }else {
                    input.num = MAX_RULE;
                }

            } else {
                input.num = MAX_RULE;
            }

            input.index = index + j * MAX_RULE;
            rv = frcapi_rule_stat_get(&input, (frc_rule_stat_out_t *)buf);
            UNPACK_U32(p, stat_out.num);

            for (i =0; i < stat_out.num;i++)
            {
                p = frc_rule_stat_unpack(p, &stat_out.rule_stat[i]);
                fr_ipv4_string(stat_out.rule_stat[i].rule.sip, str_sip);
                fr_ipv4_string(stat_out.rule_stat[i].rule.dip, str_dip);
                fr_port_string(stat_out.rule_stat[i].rule.sp, str_sp);
                fr_port_string(stat_out.rule_stat[i].rule.dp, str_dp);
                fr_proto_string(stat_out.rule_stat[i].rule.proto, str_proto);
                printf("%-6d %-18s %-18s %-6s %-6s %-6s %-8d %-8d %-8d %-6d %-8d %-4d\n",
                       stat_out.rule_stat[i].rule.index, str_sip, str_dip, str_sp, str_dp, str_proto,
                       stat_out.rule_stat[i].stat.pkts, stat_out.rule_stat[i].stat.bytes,
                       stat_out.rule_stat[i].rule.rule_source, stat_out.rule_stat[i].rule.rule_type, stat_out.rule_stat[i].rule.action, stat_out.rule_stat[i].rule.op);
            }
        }
        break;
    case FRCTWEAK_RULE_ENABLE_CMD:
    case FRCTWEAK_RULE_DISABLE_CMD:
        if (parse_bool(argv[1], &enable))
        {
            return FRE_PARAM;
        }
        //rule_enable = enable;
        rv = frcapi_rule_enable(enable);
        break;
    case FRCTWEAK_RULE_CNT_CMD:
        if (argc < 4)
        {
            frctweak_cmd_rule_cnt_usage();
            return FRE_SUCCESS;
        }
        sscanf(argv[2], "%d", &input.index);
        if (input.index < 1 || input.index > RULE_MAX)
        {
            FRCTWEAK_ERROR("Invalid rule index!");
            return FRE_PARAM;
        }

        sscanf(argv[3], "%d", &input.num);
        if (input.num < 1 || input.num > RULE_MAX)
        {
            FRCTWEAK_ERROR("Invalid rule number!");
            return FRE_PARAM;
        }

        if (input.index + input.num > RULE_MAX + 1)
        {
            return FRE_PARAM;
        }

        index   = input.index;
        rule_num = input.num;
        num     = input.num / MAX_RULE;
        if (input.num%MAX_RULE)
        {
            num +=1;
        }
        for (j = 0; j < num; j++)
        {
            p = buf;
            if (j == num-1)
            {
                if (rule_num%MAX_RULE)
                {
                    input.num = rule_num % MAX_RULE;
                }else {
                    input.num = MAX_RULE;
                }

            } else {
                input.num = MAX_RULE;
            }

            input.index = index + j * MAX_RULE;
            rv = frcapi_rule_stat_get(&input, (frc_rule_stat_out_t *)buf);
            UNPACK_U32(p, stat_out.num);


            for (i =0; i < stat_out.num;i++)
            {
                p = frc_rule_stat_unpack(p, &stat_out.rule_stat[i]);
                cnt.pkts += stat_out.rule_stat[i].stat.pkts;
                cnt.bytes += stat_out.rule_stat[i].stat.bytes;
            }
        }

        //rv  = frcapi_rule_cnt_get(&input, &cnt);
        printf("sum_pkts: %d\n", cnt.pkts);
        printf("sum_bytes: %d\n", cnt.bytes);
        break;
    case FRCTWEAK_RULE_UPDATE_CMD:
        rv = frcapi_rule_update();
        break;
    case FRCTWEAK_RULE_NUM_GET_CMD:
        rv = frcapi_rule_num_get(&rule_num);
        printf("Current rules: %d\n", rule_num);
        break;
    case FRCTWEAK_RULE_MATCH_CMD:
        if (argc < 7)
        {
            frctweak_cmd_rule_match_usage();
            return FRE_SUCCESS;
        }

        if (parse_ipv4_inc(argv[2], &sips, &sipi))
        {
            printf("%s.%d\n", __func__, __LINE__);
            return FRE_PARAM;
        }

        if (parse_ipv4_inc(argv[3], &dips, &dipi))
        {
            return FRE_PARAM;
        }

        if (parse_u16_inc(argv[4], &sps, &spi))
        {
            printf("%s.%d\n", __func__, __LINE__);
            return FRE_PARAM;
        }

        if (parse_u16_inc(argv[5], &dps, &dpi))
        {
            return FRE_PARAM;
        }

        if (parse_protocal(argv[6], &proto))
        {
            return FRE_PARAM;
        }


        total = sipi * dipi * spi * dpi;

        if (total > RULE_MAX)
        {
            printf("Total %d rules, out of bond.\n", total);
            return FRE_PARAM;
        }

        //rv = frcapi_rule_num_get(&rule_num);
        rule_num = RULE_MAX;

        k=0;
        input.index = 1;
        input.num = rule_num;
        index   = input.index;
        num     = input.num / MAX_RULE;
        if (input.num%MAX_RULE)
        {
            num +=1;
        }
        for (j = 0; j < num; j++)
        {
            p = buf;
            if (j == num-1)
            {
                if (rule_num%MAX_RULE)
                {
                    input.num = rule_num % MAX_RULE;
                }else {
                    input.num = MAX_RULE;
                }

            } else {
                input.num = MAX_RULE;
            }

            input.index = index + j * MAX_RULE;
            rv = frcapi_rule_stat_get(&input, (frc_rule_stat_out_t *)buf);
            UNPACK_U32(p, stat_out.num);


            for (i =0; i < stat_out.num;i++)
            {
                p = frc_rule_stat_unpack(p, &stat_out.rule_stat[i]);
                memcpy(&rule_stat[k], &stat_out.rule_stat[i], sizeof(frc_rule_stat_t));
                k ++;
            }

        }

        sipe = sips + sipi;
        dipe = dips + dipi;
        spe = sps + spi;
        dpe = dps + dpi;

        for (sip = sips; sip < sipe; sip++)
        {
            for (dip = dips; dip < dipe; dip++)
            {
                for (sp = sps; sp < spe; sp++)
                {
                    for (dp = dps; dp < dpe; dp++)
                    {
                       rule[cur].sip    = sip;
                       rule[cur].dip    = dip;
                       rule[cur].sp     = sp;
                       rule[cur].dp     = dp;
                       rule[cur].proto  = proto;
#if 0
                       printf("sip = 0x%x\n", rule[cur].sip);
                       printf("dip = 0x%x\n", rule[cur].dip);
                       printf("sp = %d\n", rule[cur].sp);
                       printf("dp = %d\n", rule[cur].dp);
                       printf("proto = %d, total = %d\n", rule[cur].proto, total);
#endif
                       //rv = frcapi_rule_match(&rule[cur], &num);

                       rv = frcapi_rule_match(rule_stat, &rule[cur], rule_num, &match_num);
                       cur ++;

                    }
                }
            }
        }
        printf("matched: %d\n", match_num);
        break;
    default:
        return FRE_PARAM;
    }

    return rv;

}

int frctweak_rule_cmd_init(frctweak_cmd_t *cmd)
{
    frctweak_cmd_register(cmd, "rule", "Get or set for rule module", frctweak_cmd_rule, frctweak_cmd_rule_usage);

    return 0;

}


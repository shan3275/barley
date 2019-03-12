#include <stdlib.h>
#include <string.h>

#include "frctweak.h"
#include "frc_pack.h"
#include "frc_dma.h"
#include "cvmx-swap.h"
#include "frc_api.h"
#include "frc_debug.h"

#if FRC_CONFIG_TWO_TUPLE

typedef enum{
    FRCTWEAK_ACL_STATUS_CMD,
    FRCTWEAK_ACL_ADD_CMD,
    FRCTWEAK_ACL_DEL_CMD,
    FRCTWEAK_ACL_STAT_CLEAR_CMD,
    FRCTWEAK_ACL_CLEAR_CMD,
    FRCTWEAK_ACL_GET_CMD,
    FRCTWEAK_ACL_ENABLE_CMD,
    FRCTWEAK_ACL_DISABLE_CMD,
    FRCTWEAK_ACL_CNT_CMD,
    FRCTWEAK_ACL_UPDATE_CMD,
    FRCTWEAK_ACL_Num_GET_CMD,
    FRCTWEAK_ACL_MATCH_CMD,
    FRCTWEAK_ACL_GET_HASH_TABLE_CMD,
}frctweak_acl_t;

void frctweak_cmd_acl_usage()
{
    printf("Usage: %s acl SUBCOMMAND OPTION\n",program);
    printf("       %s acl status\n",program);
    printf("       %s acl enable\n",program);
    printf("       %s acl disable\n",program);
    printf("       %s acl add          Index Acl_type (SIP|DIP|SP|DP) PROTO\n",program);
    printf("       %s acl del          Index Num\n", program);
    printf("       %s acl get          Index Num\n", program);
    printf("       %s acl stat_clear   Index Num\n", program);
    printf("       %s acl num\n", program);
    printf("       %s acl match        Acl_type (SIP|DIP|SP|DP) PROTO\n", program);
    printf("       %s acl cnt          Index Num\n", program);
    printf("       %s acl clear                 \n", program);
    printf("       %s acl hash                   ", program);
    printf("\n  SUBCOMMAND:\n");
    printf("    status          -- Get acl module status:(enable|disable).\n");
    printf("    enable          -- Enable acl module.\n");
    printf("    disable         -- Disable acl module.\n");
    printf("    add             -- Add one acl.\n");
    printf("    del             -- Delete acl.\n");
    printf("    get             -- Get acl.\n");
    printf("    stat_clear      -- Clear acl statistics.\n");
    printf("    num             -- Get acl num.\n");
    printf("    match           -- Match acl.\n");
    printf("    cnt             -- Print total statistics of acl num.\n");
    printf("    clear           -- Clear all acl.\n");
    printf("    hash            -- Get acl hash table.\n");
    printf("\n  OPTION:\n");
    printf("    Index           -- Index, from 1 to 2000.\n");
    printf("    Acl_type        -- Acl Type,like:SIP DIP SP DP.\n");
    printf("    SIP             -- Source IP or ip increment: like 1.1.1.0 or 1.1.1.0+100\n");
    printf("    DIP             -- Destination IP or ip increment, like: 1.1.1.0 or 1.1.1.0+100\n");
    printf("    SP              -- TCP/UDP source port or port increment, like: 80 or 80+100\n");
    printf("    DP              -- TCP/UDP destination port or port increment, like: 80 or 80+100\n");
    printf("    PROTO           -- Protocol, like: (TCP|UDP).\n");
    printf("    Num             -- Numer to del.\n");
}

void frctweak_cmd_acl_add_usage()
{
    printf("USAGE:       %s acl add Index Acl_type (SIP|DIP|SP|DP) PROTO\n",program);
    printf("\n  OPTION:\n");
    printf("    Index           -- Index, from 1 to 2000.\n");
    printf("    Acl_type        -- Acl Type,like:SIP DIP SP DP.\n");
    printf("    SIP             -- Source IP or ip increment: like 1.1.1.0 or 1.1.1.0+100\n");
    printf("    DIP             -- Destination IP or ip increment, like: 1.1.1.0 or 1.1.1.0+100\n");
    printf("    SP              -- TCP/UDP source port or port increment, like: 80 or 80+100\n");
    printf("    DP              -- TCP/UDP destination port or port increment, like: 80 or 80+100\n");
    printf("    PROTO           -- Protocol, like: (TCP|UDP).\n");
}


void frctweak_cmd_acl_del_usage()
{
    printf("Usage: %s acl del Index Num\n",program);
    printf("\n  OPTION:\n");
    printf("    Index           -- Index, from 1 to 2000.\n");
    printf("    Num             -- Numer to del.\n");
}

void frctweak_cmd_acl_get_usage()
{
    printf("Usage: %s acl get Index Num\n",program);
    printf("\n  OPTION:\n");
    printf("    Index           -- Index, from 1 to 2000.\n");
    printf("    Num             -- Numer to del.\n");
}

void frctweak_cmd_acl_hash_table_get_usage()
{
    printf("Usage: %s acl hash Acl_type\n",program);
    printf("\n  OPTION:\n");
    printf("    Acl_type        -- Acl Type,like:SIP DIP SP DP.\n");
}

void frctweak_cmd_acl_stat_clear_usage()
{
    printf("Usage: %s acl stat_clear {Index Num, all}\n",program);
    printf("\n  OPTION:\n");
    printf("    Index           -- Index, from 1 to 2000.\n");
    printf("    Num             -- Numer to del.\n");
}


void frctweak_cmd_acl_cnt_usage()
{
    printf("Usage: %s acl cnt Index Num\n",program);
    printf("\n  OPTION:\n");
    printf("    Index           -- Index, from 1 to 2000.\n");
    printf("    Num             -- Numer to del.\n");
}

void frctweak_cmd_acl_match_usage()
{
    printf("USAGE:       %s acl match  Acl_type (SIP|DIP|SP|DP) PROTO\n",program);
    printf("\n  OPTION:\n");
    printf("    Acl_type        -- Acl Type,like:SIP DIP SP DP.\n");
    printf("    SIP             -- Source IP or ip increment: like 1.1.1.0 or 1.1.1.0+100\n");
    printf("    DIP             -- Destination IP or ip increment, like: 1.1.1.0 or 1.1.1.0+100\n");
    printf("    SP              -- TCP/UDP source port or port increment, like: 80 or 80+100\n");
    printf("    DP              -- TCP/UDP destination port or port increment, like: 80 or 80+100\n");
    printf("    PROTO           -- Protocol, like: (TCP|UDP.\n");
}


int frctweak_cmd_acl(int argc, char **argv)
{
    int rv;
    int op;
    uint8_t enable;
    frc_acl_t acl[ACL_MAX];
    frc_acl_stat_t acl_stat[ACL_MAX];
    frc_acl_op_in_t input;
    frc_acl_hash_table_op_in_t hash_input;
    frc_acl_hash_table_stat_out_t hash_out;
    frc_acl_pkt_stat_t cnt;
    frc_acl_stat_out_t stat_out;

    int i, j, k;
    uint32_t one_tuple, inc;
    uint32_t cur = 0;
    uint8_t acl_source = 0, action = 0, acl_op = 0;
    uint16_t acl_type = 0;
    uint16_t index, proto;
    char str_sip[18], str_dip[18], str_sp[18], str_dp[18], str_proto[18];
    uint16_t clear_num, acl_num, match_num = 0;
    uint16_t num = 0;
    char buf[sizeof(frc_acl_stat_out_t)], *p;
    uint16_t hash_num = 0;

    memset(acl, 0, sizeof(frc_acl_t));
    memset(&acl_stat, 0 , sizeof(frc_acl_stat_t));
    memset(&input, 0, sizeof(frc_acl_op_in_t));
    memset(&cnt, 0, sizeof(frc_acl_pkt_stat_t));
    memset(&stat_out, 0, sizeof(frc_acl_stat_out_t));

    //printf("argc  = %d , argv[0] = %s ,argv[1]= %s\n", argc, argv[0], argv[1]);

    if (argc < 2)
    {
        frctweak_cmd_acl_usage();
        return FRE_SUCCESS;
    }

    if (!(strcmp("status", argv[1])))
    {
        op = FRCTWEAK_ACL_STATUS_CMD;
    }

    else if (!(strcmp("add", argv[1])))
    {
        op = FRCTWEAK_ACL_ADD_CMD;
    }

    else if (!(strcmp("del", argv[1])))
    {
        op = FRCTWEAK_ACL_DEL_CMD;
    }

    else if (!(strcmp("stat_clear", argv[1])))
    {
        op = FRCTWEAK_ACL_STAT_CLEAR_CMD;
    }

    else if (!(strcmp("clear", argv[1])))
    {
        op = FRCTWEAK_ACL_CLEAR_CMD;
    }

    else if (!(strcmp("get", argv[1])))
    {
        op = FRCTWEAK_ACL_GET_CMD;
    }

    else if (!strcmp("enable", argv[1]))
    {
        op = FRCTWEAK_ACL_ENABLE_CMD;
    }

    else if (!strcmp("disable", argv[1]))
    {
        op = FRCTWEAK_ACL_DISABLE_CMD;
    }
    else if (!strcmp("cnt", argv[1]))
    {
        op = FRCTWEAK_ACL_CNT_CMD;
    }
    else if (!strcmp("num", argv[1]))
    {
        op = FRCTWEAK_ACL_Num_GET_CMD;
    }
    else if (!strcmp("match", argv[1]))
    {
        op = FRCTWEAK_ACL_MATCH_CMD;
    }
    else if (!strcmp("hash", argv[1]))
    {
        op = FRCTWEAK_ACL_GET_HASH_TABLE_CMD;
    }
    else
    {
        frctweak_cmd_acl_usage();
        return 0;
    }

    switch (op)
    {
    case FRCTWEAK_ACL_STATUS_CMD:
        rv = frcapi_acl_status_get(&enable);
        printf("ACL function status: %s\n", enable?"enable":"disable");
        break;
    case FRCTWEAK_ACL_ADD_CMD:
        if (argc < 6 || argc > 9)
        {
            frctweak_cmd_acl_add_usage();
            return FRE_SUCCESS;
        }

        sscanf(argv[2], "%hu", &index);
        if (index < ACL_INDEX_START || index > ACL_MAX)
        {
            FRCTWEAK_ERROR("Invalid acl index!");
            return FRE_PARAM;
        }

        if (parse_acl_type(argv[3], &acl_type))
        {
            FRCTWEAK_ERROR("Invalid acl type!");
            return FRE_PARAM;
        }

        if (acl_type == FRC_ACL_SIP || acl_type == FRC_ACL_DIP)
        {
            if (parse_ipv4_inc(argv[4], &one_tuple, &inc))
            {
                FRCTWEAK_ERROR("Invalid acl one_typle!");
                return FRE_PARAM;
            }
        } else if (acl_type == FRC_ACL_SP || acl_type == FRC_ACL_DP)
        {
            if (parse_u32_inc(argv[4], &one_tuple, &inc))
            {
                FRCTWEAK_ERROR("Invalid acl one_typle!");
                return FRE_PARAM;
            }
        }else {
            return FRE_PARAM;
        }

        if (parse_protocal(argv[5], &proto))
        {
            return FRE_PARAM;
        }

        if (9 == argc)
        {
            if (parse_u8(argv[6], &acl_source))
            {
                return FRE_PARAM;
            }

            if (parse_u8(argv[7], &action))
            {
                return FRE_PARAM;
            }

            if (parse_u8(argv[8], &acl_op))
            {
                return FRE_PARAM;
            }
        }

        acl[cur].one_tuple = one_tuple;
        acl[cur].proto  = proto;
        acl[cur].acl_source = acl_source;
        acl[cur].acl_type = acl_type;
        acl[cur].action = action;
        acl[cur].op = acl_op;
        acl[cur].index = index;
        for (i = 0; i < inc; i++)
        {
            rv = frcapi_acl_add(&acl[cur]);
            acl[cur].one_tuple ++ ;
            acl[cur].index ++;
        }
        break;
    case FRCTWEAK_ACL_DEL_CMD:
        if (argc < 4)
        {
            frctweak_cmd_acl_del_usage();
            return FRE_SUCCESS;
        }
        sscanf(argv[2], "%d", &input.index);
        if (input.index < ACL_INDEX_START || input.index > ACL_MAX)
        {
            FRCTWEAK_ERROR("Invalid acl index!");
            return FRE_PARAM;
        }

        sscanf(argv[3], "%d", &input.num);
        if (input.num < ACL_INDEX_START || input.num > ACL_MAX)
        {
            FRCTWEAK_ERROR("Invalid acl number!");
            return FRE_PARAM;
        }

        if (input.index + input.num > ACL_MAX + ACL_INDEX_START)
        {
            return FRE_PARAM;
        }
        rv = frcapi_acl_del(&input, (uint16_t *)&input.num);
        break;
    case FRCTWEAK_ACL_STAT_CLEAR_CMD:
        if ((argc < 3) || (argc > 4))
        {
            frctweak_cmd_acl_stat_clear_usage();
            return FRE_SUCCESS;
        }
        if (4 == argc)
        {
            sscanf(argv[2], "%d", &input.index);
            if (input.index < ACL_INDEX_START || input.index > (ACL_MAX))
            {
                FRCTWEAK_ERROR("Invalid acl index!");
                return FRE_PARAM;
            }
            if (argc < 4)
            {
                frctweak_cmd_acl_usage();
                return FRE_PARAM;
            }
            sscanf(argv[3], "%d", &input.num);
            if (input.num < ACL_INDEX_START || input.num > ACL_MAX)
            {
                FRCTWEAK_ERROR("Invalid acl number!");
                return FRE_PARAM;
            }

            if (input.index + input.num > ACL_MAX + ACL_INDEX_START)
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

            input.index = ACL_INDEX_START;
            input.num = ACL_MAX;
        }

        rv  = frcapi_acl_stat_clear(&input, &clear_num);
        if (rv != FRE_SUCCESS)
        {
            FRCTWEAK_ERROR("Clearing up the statistics of acls failed!\n");
        }
        break;
    case FRCTWEAK_ACL_CLEAR_CMD:
        input.index = ACL_INDEX_START;
        input.num = ACL_MAX;
        rv = frcapi_acl_clear(&input, &clear_num);
        if (rv != FRE_SUCCESS)
        {
            FRCTWEAK_ERROR("Clearing up acls failed\n");
        }
        break;
    case FRCTWEAK_ACL_GET_CMD:
        if (argc < 4)
        {
            frctweak_cmd_acl_get_usage();
            return FRE_SUCCESS;
        }
        sscanf(argv[2], "%d", &input.index);
        if (input.index < ACL_INDEX_START || input.index > ACL_MAX)
        {
            FRCTWEAK_ERROR("Invalid acl index!");
            return FRE_PARAM;
        }
        sscanf(argv[3], "%d", &input.num);
        if (input.num < ACL_INDEX_START || input.num > ACL_MAX)
        {
            FRCTWEAK_ERROR("Invalid acl number!");
            return FRE_PARAM;
        }

        if (input.index + input.num > (ACL_MAX+ACL_INDEX_START))
        {
            return FRE_PARAM;
        }
        index   = input.index;
        acl_num = input.num;
        num     = input.num / MAX_ACL;
        if (input.num%MAX_ACL)
        {
            num +=1;
        }
        printf("%-6s %-18s %-18s %-6s %-6s %-6s %-8s %-8s %-6s %-8s %-6s %-8s %-4s\n",
               "Index","SIP", "DIP", "SP", "DP", "PROTO", "PKTS", "BYTES", "HASH",
               "SOURCE", "TYPE", "ACTION", "OP");
        for (j = 0; j < num; j++)
        {
            p = buf;
            if (j == num-1)
            {
                if (acl_num%MAX_ACL)
                {
                    input.num = acl_num % MAX_ACL;
                }else {
                    input.num = MAX_ACL;
                }

            } else {
                input.num = MAX_ACL;
            }

            input.index = index + j * MAX_ACL;
            rv = frcapi_acl_stat_get(&input, (frc_acl_stat_out_t *)buf);
            UNPACK_U32(p, stat_out.num);

            for (i =0; i < stat_out.num;i++)
            {
                p = frc_acl_stat_unpack(p, &stat_out.acl_stat[i]);
                if (stat_out.acl_stat[i].acl.acl_type == FRC_ACL_SIP)
                {
                    fr_ipv4_string(stat_out.acl_stat[i].acl.one_tuple, str_sip);
                    fr_ipv4_string(0, str_dip);
                    fr_port_string(0, str_sp);
                    fr_port_string(0, str_dp);
                }else if (stat_out.acl_stat[i].acl.acl_type == FRC_ACL_DIP)
                {
                    fr_ipv4_string(0, str_sip);
                    fr_ipv4_string(stat_out.acl_stat[i].acl.one_tuple, str_dip);
                    fr_port_string(0, str_sp);
                    fr_port_string(0, str_dp);
                }else if (stat_out.acl_stat[i].acl.acl_type == FRC_ACL_SP)
                {
                    fr_ipv4_string(0, str_sip);
                    fr_ipv4_string(0, str_dip);
                    fr_port_string(stat_out.acl_stat[i].acl.one_tuple, str_sp);
                    fr_port_string(0, str_dp);
                }else if (stat_out.acl_stat[i].acl.acl_type == FRC_ACL_DP)
                {
                    fr_ipv4_string(0, str_sip);
                    fr_ipv4_string(0, str_dip);
                    fr_port_string(0, str_sp);
                    fr_port_string(stat_out.acl_stat[i].acl.one_tuple, str_dp);
                }
                fr_proto_string(stat_out.acl_stat[i].acl.proto, str_proto);
                printf("%-6d %-18s %-18s %-6s %-6s %-6s %-8d %-8d %-6d %-8d %-6d %-8d %-4d\n",
                       stat_out.acl_stat[i].acl.index, str_sip, str_dip, str_sp, str_dp, str_proto,
                       stat_out.acl_stat[i].stat.pkts, stat_out.acl_stat[i].stat.bytes,
                       stat_out.acl_stat[i].acl.hash,  stat_out.acl_stat[i].acl.acl_source,
                       stat_out.acl_stat[i].acl.acl_type, stat_out.acl_stat[i].acl.action,
                       stat_out.acl_stat[i].acl.op);
            }
        }
        break;
    case FRCTWEAK_ACL_ENABLE_CMD:
    case FRCTWEAK_ACL_DISABLE_CMD:
        if (parse_bool(argv[1], &enable))
        {
            return FRE_PARAM;
        }
        //acl_enable = enable;
        rv = frcapi_acl_enable(enable);
        break;
    case FRCTWEAK_ACL_CNT_CMD:
        if (argc < 4)
        {
            frctweak_cmd_acl_cnt_usage();
            return FRE_SUCCESS;
        }
        sscanf(argv[2], "%d", &input.index);
        if (input.index < ACL_INDEX_START || input.index > ACL_MAX)
        {
            FRCTWEAK_ERROR("Invalid acl index!");
            return FRE_PARAM;
        }

        sscanf(argv[3], "%d", &input.num);
        if (input.num < ACL_INDEX_START || input.num > ACL_MAX)
        {
            FRCTWEAK_ERROR("Invalid acl number!");
            return FRE_PARAM;
        }

        if (input.index + input.num > ACL_MAX + ACL_INDEX_START)
        {
            return FRE_PARAM;
        }

        index   = input.index;
        acl_num = input.num;
        num     = input.num / MAX_ACL;
        if (input.num%MAX_ACL)
        {
            num +=1;
        }
        for (j = 0; j < num; j++)
        {
            p = buf;
            if (j == num-1)
            {
                if (acl_num%MAX_ACL)
                {
                    input.num = acl_num % MAX_ACL;
                }else {
                    input.num = MAX_ACL;
                }

            } else {
                input.num = MAX_ACL;
            }

            input.index = index + j * MAX_ACL;
            rv = frcapi_acl_stat_get(&input, (frc_acl_stat_out_t *)buf);
            UNPACK_U32(p, stat_out.num);


            for (i =0; i < stat_out.num;i++)
            {
                p = frc_acl_stat_unpack(p, &stat_out.acl_stat[i]);
                cnt.pkts += stat_out.acl_stat[i].stat.pkts;
                cnt.bytes += stat_out.acl_stat[i].stat.bytes;
            }
        }

        //rv  = frcapi_acl_cnt_get(&input, &cnt);
        printf("sum_pkts: %d\n", cnt.pkts);
        printf("sum_bytes: %d\n", cnt.bytes);
        break;
    case FRCTWEAK_ACL_Num_GET_CMD:
        rv = frcapi_acl_num_get(&acl_num);
        printf("Current acls: %d\n", acl_num);
        break;
    case FRCTWEAK_ACL_MATCH_CMD:
        if (argc < 5)
        {
            frctweak_cmd_acl_match_usage();
            return FRE_SUCCESS;
        }

        if (parse_acl_type(argv[2], &acl_type))
        {
            FRCTWEAK_ERROR("Invalid acl type!");
            return FRE_PARAM;
        }

        if (acl_type == FRC_ACL_SIP || acl_type == FRC_ACL_DIP)
        {
            if (parse_ipv4_inc(argv[3], &one_tuple, &inc))
            {
                FRCTWEAK_ERROR("Invalid acl one_typle!");
                return FRE_PARAM;
            }
        } else if (acl_type == FRC_ACL_SP || acl_type == FRC_ACL_DP)
        {
            if (parse_u32_inc(argv[3], &one_tuple, &inc))
            {
                FRCTWEAK_ERROR("Invalid acl one_typle!");
                return FRE_PARAM;
            }
        }else {
            return FRE_PARAM;
        }

        if (parse_protocal(argv[4], &proto))
        {
            return FRE_PARAM;
        }

        acl_num = ACL_MAX;

        k=0;
        input.index = ACL_INDEX_START;
        input.num = acl_num;
        index   = input.index;
        num     = input.num / MAX_ACL;
        if (input.num%MAX_ACL)
        {
            num +=1;
        }
        for (j = 0; j < num; j++)
        {
            p = buf;
            if (j == num-1)
            {
                if (acl_num%MAX_ACL)
                {
                    input.num = acl_num % MAX_ACL;
                }else {
                    input.num = MAX_ACL;
                }

            } else {
                input.num = MAX_ACL;
            }

            input.index = index + j * MAX_ACL;
            rv = frcapi_acl_stat_get(&input, (frc_acl_stat_out_t *)buf);
            UNPACK_U32(p, stat_out.num);


            for (i =0; i < stat_out.num;i++)
            {
                p = frc_acl_stat_unpack(p, &stat_out.acl_stat[i]);
                memcpy(&acl_stat[k], &stat_out.acl_stat[i], sizeof(frc_acl_stat_t));
                k ++;
            }

        }

        acl[cur].one_tuple = one_tuple;
        acl[cur].proto     = proto;
        #if 0
        printf("one_tuple = %d\n", acl[cur].one_tuple);
        printf("proto = %d, total = %d\n", acl[cur].proto, total);
        #endif

        for (i = 0; i < inc; i++)
        {
            rv = frcapi_acl_match(acl_stat, &acl[cur], acl_num, &match_num);
            acl[cur].one_tuple ++;
        }

        printf("matched: %d\n", match_num);
        break;
    case FRCTWEAK_ACL_GET_HASH_TABLE_CMD:
        if (argc < 3)
        {
            frctweak_cmd_acl_hash_table_get_usage();
            return FRE_SUCCESS;
        }

        if (parse_acl_type(argv[2], &acl_type))
        {
            FRCTWEAK_ERROR("Invalid acl type!");
            return FRE_PARAM;
        }
        hash_input.acl_type = acl_type;
        hash_input.index = ACL_INDEX_START - 1;
        hash_input.num   = ACL_HASH_MAX;

        index   = hash_input.index;
        acl_num = hash_input.num;
        num     = hash_input.num / MAX_ACL;
        if (hash_input.num%MAX_ACL)
        {
            num +=1;
        }
        printf("%-6s %-18s %-18s %-18s\n", "Index","BucketDepth", "TotalCell", "DelCell");
        for (j = 0; j < num; j++)
        {
            p = buf;
            if (j == num-1)
            {
                if (acl_num%MAX_ACL)
                {
                    hash_input.num = acl_num % MAX_ACL;
                }else {
                    hash_input.num = MAX_ACL;
                }

            } else {
                hash_input.num = MAX_ACL;
            }

            hash_input.index = index + j * MAX_ACL;
            rv = frcapi_acl_hash_table_stat_get(&hash_input, &hash_out);
            swap_buff(sizeof(frc_acl_hash_table_stat_out_t)>>3, (void *)&hash_out);
            for (i =0; i < hash_out.num;i++)
            {
                printf("%-6d %-18d %-18d %-18d\n", hash_out.acl_hash_stat[i].hash,
                       hash_out.acl_hash_stat[i].bucket_depth, hash_out.acl_hash_stat[i].total_cell,
                       hash_out.acl_hash_stat[i].del_cell);
                hash_num += hash_out.acl_hash_stat[i].bucket_depth;
            }
        }
        printf("acl hash num:%d\n", hash_num);
        break;
    default:
        return FRE_PARAM;
    }

    return rv;

}

int frctweak_acl_cmd_init(frctweak_cmd_t *cmd)
{
    frctweak_cmd_register(cmd, "acl", "Get or set for acl module", frctweak_cmd_acl, frctweak_cmd_acl_usage);

    return 0;

}
#endif /* end of FRC_CONFIG_TWO_TUPLE*/

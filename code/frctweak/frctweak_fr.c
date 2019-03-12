#include <stdlib.h>
#include <string.h>

#include "frctweak.h"
#include "frc_pack.h"
#include "frc_dma.h"
#include "cvmx-swap.h"
#include "frc_api.h"
#include "frc_debug.h"

void frctweak_cmd_ssn_usage(void)
{
    printf("%s ssn match OPTIONS\n", program);
}

int frctweak_cmd_ssn(int argc, char **argv)
{
   printf("%s ssn match OPTIONS\n", program);
   return 0;
}



int fr_status_get(frc_fr_status_t *status)
{
    int rv;
    rv = frcapi_fr_status_get(status);
    if (rv != FRE_SUCCESS)
    {
        FRCTWEAK_ERROR("Get fr status fail: %d!\n", rv);
        return rv;
    }

    printf("FR function status: %s\n", status->enable?"enable":"disable");

    printf("ssn age: %d\n", status->age_time);
    //printf("retrans time: %d\n", status->retrans_time);
    printf("disorder max range: %d\n", status->disorder_depth);


    return FRE_SUCCESS;
}


void frctweak_cmd_fr_usage(void)
{
    printf("%s fr (age|disorder) PARAM\n", program);
    printf("%s fr status\n", program);
    printf("%s fr (enable/disable)\n", program);
    printf("  %-16s --%s", "age", "Set the age time for session, PARAM range from 1 to 15, initial value is 3\n");
    printf("  %-16s --%s", "disorder", "Set the disorder depth for flow restoration, PARAM range from 1 to 15, initial value is 5\n");
    printf("  %-16s --%s", "status", "Get the status for flow restoration\n");
    printf("  %-16s --%s", "(enable/disable)", "Enable or disable the flow restoration\n");
}

int frctweak_cmd_fr(int argc, char **argv)
{
    int rv;
    uint32_t age_time, disorder_depth;
    uint8_t enable;
    frc_fr_status_t status;

    memset(&status, 0, sizeof(frc_fr_status_t));

    if (argc < 2 || argc > 3)
    {
        frctweak_cmd_fr_usage();
        return FRE_SUCCESS;
    }

    if (argc == 2)
    {
        if (!strcasecmp("status", argv[1]))
        {
            memset(&status, 0, sizeof(frc_fr_status_t));
            rv = fr_status_get(&status);
        }

        else
        {
            if(parse_bool(argv[1], &enable))
            {
                return FRE_PARAM;
            }
            rv = frcapi_fr_enable(enable);
        }
    }

    if (argc == 3)
    {
        if (!strcasecmp("disorder", argv[1]))
        {
            sscanf(argv[2], "%d", &disorder_depth);
            if (disorder_depth < 1 || disorder_depth > 15)
            {
                FRCTWEAK_ERROR("Invalid disorder depth!");
                return FRE_PARAM;
            }

            rv = frcapi_fr_disorder_depth_set(disorder_depth);
        }

        else if (!strcasecmp("age", argv[1]))
        {
            sscanf(argv[2], "%d", &age_time);
            if (age_time < 1 || age_time > 2000)
            {
                FRCTWEAK_ERROR("Invalid age time!");
                return FRE_PARAM;
            }

            rv = frcapi_fr_age_time_set(age_time);
        }

        else
        {
            return FRE_PARAM;
        }
    }

    if (rv != FRE_SUCCESS)
    {
        FRCTWEAK_ERROR("Get or set flow restoration fail: %d!\n", rv);
        return rv;
    }

    return FRE_SUCCESS;

}



void frctweak_cmd_ssn_get_fivetuple_usage()
{
    printf("USAGE:%s ssn get SIP DIP SP DP PROTO\n", program);
}

int frctweak_cmd_ssn_get_fivetuple(int argc, char **argv)
{
    int rv, i;
    frc_fr_tuple_t ssn;
    frc_fr_session_stat_t stat;
    memset(&ssn, 0, sizeof(frc_fr_tuple_t));
    //memset(&five, 0, sizeof(frc_fr_tuple_t));
    memset(&stat, 0, sizeof(frc_fr_session_stat_t));

    if (argc < 6)
    {
        frctweak_cmd_ssn_get_fivetuple_usage();
        exit(0);
    }

    if (parse_ipv4(argv[1], &ssn.sip))
    {
        return FRE_PARAM;
    }

    if (parse_ipv4(argv[2], &ssn.dip))
    {
        return FRE_PARAM;
    }

    if (parse_u16(argv[3], &ssn.sp))
    {
        return FRE_PARAM;
    }

    if (parse_u16(argv[4], &ssn.dp))
    {
        return FRE_PARAM;
    }

    if (parse_protocal(argv[5], &ssn.proto))
    {
        return FRE_PARAM;
    }



    rv = frcapi_ssn_get(&ssn, &stat);
    //rv = frcapi_ssn_get(&ssn, &stat);
    //printf("%s.%d\n", __func__, __LINE__);
    if (rv != FRE_SUCCESS)
    {
        FRCTWEAK_ERROR("Get ssn by five tuple fail: %d!\n", rv);
        return rv;
    }

    //swap_buff(sizeof(frc_fr_tuple_t) >> 3, &five);

#if 1
    for (i = 0; i<sizeof(frc_fr_session_stat_t); i ++)
    {
        printf(" 0x%.2x\n", ((uint8_t *)(&stat))[i]);
    }
    swap_buff(sizeof(frc_fr_session_stat_t) >> 3, &stat);
    printf("PKTS: %d,   BYTES: %d\n", stat.pkts, stat.bytes);
#endif
    return FRE_SUCCESS;

}


void frctweak_cmd_ssn_bucket_get_hash_usage(void)
{
    printf("USAGE:%s get HASH\n", program);
}

int frctweak_cmd_ssn_bucket_get_hash(int argc, char **argv)
{
    int rv;
    int i;
    uint32_t hash;
    //char *p;
    //uint16_t olen;
    char sip[18], dip[18];
    frc_fr_hash_session_t bucket;

    char buf[sizeof(frc_fr_hash_session_t)], *p;

    p = buf;

    memset(&bucket, 0, sizeof(frc_fr_hash_session_t));

    if (argc != 3)
    {
        frctweak_cmd_ssn_bucket_get_hash_usage();
        exit(0);
    }

    if (strcasecmp("get", argv[1]))
    {
        return FRE_PARAM;
    }

    if (parse_u32(argv[2], &hash))
    {
        return FRE_PARAM;
    }

    //olen = sizeof(bucket);
    rv = frcapi_ssn_bucket_get(hash, (frc_fr_hash_session_t *)buf);
    if (rv != FRE_SUCCESS)
    {
        FRCTWEAK_ERROR("Get ssn bucket fail: %d!\n", rv);
        return rv;
    }

#if 0
    for (i = 0; i < sizeof(frc_fr_hash_session_t); i ++)
    {
        printf("0x%x\n", buf[i]);
    }
#endif


    UNPACK_U32(p, bucket.num);

    printf("ssn number: %d\n", bucket.num);
    printf("%-18s %-18s %-4s %-4s %-6s %-6s %-6s\n",
               "SIP", "DIP", "SP", "DP", "PROTO", "PKTS", "BYTES");
    for (i=0;i<bucket.num;i++)
    {
        p = frc_fr_session_t_unpack(p, &bucket.session[i]);
        fr_ipv4_string(bucket.session[i].five_tuple.sip, sip);
        fr_ipv4_string(bucket.session[i].five_tuple.dip, dip);
        printf("%-18s %-18s %-4d %-4d %-6d %-6d %-6d\n",
                   sip, dip, bucket.session[i].five_tuple.sp, bucket.session[i].five_tuple.dp, bucket.session[i].five_tuple.proto, bucket.session[i].session_stat.pkts, bucket.session[i].session_stat.bytes);
    }

    return FRE_SUCCESS;
}

void frctweak_cmd_ssn_match_usage()
{
    printf("USAGE: %s ssn match SIP DIP SP DP\n", program);
    printf("    SIP+            -- Source IP or source ip increment, like: 192.168.1.0/255.255.255.0 or 10.0.0.1+100\n");
    printf("    DIP+            -- Destination IP or source ip increment, like: 192.168.1.0/255.255.255.0 or 10.0.0.1+100\n");
    printf("    SP+             -- TCP/UDP source port & mask or source port increment, like: 80,80/0x00ff, 1024+10\n");
    printf("    DP+             -- TCP/UDP destination port & mask or destination port increment, like: 80,80/0x00ff, 1024+10\n");
}

int frctweak_cmd_ssn_match(int argc, char **argv)
{
    int rv;
    uint64_t num = 0;
    uint32_t sip, sips, sipi, sipe, dip, dips, dipi, dipe;
    uint16_t sp, sps, spi, spe, dp, dps, dpi, dpe;
    frc_ssn_match_t ssn_match;

    if (argc < 5)
    {
            frctweak_cmd_ssn_match_usage();
        return FRE_SUCCESS;
    }

    if (parse_ipv4_inc(argv[1], &sips, &sipi))
    {
        return FRE_PARAM;
    }

    if (parse_ipv4_inc(argv[2], &dips, &dipi))
    {
        return FRE_PARAM;
    }

    if (parse_u16_inc(argv[3], &sps, &spi))
    {
        return FRE_PARAM;
    }

    if (parse_u16_inc(argv[4], &dps, &dpi))
    {
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

                   ssn_match.tuple.sip = sip;
                   ssn_match.tuple.dip = dip;
                   ssn_match.tuple.sp  = sp;
                   ssn_match.tuple.dp  = dp;
                   ssn_match.tuple.proto = 6;

                   rv = frcapi_ssn_match(&ssn_match, &ssn_match.num);
                   swap_buff(1, &ssn_match.num);
                   if (FRE_SUCCESS == rv)
                   {
                       num += ssn_match.num;
                   }


                }
            }
        }
    }
    printf("matched: %lld\n", (ULL)num);
    return FRE_SUCCESS;
}


int frctweak_fr_cmd_init(frctweak_cmd_t *cmd)
{
    frctweak_cmd_t *ssn_cmd;
    frctweak_cmd_register(cmd, "fr", "Set or get flow restoration module", frctweak_cmd_fr, frctweak_cmd_fr_usage);
    //frctweak_cmd_register(cmd, "ssn_bucket", "Get ssn information hash", frctweak_cmd_ssn_bucket_get_hash, frctweak_cmd_ssn_bucket_get_hash_usage);
    ssn_cmd = frctweak_cmd_register(cmd, "ssn", "get the ssn information", frctweak_cmd_ssn, frctweak_cmd_ssn_usage);
    frctweak_cmd_register(ssn_cmd, "match", "Match the ssn from the specified five tuple", frctweak_cmd_ssn_match, frctweak_cmd_ssn_match_usage);
    //frctweak_cmd_register(ssn_cmd, "get", "get the ssn stat by five tuple", frctweak_cmd_ssn_get_fivetuple, frctweak_cmd_ssn_get_fivetuple_usage);
    return 0;

}


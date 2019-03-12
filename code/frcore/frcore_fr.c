#include "frcore.h"
#include "frcore_cmd.h"
#include "frcore_misc.h"
#include "frc_pack.h"

#include "cvmx-mdio.h"
#include "frcore_fr.h"
#include "frcore_ssn.h"

//extern CVMX_SHARD uint8_t frcore_ssn_age;
uint8_t retrans_time = 2;
extern CVMX_SHARED uint8_t disorder_depth;
extern CVMX_SHARED uint8_t fr_enable;

#if FRC_CONFIG_SSN

void get_ssn_stat_by_five_tuple(frc_fr_tuple_t *tuple, frc_fr_session_stat_t *stat)
{
    ;
}

void get_ssn_stat_bucket_by_hash(uint32_t hash, frc_fr_hash_session_t *hash_session)
{
    ;
}



int frcore_cmd_set_age_time(uint16_t plen, void *param, uint16_t *olen, void *outbuf)
{
    int rv;
    uint64_t time ;
    time = (*((uint8_t *)param));
    printf("time = %d\n", (int)time);

    rv = frcore_set_ssn_age((uint8_t)time);
    return rv;
}

int frcore_cmd_set_retrans_time(uint16_t plen, void *param, uint16_t *olen, void *outbuf)
{
    uint8_t *time = (uint8_t *) param;

    retrans_time = *time;

    return FRE_SUCCESS;
}


int frcore_cmd_set_disorder_depth(uint16_t plen, void *param, uint16_t *olen, void *outbuf)
{
    uint8_t *range = (uint8_t *) param;

    if (*range < FRCORE_DISORDER_DEPTH_MIN ||
        *range > FRCORE_DISORDER_DEPTH_MAX)
    {
        return FRE_EXCEED;
    }
    disorder_depth = *range;

    return FRE_SUCCESS;
}

int frcore_cmd_fr_enable(uint16_t plen, void *param, uint16_t *olen, void *outbuf)
{
    uint8_t *start = (uint8_t *) param;

    fr_enable = *start;

    return FRE_SUCCESS;
}


int frcore_cmd_fr_status_get(uint16_t plen, void *param, uint16_t *olen, void *outbuf)
{
    frc_fr_status_t status;
    memset(&status, 0 , sizeof(frc_fr_status_t));

    frcore_get_ssn_age(&status.age_time);
    status.retrans_time = retrans_time;
    status.disorder_depth = disorder_depth;
    status.enable = fr_enable;
    memcpy(outbuf, &status, sizeof(frc_fr_status_t));
    *olen = sizeof(frc_fr_status_t);

    return FRE_SUCCESS;

}

int frcore_cmd_ssn_get_by_five_tuple(uint16_t plen, void *param, uint16_t *olen, void *outbuf)
{
    //int i;
    //char *p;
    frc_fr_tuple_t *tuple = (frc_fr_tuple_t *)param;
    //p = (char *)param;
    frc_fr_session_stat_t session_stat;

    //frc_fr_tuple_t five;
    //memset(&five, 0, sizeof(frc_fr_tuple_t));
    memset(&session_stat, 0, sizeof(frc_fr_session_stat_t));

   // p = frc_tuple_unpack(p, &tuple);

    printf("===========================================================\n");
    printf("sip = 0x%x\n", tuple->sip);
    printf("dip = 0x%x\n", tuple->dip);
    printf("sp = %d\n", tuple->sp);
    printf("dp = %d\n", tuple->dp);
    printf("proto = %d\n", tuple->proto);
    printf("reserve = %d\n", tuple->reserve);

    //get_ssn_stat_by_five_tuple(&tuple, &session_stat);

    session_stat.pkts = 100;
    session_stat.bytes = 8000;


#if 0
    five.sip = 0xc0000003;
    five.dip = 0xc0000004;
    five.sp = 667;
    five.dp = 445;
    five.proto = 0x11;
#endif
    memcpy(outbuf, &session_stat, sizeof(frc_fr_session_stat_t));
    *olen = sizeof(frc_fr_session_stat_t);


    return FRE_SUCCESS;

}


int frcore_cmd_ssn_bucket_get_by_hash(uint16_t plen, void *param, uint16_t *olen, void *outbuf)
{
    //int i;
    uint32_t *value = (uint32_t *) param;
    frc_fr_hash_session_t hash_session;
    uint32_t hash = *value;

    memset(&hash_session, 0, sizeof(frc_fr_hash_session_t));
    get_ssn_stat_bucket_by_hash(hash, &hash_session);

    hash_session.num = 2;
    hash_session.session[0].five_tuple.sip = 0xca000001;
    hash_session.session[0].five_tuple.dip = 0xca000002;
    hash_session.session[0].five_tuple.sp = 998;
    hash_session.session[0].five_tuple.dp = 999;
    hash_session.session[0].five_tuple.proto = 0x11;
    hash_session.session[0].session_stat.pkts = 200;
    hash_session.session[0].session_stat.bytes = 20000;
    hash_session.session[1].five_tuple.sip = 0xc0000001;
    hash_session.session[1].five_tuple.dip = 0xc0000002;
    hash_session.session[1].five_tuple.sp = 898;
    hash_session.session[1].five_tuple.dp = 899;
    hash_session.session[1].five_tuple.proto = 0x6;
    hash_session.session[1].session_stat.pkts = 500;
    hash_session.session[1].session_stat.bytes = 50000;

    memcpy(outbuf, &hash_session, sizeof(frc_fr_hash_session_t));
    *olen = sizeof(frc_fr_hash_session_t);

    printf("olen = %d, outbuf length is %d\n", (int)(*olen), (int)sizeof(frc_fr_hash_session_t));
#if 0
    for (i = 0; i < sizeof(frc_fr_hash_session_t); i ++)
    {
        printf("0x%x\n", ((uint8_t *)outbuf)[i]);
    }
#endif
    return FRE_SUCCESS;

}


int frcore_cmd_ssn_match(uint16_t plen, void *param, uint16_t *olen, void *outbuf)
{
    frc_ssn_match_t *ssn = (frc_ssn_match_t *) param;
    int rv;
    uint64_t num;

    rv = frcore_match_ssn_by_tuple(ssn->tuple.sip, ssn->tuple.dip, ssn->tuple.sp, ssn->tuple.dp, ssn->tuple.proto);
#if 0
    printf("===========================================================\n");
    printf("sip = 0x%x\n", ssn->tuple.sip);
    printf("dip = 0x%x\n", ssn->tuple.dip);
    printf("sp = %d\n", ssn->tuple.sp);
    printf("dp = %d\n", ssn->tuple.dp);
    printf("proto = %d\n", ssn->tuple.proto);
    printf("reserve = %d\n", ssn->tuple.reserve);
#endif
    if (rv != FRE_SUCCESS)
    {
        FRCORE_ERROR("match this ssn fail! %d", rv);
        return rv;
    }

    num = 1;

    memcpy(outbuf, &num, 8);

    *olen = 8;

    return FRE_SUCCESS;
}


int
frcore_fr_init()
{
    FRCORE_REGISTER_CMD(CMD_TYPE_USER, USER_CMD_SET_AGE_TIME,  frcore_cmd_set_age_time);
    FRCORE_REGISTER_CMD(CMD_TYPE_USER, USER_CMD_SET_RETRANS_TIME,  frcore_cmd_set_retrans_time);
    FRCORE_REGISTER_CMD(CMD_TYPE_USER, USER_CMD_SET_DISORDER_DEPTH,  frcore_cmd_set_disorder_depth);
    FRCORE_REGISTER_CMD(CMD_TYPE_USER, USER_CMD_FR_ENABLE,  frcore_cmd_fr_enable);
    FRCORE_REGISTER_CMD(CMD_TYPE_USER, USER_CMD_GET_FR_STATUS,  frcore_cmd_fr_status_get);
    FRCORE_REGISTER_CMD(CMD_TYPE_USER, USER_CMD_SSN_GET_FIVETUPLE,  frcore_cmd_ssn_get_by_five_tuple);
    FRCORE_REGISTER_CMD(CMD_TYPE_USER, USER_CMD_SSN_BUCKET_GET,  frcore_cmd_ssn_bucket_get_by_hash);
    FRCORE_REGISTER_CMD(CMD_TYPE_USER, USER_CMD_SSN_MATCH,  frcore_cmd_ssn_match);

    return 0;
}
#endif

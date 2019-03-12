#include <string.h>
#include <stdint.h>
#include <unistd.h>

#include "frc_api.h"
#include "frc_ioctl.h"
#include <sys/ioctl.h>
#include <fcntl.h>
#include "frc_types.h"
#include "frc_cmd.h"
#include "frc_pack.h"


#if FRC_DEBUG_API
#   define FRCAPI_ERROR(_fmt, _args...)  printf("[ERROR] %s.%d:" _fmt, __func__, __LINE__, ##_args)
#   define FRCAPI_DEBUG(_fmt, _args...)  printf("[DEBUG] %s.%d:" _fmt, __func__, __LINE__, ##_args)
#else
#   define FRCAPI_ERROR(_fmt, _args...)
#   define FRCAPI_DEBUG(_fmt, _args...)
#endif


int frcapi_ioctl(frc_ioctl_t *arg)
{
    int rv, fd;
    fd = open(FRC_DEVICE_PATH, O_RDWR);
    if(fd < 0)
    {
        FRCAPI_ERROR("Can't open %s.\n", FRC_DEVICE_PATH);
        return FRE_OPEN;
    }

    //FRCTWEAK_DEBUG("%s.%d, type = %d\n", __func__,__LINE__, arg->type);
    if (arg->type == CMD_TYPE_USER)
    {
        FRCAPI_DEBUG("%s.%d\n", __func__,__LINE__);
        rv = ioctl(fd, IOCTL_FRCORE_REQUEST, arg);
    }
    if(arg->type == CMD_TYPE_DRV)
    {
        FRCAPI_DEBUG("%s.%d\n", __func__,__LINE__);
        rv = ioctl(fd, IOCTL_FRCDRV_REQUEST, arg);
    }
    close(fd);
    if (rv != FRE_SUCCESS)
    {
        FRCAPI_DEBUG("Ioctl fail: rv = %d!\n", rv);
        return FRE_IOCTL;
    }

    if (arg->rv != FRE_SUCCESS)
    {
        FRCAPI_DEBUG("Ioctl fail: arg->rv = %d!\n", arg->rv);
        return arg->rv;
    }


    return FRE_SUCCESS;
}


int frcapi_fr_status_get(frc_fr_status_t *status)
{
    int rv;
    frc_ioctl_t arg;

    memset(&arg, 0, sizeof(frc_ioctl_t));

    arg.type = CMD_TYPE_USER;
    arg.cmd = USER_CMD_GET_FR_STATUS;
    arg.input = NULL;
    arg.ilen = 0;
    arg.output = (void *)status;
    arg.olen = sizeof(frc_fr_status_t);

    rv = frcapi_ioctl(&arg);

    return rv;

}


int frcapi_fr_enable(uint8_t enable)
{
    int rv;
    frc_ioctl_t arg;

    memset(&arg, 0, sizeof(frc_ioctl_t));

    arg.type = CMD_TYPE_USER;
    arg.cmd = USER_CMD_FR_ENABLE;
    arg.input = (void *)(&enable);
    arg.ilen = 1;
    arg.output = NULL;
    arg.olen = 0;

    rv = frcapi_ioctl(&arg);

    return rv;

}


int frcapi_fr_age_time_set(uint8_t age)
{
    int rv;
    frc_ioctl_t arg;

    memset(&arg, 0, sizeof(frc_ioctl_t));

    arg.type = CMD_TYPE_USER;
    arg.cmd = USER_CMD_SET_AGE_TIME;
    arg.input = (void *)(&age);
    arg.ilen = 1;
    arg.output = NULL;
    arg.olen = 0;

    rv = frcapi_ioctl(&arg);

    return rv;

}


int frcapi_fr_disorder_depth_set(uint8_t disorder)
{
    int rv;
    frc_ioctl_t arg;

    memset(&arg, 0, sizeof(frc_ioctl_t));

    arg.type = CMD_TYPE_USER;
    arg.cmd = USER_CMD_SET_DISORDER_DEPTH;
    arg.input = (void *)(&disorder);
    arg.ilen = 1;
    arg.output = NULL;
    arg.olen = 0;

    rv = frcapi_ioctl(&arg);

    return rv;

}
#if 0
int frcapi_fr_retrans_time_set(uint8_t retrans)
{
    int rv;
    frc_ioctl_t arg;

    memset(&arg, 0, sizeof(frc_ioctl_t));

    arg.type = CMD_TYPE_USER;
    arg.cmd = USER_CMD_SET_RETRANS_TIME;
    arg.input = (void *)(&retrans);
    arg.ilen = 1;
    arg.output = NULL;
    arg.olen = NULL;

    rv = frcapi_ioctl(&arg);

    return rv;

}
#endif

int frcapi_ssn_get(frc_fr_tuple_t *fr_tuple, frc_fr_session_stat_t *fr_stat)
{
    int rv;
    frc_ioctl_t arg;

    //char buff[sizeof(frc_fr_tuple_t)], *p;

    memset(&arg, 0, sizeof(frc_ioctl_t));
    //p = buff;

    //p = frc_tuple_pack(p, fr_tuple);

    arg.type = CMD_TYPE_USER;
    arg.cmd = USER_CMD_SSN_GET_FIVETUPLE;
    arg.input = (void *)fr_tuple;
    arg.ilen = sizeof(frc_fr_tuple_t);
    arg.output = (void *)fr_stat;
    arg.olen = sizeof(frc_fr_session_stat_t);

    rv = frcapi_ioctl(&arg);

    return rv;

}

int frcapi_ssn_bucket_get(uint32_t hash, frc_fr_hash_session_t *hash_session)
{
    int rv;
    frc_ioctl_t arg;

    memset(&arg, 0, sizeof(frc_ioctl_t));

    arg.type = CMD_TYPE_USER;
    arg.cmd = USER_CMD_SSN_BUCKET_GET;
    arg.input = (void *)(&hash);
    arg.ilen = 4;
    arg.output = (void *)hash_session;
    arg.olen = sizeof(frc_fr_hash_session_t);

    rv = frcapi_ioctl(&arg);

    return rv;

}


int frcapi_ssn_match(frc_ssn_match_t *ssn_match, uint64_t *num)
{
    int rv;
    frc_ioctl_t arg;

    memset(&arg, 0, sizeof(frc_ioctl_t));

    arg.type = CMD_TYPE_USER;
    arg.cmd = USER_CMD_SSN_MATCH;
    arg.input = (void *)ssn_match;
    arg.ilen = sizeof(frc_ssn_match_t);
    arg.output = (void *)num;
    arg.olen = 8;

    rv = frcapi_ioctl(&arg);

    return rv;
}


int frcapi_rule_status_get(uint8_t *enable)
{
    int rv;
    frc_ioctl_t arg;

    memset(&arg, 0, sizeof(frc_ioctl_t));

    arg.type = CMD_TYPE_USER;
    arg.cmd = USER_CMD_GET_RULE_STATUS;
    arg.input = NULL;
    arg.ilen = 0;
    arg.output = (void *)enable;
    arg.olen = 1;

    rv = frcapi_ioctl(&arg);

    return rv;

}


int frcapi_rule_enable(uint8_t enable)
{
    int rv;
    frc_ioctl_t arg;

    memset(&arg, 0, sizeof(frc_ioctl_t));

    arg.type = CMD_TYPE_USER;
    arg.cmd = USER_CMD_RULE_ENABLE;
    arg.input = (void *)(&enable);
    arg.ilen = 1;
    arg.output = NULL;
    arg.olen = 0;

    rv  = frcapi_ioctl(&arg);

    return rv;
}

int frcapi_rule_add(frc_rule_t *rule)
{
    int rv;
    frc_ioctl_t arg;

    memset(&arg, 0, sizeof(frc_ioctl_t));

    arg.type = CMD_TYPE_USER;
    arg.cmd = USER_CMD_ADD_RULE;
    arg.input = (void *)rule;
    arg.ilen = sizeof(frc_rule_t);
    arg.output = NULL;
    arg.olen = 0;

    rv = frcapi_ioctl(&arg);

    return rv;
}

int frcapi_rule_del(frc_rule_op_in_t *rule_del_in, uint16_t *num)
{
    int rv;
    frc_ioctl_t arg;

    memset(&arg, 0, sizeof(frc_ioctl_t));

    arg.type = CMD_TYPE_USER;
    arg.cmd = USER_CMD_DEL_RULE;
    arg.input = (void *)rule_del_in;
    arg.ilen = sizeof(frc_rule_op_in_t);
    arg.output = (void *)num;
    arg.olen = 2;

    rv = frcapi_ioctl(&arg);

    return rv;

}

int frcapi_rule_stat_get(frc_rule_op_in_t *stat_in, frc_rule_stat_out_t *stat_out)
{
    int rv;
    frc_ioctl_t arg;

    memset(&arg, 0, sizeof(frc_ioctl_t));

    arg.type = CMD_TYPE_USER;
    arg.cmd = USER_CMD_GET_RULE;
    arg.input = (void *)stat_in;
    arg.ilen = sizeof(frc_rule_op_in_t);
    arg.output = (void *)stat_out;
    arg.olen = sizeof(frc_rule_stat_out_t);

    rv = frcapi_ioctl(&arg);

    return rv;

}

#if 0
int frcapi_rule_cnt_get(frc_rule_op_in_t *rule_cnt_in, frc_rule_pkt_stat_t *cnt_out)
{
    int rv;
    int i;
    //frc_ioctl_t arg;
    frc_rule_stat_out_t stat_out;
    //memset(&arg, 0, sizeof(frc_ioctl_t));
    memset(&stat_out, 0, sizeof(frc_rule_stat_out_t));

    rv = frcapi_rule_stat_get(rule_cnt_in, &stat_out);

    for (i = 0; i < rule_cnt_in->num; i ++)
    {
        cnt_out->pkts += stat_out.rule_stat[i].stat.pkts;
        cnt_out->bytes += stat_out.rule_stat[i].stat.bytes;
    }
#if 0
    arg.type = CMD_TYPE_USER;
    arg.cmd = USER_CMD_GET_RULE_CNT;
    arg.input = (void *)rule_cnt_in;
    arg.ilen = sizeof(frc_rule_op_in_t);
    arg.output = (void *)cnt_out;
    arg.olen = sizeof(frc_rule_pkt_stat_t);
#endif
    //rv = frcapi_ioctl(&arg);

    return rv;
}
#endif


#define UP16(_buf, _var) \
        _var  = (*(_buf) & 0xff) << 8; \
        _var |= (*(_buf + 1) & 0xff);

int frcapi_rule_stat_clear(frc_rule_op_in_t *clear_stat_in, uint16_t *num)
{
    int rv;
    frc_ioctl_t arg;
    memset(&arg, 0, sizeof(frc_ioctl_t));
    uint16_t clear_num;

    arg.type = CMD_TYPE_USER;
    arg.cmd = USER_CMD_CLEAN_RULE_STAT;
    arg.input = (void *)clear_stat_in;
    arg.ilen = sizeof(frc_rule_op_in_t);
    arg.output = (void *)num;
    arg.olen = 2;

    rv = frcapi_ioctl(&arg);
    UP16((uint8_t *)num, clear_num);
    *num = clear_num;

    return rv;
}

int frcapi_rule_clear(frc_rule_op_in_t *rule_del_in, uint16_t *num)
{
    int rv;
    frc_ioctl_t arg;
    uint16_t clear_num;
    memset(&arg, 0, sizeof(frc_ioctl_t));

    arg.type = CMD_TYPE_USER;
    arg.cmd = USER_CMD_CLEAN_ALL_RULE;
    arg.input = (void *)rule_del_in;
    arg.ilen = sizeof(frc_rule_op_in_t);
    arg.output = (void *)num;
    arg.olen = 2;

    rv = frcapi_ioctl(&arg);
    UP16((uint8_t *)num, clear_num);
    *num = clear_num;
    return rv;
}

int frcapi_rule_update(void)
{
    int rv;
    frc_ioctl_t arg;
    memset(&arg, 0, sizeof(frc_ioctl_t));

    arg.type = CMD_TYPE_USER;
    arg.cmd = USER_CMD_UPDATE_RULE;
    arg.input = NULL;
    arg.ilen = 0;
    arg.output = NULL;
    arg.olen = 0;

    rv = frcapi_ioctl(&arg);
    if (FRE_TIMEOUT == rv)
    {
        sleep(30);
        rv = FRE_SUCCESS;
    }

    return rv;
}

int frcapi_rule_num_get(uint16_t *num)
{
    int rv;
    frc_ioctl_t arg;
    uint16_t rule_num;

    memset(&arg, 0, sizeof(frc_ioctl_t));

    arg.type = CMD_TYPE_USER;
    arg.cmd = USER_CMD_GET_RULE_NUM;
    arg.input = (void *)NULL;
    arg.ilen = 0;
    arg.output = (void *)num;
    arg.olen = 2;

    rv = frcapi_ioctl(&arg);
    UP16((uint8_t *)num, rule_num);
    *num = rule_num;
    return rv;
}



int frcapi_rule_match(frc_rule_stat_t *rule_stat, frc_rule_t *rule, uint16_t rule_num, uint16_t *match_num)
{
    int i;


    for (i = 0; i < rule_num; i ++)
    {

        //if (!strncmp((char *)&stat_out.rule_stat[i].rule, (char *)rule, sizeof(frc_rule_t)))

        if ((rule->sip == rule_stat[i].rule.sip) &&
             (rule->dip == rule_stat[i].rule.dip) &&
              (rule->sp == rule_stat[i].rule.sp) &&
               (rule->dp == rule_stat[i].rule.dp) &&
                (rule->proto == rule_stat[i].rule.proto))
        {

            (*match_num) ++;
            break;

        }

    }

    return FRE_SUCCESS;
}


#if FRC_CONFIG_TWO_TUPLE
int frcapi_acl_status_get(uint8_t *enable)
{
    int rv;
    frc_ioctl_t arg;

    memset(&arg, 0, sizeof(frc_ioctl_t));

    arg.type = CMD_TYPE_USER;
    arg.cmd = USER_CMD_GET_ACL_STATUS;
    arg.input = NULL;
    arg.ilen = 0;
    arg.output = (void *)enable;
    arg.olen = 1;

    rv = frcapi_ioctl(&arg);

    return rv;

}


int frcapi_acl_enable(uint8_t enable)
{
    int rv;
    frc_ioctl_t arg;

    memset(&arg, 0, sizeof(frc_ioctl_t));

    arg.type = CMD_TYPE_USER;
    arg.cmd = USER_CMD_ACL_ENABLE;
    arg.input = (void *)(&enable);
    arg.ilen = 1;
    arg.output = NULL;
    arg.olen = 0;

    rv  = frcapi_ioctl(&arg);

    return rv;
}

int frcapi_acl_add(frc_acl_t *acl)
{
    int rv;
    frc_ioctl_t arg;

    memset(&arg, 0, sizeof(frc_ioctl_t));

    arg.type = CMD_TYPE_USER;
    arg.cmd = USER_CMD_ADD_ACL;
    arg.input = (void *)acl;
    arg.ilen = sizeof(frc_acl_t);
    arg.output = NULL;
    arg.olen = 0;

    rv = frcapi_ioctl(&arg);

    return rv;
}

int frcapi_acl_del(frc_acl_op_in_t *acl_del_in, uint16_t *num)
{
    int rv;
    frc_ioctl_t arg;

    memset(&arg, 0, sizeof(frc_ioctl_t));

    arg.type = CMD_TYPE_USER;
    arg.cmd = USER_CMD_DEL_ACL;
    arg.input = (void *)acl_del_in;
    arg.ilen = sizeof(frc_acl_op_in_t);
    arg.output = (void *)num;
    arg.olen = 2;

    rv = frcapi_ioctl(&arg);

    return rv;

}

int frcapi_acl_stat_get(frc_acl_op_in_t *stat_in, frc_acl_stat_out_t *stat_out)
{
    int rv;
    frc_ioctl_t arg;

    memset(&arg, 0, sizeof(frc_ioctl_t));

    arg.type = CMD_TYPE_USER;
    arg.cmd = USER_CMD_GET_ACL;
    arg.input = (void *)stat_in;
    arg.ilen = sizeof(frc_acl_op_in_t);
    arg.output = (void *)stat_out;
    arg.olen = sizeof(frc_acl_stat_out_t);

    rv = frcapi_ioctl(&arg);

    return rv;

}

int frcapi_acl_hash_table_stat_get(frc_acl_hash_table_op_in_t *stat_in,
                                   frc_acl_hash_table_stat_out_t *stat_out)
{
    int rv;
    frc_ioctl_t arg;

    memset(&arg, 0, sizeof(frc_ioctl_t));

    arg.type = CMD_TYPE_USER;
    arg.cmd = USER_CMD_GET_ACL_HASH_TABLE;
    arg.input = (void *)stat_in;
    arg.ilen = sizeof(frc_acl_hash_table_op_in_t);
    arg.output = (void *)stat_out;
    arg.olen = sizeof(frc_acl_hash_table_stat_out_t);

    rv = frcapi_ioctl(&arg);

    return rv;

}

#define UP16(_buf, _var) \
        _var  = (*(_buf) & 0xff) << 8; \
        _var |= (*(_buf + 1) & 0xff);

int frcapi_acl_stat_clear(frc_acl_op_in_t *clear_stat_in, uint16_t *num)
{
    int rv;
    frc_ioctl_t arg;
    memset(&arg, 0, sizeof(frc_ioctl_t));
    uint16_t clear_num;

    arg.type = CMD_TYPE_USER;
    arg.cmd = USER_CMD_CLEAN_ACL_STAT;
    arg.input = (void *)clear_stat_in;
    arg.ilen = sizeof(frc_acl_op_in_t);
    arg.output = (void *)num;
    arg.olen = 2;

    rv = frcapi_ioctl(&arg);
    UP16((uint8_t *)num, clear_num);
    *num = clear_num;

    return rv;
}

int frcapi_acl_clear(frc_acl_op_in_t *acl_del_in, uint16_t *num)
{
    int rv;
    frc_ioctl_t arg;
    uint16_t clear_num;
    memset(&arg, 0, sizeof(frc_ioctl_t));

    arg.type = CMD_TYPE_USER;
    arg.cmd = USER_CMD_CLEAN_ALL_ACL;
    arg.input = (void *)acl_del_in;
    arg.ilen = sizeof(frc_acl_op_in_t);
    arg.output = (void *)num;
    arg.olen = 2;

    rv = frcapi_ioctl(&arg);
    UP16((uint8_t *)num, clear_num);
    *num = clear_num;
    return rv;
}

int frcapi_acl_num_get(uint16_t *num)
{
    int rv;
    frc_ioctl_t arg;
    uint16_t acl_num;

    memset(&arg, 0, sizeof(frc_ioctl_t));

    arg.type = CMD_TYPE_USER;
    arg.cmd = USER_CMD_GET_ACL_NUM;
    arg.input = (void *)NULL;
    arg.ilen = 0;
    arg.output = (void *)num;
    arg.olen = 2;

    rv = frcapi_ioctl(&arg);
    UP16((uint8_t *)num, acl_num);
    *num = acl_num;
    return rv;
}



int frcapi_acl_match(frc_acl_stat_t *acl_stat, frc_acl_t *acl, uint16_t acl_num, uint16_t *match_num)
{
    int i;

    for (i = 0; i < acl_num; i ++)
    {
        if ((acl->one_tuple == acl_stat[i].acl.one_tuple) &&
                (acl->proto == acl_stat[i].acl.proto))
        {
            (*match_num) ++;
            break;
        }
    }

    return FRE_SUCCESS;
}

#endif

int frcapi_bdd_info_get(frc_bdd_info_out_t *bdd_info)
{
    int rv;
    frc_ioctl_t arg;

    memset(&arg, 0, sizeof(frc_ioctl_t));

    arg.type = CMD_TYPE_USER;
    arg.cmd = USER_CMD_GET_BDD_INFO;
    arg.input = NULL;
    arg.ilen = 0;
    arg.output = (void *)bdd_info;
    arg.olen = sizeof(frc_bdd_info_out_t);

    rv = frcapi_ioctl(&arg);

    return rv;
}

int frcapi_frcdrv_version_get(frc_version_t *frcdrv)
{
    int rv;
    frc_ioctl_t arg;

    memset(&arg, 0, sizeof(frc_ioctl_t));

    arg.type = CMD_TYPE_DRV;
    arg.cmd = DRV_CMD_GET_VERSION;
    arg.input = NULL;
    arg.ilen = 0;
    arg.output = (void *)frcdrv;
    arg.olen = sizeof(frc_version_t);

    rv = frcapi_ioctl(&arg);

    return rv;

}


int frcapi_bdd_status_get(frc_bdd_status_out_t *bdd_status)
{
    int rv;
    frc_ioctl_t arg;

    memset(&arg, 0, sizeof(frc_ioctl_t));

    arg.type = CMD_TYPE_USER;
    arg.cmd = USER_CMD_GET_BDD_STATUS;
    arg.input = NULL;
    arg.ilen = 0;
    arg.output = (void *)bdd_status;
    arg.olen = sizeof(frc_bdd_status_out_t);

    rv = frcapi_ioctl(&arg);

    return rv;
}

int frcapi_bdd_poweroff(void)
{
    int rv;
    frc_ioctl_t arg;

    memset(&arg, 0, sizeof(frc_ioctl_t));

    arg.type = CMD_TYPE_USER;
    arg.cmd = USER_CMD_POWEROFF;
    arg.input = NULL;
    arg.ilen = 0;
    arg.output = NULL;
    arg.olen = 0;

    rv = frcapi_ioctl(&arg);

    return rv;
}

int frcapi_bdd_port_enable(frc_bdd_set_port_in_t *set_port)
{
    int rv;
    frc_ioctl_t arg;

    memset(&arg, 0, sizeof(frc_ioctl_t));

    arg.type = CMD_TYPE_USER;
    arg.cmd = USER_CMD_BDD_PORT_ENABLE;
    arg.input = (void *)set_port;
    arg.ilen = sizeof(frc_bdd_set_port_in_t);
    arg.output = NULL;
    arg.olen = 0;

    rv = frcapi_ioctl(&arg);

    return rv;
}

int frcapi_bdd_port_loopback_set(frc_bdd_set_loopback_in_t *mode)
{
    int rv;
    frc_ioctl_t arg;

    memset(&arg, 0 ,sizeof(frc_ioctl_t));

    arg.type = CMD_TYPE_USER;
    arg.cmd = USER_CMD_SET_LOOPBACK_MODE;
    arg.input = (void *)mode;
    arg.ilen = sizeof(frc_bdd_set_loopback_in_t);
    arg.output = NULL;
    arg.olen = 0;

    rv = frcapi_ioctl(&arg);

    return rv;
}

int frcapi_bdd_log_config_set(frc_bdd_log_in_t *log_in)
{
    int rv;
    frc_ioctl_t arg;

    memset(&arg, 0, sizeof(frc_ioctl_t));

    arg.type = CMD_TYPE_USER;
    arg.cmd = USER_CMD_CONFIG_LOG_SW;
    arg.input = (void *)log_in;
    arg.ilen = sizeof(frc_bdd_log_in_t);
    arg.output = NULL;
    arg.olen = 0;

    rv = frcapi_ioctl(&arg);

    return rv;
}

int frcapi_bdd_phy(frc_bdd_phy_t *input, frc_phy_op_t *output)
{
    int rv;
    frc_ioctl_t arg;
    memset(&arg, 0 ,sizeof(frc_ioctl_t));

    arg.type = CMD_TYPE_USER;
    arg.ilen = 8;
    arg.input = &(input->phy);
    if (!input->op)
    {
        arg.cmd = USER_CMD_GET_PHY;
        arg.output = output;
        arg.olen = 8;
    }
    else
    {
        arg.cmd = USER_CMD_SET_PHY;
        arg.output = NULL;
        arg.olen = 0;
    }

    rv = frcapi_ioctl(&arg);

    return rv;
}


int frcapi_bdd_cpld(frc_bdd_cpld_t *input, frc_cpld_op_t *output)
{
    int rv;
    frc_ioctl_t arg;

    memset(&arg, 0, sizeof(frc_ioctl_t));

    arg.type = CMD_TYPE_USER;
    arg.input = &(input->cpld);
    arg.ilen = sizeof(frc_cpld_op_t);
    if (!input->op)
    {
        arg.cmd = USER_CMD_GET_CPLD;
        arg.output = output;
        arg.olen = sizeof(frc_cpld_op_t);
    }
    else
    {
        arg.cmd = USER_CMD_SET_CPLD;
        arg.output = NULL;
        arg.olen = 0;
    }

    rv = frcapi_ioctl(&arg);

    return rv;
}

int frcapi_bdd_workmode_set(frc_bdd_workmode_set_in_t *workmode)
{
    int rv;
    frc_ioctl_t arg;

    memset(&arg, 0, sizeof(frc_ioctl_t));

    arg.type = CMD_TYPE_USER;
    arg.cmd = USER_CMD_SET_WORK_MODE;
    arg.input = (void *)workmode;
    arg.ilen = sizeof(frc_bdd_workmode_set_in_t);
    arg.output = NULL;
    arg.olen = 0;

    rv = frcapi_ioctl(&arg);

    return rv;
}


int frcapi_bdd_force_link(frc_bdd_force_link_in_t *force_link)
{
    int rv;
    frc_ioctl_t arg;

    memset(&arg, 0, sizeof(frc_ioctl_t));

    arg.type = CMD_TYPE_USER;
    arg.cmd = USER_CMD_FORCE_LINK;
    arg.input = (void *)force_link;
    arg.ilen = sizeof(frc_bdd_force_link_in_t);
    arg.output = NULL;
    arg.olen = 0;

    rv = frcapi_ioctl(&arg);

    return rv;
}


int frcapi_dma_block_set(uint8_t size)
{
    int rv;
    frc_ioctl_t arg;

    memset(&arg, 0, sizeof(frc_ioctl_t));

    arg.type = CMD_TYPE_USER;
    arg.cmd = USER_CMD_SET_DMA_BLOCK_SIZE;
    arg.input = (void *)&size;
    arg.ilen = 1;
    arg.output = NULL;
    arg.olen = 0;

    rv = frcapi_ioctl(&arg);

    return rv;
}

int frcapi_dma_mem_read(frc_bdd_dma_read_in_t *dma_in, uint8_t *data)
{
    int rv;
    frc_ioctl_t arg;

    memset(&arg, 0, sizeof(frc_ioctl_t));

    arg.type = CMD_TYPE_USER;
    arg.cmd = USER_CMD_READ_DMA;
    arg.input = (void *)dma_in;
    arg.ilen = sizeof(frc_bdd_dma_read_in_t);
    arg.output = (void *)data;
    arg.olen = dma_in->size;

    rv = frcapi_ioctl(&arg);

    return rv;
}

int frcapi_stat_clear(void)
{
    int rv;
    frc_ioctl_t arg;

    memset(&arg, 0, sizeof(frc_ioctl_t));

    arg.type = CMD_TYPE_USER;
    arg.cmd = USER_CMD_CLEAR_STAT;
    arg.input = NULL;
    arg.ilen = 0;
    arg.output = NULL;
    arg.olen = 0;

    rv = frcapi_ioctl(&arg);

    return rv;
}

int frcapi_pkt_stat_get(frc_stat_op_in_t *input, uint64_t *stat)
{
    int rv;
    frc_ioctl_t arg;

    memset(&arg, 0, sizeof(frc_ioctl_t));

    arg.type = CMD_TYPE_USER;
    arg.cmd = USER_CMD_GET_STAT;
    arg.input = (void *)input;
    arg.ilen = sizeof(frc_stat_op_in_t);
    arg.output = (void *)stat;
    arg.olen = sizeof(uint64_t) * input->num;

    rv = frcapi_ioctl(&arg);

    return rv;
}


int frcapi_chan_or_pr_ssn_start(uint64_t type, frc_dma_ssn_chan_desc_t *desc)
{
    int rv;
    frc_ioctl_t arg;

    memset(&arg, 0, sizeof(frc_ioctl_t));

    arg.type = CMD_TYPE_DRV;
    arg.cmd = DRV_CMD_GET_POOL_AND_RING_ADDR;
    arg.input = (void *)(&type);
    arg.ilen = 8;
    arg.output = (void *)desc;
    arg.olen = sizeof(frc_dma_ssn_chan_desc_t);

    rv = frcapi_ioctl(&arg);

    return rv;
}

int frcapi_chan_or_pr_rule_or_udp_start(uint64_t type, frc_dma_chan_desc_t *desc)
{
    int rv;
    frc_ioctl_t arg;

    memset(&arg, 0, sizeof(frc_ioctl_t));

    arg.type = CMD_TYPE_DRV;
    arg.cmd = DRV_CMD_GET_POOL_AND_RING_ADDR;
    arg.input = (void *)(&type);
    arg.ilen = 8;
    arg.output = (void *)desc;
    arg.olen = sizeof(frc_dma_chan_desc_t);

    rv = frcapi_ioctl(&arg);

    return rv;
}

int frcapi_chan_test_start(uint64_t *chan, uint16_t *olen)
{
    void *output = NULL;
    frc_ioctl_t arg;
    int fd, rv;

    fd = open(FRC_DEVICE_PATH, O_RDWR);
    if(fd < 0) {
        FRCAPI_ERROR("Can't open %s.\n", FRC_DEVICE_PATH);
        return FRE_OPEN;
    }

    memset(&arg, 0, sizeof(frc_ioctl_t));

    arg.type   = CMD_TYPE_TEST;
    arg.cmd    = TEST_CMD_SIMPLE_PACKET_CHAN;
    arg.ilen   = 8;
    arg.input  = chan;
    if (olen != NULL && output != NULL)
    {
        arg.olen   = *olen;
        arg.output = output;
    }

    switch (arg.type)
    {
    case CMD_TYPE_DRV:
        rv = ioctl(fd, IOCTL_FRCDRV_REQUEST, &arg);
        break;
    case CMD_TYPE_TEST:
    case CMD_TYPE_CTRL:
    case CMD_TYPE_USER:
        rv = ioctl(fd, IOCTL_FRCORE_REQUEST, &arg);
        break;
    default:
        rv = FRE_UNSUPPORT;
        break;
    }

    close(fd);

    if (rv != FRE_SUCCESS) {
        FRCAPI_ERROR("Ioctl fail: rv = %d!\n", rv);
        return FRE_IOCTL;
    }

    if (arg.rv != FRE_SUCCESS)
    {
        return arg.rv;
    }

    if (olen != NULL)
    {
        *olen = arg.olen;
    }

    return FRE_SUCCESS;
}

int frcapi_sysinfo_get(frc_system_info_t *output)
{
    int rv;
    frc_ioctl_t arg;

    memset(&arg, 0, sizeof(frc_ioctl_t));

    arg.type = CMD_TYPE_USER;
    arg.cmd = USER_CMD_GET_SYSINFO;
    arg.input = NULL;
    arg.ilen = 0;
    arg.output = (void *)output;
    arg.olen = sizeof(frc_system_info_t);

    rv = frcapi_ioctl(&arg);

    return rv;
}

#if FRC_CONFIG_UDP
int frcapi_udp_status_get(uint8_t *enable)
{
    int rv;
    frc_ioctl_t arg;

    memset(&arg, 0, sizeof(frc_ioctl_t));

    arg.type = CMD_TYPE_USER;
    arg.cmd = USER_CMD_GET_UDP_STATUS;
    arg.input = NULL;
    arg.ilen = 0;
    arg.output = (void *)enable;
    arg.olen = 1;

    rv = frcapi_ioctl(&arg);

    return rv;

}


int frcapi_udp_enable(uint8_t enable)
{
    int rv;
    frc_ioctl_t arg;

    memset(&arg, 0, sizeof(frc_ioctl_t));

    arg.type = CMD_TYPE_USER;
    arg.cmd = USER_CMD_UDP_ENABLE;
    arg.input = (void *)(&enable);
    arg.ilen = 1;
    arg.output = NULL;
    arg.olen = 0;

    rv  = frcapi_ioctl(&arg);

    return rv;
}

#endif

#if FRC_CONFIG_VLAN_CHECK
int frcapi_vlan_check_status_get(frc_vlan_check_para_t *vlan_check_para)
{
    int rv;
    frc_ioctl_t arg;

    memset(&arg, 0, sizeof(frc_ioctl_t));

    arg.type = CMD_TYPE_USER;
    arg.cmd = USER_CMD_GET_VLAN_CHECK_PARAMETER;
    arg.input = NULL;
    arg.ilen = 0;
    arg.output = (void *)vlan_check_para;
    arg.olen = sizeof(frc_vlan_check_para_t);

    rv = frcapi_ioctl(&arg);

    return rv;

}


int frcapi_vlan_check_set_parameter(frc_vlan_check_para_t *vlan_check_para)
{
    int rv;
    frc_ioctl_t arg;

    memset(&arg, 0, sizeof(frc_ioctl_t));

    arg.type = CMD_TYPE_USER;
    arg.cmd = USER_CMD_SET_VLAN_CHECK_PARAMETER;
    arg.input = (void *)(vlan_check_para);
    arg.ilen = sizeof(frc_vlan_check_para_t);
    arg.output = NULL;
    arg.olen = 0;

    rv  = frcapi_ioctl(&arg);

    return rv;
}

int frcapi_vlan_check_stat_clear(void)
{
    int rv;
    frc_ioctl_t arg;

    memset(&arg, 0, sizeof(frc_ioctl_t));

    arg.type = CMD_TYPE_USER;
    arg.cmd = USER_CMD_CLEAR_VLAN_CHECK_STAT;
    arg.input = NULL;
    arg.ilen = 0;
    arg.output = NULL;
    arg.olen = 0;

    rv = frcapi_ioctl(&arg);

    return rv;
}

int frcapi_vlan_check_stat_get(frc_vlan_op_in_t *input, uint64_t *stat)
{
    int rv;
    frc_ioctl_t arg;

    memset(&arg, 0, sizeof(frc_ioctl_t));

    arg.type = CMD_TYPE_USER;
    arg.cmd = USER_CMD_GET_VLAN_CHECK_STAT;
    arg.input = (void *)input;
    arg.ilen = sizeof(frc_vlan_op_in_t);
    arg.output = (void *)stat;
    arg.olen = sizeof(uint64_t) * input->num;

    rv = frcapi_ioctl(&arg);

    return rv;
}
#endif

#if FRC_CONFIG_MAC_STATISTICS
int frcapi_mac_stat_set(frc_mac_stat_in_t *input, frcore_user_cmd_e cmd)
{
    int rv;
    frc_ioctl_t arg;

    memset(&arg, 0, sizeof(frc_ioctl_t));
    arg.type = CMD_TYPE_USER;
    arg.cmd = cmd;
    arg.input = (void *)input;
    arg.ilen = sizeof(frc_mac_stat_in_t);
    arg.output = NULL;
    arg.olen = 0;

    rv = frcapi_ioctl(&arg);

    return rv;
}

int frcapi_mac_stat_get(frc_mac_stat_in_t *input, frc_mac_stat_out_t *stat, frcore_user_cmd_e cmd)
{
    int rv;
    frc_ioctl_t arg;

    memset(&arg, 0, sizeof(frc_ioctl_t));
    arg.type = CMD_TYPE_USER;
    arg.cmd = cmd;
    arg.input = (void *)input;
    arg.ilen = sizeof(frc_mac_stat_in_t);
    arg.output = (void *) stat;
    arg.olen = sizeof(frc_mac_stat_out_t) ;

    rv = frcapi_ioctl(&arg);

    return rv;
}
int frcapi_mac_stat_get_all(frc_mac_stat_in_t *input, frc_mac_stat_out_t *stat, uint16_t num, frcore_user_cmd_e cmd)
{
    int rv;
    frc_ioctl_t arg;

    memset(&arg, 0, sizeof(frc_ioctl_t));
    arg.type = CMD_TYPE_USER;
    arg.cmd = cmd;
    arg.input = (void *)input;
    arg.ilen = sizeof(frc_mac_stat_in_t);
    arg.output = (void *) stat;
    arg.olen = sizeof(frc_mac_stat_out_t);

    rv = frcapi_ioctl(&arg);

    return rv;
}

#endif

#if FRC_CONFIG_TIMESTAMP_CHECK
int frcapi_timestamp_check_stat_clear(void)
{
    int rv;
    frc_ioctl_t arg;

    memset(&arg, 0, sizeof(frc_ioctl_t));

    arg.type = CMD_TYPE_USER;
    arg.cmd = USER_CMD_CLEAR_TIMESTAMP_CHECK_STAT;
    arg.input = NULL;
    arg.ilen = 0;
    arg.output = NULL;
    arg.olen = 0;

    rv = frcapi_ioctl(&arg);

    return rv;
}

int frcapi_timestamp_check_stat_get(frc_timestamp_op_in_t *input, uint64_t *stat)
{
    int rv;
    frc_ioctl_t arg;

    memset(&arg, 0, sizeof(frc_ioctl_t));

    arg.type = CMD_TYPE_USER;
    arg.cmd = USER_CMD_GET_TIMESTAMP_CHECK_STAT;
    arg.input = (void *)input;
    arg.ilen = sizeof(frc_timestamp_op_in_t);
    arg.output = (void *)stat;
    arg.olen = sizeof(uint64_t) * input->num;

    rv = frcapi_ioctl(&arg);

    return rv;
}
#endif

#if FRC_CONFIG_IPC
int frcapi_ipc_misc_get(ipc_misc_t *misc)
{
    int rv;
    frc_ioctl_t arg;

    memset(&arg, 0, sizeof(frc_ioctl_t));
    arg.type = CMD_TYPE_USER;
    arg.cmd = USER_CMD_IPC_MISC_GET;
    arg.input = NULL;
    arg.ilen = 0;
    arg.output = (void *) misc;
    arg.olen = sizeof(ipc_misc_t);

    rv = frcapi_ioctl(&arg);

    return rv;
}


int frcapi_ipc_misc_set(ipc_misc_t *misc)
{
    int rv;
    frc_ioctl_t arg;

    memset(&arg, 0, sizeof(frc_ioctl_t));
    arg.type = CMD_TYPE_USER;
    arg.cmd = USER_CMD_IPC_MISC_SET;
    arg.input = (void *) misc;
    arg.ilen = sizeof(ipc_misc_t);
    arg.output = NULL;
    arg.olen = 0;

    rv = frcapi_ioctl(&arg);

    return rv;
}

int frcapi_ipc_cur_get(ipc_cur_t *cur)
{
    int rv;
    frc_ioctl_t arg;

    memset(&arg, 0, sizeof(frc_ioctl_t));
    arg.type = CMD_TYPE_USER;
    arg.cmd = USER_CMD_IPC_CUR_GET;
    arg.input = NULL;
    arg.ilen = 0;
    arg.output = (void *) cur;
    arg.olen = sizeof(ipc_cur_t);

    rv = frcapi_ioctl(&arg);

    return rv;
}


int frcapi_ipc_cur_set(ipc_cur_t *cur)
{
    int rv;
    frc_ioctl_t arg;

    memset(&arg, 0, sizeof(frc_ioctl_t));
    arg.type = CMD_TYPE_USER;
    arg.cmd = USER_CMD_IPC_CUR_SET;
    arg.input = (void *) cur;
    arg.ilen = sizeof(ipc_cur_t);
    arg.output = NULL;
    arg.olen = 0;

    rv = frcapi_ioctl(&arg);

    return rv;
}

int frcapi_ipc_exp_get(ipc_exp_t *exp)
{
    int rv;
    frc_ioctl_t arg;

    memset(&arg, 0, sizeof(frc_ioctl_t));
    arg.type = CMD_TYPE_USER;
    arg.cmd = USER_CMD_IPC_EXP_GET;
    arg.input = NULL;
    arg.ilen = 0;
    arg.output = (void *) exp;
    arg.olen = sizeof(ipc_exp_t);

    rv = frcapi_ioctl(&arg);

    return rv;
}


int frcapi_ipc_exp_set(ipc_exp_t *exp)
{
    int rv;
    frc_ioctl_t arg;

    memset(&arg, 0, sizeof(frc_ioctl_t));
    arg.type = CMD_TYPE_USER;
    arg.cmd = USER_CMD_IPC_EXP_SET;
    arg.input = (void *) exp;
    arg.ilen = sizeof(ipc_exp_t);
    arg.output = NULL;
    arg.olen = 0;

    rv = frcapi_ioctl(&arg);

    return rv;
}


int frcapi_ipc_instr_get(ipc_instr_cfg_t *instr)
{
    int rv;
    frc_ioctl_t arg;

    memset(&arg, 0, sizeof(frc_ioctl_t));
    arg.type = CMD_TYPE_USER;
    arg.cmd = USER_CMD_IPC_INSTR_GET;
    arg.input = NULL;
    arg.ilen = 0;
    arg.output = (void *) instr;
    arg.olen = sizeof(ipc_instr_cfg_t);

    rv = frcapi_ioctl(&arg);

    return rv;
}


int frcapi_ipc_instr_set(ipc_instr_cfg_t *instr)
{
    int rv;
    frc_ioctl_t arg;

    memset(&arg, 0, sizeof(frc_ioctl_t));
    arg.type = CMD_TYPE_USER;
    arg.cmd = USER_CMD_IPC_INSTR_SET;
    arg.input = (void *) instr;
    arg.ilen = sizeof(ipc_instr_cfg_t);
    arg.output = NULL;
    arg.olen = 0;

    rv = frcapi_ioctl(&arg);

    return rv;
}


int frcapi_ipc_instr_payload_set(ipc_payload_set_in_t *payload_set)
{
    int rv;
    frc_ioctl_t arg;

    memset(&arg, 0, sizeof(frc_ioctl_t));
    arg.type = CMD_TYPE_USER;
    arg.cmd = USER_CMD_IPC_INSTR_PAYLOAD_SET;
    arg.input = (void *) payload_set;
    arg.ilen = sizeof(ipc_payload_set_in_t);
    arg.output = NULL;
    arg.olen = 0;

    rv = frcapi_ioctl(&arg);

    return rv;
}


#if FRC_CONFIG_VLAN_IV
int frctweak_ipc_hash4_mask_set(frc_vlan_hash_mask_t *mask_set)
{
    int rv;
    frc_ioctl_t arg;

    memset(&arg, 0, sizeof(frc_ioctl_t));
    arg.type = CMD_TYPE_USER;
    arg.cmd = USER_CMD_VLAN_V4_HASH_MASK_SET;
    arg.input = (void *) mask_set;
    arg.ilen = sizeof(frc_vlan_hash_mask_t);
    arg.output = NULL;
    arg.olen = 0;

    rv = frcapi_ioctl(&arg);

    return rv;
}


int frctweak_ipc_hash6_mask_set(frc_vlan_hash_mask_t *mask_set)
{
    int rv;
    frc_ioctl_t arg;

    memset(&arg, 0, sizeof(frc_ioctl_t));
    arg.type = CMD_TYPE_USER;
    arg.cmd = USER_CMD_VLAN_V6_HASH_MASK_SET;
    arg.input = (void *) mask_set;
    arg.ilen = sizeof(frc_vlan_hash_mask_t);
    arg.output = NULL;
    arg.olen = 0;

    rv = frcapi_ioctl(&arg);

    return rv;
}
#endif

#endif

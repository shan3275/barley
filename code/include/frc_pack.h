#ifndef __FRC_PACK_H__
#define __FRC_PACK_H__


#include "frc.h"
#include "frc_util.h"
//#include "frc_api.h"

#define PACK_U8(_buf, _var) \
                *_buf++ = (_var) & 0xff
#define UNPACK_U8(_buf, _var) \
                _var = *_buf++ & 0xff

#define PACK_U16(_buf, _var) \
                *_buf++ = ((_var) >> 8) & 0xff; \
                *_buf++ = (_var) & 0xff;
#define UNPACK_U16(_buf, _var) \
                _var  = (*_buf++ & 0xff) << 8; \
                _var |= (*_buf++ & 0xff);

#define PACK_U32(_buf, _var) \
                *_buf++ = ((_var) >> 24) & 0xff; \
                *_buf++ = ((_var) >> 16) & 0xff; \
                *_buf++ = ((_var) >> 8) & 0xff; \
                *_buf++ = (_var) & 0xff;

#define UNPACK_U32(_buf, _var) \
        _var = 0; \
                _var |= (*_buf++ & 0xff) << 24; \
                _var |= (*_buf++ & 0xff) << 16; \
                _var |= (*_buf++ & 0xff) << 8; \
                _var |= (*_buf++ & 0xff);

#define PACK_BYTES(_buf, _var, _len) \
{ \
    int b; \
    for (b = 0; b < _len; b++) { \
        PACK_U8(_buf, (_var)[b]); \
    } \
}

#define UNPACK_BYTES(_buf, _var, _len) \
{ \
    int b; \
    for (b = 0; b < _len; b++) { \
        UNPACK_U8(_buf, (_var)[b]); \
    } \
}

#define PACK_MAC(_buf, _mac) \
    *_buf++ = ((uint8_t *) &(_mac))[0] & 0xff; \
    *_buf++ = ((uint8_t *) &(_mac))[1] & 0xff; \
    *_buf++ = ((uint8_t *) &(_mac))[2] & 0xff; \
    *_buf++ = ((uint8_t *) &(_mac))[3] & 0xff; \
    *_buf++ = ((uint8_t *) &(_mac))[4] & 0xff; \
    *_buf++ = ((uint8_t *) &(_mac))[5] & 0xff;

#define UNPACK_MAC(_buf, _mac) \
    ((uint8_t *) &(_mac))[0] = *_buf++ & 0xff; \
    ((uint8_t *) &(_mac))[1] = *_buf++ & 0xff; \
    ((uint8_t *) &(_mac))[2] = *_buf++ & 0xff; \
    ((uint8_t *) &(_mac))[3] = *_buf++ & 0xff; \
    ((uint8_t *) &(_mac))[4] = *_buf++ & 0xff; \
    ((uint8_t *) &(_mac))[5] = *_buf++ & 0xff;

static inline char *
u8_pack(char *buf, uint8_t *var)
{
    PACK_U8(buf, *var);

    return buf;
}

static inline char *
u8_unpack(char *buf, uint8_t *var)
{
    UNPACK_U8(buf, *var);

    return buf;
}

static inline char *
u16_pack(char *buf, uint16_t *var)
{
    PACK_U16(buf, *var);

    return buf;
}

static inline char *
u16_unpack(char *buf, uint16_t *var)
{
    UNPACK_U16(buf, *var);

    return buf;
}

static inline char *
u32_pack(char *buf, uint32_t *var)
{
    PACK_U32(buf, *var);

    return buf;
}

static inline char *
u32_unpack(char *buf, uint32_t *var)
{
    UNPACK_U32(buf, *var);

    return buf;
}

static inline char *
u64_pack(char *buf, uint64_t *var)
{
    uint32_t _u32;

    _u32 = FRC_64_HI(*var);
    PACK_U32(buf, _u32);
    _u32 = FRC_64_LO(*var);
    PACK_U32(buf, _u32);

        return buf;
}

static inline char *
u64_unpack(char *buf, uint64_t *var)
{
    uint32_t _u32h, _u32l;

    UNPACK_U32(buf, _u32h);
    UNPACK_U32(buf, _u32l);
    FRC_64_SET(*var, _u32h, _u32l);

        return buf;
}


static inline char*
frc_version_pack(char *buf, frc_version_t *var)
{

    PACK_U8(buf, var->major);
    PACK_U8(buf, var->minor);
    PACK_U16(buf, var->build);

    return buf;
}

static inline char*
frc_version_unpack(char *buf, frc_version_t *var)
{
    UNPACK_U8(buf, var->major);
    UNPACK_U8(buf, var->minor);
    UNPACK_U16(buf, var->build);

    return buf;
}


static inline char *
frc_bdd_status_unpack(char *buf, frc_bdd_status_out_t *var)
{
    buf = u64_unpack(buf, &var->temp.local);
    buf = u64_unpack(buf, &var->temp.remote);
    UNPACK_U16(buf, var->dma_block_size);
    UNPACK_U16(buf, var->running_status);
    UNPACK_U32(buf, var->rule_max);
    UNPACK_U32(buf, var->ssn_max);
    UNPACK_U8(buf, var->oct0.link);
    UNPACK_U8(buf, var->oct0.enable);
    UNPACK_U8(buf, var->oct0.work_mode);
    UNPACK_U8(buf, var->oct0.loopback_mode);
    UNPACK_U16(buf, var->oct0.optical_status);
    UNPACK_U16(buf, var->oct0.optical);
    UNPACK_U32(buf, var->oct0.rx_pkts_rate);
    UNPACK_U32(buf, var->oct0.rx_bytes_rate);
    UNPACK_U32(buf, var->oct0.tx_pkts_rate);
    UNPACK_U32(buf, var->oct0.tx_bytes_rate);
    UNPACK_U8(buf, var->oct1.link);
    UNPACK_U8(buf, var->oct1.enable);
    UNPACK_U8(buf, var->oct1.work_mode);
    UNPACK_U8(buf, var->oct1.loopback_mode);
    UNPACK_U16(buf, var->oct1.optical_status);
    UNPACK_U16(buf, var->oct1.optical);
    UNPACK_U32(buf, var->oct1.rx_pkts_rate);
    UNPACK_U32(buf, var->oct1.rx_bytes_rate);
    UNPACK_U32(buf, var->oct1.tx_pkts_rate);
    UNPACK_U32(buf, var->oct1.tx_bytes_rate);

    return buf;
}


static inline char *
frc_bdd_info_unpack(char *buf, frc_bdd_info_out_t *var)
{
    buf = frc_version_unpack(buf, &var->frcore_version);
    buf = frc_version_unpack(buf, &var->frcdrv_version);
    buf = frc_version_unpack(buf, &var->frctweak_version);
    buf = frc_version_unpack(buf, &var->octdrv_version);
    buf = frc_version_unpack(buf, &var->octnic_version);
    UNPACK_U32(buf, var->cpld_version);
    UNPACK_U32(buf, var->port_number);

    return buf;
}

static inline char *
frc_tuple_pack(char *buf, frc_fr_tuple_t *var)
{
    PACK_U32(buf, var->sip);
    PACK_U32(buf, var->dip);
    PACK_U16(buf, var->sp);
    PACK_U16(buf, var->dp);
    PACK_U16(buf, var->proto);
    PACK_U16(buf, var->reserve);

    return buf;
}


static inline char *
frc_tuple_unpack(char *buf, frc_fr_tuple_t *var)
{
#if __FRC_BYTE_ORDER == __FRC_LITTLE_ENDIAN
    UNPACK_U32(buf, var->dip);
    UNPACK_U32(buf, var->sip);
    UNPACK_U16(buf, var->reserve);
    UNPACK_U16(buf, var->proto);
    UNPACK_U16(buf, var->dp);
    UNPACK_U16(buf, var->sp);
#else
    UNPACK_U32(buf, var->sip);
    UNPACK_U32(buf, var->dip);
    UNPACK_U16(buf, var->sp);
    UNPACK_U16(buf, var->dp);
    UNPACK_U16(buf, var->proto);
    UNPACK_U16(buf, var->reserve);
#endif

    return buf;
}


static inline char *
frc_fr_session_t_unpack(char *buf, frc_fr_session_t *var)
{
    buf = frc_tuple_unpack(buf, &var->five_tuple);
#if __FRC_BYTE_ORDER == __FRC_LITTLE_ENDIAN
    UNPACK_U32(buf, var->session_stat.bytes);
    UNPACK_U32(buf, var->session_stat.pkts);
#else
    UNPACK_U32(buf, var->session_stat.pkts);
    UNPACK_U32(buf, var->session_stat.bytes);
#endif
    return buf;

}

static inline char *
frc_rule_unpack(char *buf, frc_rule_t *var)
{
#if __FRC_BYTE_ORDER == __FRC_LITTLE_ENDIAN
    //UNPACK_U32(buf, var->rule_type);
    //UNPACK_U32(buf, var->rule_source);
    //UNPACK_U32(buf, var->op);
    //UNPACK_U32(buf, var->action);
    UNPACK_U16(buf, var->op);
    UNPACK_U16(buf, var->action);
    UNPACK_U16(buf, var->rule_type);
    UNPACK_U16(buf, var->rule_source);
    UNPACK_U32(buf, var->dip);
    //printf("var->dip = 0x%x\n", var->dip);
    UNPACK_U32(buf, var->sip);
    //printf("var->sip = 0x%x\n", var->sip);
    UNPACK_U16(buf, var->index);
    UNPACK_U16(buf, var->proto);
    //printf("var->proto = 0x%x\n", var->proto);
    UNPACK_U16(buf, var->dp);
    //printf("var->dp = 0x%x\n", var->dp);
    UNPACK_U16(buf, var->sp);
    //printf("var->sp = 0x%x\n", var->sp);
#else
    //UNPACK_U32(buf, var->rule_source);
    //UNPACK_U32(buf, var->rule_type);
    //UNPACK_U32(buf, var->action);
    //UNPACK_U32(buf, var->op);
    UNPACK_U16(buf, var->rule_source);
    UNPACK_U16(buf, var->rule_type);
    UNPACK_U16(buf, var->action);
    UNPACK_U16(buf, var->op);
    UNPACK_U32(buf, var->sip);
    printf("var->sip = 0x%x\n", var->sip);
    UNPACK_U32(buf, var->dip);
    printf("var->dip = 0x%x\n", var->dip);
    UNPACK_U16(buf, var->sp);
    printf("var->sp = 0x%x\n", var->sp);
    UNPACK_U16(buf, var->dp);
    printf("var->dp = 0x%x\n", var->dp);
    UNPACK_U16(buf, var->proto);
     printf("var->proto = 0x%x\n", var->proto);
    UNPACK_U16(buf, var->index);
#endif
    return buf;

}

static inline char *
frc_rule_stat_unpack(char *buf, frc_rule_stat_t *var)
{
    buf = frc_rule_unpack(buf, &var->rule);
#if __FRC_BYTE_ORDER == __FRC_LITTLE_ENDIAN
    UNPACK_U32(buf, var->stat.bytes);
    UNPACK_U32(buf, var->stat.pkts);
#else
    UNPACK_U32(buf, var->stat.pkts);
    UNPACK_U32(buf, var->stat.bytes);
#endif
    return buf;
}

#if FRC_CONFIG_TWO_TUPLE
static inline char *
frc_acl_unpack(char *buf, frc_acl_t *var)
{
#if __FRC_BYTE_ORDER == __FRC_LITTLE_ENDIAN
    //UNPACK_U32(buf, var->acl_type);
    //UNPACK_U32(buf, var->acl_source);
    //UNPACK_U32(buf, var->op);
    //UNPACK_U32(buf, var->action);
    UNPACK_U16(buf, var->op);
    UNPACK_U16(buf, var->action);
    UNPACK_U16(buf, var->acl_type);
    UNPACK_U16(buf, var->acl_source);
    //printf("var->dip = 0x%x\n", var->dip);
    //printf("var->sip = 0x%x\n", var->sip);
    UNPACK_U16(buf, var->index);
    UNPACK_U16(buf, var->proto);
    UNPACK_U32(buf, var->one_tuple);
    UNPACK_U32(buf, var->reserved);
    UNPACK_U32(buf, var->hash);
    //printf("var->proto = 0x%x\n", var->proto);
    //printf("var->dp = 0x%x\n", var->dp);
    //printf("var->sp = 0x%x\n", var->sp);
#else
    //UNPACK_U32(buf, var->acl_source);
    //UNPACK_U32(buf, var->acl_type);
    //UNPACK_U32(buf, var->action);
    //UNPACK_U32(buf, var->op);
    UNPACK_U16(buf, var->acl_source);
    UNPACK_U16(buf, var->acl_type);
    UNPACK_U16(buf, var->action);
    UNPACK_U16(buf, var->op);
    UNPACK_U32(buf, var->one_tuple);
    UNPACK_U16(buf, var->proto);
    UNPACK_U16(buf, var->index);
    UNPACK_U32(buf, var->hash);
    UNPACK_U32(buf, var->reserved);
#endif
    return buf;

}

static inline char *
frc_acl_stat_unpack(char *buf, frc_acl_stat_t *var)
{
    buf = frc_acl_unpack(buf, &var->acl);
#if __FRC_BYTE_ORDER == __FRC_LITTLE_ENDIAN
    UNPACK_U32(buf, var->stat.bytes);
    UNPACK_U32(buf, var->stat.pkts);
#else
    UNPACK_U32(buf, var->stat.pkts);
    UNPACK_U32(buf, var->stat.bytes);
#endif
    return buf;
}
#endif /*end of FRC_CONFIG_TWO_TUPLE*/

#endif

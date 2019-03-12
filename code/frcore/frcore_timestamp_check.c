#include "frcore_timestamp_check.h"

#if FRC_CONFIG_TIMESTAMP_CHECK
CVMX_SHARED int64_t timestamp_stat[stat_timestamp_max]= {0x00};
CVMX_SHARED struct timestamp_check timestamp_checks[2][16];
#define FRCORE_TIMESTAMP_CHECK_LOCK(_xeport, _ipport)     cvmx_spinlock_lock(&(timestamp_checks[_xeport][_ipport].lock))
#define FRCORE_TIMESTAMP_CHECK_UNLOCK(_xeport, _ipport)   cvmx_spinlock_unlock(&(timestamp_checks[_xeport][_ipport].lock))
int frcore_timestamp_check_v4(cvmx_wqe_t *work, uint8_t *eth_ptr, uint8_t *ip_ptr, uint64_t smac, uint64_t dmac)
{
    int eth_len;
    uint32_t sip = 0;
    uint32_t year,month,day,hour,minute,second,ms,us,ns;
    uint8_t xeport,ipport;
    uint8_t flag_invalid = 0;
    uint64_t stamptime=0;
    uint64_t temp = 0;

    if (work->ipprt == 0)
    {
        xeport = 0;
    }else {
        xeport = 1;
    }
    UP32(ip_ptr + 12, sip);
    ipport = (*(ip_ptr +12) & 0xff)% 16;
    eth_len   = work->len;
    FRCORE_TIMESTAMP_CHECK("sip=0x%x\n", sip);
    FRCORE_TIMESTAMP_CHECK("ipport=0x%x\n", ipport);
    //frc_dump_buff(14, eth_ptr);
    year = 0;
    year |= (*(eth_ptr + 3) & 0x07)<< 8;
    year |= *(eth_ptr + 4) & 0xff;
    month = 0;
    month |= (*(eth_ptr + 5) & 0xf0) >> 4;
    day = 0;
    day |= (*(eth_ptr + 5) & 0x0f) << 1;
    day |= (*(eth_ptr + 6) & 0x80) >> 7;
    hour = 0;
    hour |= (*(eth_ptr + 6) & 0x7c) >> 2;
    minute = 0;
    minute |= (*(eth_ptr + 6) & 0x03) << 4;
    minute |= (*(eth_ptr + 7) & 0xf0) >> 4;
    second = 0;
    second |= (*(eth_ptr + 7) & 0x0f) << 2;
    second |= (*(eth_ptr + 8) & 0xc0) >> 6;
    ms = 0;
    ms |= (*(eth_ptr + 8) & 0x3f) << 4;
    ms |= (*(eth_ptr + 9) & 0xf0) >> 4;
    us = 0;
    us |= (*(eth_ptr + 9) & 0x0f) << 6;
    us |= (*(eth_ptr + 10) & 0xfc) >> 2;
    ns = 0;
    ns |= (*(eth_ptr + 10) & 0x03) << 8;
    ns |= (*(eth_ptr + 11) & 0xff);
    FRCORE_TIMESTAMP_CHECK("timestamp->year:%d\n", year);
    FRCORE_TIMESTAMP_CHECK("timestamp->month:%d\n", month);
    FRCORE_TIMESTAMP_CHECK("timestamp->date_h:%d\n", day);
    FRCORE_TIMESTAMP_CHECK("timestamp->hour:%d\n", hour);
    FRCORE_TIMESTAMP_CHECK("timestamp->minute:%d\n", minute);
    FRCORE_TIMESTAMP_CHECK("timestamp->second:%d\n", second);
    FRCORE_TIMESTAMP_CHECK("timestamp->ms:%d\n", ms);
    FRCORE_TIMESTAMP_CHECK("timestamp->us_h:%d\n", us);
    FRCORE_TIMESTAMP_CHECK("timestamp->ns:%d\n", ns);

    /* statistics */
    FRCORE_TIMESTAMP_STAT_RX_PKTS_INC();
    FRCORE_TIMESTAMP_STAT_RX_BYTES_ADD(eth_len);
    FRCORE_TIMESTAMP_STAT_XE_PKTS_INC(xeport);
    FRCORE_TIMESTAMP_STAT_XE_BYTES_ADD(xeport, eth_len);
    FRCORE_TIMESTAMP_STAT_PORT_PKTS_INC(ipport);
    FRCORE_TIMESTAMP_STAT_XE_PORT_PKTS_INC(xeport, ipport);
   /* check timestamp threshold */

    if (year < 1970 || year > 2050)
    {
        FRCORE_TIMESTAMP_STAT_XE_PORT_YEAR_INVALID_INC(xeport, ipport);
        flag_invalid = 1;
    }

    if (month < 1 || month > 12)
    {
        FRCORE_TIMESTAMP_STAT_XE_PORT_MONTH_INVALID_INC(xeport, ipport);
        flag_invalid = 1;
    }

    if (day < 1 || day > 31)
    {
        FRCORE_TIMESTAMP_STAT_XE_PORT_DAY_INVALID_INC(xeport, ipport);
        flag_invalid = 1;
    }
    if (hour > 23)
    {
        FRCORE_TIMESTAMP_STAT_XE_PORT_HOUR_INVALID_INC(xeport, ipport);
        flag_invalid = 1;
    }
    if (minute > 60)
    {
        FRCORE_TIMESTAMP_STAT_XE_PORT_MINUTE_INVALID_INC(xeport, ipport);
        flag_invalid = 1;
    }
    if (second > 60)
    {
        FRCORE_TIMESTAMP_STAT_XE_PORT_SECOND_INVALID_INC(xeport, ipport);
        flag_invalid = 1;
    }

    if (flag_invalid)
    {
        return FRCORE_ACT_FORWARD;
    }

    stamptime = ns + us*1000 + ms*1000000;
    temp = second + minute*60 + hour*3600+(year*365+month*30+day)*24*3600;
    stamptime += temp*1000000000;
    FRCORE_TIMESTAMP_CHECK_LOCK(xeport, ipport);
    if (timestamp_checks[xeport][ipport].update)
    {
        if (timestamp_checks[xeport][ipport].sip >= sip)
        {
            FRCORE_TIMESTAMP_STAT_XE_PORT_IP_ERR_INC(xeport, ipport);
            goto end;
        }

        if (timestamp_checks[xeport][ipport].timestamp >= stamptime)
        {
            FRCORE_TIMESTAMP_STAT_XE_PORT_TIMESTAMP_ERR_INC(xeport, ipport);
            goto end;
        }
    }else {
        /* first time, update */
        timestamp_checks[xeport][ipport].timestamp = stamptime;
        timestamp_checks[xeport][ipport].sip = sip;
        timestamp_checks[xeport][ipport].update = 1;
    }
    end:
    FRCORE_TIMESTAMP_CHECK_UNLOCK(xeport, ipport);
    return FRCORE_ACT_FORWARD;
}

int frcore_timestamp_check_v6(cvmx_wqe_t *work, uint8_t *eth_ptr, uint8_t *ip_ptr, uint64_t smac, uint64_t dmac)
{
    int eth_len;
    uint32_t id_check;
    uint32_t sip = 0;
    uint32_t year,month,day,hour,minute,second,ms,us,ns;
    uint8_t xeport,ipport;
    uint8_t flag_invalid = 0;
    uint64_t stamptime=0;
    uint64_t temp = 0;

    if (work->ipprt == 0)
    {
        xeport = 0;
    }else {
        xeport = 1;
    }
    UP32(ip_ptr + 20, sip);
    ipport = (*(ip_ptr +20) & 0xff)%16;
    eth_len   = work->len;
    FRCORE_TIMESTAMP_CHECK("sip=0x%x\n", sip);
    FRCORE_TIMESTAMP_CHECK("ipport=0x%x\n", ipport);
    //frc_dump_buff(14, eth_ptr);

    year = 0;
    year |= (*(eth_ptr + 3) & 0x07)<< 8;
    year |= *(eth_ptr + 4) & 0xff;
    month = 0;
    month |= (*(eth_ptr + 5) & 0xf0) >> 4;
    day = 0;
    day |= (*(eth_ptr + 5) & 0x0f) << 1;
    day |= (*(eth_ptr + 6) & 0x80) >> 7;
    hour = 0;
    hour |= (*(eth_ptr + 6) & 0x7c) >> 2;
    minute = 0;
    minute |= (*(eth_ptr + 6) & 0x03) << 4;
    minute |= (*(eth_ptr + 7) & 0xf0) >> 4;
    second = 0;
    second |= (*(eth_ptr + 7) & 0x0f) << 2;
    second |= (*(eth_ptr + 8) & 0xc0) >> 6;
    ms = 0;
    ms |= (*(eth_ptr + 8) & 0x3f) << 4;
    ms |= (*(eth_ptr + 9) & 0xf0) >> 4;
    us = 0;
    us |= (*(eth_ptr + 9) & 0x0f) << 6;
    us |= (*(eth_ptr + 10) & 0xfc) >> 2;
    ns = 0;
    ns |= (*(eth_ptr + 10) & 0x03) << 8;
    ns |= (*(eth_ptr + 11) & 0xff);
    FRCORE_TIMESTAMP_CHECK("timestamp->year:%d\n", year);
    FRCORE_TIMESTAMP_CHECK("timestamp->month:%d\n", month);
    FRCORE_TIMESTAMP_CHECK("timestamp->date_h:%d\n", day);
    FRCORE_TIMESTAMP_CHECK("timestamp->hour:%d\n", hour);
    FRCORE_TIMESTAMP_CHECK("timestamp->minute:%d\n", minute);
    FRCORE_TIMESTAMP_CHECK("timestamp->second:%d\n", second);
    FRCORE_TIMESTAMP_CHECK("timestamp->ms:%d\n", ms);
    FRCORE_TIMESTAMP_CHECK("timestamp->us_h:%d\n", us);
    FRCORE_TIMESTAMP_CHECK("timestamp->ns:%d\n", ns);
    /* statistics */
    FRCORE_TIMESTAMP_STAT_RX_PKTS_INC();
    FRCORE_TIMESTAMP_STAT_RX_BYTES_ADD(eth_len);
    FRCORE_TIMESTAMP_STAT_XE_PKTS_INC(xeport);
    FRCORE_TIMESTAMP_STAT_XE_BYTES_ADD(xeport, eth_len);
    FRCORE_TIMESTAMP_STAT_PORT_PKTS_INC(ipport);
    FRCORE_TIMESTAMP_STAT_XE_PORT_PKTS_INC(xeport, ipport);
   /* check timestamp threshold */

    if (year < 1970 || year > 2050)
    {
        FRCORE_TIMESTAMP_STAT_XE_PORT_YEAR_INVALID_INC(xeport, ipport);
        flag_invalid = 1;
    }

    if (month < 1 || month > 12)
    {
        FRCORE_TIMESTAMP_STAT_XE_PORT_MONTH_INVALID_INC(xeport, ipport);
        flag_invalid = 1;
    }

    if (day < 1 || day > 31)
    {
        FRCORE_TIMESTAMP_STAT_XE_PORT_DAY_INVALID_INC(xeport, ipport);
        flag_invalid = 1;
    }
    if (hour > 23)
    {
        FRCORE_TIMESTAMP_STAT_XE_PORT_HOUR_INVALID_INC(xeport, ipport);
        flag_invalid = 1;
    }
    if (minute > 60)
    {
        FRCORE_TIMESTAMP_STAT_XE_PORT_MINUTE_INVALID_INC(xeport, ipport);
        flag_invalid = 1;
    }
    if (second > 60)
    {
        FRCORE_TIMESTAMP_STAT_XE_PORT_SECOND_INVALID_INC(xeport, ipport);
        flag_invalid = 1;
    }

    if (flag_invalid)
    {
        return FRCORE_ACT_FORWARD;
    }

    stamptime = ns + us*1000 + ms*1000000;
    temp = second + minute*60 + hour*3600+(year*365+month*30+day)*24*3600;
    stamptime += temp*1000000000;
    FRCORE_TIMESTAMP_CHECK_LOCK(xeport, ipport);
    if (timestamp_checks[xeport][ipport].update)
    {
        if (timestamp_checks[xeport][ipport].sip >= sip)
        {
            FRCORE_TIMESTAMP_STAT_XE_PORT_IP_ERR_INC(xeport, ipport);
            goto end;
        }

        if (timestamp_checks[xeport][ipport].timestamp >= stamptime)
        {
            FRCORE_TIMESTAMP_STAT_XE_PORT_TIMESTAMP_ERR_INC(xeport, ipport);
            goto end;
        }
    }else {
        /* first time, update */
        timestamp_checks[xeport][ipport].timestamp = stamptime;
        timestamp_checks[xeport][ipport].sip = sip;
        timestamp_checks[xeport][ipport].update = 1;
    }
    end:
    FRCORE_TIMESTAMP_CHECK_UNLOCK(xeport, ipport);
    return FRCORE_ACT_FORWARD;
}


int frcore_cmd_timestamp_check_get_stat(uint16_t plen, void *param, uint16_t *olen, void *outbuf)
{
    int i;
    frc_timestamp_op_in_t *input = (frc_timestamp_op_in_t *) param;
    uint64_t *v64p;
    int cnt_offset = 0;

    //FRCORE_TIMESTAMP_CHECK("plen %d, param %p, *olne %d, outbuf %p.\n", plen, param, *olen, outbuf);

    FRCORE_TIMESTAMP_CHECK("input->index=%d\n", input->index);
    FRCORE_TIMESTAMP_CHECK("input->num  =%d\n", input->num);
    cnt_offset = 0;
    v64p = outbuf;
    for (i = 0; i < input->num; i++, v64p++)
    {
        *v64p = FRCORE_TIMESTAMP_STAT_VAL(input->index + i + cnt_offset);
        if (*v64p > 0)
        {
            FRCORE_TIMESTAMP_CHECK("timestamp_stat[%d]= %lld\n", input->index+i,*v64p);
        }
    }

    *olen = input->num * sizeof(uint64_t);

    return 0;
}

void frcore_timestamp_check_stat_clear()
{
    int i;

    for (i = 0; i < stat_timestamp_max; i++)
    {
        FRCORE_TIMESTAMP_STAT_CLEAR(i);
    }
}
int frcore_cmd_timestamp_check_clear_stat(uint16_t plen, void *param, uint16_t *olen, void *outbuf)
{
    *olen = 0;
    frcore_timestamp_check_stat_clear();
    return FRE_SUCCESS;
}
int
frcore_timestamp_check_init()
{
    FRCORE_REGISTER_CMD(CMD_TYPE_USER, USER_CMD_GET_TIMESTAMP_CHECK_STAT,       frcore_cmd_timestamp_check_get_stat);
    FRCORE_REGISTER_CMD(CMD_TYPE_USER, USER_CMD_CLEAR_TIMESTAMP_CHECK_STAT,     frcore_cmd_timestamp_check_clear_stat);

    memset(timestamp_checks, 0, sizeof(timestamp_checks));
    return 0;
}
#endif

/* End of file */

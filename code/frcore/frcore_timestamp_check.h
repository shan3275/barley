#ifndef _FRCORE_TIMESTAMP_CHEKC_H_
#define _FRCORE_TIMESTAMP_CHECK_H_
#include "frcore_pkt.h"
#include "frcore_stat.h"
#include "frc_dma.h"
#include "frcore_proto.h"
#include "frcore_ssn.h"
#include "frcore_ssn_priv.h"
#include "frcore.h"
#include "frcore_cmd.h"
#include "frcore_misc.h"
#include "frc_pack.h"
#include "frcore_proto.h"
#include "frcore_init.h"
#include "frc_types.h"
#include "frcore_stat.h"
#include "frcore_alg.h"

#if FRC_CONFIG_TIMESTAMP_CHECK

#define FRCORE_TIMESTAMP_STAT_INC(_cnt)         cvmx_atomic_add64((int64_t *)(&timestamp_stat[_cnt]), 1)
#define FRCORE_TIMESTAMP_STAT_ADD(_cnt, _val)   cvmx_atomic_add64((int64_t *)(&timestamp_stat[_cnt]), _val)
#define FRCORE_TIMESTAMP_STAT_VAL(_cnt)         cvmx_atomic_get64((int64_t *)(&timestamp_stat[_cnt]))
#define FRCORE_TIMESTAMP_STAT_CLEAR(_cnt)       cvmx_atomic_set64((int64_t *)(&timestamp_stat[_cnt]), 0)

#define FRCORE_TIMESTAMP_STAT_RX_PKTS_INC()     FRCORE_TIMESTAMP_STAT_INC(stat_rxx_pkts)
#define FRCORE_TIMESTAMP_STAT_RX_BYTES_ADD(_val)    FRCORE_TIMESTAMP_STAT_ADD(stat_rxx_bytes, _val)

#define FRCORE_TIMESTAMP_STAT_XE_PKTS_INC(_xeport)   \
 if (0 == _xeport) { \
   FRCORE_TIMESTAMP_STAT_INC(xe0_stat_rxx_pkts); \
} else \
{ \
   FRCORE_TIMESTAMP_STAT_INC(xe1_stat_rxx_pkts); \
}

#define FRCORE_TIMESTAMP_STAT_XE_BYTES_ADD(_xeport, _val) \
   if (0 == _xeport) { \
   FRCORE_TIMESTAMP_STAT_ADD(xe0_stat_rxx_bytes, _val); \
}else \
{\
   FRCORE_TIMESTAMP_STAT_ADD(xe1_stat_rxx_bytes, _val); \
}


#define FRCORE_TIMESTAMP_STAT_PORT_PKTS_INC(_ipport)  FRCORE_TIMESTAMP_STAT_INC(stat_port0_rxx_pkts + (_ipport)*19 )

#define FRCORE_TIMESTAMP_STAT_XE_PORT_PKTS_INC(_xeport, _ipport)   \
 if (0 == _xeport) { \
   FRCORE_TIMESTAMP_STAT_INC(xe0_stat_port0_rxx_pkts + (_ipport )*19); \
} else \
{ \
   FRCORE_TIMESTAMP_STAT_INC(xe1_stat_port0_rxx_pkts + (_ipport )*19); \
}

#define FRCORE_TIMESTAMP_STAT_XE_PORT_YEAR_INVALID_INC(_xeport, _ipport)   \
 if (0 == _xeport) { \
   FRCORE_TIMESTAMP_STAT_INC(xe0_stat_port0_year_invalid_pkts + (_ipport )*19); \
} else \
{ \
   FRCORE_TIMESTAMP_STAT_INC(xe1_stat_port0_year_invalid_pkts + (_ipport )*19); \
}

#define FRCORE_TIMESTAMP_STAT_XE_PORT_MONTH_INVALID_INC(_xeport, _ipport)   \
 if (0 == _xeport) { \
   FRCORE_TIMESTAMP_STAT_INC(xe0_stat_port0_month_invalid_pkts + (_ipport )*19); \
} else \
{ \
   FRCORE_TIMESTAMP_STAT_INC(xe1_stat_port0_month_invalid_pkts + (_ipport )*19); \
}

#define FRCORE_TIMESTAMP_STAT_XE_PORT_DAY_INVALID_INC(_xeport, _ipport)   \
 if (0 == _xeport) { \
   FRCORE_TIMESTAMP_STAT_INC(xe0_stat_port0_day_invalid_pkts + (_ipport )*19); \
} else \
{ \
   FRCORE_TIMESTAMP_STAT_INC(xe1_stat_port0_day_invalid_pkts + (_ipport )*19); \
}

#define FRCORE_TIMESTAMP_STAT_XE_PORT_HOUR_INVALID_INC(_xeport, _ipport)   \
 if (0 == _xeport) { \
   FRCORE_TIMESTAMP_STAT_INC(xe0_stat_port0_hour_invalid_pkts + (_ipport )*19); \
} else \
{ \
   FRCORE_TIMESTAMP_STAT_INC(xe1_stat_port0_hour_invalid_pkts + (_ipport )*19); \
}

#define FRCORE_TIMESTAMP_STAT_XE_PORT_MINUTE_INVALID_INC(_xeport, _ipport)   \
 if (0 == _xeport) { \
   FRCORE_TIMESTAMP_STAT_INC(xe0_stat_port0_minute_invalid_pkts + (_ipport )*19); \
} else \
{ \
   FRCORE_TIMESTAMP_STAT_INC(xe1_stat_port0_minute_invalid_pkts + (_ipport )*19); \
}

#define FRCORE_TIMESTAMP_STAT_XE_PORT_SECOND_INVALID_INC(_xeport, _ipport)   \
 if (0 == _xeport) { \
   FRCORE_TIMESTAMP_STAT_INC(xe0_stat_port0_second_invalid_pkts + (_ipport )*19); \
} else \
{ \
   FRCORE_TIMESTAMP_STAT_INC(xe1_stat_port0_second_invalid_pkts + (_ipport )*19); \
}


#define FRCORE_TIMESTAMP_STAT_XE_PORT_IP_ERR_INC(_xeport, _ipport)   \
 if (0 == _xeport) { \
   FRCORE_TIMESTAMP_STAT_INC(xe0_stat_port0_ip_err_pkts + (_ipport )*19); \
} else \
{ \
   FRCORE_TIMESTAMP_STAT_INC(xe1_stat_port0_ip_err_pkts + (_ipport )*19); \
}

#define FRCORE_TIMESTAMP_STAT_XE_PORT_TIMESTAMP_ERR_INC(_xeport, _ipport)   \
 if (0 == _xeport) { \
 FRCORE_TIMESTAMP_STAT_INC(xe0_stat_port0_timestamp_err_pkts + (_ipport )*19); \
} else \
{ \
 FRCORE_TIMESTAMP_STAT_INC(xe1_stat_port0_timestamp_err_pkts + (_ipport )*19); \
}

#if 1
typedef struct timestamp {
   uint8_t reserved[3]; /* dmac bit 47--24 */
   uint16_t zero:5;   /* dmac bit 23--19, all 0*/
   uint16_t year:11;  /* dmac bit 18-8, year */
   uint8_t  month:4;  /* dmac bit 7--4, month */
   uint8_t  date_h:4; /* dmac bit 3--0, date high 4 bit */
   uint32_t date_l:1; /* smac bit 47, date low 1 bit */
   uint32_t hour:5;   /* smac bit 46--42, hour*/
   uint32_t minute:6; /* smac bit 41--36, minute*/
   uint32_t second:6; /* smac bit 35--30, second*/
   uint32_t ms:10;    /* smac bit 29--20, ms*/
   uint32_t us_h:4;   /* smac bit 19--16, us hig 4 bit*/
   uint16_t us_l:6;   /* smac bit 15--10, us low 6 bit*/
   uint16_t ns:10;    /* smac bit 9--0, ns*/
}timestamp_t;
#else
struct timestamp {
   uint16_t year;
   uint8_t  month;
   uint8_t  day;
   uint8_t  hour;
   uint8_t  minute;
   uint8_t  second;
   uint16_t ms;
   uint16_t us;
   uint16_t ns;

};
#endif

struct timestamp_check{
   uint8_t update; /* default 0, 1 for effective */
   uint32_t sip;   /* source ip */
   uint64_t timestamp; /* timestamp */
   cvmx_spinlock_t lock;
};

int frcore_timestamp_check_v4(cvmx_wqe_t *work, uint8_t *eth_ptr, uint8_t *ip_ptr, uint64_t smac, uint64_t dmac);
int frcore_timestamp_check_v6(cvmx_wqe_t *work, uint8_t *eth_ptr, uint8_t *ip_ptr, uint64_t smac, uint64_t dmac);
int frcore_timestamp_check_init();
#endif /* end of FRC_CONFIG_VLAN_CHECK */
#endif /* !_FRCORE_VLAN_CHECK_H_ */

#ifndef __FRCORE_PKT_H__
#define __FRCORE_PKT_H__

#include "cvmx-config.h"
#include "cvmx.h"
#include "cvmx-higig.h"
#include "cvmx-spinlock.h"
#include "cvmx-fpa.h"
#include "cvmx-pip.h"
//#include "cvmx-ciu.h"
#include "cvmx-ipd.h"
#include "cvmx-pko.h"
#include "cvmx-dfa.h"
#include "cvmx-pow.h"
#include "cvmx-gmx.h"
//#include "cvmx-asx.h"
#include "cvmx-uart.h"
#include "cvmx-sysinfo.h"
#include "cvmx-coremask.h"
#include "cvmx-bootmem.h"
#include "cvmx-atomic.h"
#include "cvmx-helper.h"
#include "cvmx-helper-board.h"
#include "cvmx-npi.h"
#include "cvmx-ebt3000.h"
#include "cvmx-spi.h"
#include "cvmx-tim.h"
#include "cvmx-csr-db.h"


#include "cvmcs-common.h"
#include  <cvmx-atomic.h>

//#include "lib_octeon_shared.h"
//#include "octeon_mem_map.h"

#include "frcore_config.h"

#include "frcore.h"


#define NUM_PACKET_BUFFERS  4096
#define NUM_WORK_BUFFERS    NUM_PACKET_BUFFERS

#define FRCORE_SIM_CLOCK_HZ    1000000

#define FRCORE_CORE_NUM        12
#define FRCORE_PORT_NUM        4
#define FRCORE_DMAC_NUM        16

#define FRCORE_TIM_TICKS       100
#define FRCORE_TIM_TICKS_PSEC  (1000000 / FRCORE_TIM_TICKS)


#define FRCORE_MTU             1512

#define FRCORE_IP_HD_SZ        20

#define FRCORE_TCP_HD_SZ       20
#define FRCORE_TCP_OFFSET      (FRCORE_IP_HD_SZ + FRCORE_TCP_HD_SZ)
#define FRCORE_TCP_MTU         (FRCORE_MTU - FRCORE_TCP_OFFSET)

#define FRCORE_NUM_PKT_BUFS    (K)
#define FRCORE_ETHERNET_CRC    4
#define TCP_WINDOWS_SIZE    65535
//#define TCP_WINDOWS_SIZE         (6 * K)

typedef struct
{
    cvmx_pip_port_status_t  input_statistics;
    cvmx_helper_interface_mode_t imode;
    int                     display;
    uint64_t             link_state;
    int                     link_up;
} frcore_port_state_t;

typedef enum {
    FRCORE_ACT_DEBUG,
    FRCORE_ACT_DROP,
    FRCORE_ACT_FORWARD,
    FRCORE_ACT_DELAY,
    FRCORE_ACT_UNFREE,
    FRCORE_ACT_FREE,
    FRCORE_ACT_MAX
} frcore_act_t;

typedef enum {
    SPTE_SUCCESS,
    SPTE_FAIL,
    SPTE_MEMORY,
    SPTE_MAX
} frcore_err_t;

typedef enum {
    FRCORE_WORK_PKT,
    FRCORE_WORK_STAT,
    FRCORE_WORK_QUEUE,
    FRCORE_WORK_DMA_LOOP,
    FRCORE_WORK_CHAN,
    FRCORE_WORK_CHAN_TEST,
    FRCORE_WORK_SSN_AGE
} frcore_work_t;


typedef unsigned char mac_t[6];



#define UNPACK_U8(_buf, _var) \
        _var = *_buf++ & 0xff

#define UNPACK_U16(_buf, _var) \
        _var  = (*_buf++ & 0xff) << 8; \
        _var |= (*_buf++ & 0xff);

#define UNPACK_U32(_buf, _var) \
        _var = 0; \
        _var |= (*_buf++ & 0xff) << 24; \
        _var |= (*_buf++ & 0xff) << 16; \
        _var |= (*_buf++ & 0xff) << 8; \
        _var |= (*_buf++ & 0xff);


#define UP8(_buf, _var) \
        _var = *(_buf) & 0xff

#define UP16(_buf, _var) \
        _var  = (*(_buf) & 0xff) << 8; \
        _var |= (*(_buf + 1) & 0xff);

#define UP32(_buf, _var) \
        _var = 0; \
        _var |= (*(_buf) & 0xff) << 24; \
        _var |= (*(_buf + 1) & 0xff) << 16; \
        _var |= (*(_buf + 2) & 0xff) << 8; \
        _var |= (*(_buf + 3) & 0xff);

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


#define TRAFFICGEN_SCR_WORK            (0)                  /**< Async get work */

inline void frcore_tx_work(cvmx_wqe_t *work);

#define IP_STR_LEN      100

extern void ip_string(uint32_t ip, char str[IP_STR_LEN]);

#define FRCORE_DROP(_cnt) \
    FRCORE_STAT_INC(_cnt); \
    return FRCORE_ACT_DROP;

#define FRCORE_FREE \
    return FRCORE_ACT_FREE



int frcore_pkt_main(void);
void frcore_work_process(cvmx_wqe_t *wqe);

void frcore_work_free(cvmx_wqe_t *work);

void kinds_of_pkt_len_stat(cvmx_wqe_t  *work);

#endif /* !__FRCORE_PKT_H__ */

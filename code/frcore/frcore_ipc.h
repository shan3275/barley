#ifndef __FRCORE_IPC_H__
#define __FRCORE_IPC_H__

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


typedef struct
{
    uint32_t oui    : 24;
    uint32_t resv0  : 8;
    uint16_t resv1  : 5;
    uint16_t iif    : 11; 
    uint8_t smac[6];
    uint16_t protocol;
    uint16_t cmd_number;
} ipc_instr_hd_t;

typedef union
{
    struct {
        uint16_t  oui;
        uint32_t  pid_l;
        uint8_t   chassis   : 4;
        uint8_t   resv      : 4;
        uint16_t  ori_vid;
        uint16_t  pid_h;
        uint8_t   slot      : 5;
        uint8_t   card      : 1;
        uint8_t   ifid      : 2;
    } field;
    uint32_t word[3];
} ipc_dpkt_hd_t;



typedef struct
{
    cvmx_spinlock_t lock;
    char *outbuf;
    //cvmx_wqe_t *wqe;
} ipc_instr_entry_t;



typedef struct
{
    uint64_t     pktid;
    uint16_t     dtype   : 4;
    uint16_t     ip_len  : 12;
} ipc_cmd_t;

typedef union 
{
    struct {
        uint32_t version        : 4;
        uint32_t ih             : 4;
        uint32_t tos            : 8;
        uint32_t tot_len        : 16;
        uint32_t id             : 16;
        uint32_t rflag          : 1;
        uint32_t dflag          : 1;
        uint32_t mflag          : 1;
        uint32_t frag_offset    : 13;

        uint32_t ttl            : 8;
        uint32_t protocol       : 8;
        uint32_t check          : 16;
        uint32_t saddr;
        uint32_t daddr;
    } field;
    uint32_t word[5];
} ipc_ipv4_hd_t;

int frcore_ipc_init();

#endif /* !__FRCORE_IPC_H__ */

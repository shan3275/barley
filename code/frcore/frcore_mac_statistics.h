#ifndef _FRC_MAC_STATISTICS_H_
#define _FRC_MAC_STATISTICS_H_
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

#if FRC_CONFIG_MAC_STATISTICS

#define HASH_SIP            0
#define HASH_DIP            1
#define HASH_SIP_DIP        2
#define HASH_FIVE_TUPLE     3

#define HEART_BEAT_OFF                  0
#define HEART_BEAT_ON                   1
#define HEART_BEAT_ON_WITH_DROP         2

#define IP_TOS              1
#define IP_TTL              2

extern CVMX_SHARED uint16_t heart_beat;
struct frc_mac_stat {
    uint64_t mac;
    uint64_t total;
    uint64_t errors;
};

int frcore_mac_statistics_add_mac(uint16_t plen, void *param, uint16_t *olen, void *outbuf);
int frcore_mac_statistics_del_mac(uint16_t plen, void *param, uint16_t *olen, void *outbuf);
int frcore_mac_statistics_del_all(uint16_t plen, void *param, uint16_t *olen, void *outbuf);
int frcore_mac_statistics_clear_counter(uint16_t plen, void *param, uint16_t *olen, void *outbuf);
int frcore_mac_statistics_clear_all_counter(uint16_t plen, void *param, uint16_t *olen, void *outbuf);
int frcore_mac_statistics_show_by_mac(uint16_t plen, void *param, uint16_t *olen, void *outbuf);
int frcore_mac_statistics_show_by_index(uint16_t plen, void *param, uint16_t *olen, void *outbuf);

int frcore_mac_statistics_set_heart_beat(uint16_t plen, void *param, uint16_t *olen, void *outbuf);
int frcore_mac_statistics_set_hash_mode(uint16_t plen, void *param, uint16_t *olen, void *outbuf);
int frcore_mac_statistics_count_inc(uint8_t *ether_ptr);

void pkt_dump(uint8_t *ether, int length);
int frcore_mac_statistics_init();

#endif
#endif

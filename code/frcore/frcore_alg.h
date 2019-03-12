#ifndef __TYPES_H__
#define __TYPES_H__

#include "cvmx-config.h"
#include "cvmx.h"
#include "cvmx-wqe.h"
#include "cvmx-fau.h"
#include "cvmx-atomic.h"
#include "cvmx-spinlock.h"

#include "frcore_init.h"
#include "frcore_config.h"
//#include "mpp_counter.h"
#include "frcore_debug.h"
#include "frc_pack.h"

#if FRC_CONFIG_VLAN_CHECK
#define E0 2001
#define EMAX4 256
#define EMAX6 256

typedef unsigned int bcm_ip_t ;
typedef unsigned char bcm_ip6_t[16] ;

#define BIT(b)  (1ULL<<b)
#define BIT_GET(v,b) (!!(v & BIT(b)))
#define BIT_SET(v,b,val) \
        do \
{ \
        if (val) v |= BIT(b); \
        else v &= (~(BIT(b))); \
}while(0)

typedef enum
{
        FS_HSEL_NULL,
        FS_HSEL_SIP,
        FS_HSEL_DIP,
        FS_HSEL_SDIP
}HASH_EM;

typedef enum
{
        IPV4,
        IPV6
}IP_TYPE;

typedef struct ip4_pkt
{
        bcm_ip_t sip_data;
        bcm_ip_t dip_data;
        bcm_ip_t sip_mask;
        bcm_ip_t dip_mask;
}ip4_pkt_t;

typedef struct ip6_pkt
{
        bcm_ip6_t sip6_data;
        bcm_ip6_t dip6_data;
        bcm_ip6_t sip6_mask;
        bcm_ip6_t dip6_mask;
}ip6_pkt_t;

uint32_t ip4_calc_vlan(ip4_pkt_t *ip4_pkt, HASH_EM hash, uint32_t vlan_n, uint32_t vlan_0);
uint32_t ip6_calc_vlan(ip6_pkt_t *ip6_pkt, HASH_EM hash, uint32_t vlan_n, uint32_t vlan_0);
#endif /* end of FRC_CONFIG_VLAN_CHECK */
#endif /*__TYPES_H__*/

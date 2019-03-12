#include "frcore_pkt.h"
#include "frcore_tcp.h"
#include "frcore_udp.h"
#include "frcore_ip.h"
#include "frcore_stat.h"
#include "frcore_proto.h"
#include "frcore_rule.h"
#include "frcore_twotuple.h"
#include "frcore_vlan_check.h"

CVMX_SHARED uint8_t fr_enable = 1;
//CVMX_SHARED uint8_t rule_enable = 1;

int frcore_ip_option_process(cvmx_wqe_t *work, uint8_t *ip_ptr)
{
    FRCORE_STAT_INC(stat_ip_option);

    FRCORE_DROP(stat_drop_ip_option);
}

int frcore_ip_frag_process(cvmx_wqe_t *work, uint8_t *ip_ptr)
{
    /* Unsupport ip fragement packet, drop it. */
    FRCORE_DROP(stat_drop_ip_frag);
}

CVMX_SHARED uint64_t rx_ip_pkts = 0;


inline int frcore_ip_process(cvmx_wqe_t *work, uint8_t *eth_ptr, uint8_t *ip_ptr, uint64_t smac, uint64_t dmac)
{
    //FRCORE_CYCLE_RECORDING();
    uint8_t header_len;
    uint16_t total_len;
    uint8_t  proto;
    #if FRC_CONFIG_VLAN_CHECK | FRC_CONFIG_TIMESTAMP_CHECK
    uint8_t ip_version;
    uint8_t vlan_flag = 0;
    #endif
    uint16_t paylen = 0;
    uint8_t *payload = NULL;
    int rv;

    FRCORE_PKT("=============== rx_ip_pkts %lld.\n", (ULL) ++rx_ip_pkts);

    FRCORE_STAT_INC(stat_ip);
    //printf("pkt_len = %d\n", work->len);
#if FRCORE_CONFIG_ADVANCED_IP

    /* Unspport IPV6 in this version, drop packet. */
    if (work->word2.s.is_v6) {
        FRCORE_PKT("work->word2.s.is_v6 = %d\n", work->word2.s.is_v6);
        FRCORE_STAT_INC(stat_ip_v6);
        FRCORE_DROP(stat_drop_ip_v6);
    } else {
        FRCORE_STAT_INC(stat_ip_v4);
    }



    /* Process ip fragement packet. */
    if (work->word2.s.is_frag) {
        FRCORE_PKT("work->word2.s.is_frag = %d\n", work->word2.s.is_frag);
        FRCORE_STAT_INC(stat_ip_frag);
        return frcore_ip_frag_process(work, ip_ptr);
    }
#else
    if (work->word2.s.is_v6) {
        FRCORE_PKT("work->word2.s.is_v6 = %d\n", work->word2.s.is_v6);
        FRCORE_STAT_INC(stat_ip_v6);
        #if FRC_CONFIG_VLAN_CHECK | FRC_CONFIG_TIMESTAMP_CHECK
        ip_version = FRCORE_IPV6;
        #else
        FRCORE_DROP(stat_drop_ip_v6);
        #endif
    } else {
        FRCORE_STAT_INC(stat_ip_v4);
        #if FRC_CONFIG_VLAN_CHECK | FRC_CONFIG_TIMESTAMP_CHECK
        ip_version = FRCORE_IPV4;
        #endif
    }

    /* vlan packet */
    if (work->word2.s.vlan_valid) {
        FRCORE_STAT_INC(stat_vlan);
        //FRCORE_DROP(stat_drop_vlan);
		#if FRC_CONFIG_VLAN_CHECK | FRC_CONFIG_TIMESTAMP_CHECK
        vlan_flag = 1;
		#endif
    }

    if (work->word2.s.is_frag) {
        FRCORE_PKT("work->word2.s.is_frag = %d\n", work->word2.s.is_frag);
        FRCORE_STAT_INC(stat_ip_frag);
        #if !FRC_CONFIG_VLAN_CHECK
        FRCORE_DROP(stat_drop_ip_frag);
        #endif
    }
#endif

    /* Drop not tcp or udp packet. */
    if (!work->word2.s.tcp_or_udp) {
        FRCORE_PKT("work->word2.s.tcp_or_udp = %d\n", work->word2.s.tcp_or_udp);
        FRCORE_STAT_INC(stat_ip_not_tcp_udp);
        FRCORE_DROP(stat_drop_ip_not_tcp_udp);
    }

    #if FRC_CONFIG_VLAN_CHECK | FRC_CONFIG_TIMESTAMP_CHECK
    if (ip_version == FRCORE_IPV4) {
        UP8(ip_ptr, header_len);
        header_len = (header_len & 0x0F) * 4;

        if (header_len != 20) {
            //return frcore_ip_option_process(work, ip_ptr);

            FRCORE_STAT_INC(stat_ip_option);
        }

        UP16(ip_ptr + 2, total_len);
        UP8(ip_ptr + 9, proto);

        /* Get ip protocol. */
        paylen = total_len - header_len;
        FRCORE_PKT("total_len %u header_len %u.\n", total_len, header_len);

        payload = ip_ptr + header_len;
        if (total_len > 1582)
        {
            //FRCORE_ERROR("total_len(%d) > 1600.\n", total_len);
            FRCORE_STAT_INC(stat_ip_other);
            FRCORE_DROP(stat_drop_ip_other);
        }

        if (paylen == 0)
        {
            FRCORE_ERROR("paylen == 0.\n");
            FRCORE_STAT_INC(stat_ip_other);
            FRCORE_DROP(stat_drop_ip_other);
        }

    }
	#endif
#if FRC_CONFIG_RULE
    frcore_rule_process(work, eth_ptr, ip_ptr, payload, paylen, smac, dmac);
#endif

#if FRC_CONFIG_TWO_TUPLE
    frcore_acl_process(work, eth_ptr, ip_ptr, payload, paylen, smac, dmac);
#endif
    //FRCORE_STAT_INC(stat_ip);
    #if FRC_CONFIG_VLAN_CHECK | FRC_CONFIG_TIMESTAMP_CHECK
    if (ip_version == FRCORE_IPV4) {
        switch (proto) {
        case PROTO_TCP: /* Process tcp packet. */
            FRCORE_STAT_INC(stat_tcp);
        #if FRC_CONFIG_TCP
            if (fr_enable) {
                rv = frcore_tcp_process(work, eth_ptr, ip_ptr, payload, paylen, smac, dmac);
            }
            else
                rv = FRCORE_ACT_DROP;
        #else
            rv = FRCORE_ACT_DROP;
        #endif
            break;
        case PROTO_UDP: /* Process udp packet. */
            FRCORE_STAT_INC(stat_udp);
        #if FRC_CONFIG_UDP
            rv = frcore_udp_process(work, eth_ptr, ip_ptr, payload, paylen, smac, dmac);
        #else
            rv = FRCORE_ACT_DROP;
        #endif

            break;
        default: /* Drop other ip packet. */
            FRCORE_STAT_INC(stat_ip_other);
            FRCORE_DROP(stat_drop_ip_other);
            rv = FRCORE_ACT_DROP;
            break;
        }
    }
	#endif
    /* handle vlan checker */
    #if FRC_CONFIG_VLAN_CHECK
    if (vlan_flag) {
        if (ip_version == FRCORE_IPV4) {
            rv = frcore_vlan_check_v4(work, eth_ptr, ip_ptr, smac, dmac);
        } else if (ip_version == FRCORE_IPV6) {
            rv = frcore_vlan_check_v6(work, eth_ptr, ip_ptr, smac, dmac);
        }
    }
    #endif

    /* handle timestamp checker */
    #if FRC_CONFIG_TIMESTAMP_CHECK
    if (ip_version == FRCORE_IPV4) {
        rv = frcore_timestamp_check_v4(work, eth_ptr, ip_ptr, smac, dmac);
    } else if (ip_version == FRCORE_IPV6) {
        rv = frcore_timestamp_check_v6(work, eth_ptr, ip_ptr, smac, dmac);
    }
    #endif
    return rv;
}
/* End of file */

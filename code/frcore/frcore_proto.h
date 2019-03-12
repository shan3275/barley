#ifndef __FRCORE_PROTO_H__
#define __FRCORE_PROTO_H__


#ifndef _LINUX_IF_ETHER_H
/*
 *  IEEE 802.3 Ethernet magic constants.  The frame sizes omit the preamble
 *  and FCS/CRC (frame check sequence).
 */

#define ETH_ALEN    6       /* Octets in one ethernet addr   */
#define ETH_HLEN    14      /* Total octets in header.   */
#define ETH_ZLEN    60      /* Min. octets in frame sans FCS */
#define ETH_DATA_LEN    1500        /* Max. octets in payload    */
#define ETH_FRAME_LEN   1514        /* Max. octets in frame sans FCS */
#define ETH_FCS_LEN 4       /* Octets in the FCS         */

/*
 *  These are the defined Ethernet Protocol ID's.
 */

#define ETH_P_LOOP  0x0060      /* Ethernet Loopback packet */
#define ETH_P_PUP   0x0200      /* Xerox PUP packet     */
#define ETH_P_PUPAT 0x0201      /* Xerox PUP Addr Trans packet  */
#define ETH_P_IP    0x0800      /* Internet Protocol packet */
#define ETH_P_X25   0x0805      /* CCITT X.25           */
#define ETH_P_ARP   0x0806      /* Address Resolution packet    */
#define ETH_P_BPQ   0x08FF      /* G8BPQ AX.25 Ethernet Packet  [ NOT AN OFFICIALLY REGISTERED ID ] */
#define ETH_P_IEEEPUP   0x0a00      /* Xerox IEEE802.3 PUP packet */
#define ETH_P_IEEEPUPAT 0x0a01      /* Xerox IEEE802.3 PUP Addr Trans packet */
#define ETH_P_DEC       0x6000          /* DEC Assigned proto           */
#define ETH_P_DNA_DL    0x6001          /* DEC DNA Dump/Load            */
#define ETH_P_DNA_RC    0x6002          /* DEC DNA Remote Console       */
#define ETH_P_DNA_RT    0x6003          /* DEC DNA Routing              */
#define ETH_P_LAT       0x6004          /* DEC LAT                      */
#define ETH_P_DIAG      0x6005          /* DEC Diagnostics              */
#define ETH_P_CUST      0x6006          /* DEC Customer use             */
#define ETH_P_SCA       0x6007          /* DEC Systems Comms Arch       */
#define ETH_P_RARP      0x8035      /* Reverse Addr Res packet  */
#define ETH_P_ATALK 0x809B      /* Appletalk DDP        */
#define ETH_P_AARP  0x80F3      /* Appletalk AARP       */
#define ETH_P_8021Q 0x8100          /* 802.1Q VLAN Extended Header  */
#define ETH_P_IPX   0x8137      /* IPX over DIX         */
#define ETH_P_IPV6  0x86DD      /* IPv6 over bluebook       */
#define ETH_P_PAUSE 0x8808      /* IEEE Pause frames. See 802.3 31B */
#define ETH_P_SLOW  0x8809      /* Slow Protocol. See 802.3ad 43B */
#define ETH_P_WCCP  0x883E      /* Web-cache coordination protocol
                     * defined in draft-wilson-wrec-wccp-v2-00.txt */
#define ETH_P_PPP_DISC  0x8863      /* PPPoE discovery messages     */
#define ETH_P_PPP_SES   0x8864      /* PPPoE session messages   */
#define ETH_P_MPLS_UC   0x8847      /* MPLS Unicast traffic     */
#define ETH_P_MPLS_MC   0x8848      /* MPLS Multicast traffic   */
#define ETH_P_ATMMPOA   0x884c      /* MultiProtocol Over ATM   */
#define ETH_P_ATMFATE   0x8884      /* Frame-based ATM Transport
                     * over Ethernet
                     */
#define ETH_P_AOE   0x88A2      /* ATA over Ethernet        */
#define ETH_P_TIPC  0x88CA      /* TIPC             */



/*
 *  Non DIX types. Won't clash for 1500 types.
 */

#define ETH_P_802_3 0x0001      /* Dummy type for 802.3 frames  */
#define ETH_P_AX25  0x0002      /* Dummy protocol id for AX.25  */
#define ETH_P_ALL   0x0003      /* Every packet (be careful!!!) */
#define ETH_P_802_2 0x0004      /* 802.2 frames         */
#define ETH_P_SNAP  0x0005      /* Internal only        */
#define ETH_P_DDCMP     0x0006          /* DEC DDCMP: Internal only     */
#define ETH_P_WAN_PPP   0x0007          /* Dummy type for WAN PPP frames*/
#define ETH_P_PPP_MP    0x0008          /* Dummy type for PPP MP frames */
#define ETH_P_LOCALTALK 0x0009      /* Localtalk pseudo type    */
#define ETH_P_CAN   0x000C      /* Controller Area Network      */
#define ETH_P_PPPTALK   0x0010      /* Dummy type for Atalk over PPP*/
#define ETH_P_TR_802_2  0x0011      /* 802.2 frames         */
#define ETH_P_MOBITEX   0x0015      /* Mobitex (kaz@cafe.net)   */
#define ETH_P_CONTROL   0x0016      /* Card specific control frames */
#define ETH_P_IRDA  0x0017      /* Linux-IrDA           */
#define ETH_P_ECONET    0x0018      /* Acorn Econet         */
#define ETH_P_HDLC  0x0019      /* HDLC frames          */
#define ETH_P_ARCNET    0x001A      /* 1A for ArcNet :-)            */


/*
 * PPP
 */
#define ETH_PPP_IP          0x0021
#define ETH_PPP_VJ_COMP     0x002d
#define ETH_PPP_VJ_UNCOMP   0x002f


#define IP_P_TCP   0x6
#define IP_P_UDP   0x11
#define IP_P_GRE   0x2f
#define IP_P_SCTP  0x84


struct ethhdr {
    unsigned char   h_dest[ETH_ALEN];   /* destination eth addr */
    unsigned char   h_source[ETH_ALEN]; /* source ether addr    */
    uint16_t        h_proto;        /* packet type ID field */
} __attribute__((packed));
#define     PKT_ETHERNET_HEADER_LEN     sizeof(struct ethhdr)

struct  vlan_header {
    uint8_t     ether_dhost[6];
    uint8_t     ether_shost[6];
    uint16_t    ether_type;
    uint16_t    vlan_tci;
    uint16_t    vlan_entype;
} __attribute__((packed));

struct vlan_ethhdr {
   unsigned char    h_dest[ETH_ALEN];      /* destination eth addr  */
   unsigned char    h_source[ETH_ALEN];    /* source ether addr */
   uint16_t               h_vlan_proto;              /* Should always be 0x8100 */
   uint16_t               h_vlan_TCI;                /* Encapsulates priority and VLAN ID */
   uint16_t     h_vlan_encapsulated_proto; /* packet type ID field (or len) */
}__attribute__((packed));
#endif

#ifndef _LINUX_TCP_H
struct tcphdr {
    uint16_t    source;
    uint16_t    dest;
    uint32_t    seq;
    uint32_t    ack_seq;

    uint16_t    doff:4,
        res1:4,
        cwr:1,
        ece:1,
        urg:1,
        ack:1,
        psh:1,
        rst:1,
        syn:1,
        fin:1;
#define TH_FIN  0x01
#define TH_SYN  0x02
#define TH_RST  0x04
#define TH_PUSH 0x08
#define TH_ACK  0x10
#define TH_URG  0x20

        uint16_t    window;
        uint16_t    check;
        uint16_t    urg_ptr;
};
#define     PKT_TCP_HEADER_LEN     sizeof(struct tcphdr)
#endif


#if !defined(_LINUX_IP_H) && !defined(__NETINET_IP_H)
struct iphdr {
#if defined(__LITTLE_ENDIAN_BITFIELD)
    uint8_t         ihl:4,
                version:4;
#elif defined (__BIG_ENDIAN_BITFIELD)
    uint8_t     version:4,
                ihl:4;
#else
uint8_t         version:4,
                ihl:4;
#endif
    uint8_t     tos;
    uint16_t    tot_len;
    uint16_t    id;
    uint16_t    rflag:1;
    uint16_t    dflag:1;
    uint16_t    mflag:1;
    uint16_t    frag_offset:13;
    uint8_t     ttl;
    uint8_t     protocol;
    uint16_t    check;
    uint32_t    saddr;
    uint32_t    daddr;
    /*The options start here. */
};
#endif

#ifndef _LINUX_UDP_H
struct udphdr {
    uint16_t    source;
    uint16_t    dest;
    uint16_t    len;
    uint16_t    check;
};
//#define     PKT_UDP_HEADER_LEN     sizeof(struct udphdr)
#define     PKT_UDP_HEADER_LEN     8
#endif

#ifndef _LINUX_IN6_H

/*
 *      IPv6 address structure
 */

struct in6_addr {
        union {
                uint8_t           u6_addr8[16];
                uint16_t          u6_addr16[8];
                uint16_t          u6_addr32[4];
        } in6_u;
#define s6_addr                 in6_u.u6_addr8
#define s6_addr16               in6_u.u6_addr16
#define s6_addr32               in6_u.u6_addr32
};

#endif /* end of _LINUX_IN6_H*/

#ifndef _IPV6_H
/*
 *      IPv6 fixed header
 *
 *      BEWARE, it is incorrect. The first 4 bits of flow_lbl
 *      are glued to priority now, forming "class".
 */

struct ipv6hdr {
#if defined(__LITTLE_ENDIAN_BITFIELD)
        uint8_t                    priority:4,
                                version:4;
#elif defined(__BIG_ENDIAN_BITFIELD)
        uint8_t                    version:4,
                                priority:4;
#else
        uint8_t                    version:4,
                                priority:4;
#endif
        uint8_t                    flow_lbl[3];

        uint16_t                  payload_len;
        uint8_t                    nexthdr;
        uint8_t                    hop_limit;

        struct  in6_addr        saddr;
        struct  in6_addr        daddr;
};

#endif
#endif

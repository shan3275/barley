#ifndef __FRCORE_SSN_H__
#define __FRCORE_SSN_H__
#include "frcore_tcpflowrec.h"
#include "frcore_init.h"
#include "cvmx-atomic.h"
#include "frc_dma.h"

#if FRC_CONFIG_SSN_CHAN
#define SSN_HASH_BUCKET_SIZE_BITS    18          /* 262144          */
#define SSN_HASH_BUCKET_SIZE         (1<<SSN_HASH_BUCKET_SIZE_BITS)
#define SSN_HASH_BUCKET_SIZE_MASK    (SSN_HASH_BUCKET_SIZE-1)
#define MC_FPA_SSN_BUF_NUMS          (300000)     /* 4K*128=512k extra */
//#define SSN_TOTAL_NUM                (SSN_HASH_BUCKET_SIZE+MC_FPA_SSN_BUF_NUMS)
#define SSN_TOTAL_NUM                (300000)

#define CVMX_SSN_CACHE_LINE_SIZE         (256)
#define CVMX_SSN_CACHE_LINE_ALIGNED __attribute__ ((aligned (CVMX_SSN_CACHE_LINE_SIZE)))

#define CVMX_FPA_SSN_POOL                   (5)
#define CVMX_FPA_SSN_POOL_SIZE              CVMX_SSN_CACHE_LINE_SIZE
#define CVMX_FPA_TCPFLOWREC_POOL            (6)             /**< for tcp flow recovery */
#define CVMX_FPA_TCPFLOWREC_POOL_SIZE       CVMX_CACHE_LINE_SIZE

#define QOS_INVALID_ID  -1
#define MPP_QOS_ID_MAX_NUM  65536

extern CVMX_SHARED cvmx_spinlock_t  ssn_spinlock;


typedef struct mpp_ssn_cont {
    union {
        struct {
            /*word 6*/
            uint32_t        new_sip;    /*new source ip if mod_sip=1 */
            uint32_t        new_dip;    /*new dest ip if mod_dip=1  */
            /*word 7*/
            uint16_t        new_sp; /*new dest port if mod_dp=1 */
            uint16_t        smac_hi;
            uint32_t        smac_lo;


            /*word 8*/
            uint16_t        new_dp;     /*new src port if mod_sp=1  */
            uint16_t        dmac_hi;    /*new dest mac if mod_dmac=1 */
            uint32_t        dmac_lo;    /*new dest mac if mod_dmac=1 */

            /*word 9*/
            uint64_t        new_tos     : 8;
            uint64_t    syncookie_svr:1;
            uint64_t        unused      : 21;
            uint64_t    qos1_en     : 1;
            uint64_t    qos2_en     : 1;
            uint64_t        qos1_id     : 16;
            uint64_t        qos2_id     : 16;
        };
        uint64_t data_64[4];
        uint32_t data_32[8];
    };
}mpp_ssn_cont;

typedef struct frc_ssn_submit_addr {
    union {
        struct {
            /*word 6*/
            uint64_t        block_addr;  /* submit block addr */
            /*word 7*/
            uint16_t        head_offset; /* submit head offset */
            uint16_t        info_offset; /* submit info offset */
            uint16_t        payload_offset; /* submit payload offset */
            uint16_t        addr_flag;      /* 1 for effective; 0 for uneffective */
            /*word 8*/
            uint64_t        payload_len;    /* for every block, need to know total payload length */

            /*word 9*/
            uint64_t        pkt_num;        /* for every block, need to know total pkt number */
        };
        uint64_t data_64[4];
        uint32_t data_32[8];
    };
}frc_ssn_submit_addr;

typedef struct mpp_ssn_key {
    union {
        struct {
            union {
                struct {
                    /* Don't change the five-tuple sequence unless you know what you are doing. */
                    /* word 0 */
                    uint64_t        sip         : 32;   /* source ip of the session */
                    uint64_t        dip         : 32;   /* dest ip of the session   */
                    /* word 1 */
#define     MPP_SSN_HASH_MASK_SNIFFER           0xffffffffff000000
#define     MPP_SSN_HASH_MASK_DATA64            0xffffffffffff0000
#define     MPP_SSN_HASH_PROTO_MASK_DATA64_3G       0xffffffff00ff1f00        //for finding ssn in 3g  session
                    uint64_t        sp          : 16;   /* source port of the session */
                    uint64_t        dp          : 16;   /* dest port of the session   */
                    uint64_t        proto       : 8;    /* protocol of the session    */
                    uint64_t        st          : 3;    /* session type, such as HTTP, 3G and so on*/
                    uint64_t        ipif        : 5;    /* ingress interface*/
                    uint64_t        rsv0        : 2;
                    uint64_t        copyif      : 5;  /* copy to interface    */
                    uint64_t        tcpflowrec  : 1;    /* tcp flow rec */
                    uint64_t        host        : 1;    /* action when ssn hit  */
                    uint64_t        forward     : 1;
                    uint64_t        mirror      : 1;
                    uint64_t        epif        : 5;

                };
#define MPP_SSN_ACTION_TCPFLOWREC   0x8
#define MPP_SSN_ACTION_HOST     0x4
#define MPP_SSN_ACTION_DROP     0x3
#define MPP_SSN_ACTION_FWD      0x2
#define MPP_SSN_ACTION_MIRROR   0x1

                struct {
                    /*word0*/
#define     MPP_SSN_HASH_MAC_MASK_DATA64    0xffffffff00000000
                    uint64_t        dmac    :48;
                    uint64_t        smac_hi    : 16;
                    /*word1*/
                    uint64_t        smac_lo    : 32;
                    uint64_t        proto       : 8;    /* protocol of the session    */
                    uint64_t        st          : 8;    /*session type, such as HTTP, 3G and so on*/
                    uint64_t        rsv0        : 2;
                    uint64_t        ipif        : 5;    /* ingress interface          */
                    uint64_t    action      : 4;
                    uint64_t        epif        : 5;    /* action when ssn hit        */
                };
            };
            /*word 2*/
#define     MPP_SSN_KEY_PKTS_COUNT_POS_BITS     24
#define     MPP_SSN_KEY_PKTS_COUNT_MASK     0xffffffffff
            uint64_t        elif        : 8;
            uint64_t        hijack_epif        : 8;
            uint64_t        mod_add_vlan    : 1;
            uint64_t        mod_dmac    : 1;
            uint64_t        mod_smac    : 1;
            uint64_t        mod_tos     : 1;
            uint64_t        mod_sip     : 1;
            uint64_t        mod_dip     : 1;
            uint64_t        mod_sp      : 1;
            uint64_t        mod_dp      : 1;
            uint64_t        packet_count: 40;   /* packets count of the session */
            /*word 3*/
#define     MPP_SSN_KEY_BYTES_COUNT_POS_BITS    16
#define     MPP_SSN_KEY_BYTES_COUNT_MASK        0xffffffffffff
            uint64_t        rsv_x       : 14;
            uint64_t        qos_prio    : 2;
            uint64_t        byte_count  : 48;   /* bytes count of the session  */

        };
        uint64_t data_64[4];
    };
}mpp_ssn_key_t;


typedef struct mpp_ssn {
    union {
        struct {
            /* word 0-1 */
            union{
                struct{
                    uint16_t                   hw_chksum;
                    uint8_t                    wqe_unused;
                    uint64_t                   next_ptr      :40;
                    uint64_t                   len           :16;
                    uint64_t                   ipprt         : 6;
                    uint64_t                   qos           : 3;
                    uint64_t                   grp           : 4;
                    cvmx_pow_tag_type_t        tag_type      : 3;
                    uint64_t                   tag           :32;
                };
                uint32_t hw_data[4];
            };

            /* word 2-5 */
            struct mpp_ssn_key  key;
            /* word 6-9 */
            struct mpp_ssn_cont cont;
            /* word 10-13 */
            struct frc_ssn_submit_addr ssn_addr;
            /* word 14 */
            //uint32_t              positive_flag; /* 1 for have positive packet; 0 for have no positive packet */
            //uint32_t              negative_flag; /* 1 for have nagative packet; 0 for have no negative packet*/
            uint32_t    pkts;    /* ssn pkt number */
            uint32_t    bytes;   /* ssn bytes number */
            /* word 15 */
            struct mpp_ssn      *next_sc;
#define SSN_STATUS_GOOD     0x00000001
#define SSN_STATUS_EMPTY    0x00000002
#define SSN_STATUS_EXPIRED  0x00000004
#define SSN_STATUS_HOST     0x00000008
#define SSN_STATUS_PEND     0x00000010
#define SSN_STATUS_ESTABLISHED      0x00000020
#define SSN_STATUS_WAIT     0x00000040
#define SSN_STATUS_MACHINE_MASK     \
~(SSN_STATUS_EMPTY|SSN_STATUS_EXPIRED|SSN_STATUS_HOST|(1<<31))

            /* word 16 */
            uint32_t            ssn_status;
            uint32_t            ssn_index;
            /* word 17 */
            int32_t             ttl;
            uint32_t            age;
            /* word 18*/
            uint32_t            seq; /* first packet seq */
            uint32_t            ack_seq; /* first packet ack seq */
            /* word 18-22 tcp flow recovery positive */
            tcp_flow_data       tcp_flow_positive_data;
            /* word 23-26 tcp flow recovery negative */
            tcp_flow_data       tcp_flow_negative_data;
        };
        uint64_t        data_64[32];
    };
}CVMX_SSN_CACHE_LINE_ALIGNED mpp_ssn;

#define FRCORE_SSN_LOOKUP_MAX 2
typedef struct frcore_ssn_stat{
    uint32_t sip;
    uint32_t dip;
    uint16_t sp;
    uint16_t dp;
    uint16_t  proto;
    uint16_t  rsvrd;
    uint32_t pkts;
    uint32_t bytes;
}frcore_ssn_stat_t;

typedef struct frcore_ssn_stats{
    uint64_t num;
    frcore_ssn_stat_t ssn_stat[FRCORE_SSN_LOOKUP_MAX];
}frcore_ssn_stats_t;

static inline int ssn_sort_tuple(struct mpp_tuple *src_tuple, struct mpp_tuple *des_tuple)
{
    des_tuple->data[0]=0;
    des_tuple->data[1]=0;
    if(src_tuple->data_32[0] >= src_tuple->data_32[1]) {
        des_tuple->data[0] = src_tuple->data[0];
        des_tuple->data_32[2] = src_tuple->data_32[2];
    }else{
        des_tuple->data_32[0] = src_tuple->data_32[1];
        des_tuple->data_32[1] = src_tuple->data_32[0];
        des_tuple->sp = src_tuple->dp;
        des_tuple->dp = src_tuple->sp;
    }

    des_tuple->proto = src_tuple->proto;

    return 0;
}

/* 0 for positive; 1 for negative */
static inline int ssn_direction_tuple(struct mpp_tuple *src_tuple, struct mpp_tuple *des_tuple)
{
    if(src_tuple->data[0] == des_tuple->data[0]
       && src_tuple->data_32[2] == des_tuple->data_32[2]) {
        return FRC_SSN_POSIVTE_FLOW;
    }
    return FRC_SSN_NEGATIVE_FLOW;
}

static inline uint32_t ssn_hash_by_tuple(uint64_t *data_64, uint32_t mask)
{

    int idx;
    uint64_t t1, t2;

    CVMX_MT_CRC_POLYNOMIAL (0x1edc6f41);
    CVMX_MT_CRC_IV (0);

    t1 = *data_64++;
    t2 = *data_64++;
    CVMX_MT_CRC_DWORD (t1);
    CVMX_MT_CRC_DWORD (t2);

    CVMX_MF_CRC_IV (idx);

    return (idx & mask);
}

static inline uint64_t ssn_set_tcpflowrec(struct mpp_ssn *ssn, int enable)
{
    uint64_t debug;

    //printk("key.data_64[1] 0x%08x ", ssn->key.data_64[1]);
    if(!enable){
        debug = cvmx_atomic_fetch_and_bclr64_nosync(&ssn->key.data_64[1],
                                        (0x1ul<<8));
    }
    else
        debug = cvmx_atomic_fetch_and_bset64_nosync(&ssn->key.data_64[1],
                                                    (0x1ul<<8));
    //printk("set_host 0x%lx\n", ssn->key.data_64[1]);
    return debug;
    /* not sure if */
    //CVMX_SYNCWS;
}

/*
* ssn of tcp packet status machine:
* pend---->establish---->wait---->good
*/
static inline int ssn_add(struct mpp_ssn *ssn)
{
    cvmx_atomic_fetch_and_bclr32_nosync(&ssn->ssn_status, SSN_STATUS_MACHINE_MASK);
    cvmx_atomic_fetch_and_bset32_nosync(&ssn->ssn_status, SSN_STATUS_GOOD);
    //md_fau_atomic_add64(CNTER_SSN_CONFIRMED, 1);
    return 0;
}

static inline int ssn_add_by_host(struct mpp_ssn *ssn)
{
    cvmx_atomic_fetch_and_bset32_nosync(&ssn->ssn_status, (SSN_STATUS_GOOD|SSN_STATUS_HOST));
    //md_fau_atomic_add64(CNTER_SSN_CONFIRMED, 1);
    return 0;
}

static inline int ssn_del(struct mpp_ssn *ssn)
{
    /* clear GOOD bit, set aging                              */
    /*    and let timer aging it                              */
    ssn->ttl = 1;
    cvmx_atomic_fetch_and_bclr32_nosync(&ssn->ssn_status,
                                            SSN_STATUS_GOOD);

    return 0;
}

static inline int ssn_host_ref(struct mpp_ssn *ssn)
{
    cvmx_atomic_fetch_and_bset32_nosync(&ssn->ssn_status,
                                            SSN_STATUS_HOST);

    return 0;
}

static inline int ssn_host_deref(struct mpp_ssn *ssn)
{
    cvmx_atomic_fetch_and_bclr32_nosync(&ssn->ssn_status,
                                            SSN_STATUS_HOST);

    return 0;
}

static inline int ssn_status_set_pend(struct mpp_ssn *ssn)
{
    cvmx_atomic_fetch_and_bclr32_nosync(&ssn->ssn_status, SSN_STATUS_MACHINE_MASK);
    cvmx_atomic_fetch_and_bset32_nosync(&ssn->ssn_status,
                                            SSN_STATUS_PEND);

    return 0;
}

static inline int ssn_status_unset_pend(struct mpp_ssn *ssn)
{
    cvmx_atomic_fetch_and_bclr32_nosync(&ssn->ssn_status,
                                            SSN_STATUS_PEND);

    return 0;
}

static inline int ssn_status_set_wait(struct mpp_ssn *ssn)
{
    cvmx_atomic_fetch_and_bclr32_nosync(&ssn->ssn_status, SSN_STATUS_MACHINE_MASK);
    cvmx_atomic_fetch_and_bset32_nosync(&ssn->ssn_status,
                                            SSN_STATUS_WAIT);

    return 0;
}

static inline int ssn_status_unset_wait(struct mpp_ssn *ssn)
{
    cvmx_atomic_fetch_and_bclr32_nosync(&ssn->ssn_status,
                                            SSN_STATUS_WAIT);

    return 0;
}


static inline int ssn_status_set_established(struct mpp_ssn *ssn)
{
    cvmx_atomic_fetch_and_bclr32_nosync(&ssn->ssn_status, SSN_STATUS_MACHINE_MASK);
    cvmx_atomic_fetch_and_bset32_nosync(&ssn->ssn_status,
                                            SSN_STATUS_ESTABLISHED);
    return 0;
}

static inline int ssn_status_unset_established(struct mpp_ssn *ssn)
{
    cvmx_atomic_fetch_and_bclr32_nosync(&ssn->ssn_status,
                                            SSN_STATUS_ESTABLISHED);
    return 0;
}

static inline uint64_t ssn_set_smac(struct mpp_ssn *ssn, uint64_t mac)
{
    cvmx_atomic_fetch_and_bclr64_nosync(&ssn->cont.data_64[1],
                                        0xfffffffffffful);

    return cvmx_atomic_fetch_and_bset64_nosync(&ssn->cont.data_64[1],
                                               mac&0xfffffffffffful);
    /* not sure if */
    //CVMX_SYNCWS;
}

static inline uint64_t ssn_set_dmac(struct mpp_ssn *ssn, uint64_t mac)
{
    cvmx_atomic_fetch_and_bclr64_nosync(&ssn->cont.data_64[2],
                                        0xfffffffffffful);

    return cvmx_atomic_fetch_and_bset64_nosync(&ssn->cont.data_64[2],
                                               mac&0xfffffffffffful);
}

int md_dump_ssn(struct mpp_ssn *ssn);
int frcore_set_ssn_age(uint8_t ssn_age);
int frcore_get_ssn_age(uint8_t *ssn_age);
int frcore_get_ssn_by_hash(uint32_t hash, frcore_ssn_stats_t *ssn_stats);
int frcore_get_ssn_by_tuple(uint32_t sip, uint32_t dip, uint16_t sp, uint16_t dp,
                            uint8_t proto, frcore_ssn_stat_t *ssn_stat);
int frcore_match_ssn_by_tuple(uint32_t sip, uint32_t dip, uint16_t sp, uint16_t dp, uint8_t proto);
struct mpp_ssn *ssn_prepare(uint32_t ssn_index, uint64_t *data, uint64_t mask, uint8_t th_flags,
                            uint32_t seq, uint32_t ack_seq);
int ssn_purge_ssn(struct mpp_ssn *ssn);

#endif /* end of frc_config_ssn_chan */
#endif

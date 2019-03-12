
#ifndef _FRCORE_ACL_H
#define _FRCORE_ACL_H

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


#include "frcore_rfc.h"

#define ACL_PROTOCOL_UDP        17
#define ACL_PROTOCOL_TCP        6
#define ACL_PROTOCOL_ANY        0

#define FRCORE_ACL_API_INTERFACE

typedef struct mpp_acl_filtset{
    struct FILTSET filtset;
    Pnode  phase0_Nodes[7];   // structure for Phase 0
#ifdef RFC3_DEBUG
    Pnoder  phase1_Nodes[2];
    Pnoder  phase2_Node;
    uint8_t dot[7];
    int indx0a ;
    int indx0b ;
    int index1a ;
#endif
#ifdef RFC4_DEBUG
    Pnoder phase1_Nodes[3];   // structure for Phase 1
    Pnoder phase2_Nodes[2];   // structure for Phase 2
    Pnoder phase3_Node;   // structure for Phase 3
#endif

}mpp_acl_filtset;

void LoadFilters(FILE *fp, struct FILTSET * filtset);
#ifndef FRCORE_ACL_API_INTERFACE
void acl_readiprange(char *fp,unsigned int* highRange,unsigned int* lowRange);
void acl_ReadProtocol(char *fp, unsigned int  *from, unsigned int  *to);
void acl_ReadPort(char *fp, unsigned int *from, unsigned int *to);
#else
void acl_readiprange(uint32_t ip,unsigned int* highRange,unsigned int* lowRange);
// Read protocol
// fp: pointer to filter
// protocol: 17 for tcp
// return: void
void acl_ReadProtocol(uint16_t proto, unsigned int  *from, unsigned int  *to);
// Read port
// fp: pointer to filter
// from:to  =>  0:65535 : specify the port range
// return: void
void acl_ReadPort(uint16_t port, unsigned int *from, unsigned int *to);
#endif
typedef struct mpp_aclip_hash{
    union{
        struct{
            uint32_t ip;        //ip addr
            uint32_t tag;   //the value is tagid of the ip
        };
        uint64_t data;
    };
}mpp_aclip_hash;

typedef struct mpp_acl_ip{
    cvmx_spinlock_t lock;
    uint32_t src_cnt[MAX_ACL_IP_NUM];
    uint32_t dest_cnt[MAX_ACL_IP_NUM];
    mpp_aclip_hash srcip[MAX_ACL_IP_NUM][MAX_ACL_IP_HASH_BUCKET_LEN];       //for look up hash
    mpp_aclip_hash destip[MAX_ACL_IP_NUM][MAX_ACL_IP_HASH_BUCKET_LEN];
    uint32_t tagip[MAX_ACL_IP_NUM];     //for config and show
    #define FLAG_SRC_IP 1
    #define FLAG_DEST_IP 2
    uint32_t ipflag[MAX_ACL_IP_NUM];        //flag ; 1:src ip, 2:dest ip
}mpp_acl_ip;

typedef struct mpp_match_group{
        int16_t count;
        int16_t sm[MPP_MAX_ACSM_NUM];
}mpp_match_group;
typedef struct mpp_acl{
    mpp_acl_filtset acl_data[MPP_MAX_ACL_NUM];
//  int16_t acl_sm[MPP_MAX_ACL_NUM][MPP_MAX_FILTERS];
    mpp_match_group acl_sm[MPP_MAX_ACL_NUM][MPP_MAX_FILTERS];
    mpp_acl_ip acl_ip[MPP_MAX_ACL_NUM];
}mpp_acl;

uint8_t get_ssn_state();
uint8_t get_acl_state();
uint16_t acl_getfilter_count(uint8_t gid);
uint8_t acl_getfilter_state(uint8_t gid, uint16_t tagid);
int  acl_getfilter_ruleid_by_tagid(uint8_t gid, uint16_t tagid);
inline uint16_t acl_get_tagid_by_ruleid(uint8_t aclgid, uint16_t ruleid);
void acl_GetOneFilter_Cmd(uint8_t gid, uint16_t ruleid, char *sip, char *dip, char *sp, char *dp, char *protocol);
#ifndef FRCORE_ACL_API_INTERFACE
int acl_getfilter_by_ruleid(uint8_t gid, uint16_t ruleid, int isdump, uint16_t *tagid, uint8_t *state, uint8_t *sync, char *sip, char *dip, char *sp, char *dp, char *protocol);
int acl_getfilter_by_tagid(uint8_t gid, uint16_t tagid, uint16_t *rulenum,uint8_t *state, uint8_t *sync,
                           char *sip, char *dip, char *sp, char *dp, char *protocol);
#else
int acl_getfilter_by_tagid(uint8_t gid, uint16_t tagid, uint16_t *rulenum,uint8_t *state, uint8_t *sync,
                           uint32_t *sip, uint32_t *dip, uint16_t *sp, uint16_t *dp, uint16_t *protocol,
                           uint32_t *pkts, uint32_t *bytes, uint16_t *op, uint16_t *action, uint16_t *rule_type,
                           uint16_t *rule_source);
void acl_initfilter_statistics_by_tagid(uint8_t gid, uint16_t tagid);
#endif
uint8_t acl_enable_one_filter_by_tagid(uint8_t gid, uint16_t targetid, uint8_t enable);
uint8_t acl_enable_all_filter(uint8_t gid, uint8_t enable);

int acl_initOneFilter_by_index(uint8_t gid, int index);
void acl_init_one_acl_sm();
void acl_init();
int acl_DeleteOneFilter_by_tagid(uint8_t gid, uint16_t tagid);
int acl_updatefiltertagid_by_stagid(uint8_t gid, uint16_t stagid, uint16_t dtagid);
#ifndef FRCORE_ACL_API_INTERFACE
void acl_setfilter(uint8_t gid, uint16_t targetid, char *sip, char *dip, char *sp, char *dp, char *protocol);
#else
void acl_setfilter(uint8_t gid, uint16_t targetid, uint32_t sip, uint32_t dip, uint16_t sp, uint16_t dp, uint16_t protocol,
                       uint16_t op, uint16_t action, uint16_t rule_type, uint16_t rule_source);
#endif
int pre_acl_lookup_by_tuple(uint8_t aclgid);
#ifdef RFC3_DEBUG
unsigned short acl_lookup_by_tuple(Pnode *phase0_Nodes, Pnoder *phase1_Nodes, Pnoder *phase2_Node, uint8_t *dot, uint16_t siph, uint16_t sipl, uint16_t diph, uint16_t dipl, uint16_t sp, uint16_t dp ,uint8_t proto);
#endif
#ifdef RFC4_DEBUG
uint16_t acl_lookup_by_tuple(Pnode *phase0_Nodes, Pnoder *phase1_Nodes, Pnoder *phase2_Nodes, Pnoder *phase3_Node,  uint16_t siph, uint16_t sipl, uint16_t diph, uint16_t dipl, uint16_t sp, uint16_t dp ,uint8_t proto);
#endif

static inline void acl_statistic_hit(uint8_t aclgid, int32_t tagid)
{
    cvmx_atomic_fetch_and_add64(&gsdata.acl->acl_data[aclgid].filtset.pkt_num[tagid], 1);
}
static inline void acl_init_statistic_hit(uint8_t aclgid, int32_t tagid)
{
    cvmx_atomic_set64(&gsdata.acl->acl_data[aclgid].filtset.pkt_num[tagid], 0);
}
static inline uint64_t acl_get_statistic_hit(uint8_t aclgid, int32_t tagid)
{
    return cvmx_atomic_fetch_and_add64(&gsdata.acl->acl_data[aclgid].filtset.pkt_num[tagid], 0);
}

static inline void acl_pkt_bytes_add(uint8_t aclgid, int32_t tagid, uint16_t pkt_len)
{
    cvmx_atomic_fetch_and_add64(&gsdata.acl->acl_data[aclgid].filtset.pkt_bytes[tagid], pkt_len);
}
static inline void acl_init_pkt_bytes(uint8_t aclgid, int32_t tagid)
{
    cvmx_atomic_set64(&gsdata.acl->acl_data[aclgid].filtset.pkt_bytes[tagid], 0);
}
static inline uint64_t acl_get_pkt_bytes(uint8_t aclgid, int32_t tagid)
{
    return cvmx_atomic_fetch_and_add64(&gsdata.acl->acl_data[aclgid].filtset.pkt_bytes[tagid], 0);
}
//void ReadFilterFile_test(struct FILTSET *filtset);
//void Lookup_test(Pnode *phase0_Nodes, Pnoder *phase1_Nodes, Pnoder *phase2_Node);


/*The following code is for filter  only by srcip or destip*/
static inline uint32_t mpp_acl_ip_hash_crc(uint32_t ip){
    int idx;
    CVMX_MT_CRC_POLYNOMIAL (0x1edc6f41);
    CVMX_MT_CRC_IV (0);
    CVMX_MT_CRC_WORD (ip);
    CVMX_MF_CRC_IV (idx);
    return idx&MAX_ACL_IP_NUM_MASK;
}
/*
* compare source ip
* return 0: finded
*/
static inline int mpp_cmp_src_ip(uint8_t gid, uint32_t ip, uint32_t *tag){
    int i;
    uint32_t index = mpp_acl_ip_hash_crc(ip);
    for(i=0; i<MAX_ACL_IP_HASH_BUCKET_LEN; i++) {
        if(cvmx_likely(gsdata.acl->acl_ip[gid].srcip[index][i].ip==ip)) {
            MC_PRINTF_DEBUG("successed to find the source ip\n");
            *tag = gsdata.acl->acl_ip[gid].srcip[index][i].tag;
            return 0;
        }
    }
    return -1;
}

/*
* compare destination ip
* return 0: finded
*/
static inline int mpp_cmp_dest_ip(uint8_t gid, uint32_t ip, uint32_t *tag){
    int i;
    uint32_t index = mpp_acl_ip_hash_crc(ip);
    for(i=0; i<MAX_ACL_IP_HASH_BUCKET_LEN; i++) {
        if(cvmx_likely(gsdata.acl->acl_ip[gid].destip[index][i].ip==ip)) {
            MC_PRINTF_DEBUG("successed to find the destination ip\n");
            *tag = gsdata.acl->acl_ip[gid].destip[index][i].tag;
            return 0;
        }
    }
    return -1;
}

/*
* insert source ip to srcip hash bucket
*/
static inline int mpp_insert_src_ip(uint8_t gid, uint32_t tagid, uint32_t ip){
    int i;
    uint32_t index = mpp_acl_ip_hash_crc(ip);
    cvmx_spinlock_lock(&gsdata.acl->acl_ip[gid].lock);
    for(i=0; i<MAX_ACL_IP_HASH_BUCKET_LEN; i++) {
        if(cvmx_likely(gsdata.acl->acl_ip[gid].srcip[index][i].ip==0)) {
            gsdata.acl->acl_ip[gid].tagip[tagid] = ip;
            gsdata.acl->acl_ip[gid].ipflag[tagid] = FLAG_SRC_IP;
            gsdata.acl->acl_ip[gid].srcip[index][i].ip = ip;
            gsdata.acl->acl_ip[gid].srcip[index][i].tag = tagid+MPP_ACL_IP_START;
            gsdata.acl->acl_ip[gid].src_cnt[index]++;
            cvmx_spinlock_unlock(&gsdata.acl->acl_ip[gid].lock);
            MC_PRINTF_DEBUG("successed to insert new destn ip in the destip bucket\n");
            return  0;
        }else if(gsdata.acl->acl_ip[gid].srcip[index][i].ip==ip){
            cvmx_spinlock_unlock(&gsdata.acl->acl_ip[gid].lock);
             //MC_PRINTF_DEBUG("the ip is in the destip bucket\n");
             return -1;
        }
    }

    i=gsdata.acl->acl_ip[gid].src_cnt[index]%MAX_ACL_IP_HASH_BUCKET_LEN;
    gsdata.acl->acl_ip[gid].tagip[tagid] = ip;
    gsdata.acl->acl_ip[gid].ipflag[tagid] = FLAG_SRC_IP;
    gsdata.acl->acl_ip[gid].srcip[index][i].ip = ip;
    gsdata.acl->acl_ip[gid].srcip[index][i].tag = tagid+MPP_ACL_IP_START;
    gsdata.acl->acl_ip[gid].src_cnt[index]++;
    cvmx_spinlock_unlock(&gsdata.acl->acl_ip[gid].lock);
    //md_fau_atomic_add64(CNTER_ACL_SRC_IP_FULL, 1);

    return 0;
}

/*
* insert dest ip to DEST IP hash bucket
*/

static inline int mpp_insert_dest_ip(uint8_t gid, uint32_t tagid, uint32_t ip){
    int i;
    uint32_t index = mpp_acl_ip_hash_crc(ip);
    cvmx_spinlock_lock(&gsdata.acl->acl_ip[gid].lock);
    for(i=0; i<MAX_ACL_IP_HASH_BUCKET_LEN; i++) {
        if(gsdata.acl->acl_ip[gid].destip[index][i].ip==0) {
            gsdata.acl->acl_ip[gid].tagip[tagid] = ip;
            gsdata.acl->acl_ip[gid].ipflag[tagid] = FLAG_DEST_IP;
            gsdata.acl->acl_ip[gid].destip[index][i].ip = ip;
            gsdata.acl->acl_ip[gid].destip[index][i].tag = tagid+MPP_ACL_IP_START;
            gsdata.acl->acl_ip[gid].dest_cnt[index]++;
            cvmx_spinlock_unlock(&gsdata.acl->acl_ip[gid].lock);
            MC_PRINTF_DEBUG("successed to insert new src ip in the srcip bucket\n");
            return 0;
        }else if(gsdata.acl->acl_ip[gid].destip[index][i].ip==ip){
            cvmx_spinlock_unlock(&gsdata.acl->acl_ip[gid].lock);
             MC_PRINTF_DEBUG("the ip is in the src ip bucket\n");
             return -1;
        }
    }

    i = gsdata.acl->acl_ip[gid].dest_cnt[index]%MAX_ACL_IP_HASH_BUCKET_LEN;
    gsdata.acl->acl_ip[gid].tagip[tagid] = ip;
    gsdata.acl->acl_ip[gid].ipflag[tagid] = FLAG_DEST_IP;
    gsdata.acl->acl_ip[gid].destip[index][i].ip = ip;
    gsdata.acl->acl_ip[gid].destip[index][i].tag = tagid+MPP_ACL_IP_START;
    gsdata.acl->acl_ip[gid].dest_cnt[index]++;
    cvmx_spinlock_unlock(&gsdata.acl->acl_ip[gid].lock);
    //MC_PRINTF_WARN("SRC IP bucket %d is full\n", index);

    //md_fau_atomic_add64(CNTER_ACL_DEST_IP_FULL, 1);

    return 0;
}

static inline void mpp_del_src_ip(uint8_t gid, uint32_t tagid, uint32_t ip){
    int i;
    uint32_t index = mpp_acl_ip_hash_crc(ip);
    cvmx_spinlock_lock(&gsdata.acl->acl_ip[gid].lock);
    for(i=0; i<MAX_ACL_IP_HASH_BUCKET_LEN; i++) {
        if(cvmx_likely(gsdata.acl->acl_ip[gid].srcip[index][i].ip==ip)){
            gsdata.acl->acl_ip[gid].tagip[tagid] = 0;
            gsdata.acl->acl_ip[gid].ipflag[tagid] = 0;
            gsdata.acl->acl_ip[gid].srcip[index][i].data = 0;
            gsdata.acl->acl_ip[gid].srcip[index][i].tag = 0;
            gsdata.acl->acl_ip[gid].src_cnt[index]--;
            cvmx_spinlock_unlock(&gsdata.acl->acl_ip[gid].lock);
             MC_PRINTF_DEBUG("the ip is in the srcip bucket, delete it\n");
             return;
        }
    }
    MC_PRINTF_INFO("the ip isn't in the srcip bucket\n");
    cvmx_spinlock_unlock(&gsdata.acl->acl_ip[gid].lock);
    return;
}

static inline void mpp_del_dest_ip(uint8_t gid, uint32_t tagid, uint32_t ip){
    int i;
    uint32_t index = mpp_acl_ip_hash_crc(ip);
    cvmx_spinlock_lock(&gsdata.acl->acl_ip[gid].lock);
    for(i=0; i<MAX_ACL_IP_HASH_BUCKET_LEN; i++) {
        if(cvmx_likely(gsdata.acl->acl_ip[gid].destip[index][i].ip==ip)){
            gsdata.acl->acl_ip[gid].tagip[tagid] = 0;
            gsdata.acl->acl_ip[gid].ipflag[tagid] = 0;
            gsdata.acl->acl_ip[gid].destip[index][i].data = 0;
            gsdata.acl->acl_ip[gid].destip[index][i].tag = 0;
            gsdata.acl->acl_ip[gid].dest_cnt[index]--;
            cvmx_spinlock_unlock(&gsdata.acl->acl_ip[gid].lock);
             MC_PRINTF_DEBUG("the ip is in the destip bucket, delete it\n");
             return;
        }
    }
    MC_PRINTF_INFO("the ip isn't in the destip bucket\n");
    cvmx_spinlock_unlock(&gsdata.acl->acl_ip[gid].lock);
    return;
}

static inline uint32_t acl_ip_get_flag(uint8_t gid, uint32_t tagid){
    return gsdata.acl->acl_ip[gid].ipflag[tagid];
}

static inline uint32_t acl_ip_get(uint8_t gid, uint32_t tagid){
    return gsdata.acl->acl_ip[gid].tagip[tagid];
}

static inline void mpp_del_ip(uint8_t gid, uint32_t tagid){
    uint32_t ip;
    if(tagid>=MAX_ACL_IP_NUM) return;
    if((ip=gsdata.acl->acl_ip[gid].tagip[tagid])==0)  return;

    uint32_t flag=gsdata.acl->acl_ip[gid].ipflag[tagid];
    switch(flag) {
    case FLAG_SRC_IP:
        mpp_del_src_ip(gid, tagid, ip);
        break;
    case FLAG_DEST_IP:
        mpp_del_dest_ip(gid, tagid, ip);
        break;
    default:
        break;
    }
    return;
}

#endif

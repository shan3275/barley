
/* $Id: mp_proc.c 77 2011-05-29 14:27:29Z roccen $ */

/**
 * Semptian Multicore Packet Parallelling Source Code
 *
 * Copyright (c) 2010,2011 Semptian Technologies.
 * All rights reserved.
 *
 * This file contains proprietary and confidential information of
 * Semptian Technologes.
 *
 * Any licensed reproduction, distribution, modification, or other use of
 * this file or the confidential information or patented inventions
 * embodied in this file is subject to your license agreement with Semptian
 * Technologies.
 *
 * All other use and disclosure is prohibited.
 */

/**
 * @file md_ssn.c
 * @brief This is the session processing procedure.
 *
 *
 * @author Chen Peng (roccen)
 * @bug No known bugs.
 */

#include "cvmx.h"
#include "cvmx-config.h"
#include "cvmx-sysinfo.h"
#include "cvmx-bootmem.h"
#include "cvmx-spinlock.h"
#include "cvmx-rwlock.h"
#include "cvmx-fpa.h"
#include "cvmx-ipd.h"
#include "cvmx-pko.h"
#include "cvmx-dfa.h"
#include "cvmx-gmx.h"
#include "cvmx-coremask.h"
#include "cvmx-helper.h"
#include "cvmx-tim.h"
#include "cvmx-interrupt.h"
#include "cvmx-pip.h"
#include "cvmx-pow.h"
#include "cvmx-fau.h"

#include "frcore.h"
#include "frcore_cmd.h"
#include "frcore_misc.h"
#include "frcore_phy.h"
#include "frcore_dma.h"
#include "frcore_chan.h"
#include "frcore_pkt.h"
#include "frcore_stat.h"
#include "frcore_ssn.h"
#include "frcore_debug.h"
#include "frcore_ssn_priv.h"
#include "frcore_proto.h"

CVMX_SHARED mpp_shared_data gsdata;
#if FRC_CONFIG_SSN_CHAN
CVMX_SHARED uint8_t frcore_ssn_age = 3; /* 3s */   /* ssn age time, default 3s */
CVMX_SHARED cvmx_spinlock_t  ssn_spinlock;
static inline int ssn_purge_ssn_nolock(struct mpp_ssn *ssn)
{
    struct mpp_ssn *ssn1=&gsdata.ssn[ssn->ssn_index];

    //md_fau_atomic_add64(CNTER_SSN_PURGE, 1);
    if(ssn == ssn1) {
        cvmx_atomic_fetch_and_bset32_nosync(&ssn->ssn_status, SSN_STATUS_EMPTY);
        CVMX_SYNCWS;
        goto ret;
    }

    while(ssn1) {
        CVMX_PREFETCH(ssn1->next_sc, 0);
        if(ssn1->next_sc == ssn) {
            /* found the pointer */
            break;
        }
        ssn1 = ssn1->next_sc;
    }

    if(!ssn1) {
        MC_PRINTF_ERR("Wrong ssn with index 0x%x\n", ssn->ssn_index);
        //md_fau_atomic_add64(CNTER_SSN_PURGE_UNKNOWN, 1);
    }else{
        ssn1->next_sc = ssn->next_sc;
#if 0
        printf("purge ssn index 0x%x 0x%x head %p->%p prev %p->%p this %p->%p \n",
               ssn->ssn_index, gsdata.ssn[ssn->ssn_index].ssn_index,
               &gsdata.ssn[ssn->ssn_index], gsdata.ssn[ssn->ssn_index].next_sc,
               ssn1, ssn1->next_sc, ssn, ssn->next_sc);
#endif
    }
    //md_fau_atomic_add64(CNTER_SSN_PURGE_TO_FPA, 1);
    FRCORE_STAT_INC(stat_ssn_purge_to_fpa);
    cvmx_fpa_free((void*)ssn, CVMX_FPA_SSN_POOL, 0);

ret:
    //md_fau_atomic_add64(CNTER_SSN_ACTIVE_NUM, -1);
    return 0;
}

int ssn_purge_ssn(struct mpp_ssn *ssn)
{
    struct mpp_ssn *ssn1=&gsdata.ssn[ssn->ssn_index];
    //uint32_t *s = &ssn1->ssn_status;
    //cvmx_spinlock_bit_lock(s);
    /* for debug */
    struct tseg_qent *q;
    if(ssn->tcp_flow_positive_data.head.lh_first) {
        q = LIST_FIRST(&ssn->tcp_flow_positive_data.head);
        printf("%s %d,q->ack_num=0x%llx, q->data_len=%lld\n", __func__, __LINE__, (ULL)q->ack_num, (ULL)q->data_len);
    }
    if(ssn->tcp_flow_negative_data.head.lh_first) {
        q = LIST_FIRST(&ssn->tcp_flow_negative_data.head);
        printf("%s %d,q->ack_num=0x%llx, q->data_len=%lld\n", __func__, __LINE__, (ULL)q->ack_num, (ULL)q->data_len);
    }
    FRCORE_STAT_INC(stat_ssn_purge);
    if(ssn == ssn1) {
        cvmx_atomic_fetch_and_bset32_nosync(&ssn->ssn_status, SSN_STATUS_EMPTY);
        cvmx_atomic_fetch_and_bclr32_nosync(&ssn->ssn_status, SSN_STATUS_GOOD);
        /* sbumit dma head */
         //FRCORE_CYCLE_RECORDING();
        sbumit_dma_header_to_fifo(ssn);
         //FRCORE_CYCLE_RECORDING();
        goto ret;
    }

    //cvmx_spinlock_lock(&ssn_spinlock);
    while(ssn1) {
        CVMX_PREFETCH(ssn1->next_sc, 0);
        if(ssn1->next_sc == ssn) {
            /* found the pointer */
            break;
        }
        ssn1 = ssn1->next_sc;
    }

    if(!ssn1) {
        MC_PRINTF_ERR("Wrong ssn with index 0x%x. ssn1: %p\n", ssn->ssn_index, ssn1);
        //md_fau_atomic_add64(CNTER_SSN_PURGE_UNKNOWN, 1);
        FRCORE_STAT_INC(stat_ssn_purge_unknown);
    }else{
        ssn1->next_sc = ssn->next_sc;
        /* submit dma header */
        sbumit_dma_header_to_fifo(ssn);
#if 0
        printf("purge ssn index 0x%x 0x%x head %p->%p prev %p->%p this %p->%p \n",
               ssn->ssn_index, gsdata.ssn[ssn->ssn_index].ssn_index,
               &gsdata.ssn[ssn->ssn_index], gsdata.ssn[ssn->ssn_index].next_sc,
               ssn1, ssn1->next_sc, ssn, ssn->next_sc);
#endif
    }
    //cvmx_spinlock_unlock(&ssn_spinlock);
    //cvmx_spinlock_bit_unlock(s);
    //md_fau_atomic_add64(CNTER_SSN_PURGE_TO_FPA, 1);
    FRCORE_STAT_INC(stat_ssn_purge_to_fpa);
    if(!cvmx_fpa_is_member(CVMX_FPA_SSN_POOL, (void *)ssn)){
        //md_fau_atomic_add64(CNTER_FPA_FREE_ERR, 1);
        FRCORE_STAT_INC(stat_ssn_fpa_free_err);
        //cvmx_spinlock_bit_unlock(s);
        printf("ERROR PURGE SSN: %p   !!!\n", ssn1);
        return 0;
    }
    cvmx_fpa_free((void*)ssn, CVMX_FPA_SSN_POOL, 0);

ret:
    //cvmx_spinlock_bit_unlock(s);
    FRCORE_STAT_ADD(stat_ssn_active_num, -1);
    return 0;
}


struct mpp_ssn *ssn_prepare(uint32_t ssn_index, uint64_t *data, uint64_t mask, uint8_t th_flags,
                            uint32_t seq, uint32_t ack_seq)
{
    struct mpp_ssn *ssn1, *ssn;

    /* if first packet is FIN or RST packet, drop it */
    //printf("ssn_prepare th_flags:0x%x\n", th_flags);
    if ((th_flags & TH_FIN) || (th_flags & TH_RST))
    {
        FRCORE_STAT_INC(stat_ssn_fail_fin_or_rst);
        return NULL;
    }
    /* count total number*/
    if(cvmx_unlikely(FRCORE_STAT_FETCH_AND_ADD(stat_ssn_active_num, 1)
                     >= SSN_TOTAL_NUM)) {
        FRCORE_STAT_ADD(stat_ssn_active_num, -1);
        FRCORE_STAT_INC(stat_ssn_fail_overflow);
        return NULL;
    }

    ssn = &gsdata.ssn[ssn_index];
#if 0
    printf("Current ssn_index 0x%x, ssn status 0x%x\n",
           ssn_index,
           ssn->ssn_status);
    printf("tuple 0x%x 0x%x %d %d %d %2d %d\n",
           ((struct mpp_tuple*)data)->dip, ((struct mpp_tuple*)data)->sip,
           ((struct mpp_tuple*)data)->dp, ((struct mpp_tuple*)data)->sp,
           ((struct mpp_tuple*)data)->ipif, ((struct mpp_tuple*)data)->proto,
           ((struct mpp_tuple*)data)->session_type);
#endif
    /* not empty, new new ssn table entry */
    if(!(cvmx_atomic_fetch_and_bclr32_nosync(
        &ssn->ssn_status, SSN_STATUS_EMPTY)&SSN_STATUS_EMPTY)){
        ssn1 = (struct mpp_ssn *)cvmx_fpa_alloc(CVMX_FPA_SSN_POOL);
        //md_fau_atomic_add64(CNTER_SSN_PREPARED_BY_FPA_ALLOC, 1);
        FRCORE_STAT_INC(stat_ssn_prepared_by_fpa_alloc);
        if(cvmx_likely(ssn1)){
            MC_PRINTF_INFO("prepare: index: 0x%x, this %p, \n", ssn_index, ssn1);
#if 0
                printf("FPA: name: %s, base: %p, size:%d, num: %d\n",
                            cvmx_fpa_pool_info[CVMX_FPA_SSN_POOL].name,
                        cvmx_fpa_pool_info[CVMX_FPA_SSN_POOL].base,
                             cvmx_fpa_pool_info[CVMX_FPA_SSN_POOL].size,
                       cvmx_fpa_pool_info[CVMX_FPA_SSN_POOL].starting_element_count );
#endif
            if(!cvmx_fpa_is_member(CVMX_FPA_SSN_POOL, (void *)ssn1)){
               // md_fau_atomic_add64(CNTER_FPA_ALLOC_ERR, 1);
                FRCORE_STAT_ADD(stat_ssn_fpa_alloc_err,  1);
                MC_PRINTF_INFO("ERROR PREPARE SSN1: %p   !!!\n", ssn1);
                return NULL;
            }
            ssn1->ssn_status = SSN_STATUS_EMPTY;
            ssn1->ssn_addr.addr_flag = 0;
            ssn1->next_sc = ssn->next_sc;
            ssn->next_sc = ssn1;
#if 0
            printf("prepare ssn index 0x%x head %p->%p status 0x%xthis %p->%p \n",
                   ssn_index, ssn, ssn->next_sc, ssn->ssn_status, ssn1,
                   ssn1->next_sc);
#endif
        }else{
            //md_fau_atomic_add64(CNTER_SSN_FPA_FAILED, 1);
            FRCORE_STAT_ADD(stat_ssn_fpa_failed,  1);
            FRCORE_STAT_ADD(stat_ssn_active_num, -1);
            return NULL;
        }
    }else{
        /* empty, so you can put first ssn table entry here */
        //CVMX_SYNCWS;
        ssn1 = ssn;
        ssn1->ssn_addr.addr_flag = 0;
    }
    ssn1->seq = seq;
    ssn1->ack_seq = ack_seq;
    ssn1->bytes = 0;
    ssn1->pkts  = 0;
    ssn1->key.data_64[0] = data[0];
    ssn1->key.data_64[1] = data[1]&mask;
    ssn1->key.action = MPP_SSN_ACTION_TCPFLOWREC;
    ssn1->ttl = ssn1->age = frcore_ssn_age-1;
    ssn1->ssn_index = ssn_index;
    ssn1->qos = 1;
    ssn1->wqe_unused = FRCORE_WORK_SSN_AGE;
    ssn1->ipprt = 0;
    #if !FRC_CONFIG_SSN_ATOMIC
    ssn1->grp = GROUP_TO_DATA_PLANE_AGING;
    ssn1->tag = staging_tag(ssn_index,SESSION_STAGING);
    ssn1->tag_type = CVMX_POW_TAG_TYPE_ORDERED;
    #else
    ssn1->grp = cvmx_get_core_num() + 1;
    ssn1->tag = ssn1->grp;
    ssn1->tag_type = CVMX_POW_TAG_TYPE_ATOMIC;
    #endif
    /* add timer for this ssn*/
    if(ssn_add_timer(ssn1, MC_SSN_TTL_UNIT)<0)
    {
        MC_PRINTF_ERR("ssn add timer failed.\n");
        ssn_purge_ssn_nolock(ssn1);
        ssn1 = NULL;
    }
    return ssn1;
}


static char *_status[]=
{
    "GOOD",
    "EMPTY",
    "EXPIRED",
    NULL,
};

static char *_action[]=
{
    "NOACTION",
    "MIRROR",
    "FORWARD",
    "DROP",
    "HOST",
    NULL,
};

int md_dump_ssn(struct mpp_ssn *ssn)
{
    char *str="PREPARED";
    char *str1 = "NOACTION";
    int i;
    for(i=0; i<3; i++) {
        if(ssn->ssn_status & (1<<i))
            str = _status[i];
    }
    if((ssn->key.action>0)&&(ssn->key.action<5))
        str1 = _action[ssn->key.action];
    printf("ssn action: %d\n", ssn->key.action);

    printf("ssn %p: index 0x%x ttl %d status 0x%x (%s)\n",
             ssn, ssn->ssn_index, ssn->ttl, ssn->ssn_status, str);
    printf("  IF      SIP           DIP         PR      SP    DP \n"
                    "  ===================================================\n"
                    " %2d      0x%08x   0x%08x    %02d   %05d  %05d\n",
             ssn->key.ipif, ssn->key.sip, ssn->key.dip,
             ssn->key.proto, ssn->key.sp, ssn->key.dp);

    printf("  TYPE     EPIF    ACTION       BYTES       PACKETS  \n"
                    "  ===================================================\n"
                    "  %d        %2d      %s      %8d     %8d\n",
             ssn->key.st, ssn->key.epif, str1,
             (int)ssn->key.byte_count, (int)ssn->key.packet_count);

    printf("           DMAC                       SMAC           \n"
                    "  ===================================================\n"
                    "       %04x%08x                  %04x%08x            \n\n"
                    "     NEW_DIP         NEW_SIP         NEW_DP  NEW_SP  \n"
                    "  ===================================================\n"
                    "   0x%08x      0x%08x        %05d  %05d\n",
             ssn->cont.dmac_hi, ssn->cont.dmac_lo,
             ssn->cont.smac_hi, ssn->cont.smac_lo,
             ssn->cont.new_dip, ssn->cont.new_sip,
             ssn->cont.new_dp, ssn->cont.new_sp);


    return 0;

}
void ssn_count_ssn(int *cgood, int *cempty, int *cexpired)
{
    //struct mpp_ssn *ssn;
    int i;
    for(i = 0; i < SSN_HASH_BUCKET_SIZE; i++){
        if((gsdata.ssn[i].ssn_status & SSN_STATUS_GOOD) == SSN_STATUS_GOOD)
            (*cgood)++;
        if((gsdata.ssn[i].ssn_status & SSN_STATUS_EMPTY) == SSN_STATUS_EMPTY)
            (*cempty)++;
        if(gsdata.ssn[i].ssn_status == SSN_STATUS_EXPIRED)
            (*cexpired)++;

    }

}
int ssn_isgoodindex(int index)
{
    if(SSN_STATUS_GOOD == (SSN_STATUS_GOOD & gsdata.ssn[index].ssn_status))
        return 1;
    return 0;
}

int frcore_set_ssn_age(uint8_t ssn_age)
{
    if(ssn_age < FRCORE_SSN_AGE_MIN || ssn_age > FRCORE_SSN_AGE_MAX) {
        return FRE_EXCEED;
    }
    frcore_ssn_age = ssn_age;
    return FRE_SUCCESS;
}

int frcore_get_ssn_age(uint8_t *ssn_age)
{
    if(!ssn_age) {
        return FRE_MEMORY;
    }
    *ssn_age = frcore_ssn_age;
    return FRE_SUCCESS;
}

int frcore_get_ssn_by_hash(uint32_t hash, frcore_ssn_stats_t *ssn_stats)
{
    int i=0;
    uint32_t index = hash & SSN_HASH_BUCKET_SIZE_MASK;
    memset(ssn_stats, 0, sizeof(frcore_ssn_stats_t));
    struct mpp_ssn *ssn = &gsdata.ssn[index];
    if(ssn->ssn_status & SSN_STATUS_GOOD) {
        ssn_stats->ssn_stat[i].sip = ssn->key.sip;
        ssn_stats->ssn_stat[i].dip = ssn->key.dip;
        ssn_stats->ssn_stat[i].sp  = ssn->key.sp;
        ssn_stats->ssn_stat[i].dp  = ssn->key.dp;
        ssn_stats->ssn_stat[i].proto = ssn->key.proto;
        ssn_stats->ssn_stat[i].pkts  = ssn->pkts;
        ssn_stats->ssn_stat[i].bytes = ssn->bytes;
        ssn_stats->num++;
        i++;
    }
    ssn = ssn->next_sc;
    if(ssn) {
        if(ssn->ssn_status & SSN_STATUS_GOOD) {
            ssn_stats->ssn_stat[i].sip = ssn->key.sip;
            ssn_stats->ssn_stat[i].dip = ssn->key.dip;
            ssn_stats->ssn_stat[i].sp  = ssn->key.sp;
            ssn_stats->ssn_stat[i].dp  = ssn->key.dp;
            ssn_stats->ssn_stat[i].proto = ssn->key.proto;
            ssn_stats->ssn_stat[i].pkts  = ssn->pkts;
            ssn_stats->ssn_stat[i].bytes = ssn->bytes;
            ssn_stats->num++;
        }
    }
    return FRE_SUCCESS;
}

int frcore_get_ssn_by_tuple(uint32_t sip, uint32_t dip, uint16_t sp, uint16_t dp,
                            uint8_t proto, frcore_ssn_stat_t *ssn_stat)
{
    struct mpp_tuple tuple, tuple_sort;
    tuple.sip = sip;
    tuple.dip = dip;
    tuple.sp  = sp;
    tuple.dp  = dp;
    tuple.proto = proto;
    ssn_sort_tuple(&tuple, &tuple_sort);
    uint32_t ssn_index = ssn_hash_by_tuple(tuple_sort.data, SSN_HASH_BUCKET_SIZE_MASK);
    struct mpp_ssn *ssn = ssn_lookup_by_tuple(ssn_index, &tuple_sort, MPP_SSN_HASH_MASK_SNIFFER);
    if(ssn && (ssn->ssn_status & SSN_STATUS_GOOD)) {
        memset(ssn_stat, 0, sizeof(frcore_ssn_stat_t));
        ssn_stat->sip   = ssn->key.sip;
        ssn_stat->dip   = ssn->key.dip;
        ssn_stat->sp    = ssn->key.sp;
        ssn_stat->dp    = ssn->key.dp;
        ssn_stat->proto = ssn->key.proto;
        ssn_stat->pkts  = ssn->pkts;
        ssn_stat->bytes = ssn->bytes;
    }else {
        return FRE_NOTFOUND;
    }
    return FRE_SUCCESS;
}

int frcore_match_ssn_by_tuple(uint32_t sip, uint32_t dip, uint16_t sp, uint16_t dp, uint8_t proto)
{
    struct mpp_tuple tuple, tuple_sort;
    tuple.sip = sip;
    tuple.dip = dip;
    tuple.sp  = sp;
    tuple.dp  = dp;
    tuple.proto = proto;
    ssn_sort_tuple(&tuple, &tuple_sort);
    uint32_t ssn_index = ssn_hash_by_tuple(tuple_sort.data, SSN_HASH_BUCKET_SIZE_MASK);
    struct mpp_ssn *ssn = ssn_lookup_by_tuple(ssn_index, &tuple_sort, MPP_SSN_HASH_MASK_SNIFFER);
    if(ssn && (ssn->ssn_status & SSN_STATUS_GOOD)) {
        return FRE_SUCCESS;
    }else {
        return FRE_NOTFOUND;
    }
    return FRE_SUCCESS;
}

#endif /* end of FRC_CONFIG_SSN_CHAN */

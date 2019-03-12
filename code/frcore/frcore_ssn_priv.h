
#ifndef _MPP_SSN_PRIV_H
#define _MPP_SSN_PRIV_H

#include "frcore_config.h"
#include "frcore_ssn.h"

#if FRC_CONFIG_SSN_CHAN
/**
 * @brief lookup ssn by 5-tuple
 *
 * This function must be called under exclusive lock by mpp->ssn_index
 *
 * @param mpp
 *
 * @return struct mpp_ssn*
 */

static inline struct mpp_ssn * ssn_lookup_by_tuple(uint32_t index, struct mpp_tuple *tuple, uint64_t mask)
{
    struct mpp_ssn * ssn = &gsdata.ssn[index];

    if(!(ssn->ssn_status & SSN_STATUS_EMPTY)){/* not empty */
        MC_PRINTF_INFO("ssn status 0x%x index 0x%x\n",
                       ssn->ssn_status, index);
        if(((ssn->key.data_64[0]==tuple->data[0])&&((ssn->key.data_64[1]&mask)==(tuple->data[1]&mask)))
           || ((ssn->key.sip == tuple->dip) &&(ssn->key.dip == tuple->sip)
               && (ssn->key.sp == tuple->dp) &&(ssn->key.dp == tuple->sp)
               && (ssn->key.proto == tuple->proto))){
            MC_PRINTF_INFO("Find the ssn with index: 0x%x\n", index);
            MC_PRINTF_INFO("[0x%08x 0x%08x 0x%04x 0x%4x 0x%02x]\n",
                       ssn->key.sip, ssn->key.dip, ssn->key.sp,
                       ssn->key.dp, ssn->key.proto);
            goto done;
        }

        //else
            //printf("key: 0x%lx, 0x%lx\n", ssn->key.data_64[1], tuple->data[1]);
    }//else
        //printf("status: 0x%lx\n", ssn->ssn_status);


    ssn = ssn->next_sc;
    while(ssn) {
        CVMX_PREFETCH(ssn->next_sc, 0);
        if (((ssn->key.data_64[0]==tuple->data[0])&&(ssn->key.data_64[1]&mask)==(tuple->data[1]&mask))
            ||((ssn->key.sip == tuple->dip) &&(ssn->key.dip == tuple->sip)
               && (ssn->key.sp == tuple->dp) &&(ssn->key.dp == tuple->sp)
               && (ssn->key.proto == tuple->proto))) {
            MC_PRINTF_INFO("Find the ssn with index: 0x%x",
                       index);
            MC_PRINTF_INFO("[0x%08x 0x%08x 0x%04x 0x%4x 0x%02x]\n",
                       ssn->key.sip, ssn->key.dip, ssn->key.sp,
                       ssn->key.dp, ssn->key.proto);
            goto done;
        }
        ssn = ssn->next_sc;
    }
    MC_PRINTF_INFO("Not Find the ssn with index: 0x%x\n", index);
    return NULL;
done:
    return ssn;
}

static inline int ssn_add_timer(struct mpp_ssn *ssn, uint64_t ticks)
{
    MC_PRINTF_INFO("ssn add timer %p\n", ssn);
    if(cvmx_unlikely(cvmx_tim_add_entry((cvmx_wqe_t*)ssn, ticks, NULL))){
        //md_fau_atomic_add64(CNTER_SSN_TIM_FAILED, 1);
        FRCORE_STAT_INC(stat_ssn_tim_failed);
        return -1;
    }else{
        //md_fau_atomic_add64(CNTER_SSN_TIM_ADDOK, 1);
        FRCORE_STAT_INC(stat_ssn_tim_addok);
    return 0;
    }
}

static inline int ssn_execute_packet(cvmx_wqe_t *work,
                       struct mpp_ssn *ssn, struct mpp_control *mpp, uint8_t direction)
{
    if(!(ssn->ssn_status & SSN_STATUS_MACHINE_MASK)) {
        MC_PRINTF_ERR("ssn ssn_status 0x%x\n", ssn->ssn_status);
        return 1;
    }

    //struct iphdr *_iph = (struct iphdr*)(&mpp->packet[mpp->len_header]);

    MC_PRINTF_DEBUG("ssn->key.epif=%d, ssn->key.ipif=%d, ssn->index=0x%x\n", \
                   ssn->key.epif, ssn->key.ipif, ssn->ssn_index);

    ssn->ttl = ssn->age;
    CVMX_SYNCWS;

    switch(ssn->key.action){
    case MPP_SSN_ACTION_DROP:
        //mpp_free_wqe_packet_data(work);
        break;
    case MPP_SSN_ACTION_HOST:
        //host_packet(work,mpp);
        break;
    case MPP_SSN_ACTION_TCPFLOWREC:
        tcp_flow_recovery(work, ssn, mpp, direction);
        FRCORE_STAT_INC(stat_ssn_fwd_tcpflowrec);
        break;
    case MPP_SSN_ACTION_MIRROR:
        break;
    default:
        break;
    }
    return 0;
}

#endif /* end of FRC_CONFIG_SSN_CHAN */

#endif /*!_MPP_SSN_PRIV_H*/

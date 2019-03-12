
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <stddef.h>

#include <cvmx.h>
#include <cvmx-atomic.h>
#include "frcore_acl.h"
#include "frc_types.h"
#include "frcore_acl.h"

#define FILTERFIRST 0
#ifndef FRCORE_ACL_API_INTERFACE
static void getfileter(uint8_t gid, uint16_t ruleid,  char *sip, char *dip, char *sp, char *dp, char *protocol)
{
    struct FILTSET *filtset = &gsdata.acl->acl_data[gid].filtset;
    sprintf(sip, "%u.%u.%u.%u - %u.%u.%u.%u",
            (filtset->filtArr[ruleid].dim[0][0] >> 8) & 0xFF,
            filtset->filtArr[ruleid].dim[0][0] & 0xFF,
            (filtset->filtArr[ruleid].dim[1][0] >> 8) & 0xFF,
            filtset->filtArr[ruleid].dim[1][0] & 0xFF,
            (filtset->filtArr[ruleid].dim[0][1] >> 8) & 0xFF,
            filtset->filtArr[ruleid].dim[0][1] & 0xFF,
            (filtset->filtArr[ruleid].dim[1][1] >> 8) & 0xFF,
            filtset->filtArr[ruleid].dim[1][1] & 0xFF);
    sprintf(dip, "%u.%u.%u.%u - %u.%u.%u.%u",
            (filtset->filtArr[ruleid].dim[2][0] >> 8) & 0xFF,
            filtset->filtArr[ruleid].dim[2][0] & 0xFF,
            (filtset->filtArr[ruleid].dim[3][0] >> 8) & 0xFF,
            filtset->filtArr[ruleid].dim[3][0] & 0xFF,
            (filtset->filtArr[ruleid].dim[2][1] >> 8) & 0xFF,
            filtset->filtArr[ruleid].dim[2][1] & 0xFF,
            (filtset->filtArr[ruleid].dim[3][1] >> 8) & 0xFF,
            filtset->filtArr[ruleid].dim[3][1] & 0xFF);

    if(filtset->filtArr[ruleid].dim[4][0] == filtset->filtArr[ruleid].dim[4][1])
        sprintf(sp, "%u", filtset->filtArr[ruleid].dim[4][0]);
    else
        sprintf(sp, "%u-%u", filtset->filtArr[ruleid].dim[4][0], filtset->filtArr[ruleid].dim[4][1]);

    if(filtset->filtArr[ruleid].dim[5][0] == filtset->filtArr[ruleid].dim[5][1])
        sprintf(dp, "%u", filtset->filtArr[ruleid].dim[5][0]);
    else
        sprintf(dp, "%u-%u", filtset->filtArr[ruleid].dim[5][0], filtset->filtArr[ruleid].dim[5][1]);

    switch(filtset->filtArr[ruleid].dim[6][0]){
        case ACL_PROTOCOL_UDP:
            sprintf(protocol , "%s", "udp");
            break;
        case ACL_PROTOCOL_TCP:
            sprintf(protocol , "%s", "tcp");
            break;
        case ACL_PROTOCOL_ANY:// any 0 -255
            sprintf(protocol , "%s", "any");
            break;
        default:  //any
            sprintf(protocol , "%s", "any");
            break;

    }
}
#else
static void getfileter(uint8_t gid, uint16_t ruleid,
                       uint32_t *sip, uint32_t *dip, uint16_t *sp, uint16_t *dp, uint16_t *protocol)
{
    struct FILTSET *filtset = &gsdata.acl->acl_data[gid].filtset;
    uint32_t ip;
    uint16_t port, proto;
    /* sip */
    ip = ((filtset->filtArr[ruleid].dim[0][0] << 16) & 0xFFFF0000) |
        ((filtset->filtArr[ruleid].dim[1][0] ) & 0x0000FFFF);
    *sip = ip;
    /* dip */
    ip = ((filtset->filtArr[ruleid].dim[2][0] << 16) & 0xFFFF0000) |
        ((filtset->filtArr[ruleid].dim[3][0] ) & 0x0000FFFF);
    *dip = ip;
    /* sp */
    port = (filtset->filtArr[ruleid].dim[4][0] ) & 0x0000FFFF;
    *sp = port;
    /* dp */
    port = (filtset->filtArr[ruleid].dim[5][0] ) & 0x0000FFFF;
    *dp = port;
    /* protocol */
    proto = (filtset->filtArr[ruleid].dim[6][0] ) & 0x0000FFFF;
    *protocol = proto;
}
#endif
void acl_GetOneFilter_Cmd(uint8_t gid, uint16_t ruleid, char *sip, char *dip, char *sp, char *dp, char *protocol)
{
    uint8_t sipmask = 32, dipmask = 32;
    int i = 0;
    uint64_t index = 1;
    struct FILTSET *filtset = &gsdata.acl->acl_data[gid].filtset;

    uint32_t tmp = filtset->filtArr[ruleid].dim[0][1] * 65536 + filtset->filtArr[ruleid].dim[1][1] - (filtset->filtArr[ruleid].dim[0][0] * 65536 + filtset->filtArr[ruleid].dim[1][0]);
    for(i = 0; i <= 32; i++){
        if(tmp == ((index << i) - 1)){
            sipmask = 32 -i;
            break;
        }
    }
    sprintf(sip, "%u.%u.%u.%u/%u",
            (filtset->filtArr[ruleid].dim[0][0] >> 8) & 0xFF,
            filtset->filtArr[ruleid].dim[0][0] & 0xFF,
            (filtset->filtArr[ruleid].dim[1][0] >> 8) & 0xFF,
            filtset->filtArr[ruleid].dim[1][0] & 0xFF, sipmask);

    tmp = filtset->filtArr[ruleid].dim[2][1] * 65536 + filtset->filtArr[ruleid].dim[3][1] - (filtset->filtArr[ruleid].dim[2][0] * 65536 + filtset->filtArr[ruleid].dim[3][0]);
    for(i = 0; i <= 32; i++){
        if(tmp == ((index << i) - 1)){
            dipmask = 32 -i;
            break;
        }
    }
    sprintf(dip, "%u.%u.%u.%u/%u",
            (filtset->filtArr[ruleid].dim[2][0] >> 8) & 0xFF,
            filtset->filtArr[ruleid].dim[2][0] & 0xFF,
            (filtset->filtArr[ruleid].dim[3][0] >> 8) & 0xFF,
            filtset->filtArr[ruleid].dim[3][0] & 0xFF, dipmask);

    if(filtset->filtArr[ruleid].dim[4][0] == filtset->filtArr[ruleid].dim[4][1])
        sprintf(sp, "%u", filtset->filtArr[ruleid].dim[4][0]);
    else
        sprintf(sp, "%u-%u", filtset->filtArr[ruleid].dim[4][0], filtset->filtArr[ruleid].dim[4][1]);

    if(filtset->filtArr[ruleid].dim[5][0] == filtset->filtArr[ruleid].dim[5][1])
        sprintf(dp, "%u", filtset->filtArr[ruleid].dim[5][0]);
    else
        sprintf(dp, "%u-%u", filtset->filtArr[ruleid].dim[5][0], filtset->filtArr[ruleid].dim[5][1]);

    switch(filtset->filtArr[ruleid].dim[6][0]){
        case ACL_PROTOCOL_UDP:
            sprintf(protocol , "%s", "udp");
            break;
        case ACL_PROTOCOL_TCP:
            sprintf(protocol , "%s", "tcp");
            break;
        case ACL_PROTOCOL_ANY:// any 0 -255
            sprintf(protocol , "%s", "any");
            break;
        default:  //any
            sprintf(protocol , "%s", "any");
            break;

    }
}
static void swap_filt_statistics(struct FILTSET * filtset, int stag_id, int dtag_id)
{
    uint64_t tmp;
    tmp = filtset->pkt_num[stag_id];
    filtset->pkt_num[stag_id] = filtset->pkt_num[dtag_id];
    filtset->pkt_num[dtag_id] = tmp;
}
uint16_t acl_getfilter_count(uint8_t gid)
{
    struct FILTSET *filtset = &gsdata.acl->acl_data[gid].filtset;
    return filtset->numFilters;
}
uint8_t acl_getfilter_state(uint8_t gid, uint16_t targetid)
{
    struct FILTSET *filtset = &gsdata.acl->acl_data[gid].filtset;
    int i, count = filtset->numFilters;
    for(i = 0; i < count; i++){
        if(targetid == filtset->filtArr[i].cost)
            return filtset->filtArr[i].state;
    }
    return ACL_UNSET;
}
int  acl_getfilter_ruleid_by_tagid(uint8_t gid, uint16_t tagid)
{
    struct FILTSET *filtset = &gsdata.acl->acl_data[gid].filtset;
    int i, count = filtset->numFilters;
    for(i = 0; i < count; i++){
        if(tagid == filtset->filtArr[i].cost)
            return i;
    }
    return  MC_ERROR;
}
inline uint16_t acl_get_tagid_by_ruleid(uint8_t aclgid, uint16_t ruleid)
{
    return gsdata.acl->acl_data[aclgid].filtset.filtArr[ruleid].cost;
}
uint8_t acl_enable_one_filter_by_tagid(uint8_t gid, uint16_t targetid, uint8_t enable)
{
    struct FILTSET *filtset = &gsdata.acl->acl_data[gid].filtset;
    int i, count = filtset->numFilters;
    for(i = 0; i < count; i++){
        if(targetid == filtset->filtArr[i].cost){
            filtset->filtArr[i].state = enable;
            return 1;
        }
    }
    return 0;
}
uint8_t acl_enable_all_filter(uint8_t gid, uint8_t enable)
{
    struct FILTSET *filtset = &gsdata.acl->acl_data[gid].filtset;
    int i, count = filtset->numFilters;
    for(i = 0; i < count; i++){
        filtset->filtArr[i].state = enable;
    }
    return 1;
}

#ifndef FRCORE_ACL_API_INTERFACE
int acl_getfilter_by_ruleid(uint8_t gid, uint16_t ruleid, int isdump, uint16_t *tagid, uint8_t *state, uint8_t *sync, char *sip, char *dip, char *sp, char *dp, char *protocol){
    struct FILTSET *filtset = &gsdata.acl->acl_data[gid].filtset;
    //int  count = filtset->numFilters;

    if(ACL_UNSET == filtset->filtArr[ruleid].state)
        return 0;

    *tagid = filtset->filtArr[ruleid].cost;
    *state = filtset->filtArr[ruleid].state;
    *sync  = filtset->filtArr[ruleid].sync;
    if(isdump)
        acl_GetOneFilter_Cmd(gid, ruleid, sip, dip, sp, dp, protocol);
    else
        getfileter(gid, ruleid, sip, dip, sp, dp, protocol);

    return 1;
}
int acl_getfilter_by_tagid(uint8_t gid, uint16_t tagid, uint16_t *rulenum,uint8_t *state, uint8_t *sync,
                           char *sip, char *dip, char *sp, char *dp, char *protocol)
{
    struct FILTSET *filtset = &gsdata.acl->acl_data[gid].filtset;
    int i, count = filtset->numFilters;
    int ruleid = -1;
    for(i = 0; i < count; i++){
        if(tagid == filtset->filtArr[i].cost){
            ruleid = i;
            break;
        }
    }
    if(-1 == ruleid)
        return 0;

    *rulenum = ruleid;
    *state = filtset->filtArr[ruleid].state;
    *sync = filtset->filtArr[ruleid].sync;
    getfileter(gid, ruleid, sip, dip, sp, dp, protocol);
    return 1;
}
#else
int acl_getfilter_by_tagid(uint8_t gid, uint16_t tagid, uint16_t *rulenum,uint8_t *state, uint8_t *sync,
                           uint32_t *sip, uint32_t *dip, uint16_t *sp, uint16_t *dp, uint16_t *protocol,
                           uint32_t *pkts, uint32_t *bytes, uint16_t *op, uint16_t *action, uint16_t *rule_type,
                           uint16_t *rule_source)
{
    struct FILTSET *filtset = &gsdata.acl->acl_data[gid].filtset;
    int i, count = filtset->numFilters;
    int ruleid = -1;
    for(i = 0; i < count; i++){
        if(tagid == filtset->filtArr[i].cost){
            ruleid = i;
            break;
        }
    }
    if(-1 == ruleid)
        return FRE_FAIL;

    *rulenum = ruleid;
    *state = filtset->filtArr[ruleid].state;
    *sync = filtset->filtArr[ruleid].sync;
    *op = filtset->filtArr[ruleid].op;
    *action = filtset->filtArr[ruleid].action;
    *rule_type = filtset->filtArr[ruleid].rule_type;
    *rule_source = filtset->filtArr[ruleid].rule_source;
    *pkts = acl_get_statistic_hit(0, tagid);
    *bytes = acl_get_pkt_bytes(0, tagid);
    getfileter(gid, ruleid, sip, dip, sp, dp, protocol);
    return FRE_SUCCESS;
}

void acl_initfilter_statistics_by_tagid(uint8_t gid, uint16_t tagid)
{
    acl_init_statistic_hit(gid, tagid);
    acl_init_pkt_bytes(gid, tagid);
}
#endif

static int move_Forward_Filter_from_index(uint8_t gid, int index)
{
    struct FILTSET *filtset = &gsdata.acl->acl_data[gid].filtset;
    int j, m, count = filtset->numFilters;
    for(j = index; j < count - 1; j++){
        filtset->filtArr[j].state = filtset->filtArr[j+1].state;
        filtset->filtArr[j].sync = filtset->filtArr[j+1].sync;
        filtset->pkt_num[filtset->filtArr[j].cost] = filtset->pkt_num[filtset->filtArr[j+1].cost];
        filtset->pkt_bytes[filtset->filtArr[j].cost] = filtset->pkt_bytes[filtset->filtArr[j+1].cost];
    //  filtset->filtArr[j].count = filtset->filtArr[j+1].count;
        filtset->filtArr[j].cost = filtset->filtArr[j+1].cost;
        filtset->filtArr[j].op = filtset->filtArr[j+1].op;
        filtset->filtArr[j].action = filtset->filtArr[j+1].action;
        filtset->filtArr[j].rule_type = filtset->filtArr[j+1].rule_type;
        filtset->filtArr[j].rule_source = filtset->filtArr[j+1].rule_source;
        for(m = 0; m < 7; m++){
            filtset->filtArr[j].dim[m][0] = filtset->filtArr[j+1].dim[m][0];
            filtset->filtArr[j].dim[m][1] = filtset->filtArr[j+1].dim[m][1];
        }
    }
    acl_initOneFilter_by_index(gid, count - 1);
    return 1;

}
static int move_Back_Filter_from_index(uint8_t gid, int index)
{
    struct FILTSET *filtset = &gsdata.acl->acl_data[gid].filtset;
    int j, m, count = filtset->numFilters;
    for(j = count - 1; j >= index; j--){
        filtset->filtArr[j + 1].state = filtset->filtArr[j].state;
        filtset->filtArr[j + 1].sync = filtset->filtArr[j].sync;
        filtset->filtArr[j + 1].op = filtset->filtArr[j].op;
        filtset->filtArr[j + 1].action = filtset->filtArr[j].action;
        filtset->filtArr[j + 1].rule_type = filtset->filtArr[j].rule_type;
        filtset->filtArr[j + 1].rule_source = filtset->filtArr[j].rule_source;
//      filtset->filtArr[j + 1].count = filtset->filtArr[j].count;
        filtset->pkt_num[filtset->filtArr[j+1].cost] = filtset->pkt_num[filtset->filtArr[j].cost];
        filtset->pkt_bytes[filtset->filtArr[j+1].cost] = filtset->pkt_bytes[filtset->filtArr[j].cost];
        filtset->filtArr[j + 1].cost = filtset->filtArr[j].cost;
        for(m = 0; m < 7; m++){
            filtset->filtArr[j + 1].dim[m][0] = filtset->filtArr[j].dim[m][0];
            filtset->filtArr[j + 1].dim[m][1] = filtset->filtArr[j].dim[m][1];
        }
    }
    return 1;
}
int acl_initOneFilter_by_index(uint8_t gid, int index)
{
    struct FILTSET *filtset = &gsdata.acl->acl_data[gid].filtset;
    int m = 0;
    filtset->filtArr[index].state = ACL_UNSET;
    filtset->filtArr[index].sync = ACL_NOSYNC;
    filtset->pkt_num[filtset->filtArr[index].cost] = 0;
    filtset->pkt_bytes[filtset->filtArr[index].cost] = 0;
    filtset->filtArr[index].cost = 0;
    filtset->filtArr[index].op = 0;
    filtset->filtArr[index].action = 0;
    filtset->filtArr[index].rule_type = 0;
    filtset->filtArr[index].rule_source = 0;
//  filtset->filtArr[index].count = 0;
    for(m = 0; m < 7; m++){
        filtset->filtArr[index].dim[m][0] = 0;
        filtset->filtArr[index].dim[m][1] = 0;
    }
    return 1;

}

void acl_init_one_ipbucket(int gid, int index)
{
    int i=0;
    gsdata.acl->acl_ip[gid].src_cnt[index]=0;
    gsdata.acl->acl_ip[gid].dest_cnt[index]=0;
    gsdata.acl->acl_ip[gid].tagip[index]=0;
    gsdata.acl->acl_ip[gid].ipflag[index]=0;

    for(i=0; i<MAX_ACL_IP_HASH_BUCKET_LEN; i++) {
        gsdata.acl->acl_ip[gid].srcip[index][i].data=0;
        gsdata.acl->acl_ip[gid].destip[index][i].data=0;
    }
}

void acl_init_one_acl_sm(int gid, int tagid)
{
    int count = gsdata.acl->acl_sm[gid][tagid].count;
    gsdata.acl->acl_sm[gid][tagid].count = 0;
    while(count--){
        gsdata.acl->acl_sm[gid][tagid].sm[count] = MPP_ACL_SM_DEFAULT_VALUE;
    }
}
void acl_init()
{
    int i, j, tagid, gid;
    for(i = 0; i < MPP_MAX_ACL_NUM; i++){
        for(j = 0; j < MPP_MAX_FILTERS; j++){
            acl_init_one_acl_sm(i, j);
        }
        for(j = 0; j < MAX_ACL_IP_NUM; j++){
            acl_init_one_ipbucket(i, j);
        }
        acl_initOneFilter_by_index(i, MPP_MAX_FILTERS - 1);
    }

    for(gid = 0; gid < MPP_MAX_ACL_NUM; gid++){
        for(tagid = 0; tagid < MPP_MAX_FILTERS; tagid++){
                acl_DeleteOneFilter_by_tagid(gid, tagid);
        }
        for(tagid=0; tagid<MAX_ACL_IP_NUM; tagid++){
                mpp_del_ip(gid,tagid);
        }
    }
}

int acl_DeleteOneFilter_by_tagid(uint8_t gid, uint16_t tagid)
{
    struct FILTSET *filtset = &gsdata.acl->acl_data[gid].filtset;
    int i, count = filtset->numFilters;
    for(i = 0; i < count; i++){
        if(tagid == filtset->filtArr[i].cost){
            if(i == count -1)
                acl_initOneFilter_by_index(gid, i);
            else
                move_Forward_Filter_from_index(gid, i);
            filtset->numFilters--;
            return FRE_SUCCESS;
        }
    }
    return FRE_NOTFOUND;
}
static void copy_one_filter(struct FILTER   *filt, int gid, int index){
    struct FILTSET *filtset = &gsdata.acl->acl_data[gid].filtset;
    int m;

    filt->state = filtset->filtArr[index].state;
    filt->sync = filtset->filtArr[index].sync;
//  filt->count = filtset->filtArr[index].count;
    filt->cost =  filtset->filtArr[index].cost;
    for(m = 0; m < 7; m++){
        filt->dim[m][0] = filtset->filtArr[index].dim[m][0];
        filt->dim[m][1] = filtset->filtArr[index].dim[m][1];
    }
}
static void restore_one_filter(int gid, int index, struct FILTER *filt)
{
    struct FILTSET *filtset = &gsdata.acl->acl_data[gid].filtset;
    int m;

    filtset->filtArr[index].state = filt->state;
    filtset->filtArr[index].sync = filt->sync;
//  filtset->filtArr[index].count = filt->count;
    filtset->filtArr[index].cost = filt->cost;
    for(m = 0; m < 7; m++){
         filtset->filtArr[index].dim[m][0] = filt->dim[m][0];
         filtset->filtArr[index].dim[m][1] = filt->dim[m][1];
    }

}
static void cover_one_filter(int gid, int sindex, int dindex)
{
    struct FILTSET *filtset = &gsdata.acl->acl_data[gid].filtset;
    int  m;
    filtset->filtArr[sindex].state = filtset->filtArr[dindex].state;
    filtset->filtArr[sindex].sync = filtset->filtArr[dindex].sync;
//  filtset->filtArr[sindex].count = filtset->filtArr[dindex].count;
//  filtset->statistics[filtset->filtArr[sindex].cost] = filtset->statistics[filtset->filtArr[dindex].cost];
    filtset->filtArr[sindex].cost = filtset->filtArr[dindex].cost;
    for(m = 0; m < 7; m++){
        filtset->filtArr[sindex].dim[m][0] = filtset->filtArr[dindex].dim[m][0] ;
        filtset->filtArr[sindex].dim[m][1] = filtset->filtArr[dindex].dim[m][1];
    }
}
static int updatefiltertagid_by_stagid(uint8_t gid, uint16_t stagid, uint16_t dtagid)
{
    struct FILTSET *filtset = &gsdata.acl->acl_data[gid].filtset;
    int i, count = filtset->numFilters;
    int sindex = 0, dindex = 0;
    struct FILTER   filt;
    memset(&filt, 0, sizeof(struct FILTER));
    if(ACL_UNSET!= acl_getfilter_state(gid, stagid) && ACL_UNSET !=acl_getfilter_state(gid, dtagid)){
        for(i = 0; i < count; i++){
            if(stagid == filtset->filtArr[i].cost){
                copy_one_filter(&filt, gid, i);
                sindex = i;
            }
            if(dtagid == filtset->filtArr[i].cost){
                dindex = i;
            }
        }
        cover_one_filter(gid, sindex, dindex);
        restore_one_filter(gid, dindex, &filt);
        filtset->filtArr[sindex].cost = stagid;
        filtset->filtArr[dindex].cost = dtagid;
        swap_filt_statistics(filtset, stagid, dtagid);
        return 1;
    }
    return 0;
}
int acl_updatefiltertagid_by_stagid(uint8_t gid, uint16_t stagid, uint16_t dtagid)
{
    struct FILTSET *filtset = &gsdata.acl->acl_data[gid].filtset;
    int i, count = filtset->numFilters;
    int index = 0;
    struct FILTER   filt;
    memset(&filt,0, sizeof(struct FILTER));
    if(updatefiltertagid_by_stagid(gid, stagid, dtagid))
        return 1;

    for(i = 0; i < count; i++){
        if(stagid == filtset->filtArr[i].cost){
            copy_one_filter(&filt, gid, i);
            filt.cost = dtagid;

            if(i == count -1)
                acl_initOneFilter_by_index(gid, i);
            else
                move_Forward_Filter_from_index(gid, i);
            filtset->numFilters--;
            break;
        }
    }

    count = filtset->numFilters;
    for(i = 0; i < count; i++){
        if(dtagid < filtset->filtArr[i].cost){

                move_Back_Filter_from_index(gid, index);

                restore_one_filter(gid, i, &filt);

                filtset->numFilters++;

            return 1;
        }
    }
    restore_one_filter(gid, count, &filt);

    filtset->numFilters++;
    return 1;
}

/*
static int qsorting(struct FILTSET *filtset, ,int first,int end)
{
    int i = 0;
    struct FILTER   filt

    int key = filtset->filtArr[first].cost;
    filt.state = filtset->filtArr[first].state;
    filt.sync = filtset->filtArr[first].sync;
    filt.cost = filtset->filtArr[first].cost;
    for(i = 0; i < 7; i++){
        filt.dim[i][0] = filtset->filtArr[first].dim[i][0];
        filt.dim[i][1] = filtset->filtArr[first].dim[i][1];
    }

    while(first < end){
        while(first < end && filtset->filtArr[end].cost >= key)
            end--;
        if(first != end){

            filtset->filtArr[first].state = filtset->filtArr[end].state;
            filtset->filtArr[first].sync = filtset->filtArr[end].sync;
            filtset->filtArr[first].cost = filtset->filtArr[end].cost;
            for(i = 0; i < 7; i++){
                filtset->filtArr[first].dim[i][0] = filtset->filtArr[end].dim[i][0];
                filtset->filtArr[first].dim[i][1] = filtset->filtArr[end].dim[i][1];
            }
            first++;
        }
        while(first < end && filtset->filtArr[first].cost <= key)
            first++;
        if(first != end){
            filtset->filtArr[end].state = filtset->filtArr[first].state;
            filtset->filtArr[end].sync = filtset->filtArr[first].sync;
            filtset->filtArr[end].cost = filtset->filtArr[first].cost;
            for(i = 0; i < 7; i++){
                filtset->filtArr[end].dim[i][0] = filtset->filtArr[first].dim[i][0];
                filtset->filtArr[end].dim[i][1] = filtset->filtArr[first].dim[i][1];
            }
            end--;
        }
    }

    filtset->filtArr[first].state = filt.state;
    filtset->filtArr[first].sync = filt.sync;
    filtset->filtArr[first].cost = filt.cost;
    for(i = 0; i < 7; i++){
        filtset->filtArr[first].dim[i][0] = filt.dim[i][0];
        filtset->filtArr[first].dim[i][1] = filt.dim[i][1];
    }

    return first;
}
static void qsort(struct FILTSET *filtset, ,int first,int end)
{
    if(first < end)
    {
        int middle = qsorting(filtset, first, end);
        qsort(filtset,first, middle - 1);
        qsort(filtset,middle + 1, end);
    }
}
*/
#ifndef FRCORE_ACL_API_INTERFACE
void setfilter(uint8_t gid, uint16_t index, uint16_t targetid, char *sip, char *dip, char *sp, char *dp, char *protocol)
{
    struct FILTSET *filtset = &gsdata.acl->acl_data[gid].filtset;

//  printf("###%s,%s,%s,%s,%s\n", sip, dip, sp, dp, protocol);

    acl_readiprange(sip, filtset->filtArr[index].dim[0] ,filtset->filtArr[index].dim[1]);
    acl_readiprange(dip, filtset->filtArr[index].dim[2] ,filtset->filtArr[index].dim[3]);
    acl_ReadPort(sp, &(filtset->filtArr[index].dim[4][0]), &(filtset->filtArr[index].dim[4][1]));
    acl_ReadPort(dp, &(filtset->filtArr[index].dim[5][0]), &(filtset->filtArr[index].dim[5][1]));
    acl_ReadProtocol(protocol,&(filtset->filtArr[index].dim[6][0]), &(filtset->filtArr[index].dim[6][1]));

    filtset->filtArr[index].state = ACL_ENABLE;
    filtset->filtArr[index].sync = ACL_NOSYNC;
    filtset->filtArr[index].cost = targetid;
//  filtset->filtArr[index].count = 0;
    filtset->pkt_num[targetid] = 0;
    filtset->numFilters++;
    int i;
    printf("index:%d\n", index);
    for(i = 0; i < 7; i++) {
        printf("dim[%d][0]=0x%4x dim[%d][1]=0x%4x\n", i,
               filtset->filtArr[index].dim[i][0], i, filtset->filtArr[index].dim[i][1]);
    }
}


static void acl_insertTopfilter(uint8_t gid, uint16_t targetid, char *sip, char *dip, char *sp, char *dp, char *protocol)
{
    struct FILTSET *filtset = &gsdata.acl->acl_data[gid].filtset;
    int m, j, count = filtset->numFilters;
    for(j = count - 1; j >= 0; j--){
        filtset->filtArr[j + 1].state = filtset->filtArr[j].state;
        filtset->filtArr[j + 1].sync = filtset->filtArr[j].sync;
        filtset->pkt_num[filtset->filtArr[j+1].cost] = filtset->pkt_num[filtset->filtArr[j].cost];
//      filtset->filtArr[j + 1].count = filtset->filtArr[j].count;
        filtset->filtArr[j + 1].cost = filtset->filtArr[j].cost + 1;
        for(m = 0; m < 7; m++){
            filtset->filtArr[j + 1].dim[m][0] = filtset->filtArr[j].dim[m][0];
            filtset->filtArr[j + 1].dim[m][1] = filtset->filtArr[j].dim[m][1];
        }
    }
    setfilter(gid, 0, 0, sip, dip, sp, dp, protocol);
    return;

}

void acl_setfilter(uint8_t gid, uint16_t targetid, char *sip, char *dip, char *sp, char *dp, char *protocol)
{
    struct FILTSET *filtset = &gsdata.acl->acl_data[gid].filtset;
    int i, j, m, count = filtset->numFilters;

    printf("count=%d\n", count);
    for(i = 0; i < count; i++){
        if(targetid < filtset->filtArr[i].cost){
            for(j = count - 1; j >= i; j--){
                filtset->filtArr[j + 1].state = filtset->filtArr[j].state;
                filtset->filtArr[j + 1].sync = filtset->filtArr[j].sync;
                filtset->pkt_num[filtset->filtArr[j+1].cost] = filtset->pkt_num[filtset->filtArr[j].cost];
                filtset->filtArr[j + 1].cost = filtset->filtArr[j].cost;
//              filtset->filtArr[j + 1].count = filtset->filtArr[j].count;
                for(m = 0; m < 7; m++){
                    filtset->filtArr[j + 1].dim[m][0] = filtset->filtArr[j].dim[m][0];
                    filtset->filtArr[j + 1].dim[m][1] = filtset->filtArr[j].dim[m][1];
                }
            }
            setfilter(gid, i, targetid, sip, dip, sp, dp, protocol);
            return;
        }
    }
    setfilter(gid, count, targetid, sip, dip, sp, dp, protocol);
    return;
}

void acl_readiprange(char *fp,unsigned int* highRange,unsigned int* lowRange)
{
    /*assumes IPv4 prefixes*/
    // temporary variables to store IP range
    unsigned int trange[4];
    unsigned int mask;
    char validslash;
    // read IP range described by IP/mask
//  fscanf(fp, "%d.%d.%d.%d/%d", &trange[0],&trange[1],&trange[2],&trange[3],&mask);
    sscanf(fp, "%d.%d.%d.%d%c%d", &trange[0], &trange[1], &trange[2], &trange[3], &validslash, &mask);
//  sscanf(fp, "%c", &validslash);

    // deal with default mask
    if(validslash != '/')
        mask = 32;
//  else
//      fscanf(fp,"%d", &mask);

    int masklit1;
    unsigned int masklit2,masklit3;
    mask = 32 - mask;
    masklit1 = mask / 8;
    masklit2 = mask % 8;

    unsigned int ptrange[4];
    int i;
    for(i=0;i<4;i++)
        ptrange[i] = trange[i];

    // count the start IP
    for(i=3;i>3-masklit1;i--)
        ptrange[i] = 0;
    if(masklit2 != 0){
        masklit3 = 1;
        masklit3 <<= masklit2;
        masklit3 -= 1;
        masklit3 = ~masklit3;
        ptrange[3-masklit1] &= masklit3;
    }
    // store start IP
    highRange[0] = ptrange[0];
    highRange[0] <<= 8;
    highRange[0] += ptrange[1];
    lowRange[0] = ptrange[2];
    lowRange[0] <<= 8;
    lowRange[0] += ptrange[3];

    // count the end IP
    for(i=3;i>3-masklit1;i--)
        ptrange[i] = 255;
    if(masklit2 != 0){
        masklit3 = 1;
        masklit3 <<= masklit2;
        masklit3 -= 1;
        ptrange[3-masklit1] |= masklit3;
    }
    // store end IP
    highRange[1] = ptrange[0];
    highRange[1] <<= 8;
    highRange[1] += ptrange[1];
    lowRange[1] = ptrange[2];
    lowRange[1] <<= 8;
    lowRange[1] += ptrange[3];
}

// Read protocol
// fp: pointer to filter
// protocol: 17 for tcp
// return: void
void acl_ReadProtocol(char *fp, unsigned int  *from, unsigned int  *to)
{
    unsigned int tfrom, tto;
        sscanf(fp, "%u", &tfrom);
    if(ACL_PROTOCOL_ANY == tfrom){
        tfrom = 0;
        tto = 255;
    }
    else{
        tto = tfrom;
    }
    *from = tfrom;
    *to = tto;
}


// Read port
// fp: pointer to filter
// from:to  =>  0:65535 : specify the port range
// return: void
void acl_ReadPort(char *fp, unsigned int *from, unsigned int *to)
{
    unsigned int tfrom;
    unsigned int tto;
    if(strchr(fp, '-'))
        sscanf(fp,"%d-%d",&tfrom, &tto);
    else{
        sscanf(fp, "%d", &tfrom);
        tto = tfrom;
    }

    *from = tfrom;
    *to = tto;
}
#else
/* cover: 0 for add a acl
 *        1 for cover a acl
 */
void setfilter(uint8_t gid, uint16_t index, uint16_t targetid, uint32_t sip, uint32_t dip, uint16_t sp,
               uint16_t dp, uint16_t protocol, uint16_t op, uint16_t action, uint16_t rule_type,
               uint16_t rule_source, uint8_t cover)
{
    struct FILTSET *filtset = &gsdata.acl->acl_data[gid].filtset;

//  printf("###%s,%s,%s,%s,%s\n", sip, dip, sp, dp, protocol);

    acl_readiprange(sip, filtset->filtArr[index].dim[0] ,filtset->filtArr[index].dim[1]);
    acl_readiprange(dip, filtset->filtArr[index].dim[2] ,filtset->filtArr[index].dim[3]);
    acl_ReadPort(sp, &(filtset->filtArr[index].dim[4][0]), &(filtset->filtArr[index].dim[4][1]));
    acl_ReadPort(dp, &(filtset->filtArr[index].dim[5][0]), &(filtset->filtArr[index].dim[5][1]));
    acl_ReadProtocol(protocol,&(filtset->filtArr[index].dim[6][0]), &(filtset->filtArr[index].dim[6][1]));

    filtset->filtArr[index].state = ACL_ENABLE;
    filtset->filtArr[index].sync = ACL_NOSYNC;
    filtset->filtArr[index].cost = targetid;
    filtset->filtArr[index].op = op;
    filtset->filtArr[index].action = action;
    filtset->filtArr[index].rule_type = rule_type;
    filtset->filtArr[index].rule_source = rule_source;
//  filtset->filtArr[index].count = 0;
    filtset->pkt_num[targetid] = 0;
    filtset->pkt_bytes[targetid] = 0;
    if(!cover) {
        filtset->numFilters++;
    }
    int i;
    FRCORE_RULE("index:%d\n", index);
    for(i = 0; i < 7; i++) {
        FRCORE_RULE("dim[%d][0]=0x%4x dim[%d][1]=0x%4x\n", i,
               filtset->filtArr[index].dim[i][0], i, filtset->filtArr[index].dim[i][1]);
    }
}

void acl_setfilter(uint8_t gid, uint16_t targetid, uint32_t sip, uint32_t dip, uint16_t sp, uint16_t dp, uint16_t protocol,
                       uint16_t op, uint16_t action, uint16_t rule_type, uint16_t rule_source)
{
    struct FILTSET *filtset = &gsdata.acl->acl_data[gid].filtset;
    int i, j, m, count = filtset->numFilters;

    FRCORE_RULE("count=%d\n", count);
    for(i = 0; i < count; i++){
        if(targetid < filtset->filtArr[i].cost){
            for(j = count - 1; j >= i; j--){
                filtset->filtArr[j + 1].state = filtset->filtArr[j].state;
                filtset->filtArr[j + 1].sync = filtset->filtArr[j].sync;
                filtset->pkt_num[filtset->filtArr[j+1].cost] = filtset->pkt_num[filtset->filtArr[j].cost];
                filtset->pkt_bytes[filtset->filtArr[j+1].cost] = filtset->pkt_bytes[filtset->filtArr[j].cost];
                filtset->filtArr[j + 1].cost = filtset->filtArr[j].cost;
                filtset->filtArr[j + 1].op = filtset->filtArr[j].op;
                filtset->filtArr[j + 1].action = filtset->filtArr[j].action;
                filtset->filtArr[j + 1].rule_type = filtset->filtArr[j].rule_type;
                filtset->filtArr[j + 1].rule_source = filtset->filtArr[j].rule_source;
//              filtset->filtArr[j + 1].count = filtset->filtArr[j].count;
                for(m = 0; m < 7; m++){
                    filtset->filtArr[j + 1].dim[m][0] = filtset->filtArr[j].dim[m][0];
                    filtset->filtArr[j + 1].dim[m][1] = filtset->filtArr[j].dim[m][1];
                }
            }
            setfilter(gid, i, targetid, sip, dip, sp, dp, protocol, op, action, rule_type, rule_source, 0);
            return;
        }else if(targetid == filtset->filtArr[i].cost) {
            setfilter(gid, i, targetid, sip, dip, sp, dp, protocol, op, action, rule_type, rule_source, 1);
            return;
        }
    }
    setfilter(gid, count, targetid, sip, dip, sp, dp, protocol, op, action, rule_type, rule_source, 0);
    return;
}

void acl_readiprange(uint32_t ip,unsigned int* highRange,unsigned int* lowRange)
{
    /*assumes IPv4 prefixes*/
    // temporary variables to store IP range
    unsigned int trange[4];
    unsigned int mask;
    int i;

    if(ip) {
        mask = 32;
    }else{
        mask = 0;
    }

    for(i = 0; i < 4; i++) {
        trange[i] = (ip >> ((3-i)*8)) & 0xff;
        FRCORE_RULE("trange[%d]=0x%2x\n", i, trange[i]);
    }

    int masklit1;
    unsigned int masklit2,masklit3;
    mask = 32 - mask;
    masklit1 = mask / 8;
    masklit2 = mask % 8;

    unsigned int ptrange[4];
    for(i=0;i<4;i++)
        ptrange[i] = trange[i];

    // count the start IP
    for(i=3;i>3-masklit1;i--)
        ptrange[i] = 0;
    if(masklit2 != 0){
        masklit3 = 1;
        masklit3 <<= masklit2;
        masklit3 -= 1;
        masklit3 = ~masklit3;
        ptrange[3-masklit1] &= masklit3;
    }
    // store start IP
    highRange[0] = ptrange[0];
    highRange[0] <<= 8;
    highRange[0] += ptrange[1];
    lowRange[0] = ptrange[2];
    lowRange[0] <<= 8;
    lowRange[0] += ptrange[3];

    // count the end IP
    for(i=3;i>3-masklit1;i--)
        ptrange[i] = 255;
    if(masklit2 != 0){
        masklit3 = 1;
        masklit3 <<= masklit2;
        masklit3 -= 1;
        ptrange[3-masklit1] |= masklit3;
    }
    // store end IP
    highRange[1] = ptrange[0];
    highRange[1] <<= 8;
    highRange[1] += ptrange[1];
    lowRange[1] = ptrange[2];
    lowRange[1] <<= 8;
    lowRange[1] += ptrange[3];
}

// Read protocol
// fp: pointer to filter
// protocol: 17 for tcp
// return: void
void acl_ReadProtocol(uint16_t proto, unsigned int  *from, unsigned int  *to)
{
    unsigned int tfrom, tto;
    tfrom = proto;
    if(ACL_PROTOCOL_ANY == tfrom){
        tfrom = 0;
        tto = 255;
    }
    else{
        tto = tfrom;
    }
    *from = tfrom;
    *to = tto;
}


// Read port
// fp: pointer to filter
// from:to  =>  0:65535 : specify the port range
// return: void
void acl_ReadPort(uint16_t port, unsigned int *from, unsigned int *to)
{
    unsigned int tfrom;
    unsigned int tto;
    /* if port > 0 ,then exact port */
    if(port) {
        tfrom = port;
        tto = tfrom;
    }else {
        tfrom = 0;
        tto = 65535;
    }

    *from = tfrom;
    *to = tto;
}
#endif

#ifdef RFC3_DEBUG
void pre_acl_lookup_by_tuple(uint8_t aclgid)
{
    SetPhase0_Cell(gsdata.acl->acl_data[aclgid].phase0_Nodes, &gsdata.acl->acl_data[aclgid].filtset);
    SetPhase1_Cell(gsdata.acl->acl_data[aclgid].phase0_Nodes, gsdata.acl->acl_data[aclgid].phase1_Nodes, gsdata.acl->acl_data[aclgid].dot);
    SetPhase2_Cell(gsdata.acl->acl_data[aclgid].phase1_Nodes, &gsdata.acl->acl_data[aclgid].phase2_Node);

    Pnode *phase0_Nodes = gsdata.acl->acl_data[aclgid].phase0_Nodes;
    Pnoder *phase1_Nodes = gsdata.acl->acl_data[aclgid].phase1_Nodes;
    Pnoder *phase2_Node = &gsdata.acl->acl_data[aclgid].phase2_Node;

    uint8_t *dot = gsdata.acl->acl_data[aclgid].dot;

    int phase0nces[7];
    //simphase_node(phase0_Nodes, phase1_Nodes, phase2_Node);
    phase0nces[0] = phase0_Nodes[dot[0]].listEqs.nCES;
    phase0nces[1] = phase0_Nodes[dot[1]].listEqs.nCES;
    phase0nces[2] = phase0_Nodes[dot[2]].listEqs.nCES;
    phase0nces[3] = phase0_Nodes[dot[3]].listEqs.nCES;
    phase0nces[4] = phase0_Nodes[dot[4]].listEqs.nCES;
    phase0nces[5] = phase0_Nodes[dot[5]].listEqs.nCES;
    phase0nces[6] = phase0_Nodes[6].listEqs.nCES;
    int i = 0;
    for(i = 0; i < 7; i++)
        printf("phase0nces[%u] : %u\n", i, phase0nces[i]);

    printf("phase1_Nodes[0].listEqs.nCES : %u\n", phase1_Nodes[0].listEqs.nCES);
    printf("phase1_Nodes[1].listEqs.nCES : %u\n", phase1_Nodes[1].listEqs.nCES);

    gsdata.acl->acl_data[aclgid].indx0a = phase0nces[1] * phase0nces[2] * phase0nces[6];
    gsdata.acl->acl_data[aclgid].indx0b = phase0nces[2] * phase0nces[6];

    //phase1nces = phase1_Nodes[1].listEqs.nCES;

    gsdata.acl->acl_data[aclgid].index1a = phase0nces[4] * phase0nces[5];

}

unsigned short acl_lookup_by_tuple(Pnode *phase0_Nodes, Pnoder *phase1_Nodes, Pnoder *phase2_Node, uint8_t *dot,
                   uint16_t siph, uint16_t sipl, uint16_t diph, uint16_t dipl, uint16_t sp, uint16_t dp ,uint8_t proto)
{
    //acl_setfilter(uint8_t gid, uint8_t targetid, char *sip, char *dip, char *sp, char *dp, char *protocol)
//  ReadFilterFile_test(&gsdata.acl->acl_data[0].filtset);
//  pre_acl_lookup_by_tuple(0);
// Lookup process
    unsigned int cid[9];
    unsigned int indx[3];

//  printf("###sip: %u,%u  dip: %u,%u  sp: %u  dp: %u pro: %u\n", siph, sipl, diph, dipl, sp, dp, proto);
//  uint8_t *dot = gsdata.acl->acl_data[gid].dot;

    cid[0] = phase0_Nodes[0].cell[siph];
    cid[1] = phase0_Nodes[1].cell[sipl];
    cid[2] = phase0_Nodes[2].cell[diph];
    cid[3] = phase0_Nodes[3].cell[dipl];
    cid[4] = phase0_Nodes[4].cell[sp];
    cid[5] = phase0_Nodes[5].cell[dp];
    cid[6] = phase0_Nodes[6].cell[proto];


    indx[0] = cid[dot[0]] * gsdata.acl->acl_data[0].indx0a
        + cid[dot[1]] * gsdata.acl->acl_data[0].indx0b
        + cid[dot[2]] * phase0_Nodes[6].listEqs.nCES
        + cid[6];
    indx[1] = cid[dot[3]] * gsdata.acl->acl_data[0].index1a
        + cid[dot[4]] * phase0_Nodes[dot[5]].listEqs.nCES
        + cid[dot[5]];

    cid[7] = phase1_Nodes[0].cell[indx[0]];
    cid[8] = phase1_Nodes[1].cell[indx[1]];

    indx[2] = cid[7] * phase1_Nodes[1].listEqs.nCES
        + cid[8];

    return phase2_Node->cell[indx[2]];

}
#endif
#ifdef RFC4_DEBUG
int pre_acl_lookup_by_tuple(uint8_t aclgid)
{
    int ret = ACL_OK;
    ret = SetPhase0_Cell(gsdata.acl->acl_data[aclgid].phase0_Nodes, &gsdata.acl->acl_data[aclgid].filtset);
    if(ACL_ERROR == ret)
        return MC_ERROR;
    ret = SetPhase1_Cell(gsdata.acl->acl_data[aclgid].phase0_Nodes, gsdata.acl->acl_data[aclgid].phase1_Nodes);
    if(ACL_ERROR == ret)
        return MC_ERROR;
    ret = SetPhase2_Cell(gsdata.acl->acl_data[aclgid].phase0_Nodes, gsdata.acl->acl_data[aclgid].phase1_Nodes, gsdata.acl->acl_data[aclgid].phase2_Nodes);
    if(ACL_ERROR == ret)
        return MC_ERROR;
    ret = SetPhase3_Cell(aclgid, gsdata.acl->acl_data[aclgid].phase2_Nodes, &gsdata.acl->acl_data[aclgid].phase3_Node);
    if(ACL_ERROR == ret)
        return MC_ERROR;

    return FRE_SUCCESS;
}
uint16_t  acl_lookup_by_tuple(Pnode *phase0_Nodes, Pnoder *phase1_Nodes, Pnoder *phase2_Nodes, Pnoder *phase3_Node,
                   uint16_t siph, uint16_t sipl, uint16_t diph, uint16_t dipl, uint16_t sp, uint16_t dp ,uint8_t proto)
{
    // Lookup process
    unsigned int cid[12];
    unsigned int indx[6];
/*
    printf("phase0_Nodes[0].listEqs.nCES = %u\n", phase0_Nodes[0].listEqs.nCES);
    printf("phase0_Nodes[1].listEqs.nCES = %u\n", phase0_Nodes[1].listEqs.nCES);
    printf("phase0_Nodes[2].listEqs.nCES = %u\n", phase0_Nodes[2].listEqs.nCES);
    printf("phase0_Nodes[3].listEqs.nCES = %u\n", phase0_Nodes[3].listEqs.nCES);
    printf("phase0_Nodes[4].listEqs.nCES = %u\n", phase0_Nodes[4].listEqs.nCES);
    printf("phase0_Nodes[5].listEqs.nCES = %u\n", phase0_Nodes[5].listEqs.nCES);
    printf("phase0_Nodes[6].listEqs.nCES = %u\n", phase0_Nodes[6].listEqs.nCES);
    printf("**********************************************************************\n");
    printf("phase1_Nodes[0].listEqs.nCES = %u\n", phase1_Nodes[0].listEqs.nCES);
    printf("phase1_Nodes[1].listEqs.nCES = %u\n", phase1_Nodes[1].listEqs.nCES);
    printf("phase1_Nodes[2].listEqs.nCES = %u\n", phase1_Nodes[2].listEqs.nCES);
    printf("**********************************************************************\n");
    printf("phase2_Nodes[0].listEqs.nCES = %u\n", phase2_Nodes[0].listEqs.nCES);
    printf("phase2_Nodes[1].listEqs.nCES = %u\n", phase2_Nodes[1].listEqs.nCES);
    printf("**********************************************************************\n");
*/
// phase 0
    cid[0] = phase0_Nodes[0].cell[siph];
    cid[1] = phase0_Nodes[1].cell[sipl];
    cid[2] = phase0_Nodes[2].cell[diph];
    cid[3] = phase0_Nodes[3].cell[dipl];
    cid[4] = phase0_Nodes[4].cell[sp];
    cid[5] = phase0_Nodes[5].cell[dp];
    cid[6] = phase0_Nodes[6].cell[proto];
/*
    int l;
    for(l = 0; l < 7; l++)
        printf("cid[%d] = %u\n", l, cid[l]);
*/
    //phase 1
    indx[0] = cid[0] * phase0_Nodes[1].listEqs.nCES + cid[1];
    indx[1] = cid[2] * phase0_Nodes[3].listEqs.nCES + cid[3];
    indx[2] = cid[4] * phase0_Nodes[5].listEqs.nCES + cid[5];
    //      indx[3] = cid[6];
    //phase 2
    cid[7]  =  phase1_Nodes[0].cell[indx[0]];
    cid[8]  =  phase1_Nodes[1].cell[indx[1]];
    cid[9]  =  phase1_Nodes[2].cell[indx[2]];
    //      cid[6];

    indx[3] = cid[7] * phase1_Nodes[1].listEqs.nCES + cid[8];
    indx[4] = cid[9] * phase0_Nodes[6].listEqs.nCES + cid[6];
    //phase 3

    cid[10] = phase2_Nodes[0].cell[indx[3]];
    cid[11] = phase2_Nodes[1].cell[indx[4]];

    indx[5] = cid[10] * phase2_Nodes[1].listEqs.nCES + cid[11];
/*
    printf("index[0] = %u\n", indx[0]);
    printf("index[1] = %u\n", indx[1]);
    printf("index[2] = %u\n", indx[2]);
    printf("index[3] = %u\n", indx[3]);
    printf("index[4] = %u\n", indx[4]);
    printf("index[5] = %u\n", indx[5]);
*/
    // store lookup result into lookupResult[]
    return phase3_Node->cell[indx[5]];
}
/*
inline void acl_statistic_hit(uint8_t aclgid, uint16_t tagid)
{
    cvmx_atomic_fetch_and_add64(&gsdata.acl->acl_data[aclgid].filtset.statistics[tagid], 1);
}
inline void acl_init_statistic_hit(uint8_t aclgid, uint16_t tagid)
{
    cvmx_atomic_set64(&gsdata.acl->acl_data[aclgid].filtset.statistics[tagid], 0);
}
inline uint64_t acl_get_statistic_hit(uint8_t aclgid, uint16_t tagid)
{
    return cvmx_atomic_fetch_and_add64(&gsdata.acl->acl_data[aclgid].filtset.statistics[tagid], 0);
}
*/

#endif


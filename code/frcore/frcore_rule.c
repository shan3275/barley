#include "frcore.h"
#include "frcore_cmd.h"
#include "frcore_misc.h"
#include "frc_pack.h"
#include "frcore_proto.h"
#include "cvmx-mdio.h"
#include "frcore_init.h"
#include "frcore_acl.h"
#include "frcore_pkt.h"
#include "frc_types.h"
#include "frcore_stat.h"
#include "frcore_chan.h"

#if FRC_CONFIG_RULE
static CVMX_SHARED uint8_t rule_enable = 0;

int frcore_cmd_rule_enable(uint16_t plen, void *param, uint16_t *olen, void *outbuf)
{
    uint8_t *enable = (uint8_t *)param;
    rule_enable = *enable;
    FRCORE_RULE("===========================================================\n");
    //printf("rule_enable=%d\n", rule_enable);

    return FRE_SUCCESS;
}

int frcore_cmd_rule_status_get(uint16_t plen, void *param, uint16_t *olen, void *outbuf)
{
    uint8_t enable;
    enable = rule_enable;
    memcpy(outbuf, &enable, 1);
    *olen = 1;

    return FRE_SUCCESS;
}

int frcore_cmd_rule_add(uint16_t plen, void *param, uint16_t *olen, void *outbuf)
{
    frc_rule_t *rule = (frc_rule_t *)param;

    FRCORE_RULE("===========================================================\n");
    FRCORE_RULE("sip = 0x%x\n", rule->sip);
    FRCORE_RULE("dip = 0x%x\n", rule->dip);
    FRCORE_RULE("sp = %d\n", rule->sp);
    FRCORE_RULE("dp = %d\n", rule->dp);
    FRCORE_RULE("proto = %d\n", rule->proto);
    FRCORE_RULE("index = %d\n", rule->index);
    FRCORE_RULE("rule_source = %d\n", rule->rule_source);
    FRCORE_RULE("rule_type = %d\n", rule->rule_type);
    FRCORE_RULE("action = %d\n", rule->action);
    FRCORE_RULE("op = %d\n", rule->op);

#ifndef FRCORE_ACL_API_INTERFACE
    uint16_t targetid=1;
    char sip[20] = {0}, dip[20] = {0}, sp[20] = {0}, dp[20] = {0}, protocol[20] = {0};
    memcpy(sip, "192.168.0.1         ", 20);
    memcpy(dip, "10.0.0.1            ", 20);
    memcpy(sp,  "30-50               ", 20);
    memcpy(dp,  "30-50               ", 20);
    memcpy(protocol, "17                  ", 20);
    acl_setfilter(0, targetid, sip, dip, sp, dp, protocol);
    //setfilter(0, targetid, targetid, sip, dip, sp, dp, protocol);

    targetid=2;
    memcpy(sip, "192.168.0.2         ", 20);
    memcpy(dip, "10.0.0.1            ", 20);
    memcpy(sp,  "30-50               ", 20);
    memcpy(dp,  "30-50               ", 20);
    memcpy(protocol, "17                  ", 20);
    acl_setfilter(0, targetid, sip, dip, sp, dp, protocol);
    //setfilter(0, targetid, targetid, sip, dip, sp, dp, protocol);
#else
    acl_setfilter(0, rule->index, rule->sip, rule->dip, rule->sp, rule->dp, rule->proto,
                  rule->op, rule->action, rule->rule_type, rule->rule_source);
#endif

    return FRE_SUCCESS;

}

int frcore_cmd_rule_del(uint16_t plen, void *param, uint16_t *olen, void *outbuf)
{
    int i, rv = 0;
    uint16_t num = 0;
    frc_rule_op_in_t *del = (frc_rule_op_in_t *)param;

    uint16_t tagid;
    FRCORE_RULE("===========================================================\n");
    FRCORE_RULE("index = %d\n", del->index);
    FRCORE_RULE("num = %d\n", del->num);

    tagid = del->index;
    for (i = 0; i < (int)del->num; i++)
    {
        rv = acl_DeleteOneFilter_by_tagid(0, tagid+i);
        if (!rv)
        {
            num++;
        }
    }
    *((uint16_t *) outbuf) = num;

    return rv;

}

int frcore_cmd_rule_clear_all(uint16_t plen, void *param, uint16_t *olen, void *outbuf)
{
    int i, rv;
    frc_rule_op_in_t *del = (frc_rule_op_in_t *)param;
    uint16_t num;

    uint16_t tagid;
    FRCORE_RULE("===========================================================\n");
    FRCORE_RULE("index = %d\n", del->index);
    FRCORE_RULE("num = %d\n", del->num);

    num = acl_getfilter_count(0);
    tagid = del->index;
    for (i = 0; i < (int)del->num; i++)
    {
        rv = acl_DeleteOneFilter_by_tagid(0, tagid+i);
    }
    if (!acl_getfilter_count(0))
    {
        *((uint16_t *) outbuf) = num;
    } else {
        FRCORE_RULE("num = %d\n", acl_getfilter_count(0));
        return FRE_FAIL;
    }

    return FRE_SUCCESS;

}

int frcore_cmd_rule_update(uint16_t plen, void *param, uint16_t *olen, void *outbuf)
{
    int rv;
    //frc_rule_op_in_t *del = (frc_rule_op_in_t *)param;

    //uint16_t tagid;
    FRCORE_RULE("===========================================================\n");
    FRCORE_RULE("update rule.\n");
    rv = pre_acl_lookup_by_tuple(0);
    if (rv)
    {
        printf("%s rv=%d\n", __func__, rv);
    }
    //printf("%s rv=%d\n", __func__, rv);
    return rv;

}

int frcore_cmd_rule_stat_get(uint16_t plen, void *param, uint16_t *olen, void *outbuf)
{
    frc_rule_op_in_t *input = (frc_rule_op_in_t *)param;
    frc_rule_stat_out_t output;

    FRCORE_RULE("===========================================================\n");
    FRCORE_RULE("index = %d\n", input->index);
    FRCORE_RULE("num = %d\n", input->num);
#ifndef FRCORE_ACL_API_INTERFACE
    memset(&output, 0, sizeof(frc_rule_stat_out_t));
    output.num = input->num;
    output.rule_stat[0].rule.sip = 0xca000001;
    output.rule_stat[0].rule.dip = 0xca000002;
    output.rule_stat[0].rule.sp  = 435;
    output.rule_stat[0].rule.dp  = 526;
    output.rule_stat[0].rule.proto = 0x11;
    output.rule_stat[0].rule.index = input->index;
    output.rule_stat[0].rule.rule_source = 4;
    output.rule_stat[0].rule.rule_type  = 5;
    output.rule_stat[0].rule.action = 1;
    output.rule_stat[0].rule.op = 2;
    output.rule_stat[0].stat.pkts = 400;
    output.rule_stat[0].stat.bytes = 50000;

    output.rule_stat[1].rule.sip = 0xc0000003;
    output.rule_stat[1].rule.dip = 0xca000004;
    output.rule_stat[1].rule.sp  = 676;
    output.rule_stat[1].rule.dp  = 787;
    output.rule_stat[1].rule.proto = 0x6;
    output.rule_stat[1].rule.index = input->index + 1;
    output.rule_stat[1].rule.rule_source = 6;
    output.rule_stat[1].rule.rule_type  = 7;
    output.rule_stat[1].rule.action = 8;
    output.rule_stat[1].rule.op = 9;
    output.rule_stat[1].stat.pkts = 200;
    output.rule_stat[1].stat.bytes = 30000;

    output.rule_stat[2].rule.sip = 0xb0000003;
    output.rule_stat[2].rule.dip = 0xb0000004;
    output.rule_stat[2].rule.sp  = 232;
    output.rule_stat[2].rule.dp  = 345;
    output.rule_stat[2].rule.proto = 0x11;
    output.rule_stat[2].rule.index = input->index + 2;
    output.rule_stat[2].rule.rule_source = 12;
    output.rule_stat[2].rule.rule_type  = 13;
    output.rule_stat[2].rule.action = 14;
    output.rule_stat[2].rule.op = 15;
    output.rule_stat[2].stat.pkts = 600;
    output.rule_stat[2].stat.bytes = 80000;
#else
    int i,j, rv;
    uint32_t sip, dip;
    uint16_t sp ,dp, protocol;
    uint16_t tagid, rulenum;
    uint8_t state, sync;
    uint16_t op, action, rule_type, rule_source;
    uint32_t pkts, bytes;
    output.num = 0;
    for (i = 0, j = 0; i < (int)input->num; i++)
    {
        tagid = input->index + i;
        rv = acl_getfilter_by_tagid(0, tagid, &rulenum, &state, &sync,
                                    &sip, &dip, &sp, &dp, &protocol, &pkts, &bytes,
                                    &op, &action, &rule_type, &rule_source);
        if (rv)
        {
            continue;
        }
        output.rule_stat[j].rule.sip = sip;
        output.rule_stat[j].rule.dip = dip;
        output.rule_stat[j].rule.sp  = sp;
        output.rule_stat[j].rule.dp  = dp;
        output.rule_stat[j].rule.proto = protocol;
        output.rule_stat[j].rule.index = tagid;
        output.rule_stat[j].rule.rule_source = rule_source;
        output.rule_stat[j].rule.rule_type  = rule_type;
        output.rule_stat[j].rule.action = action;
        output.rule_stat[j].rule.op = op;
        output.rule_stat[j].stat.pkts = pkts;
        output.rule_stat[j].stat.bytes = bytes;
        output.num++;
        j++;
    }
#endif
    memcpy(outbuf, &output, sizeof(frc_rule_stat_out_t));
    *olen = sizeof(frc_rule_stat_out_t);

    return FRE_SUCCESS;
}


int frcore_cmd_rule_stat_clear(uint16_t plen, void *param, uint16_t *olen, void *outbuf)
{
    frc_rule_op_in_t *input = (frc_rule_op_in_t *)param;
    int i;

    FRCORE_RULE("===========================================================\n");
    FRCORE_RULE("index = %d\n", input->index);
    FRCORE_RULE("num = %d\n", input->num);
    uint16_t tagid;
    for (i= 0; i < (int)input->num; i++)
    {
        tagid = input->index + i;
        acl_initfilter_statistics_by_tagid(0, tagid);
    }
    *((uint16_t *)outbuf) = i;
    *olen = 2;
    FRCORE_RULE(" *((uint16_t *)outbuf)= %d\n",  *((uint16_t *)outbuf));
    return FRE_SUCCESS;
}


int frcore_cmd_rule_get_num(uint16_t plen, void *param, uint16_t *olen, void *outbuf)
{
    FRCORE_RULE("===========================================================\n");
    *((uint16_t *)outbuf) = acl_getfilter_count(0);
    *olen = 2;
    FRCORE_RULE(" *((uint16_t *)outbuf)= %d\n",  *((uint16_t *)outbuf));
    return FRE_SUCCESS;
}

int
frcore_rule_init()
{
    FRCORE_REGISTER_CMD(CMD_TYPE_USER, USER_CMD_RULE_ENABLE,  frcore_cmd_rule_enable);
    FRCORE_REGISTER_CMD(CMD_TYPE_USER, USER_CMD_GET_RULE_STATUS,  frcore_cmd_rule_status_get);
    FRCORE_REGISTER_CMD(CMD_TYPE_USER, USER_CMD_ADD_RULE,  frcore_cmd_rule_add);
    FRCORE_REGISTER_CMD(CMD_TYPE_USER, USER_CMD_DEL_RULE,  frcore_cmd_rule_del);
    FRCORE_REGISTER_CMD(CMD_TYPE_USER, USER_CMD_CLEAN_ALL_RULE, frcore_cmd_rule_clear_all);
    FRCORE_REGISTER_CMD(CMD_TYPE_USER, USER_CMD_UPDATE_RULE, frcore_cmd_rule_update);
    FRCORE_REGISTER_CMD(CMD_TYPE_USER, USER_CMD_GET_RULE,  frcore_cmd_rule_stat_get);
    FRCORE_REGISTER_CMD(CMD_TYPE_USER, USER_CMD_CLEAN_RULE_STAT, frcore_cmd_rule_stat_clear);
    FRCORE_REGISTER_CMD(CMD_TYPE_USER, USER_CMD_GET_RULE_NUM, frcore_cmd_rule_get_num);

    return 0;
}


int frcore_rule_process(cvmx_wqe_t *work, uint8_t *eth_ptr, uint8_t *ip_ptr,
                       uint8_t *udp_ptr, uint16_t ip_paylen, uint64_t smac, uint64_t dmac)
{
    //int smid = 0;
    uint8_t aclgid = 0;
    //int ifidx;
    int rv = 0;
    struct mpp_tuple tuple;
    uint16_t tag = 0;
    uint16_t pkt_len;    /* packet len */
    struct tcphdr *th;

    /* rule switch */
    if (!rule_enable)
    {
        return 0;
    }

    /* for packet submit */
    frc_dma_hdr_t hdr;
    frc_dma_pkt_info_t info;
    int eth_len;

    #define _IPH ((struct iphdr*)(ip_ptr))

    tuple.data[0] = *((uint64_t*)(((char*)_IPH)+12));
    tuple.data[1] = 0ul;
    tuple.data_32[2] = *(uint32_t*)(udp_ptr);
    tuple.proto = _IPH->protocol;

    MC_PRINTF_INFO("sip:%u,%u,%u,%u dip:%u,%u,%u,%u sp:%u,dp:%u,pro:%u\n",
            tuple.sip >> 24 & 0xFF,
            tuple.sip >>16 & 0xFF,
            tuple.sip >> 8 & 0xFF,
            tuple.sip  & 0xFF,
            tuple.dip >> 24 & 0xFF,
            tuple.dip >>16 & 0xFF,
            tuple.dip >> 8 & 0xFF,
            tuple.dip  & 0xFF,
            tuple.sp,
            tuple.dp,
            tuple.proto);

        /* count malformed packet */
    if (tuple.proto == PROTO_TCP)
    {
        th = (struct tcphdr *)udp_ptr;
        if(((_IPH->ihl + th->doff) <<2) > (_IPH->tot_len)) {
            FRCORE_STAT_INC(stat_rule_malformed_packet);
            return FRCORE_ACT_DROP;
        }
    }

//  uint32_t acl_rst = acl_lookup_by_tuple(
    tag = acl_lookup_by_tuple(
            gsdata.acl->acl_data[aclgid].phase0_Nodes,
            gsdata.acl->acl_data[aclgid].phase1_Nodes,
            gsdata.acl->acl_data[aclgid].phase2_Nodes,
            &gsdata.acl->acl_data[aclgid].phase3_Node,
            tuple.sip>>16 & 0xFFFF,
            tuple.sip & 0xFFFF,
            tuple.dip>>16 & 0xFFFF,
            tuple.dip & 0xFFFF,
            tuple.sp,
            tuple.dp,
            tuple.proto);

    FRCORE_RULE("tag=%d\n", tag);
    if (tag)
    {
            if(tag != MPP_MAX_FILTERS-1) {
                FRCORE_STAT_INC(stat_rule_matched_pkts);
                FRCORE_STAT_ADD(stat_rule_matched_bytes, work->len);
                /* add pktnum and pkt bytes statustics */
                pkt_len = work->len;;
                MC_PRINTF_INFO("pkt_len: %d\n");
                /* add pkt_num */
                acl_statistic_hit(aclgid, tag);
                /* add pkt_byte */
                acl_pkt_bytes_add(aclgid, tag, pkt_len);

                /* submit packet to fifo */
                memset(&hdr, 0, sizeof(frc_dma_hdr_t));
                memset(&info, 0, sizeof(frc_dma_pkt_info_t));

                UP32(ip_ptr + 12, hdr.sip);

                UP32(ip_ptr + 16, hdr.dip);

                UP16(udp_ptr, hdr.sport);
                UP16(udp_ptr + 2, hdr.dport);

                eth_len          = work->len;

                hdr.protocol     = tuple.proto;
                hdr.hash         = work->tag & 0xffffff;
                //hdr.info_offset  = FRC_DMA_PKT_INFO_OFFSET;
                hdr.pkt_num      = 1;
                hdr.total_paylen = eth_len;
                switch (tuple.proto) {
                case PROTO_TCP: /* Process tcp packet. */
                    th = (struct tcphdr *)udp_ptr;
                    info.data_offset = (uint32_t)(udp_ptr - eth_ptr + (th->doff << 2)); // tcp head length
                    info.payload_len = ip_paylen - (th->doff << 2); // tcp head length 8 bytes
                    info.sequence = th->seq;
                    info.ack_seq  = th->ack_seq;
                    break;
                case PROTO_UDP: /* Process udp packet. */
                    info.data_offset = (uint32_t)(udp_ptr - eth_ptr + 8); // udp head length 8 bytes
                    info.payload_len = ip_paylen - 8; // udp head length 8 bytes
                    break;
                default: /* Drop other ip packet. */
                    break;
                }
                info.smac        = smac;
                info.dmac        = dmac;

                //rv = frcore_forward_simple_pkt_to_fifo(FRC_WORK_RULE, &hdr, &info, eth_ptr);
                rv = frcore_forward_rule_pkt_to_fifo(&tag, &hdr, &info, eth_ptr);
                FRCORE_RULE("frcore_forward_simple_pkt_to_fifo return %d.\n", rv);
                if (rv)
                {
                    //FRCORE_STAT_INC(stat_dma_errors);
                }
            }
    }
    else
    {
        FRCORE_STAT_INC(stat_rule_not_matched_pkts);
        FRCORE_STAT_ADD(stat_rule_not_matched_bytes, work->len);
    }

    return rv;
}
#endif

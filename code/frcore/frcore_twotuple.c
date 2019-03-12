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
#include "frcore_filter.h"

#if FRC_CONFIG_TWO_TUPLE
static CVMX_SHARED uint8_t acl_enable = 0;

int frcore_cmd_acl_enable(uint16_t plen, void *param, uint16_t *olen, void *outbuf)
{
    uint8_t *enable = (uint8_t *)param;
    acl_enable = *enable;
    FRCORE_TwoTuple("===========================================================\n");
    printf("acl_enable=%d\n", acl_enable);

    return FRE_SUCCESS;
}

int frcore_cmd_acl_status_get(uint16_t plen, void *param, uint16_t *olen, void *outbuf)
{
    uint8_t enable;
    enable = acl_enable;
    memcpy(outbuf, &enable, 1);
    *olen = 1;

    return FRE_SUCCESS;
}

int frcore_cmd_acl_add(uint16_t plen, void *param, uint16_t *olen, void *outbuf)
{
    frc_acl_t *acl = (frc_acl_t *)param;

    FRCORE_TwoTuple("===========================================================\n");
    FRCORE_TwoTuple("onetuple = 0x%x\n", acl->one_tuple);
    FRCORE_TwoTuple("proto = %d\n", acl->proto);
    FRCORE_TwoTuple("index = %d\n", acl->index);
    FRCORE_TwoTuple("acl_source = %d\n", acl->acl_source);
    FRCORE_TwoTuple("acl_type = %d\n", acl->acl_type);
    FRCORE_TwoTuple("action = %d\n", acl->action);
    FRCORE_TwoTuple("op = %d\n", acl->op);

#if FRC_CONFIG_TWO_TUPLE_TSET
#else
    acl_set_filter(acl->index,  acl->one_tuple, acl->proto, acl->op,
                   acl->action, acl->acl_type,  acl->acl_source);
#endif

    return FRE_SUCCESS;

}

int frcore_cmd_acl_del(uint16_t plen, void *param, uint16_t *olen, void *outbuf)
{
    int i, rv = 0;
    uint16_t num = 0;
    frc_acl_op_in_t *del = (frc_acl_op_in_t *)param;

    uint16_t tagid;
    FRCORE_TwoTuple("===========================================================\n");
    FRCORE_TwoTuple("index = %d\n", del->index);
    FRCORE_TwoTuple("num = %d\n", del->num);

    #if FRC_CONFIG_TWO_TUPLE_TSET
    num = del->num;
    #else
    tagid = del->index;
    for (i = 0; i < (int)del->num; i++)
    {
        rv = acl_del_filter(tagid+i);
        if (!rv)
        {
            num++;
        }
    }
    #endif
    *((uint16_t *) outbuf) = num;

    return rv;

}

int frcore_cmd_acl_clear_all(uint16_t plen, void *param, uint16_t *olen, void *outbuf)
{
    int i, rv;
    frc_acl_op_in_t *del = (frc_acl_op_in_t *)param;
    uint16_t num_old, num_new;

    uint16_t tagid;
    FRCORE_TwoTuple("===========================================================\n");
    FRCORE_TwoTuple("index = %d\n", del->index);
    FRCORE_TwoTuple("num = %d\n", del->num);

    #if FRC_CONFIG_TWO_TUPLE_TSET
    *((uint16_t *) outbuf) = del->num;
    #else
    rv = acl_get_filter_count(&num_old);
    if (rv)
    {
        return FRE_FAIL;
    }

    tagid = del->index;
    for (i = 0; i < (int)del->num; i++)
    {
        rv = acl_del_filter(tagid+i);
    }

    rv = acl_get_filter_count(&num_new);
    if (rv)
    {
        return FRE_FAIL;
    }

    if (!num_new)
    {
        *((uint16_t *) outbuf) = num_old;
    } else {
        FRCORE_TwoTuple("num = %d\n", num_new);
        return FRE_FAIL;
    }
    #endif
    return FRE_SUCCESS;

}


int frcore_cmd_acl_stat_get(uint16_t plen, void *param, uint16_t *olen, void *outbuf)
{
    frc_acl_op_in_t *input = (frc_acl_op_in_t *)param;
    frc_acl_stat_out_t output;

    FRCORE_TwoTuple("===========================================================\n");
    FRCORE_TwoTuple("index = %d\n", input->index);
    FRCORE_TwoTuple("num = %d\n", input->num);

    #if FRC_CONFIG_TWO_TUPLE_TSET
    memset(&output, 0, sizeof(frc_acl_stat_out_t));
    output.num = input->num;
    output.acl_stat[0].acl.one_tuple = 0xca000001;
    output.acl_stat[0].acl.proto = 0x11;
    output.acl_stat[0].acl.index = input->index;
    output.acl_stat[0].acl.acl_source = 4;
    output.acl_stat[0].acl.acl_type  = FRC_ACL_SIP;
    output.acl_stat[0].acl.action = 1;
    output.acl_stat[0].acl.op = 2;
    output.acl_stat[0].stat.pkts = 400;
    output.acl_stat[0].stat.bytes = 50000;

    output.acl_stat[1].acl.one_tuple= 0xc0000003;
    output.acl_stat[1].acl.proto = 0x6;
    output.acl_stat[1].acl.index = input->index + 1;
    output.acl_stat[1].acl.acl_source = 6;
    output.acl_stat[1].acl.acl_type  = FRC_ACL_DIP;
    output.acl_stat[1].acl.action = 8;
    output.acl_stat[1].acl.op = 9;
    output.acl_stat[1].stat.pkts = 200;
    output.acl_stat[1].stat.bytes = 30000;

    output.acl_stat[2].acl.one_tuple  = 232;
    output.acl_stat[2].acl.proto = 0x11;
    output.acl_stat[2].acl.index = input->index + 2;
    output.acl_stat[2].acl.acl_source = 12;
    output.acl_stat[2].acl.acl_type  = FRC_ACL_SP;
    output.acl_stat[2].acl.action = 14;
    output.acl_stat[2].acl.op = 15;
    output.acl_stat[2].stat.pkts = 600;
    output.acl_stat[2].stat.bytes = 80000;
    #else
    int i,j, rv;
    uint32_t one_tuple;
    uint16_t proto;
    uint16_t tagid, aclnum;
    uint16_t op, action, acl_type, acl_source;
    uint32_t pkts, bytes, hash;
    output.num = 0;
    for (i = 0, j = 0; i < (int)input->num; i++)
    {
        tagid = input->index + i;
        rv = acl_get_one_filter(tagid, &one_tuple, &proto, &op, &action,
                                &acl_type, &acl_source, &pkts, &bytes, &hash);
        if (rv)
        {
            continue;
        }
        output.acl_stat[j].acl.one_tuple = one_tuple;
        output.acl_stat[j].acl.proto = proto;
        output.acl_stat[j].acl.index = tagid;
        output.acl_stat[j].acl.acl_source = acl_source;
        output.acl_stat[j].acl.acl_type  = acl_type;
        output.acl_stat[j].acl.action = action;
        output.acl_stat[j].acl.op = op;
        output.acl_stat[j].acl.hash = hash;
        output.acl_stat[j].stat.pkts = pkts;
        output.acl_stat[j].stat.bytes = bytes;
        output.num++;
        j++;
    }
    #endif
    memcpy(outbuf, &output, sizeof(frc_acl_stat_out_t));
    *olen = sizeof(frc_acl_stat_out_t);

    return FRE_SUCCESS;
}


int frcore_cmd_acl_hash_table_get(uint16_t plen, void *param, uint16_t *olen, void *outbuf)
{
    frc_acl_hash_table_op_in_t *input = (frc_acl_hash_table_op_in_t *)param;
    frc_acl_hash_table_stat_out_t output;

    FRCORE_TwoTuple("===========================================================\n");
    FRCORE_TwoTuple("acl_type = %d\n", input->acl_type);
    FRCORE_TwoTuple("index = %d\n", input->index);
    FRCORE_TwoTuple("num = %d\n", input->num);

    #if FRC_CONFIG_TWO_TUPLE_TSET
    memset(&output, 0, sizeof(frc_acl_stat_out_t));
    output.num = input->num;
    output.acl_hash_stat[0].hash = input->index;
    output.acl_hash_stat[0].bucket_depth = 1;
    output.acl_hash_stat[0].total_cell = 1;
    output.acl_hash_stat[0].del_cell = 0

    output.acl_hash_stat[1].hash = input->index + 1;
    output.acl_hash_stat[1].bucket_depth = 1;
    output.acl_hash_stat[1].total_cell = 1;
    output.acl_hash_stat[1].del_cell = 0

    output.acl_hash_stat[2].hash = input->index + 2;
    output.acl_hash_stat[2].bucket_depth = 1;
    output.acl_hash_stat[2].total_cell = 1;
    output.acl_hash_stat[2].del_cell = 0

    output.acl_hash_stat[3].hash = input->index + 3;
    output.acl_hash_stat[3].bucket_depth = 1;
    output.acl_hash_stat[3].total_cell = 1;
    output.acl_hash_stat[3].del_cell = 0
    #else
    int i,j, rv;
    uint16_t tagid;
    uint32_t bucket_depth, total_cell, del_cell;
    output.num = 0;
    for (i = 0, j = 0; i < (int)input->num; i++)
    {
        tagid = input->index + i;
        rv = acl_get_one_hash_table(input->acl_type, tagid, &bucket_depth, &total_cell, &del_cell);
        if (rv)
        {
            continue;
        }

        output.acl_hash_stat[j].bucket_depth = bucket_depth;
        output.acl_hash_stat[j].total_cell   = total_cell;
        output.acl_hash_stat[j].del_cell     = del_cell;
        output.acl_hash_stat[j].hash         = tagid;
        output.num++;
        j++;
    }
    #endif
    memcpy(outbuf, &output, sizeof(frc_acl_hash_table_stat_out_t));
    *olen = sizeof(frc_acl_hash_table_stat_out_t);

    return FRE_SUCCESS;
}

int frcore_cmd_acl_stat_clear(uint16_t plen, void *param, uint16_t *olen, void *outbuf)
{
    frc_acl_op_in_t *input = (frc_acl_op_in_t *)param;
    int i;

    FRCORE_TwoTuple("===========================================================\n");
    FRCORE_TwoTuple("index = %d\n", input->index);
    FRCORE_TwoTuple("num = %d\n", input->num);
    #if FRC_CONFIG_TWO_TUPLE_TSET
    *((uint16_t *)outbuf) = ACL_MAX;
    *olen = 2;
    #else
    uint16_t tagid;
    for (i= 0; i < (int)input->num; i++)
    {
        tagid = input->index + i;
        filter_init_filter_statistics(tagid);
    }
    *((uint16_t *)outbuf) = i;
    *olen = 2;
    #endif
    FRCORE_TwoTuple(" *((uint16_t *)outbuf)= %d\n",  *((uint16_t *)outbuf));
    return FRE_SUCCESS;
}


int frcore_cmd_acl_get_num(uint16_t plen, void *param, uint16_t *olen, void *outbuf)
{
    FRCORE_TwoTuple("===========================================================\n");
    #if FRC_CONFIG_TWO_TUPLE_TSET
    *((uint16_t *)outbuf) = ACL_MAX;
    *olen = 2;
    #else
    int rv;
    uint16_t acl_num;
    rv = acl_get_filter_count(&acl_num);
    if (rv)
    {
        return FRE_FAIL;
    }
    *((uint16_t *)outbuf) = acl_num;
    *olen = 2;
    #endif
    FRCORE_TwoTuple(" *((uint16_t *)outbuf)= %d\n",  *((uint16_t *)outbuf));
    return FRE_SUCCESS;
}

int
frcore_acl_init()
{
    FRCORE_REGISTER_CMD(CMD_TYPE_USER, USER_CMD_ACL_ENABLE,  frcore_cmd_acl_enable);
    FRCORE_REGISTER_CMD(CMD_TYPE_USER, USER_CMD_GET_ACL_STATUS,  frcore_cmd_acl_status_get);
    FRCORE_REGISTER_CMD(CMD_TYPE_USER, USER_CMD_ADD_ACL,  frcore_cmd_acl_add);
    FRCORE_REGISTER_CMD(CMD_TYPE_USER, USER_CMD_DEL_ACL,  frcore_cmd_acl_del);
    FRCORE_REGISTER_CMD(CMD_TYPE_USER, USER_CMD_CLEAN_ALL_ACL, frcore_cmd_acl_clear_all);
    FRCORE_REGISTER_CMD(CMD_TYPE_USER, USER_CMD_GET_ACL,  frcore_cmd_acl_stat_get);
    FRCORE_REGISTER_CMD(CMD_TYPE_USER, USER_CMD_GET_ACL_HASH_TABLE,  frcore_cmd_acl_hash_table_get);
    FRCORE_REGISTER_CMD(CMD_TYPE_USER, USER_CMD_CLEAN_ACL_STAT, frcore_cmd_acl_stat_clear);
    FRCORE_REGISTER_CMD(CMD_TYPE_USER, USER_CMD_GET_ACL_NUM, frcore_cmd_acl_get_num);

    return 0;
}


int frcore_acl_process(cvmx_wqe_t *work, uint8_t *eth_ptr, uint8_t *ip_ptr,
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

    /* acl switch */
    if (!acl_enable)
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
            FRCORE_STAT_INC(stat_acl_malformed_packet);
            return FRCORE_ACT_DROP;
        }
    }

    tag = filter_lookup_by_tuple(tuple);

    FRCORE_TwoTuple("tag=%d\n", tag);
    if (tag)
    {
        FRCORE_STAT_INC(stat_acl_matched_pkts);
        FRCORE_STAT_ADD(stat_acl_matched_bytes, work->len);
        /* add pktnum and pkt bytes statustics */
        pkt_len = work->len;;
        MC_PRINTF_INFO("pkt_len: %d\n");
        /* add pkt_num */
        filter_statistic_hit(tag);
        /* add pkt_byte */
        filter_pkt_bytes_add(tag, pkt_len);

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

        rv = frcore_forward_rule_pkt_to_fifo(&tag, &hdr, &info, eth_ptr);
        FRCORE_TwoTuple("frcore_forward_simple_pkt_to_fifo return %d.\n", rv);
        if (rv)
        {
            //FRCORE_STAT_INC(stat_dma_errors);
        }
    }
    else
    {
        FRCORE_STAT_INC(stat_acl_not_matched_pkts);
        FRCORE_STAT_ADD(stat_acl_not_matched_bytes, work->len);
    }

    return rv;
}
#endif /* end of FRC_CONFIG_TWO_TUPLE */

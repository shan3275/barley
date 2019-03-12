#include "frcore_vlan_check.h"

#if FRC_CONFIG_VLAN_CHECK
CVMX_SHARED int64_t vlan_id_stat[stat_vlan_id_max]= {0};
CVMX_SHARED uint16_t vlan_id_start = 1024;
CVMX_SHARED uint16_t  vlan_id_num   = 128;
CVMX_SHARED uint8_t  vlan_check_hash_type = 1;


#if FRC_CONFIG_VLAN_IV

CVMX_SHARED uint32_t hash_sip4_mask = 0xffffffff;
CVMX_SHARED uint32_t hash_dip4_mask = 0xffffffff;
CVMX_SHARED bcm_ip6_t hash_sip6_mask = {0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff};
CVMX_SHARED bcm_ip6_t hash_dip6_mask = {0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff};

#endif

/**
 *
 *
 * @author shan (7/10/2013)
 *
 * @param sip
 * @param dip
 *
 * @return uint8_t
 */
static inline uint8_t vlan_check_hash_crc(uint32_t sip, uint32_t dip)
{
    int idx;
    #if 0
    CVMX_MT_CRC_POLYNOMIAL (0x1edc6f41);
    CVMX_MT_CRC_IV (0);
    CVMX_MT_CRC_WORD (sip);
    CVMX_MT_CRC_WORD (dip);
    CVMX_MF_CRC_IV (idx);
    #else
    idx = sip % 0xFF;
    #endif
    return (uint8_t)(idx & 0xFF);
}

/**
 *
 *
 * @author shan (7/10/2013)
 *
 * @param vid
 */
void stat_vlan_check(uint8_t vid, uint64_t port)
{
    FRCORE_PORT_VLAN_STAT_INC(port, (xe0_stat_vlan_id_0 + vid));
    FRCORE_PORT_VLAN_STAT_INC(port, xe0_stat_vlan_id_total);
}

/**
 *
 *
 * @author shan (7/10/2013)
 *
 * @param work
 * @param eth_ptr
 * @param ip_ptr
 * @param tcp_ptr
 * @param ip_paylen
 * @param smac
 * @param dmac
 *
 * @return int
 */
int frcore_vlan_check_v4(cvmx_wqe_t *work, uint8_t *eth_ptr, uint8_t *ip_ptr, uint64_t smac, uint64_t dmac)
{
    int eth_len;
    uint16_t vlan_id, vid;
    uint32_t id_check;
    ip4_pkt_t ip4_pkt;

    UP32(ip_ptr + 12, ip4_pkt.sip_data);
    UP32(ip_ptr + 16, ip4_pkt.dip_data);

    UP16(eth_ptr + 14, vlan_id);
    vlan_id &= 0xfff;
    eth_len          = work->len;

    //printf("vlan_id = 0x%x\n", vlan_id);
    id_check = ip4_calc_vlan(&ip4_pkt, vlan_check_hash_type, vlan_id_num, vlan_id_start);
    //printf("id_check = 0x%x\n", id_check);

    if (id_check == vlan_id ) {
        vid = vlan_id - vlan_id_start;
        stat_vlan_check(vid, work->ipprt);
    }else {
        FRCORE_PORT_VLAN_STAT_INC(work->ipprt, xe0_stat_vlan_id_error);
    }


    return FRCORE_ACT_FORWARD;
}

/**
 *
 *
 * @author shan (7/10/2013)
 *
 * @param work
 * @param eth_ptr
 * @param ip_ptr
 * @param tcp_ptr
 * @param ip_paylen
 * @param smac
 * @param dmac
 *
 * @return int
 */
int frcore_vlan_check_v6(cvmx_wqe_t *work, uint8_t *eth_ptr, uint8_t *ip_ptr, uint64_t smac, uint64_t dmac)
{
    int rv;
    int eth_len;
    uint16_t vlan_id, vid;
    uint16_t id_check;
    struct ipv6hdr *hdr;
    ip6_pkt_t ip6_pkt;

    UP16(eth_ptr + 14, vlan_id);
    vlan_id &= 0xfff;
    eth_len          = work->len;

    hdr = (struct ipv6hdr *)ip_ptr;
    memcpy(ip6_pkt.sip6_data, hdr->saddr.s6_addr, sizeof(bcm_ip6_t));
    memcpy(ip6_pkt.dip6_data, hdr->daddr.s6_addr, sizeof(bcm_ip6_t));
    #if 0
    FRCORE_VLAN_CHECK("IPV6 HDR:\n");
    FRCORE_VLAN_CHECK("version:%d priority:%d\n", hdr->version, hdr->priority);
    FRCORE_VLAN_CHECK("payload_len:%d nexthdr:%d hop_limit:%d\n", hdr->payload_len, hdr->nexthdr, hdr->hop_limit);
    FRCORE_VLAN_CHECK("sip: %.4x:%.4x:%.4x:%.4x:%.4x:%.4x:%.4x:%.4x\n", hdr->saddr.s6_addr16[0],hdr->saddr.s6_addr16[1],
                      hdr->saddr.s6_addr16[2], hdr->saddr.s6_addr16[3], hdr->saddr.s6_addr16[4],hdr->saddr.s6_addr16[5],
                      hdr->saddr.s6_addr16[6], hdr->saddr.s6_addr16[7]);
    FRCORE_VLAN_CHECK("dip: %.4x:%.4x:%.4x:%.4x:%.4x:%.4x:%.4x:%.4x\n", hdr->daddr.s6_addr16[0],hdr->daddr.s6_addr16[1],
                      hdr->daddr.s6_addr16[2], hdr->daddr.s6_addr16[3], hdr->daddr.s6_addr16[4],hdr->daddr.s6_addr16[5],
                      hdr->daddr.s6_addr16[6], hdr->daddr.s6_addr16[7]);
    FRCORE_VLAN_CHECK("ip6_pkt:sip: %.2x%.2x:%.2x%.2x:%.2x%.2x:%.2x%.2x:%.2x%.2x:%.2x%.2x:%.2x%.2x:%.2x%.2x\n",
                      ip6_pkt.sip6_data[0], ip6_pkt.sip6_data[1], ip6_pkt.sip6_data[2], ip6_pkt.sip6_data[3],
                      ip6_pkt.sip6_data[4], ip6_pkt.sip6_data[5], ip6_pkt.sip6_data[6], ip6_pkt.sip6_data[7],
                      ip6_pkt.sip6_data[8], ip6_pkt.sip6_data[9], ip6_pkt.sip6_data[10], ip6_pkt.sip6_data[11],
                      ip6_pkt.sip6_data[12], ip6_pkt.sip6_data[13], ip6_pkt.sip6_data[14], ip6_pkt.sip6_data[15]);
    FRCORE_VLAN_CHECK("ip6_pkt:dip: %.2x%.2x:%.2x%.2x:%.2x%.2x:%.2x%.2x:%.2x%.2x:%.2x%.2x:%.2x%.2x:%.2x%.2x\n",
                      ip6_pkt.dip6_data[0], ip6_pkt.dip6_data[1], ip6_pkt.dip6_data[2], ip6_pkt.dip6_data[3],
                      ip6_pkt.dip6_data[4], ip6_pkt.dip6_data[5], ip6_pkt.dip6_data[6], ip6_pkt.dip6_data[7],
                      ip6_pkt.dip6_data[8], ip6_pkt.dip6_data[9], ip6_pkt.dip6_data[10], ip6_pkt.dip6_data[11],
                      ip6_pkt.dip6_data[12], ip6_pkt.dip6_data[13], ip6_pkt.dip6_data[14], ip6_pkt.dip6_data[15]);
    #endif
    id_check = ip6_calc_vlan(&ip6_pkt, vlan_check_hash_type, vlan_id_num, vlan_id_start);

    if (id_check == vlan_id ) {
        vid = vlan_id - vlan_id_start;
        stat_vlan_check(vid, work->ipprt);
    }else {
        FRCORE_PORT_VLAN_STAT_INC(work->ipprt, xe0_stat_vlan_id_error);
    }


    return FRCORE_ACT_FORWARD;
}


#if FRC_CONFIG_VLAN_IV
int frcore_cmd_vlan_v4_set_hash_mask(uint16_t plen, void *param, uint16_t *olen, void *outbuf)
{
    frc_vlan_hash_mask_t *vlan_hash_mask = (frc_vlan_hash_mask_t *)param;

    printf("IPV4: 0x%x, type = %d\n", vlan_hash_mask->ip4, vlan_hash_mask->hash_type);
    switch (vlan_hash_mask->hash_type)
    {
        case FRC_VLAN_CHECK_SIP:
            hash_sip4_mask = vlan_hash_mask->ip4;
            break;
        case FRC_VLAN_CHECK_DIP:
            hash_dip4_mask = vlan_hash_mask->ip4;
            break;
        default:
            return FRE_PARAM;

    }
    printf("%s.%d\n", __func__, __LINE__);
    return FRE_SUCCESS;
}

int frcore_cmd_vlan_v6_set_hash_mask(uint16_t plen, void *param, uint16_t *olen, void *outbuf)
{
    int i;
    frc_vlan_hash_mask_t *vlan_hash_mask = (frc_vlan_hash_mask_t *)param;
    swap_buff(16 >> 3, vlan_hash_mask->ip6);

    for (i = 0; i < 16; i++)
    {
        printf("IPV6(%d):0x%x\n", i, vlan_hash_mask->ip6[i]);
    }
    printf("\n");
    switch (vlan_hash_mask->hash_type)
    {
        case FRC_VLAN_CHECK_SIP:
            memcpy(hash_sip6_mask, vlan_hash_mask->ip6, 16);
            break;
        case FRC_VLAN_CHECK_DIP:
            memcpy(hash_dip6_mask, vlan_hash_mask->ip6, 16);
            break;
        default:
            return FRE_PARAM;

    }
    return FRE_SUCCESS;
}

#endif





/**
 *
 *
 * @author shan (7/11/2013)
 *
 * @param plen
 * @param param
 * @param olen
 * @param outbuf
 *
 * @return int
 */
int frcore_cmd_vlan_check_get_para(uint16_t plen, void *param, uint16_t *olen, void *outbuf)
{
    frc_vlan_check_para_t *vlan_check_para = (frc_vlan_check_para_t *)outbuf;
    vlan_check_para->type     = vlan_check_hash_type;
    vlan_check_para->start_id = vlan_id_start;
    vlan_check_para->num      = vlan_id_num;
    FRCORE_VLAN_CHECK("===========================================================\n");
    FRCORE_VLAN_CHECK("type     =%d\n", vlan_check_hash_type);
    FRCORE_VLAN_CHECK("start_id =%d\n", vlan_id_start);
    FRCORE_VLAN_CHECK("num      =%d\n", vlan_id_num);

    *olen = sizeof(frc_vlan_check_para_t);
    return FRE_SUCCESS;
}

/**
 *
 *
 * @author shan (7/11/2013)
 *
 * @param plen
 * @param param
 * @param olen
 * @param outbuf
 *
 * @return int
 */
int frcore_cmd_vlan_check_set_para(uint16_t plen, void *param, uint16_t *olen, void *outbuf)
{
    frc_vlan_check_para_t *vlan_check_para = (frc_vlan_check_para_t *)param;
    vlan_check_hash_type = vlan_check_para->type;
    vlan_id_start        = vlan_check_para->start_id;
    vlan_id_num          = vlan_check_para->num;
    FRCORE_VLAN_CHECK("===========================================================\n");
    FRCORE_VLAN_CHECK("type     =%d\n", vlan_check_hash_type);
    FRCORE_VLAN_CHECK("start_id =%d\n", vlan_id_start);
    FRCORE_VLAN_CHECK("num      =%d\n", vlan_id_num);

    return FRE_SUCCESS;
}

int frcore_cmd_vlan_check_get_stat(uint16_t plen, void *param, uint16_t *olen, void *outbuf)
{
    int i;
    frc_vlan_op_in_t *input = (frc_vlan_op_in_t *) param;
    uint64_t *v64p;
    int cnt_offset = 0;

    //FRCORE_VLAN_CHECK("plen %d, param %p, *olne %d, outbuf %p.\n", plen, param, *olen, outbuf);

    if (0 == input->port)
    {
        cnt_offset = 0;
    }
    if (16 == input->port)
    {
        cnt_offset = stat_vlan_id_max / 2;
    }
    v64p = outbuf;
    for (i = 0; i < input->num; i++, v64p++)
    {
        *v64p = FRCORE_VLAN_STAT_VAL(input->index + i + cnt_offset);
    }

    *olen = input->num * sizeof(uint64_t);

    return 0;
}

void frcore_vlan_check_stat_clear()
{
    int i;

    for (i = 0; i < stat_vlan_id_max; i++)
    {
        FRCORE_VLAN_STAT_CLEAR(i);
    }
}
int frcore_cmd_vlan_check_clear_stat(uint16_t plen, void *param, uint16_t *olen, void *outbuf)
{
    *olen = 0;
    frcore_vlan_check_stat_clear();
    return FRE_SUCCESS;
}
/**
 *
 *
 * @author shan (7/11/2013)
 *
 * @return int
 */  
int
frcore_vlan_check_init()
{
    FRCORE_REGISTER_CMD(CMD_TYPE_USER, USER_CMD_SET_VLAN_CHECK_PARAMETER,  frcore_cmd_vlan_check_set_para);
    FRCORE_REGISTER_CMD(CMD_TYPE_USER, USER_CMD_GET_VLAN_CHECK_PARAMETER,  frcore_cmd_vlan_check_get_para);
    FRCORE_REGISTER_CMD(CMD_TYPE_USER, USER_CMD_GET_VLAN_CHECK_STAT,       frcore_cmd_vlan_check_get_stat);
    FRCORE_REGISTER_CMD(CMD_TYPE_USER, USER_CMD_CLEAR_VLAN_CHECK_STAT,     frcore_cmd_vlan_check_clear_stat);
#if FRC_CONFIG_VLAN_IV
    FRCORE_REGISTER_CMD(CMD_TYPE_USER, USER_CMD_VLAN_V4_HASH_MASK_SET,     frcore_cmd_vlan_v4_set_hash_mask);
    FRCORE_REGISTER_CMD(CMD_TYPE_USER, USER_CMD_VLAN_V6_HASH_MASK_SET,     frcore_cmd_vlan_v6_set_hash_mask);
#endif

    return 0;
}
#endif


/* End of file */

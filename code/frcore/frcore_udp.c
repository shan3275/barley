#include "frcore_pkt.h"

#include "frcore_udp.h"
#include "frcore_stat.h"

#include "frc_dma.h"
#include "frcore_chan.h"

#include "frcore.h"
#include "frcore_cmd.h"
#include "frc_types.h"
static CVMX_SHARED uint8_t udp_enable = 1;

#if FRC_CONFIG_UDP
int frcore_udp_process(cvmx_wqe_t *work, uint8_t *eth_ptr, uint8_t *ip_ptr, uint8_t *udp_ptr, uint16_t ip_paylen, uint64_t smac, uint64_t dmac)
{
    int rv;
    frc_dma_hdr_t hdr;
    frc_dma_pkt_info_t info;
    int eth_len;

    //FRCORE_CYCLE_RECORDING();

    /* udp switch */
    if (!udp_enable)
    {
        return FRCORE_ACT_FORWARD;
    }

    memset(&hdr, 0, sizeof(frc_dma_hdr_t));
    memset(&info, 0, sizeof(frc_dma_pkt_info_t));

    UP32(ip_ptr + 12, hdr.sip);

    UP32(ip_ptr + 16, hdr.dip);

    UP16(udp_ptr, hdr.sport);
    UP16(udp_ptr + 2, hdr.dport);

    eth_len          = work->len;

    hdr.protocol     = PROTO_UDP;
    hdr.hash         = work->tag & 0xffffff;
    //hdr.info_offset  = FRC_DMA_PKT_INFO_OFFSET;
    hdr.pkt_num      = 1;
    hdr.total_paylen = eth_len;

    info.data_offset = (uint32_t)(udp_ptr - eth_ptr + 8); // udp head length 8 bytes
    info.payload_len = ip_paylen - 8; // udp head length 8 bytes
    info.smac        = smac;
    info.dmac        = dmac;

    rv = frcore_forward_simple_pkt_to_fifo(FRC_WORK_UDP, &hdr, &info, eth_ptr);
    FRCORE_UDP("frcore_forward_simple_pkt_to_fifo return %d.\n", rv);
    if (rv != FRE_SUCCESS)
    {
        //FRCORE_STAT_INC(stat_dma_errors);
    }

    else
    {
        FRCORE_STAT_INC(stat_udp_submit_pkts);
        FRCORE_STAT_ADD(stat_udp_submit_bytes, work->len);
    }

    return FRCORE_ACT_FORWARD;
}

int frcore_cmd_udp_enable(uint16_t plen, void *param, uint16_t *olen, void *outbuf)
{
    uint8_t *enable = (uint8_t *)param;
    udp_enable = *enable;
    FRCORE_UDP("===========================================================\n");
    printf("udp_enable=%d\n", udp_enable);

    return FRE_SUCCESS;
}

int frcore_cmd_udp_status_get(uint16_t plen, void *param, uint16_t *olen, void *outbuf)
{
    uint8_t enable;
    enable = udp_enable;
    memcpy(outbuf, &enable, 1);
    *olen = 1;

    return FRE_SUCCESS;
}

int
frcore_udp_init()
{
    FRCORE_REGISTER_CMD(CMD_TYPE_USER, USER_CMD_UDP_ENABLE,  frcore_cmd_udp_enable);
    FRCORE_REGISTER_CMD(CMD_TYPE_USER, USER_CMD_GET_UDP_STATUS,  frcore_cmd_udp_status_get);

    return FRE_SUCCESS;
}
#endif


/* End of file */

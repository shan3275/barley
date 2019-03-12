#ifndef __FRC_UTIL_H__
#define __FRC_UTIL_H__

#if !__KERNEL__
#include <stdio.h>
#include <string.h>
#endif

#include "frc_defs.h"
#include "frc_dma.h"
//#include "frc_api.h"
#define MAC_FMT_STR          "%.2x:%.2x:%.2x:%.2x:%.2x:%.2x"
#define MAC_PARGS(_mac)      _mac[0],_mac[1],_mac[2],_mac[3],_mac[4],_mac[5]
#define MAC_CMP(_mac1, _mac2) (memcmp(_mac1, _mac2, sizeof(mac_t)) == 0)
#define MAC_CPY(_mac1, _mac2) memcpy(_mac1, _mac2, sizeof(mac_t))
#define MAC_BROADCAST(_mac) (_mac[0] == 0xff && \
                             _mac[1] == 0xff && \
                             _mac[2] == 0xff && \
                             _mac[3] == 0xff && \
                             _mac[4] == 0xff && \
                             _mac[5] == 0xff)


#define BIT(X)      (1 << (X))
#define BITV(X, B)  (((X) & BIT(B)) == BIT(B))
#define BITH(X, B)  ((X) |= BIT(B))
#define BITL(X, B)  ((X) &= ~BIT(B))

#define FRC_64_TO_32_LO(dst, src)   ((dst) = (uint32_t) (src))
#define FRC_64_TO_32_HI(dst, src)   ((dst) = (uint32_t) ((src) >> 32))
#define FRC_64_HI(src)              ((uint32_t) ((src) >> 32))
#define FRC_64_LO(src)              ((uint32_t) (src))
#define FRC_64_ZERO(dst)            ((dst) = 0)
#define FRC_64_IS_ZERO(src)     ((src) == 0)

#define FRC_64_SET(dst, src_hi, src_lo)             \
    ((dst) = (((uint64_t) ((uint32_t)(src_hi))) << 32) | ((uint64_t) ((uint32_t)(src_lo))))

#define DUMP_LINE_SIZE      128

#define _DUMP_FIELD32(_var, _field) \
    frc_print("  %16s:  0x%.8x\n", #_field, _var->_field)

#define _DUMP_FIELD64(_var, _field) \
    frc_print("  %16s:  0x%.16llx\n", #_field, (ULL)(_var->_field))

static inline void frc_dump_dma_hdr(frc_dma_hdr_t *hdr)
{
    //_DUMP_FIELD32(hdr, index);
    //_DUMP_FIELD32(hdr, type);
    //_DUMP_FIELD32(hdr, state);
    //_DUMP_FIELD32(hdr, info_offset);

    //_DUMP_FIELD32(hdr, start_sec);
    //_DUMP_FIELD32(hdr, start_usec);

    _DUMP_FIELD32(hdr, stop_sec);
    //_DUMP_FIELD32(hdr, stop_usec);

    _DUMP_FIELD32(hdr, sip);
    _DUMP_FIELD32(hdr, dip);

    _DUMP_FIELD32(hdr, sport);
    _DUMP_FIELD32(hdr, dport);
    _DUMP_FIELD32(hdr, protocol);
    _DUMP_FIELD32(hdr, pkt_num);

    _DUMP_FIELD32(hdr, total_paylen);
    _DUMP_FIELD32(hdr, hash);

    _DUMP_FIELD32(hdr, teid);
    _DUMP_FIELD32(hdr, pppoe_sess_id);

    _DUMP_FIELD32(hdr, pppoe_protocol);
    _DUMP_FIELD32(hdr, pppoe_paylen);
    _DUMP_FIELD32(hdr, vlan_id);
    _DUMP_FIELD32(hdr, vlan_type);

    //_DUMP_FIELD32(hdr, r1);
    //_DUMP_FIELD32(hdr, r2);
}

static inline void frc_dump_dma_pkt_info(frc_dma_pkt_info_t *info)
{
    _DUMP_FIELD32(info, sequence);
    _DUMP_FIELD32(info, ack_seq);

    _DUMP_FIELD32(info, data_offset);
    _DUMP_FIELD32(info, payload_len);
    _DUMP_FIELD32(info, direction);

    _DUMP_FIELD64(info, smac);
    _DUMP_FIELD64(info, dmac);
}

static inline void _frc_dump_buff(const char *func, int line_num, int len, uint8_t *buff)
{
    int i;
    char *p, line[DUMP_LINE_SIZE];

    p = line;
    frc_print("%s.%d: BUFF DUMP %d bytes:\n", func, line_num, len);
    for (i = 0; i < len; i++)
    {
        if ((i % 16) == 0)
        {

            memset(line, 0, DUMP_LINE_SIZE);
            p = line;
            p += sprintf(p, "%.6d:", i);
        }

        if ((i % 16) == 8)
        {
            p += sprintf(p, " -");
        }
        p += sprintf(p, " %.2x", buff[i]);

        if ((i % 16) == 15)
        {
            line[DUMP_LINE_SIZE - 1] = 0;
            frc_print("%s\n", line);
        }
    }
    if ((i % 16) != 0)
    {
        line[DUMP_LINE_SIZE - 1] = 0;
        frc_print("%s\n", line);
    }
    frc_print("\n");
}


static inline int
frc_stat_name_get(int index, char *name)
{

    char *stat_name[stat_max] = {
        "p0_rx_pkts               ",
        "p0_tx_pkts               ",
        "p0_rx_errs               ",
        "p0_rx_dropped            ",
        "p1_rx_pkts               ",
        "p1_tx_pkts               ",
        "p1_rx_errs               ",
        "p1_rx_dropped            ",
        "p1_rx_pko                ",
        "p0_PARTIAL_ERR           ",  /* RGM+SPI            1 = partially received packet (buffering/bandwidth not adequate) */
        "p0_JABBER_ERR            ",  /* RGM+SPI            2 = receive packet too large and truncated */
        "p0_OVER_FCS_ERR          ",  /* RGM                3 = max frame error (pkt len > max frame len) (with FCS error) */
        "p0_OVER_ERR              ",  /* RGM+SPI            4 = max frame error (pkt len > max frame len) */
        "p0_ALIGN_ERR             ",  /* RGM                5 = nibble error (data not byte multiple - 100M and 10M only) */
        "p0_UNDER_FCS_ERR         ",  /* RGM                6 = min frame error (pkt len < min frame len) (with FCS error) */
        "p0_GMX_FCS_ERR           ",  /* RGM                7 = FCS error */
        "p0_UNDER_ERR             ",  /* RGM+SPI            8 = min frame error (pkt len < min frame len) */
        "p0_EXTEND_ERR            ",  /* RGM                9 = Frame carrier extend error */
        "p0_LENGTH_ERR            ",  /* RGM               10 = length mismatch (len did not match len in L2 length/type) */
        "p0_DAT_ERR               ",  /* RGM               11 = Frame error (some or all data bits marked err) */
        "p0_SKIP_ERR              ",  /* RGM               12 = packet was not large enough to pass the skipper - no inspection could occur */
        "p0_NIBBLE_ERR            ",  /* RGM               13 = studder error (data not repeated - 100M and 10M only) */
        "p0_PIP_FCS               ",  /* RGM+SPI           16 = FCS error */
        "p0_PIP_SKIP_ERR          ",  /* RGM+SPI+PCI       17 = packet was not large enough to pass the skipper - no inspection could occur */
        "p0_PIP_L2_MAL_HDR        ",  /* RGM+SPI+PCI       18 = malformed l2 (packet not long enough to cover L2 hdr) */
        "p0_PUNY_ERR              ",  /* SGMII             47 = PUNY error (packet was 4B or less when FCS stripping is enabled) */
        "p0_UNKNOWN_ERR           ",

        "p1_PARTIAL_ERR           ",  /* RGM+SPI            1 = partially received packet (buffering/bandwidth not adequate) */
        "p1_JABBER_ERR            ",  /* RGM+SPI            2 = receive packet too large and truncated */
        "p1_OVER_FCS_ERR          ",  /* RGM                3 = max frame error (pkt len > max frame len) (with FCS error) */
        "p1_OVER_ERR              ",  /* RGM+SPI            4 = max frame error (pkt len > max frame len) */
        "p1_ALIGN_ERR             ",  /* RGM                5 = nibble error (data not byte multiple - 100M and 10M only) */
        "p1_UNDER_FCS_ERR         ",  /* RGM                6 = min frame error (pkt len < min frame len) (with FCS error) */
        "p1_GMX_FCS_ERR           ",  /* RGM                7 = FCS error */
        "p1_UNDER_ERR             ",  /* RGM+SPI            8 = min frame error (pkt len < min frame len) */
        "p1_EXTEND_ERR            ",  /* RGM                9 = Frame carrier extend error */
        "p1_LENGTH_ERR            ",  /* RGM               10 = length mismatch (len did not match len in L2 length/type) */
        "p1_DAT_ERR               ",  /* RGM               11 = Frame error (some or all data bits marked err) */
        "p1_SKIP_ERR              ",  /* RGM               12 = packet was not large enough to pass the skipper - no inspection could occur */
        "p1_NIBBLE_ERR            ",  /* RGM               13 = studder error (data not repeated - 100M and 10M only) */
        "p1_PIP_FCS               ",  /* RGM+SPI           16 = FCS error */
        "p1_PIP_SKIP_ERR          ",  /* RGM+SPI+PCI       17 = packet was not large enough to pass the skipper - no inspection could occur */
        "p1_PIP_L2_MAL_HDR        ",  /* RGM+SPI+PCI       18 = malformed l2 (packet not long enough to cover L2 hdr) */
        "p1_PUNY_ERR              ",  /* SGMII             47 = PUNY error (packet was 4B or less when FCS stripping is enabled) */
        "p1_UNKNOWN_ERR           ",

        "core0_pkt                ",
        "core1_pkt                ",
        "core2_pkt                ",
        "core3_pkt                ",
        "core4_pkt                ",
        "core5_pkt                ", /* 50*/
        "core6_pkt                ",
        "core7_pkt                ",
        "core8_pkt                ",
        "core9_pkt                ",
        "core10_pkt               ",
        "core11_pkt               ",  /* 56 */
        "work_free                ",
        "work_data                ",
        "work_stat                ",
        "work_queue               ", /* 10--60*/
        "work_pkt                 ",
        "work_cmd                 ",
        "work_instr               ",
        "work_unkown              ",
        "chan_timer               ",
        "act_debug                ",
        "act_drops                ",
        "act_free                 ",
        "act_forward              ",
        "act_delay                ",
        "act_unfree               ",    /* 70 */
        "act_unkown               ",
        "pkts_below_64            ",
        "pkts_64_128              ",
        "pkts_128_256             ",
        "pkts_256_512             ",
        "pkts_512_1024            ",
        "pkts_1024_1500           ",
        "pkts_1500_1600           ",
        "pkts_above_1600          ",
        "very_short_pkts          ",  /* 80 */
        "very_long_pkts           ",
        "normal_pkts              ",
        "tx_pkts                  ",
        "tx_bytes                 ",
        "tx_errs                  ",
        "rx_work                  ",
        "rx_errs                  ",
        "rx_pkts                  ",
        "rx_bytes                 ",
        "drop_port_off            ", /* 90 */
        "drop_rx_error            ",
        "not_ip                   ",
        "mpls                     ",
        "pppoe                    ",
        "drop_not_ip              ",
        "ip_pkts                  ",  /* 96 */
        "ip_v6                    ",
        "drop_ipv6_pkts           ",
        "ipv4_pkts                ",
        "vlan                     ", /* 100 */
        "drop_vlan                ",
        "ip_frag                  ",
        "drop_ip_frag             ",
        "ip_not_tcp_udp           ",
        "drop_not_tcp_udp         ",
        "ip_option                ",  /* 106 */
        "drop_ip_option           ",
        "ip_other                 ",
        "drop_ip_other            ",
        "tcp_pkts                 ", /* 110 */
        "tcp_malformed_packet     ",
        "tcp_option_pkts          ",
        "tcp_payload_zero_pkts    ",
        "total_ssn                ",
        "fail_created_ssn         ",
        "ssn_fail_fin_or_rst      ",
        "ssn_fail_overflow        ",
        "ssn_fpa_alloc_err        ",
        "ssn_fpa_failed           ",
        "ssn_active_num           ",   /* 120 */
        "ssn_fwd_tcpflowrec       ",
        "ssn_drop_packet          ",
        "tcpflowrec_retrans       ",
        "tcpflowrec_retrans_drop  ",
        "tcpflowrec_part_retrans  ",
        "tcpflowrec_dup_remove    ",
        "tcpflowrec_queue_overflow",
        "tcpflowrec_mem_failed    ",
        "ssn_age_wqe              ",
        "ssn_tim_addok            ",  /* 130 */
        "ssn_tim_fired            ",
        "ssn_tim_failed           ",
        "ssn_prepared_by_fpa_alloc",
        "ssn_purge_to_fpa         ",
        "ssn_purge_unknown        ",
        "ssn_fpa_free_err         ",
        "ssn_purge                ",
        "ssn_dma_errors           ",
        "ssn_dma_pkts             ",
        "ssn_dma_bytes            ",
        "ssn_dma_pkt_infos        ", /* 141 */
        "ssn_dma_blocks           ",
        "ssn_1packet_blocks       ",
        "ssn_2packets_blocks      ",
        "ssn_3packets_blocks      ",
        "ssn_4packets_blocks      ",
        "ssn_5packets_blocks      ",
        "ssn_above_5packets_blocks",
        "ssn_queue_no_space       ",
        "ssn_dma_enqueue_pkts     ",
        "ssn_dma_enqueue_errs     ",
        "ssn_dma_head_fail        ",
        "ssn_avail_get            ",
        "ssn_avail_get_err        ",
        "ssn_avail_get_no_space   ",
        "ssn_compl_put            ",
        "ssn_compl_put_err        ",
        "ssn_compl_put_no_space   ",
        "ssn_pkt_post             ",
        "ssn_pkt_post_err         ",
        "ssn_post_err_pkts        ",
        "udp_pkts                 ",
        "udp_malformed_packet     ",
        "udp_sumbit_pkts          ", /* 158 */
        "udp_sumbit_bytes         ",
        "rule_malformed_packet    ",
        "matched_rule_pkts        ",
        "matched_rule_bytes       ",
        "not_matched_rule_pkts    ",
        "not_matched_rule_bytes   ",
        "pkt_oversize             ",
        "dma_errors               ",
        "dma_pkts                 ",
        "dma_pkt_infos            ", /* 168 */
        "dma_bytes                ",
        "dma_blocks               ",
        "avail_get                ",
        "avail_get_err            ",
        "avail_get_no_space       ",
        "compl_put                ",
        "compl_put_err            ",
        "compl_put_no_space       ",
        "pkt_post                 ",
        "pkt_post_err             ", /* 178 */
        "post_err_pkts            ",
        "queue_post               ",
        "queue_post_err           ",
        "dma_loop_bytes           ",
        "dma_loop_entries         ",
        "dma_loop_errs            ",
        "dma_block_addr_errs      ",
        "dma_enqueue_pkts         ",
        "dma_enqueue_errs         ",
        "get_avail_bytes          ", /* 188 */
        "put_compl_bytes          ",
        "queue_no_space           ",
        #if FRC_CONFIG_TWO_TUPLE
        "acl_malformed_packet     ", /* 191*/
        "acl_type_error           ",
        "acl_hash_error           ",
        "acl_entry_exception      ",
        "acl_not_found_index      ",
        "acl_del_hash_cell        ",
        "acl_del_rule_num         ",
        "acl_add_hash_cell        ",
        "acl_add_rule_num         ",
        "acl_hash_cell_alloc_fail ",
        "matched_acl_pkts         ",
        "matched_acl_bytes        ",
        "not_matched_acl_pkts     ",
        "not_matched_acl_bytes    ",
        #endif
        "ssn_fifo_lock            ",
        "ssn_fifo_unlock          ",
#ifdef FRC_CONFIG_MAC_STATISTICS
	"swap_mac_send_pkts       ",
    	"heart_beat_pkts          ",
    	"heart_beat_send_back_pkts",
    	"heart_beat_on_drop_pkts  ",
    	"heart_beat_drop_pkts     ",
#endif
#ifdef FRC_CONFIG_IPC
        "drop_smac_zero"        ,
        "smac_ordered",
        "smac_disorder" ,
        "pktid_ordered",
        "pktid_disorder",
        "pktid_repeat",
        "head_valid",
        "head_invalid",
        "inner_vlan",
#endif
    };



    if (index >= stat_max)
    {
        return 1;
    }

    strcpy(name, stat_name[index]);

    return 0;
}

#if FRC_CONFIG_VLAN_CHECK
static inline int
frc_vlan_stat_name_get(int index, char *name)
{

    char *stat_name[stat_vlan_id_max/2] = {
        "rx_pkts                  ",
        "rx_bytes                 ",
        "vlan_id_total            ",
        "vlan_id_error            ",
        "vlan_id_0                ",
        "vlan_id_1                ",
        "vlan_id_2                ",
        "vlan_id_3                ",
        "vlan_id_4                ",
        "vlan_id_5                ",
        "vlan_id_6                ",
        "vlan_id_7                ",
        "vlan_id_8                ",
        "vlan_id_9                ",
        "vlan_id_10               ",
        "vlan_id_11               ",
        "vlan_id_12               ",
        "vlan_id_13               ",
        "vlan_id_14               ",
        "vlan_id_15               ",
        "vlan_id_16               ",
        "vlan_id_17               ",
        "vlan_id_18               ",
        "vlan_id_19               ",
        "vlan_id_20               ",
        "vlan_id_21               ",
        "vlan_id_22               ",
        "vlan_id_23               ",
        "vlan_id_24               ",
        "vlan_id_25               ",
        "vlan_id_26               ",
        "vlan_id_27               ",
        "vlan_id_28               ",
        "vlan_id_29               ",
        "vlan_id_30               ",
        "vlan_id_31               ",
        "vlan_id_32               ",
        "vlan_id_33               ",
        "vlan_id_34               ",
        "vlan_id_35               ",
        "vlan_id_36               ",
        "vlan_id_37               ",
        "vlan_id_38               ",
        "vlan_id_39               ",
        "vlan_id_40               ",
        "vlan_id_41               ",
        "vlan_id_42               ",
        "vlan_id_43               ",
        "vlan_id_44               ",
        "vlan_id_45               ",
        "vlan_id_46               ",
        "vlan_id_47               ",
        "vlan_id_48               ",
        "vlan_id_49               ",
        "vlan_id_50               ",
        "vlan_id_51               ",
        "vlan_id_52               ",
        "vlan_id_53               ",
        "vlan_id_54               ",
        "vlan_id_55               ",
        "vlan_id_56               ",
        "vlan_id_57               ",
        "vlan_id_58               ",
        "vlan_id_59               ",
        "vlan_id_60               ",
        "vlan_id_61               ",
        "vlan_id_62               ",
        "vlan_id_63               ",
        "vlan_id_64               ",
        "vlan_id_65               ",
        "vlan_id_66               ",
        "vlan_id_67               ",
        "vlan_id_68               ",
        "vlan_id_69               ",
        "vlan_id_70               ",
        "vlan_id_71               ",
        "vlan_id_72               ",
        "vlan_id_73               ",
        "vlan_id_74               ",
        "vlan_id_75               ",
        "vlan_id_76               ",
        "vlan_id_77               ",
        "vlan_id_78               ",
        "vlan_id_79               ",
        "vlan_id_80               ",
        "vlan_id_81               ",
        "vlan_id_82               ",
        "vlan_id_83               ",
        "vlan_id_84               ",
        "vlan_id_85               ",
        "vlan_id_86               ",
        "vlan_id_87               ",
        "vlan_id_88               ",
        "vlan_id_89               ",
        "vlan_id_90               ",
        "vlan_id_91               ",
        "vlan_id_92               ",
        "vlan_id_93               ",
        "vlan_id_94               ",
        "vlan_id_95               ",
        "vlan_id_96               ",
        "vlan_id_97               ",
        "vlan_id_98               ",
        "vlan_id_99               ",
        "vlan_id_100              ",
        "vlan_id_101              ",
        "vlan_id_102              ",
        "vlan_id_103              ",
        "vlan_id_104              ",
        "vlan_id_105              ",
        "vlan_id_106              ",
        "vlan_id_107              ",
        "vlan_id_108              ",
        "vlan_id_109              ",
        "vlan_id_110              ",
        "vlan_id_111              ",
        "vlan_id_112              ",
        "vlan_id_113              ",
        "vlan_id_114              ",
        "vlan_id_115              ",
        "vlan_id_116              ",
        "vlan_id_117              ",
        "vlan_id_118              ",
        "vlan_id_119              ",
        "vlan_id_120              ",
        "vlan_id_121              ",
        "vlan_id_122              ",
        "vlan_id_123              ",
        "vlan_id_124              ",
        "vlan_id_125              ",
        "vlan_id_126              ",
        "vlan_id_127              ",
        "vlan_id_128              ",
        "vlan_id_129              ",
        "vlan_id_130              ",
        "vlan_id_131              ",
        "vlan_id_132              ",
        "vlan_id_133              ",
        "vlan_id_134              ",
        "vlan_id_135              ",
        "vlan_id_136              ",
        "vlan_id_137              ",
        "vlan_id_138              ",
        "vlan_id_139              ",
        "vlan_id_140              ",
        "vlan_id_141              ",
        "vlan_id_142              ",
        "vlan_id_143              ",
        "vlan_id_144              ",
        "vlan_id_145              ",
        "vlan_id_146              ",
        "vlan_id_147              ",
        "vlan_id_148              ",
        "vlan_id_149              ",
        "vlan_id_150              ",
        "vlan_id_151              ",
        "vlan_id_152              ",
        "vlan_id_153              ",
        "vlan_id_154              ",
        "vlan_id_155              ",
        "vlan_id_156              ",
        "vlan_id_157              ",
        "vlan_id_158              ",
        "vlan_id_159              ",
        "vlan_id_160              ",
        "vlan_id_161              ",
        "vlan_id_162              ",
        "vlan_id_163              ",
        "vlan_id_164              ",
        "vlan_id_165              ",
        "vlan_id_166              ",
        "vlan_id_167              ",
        "vlan_id_168              ",
        "vlan_id_169              ",
        "vlan_id_170              ",
        "vlan_id_171              ",
        "vlan_id_172              ",
        "vlan_id_173              ",
        "vlan_id_174              ",
        "vlan_id_175              ",
        "vlan_id_176              ",
        "vlan_id_177              ",
        "vlan_id_178              ",
        "vlan_id_179              ",
        "vlan_id_180              ",
        "vlan_id_181              ",
        "vlan_id_182              ",
        "vlan_id_183              ",
        "vlan_id_184              ",
        "vlan_id_185              ",
        "vlan_id_186              ",
        "vlan_id_187              ",
        "vlan_id_188              ",
        "vlan_id_189              ",
        "vlan_id_190              ",
        "vlan_id_191              ",
        "vlan_id_192              ",
        "vlan_id_193              ",
        "vlan_id_194              ",
        "vlan_id_195              ",
        "vlan_id_196              ",
        "vlan_id_197              ",
        "vlan_id_198              ",
        "vlan_id_199              ",
        "vlan_id_200              ",
        "vlan_id_201              ",
        "vlan_id_202              ",
        "vlan_id_203              ",
        "vlan_id_204              ",
        "vlan_id_205              ",
        "vlan_id_206              ",
        "vlan_id_207              ",
        "vlan_id_208              ",
        "vlan_id_209              ",
        "vlan_id_210              ",
        "vlan_id_211              ",
        "vlan_id_212              ",
        "vlan_id_213              ",
        "vlan_id_214              ",
        "vlan_id_215              ",
        "vlan_id_216              ",
        "vlan_id_217              ",
        "vlan_id_218              ",
        "vlan_id_219              ",
        "vlan_id_220              ",
        "vlan_id_221              ",
        "vlan_id_222              ",
        "vlan_id_223              ",
        "vlan_id_224              ",
        "vlan_id_225              ",
        "vlan_id_226              ",
        "vlan_id_227              ",
        "vlan_id_228              ",
        "vlan_id_229              ",
        "vlan_id_230              ",
        "vlan_id_231              ",
        "vlan_id_232              ",
        "vlan_id_233              ",
        "vlan_id_234              ",
        "vlan_id_235              ",
        "vlan_id_236              ",
        "vlan_id_237              ",
        "vlan_id_238              ",
        "vlan_id_239              ",
        "vlan_id_240              ",
        "vlan_id_241              ",
        "vlan_id_242              ",
        "vlan_id_243              ",
        "vlan_id_244              ",
        "vlan_id_245              ",
        "vlan_id_246              ",
        "vlan_id_247              ",
        "vlan_id_248              ",
        "vlan_id_249              ",
        "vlan_id_250              ",
        "vlan_id_251              ",
        "vlan_id_252              ",
        "vlan_id_253              ",
        "vlan_id_254              ",
        "vlan_id_255              ",
    };



    if (index >= stat_vlan_id_max)
    {
        return 1;
    }

    strcpy(name, stat_name[index]);

    return 0;
}
#endif /* end of FRC_CONFIG_VLAN_CHECK*/

#if FRC_CONFIG_TIMESTAMP_CHECK
static inline int
frc_timestamp_stat_name_get(int index, char *name)
{

    char *stat_name[stat_timestamp_max] = {
    "stat_rxx_pkts                       ",
    "stat_rxx_bytes                      ",
    "xe0_stat_rxx_pkts                   ",
    "xe0_stat_rxx_bytes                  ",
    "xe1_stat_rxx_pkts                   ",
    "xe1_stat_rxx_bytes                  ",

    "stat_port0_rxx_pkts                 ",
    "xe0_stat_port0_rxx_pkts             ",
    "xe0_stat_port0_year_invalid_pkts    ",
    "xe0_stat_port0_month_invalid_pkts   ",
    "xe0_stat_port0_day_invalid_pkts     ",
    "xe0_stat_port0_hour_invalid_pkts    ",
    "xe0_stat_port0_minute_invalid_pkts  ",
    "xe0_stat_port0_second_invalid_pkts  ",
    "xe0_stat_port0_ip_err_pkts          ",
    "xe0_stat_port0_timestamp_err_pkts   ",
    "xe1_stat_port0_rxx_pkts             ",
    "xe1_stat_port0_year_invalid_pkts    ",
    "xe1_stat_port0_month_invalid_pkts   ",
    "xe1_stat_port0_day_invalid_pkts     ",
    "xe1_stat_port0_hour_invalid_pkts    ",
    "xe1_stat_port0_minute_invalid_pkts  ",
    "xe1_stat_port0_second_invalid_pkts  ",
    "xe1_stat_port0_ip_err_pkts          ",
    "xe1_stat_port0_timestamp_err_pkts   ",

    "stat_port1_rxx_pkts                 ",
    "xe0_stat_port1_rxx_pkts             ",
    "xe0_stat_port1_year_invalid_pkts    ",
    "xe0_stat_port1_month_invalid_pkts   ",
    "xe0_stat_port1_day_invalid_pkts     ",
    "xe0_stat_port1_hour_invalid_pkts    ",
    "xe0_stat_port1_minute_invalid_pkts  ",
    "xe0_stat_port1_second_invalid_pkts  ",
    "xe0_stat_port1_ip_err_pkts          ",
    "xe0_stat_port1_timestamp_err_pkts   ",
    "xe1_stat_port1_rxx_pkts             ",
    "xe1_stat_port1_year_invalid_pkts    ",
    "xe1_stat_port1_month_invalid_pkts   ",
    "xe1_stat_port1_day_invalid_pkts     ",
    "xe1_stat_port1_hour_invalid_pkts    ",
    "xe1_stat_port1_minute_invalid_pkts  ",
    "xe1_stat_port1_second_invalid_pkts  ",
    "xe1_stat_port1_ip_err_pkts          ",
    "xe1_stat_port1_timestamp_err_pkts   ",

    "stat_port2_rxx_pkts                 ",
    "xe0_stat_port2_rxx_pkts             ",
    "xe0_stat_port2_year_invalid_pkts    ",
    "xe0_stat_port2_month_invalid_pkts   ",
    "xe0_stat_port2_day_invalid_pkts     ",
    "xe0_stat_port2_hour_invalid_pkts    ",
    "xe0_stat_port2_minute_invalid_pkts  ",
    "xe0_stat_port2_second_invalid_pkts  ",
    "xe0_stat_port2_ip_err_pkts          ",
    "xe0_stat_port2_timestamp_err_pkts   ",
    "xe1_stat_port2_rxx_pkts             ",
    "xe1_stat_port2_year_invalid_pkts    ",
    "xe1_stat_port2_month_invalid_pkts   ",
    "xe1_stat_port2_day_invalid_pkts     ",
    "xe1_stat_port2_hour_invalid_pkts    ",
    "xe1_stat_port2_minute_invalid_pkts  ",
    "xe1_stat_port2_second_invalid_pkts  ",
    "xe1_stat_port2_ip_err_pkts          ",
    "xe1_stat_port2_timestamp_err_pkts   ",

    "stat_port3_rxx_pkts                 ",
    "xe0_stat_port3_rxx_pkts             ",
    "xe0_stat_port3_year_invalid_pkts    ",
    "xe0_stat_port3_month_invalid_pkts   ",
    "xe0_stat_port3_day_invalid_pkts     ",
    "xe0_stat_port3_hour_invalid_pkts    ",
    "xe0_stat_port3_minute_invalid_pkts  ",
    "xe0_stat_port3_second_invalid_pkts  ",
    "xe0_stat_port3_ip_err_pkts          ",
    "xe0_stat_port3_timestamp_err_pkts   ",
    "xe1_stat_port3_rxx_pkts             ",
    "xe1_stat_port3_year_invalid_pkts    ",
    "xe1_stat_port3_month_invalid_pkts   ",
    "xe1_stat_port3_day_invalid_pkts     ",
    "xe1_stat_port3_hour_invalid_pkts    ",
    "xe1_stat_port3_minute_invalid_pkts  ",
    "xe1_stat_port3_second_invalid_pkts  ",
    "xe1_stat_port3_ip_err_pkts          ",
    "xe1_stat_port3_timestamp_err_pkts   ",

    "stat_port4_rxx_pkts                 ",
    "xe0_stat_port4_rxx_pkts             ",
    "xe0_stat_port4_year_invalid_pkts    ",
    "xe0_stat_port4_month_invalid_pkts   ",
    "xe0_stat_port4_day_invalid_pkts     ",
    "xe0_stat_port4_hour_invalid_pkts    ",
    "xe0_stat_port4_minute_invalid_pkts  ",
    "xe0_stat_port4_second_invalid_pkts  ",
    "xe0_stat_port4_ip_err_pkts          ",
    "xe0_stat_port4_timestamp_err_pkts   ",
    "xe1_stat_port4_rxx_pkts             ",
    "xe1_stat_port4_year_invalid_pkts    ",
    "xe1_stat_port4_month_invalid_pkts   ",
    "xe1_stat_port4_day_invalid_pkts     ",
    "xe1_stat_port4_hour_invalid_pkts    ",
    "xe1_stat_port4_minute_invalid_pkts  ",
    "xe1_stat_port4_second_invalid_pkts  ",
    "xe1_stat_port4_ip_err_pkts          ",
    "xe1_stat_port4_timestamp_err_pkts   ",

    "stat_port5_rxx_pkts                 ",
    "xe0_stat_port5_rxx_pkts             ",
    "xe0_stat_port5_year_invalid_pkts    ",
    "xe0_stat_port5_month_invalid_pkts   ",
    "xe0_stat_port5_day_invalid_pkts     ",
    "xe0_stat_port5_hour_invalid_pkts    ",
    "xe0_stat_port5_minute_invalid_pkts  ",
    "xe0_stat_port5_second_invalid_pkts  ",
    "xe0_stat_port5_ip_err_pkts          ",
    "xe0_stat_port5_timestamp_err_pkts   ",
    "xe1_stat_port5_rxx_pkts             ",
    "xe1_stat_port5_year_invalid_pkts    ",
    "xe1_stat_port5_month_invalid_pkts   ",
    "xe1_stat_port5_day_invalid_pkts     ",
    "xe1_stat_port5_hour_invalid_pkts    ",
    "xe1_stat_port5_minute_invalid_pkts  ",
    "xe1_stat_port5_second_invalid_pkts  ",
    "xe1_stat_port5_ip_err_pkts          ",
    "xe1_stat_port5_timestamp_err_pkts   ",

    "stat_port6_rxx_pkts                 ",
    "xe0_stat_port6_rxx_pkts             ",
    "xe0_stat_port6_year_invalid_pkts    ",
    "xe0_stat_port6_month_invalid_pkts   ",
    "xe0_stat_port6_day_invalid_pkts     ",
    "xe0_stat_port6_hour_invalid_pkts    ",
    "xe0_stat_port6_minute_invalid_pkts  ",
    "xe0_stat_port6_second_invalid_pkts  ",
    "xe0_stat_port6_ip_err_pkts          ",
    "xe0_stat_port6_timestamp_err_pkts   ",
    "xe1_stat_port6_rxx_pkts             ",
    "xe1_stat_port6_year_invalid_pkts    ",
    "xe1_stat_port6_month_invalid_pkts   ",
    "xe1_stat_port6_day_invalid_pkts     ",
    "xe1_stat_port6_hour_invalid_pkts    ",
    "xe1_stat_port6_minute_invalid_pkts  ",
    "xe1_stat_port6_second_invalid_pkts  ",
    "xe1_stat_port6_ip_err_pkts          ",
    "xe1_stat_port6_timestamp_err_pkts   ",

    "stat_port7_rxx_pkts                 ",
    "xe0_stat_port7_rxx_pkts             ",
    "xe0_stat_port7_year_invalid_pkts    ",
    "xe0_stat_port7_month_invalid_pkts   ",
    "xe0_stat_port7_day_invalid_pkts     ",
    "xe0_stat_port7_hour_invalid_pkts    ",
    "xe0_stat_port7_minute_invalid_pkts  ",
    "xe0_stat_port7_second_invalid_pkts  ",
    "xe0_stat_port7_ip_err_pkts          ",
    "xe0_stat_port7_timestamp_err_pkts   ",
    "xe1_stat_port7_rxx_pkts             ",
    "xe1_stat_port7_year_invalid_pkts    ",
    "xe1_stat_port7_month_invalid_pkts   ",
    "xe1_stat_port7_day_invalid_pkts     ",
    "xe1_stat_port7_hour_invalid_pkts    ",
    "xe1_stat_port7_minute_invalid_pkts  ",
    "xe1_stat_port7_second_invalid_pkts  ",
    "xe1_stat_port7_ip_err_pkts          ",
    "xe1_stat_port7_timestamp_err_pkts   ",

    "stat_port8_rxx_pkts                 ",
    "xe0_stat_port8_rxx_pkts             ",
    "xe0_stat_port8_year_invalid_pkts    ",
    "xe0_stat_port8_month_invalid_pkts   ",
    "xe0_stat_port8_day_invalid_pkts     ",
    "xe0_stat_port8_hour_invalid_pkts    ",
    "xe0_stat_port8_minute_invalid_pkts  ",
    "xe0_stat_port8_second_invalid_pkts  ",
    "xe0_stat_port8_ip_err_pkts          ",
    "xe0_stat_port8_timestamp_err_pkts   ",
    "xe1_stat_port8_rxx_pkts             ",
    "xe1_stat_port8_year_invalid_pkts    ",
    "xe1_stat_port8_month_invalid_pkts   ",
    "xe1_stat_port8_day_invalid_pkts     ",
    "xe1_stat_port8_hour_invalid_pkts    ",
    "xe1_stat_port8_minute_invalid_pkts  ",
    "xe1_stat_port8_second_invalid_pkts  ",
    "xe1_stat_port8_ip_err_pkts          ",
    "xe1_stat_port8_timestamp_err_pkts   ",

    "stat_port9_rxx_pkts                 ",
    "xe0_stat_port9_rxx_pkts             ",
    "xe0_stat_port9_year_invalid_pkts    ",
    "xe0_stat_port9_month_invalid_pkts   ",
    "xe0_stat_port9_day_invalid_pkts     ",
    "xe0_stat_port9_hour_invalid_pkts    ",
    "xe0_stat_port9_minute_invalid_pkts  ",
    "xe0_stat_port9_second_invalid_pkts  ",
    "xe0_stat_port9_ip_err_pkts          ",
    "xe0_stat_port9_timestamp_err_pkts   ",
    "xe1_stat_port9_rxx_pkts             ",
    "xe1_stat_port9_year_invalid_pkts    ",
    "xe1_stat_port9_month_invalid_pkts   ",
    "xe1_stat_port9_day_invalid_pkts     ",
    "xe1_stat_port9_hour_invalid_pkts    ",
    "xe1_stat_port9_minute_invalid_pkts  ",
    "xe1_stat_port9_second_invalid_pkts  ",
    "xe1_stat_port9_ip_err_pkts          ",
    "xe1_stat_port9_timestamp_err_pkts   ",

    "stat_port10_rxx_pkts                " ,
    "xe0_stat_port10_rxx_pkts            " ,
    "xe0_stat_port10_year_invalid_pkts   " ,
    "xe0_stat_port10_month_invalid_pkts  " ,
    "xe0_stat_port10_day_invalid_pkts    " ,
    "xe0_stat_port10_hour_invalid_pkts   " ,
    "xe0_stat_port10_minute_invalid_pkts " ,
    "xe0_stat_port10_second_invalid_pkts " ,
    "xe0_stat_port10_ip_err_pkts         " ,
    "xe0_stat_port10_timestamp_err_pkts  " ,
    "xe1_stat_port10_rxx_pkts            " ,
    "xe1_stat_port10_year_invalid_pkts   " ,
    "xe1_stat_port10_month_invalid_pkts  " ,
    "xe1_stat_port10_day_invalid_pkts    " ,
    "xe1_stat_port10_hour_invalid_pkts   " ,
    "xe1_stat_port10_minute_invalid_pkts " ,
    "xe1_stat_port10_second_invalid_pkts " ,
    "xe1_stat_port10_ip_err_pkts         " ,
    "xe1_stat_port10_timestamp_err_pkts  " ,

    "stat_port11_rxx_pkts                " ,
    "xe0_stat_port11_rxx_pkts            " ,
    "xe0_stat_port11_year_invalid_pkts   " ,
    "xe0_stat_port11_month_invalid_pkts  " ,
    "xe0_stat_port11_day_invalid_pkts    " ,
    "xe0_stat_port11_hour_invalid_pkts   " ,
    "xe0_stat_port11_minute_invalid_pkts " ,
    "xe0_stat_port11_second_invalid_pkts " ,
    "xe0_stat_port11_ip_err_pkts         " ,
    "xe0_stat_port11_timestamp_err_pkts  " ,
    "xe1_stat_port11_rxx_pkts            " ,
    "xe1_stat_port11_year_invalid_pkts   " ,
    "xe1_stat_port11_month_invalid_pkts  " ,
    "xe1_stat_port11_day_invalid_pkts    " ,
    "xe1_stat_port11_hour_invalid_pkts   " ,
    "xe1_stat_port11_minute_invalid_pkts " ,
    "xe1_stat_port11_second_invalid_pkts " ,
    "xe1_stat_port11_ip_err_pkts         " ,
    "xe1_stat_port11_timestamp_err_pkts  " ,

    "stat_port12_rxx_pkts                " ,
    "xe0_stat_port12_rxx_pkts            " ,
    "xe0_stat_port12_year_invalid_pkts   " ,
    "xe0_stat_port12_month_invalid_pkts  " ,
    "xe0_stat_port12_day_invalid_pkts    " ,
    "xe0_stat_port12_hour_invalid_pkts   " ,
    "xe0_stat_port12_minute_invalid_pkts " ,
    "xe0_stat_port12_second_invalid_pkts " ,
    "xe0_stat_port12_ip_err_pkts         " ,
    "xe0_stat_port12_timestamp_err_pkts  " ,
    "xe1_stat_port12_rxx_pkts            " ,
    "xe1_stat_port12_year_invalid_pkts   " ,
    "xe1_stat_port12_month_invalid_pkts  " ,
    "xe1_stat_port12_day_invalid_pkts    " ,
    "xe1_stat_port12_hour_invalid_pkts   " ,
    "xe1_stat_port12_minute_invalid_pkts " ,
    "xe1_stat_port12_second_invalid_pkts " ,
    "xe1_stat_port12_ip_err_pkts         " ,
    "xe1_stat_port12_timestamp_err_pkts  " ,

    "stat_port13_rxx_pkts                " ,
    "xe0_stat_port13_rxx_pkts            " ,
    "xe0_stat_port13_year_invalid_pkts   " ,
    "xe0_stat_port13_month_invalid_pkts  " ,
    "xe0_stat_port13_day_invalid_pkts    " ,
    "xe0_stat_port13_hour_invalid_pkts   " ,
    "xe0_stat_port13_minute_invalid_pkts " ,
    "xe0_stat_port13_second_invalid_pkts " ,
    "xe0_stat_port13_ip_err_pkts         " ,
    "xe0_stat_port13_timestamp_err_pkts  " ,
    "xe1_stat_port13_rxx_pkts            " ,
    "xe1_stat_port13_year_invalid_pkts   " ,
    "xe1_stat_port13_month_invalid_pkts  " ,
    "xe1_stat_port13_day_invalid_pkts    " ,
    "xe1_stat_port13_hour_invalid_pkts   " ,
    "xe1_stat_port13_minute_invalid_pkts " ,
    "xe1_stat_port13_second_invalid_pkts " ,
    "xe1_stat_port13_ip_err_pkts         " ,
    "xe1_stat_port13_timestamp_err_pkts  " ,

    "stat_port14_rxx_pkts                " ,
    "xe0_stat_port14_rxx_pkts            " ,
    "xe0_stat_port14_year_invalid_pkts   " ,
    "xe0_stat_port14_month_invalid_pkts  " ,
    "xe0_stat_port14_day_invalid_pkts    " ,
    "xe0_stat_port14_hour_invalid_pkts   " ,
    "xe0_stat_port14_minute_invalid_pkts " ,
    "xe0_stat_port14_second_invalid_pkts " ,
    "xe0_stat_port14_ip_err_pkts         " ,
    "xe0_stat_port14_timestamp_err_pkts  " ,
    "xe1_stat_port14_rxx_pkts            " ,
    "xe1_stat_port14_year_invalid_pkts   " ,
    "xe1_stat_port14_month_invalid_pkts  " ,
    "xe1_stat_port14_day_invalid_pkts    " ,
    "xe1_stat_port14_hour_invalid_pkts   " ,
    "xe1_stat_port14_minute_invalid_pkts " ,
    "xe1_stat_port14_second_invalid_pkts " ,
    "xe1_stat_port14_ip_err_pkts         " ,
    "xe1_stat_port14_timestamp_err_pkts  " ,

    "stat_port15_rxx_pkts                " ,
    "xe0_stat_port15_rxx_pkts            " ,
    "xe0_stat_port15_year_invalid_pkts   " ,
    "xe0_stat_port15_month_invalid_pkts  " ,
    "xe0_stat_port15_day_invalid_pkts    " ,
    "xe0_stat_port15_hour_invalid_pkts   " ,
    "xe0_stat_port15_minute_invalid_pkts " ,
    "xe0_stat_port15_second_invalid_pkts " ,
    "xe0_stat_port15_ip_err_pkts         " ,
    "xe0_stat_port15_timestamp_err_pkts  " ,
    "xe1_stat_port15_rxx_pkts            " ,
    "xe1_stat_port15_year_invalid_pkts   " ,
    "xe1_stat_port15_month_invalid_pkts  " ,
    "xe1_stat_port15_day_invalid_pkts    " ,
    "xe1_stat_port15_hour_invalid_pkts   " ,
    "xe1_stat_port15_minute_invalid_pkts " ,
    "xe1_stat_port15_second_invalid_pkts " ,
    "xe1_stat_port15_ip_err_pkts         " ,
    "xe1_stat_port15_timestamp_err_pkts  " ,
    };



    if (index >= stat_timestamp_max)
    {
        return 1;
    }

    strcpy(name, stat_name[index]);

    return 0;
}
#endif /* end of FRC_CONFIG_TIMESTAMP_CHECK*/
#define frc_dump_buff(_len, _buff) _frc_dump_buff(__func__, __LINE__, _len, _buff)

#endif /* !__FRC_UTIL_H__ */

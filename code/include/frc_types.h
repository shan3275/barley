#ifndef __FRC_TYPES_H__
#define __FRC_TYPES_H__

#include "frc_config.h"

#if   defined(MAKE_KERNEL)
#   include <linux/types.h>
#elif defined(MAKE_APP) || defined(MAKE_CORE)
#   include <stdint.h>
#else
#endif

#define K (1024)
#define M (1024 * 1024)
#define G (1024 * 1024 * 1024)


#define FRC_CORE_MAX        12
#define FRC_DAT_CORE_NUM    10
#define FRC_DAT_CORE_MAX    FRC_DAT_CORE_NUM
#define FRC_PKT_CORE_MAX    11
#define FRC_CMD_CORE_MAX    12

#define FRC_CMD_GRP         12
#define FRC_PKT_GRP         11
#define FRC_DAT_GRP         0
#define GROUP_FROM_INPUT_PORT           0
#define GROUP_TO_DATA_PLANE_AGING       13


#define MAX_HASH_SESSION 4
#define RULE_MAX  2000
#define MAX_RULE  4

#if FRC_CONFIG_TWO_TUPLE
#define ACL_MAX  2000
#define ACL_HASH_MAX  2048
#define ACL_INDEX_START 1
#define MAX_ACL  4
#endif

#if FRC_CONFIG_VLAN_CHECK
#define VLAN_CHECK_START_ID_MIN  2
#define VLAN_CHECK_START_ID_MAX  4095
#define VLAN_CHECK_ID_MAX        4096
#endif
// Protocol



/* PROTOCOL */
#define PROTO_TCP                 0x06
#define PROTO_UDP                 0x11
#define PROTO_ICMP                0x1
#define PROTO_SCTP                0x2
#define PROTO_VLAN                0x8100
#define PROTO_IPV4                0x0800
#define PROTO_IPV6                0x86DD


#if FRC_CONFIG_VLAN_CHECK | FRC_CONFIG_TIMESTAMP_CHECK
#define FRCORE_IPV4                      0x04
#define FRCORE_IPV6                      0x06
#endif

/* Error code
 *
 */

typedef enum {
    FRE_SUCCESS,
    FRE_FAIL,
    FRE_INIT,
    FRE_MEMORY,
    FRE_UNSUPPORT,
    FRE_UNREGISTER,
    FRE_EXIST,
    FRE_DMA,
    FRE_NOSPACE,
    FRE_IOCTL,
    FRE_OPEN = 10,
    FRE_BUSY,
    FRE_PARAM,
    FRE_TIMEOUT,
    FRE_EXCEED,
    FRE_NOTFOUND,
    FRE_DEBUG,
    FRE_DIRECTION_FAIL,
    FRE_MAX
} fre_t;



#define STAT_START_INDEX          76
#define STAT_END_INDEX            125
#define STAT_INTERVAL             (STAT_END_INDEX - STAT_START_INDEX)




typedef struct frc_version {
    uint8_t  major;
    uint8_t  minor;
    uint16_t build;
} frc_version_t;

typedef struct {
    uint64_t core_num;
    uint64_t core_mask;
    uint64_t data_core_num;
    uint64_t core_freq;

    uint64_t mem_size;
    uint64_t mem_freq;

    uint64_t pcb_version;
    uint64_t boot_type;

    uint64_t pcie_version;
    uint64_t pcie_lane;

    uint64_t cpld_version;
    uint64_t port_num;
    uint64_t port_speed;
    uint64_t boot_cycle;
} frc_system_info_t;

typedef struct {

} frc_system_config_t;

typedef enum {
    stat_p0_rx_pkts                 ,
    stat_p0_tx_pkts                 ,
    stat_p0_rx_errs                 ,
    stat_p0_rx_dropped              ,
    stat_p1_rx_pkts                 ,
    stat_p1_tx_pkts                 ,
    stat_p1_rx_errs                 ,
    stat_p1_rx_dropped              ,
    stat_p1_rx_pko                  ,
    stat_p0_PARTIAL_ERR             ,  /* RGM+SPI            1 = partially received packet (buffering/bandwidth not adequate) */
    stat_p0_JABBER_ERR              ,  /* RGM+SPI            2 = receive packet too large and truncated */
    stat_p0_OVER_FCS_ERR            ,  /* RGM                3 = max frame error (pkt len > max frame len) (with FCS error) */
    stat_p0_OVER_ERR                ,  /* RGM+SPI            4 = max frame error (pkt len > max frame len) */
    stat_p0_ALIGN_ERR               ,  /* RGM                5 = nibble error (data not byte multiple - 100M and 10M only) */
    stat_p0_UNDER_FCS_ERR           ,  /* RGM                6 = min frame error (pkt len < min frame len) (with FCS error) */
    stat_p0_GMX_FCS_ERR             ,  /* RGM                7 = FCS error */
    stat_p0_UNDER_ERR               ,  /* RGM+SPI            8 = min frame error (pkt len < min frame len) */
    stat_p0_EXTEND_ERR              ,  /* RGM                9 = Frame carrier extend error */
    stat_p0_LENGTH_ERR              ,  /* RGM               10 = length mismatch (len did not match len in L2 length/type) */
    stat_p0_DAT_ERR                 ,  /* RGM               11 = Frame error (some or all data bits marked err) */
    stat_p0_SKIP_ERR                ,  /* RGM               12 = packet was not large enough to pass the skipper - no inspection could occur */
    stat_p0_NIBBLE_ERR              ,  /* RGM               13 = studder error (data not repeated - 100M and 10M only) */
    stat_p0_PIP_FCS                 ,  /* RGM+SPI           16 = FCS error */
    stat_p0_PIP_SKIP_ERR            ,  /* RGM+SPI+PCI       17 = packet was not large enough to pass the skipper - no inspection could occur */
    stat_p0_PIP_L2_MAL_HDR          ,  /* RGM+SPI+PCI       18 = malformed l2 (packet not long enough to cover L2 hdr) */
    stat_p0_PUNY_ERR                ,  /* SGMII             47 = PUNY error (packet was 4B or less when FCS stripping is enabled) */
    stat_p0_UNKNOWN_ERR             ,

    stat_p1_PARTIAL_ERR             ,  /* RGM+SPI            1 = partially received packet (buffering/bandwidth not adequate) */
    stat_p1_JABBER_ERR              ,  /* RGM+SPI            2 = receive packet too large and truncated */
    stat_p1_OVER_FCS_ERR            ,  /* RGM                3 = max frame error (pkt len > max frame len) (with FCS error) */
    stat_p1_OVER_ERR                ,  /* RGM+SPI            4 = max frame error (pkt len > max frame len) */
    stat_p1_ALIGN_ERR               ,  /* RGM                5 = nibble error (data not byte multiple - 100M and 10M only) */
    stat_p1_UNDER_FCS_ERR           ,  /* RGM                6 = min frame error (pkt len < min frame len) (with FCS error) */
    stat_p1_GMX_FCS_ERR             ,  /* RGM                7 = FCS error */
    stat_p1_UNDER_ERR               ,  /* RGM+SPI            8 = min frame error (pkt len < min frame len) */
    stat_p1_EXTEND_ERR              ,  /* RGM                9 = Frame carrier extend error */
    stat_p1_LENGTH_ERR              ,  /* RGM               10 = length mismatch (len did not match len in L2 length/type) */
    stat_p1_DAT_ERR                 ,  /* RGM               11 = Frame error (some or all data bits marked err) */
    stat_p1_SKIP_ERR                ,  /* RGM               12 = packet was not large enough to pass the skipper - no inspection could occur */
    stat_p1_NIBBLE_ERR              ,  /* RGM               13 = studder error (data not repeated - 100M and 10M only) */
    stat_p1_PIP_FCS                 ,  /* RGM+SPI           16 = FCS error */
    stat_p1_PIP_SKIP_ERR         ,  /* RGM+SPI+PCI       17 = packet was not large enough to pass the skipper - no inspection could occur */
    stat_p1_PIP_L2_MAL_HDR       ,  /* RGM+SPI+PCI       18 = malformed l2 (packet not long enough to cover L2 hdr) */
    stat_p1_PUNY_ERR             ,  /* SGMII             47 = PUNY error (packet was 4B or less when FCS stripping is enabled) */
    stat_p1_UNKNOWN_ERR          ,

    stat_core0_wqe               ,
    stat_core1_wqe               ,
    stat_core2_wqe               ,
    stat_core3_wqe               ,
    stat_core4_wqe               ,
    stat_core5_wqe               , /* 50 */
    stat_core6_wqe               ,
    stat_core7_wqe               ,
    stat_core8_wqe               ,
    stat_core9_wqe               ,
    stat_core10_wqe              ,
    stat_core11_wqe              , /* 56 */
    stat_work_free               ,
    stat_work_data               ,
    stat_work_stat               ,
    stat_work_queue              , /* 60 */
    stat_work_pkt                ,
    stat_work_cmd                ,
    stat_work_instr              ,
    stat_work_unkown             ,
    stat_chan_timer              ,
    stat_act_debug,
    stat_act_drop                ,
    stat_act_free                ,
    stat_act_forward             ,
    stat_act_delay               ,
    stat_act_unfree              ,
    stat_act_unkown              ,
    stat_pkts_below_64,
    stat_pkts_between_64_128,
    stat_pkts_between_128_256,
    stat_pkts_between_256_512,
    stat_pkts_between_512_1024,
    stat_pkts_between_1024_1500,
    stat_pkts_between_1500_1600,
    stat_pkts_above_1600,
    stat_very_short_pkts,         /* 80 */
    stat_very_long_pkts,
    stat_normal_pkts,
    stat_tx_pkts                 ,
    stat_tx_bytes                ,
    stat_tx_errs                 ,
    stat_rx_work                 ,
    stat_rx_errs                 ,
    stat_rx_pkts                 ,
    stat_rx_bytes                ,
    stat_drop_port_off           , /* 90*/
    stat_drop_rx_error           ,
    stat_not_ip                  ,
    stat_mpls                    ,
    stat_pppoe                   ,
    stat_drop_not_ip             ,
    stat_ip                      ,
    stat_ip_v6                   ,
    stat_drop_ip_v6              ,
    stat_ip_v4                   ,
    stat_vlan                    , /* 100*/
    stat_drop_vlan               ,
    stat_ip_frag                 ,
    stat_drop_ip_frag            ,
    stat_ip_not_tcp_udp          ,
    stat_drop_ip_not_tcp_udp     ,
    stat_ip_option               , /* 106 */
    stat_drop_ip_option          ,
    stat_ip_other                ,
    stat_drop_ip_other           ,
    stat_tcp                     , /* 110 */
    stat_tcp_malformed_packet    ,
    stat_tcp_option              ,
    stat_tcp_payload_zero        ,
    stat_ssn_prepare             ,
    stat_ssn_fail                ,
    stat_ssn_fail_fin_or_rst     ,
    stat_ssn_fail_overflow       ,
    stat_ssn_fpa_alloc_err       , /* 120 */
    stat_ssn_fpa_failed          ,
    stat_ssn_active_num          ,
    stat_ssn_fwd_tcpflowrec      ,
    stat_ssn_drop                ,
    stat_tcpflowrec_retrans      ,
    stat_tcpflowrec_retrans_drop ,
    stat_tcpflowrec_part_retrans ,
    stat_tcpflowrec_dup_remove   ,
    stat_tcpflowrec_queue_overflow,
    stat_tcpflowrec_mem_failed   ,
    stat_ssn_age_wqe,
    stat_ssn_tim_addok           , /* 130 */
    stat_ssn_tim_fired           ,
    stat_ssn_tim_failed          ,
    stat_ssn_prepared_by_fpa_alloc,
    stat_ssn_purge_to_fpa        ,
    stat_ssn_purge_unknown       ,
    stat_ssn_fpa_free_err        ,
    stat_ssn_purge               ,
    stat_ssn_dma_errors          ,
    stat_ssn_dma_pkts            ,
    stat_ssn_dma_bytes           ,
    stat_ssn_dma_pkt_infos       ,  /* 141 */
    stat_ssn_dma_blocks          ,
    stat_ssn_1packet_blocks       ,
    stat_ssn_2packets_blocks      ,
    stat_ssn_3packets_blocks      ,
    stat_ssn_4packets_blocks      ,
    stat_ssn_5packets_blocks      ,
    stat_ssn_above_5packets_blocks,
    stat_ssn_queue_no_space       ,
    stat_ssn_dma_enqueue_pkts     ,
    stat_ssn_dma_enqueue_errs     ,
    stat_ssn_dma_head_fail        ,
    stat_ssn_avail_get            ,
    stat_ssn_avail_get_err        ,
    stat_ssn_avail_get_no_space   ,
    stat_ssn_compl_put            ,
    stat_ssn_compl_put_err        ,
    stat_ssn_compl_put_no_space   ,
    stat_ssn_pkt_post            ,
    stat_ssn_pkt_post_err        ,
    stat_ssn_post_err_pkts       ,
    stat_udp                     ,
    stat_udp_malformed_packet    ,
    stat_udp_submit_pkts,         /* 158 */
    stat_udp_submit_bytes,
    stat_rule_malformed_packet   ,
    stat_rule_matched_pkts,
    stat_rule_matched_bytes,
    stat_rule_not_matched_pkts,
    stat_rule_not_matched_bytes,
    stat_pkt_oversize            ,
    stat_dma_errors              ,
    stat_dma_pkts                ,
    stat_dma_pkt_infos           , /* 168 */
    stat_dma_bytes               ,
    stat_dma_blocks              ,
    stat_avail_get               ,
    stat_avail_get_err           ,
    stat_avail_get_no_space      ,
    stat_compl_put               ,
    stat_compl_put_err           ,
    stat_compl_put_no_space      ,
    stat_pkt_post                ,
    stat_pkt_post_err            , /* 178 */
    stat_post_err_pkts           ,
    stat_queue_post              ,
    stat_queue_post_err          ,
    stat_dma_loop_bytes          ,
    stat_dma_loop_entries        ,
    stat_dma_loop_errs           ,
    stat_dma_block_addr_errs     ,
    stat_dma_enqueue_pkts        ,
    stat_dma_enqueue_errs        ,
    stat_get_avail_bytes         ,
    stat_put_compl_bytes         ,
    stat_queue_no_space          ,
    #if FRC_CONFIG_TWO_TUPLE
    stat_acl_malformed_packet   ,
    stat_acl_type_error          ,
    stat_acl_hash_error          ,
    stat_acl_entry_exception     ,
    stat_acl_not_found_index     ,
    stat_acl_del_hash_cell       ,
    stat_acl_del_rule_num        ,
    stat_acl_add_hash_cell       ,
    stat_acl_add_rule_num        ,
    stat_acl_hash_cell_alloc_fail,
    stat_acl_matched_pkts,
    stat_acl_matched_bytes,
    stat_acl_not_matched_pkts,
    stat_acl_not_matched_bytes,
    #endif
    stat_ssn_fifo_lock,
    stat_ssn_fifo_unlock,
#ifdef FRC_CONFIG_MAC_STATISTICS
    stat_swap_mac_send_pkts,
    stat_heart_beat_pkt,
    stat_heart_beat_send_back_pkt,
    stat_heart_beat_on_drop_pkt,
    stat_heart_beat_drop_pkt,
#endif
#ifdef FRC_CONFIG_IPC
    stat_ipc_drop_smac_zero,
    stat_ipc_smac_ordered,
    stat_ipc_smac_disorder,
    stat_ipc_pktid_ordered,
    stat_ipc_pktid_disorder,
    stat_ipc_pktid_repeat,
    stat_ipc_head_valid,
    stat_ipc_head_invalid,
    stat_ipc_inner_vlan,
#endif
    stat_max
} frc_stat_e;

#if FRC_CONFIG_VLAN_CHECK
typedef enum{
    xe0_stat_rxx_pkts            ,
    xe0_stat_rxx_bytes           ,
    xe0_stat_vlan_id_total       ,
    xe0_stat_vlan_id_error       ,
    xe0_stat_vlan_id_0           ,
    xe0_stat_vlan_id_1           ,
    xe0_stat_vlan_id_2           ,
    xe0_stat_vlan_id_3           ,
    xe0_stat_vlan_id_4           ,
    xe0_stat_vlan_id_5            ,
    xe0_stat_vlan_id_6               ,
    xe0_stat_vlan_id_7               ,
    xe0_stat_vlan_id_8               ,
    xe0_stat_vlan_id_9               ,
    xe0_stat_vlan_id_10              ,
    xe0_stat_vlan_id_11              ,
    xe0_stat_vlan_id_12              ,
    xe0_stat_vlan_id_13         ,
    xe0_stat_vlan_id_14 ,
    xe0_stat_vlan_id_15 ,
    xe0_stat_vlan_id_16 ,
    xe0_stat_vlan_id_17 ,
    xe0_stat_vlan_id_18 ,
    xe0_stat_vlan_id_19 ,
    xe0_stat_vlan_id_20 ,
    xe0_stat_vlan_id_21 ,
    xe0_stat_vlan_id_22 ,
    xe0_stat_vlan_id_23 ,
    xe0_stat_vlan_id_24 ,
    xe0_stat_vlan_id_25 ,
    xe0_stat_vlan_id_26 ,
    xe0_stat_vlan_id_27 ,
    xe0_stat_vlan_id_28 ,
    xe0_stat_vlan_id_29 ,
    xe0_stat_vlan_id_30 ,
    xe0_stat_vlan_id_31 ,
    xe0_stat_vlan_id_32 ,
    xe0_stat_vlan_id_33 ,
    xe0_stat_vlan_id_34 ,
    xe0_stat_vlan_id_35 ,
    xe0_stat_vlan_id_36 ,
    xe0_stat_vlan_id_37 ,
    xe0_stat_vlan_id_38 ,
    xe0_stat_vlan_id_39 ,
    xe0_stat_vlan_id_40 ,
    xe0_stat_vlan_id_41 ,
    xe0_stat_vlan_id_42 ,
    xe0_stat_vlan_id_43 ,
    xe0_stat_vlan_id_44 ,
    xe0_stat_vlan_id_45 ,
    xe0_stat_vlan_id_46 ,
    xe0_stat_vlan_id_47 ,
    xe0_stat_vlan_id_48 ,
    xe0_stat_vlan_id_49 ,
    xe0_stat_vlan_id_50 ,
    xe0_stat_vlan_id_51 ,
    xe0_stat_vlan_id_52 ,
    xe0_stat_vlan_id_53 ,
    xe0_stat_vlan_id_54 ,
    xe0_stat_vlan_id_55 ,
    xe0_stat_vlan_id_56 ,
    xe0_stat_vlan_id_57 ,
    xe0_stat_vlan_id_58 ,
    xe0_stat_vlan_id_59 ,
    xe0_stat_vlan_id_60 ,
    xe0_stat_vlan_id_61 ,
    xe0_stat_vlan_id_62 ,
    xe0_stat_vlan_id_63 ,
    xe0_stat_vlan_id_64 ,
    xe0_stat_vlan_id_65 ,
    xe0_stat_vlan_id_66 ,
    xe0_stat_vlan_id_67 ,
    xe0_stat_vlan_id_68 ,
    xe0_stat_vlan_id_69 ,
    xe0_stat_vlan_id_70 ,
    xe0_stat_vlan_id_71 ,
    xe0_stat_vlan_id_72 ,
    xe0_stat_vlan_id_73 ,
    xe0_stat_vlan_id_74 ,
    xe0_stat_vlan_id_75 ,
    xe0_stat_vlan_id_76 ,
    xe0_stat_vlan_id_77 ,
    xe0_stat_vlan_id_78 ,
    xe0_stat_vlan_id_79 ,
    xe0_stat_vlan_id_80 ,
    xe0_stat_vlan_id_81 ,
    xe0_stat_vlan_id_82 ,
    xe0_stat_vlan_id_83 ,
    xe0_stat_vlan_id_84 ,
    xe0_stat_vlan_id_85 ,
    xe0_stat_vlan_id_86 ,
    xe0_stat_vlan_id_87 ,
    xe0_stat_vlan_id_88 ,
    xe0_stat_vlan_id_89 ,
    xe0_stat_vlan_id_90 ,
    xe0_stat_vlan_id_91 ,
    xe0_stat_vlan_id_92 ,
    xe0_stat_vlan_id_93 ,
    xe0_stat_vlan_id_94 ,
    xe0_stat_vlan_id_95 ,
    xe0_stat_vlan_id_96 ,
    xe0_stat_vlan_id_97 ,
    xe0_stat_vlan_id_98 ,
    xe0_stat_vlan_id_99 ,
    xe0_stat_vlan_id_100 ,
    xe0_stat_vlan_id_101 ,
    xe0_stat_vlan_id_102 ,
    xe0_stat_vlan_id_103 ,
    xe0_stat_vlan_id_104 ,
    xe0_stat_vlan_id_105 ,
    xe0_stat_vlan_id_106 ,
    xe0_stat_vlan_id_107 ,
    xe0_stat_vlan_id_108 ,
    xe0_stat_vlan_id_109 ,
    xe0_stat_vlan_id_110 ,
    xe0_stat_vlan_id_111 ,
    xe0_stat_vlan_id_112 ,
    xe0_stat_vlan_id_113 ,
    xe0_stat_vlan_id_114 ,
    xe0_stat_vlan_id_115 ,
    xe0_stat_vlan_id_116 ,
    xe0_stat_vlan_id_117 ,
    xe0_stat_vlan_id_118 ,
    xe0_stat_vlan_id_119 ,
    xe0_stat_vlan_id_120 ,
    xe0_stat_vlan_id_121 ,
    xe0_stat_vlan_id_122 ,
    xe0_stat_vlan_id_123 ,
    xe0_stat_vlan_id_124 ,
    xe0_stat_vlan_id_125 ,
    xe0_stat_vlan_id_126 ,
    xe0_stat_vlan_id_127 ,
    xe0_stat_vlan_id_128 ,
    xe0_stat_vlan_id_129 ,
    xe0_stat_vlan_id_130 ,
    xe0_stat_vlan_id_131 ,
    xe0_stat_vlan_id_132 ,
    xe0_stat_vlan_id_133 ,
    xe0_stat_vlan_id_134 ,
    xe0_stat_vlan_id_135 ,
    xe0_stat_vlan_id_136 ,
    xe0_stat_vlan_id_137 ,
    xe0_stat_vlan_id_138 ,
    xe0_stat_vlan_id_139 ,
    xe0_stat_vlan_id_140 ,
    xe0_stat_vlan_id_141 ,
    xe0_stat_vlan_id_142 ,
    xe0_stat_vlan_id_143 ,
    xe0_stat_vlan_id_144 ,
    xe0_stat_vlan_id_145 ,
    xe0_stat_vlan_id_146 ,
    xe0_stat_vlan_id_147 ,
    xe0_stat_vlan_id_148 ,
    xe0_stat_vlan_id_149 ,
    xe0_stat_vlan_id_150 ,
    xe0_stat_vlan_id_151 ,
    xe0_stat_vlan_id_152 ,
    xe0_stat_vlan_id_153 ,
    xe0_stat_vlan_id_154 ,
    xe0_stat_vlan_id_155 ,
    xe0_stat_vlan_id_156 ,
    xe0_stat_vlan_id_157 ,
    xe0_stat_vlan_id_158 ,
    xe0_stat_vlan_id_159 ,
    xe0_stat_vlan_id_160 ,
    xe0_stat_vlan_id_161 ,
    xe0_stat_vlan_id_162 ,
    xe0_stat_vlan_id_163 ,
    xe0_stat_vlan_id_164 ,
    xe0_stat_vlan_id_165 ,
    xe0_stat_vlan_id_166 ,
    xe0_stat_vlan_id_167 ,
    xe0_stat_vlan_id_168 ,
    xe0_stat_vlan_id_169 ,
    xe0_stat_vlan_id_170 ,
    xe0_stat_vlan_id_171 ,
    xe0_stat_vlan_id_172 ,
    xe0_stat_vlan_id_173 ,
    xe0_stat_vlan_id_174 ,
    xe0_stat_vlan_id_175 ,
    xe0_stat_vlan_id_176 ,
    xe0_stat_vlan_id_177 ,
    xe0_stat_vlan_id_178 ,
    xe0_stat_vlan_id_179 ,
    xe0_stat_vlan_id_180 ,
    xe0_stat_vlan_id_181 ,
    xe0_stat_vlan_id_182 ,
    xe0_stat_vlan_id_183 ,
    xe0_stat_vlan_id_184 ,
    xe0_stat_vlan_id_185 ,
    xe0_stat_vlan_id_186 ,
    xe0_stat_vlan_id_187 ,
    xe0_stat_vlan_id_188 ,
    xe0_stat_vlan_id_189 ,
    xe0_stat_vlan_id_190 ,
    xe0_stat_vlan_id_191 ,
    xe0_stat_vlan_id_192 ,
    xe0_stat_vlan_id_193 ,
    xe0_stat_vlan_id_194 ,
    xe0_stat_vlan_id_195 ,
    xe0_stat_vlan_id_196 ,
    xe0_stat_vlan_id_197 ,
    xe0_stat_vlan_id_198 ,
    xe0_stat_vlan_id_199 ,
    xe0_stat_vlan_id_200 ,
    xe0_stat_vlan_id_201 ,
    xe0_stat_vlan_id_202 ,
    xe0_stat_vlan_id_203 ,
    xe0_stat_vlan_id_204 ,
    xe0_stat_vlan_id_205 ,
    xe0_stat_vlan_id_206 ,
    xe0_stat_vlan_id_207 ,
    xe0_stat_vlan_id_208 ,
    xe0_stat_vlan_id_209 ,
    xe0_stat_vlan_id_210 ,
    xe0_stat_vlan_id_211 ,
    xe0_stat_vlan_id_212 ,
    xe0_stat_vlan_id_213 ,
    xe0_stat_vlan_id_214 ,
    xe0_stat_vlan_id_215 ,
    xe0_stat_vlan_id_216 ,
    xe0_stat_vlan_id_217 ,
    xe0_stat_vlan_id_218 ,
    xe0_stat_vlan_id_219 ,
    xe0_stat_vlan_id_220 ,
    xe0_stat_vlan_id_221 ,
    xe0_stat_vlan_id_222 ,
    xe0_stat_vlan_id_223 ,
    xe0_stat_vlan_id_224 ,
    xe0_stat_vlan_id_225 ,
    xe0_stat_vlan_id_226 ,
    xe0_stat_vlan_id_227 ,
    xe0_stat_vlan_id_228 ,
    xe0_stat_vlan_id_229 ,
    xe0_stat_vlan_id_230 ,
    xe0_stat_vlan_id_231 ,
    xe0_stat_vlan_id_232 ,
    xe0_stat_vlan_id_233 ,
    xe0_stat_vlan_id_234 ,
    xe0_stat_vlan_id_235 ,
    xe0_stat_vlan_id_236 ,
    xe0_stat_vlan_id_237 ,
    xe0_stat_vlan_id_238 ,
    xe0_stat_vlan_id_239 ,
    xe0_stat_vlan_id_240 ,
    xe0_stat_vlan_id_241 ,
    xe0_stat_vlan_id_242 ,
    xe0_stat_vlan_id_243 ,
    xe0_stat_vlan_id_244 ,
    xe0_stat_vlan_id_245 ,
    xe0_stat_vlan_id_246 ,
    xe0_stat_vlan_id_247 ,
    xe0_stat_vlan_id_248 ,
    xe0_stat_vlan_id_249 ,
    xe0_stat_vlan_id_250 ,
    xe0_stat_vlan_id_251 ,
    xe0_stat_vlan_id_252 ,
    xe0_stat_vlan_id_253 ,
    xe0_stat_vlan_id_254 ,
    xe0_stat_vlan_id_255 ,
    xe1_stat_rxx_pkts            ,
    xe1_stat_rxx_bytes           ,
    xe1_stat_vlan_id_total       ,
    xe1_stat_vlan_id_error       ,
    xe1_stat_vlan_id_0           ,
    xe1_stat_vlan_id_1           ,
    xe1_stat_vlan_id_2           ,
    xe1_stat_vlan_id_3           ,
    xe1_stat_vlan_id_4           ,
    xe1_stat_vlan_id_5            ,
    xe1_stat_vlan_id_6               ,
    xe1_stat_vlan_id_7               ,
    xe1_stat_vlan_id_8               ,
    xe1_stat_vlan_id_9               ,
    xe1_stat_vlan_id_10              ,
    xe1_stat_vlan_id_11              ,
    xe1_stat_vlan_id_12              ,
    xe1_stat_vlan_id_13         ,
    xe1_stat_vlan_id_14 ,
    xe1_stat_vlan_id_15 ,
    xe1_stat_vlan_id_16 ,
    xe1_stat_vlan_id_17 ,
    xe1_stat_vlan_id_18 ,
    xe1_stat_vlan_id_19 ,
    xe1_stat_vlan_id_20 ,
    xe1_stat_vlan_id_21 ,
    xe1_stat_vlan_id_22 ,
    xe1_stat_vlan_id_23 ,
    xe1_stat_vlan_id_24 ,
    xe1_stat_vlan_id_25 ,
    xe1_stat_vlan_id_26 ,
    xe1_stat_vlan_id_27 ,
    xe1_stat_vlan_id_28 ,
    xe1_stat_vlan_id_29 ,
    xe1_stat_vlan_id_30 ,
    xe1_stat_vlan_id_31 ,
    xe1_stat_vlan_id_32 ,
    xe1_stat_vlan_id_33 ,
    xe1_stat_vlan_id_34 ,
    xe1_stat_vlan_id_35 ,
    xe1_stat_vlan_id_36 ,
    xe1_stat_vlan_id_37 ,
    xe1_stat_vlan_id_38 ,
    xe1_stat_vlan_id_39 ,
    xe1_stat_vlan_id_40 ,
    xe1_stat_vlan_id_41 ,
    xe1_stat_vlan_id_42 ,
    xe1_stat_vlan_id_43 ,
    xe1_stat_vlan_id_44 ,
    xe1_stat_vlan_id_45 ,
    xe1_stat_vlan_id_46 ,
    xe1_stat_vlan_id_47 ,
    xe1_stat_vlan_id_48 ,
    xe1_stat_vlan_id_49 ,
    xe1_stat_vlan_id_50 ,
    xe1_stat_vlan_id_51 ,
    xe1_stat_vlan_id_52 ,
    xe1_stat_vlan_id_53 ,
    xe1_stat_vlan_id_54 ,
    xe1_stat_vlan_id_55 ,
    xe1_stat_vlan_id_56 ,
    xe1_stat_vlan_id_57 ,
    xe1_stat_vlan_id_58 ,
    xe1_stat_vlan_id_59 ,
    xe1_stat_vlan_id_60 ,
    xe1_stat_vlan_id_61 ,
    xe1_stat_vlan_id_62 ,
    xe1_stat_vlan_id_63 ,
    xe1_stat_vlan_id_64 ,
    xe1_stat_vlan_id_65 ,
    xe1_stat_vlan_id_66 ,
    xe1_stat_vlan_id_67 ,
    xe1_stat_vlan_id_68 ,
    xe1_stat_vlan_id_69 ,
    xe1_stat_vlan_id_70 ,
    xe1_stat_vlan_id_71 ,
    xe1_stat_vlan_id_72 ,
    xe1_stat_vlan_id_73 ,
    xe1_stat_vlan_id_74 ,
    xe1_stat_vlan_id_75 ,
    xe1_stat_vlan_id_76 ,
    xe1_stat_vlan_id_77 ,
    xe1_stat_vlan_id_78 ,
    xe1_stat_vlan_id_79 ,
    xe1_stat_vlan_id_80 ,
    xe1_stat_vlan_id_81 ,
    xe1_stat_vlan_id_82 ,
    xe1_stat_vlan_id_83 ,
    xe1_stat_vlan_id_84 ,
    xe1_stat_vlan_id_85 ,
    xe1_stat_vlan_id_86 ,
    xe1_stat_vlan_id_87 ,
    xe1_stat_vlan_id_88 ,
    xe1_stat_vlan_id_89 ,
    xe1_stat_vlan_id_90 ,
    xe1_stat_vlan_id_91 ,
    xe1_stat_vlan_id_92 ,
    xe1_stat_vlan_id_93 ,
    xe1_stat_vlan_id_94 ,
    xe1_stat_vlan_id_95 ,
    xe1_stat_vlan_id_96 ,
    xe1_stat_vlan_id_97 ,
    xe1_stat_vlan_id_98 ,
    xe1_stat_vlan_id_99 ,
    xe1_stat_vlan_id_100 ,
    xe1_stat_vlan_id_101 ,
    xe1_stat_vlan_id_102 ,
    xe1_stat_vlan_id_103 ,
    xe1_stat_vlan_id_104 ,
    xe1_stat_vlan_id_105 ,
    xe1_stat_vlan_id_106 ,
    xe1_stat_vlan_id_107 ,
    xe1_stat_vlan_id_108 ,
    xe1_stat_vlan_id_109 ,
    xe1_stat_vlan_id_110 ,
    xe1_stat_vlan_id_111 ,
    xe1_stat_vlan_id_112 ,
    xe1_stat_vlan_id_113 ,
    xe1_stat_vlan_id_114 ,
    xe1_stat_vlan_id_115 ,
    xe1_stat_vlan_id_116 ,
    xe1_stat_vlan_id_117 ,
    xe1_stat_vlan_id_118 ,
    xe1_stat_vlan_id_119 ,
    xe1_stat_vlan_id_120 ,
    xe1_stat_vlan_id_121 ,
    xe1_stat_vlan_id_122 ,
    xe1_stat_vlan_id_123 ,
    xe1_stat_vlan_id_124 ,
    xe1_stat_vlan_id_125 ,
    xe1_stat_vlan_id_126 ,
    xe1_stat_vlan_id_127 ,
    xe1_stat_vlan_id_128 ,
    xe1_stat_vlan_id_129 ,
    xe1_stat_vlan_id_130 ,
    xe1_stat_vlan_id_131 ,
    xe1_stat_vlan_id_132 ,
    xe1_stat_vlan_id_133 ,
    xe1_stat_vlan_id_134 ,
    xe1_stat_vlan_id_135 ,
    xe1_stat_vlan_id_136 ,
    xe1_stat_vlan_id_137 ,
    xe1_stat_vlan_id_138 ,
    xe1_stat_vlan_id_139 ,
    xe1_stat_vlan_id_140 ,
    xe1_stat_vlan_id_141 ,
    xe1_stat_vlan_id_142 ,
    xe1_stat_vlan_id_143 ,
    xe1_stat_vlan_id_144 ,
    xe1_stat_vlan_id_145 ,
    xe1_stat_vlan_id_146 ,
    xe1_stat_vlan_id_147 ,
    xe1_stat_vlan_id_148 ,
    xe1_stat_vlan_id_149 ,
    xe1_stat_vlan_id_150 ,
    xe1_stat_vlan_id_151 ,
    xe1_stat_vlan_id_152 ,
    xe1_stat_vlan_id_153 ,
    xe1_stat_vlan_id_154 ,
    xe1_stat_vlan_id_155 ,
    xe1_stat_vlan_id_156 ,
    xe1_stat_vlan_id_157 ,
    xe1_stat_vlan_id_158 ,
    xe1_stat_vlan_id_159 ,
    xe1_stat_vlan_id_160 ,
    xe1_stat_vlan_id_161 ,
    xe1_stat_vlan_id_162 ,
    xe1_stat_vlan_id_163 ,
    xe1_stat_vlan_id_164 ,
    xe1_stat_vlan_id_165 ,
    xe1_stat_vlan_id_166 ,
    xe1_stat_vlan_id_167 ,
    xe1_stat_vlan_id_168 ,
    xe1_stat_vlan_id_169 ,
    xe1_stat_vlan_id_170 ,
    xe1_stat_vlan_id_171 ,
    xe1_stat_vlan_id_172 ,
    xe1_stat_vlan_id_173 ,
    xe1_stat_vlan_id_174 ,
    xe1_stat_vlan_id_175 ,
    xe1_stat_vlan_id_176 ,
    xe1_stat_vlan_id_177 ,
    xe1_stat_vlan_id_178 ,
    xe1_stat_vlan_id_179 ,
    xe1_stat_vlan_id_180 ,
    xe1_stat_vlan_id_181 ,
    xe1_stat_vlan_id_182 ,
    xe1_stat_vlan_id_183 ,
    xe1_stat_vlan_id_184 ,
    xe1_stat_vlan_id_185 ,
    xe1_stat_vlan_id_186 ,
    xe1_stat_vlan_id_187 ,
    xe1_stat_vlan_id_188 ,
    xe1_stat_vlan_id_189 ,
    xe1_stat_vlan_id_190 ,
    xe1_stat_vlan_id_191 ,
    xe1_stat_vlan_id_192 ,
    xe1_stat_vlan_id_193 ,
    xe1_stat_vlan_id_194 ,
    xe1_stat_vlan_id_195 ,
    xe1_stat_vlan_id_196 ,
    xe1_stat_vlan_id_197 ,
    xe1_stat_vlan_id_198 ,
    xe1_stat_vlan_id_199 ,
    xe1_stat_vlan_id_200 ,
    xe1_stat_vlan_id_201 ,
    xe1_stat_vlan_id_202 ,
    xe1_stat_vlan_id_203 ,
    xe1_stat_vlan_id_204 ,
    xe1_stat_vlan_id_205 ,
    xe1_stat_vlan_id_206 ,
    xe1_stat_vlan_id_207 ,
    xe1_stat_vlan_id_208 ,
    xe1_stat_vlan_id_209 ,
    xe1_stat_vlan_id_210 ,
    xe1_stat_vlan_id_211 ,
    xe1_stat_vlan_id_212 ,
    xe1_stat_vlan_id_213 ,
    xe1_stat_vlan_id_214 ,
    xe1_stat_vlan_id_215 ,
    xe1_stat_vlan_id_216 ,
    xe1_stat_vlan_id_217 ,
    xe1_stat_vlan_id_218 ,
    xe1_stat_vlan_id_219 ,
    xe1_stat_vlan_id_220 ,
    xe1_stat_vlan_id_221 ,
    xe1_stat_vlan_id_222 ,
    xe1_stat_vlan_id_223 ,
    xe1_stat_vlan_id_224 ,
    xe1_stat_vlan_id_225 ,
    xe1_stat_vlan_id_226 ,
    xe1_stat_vlan_id_227 ,
    xe1_stat_vlan_id_228 ,
    xe1_stat_vlan_id_229 ,
    xe1_stat_vlan_id_230 ,
    xe1_stat_vlan_id_231 ,
    xe1_stat_vlan_id_232 ,
    xe1_stat_vlan_id_233 ,
    xe1_stat_vlan_id_234 ,
    xe1_stat_vlan_id_235 ,
    xe1_stat_vlan_id_236 ,
    xe1_stat_vlan_id_237 ,
    xe1_stat_vlan_id_238 ,
    xe1_stat_vlan_id_239 ,
    xe1_stat_vlan_id_240 ,
    xe1_stat_vlan_id_241 ,
    xe1_stat_vlan_id_242 ,
    xe1_stat_vlan_id_243 ,
    xe1_stat_vlan_id_244 ,
    xe1_stat_vlan_id_245 ,
    xe1_stat_vlan_id_246 ,
    xe1_stat_vlan_id_247 ,
    xe1_stat_vlan_id_248 ,
    xe1_stat_vlan_id_249 ,
    xe1_stat_vlan_id_250 ,
    xe1_stat_vlan_id_251 ,
    xe1_stat_vlan_id_252 ,
    xe1_stat_vlan_id_253 ,
    xe1_stat_vlan_id_254 ,
    xe1_stat_vlan_id_255 ,
    stat_vlan_id_max,
} frc_vlan_stat_e;
#endif


#if FRC_CONFIG_TIMESTAMP_CHECK
typedef enum{
    stat_rxx_pkts                       ,
    stat_rxx_bytes                      ,
    xe0_stat_rxx_pkts                   ,
    xe0_stat_rxx_bytes                  ,
    xe1_stat_rxx_pkts                   ,
    xe1_stat_rxx_bytes                  ,

    stat_port0_rxx_pkts                 ,
    xe0_stat_port0_rxx_pkts             ,
    xe0_stat_port0_year_invalid_pkts    ,
    xe0_stat_port0_month_invalid_pkts   ,
    xe0_stat_port0_day_invalid_pkts     ,
    xe0_stat_port0_hour_invalid_pkts    ,
    xe0_stat_port0_minute_invalid_pkts  ,
    xe0_stat_port0_second_invalid_pkts  ,
    xe0_stat_port0_ip_err_pkts          ,
    xe0_stat_port0_timestamp_err_pkts   ,
    xe1_stat_port0_rxx_pkts             ,
    xe1_stat_port0_year_invalid_pkts    ,
    xe1_stat_port0_month_invalid_pkts   ,
    xe1_stat_port0_day_invalid_pkts     ,
    xe1_stat_port0_hour_invalid_pkts    ,
    xe1_stat_port0_minute_invalid_pkts  ,
    xe1_stat_port0_second_invalid_pkts  ,
    xe1_stat_port0_ip_err_pkts          ,
    xe1_stat_port0_timestamp_err_pkts   ,

    stat_port1_rxx_pkts                 ,
    xe0_stat_port1_rxx_pkts             ,
    xe0_stat_port1_year_invalid_pkts    ,
    xe0_stat_port1_month_invalid_pkts   ,
    xe0_stat_port1_day_invalid_pkts     ,
    xe0_stat_port1_hour_invalid_pkts    ,
    xe0_stat_port1_minute_invalid_pkts  ,
    xe0_stat_port1_second_invalid_pkts  ,
    xe0_stat_port1_ip_err_pkts          ,
    xe0_stat_port1_timestamp_err_pkts   ,
    xe1_stat_port1_rxx_pkts             ,
    xe1_stat_port1_year_invalid_pkts    ,
    xe1_stat_port1_month_invalid_pkts   ,
    xe1_stat_port1_day_invalid_pkts     ,
    xe1_stat_port1_hour_invalid_pkts    ,
    xe1_stat_port1_minute_invalid_pkts  ,
    xe1_stat_port1_second_invalid_pkts  ,
    xe1_stat_port1_ip_err_pkts          ,
    xe1_stat_port1_timestamp_err_pkts   ,

    stat_port2_rxx_pkts                 ,
    xe0_stat_port2_rxx_pkts             ,
    xe0_stat_port2_year_invalid_pkts    ,
    xe0_stat_port2_month_invalid_pkts   ,
    xe0_stat_port2_day_invalid_pkts     ,
    xe0_stat_port2_hour_invalid_pkts    ,
    xe0_stat_port2_minute_invalid_pkts  ,
    xe0_stat_port2_second_invalid_pkts  ,
    xe0_stat_port2_ip_err_pkts          ,
    xe0_stat_port2_timestamp_err_pkts   ,
    xe1_stat_port2_rxx_pkts             ,
    xe1_stat_port2_year_invalid_pkts    ,
    xe1_stat_port2_month_invalid_pkts   ,
    xe1_stat_port2_day_invalid_pkts     ,
    xe1_stat_port2_hour_invalid_pkts    ,
    xe1_stat_port2_minute_invalid_pkts  ,
    xe1_stat_port2_second_invalid_pkts  ,
    xe1_stat_port2_ip_err_pkts          ,
    xe1_stat_port2_timestamp_err_pkts   ,

    stat_port3_rxx_pkts                 ,
    xe0_stat_port3_rxx_pkts             ,
    xe0_stat_port3_year_invalid_pkts    ,
    xe0_stat_port3_month_invalid_pkts   ,
    xe0_stat_port3_day_invalid_pkts     ,
    xe0_stat_port3_hour_invalid_pkts    ,
    xe0_stat_port3_minute_invalid_pkts  ,
    xe0_stat_port3_second_invalid_pkts  ,
    xe0_stat_port3_ip_err_pkts          ,
    xe0_stat_port3_timestamp_err_pkts   ,
    xe1_stat_port3_rxx_pkts             ,
    xe1_stat_port3_year_invalid_pkts    ,
    xe1_stat_port3_month_invalid_pkts   ,
    xe1_stat_port3_day_invalid_pkts     ,
    xe1_stat_port3_hour_invalid_pkts    ,
    xe1_stat_port3_minute_invalid_pkts  ,
    xe1_stat_port3_second_invalid_pkts  ,
    xe1_stat_port3_ip_err_pkts          ,
    xe1_stat_port3_timestamp_err_pkts   ,

    stat_port4_rxx_pkts                 ,
    xe0_stat_port4_rxx_pkts             ,
    xe0_stat_port4_year_invalid_pkts    ,
    xe0_stat_port4_month_invalid_pkts   ,
    xe0_stat_port4_day_invalid_pkts     ,
    xe0_stat_port4_hour_invalid_pkts    ,
    xe0_stat_port4_minute_invalid_pkts  ,
    xe0_stat_port4_second_invalid_pkts  ,
    xe0_stat_port4_ip_err_pkts          ,
    xe0_stat_port4_timestamp_err_pkts   ,
    xe1_stat_port4_rxx_pkts             ,
    xe1_stat_port4_year_invalid_pkts    ,
    xe1_stat_port4_month_invalid_pkts   ,
    xe1_stat_port4_day_invalid_pkts     ,
    xe1_stat_port4_hour_invalid_pkts    ,
    xe1_stat_port4_minute_invalid_pkts  ,
    xe1_stat_port4_second_invalid_pkts  ,
    xe1_stat_port4_ip_err_pkts          ,
    xe1_stat_port4_timestamp_err_pkts   ,

    stat_port5_rxx_pkts                 ,
    xe0_stat_port5_rxx_pkts             ,
    xe0_stat_port5_year_invalid_pkts    ,
    xe0_stat_port5_month_invalid_pkts   ,
    xe0_stat_port5_day_invalid_pkts     ,
    xe0_stat_port5_hour_invalid_pkts    ,
    xe0_stat_port5_minute_invalid_pkts  ,
    xe0_stat_port5_second_invalid_pkts  ,
    xe0_stat_port5_ip_err_pkts          ,
    xe0_stat_port5_timestamp_err_pkts   ,
    xe1_stat_port5_rxx_pkts             ,
    xe1_stat_port5_year_invalid_pkts    ,
    xe1_stat_port5_month_invalid_pkts   ,
    xe1_stat_port5_day_invalid_pkts     ,
    xe1_stat_port5_hour_invalid_pkts    ,
    xe1_stat_port5_minute_invalid_pkts  ,
    xe1_stat_port5_second_invalid_pkts  ,
    xe1_stat_port5_ip_err_pkts          ,
    xe1_stat_port5_timestamp_err_pkts   ,

    stat_port6_rxx_pkts                 ,
    xe0_stat_port6_rxx_pkts             ,
    xe0_stat_port6_year_invalid_pkts    ,
    xe0_stat_port6_month_invalid_pkts   ,
    xe0_stat_port6_day_invalid_pkts     ,
    xe0_stat_port6_hour_invalid_pkts    ,
    xe0_stat_port6_minute_invalid_pkts  ,
    xe0_stat_port6_second_invalid_pkts  ,
    xe0_stat_port6_ip_err_pkts          ,
    xe0_stat_port6_timestamp_err_pkts   ,
    xe1_stat_port6_rxx_pkts             ,
    xe1_stat_port6_year_invalid_pkts    ,
    xe1_stat_port6_month_invalid_pkts   ,
    xe1_stat_port6_day_invalid_pkts     ,
    xe1_stat_port6_hour_invalid_pkts    ,
    xe1_stat_port6_minute_invalid_pkts  ,
    xe1_stat_port6_second_invalid_pkts  ,
    xe1_stat_port6_ip_err_pkts          ,
    xe1_stat_port6_timestamp_err_pkts   ,

    stat_port7_rxx_pkts                 ,
    xe0_stat_port7_rxx_pkts             ,
    xe0_stat_port7_year_invalid_pkts    ,
    xe0_stat_port7_month_invalid_pkts   ,
    xe0_stat_port7_day_invalid_pkts     ,
    xe0_stat_port7_hour_invalid_pkts    ,
    xe0_stat_port7_minute_invalid_pkts  ,
    xe0_stat_port7_second_invalid_pkts  ,
    xe0_stat_port7_ip_err_pkts          ,
    xe0_stat_port7_timestamp_err_pkts   ,
    xe1_stat_port7_rxx_pkts             ,
    xe1_stat_port7_year_invalid_pkts    ,
    xe1_stat_port7_month_invalid_pkts   ,
    xe1_stat_port7_day_invalid_pkts     ,
    xe1_stat_port7_hour_invalid_pkts    ,
    xe1_stat_port7_minute_invalid_pkts  ,
    xe1_stat_port7_second_invalid_pkts  ,
    xe1_stat_port7_ip_err_pkts          ,
    xe1_stat_port7_timestamp_err_pkts   ,

    stat_port8_rxx_pkts                 ,
    xe0_stat_port8_rxx_pkts             ,
    xe0_stat_port8_year_invalid_pkts    ,
    xe0_stat_port8_month_invalid_pkts   ,
    xe0_stat_port8_day_invalid_pkts     ,
    xe0_stat_port8_hour_invalid_pkts    ,
    xe0_stat_port8_minute_invalid_pkts  ,
    xe0_stat_port8_second_invalid_pkts  ,
    xe0_stat_port8_ip_err_pkts          ,
    xe0_stat_port8_timestamp_err_pkts   ,
    xe1_stat_port8_rxx_pkts             ,
    xe1_stat_port8_year_invalid_pkts    ,
    xe1_stat_port8_month_invalid_pkts   ,
    xe1_stat_port8_day_invalid_pkts     ,
    xe1_stat_port8_hour_invalid_pkts    ,
    xe1_stat_port8_minute_invalid_pkts  ,
    xe1_stat_port8_second_invalid_pkts  ,
    xe1_stat_port8_ip_err_pkts          ,
    xe1_stat_port8_timestamp_err_pkts   ,

    stat_port9_rxx_pkts                 ,
    xe0_stat_port9_rxx_pkts             ,
    xe0_stat_port9_year_invalid_pkts    ,
    xe0_stat_port9_month_invalid_pkts   ,
    xe0_stat_port9_day_invalid_pkts     ,
    xe0_stat_port9_hour_invalid_pkts    ,
    xe0_stat_port9_minute_invalid_pkts  ,
    xe0_stat_port9_second_invalid_pkts  ,
    xe0_stat_port9_ip_err_pkts          ,
    xe0_stat_port9_timestamp_err_pkts   ,
    xe1_stat_port9_rxx_pkts             ,
    xe1_stat_port9_year_invalid_pkts    ,
    xe1_stat_port9_month_invalid_pkts   ,
    xe1_stat_port9_day_invalid_pkts     ,
    xe1_stat_port9_hour_invalid_pkts    ,
    xe1_stat_port9_minute_invalid_pkts  ,
    xe1_stat_port9_second_invalid_pkts  ,
    xe1_stat_port9_ip_err_pkts          ,
    xe1_stat_port9_timestamp_err_pkts   ,

    stat_port10_rxx_pkts                 ,
    xe0_stat_port10_rxx_pkts             ,
    xe0_stat_port10_year_invalid_pkts    ,
    xe0_stat_port10_month_invalid_pkts   ,
    xe0_stat_port10_day_invalid_pkts     ,
    xe0_stat_port10_hour_invalid_pkts    ,
    xe0_stat_port10_minute_invalid_pkts  ,
    xe0_stat_port10_second_invalid_pkts  ,
    xe0_stat_port10_ip_err_pkts          ,
    xe0_stat_port10_timestamp_err_pkts   ,
    xe1_stat_port10_rxx_pkts             ,
    xe1_stat_port10_year_invalid_pkts    ,
    xe1_stat_port10_month_invalid_pkts   ,
    xe1_stat_port10_day_invalid_pkts     ,
    xe1_stat_port10_hour_invalid_pkts    ,
    xe1_stat_port10_minute_invalid_pkts  ,
    xe1_stat_port10_second_invalid_pkts  ,
    xe1_stat_port10_ip_err_pkts          ,
    xe1_stat_port10_timestamp_err_pkts   ,

    stat_port11_rxx_pkts                 ,
    xe0_stat_port11_rxx_pkts             ,
    xe0_stat_port11_year_invalid_pkts    ,
    xe0_stat_port11_month_invalid_pkts   ,
    xe0_stat_port11_day_invalid_pkts     ,
    xe0_stat_port11_hour_invalid_pkts    ,
    xe0_stat_port11_minute_invalid_pkts  ,
    xe0_stat_port11_second_invalid_pkts  ,
    xe0_stat_port11_ip_err_pkts          ,
    xe0_stat_port11_timestamp_err_pkts   ,
    xe1_stat_port11_rxx_pkts             ,
    xe1_stat_port11_year_invalid_pkts    ,
    xe1_stat_port11_month_invalid_pkts   ,
    xe1_stat_port11_day_invalid_pkts     ,
    xe1_stat_port11_hour_invalid_pkts    ,
    xe1_stat_port11_minute_invalid_pkts  ,
    xe1_stat_port11_second_invalid_pkts  ,
    xe1_stat_port11_ip_err_pkts          ,
    xe1_stat_port11_timestamp_err_pkts   ,

    stat_port12_rxx_pkts                 ,
    xe0_stat_port12_rxx_pkts             ,
    xe0_stat_port12_year_invalid_pkts    ,
    xe0_stat_port12_month_invalid_pkts   ,
    xe0_stat_port12_day_invalid_pkts     ,
    xe0_stat_port12_hour_invalid_pkts    ,
    xe0_stat_port12_minute_invalid_pkts  ,
    xe0_stat_port12_second_invalid_pkts  ,
    xe0_stat_port12_ip_err_pkts          ,
    xe0_stat_port12_timestamp_err_pkts   ,
    xe1_stat_port12_rxx_pkts             ,
    xe1_stat_port12_year_invalid_pkts    ,
    xe1_stat_port12_month_invalid_pkts   ,
    xe1_stat_port12_day_invalid_pkts     ,
    xe1_stat_port12_hour_invalid_pkts    ,
    xe1_stat_port12_minute_invalid_pkts  ,
    xe1_stat_port12_second_invalid_pkts  ,
    xe1_stat_port12_ip_err_pkts          ,
    xe1_stat_port12_timestamp_err_pkts   ,

    stat_port13_rxx_pkts                 ,
    xe0_stat_port13_rxx_pkts             ,
    xe0_stat_port13_year_invalid_pkts    ,
    xe0_stat_port13_month_invalid_pkts   ,
    xe0_stat_port13_day_invalid_pkts     ,
    xe0_stat_port13_hour_invalid_pkts    ,
    xe0_stat_port13_minute_invalid_pkts  ,
    xe0_stat_port13_second_invalid_pkts  ,
    xe0_stat_port13_ip_err_pkts          ,
    xe0_stat_port13_timestamp_err_pkts   ,
    xe1_stat_port13_rxx_pkts             ,
    xe1_stat_port13_year_invalid_pkts    ,
    xe1_stat_port13_month_invalid_pkts   ,
    xe1_stat_port13_day_invalid_pkts     ,
    xe1_stat_port13_hour_invalid_pkts    ,
    xe1_stat_port13_minute_invalid_pkts  ,
    xe1_stat_port13_second_invalid_pkts  ,
    xe1_stat_port13_ip_err_pkts          ,
    xe1_stat_port13_timestamp_err_pkts   ,

    stat_port14_rxx_pkts                 ,
    xe0_stat_port14_rxx_pkts             ,
    xe0_stat_port14_year_invalid_pkts    ,
    xe0_stat_port14_month_invalid_pkts   ,
    xe0_stat_port14_day_invalid_pkts     ,
    xe0_stat_port14_hour_invalid_pkts    ,
    xe0_stat_port14_minute_invalid_pkts  ,
    xe0_stat_port14_second_invalid_pkts  ,
    xe0_stat_port14_ip_err_pkts          ,
    xe0_stat_port14_timestamp_err_pkts   ,
    xe1_stat_port14_rxx_pkts             ,
    xe1_stat_port14_year_invalid_pkts    ,
    xe1_stat_port14_month_invalid_pkts   ,
    xe1_stat_port14_day_invalid_pkts     ,
    xe1_stat_port14_hour_invalid_pkts    ,
    xe1_stat_port14_minute_invalid_pkts  ,
    xe1_stat_port14_second_invalid_pkts  ,
    xe1_stat_port14_ip_err_pkts          ,
    xe1_stat_port14_timestamp_err_pkts   ,

    stat_port15_rxx_pkts                 ,
    xe0_stat_port15_rxx_pkts             ,
    xe0_stat_port15_year_invalid_pkts    ,
    xe0_stat_port15_month_invalid_pkts   ,
    xe0_stat_port15_day_invalid_pkts     ,
    xe0_stat_port15_hour_invalid_pkts    ,
    xe0_stat_port15_minute_invalid_pkts  ,
    xe0_stat_port15_second_invalid_pkts  ,
    xe0_stat_port15_ip_err_pkts          ,
    xe0_stat_port15_timestamp_err_pkts   ,
    xe1_stat_port15_rxx_pkts             ,
    xe1_stat_port15_year_invalid_pkts    ,
    xe1_stat_port15_month_invalid_pkts   ,
    xe1_stat_port15_day_invalid_pkts     ,
    xe1_stat_port15_hour_invalid_pkts    ,
    xe1_stat_port15_minute_invalid_pkts  ,
    xe1_stat_port15_second_invalid_pkts  ,
    xe1_stat_port15_ip_err_pkts          ,
    xe1_stat_port15_timestamp_err_pkts   ,

    stat_timestamp_max,
} frc_timestamp_stat_e;
#endif
#define FRC_STAT_NAME_SIZE      32

typedef struct {
    uint64_t local;
    uint64_t remote;
} frc_temp_t;


typedef struct {
    uint64_t rx_errs;
    uint64_t rx_pkts;
    uint64_t rx_bytes;

    uint64_t tx_errs;
    uint64_t tx_pkts;
    uint64_t tx_bytes;
} frc_port_stat_t;

typedef struct {
#if __FRC_BYTE_ORDER == __FRC_LITTLE_ENDIAN
    uint64_t enable:8;
    uint64_t linked:8;
    uint64_t speed:16;
    uint64_t mtu:16;
    uint64_t flags:16;
#else
    uint64_t flags:16;
    uint64_t mtu:16;
    uint64_t speed:16;
    uint64_t linked:8;
    uint64_t enable:8;
#endif
} frc_port_status_t;


typedef struct {
#if __FRC_BYTE_ORDER == __FRC_LITTLE_ENDIAN
    uint16_t port;
    uint16_t devad;
    uint16_t addr;
    uint16_t value;
#else
    uint16_t value;
    uint16_t addr;
    uint16_t devad;
    uint16_t port;
#endif
} frc_phy_op_t;


typedef struct {
    uint8_t op;
    frc_phy_op_t phy;
} frc_bdd_phy_t;

typedef struct {
    uint8_t  enable;      /*1 for enable; 0 for disable */
    uint8_t  disorder_depth; /*default 5,value  from 1-255*/
    uint8_t  age_time;    /* default 3, value from 1-31 */
    uint8_t  retrans_time;  /* default 2, value from 1-31*/
} frc_fr_status_t;


typedef struct  {
#if __FRC_BYTE_ORDER == __FRC_LITTLE_ENDIAN
    uint32_t  sip;      /*source ip */
    uint32_t  dip;    /* destination ip */
    uint16_t  sp;  /* source port*/
    uint16_t  dp; /*destination port*/
    uint16_t  proto; /*0 for tcp; 1 for udp */
    uint16_t  reserve;
#else
    uint32_t  dip;
    uint32_t  sip;
    uint16_t  reserve;
    uint16_t  proto;
    uint16_t  dp;
    uint16_t  sp;
#endif
} frc_fr_tuple_t;

typedef struct {
#if __FRC_BYTE_ORDER == __FRC_LITTLE_ENDIAN
    uint32_t  pkts;      /*pkts number */
    uint32_t  bytes;    /* bytes number */
#else
    uint32_t bytes;
    uint32_t pkts;
#endif
} frc_fr_session_stat_t;



typedef struct {
    uint8_t link;  /* 0 for down; 1 for up*/
    uint8_t enable; /* 0 for oct disable; 1 for oct enable */
    uint8_t work_mode; /*  0 for nic mode; 1 for work mode */
    uint8_t loopback_mode;
    uint16_t optical_status;
    uint16_t optical;
    uint32_t rx_pkts_rate;
    uint32_t rx_bytes_rate;
    uint32_t tx_pkts_rate;
    uint32_t tx_bytes_rate;
} frc_oct_t;


typedef struct {
    frc_version_t  frcore_version;
    frc_version_t  frcdrv_version;
    frc_version_t  frctweak_version;
    frc_version_t  octdrv_version;
    frc_version_t  octnic_version;
    uint32_t  cpld_version;
    uint32_t    port_number;
} frc_bdd_info_out_t;


typedef struct {
    frc_fr_tuple_t  five_tuple;
    frc_fr_session_stat_t session_stat;
} frc_fr_session_t;

typedef struct {
    uint32_t num;    /*session number*/
    frc_fr_session_t session[MAX_HASH_SESSION];
} frc_fr_hash_session_t;

typedef struct {
    frc_fr_tuple_t tuple;
    uint64_t num;
} frc_ssn_match_t;


typedef struct{
#if __FRC_BYTE_ORDER == __FRC_LITTLE_ENDIAN
    uint32_t addr;  /* dma addr */
    uint32_t size;  /* data length */
#else
    uint32_t size;
    uint32_t addr;
#endif
} frc_bdd_dma_read_in_t;


typedef struct {
    frc_temp_t   temp;
    uint16_t  dma_block_size;
    uint16_t  running_status;
    uint32_t rule_max;
    uint32_t ssn_max;
    frc_oct_t oct0;
    frc_oct_t oct1;
} frc_bdd_status_out_t;

typedef struct{
#if __FRC_BYTE_ORDER == __FRC_LITTLE_ENDIAN
    uint32_t addr;
    uint32_t data;
#else
    uint32_t data;
    uint32_t addr;
#endif
} frc_cpld_op_t;

typedef struct {
    uint8_t op;
    frc_cpld_op_t cpld;
} frc_bdd_cpld_t;


typedef struct {
    uint8_t oct;
    uint8_t mode;  /*0 for nic, 1 for business*/
} frc_bdd_workmode_set_in_t;

typedef struct {
    uint8_t port;
    uint8_t enable; /* 1 for enable port; 0 for disable port */
} frc_bdd_set_port_in_t;

typedef struct {
    uint8_t oct;
    uint8_t link;
} frc_bdd_force_link_in_t;

typedef struct{
    uint8_t port;  /* 0 for oct0; 1 for oct1 */
    uint8_t loopback; /* 0 for line phy loopback;
                         1 for line core loopback;
                         2 for host phy loopback;
                         3 for host core loopback? */
} frc_bdd_set_loopback_in_t;

typedef struct{
    uint8_t log; /* bit 0 for config log level;
                    bit 1 for event log level;
                    bit 2 for debug log level;
                    bit 3 for error log level;
                    bit 4 for warning log level;
                    bit 5 for alarm log level;
                    1 for effective*/
    uint8_t enable; /* 1 for enable; 0 for disable */
} frc_bdd_log_in_t;


typedef struct {
    uint16_t index;    /*rule index, from 0 to 1999*/
    uint16_t num;     /*rule number, from 1 to 2000 */
} frc_rule_del_in_t;

typedef struct {
    uint16_t index;    /*rule index, from 0 to 1999*/
    uint16_t num;     /*rule number, from 1 to 2000 */
} frc_rule_stat_in_t;


typedef struct {
    uint16_t index;    /*rule index, from 0 to 1999*/
    uint16_t num;     /*rule number, from 1 to 2000 */
} frc_rule_clear_stat_in_t;


typedef struct {
#if __FRC_BYTE_ORDER == __FRC_LITTLE_ENDIAN
    uint32_t index;
    uint32_t num;
#else
    uint32_t num;
    uint32_t index;
#endif
} frc_rule_op_in_t;

typedef struct {
#if __FRC_BYTE_ORDER == __FRC_LITTLE_ENDIAN
    uint16_t  rule_source;   /* rule source*/
    uint16_t  rule_type;     /*rule type*/
    uint16_t  action;           /*action */
    uint16_t  op;
    uint32_t  sip;      /*source ip */
    uint32_t  dip;    /* destination ip */
    uint16_t  sp;  /* source port*/
    uint16_t  dp; /*destination port*/
    uint16_t  proto; /*0 for tcp; 1 for udp */
    uint16_t index;    /*index from 1 to 2000 */
#else
    uint16_t op;
    uint16_t action;
    uint16_t rule_type;
    uint16_t rule_source;
    uint32_t dip;
    uint32_t sip;
    uint16_t index;
    uint16_t proto;
    uint16_t dp;
    uint16_t sp;
#endif
                     /*operation type*/
} frc_rule_t;

typedef struct {
#if __FRC_BYTE_ORDER == __FRC_LITTLE_ENDIAN
    uint16_t  rule_source;   /* rule source*/
    uint16_t  rule_type;     /*rule type*/
    uint16_t  action;           /*action */
    uint16_t  op;
    uint8_t   sip[24];      /*source ip */
    uint8_t   dip[24];    /* destination ip */
    uint8_t   sp[16];  /* source port*/
    uint8_t   dp[16]; /*destination port*/
    uint8_t   proto[8]; /*0 for tcp; 1 for udp */
    uint64_t  index;    /*index from 0 to 1999 */
#else
    uint16_t op;
    uint16_t action;
    uint16_t rule_type;
    uint16_t rule_source;
    uint8_t   sip[24];      /*source ip */
    uint8_t   dip[24];    /* destination ip */
    uint8_t   sp[16];  /* source port*/
    uint8_t   dp[16]; /*destination port*/
    uint8_t   proto[8]; /*0 for tcp; 1 for udp */
    uint64_t  index;    /*index from 0 to 1999 */
#endif
                     /*operation type*/
} frcc_rule_t;

typedef struct {
#if __FRC_BYTE_ORDER == __FRC_LITTLE_ENDIAN
    uint32_t  pkts;      /*pkts number */
    uint32_t  bytes;    /* bytes number */
#else
    uint32_t bytes;
    uint32_t pkts;
#endif
}frc_rule_pkt_stat_t;

typedef struct {
    frc_rule_t  rule;   /*rule*/
    frc_rule_pkt_stat_t stat;
}frc_rule_stat_t;

typedef struct {
    uint32_t  num;    /*rule number  from 1 to 2000 */
    frc_rule_stat_t  rule_stat[MAX_RULE];/* rule and statistics*/
} frc_rule_stat_out_t;

#if FRC_CONFIG_TWO_TUPLE

typedef enum {
    FRC_ACL_UNKNOWN,
    FRC_ACL_SIP,
    FRC_ACL_DIP,
    FRC_ACL_SP,
    FRC_ACL_DP,
} frc_acl_type_t;

typedef struct {
    uint16_t index;    /*acl index, from 0 to 1999*/
    uint16_t num;     /*acl number, from 1 to 2000 */
} frc_acl_del_in_t;

typedef struct {
    uint16_t index;    /*acl index, from 0 to 1999*/
    uint16_t num;     /*acl number, from 1 to 2000 */
} frc_acl_stat_in_t;


typedef struct {
    uint16_t index;    /*acl index, from 0 to 1999*/
    uint16_t num;     /*acl number, from 1 to 2000 */
} frc_acl_clear_stat_in_t;


typedef struct {
#if __FRC_BYTE_ORDER == __FRC_LITTLE_ENDIAN
    uint32_t index;
    uint32_t num;
#else
    uint32_t num;
    uint32_t index;
#endif
} frc_acl_op_in_t;

typedef struct {
#if __FRC_BYTE_ORDER == __FRC_LITTLE_ENDIAN
    uint16_t  acl_source;   /* acl source*/
    uint16_t  acl_type;     /*acl type*/
    uint16_t  action;       /*action */
    uint16_t  op;
    uint32_t  one_tuple;    /*one of SIP/DIP/SP/DP */
    uint16_t  proto;        /*0 for tcp; 1 for udp */
    uint16_t  index;        /*index from 1 to 2000 */
    uint32_t  hash;         /* hash value */
    uint32_t  reserved;
#else
    uint16_t op;
    uint16_t action;
    uint16_t acl_type;
    uint16_t acl_source;
    uint16_t index;
    uint16_t proto;
    uint32_t one_tuple;
    uint32_t reserved;
    uint32_t hash;
#endif
                     /*operation type*/
} frc_acl_t;

typedef struct{
#if __FRC_BYTE_ORDER == __FRC_LITTLE_ENDIAN
    uint32_t hash;
    uint32_t bucket_depth;
    uint32_t total_cell;
    uint32_t del_cell;
#else
    uint32_t bucket_depth;
    uint32_t hash;
    uint32_t del_cell;
    uint32_t total_cell;
#endif
} frc_acl_hash_table_t;

typedef struct {
#if __FRC_BYTE_ORDER == __FRC_LITTLE_ENDIAN
    uint16_t  acl_source;   /* acl source*/
    uint16_t  acl_type;     /*acl type*/
    uint16_t  action;           /*action */
    uint16_t  op;
    uint8_t   sip[24];      /*source ip */
    uint8_t   dip[24];    /* destination ip */
    uint8_t   sp[16];  /* source port*/
    uint8_t   dp[16]; /*destination port*/
    uint8_t   proto[8]; /*0 for tcp; 1 for udp */
    uint64_t  index;    /*index from 0 to 1999 */
#else
    uint16_t op;
    uint16_t action;
    uint16_t acl_type;
    uint16_t acl_source;
    uint8_t   sip[24];      /*source ip */
    uint8_t   dip[24];    /* destination ip */
    uint8_t   sp[16];  /* source port*/
    uint8_t   dp[16]; /*destination port*/
    uint8_t   proto[8]; /*0 for tcp; 1 for udp */
    uint64_t  index;    /*index from 0 to 1999 */
#endif
                     /*operation type*/
} frcc_acl_t;

typedef struct {
#if __FRC_BYTE_ORDER == __FRC_LITTLE_ENDIAN
    uint32_t  pkts;      /*pkts number */
    uint32_t  bytes;    /* bytes number */
#else
    uint32_t bytes;
    uint32_t pkts;
#endif
}frc_acl_pkt_stat_t;

typedef struct {
    frc_acl_t  acl;   /*acl*/
    frc_acl_pkt_stat_t stat;
}frc_acl_stat_t;

typedef struct {
    uint32_t  num;    /*acl number  from 1 to 2000 */
    frc_acl_stat_t  acl_stat[MAX_RULE];/* acl and statistics*/
} frc_acl_stat_out_t;

typedef struct {
#if __FRC_BYTE_ORDER == __FRC_LITTLE_ENDIAN
    uint32_t acl_type;
    uint32_t reserved;
    uint32_t index;
    uint32_t num;
#else
    uint32_t reserved;
    uint32_t acl_type;
    uint32_t num;
    uint32_t index;
#endif
} frc_acl_hash_table_op_in_t;

typedef struct {
    uint64_t  num;    /*hash number  from 1 to 2000 */
    frc_acl_hash_table_t  acl_hash_stat[MAX_RULE];/* hash table statistics */
} frc_acl_hash_table_stat_out_t;
#endif /* end of FRC_CONFIG_TWO_TUPLE*/


#if FRC_CONFIG_VLAN_IV
typedef struct {
#if __FRC_BYTE_ORDER == __FRC_LITTLE_ENDIAN
    uint32_t hash_type;
    uint32_t ip4;
#else
    uint32_t ip4;
    uint32_t hash_type;
#endif
    uint8_t ip6[16];
} frc_vlan_hash_mask_t;
#endif

#if FRC_CONFIG_VLAN_CHECK
typedef enum {
    FRC_VLAN_CHECK_UNKNOWN,
    FRC_VLAN_CHECK_SIP,
    FRC_VLAN_CHECK_DIP,
    FRC_VLAN_CHECK_SDIP,
    FRC_VLAN_CHECK_MAX,
} frc_vlan_check_type_t;
typedef struct {
#if __FRC_BYTE_ORDER == __FRC_LITTLE_ENDIAN
    uint16_t start_id;
    uint8_t  type;
    uint8_t  resvered0;
    uint16_t  num;
    uint16_t reserved;
#else
    uint16_t reserved;
    uint16_t  num;
    uint8_t  resvered0;
    uint8_t  type;
    uint16_t start_id;
#endif
} frc_vlan_check_para_t;
#endif
typedef struct{
#if __FRC_BYTE_ORDER == __FRC_LITTLE_ENDIAN
    uint32_t index;
    uint32_t num;
#else
    uint32_t num;
    uint32_t index;
#endif
} frc_stat_op_in_t;


typedef struct{
#if __FRC_BYTE_ORDER == __FRC_LITTLE_ENDIAN
    uint32_t index;
    uint32_t num;
    uint32_t port;
    uint32_t reserv;
#else
    uint32_t num;
    uint32_t index;
    uint32_t reserv;
    uint32_t port;
#endif
} frc_vlan_op_in_t;

typedef struct{
    uint64_t mac;
    uint64_t counter;
} frc_mac_stat_in_t;

typedef struct{
    uint64_t mac;
    uint64_t total;
    uint64_t errors;
} frc_mac_stat_out_t;
#if FRC_CONFIG_TIMESTAMP_CHECK
typedef struct{
#if __FRC_BYTE_ORDER == __FRC_LITTLE_ENDIAN
    uint32_t index;
    uint32_t num;
    uint32_t port;
    uint32_t reserv;
#else
    uint32_t num;
    uint32_t index;
    uint32_t reserv;
    uint32_t port;
#endif
} frc_timestamp_op_in_t;
#endif

#if FRC_CONFIG_IPC
#define IPC_INSTR_PROTOCOL  0x8062
#define IPC_INSTR_OUI       0x2145
#define IPC_INSTR_POOL      5
#define INSTR_PAYLOAD_SZ 32

typedef struct
{
#if __FRC_BYTE_ORDER == __FRC_LITTLE_ENDIAN
    uint16_t  ouid;
    uint16_t  ouim;
    uint16_t  iifd;
    uint16_t  iifm;
    uint64_t  pidd;
    uint64_t  pidm;
#else
    uint16_t  iifm;
    uint16_t  iifd;
    uint16_t  ouim;
    uint16_t  ouid;
    uint64_t  pidd;
    uint64_t  pidm;
#endif
} ipc_exp_t;


typedef struct
{
#if __FRC_BYTE_ORDER == __FRC_LITTLE_ENDIAN
    uint8_t smac_check;
    uint8_t pktid_check;
    uint8_t exp_check;
    uint8_t invalid_dump;
    uint8_t instr_send;
    uint8_t rev1;
    uint16_t rev2;
#else
    uint16_t rev2;
    uint8_t rev1;
    uint8_t instr_send;
    uint8_t invalid_dump;
    uint8_t exp_check;
    uint8_t pktid_check;
    uint8_t smac_check;
#endif
} ipc_misc_t;

#define IPC_IP_LEN_MAX 2048
#define IPC_IIF_MAX     0x400

typedef struct
{
#if __FRC_BYTE_ORDER == __FRC_LITTLE_ENDIAN
    uint32_t tx_port;
    uint32_t action;
    uint32_t  type;
    uint32_t ip_len;
    uint32_t sip_data;
    uint32_t sip_mask;
    uint32_t dip_data;
    uint32_t dip_mask;
#else
    uint32_t action;
    uint32_t tx_port;
    uint32_t ip_len;
    uint32_t  type;
    uint32_t sip_mask;
    uint32_t sip_data;
    uint32_t dip_mask;
    uint32_t dip_data;
#endif
    
} ipc_instr_cfg_t;

typedef struct
{
    ipc_instr_cfg_t instr_cfg;
    uint8_t payload[IPC_IP_LEN_MAX];
} ipc_instr_t;

typedef struct
{
    uint8_t data[INSTR_PAYLOAD_SZ];
    uint64_t index;
}ipc_payload_set_in_t;

typedef struct
{
    int64_t smac;
    int64_t pktid;
} ipc_cur_t;
#endif

#define FRC_CMD_RESPOND_DATA_SZ (1024)

#define ULL unsigned long long






#endif /* !__FRC_TYPES_H__ */

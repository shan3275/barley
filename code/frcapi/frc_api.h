#ifndef __FRC_API_H__
#define __FRC_API_H__

#include "frc_ioctl.h"
#include <sys/ioctl.h>
#include <fcntl.h>
#include "frc_types.h"
#include "frc_cmd.h"
#include "frc_dma.h"
#include "frc_debug.h"
//#define MAX_RULE 2000



/*The flow restoration module*/
int frcapi_fr_status_get(frc_fr_status_t *status);
int frcapi_fr_enable(uint8_t enable);
int frcapi_fr_age_time_set(uint8_t age);
//int frcapi_fr_retrans_time_set(uint8_t retrans);
int frcapi_fr_disorder_depth_set(uint8_t disorder);
//int frcapi_ssn_get(frc_fr_tuple_t *fr_tuple, frc_fr_session_stat_t *fr_stat);
int frcapi_ssn_get(frc_fr_tuple_t *fr_tuple, frc_fr_session_stat_t *fr_stat);
int frcapi_ssn_bucket_get(uint32_t hash, frc_fr_hash_session_t *hash_session);
int frcapi_ssn_match(frc_ssn_match_t *ssn_match, uint64_t *num);


/*The rule module*/
int frcapi_rule_status_get(uint8_t *enable);
int frcapi_rule_enable(uint8_t enable);
int frcapi_rule_add(frc_rule_t *rule);
int frcapi_rule_del(frc_rule_op_in_t *rule_del_in, uint16_t *num);
int frcapi_rule_stat_get(frc_rule_op_in_t *stat_in, frc_rule_stat_out_t *stat_out);
//int frcapi_rule_cnt_get(frc_rule_op_in_t *rule_cnt_in, frc_rule_pkt_stat_t *cnt_out);
int frcapi_rule_stat_clear(frc_rule_op_in_t *clear_stat_in, uint16_t *num);
int frcapi_rule_clear(frc_rule_op_in_t *rule_del_in, uint16_t *num);
int frcapi_rule_update(void);
int frcapi_rule_num_get(uint16_t *num);
//int frcapi_rule_match(frc_rule_t *rule, uint16_t *num);
int frcapi_rule_match(frc_rule_stat_t *rule_stat, frc_rule_t *rule, uint16_t rule_num, uint16_t *match_num);

#if FRC_CONFIG_TWO_TUPLE
int frcapi_acl_status_get(uint8_t *enable);
int frcapi_acl_enable(uint8_t enable);
int frcapi_acl_add(frc_acl_t *acl);
int frcapi_acl_del(frc_acl_op_in_t *acl_del_in, uint16_t *num);
int frcapi_acl_stat_get(frc_acl_op_in_t *stat_in, frc_acl_stat_out_t *stat_out);
int frcapi_acl_hash_table_stat_get(frc_acl_hash_table_op_in_t *stat_in,
                                   frc_acl_hash_table_stat_out_t *stat_out);
//int frcapi_acl_cnt_get(frc_acl_op_in_t *acl_cnt_in, frc_acl_pkt_stat_t *cnt_out);
int frcapi_acl_stat_clear(frc_acl_op_in_t *clear_stat_in, uint16_t *num);
int frcapi_acl_clear(frc_acl_op_in_t *acl_del_in, uint16_t *num);
int frcapi_acl_update(void);
int frcapi_acl_num_get(uint16_t *num);
//int frcapi_acl_match(frc_acl_t *acl, uint16_t *num);
int frcapi_acl_match(frc_acl_stat_t *acl_stat, frc_acl_t *acl, uint16_t acl_num, uint16_t *match_num);
#endif

/*The board manage module*/
int frcapi_bdd_info_get(frc_bdd_info_out_t *bdd_info);
int frcapi_frcdrv_version_get(frc_version_t *frcdrv);
int frcapi_bdd_status_get(frc_bdd_status_out_t *bdd_status);
int frcapi_bdd_poweroff(void);
int frcapi_bdd_port_enable(frc_bdd_set_port_in_t *set_port);
int frcapi_bdd_port_loopback_set(frc_bdd_set_loopback_in_t *mode);
int frcapi_bdd_log_config_set(frc_bdd_log_in_t *log_in);
int frcapi_bdd_phy(frc_bdd_phy_t *input, frc_phy_op_t *output);
int frcapi_bdd_cpld(frc_bdd_cpld_t *input, frc_cpld_op_t *output);
int frcapi_bdd_workmode_set(frc_bdd_workmode_set_in_t *workmode);
int frcapi_bdd_force_link(frc_bdd_force_link_in_t *force_link);


/*The PCI module*/
//int frcapi_dma_block_set(uint8_t size);
//int frcapi_dma_mem_read(frc_bdd_dma_read_in_t *dma_in, uint8_t *data);


/*The stat module*/
int frcapi_stat_clear(void);
int frcapi_pkt_stat_get(frc_stat_op_in_t *input, uint64_t *stat);
int frcapi_chan_or_pr_ssn_start(uint64_t type, frc_dma_ssn_chan_desc_t *desc);
int frcapi_chan_or_pr_rule_or_udp_start(uint64_t type, frc_dma_chan_desc_t *desc);

/* chan test */
int frcapi_chan_test_start(uint64_t *chan, uint16_t *olen);
int frcapi_sysinfo_get(frc_system_info_t *output);

#if FRC_CONFIG_UDP
int frcapi_udp_status_get(uint8_t *enable);
int frcapi_udp_enable(uint8_t enable);
#endif
#if FRC_CONFIG_VLAN_CHECK
int frcapi_vlan_check_status_get(frc_vlan_check_para_t *vlan_check_para);
int frcapi_vlan_check_set_parameter(frc_vlan_check_para_t *vlan_check_para);
int frcapi_vlan_check_stat_clear(void);
int frcapi_vlan_check_stat_get(frc_vlan_op_in_t *input, uint64_t *stat);
#endif
#if FRC_CONFIG_MAC_STATISTICS
int frcapi_mac_stat_set(frc_mac_stat_in_t *input, frcore_user_cmd_e cmd);
int frcapi_mac_stat_get(frc_mac_stat_in_t *input, frc_mac_stat_out_t *stat, frcore_user_cmd_e cmd);
int frcapi_mac_stat_get_all(frc_mac_stat_in_t *input, frc_mac_stat_out_t *stat, uint16_t num, frcore_user_cmd_e cmd);
#endif
#if FRC_CONFIG_TIMESTAMP_CHECK
int frcapi_timestamp_check_stat_clear(void);
int frcapi_timestamp_check_stat_get(frc_timestamp_op_in_t *input, uint64_t *stat);
#endif
#if FRC_CONFIG_IPC
int frcapi_ipc_misc_get(ipc_misc_t *misc);
int frcapi_ipc_misc_set(ipc_misc_t *misc);
int frcapi_ipc_cur_get(ipc_cur_t *cur);
int frcapi_ipc_cur_set(ipc_cur_t *cur);
int frcapi_ipc_exp_get(ipc_exp_t *exp);
int frcapi_ipc_exp_set(ipc_exp_t *exp);
int frcapi_ipc_instr_get(ipc_instr_cfg_t *instr);
int frcapi_ipc_instr_set(ipc_instr_cfg_t *instr);
int frcapi_ipc_instr_payload_set(ipc_payload_set_in_t *payload_set);
#endif
#if FRC_CONFIG_VLAN_IV
int frctweak_ipc_hash4_mask_set(frc_vlan_hash_mask_t *mask_set);
int frctweak_ipc_hash6_mask_set(frc_vlan_hash_mask_t *mask_set);
#endif
#endif


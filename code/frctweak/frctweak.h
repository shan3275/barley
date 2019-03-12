#ifndef __FRCTWEAK_H__
#define __FRCTWEAK_H__

#include "frc.h"
#include "frcapi.h"
#include "frc_list.h"

#define FRCTWEAK_PSTR_SZ    256

typedef int (*frctweak_fn_t) (int argc, char **argv);
typedef void (*frctweak_usage_fn_t) (void);

typedef struct frctweak_cmd_s {
    frc_list_t node;
    frc_list_t *head;
    struct frctweak_cmd_s *parent;
    char name[128];
    char summary[128];
    frctweak_fn_t fn;
    frctweak_usage_fn_t usage;
} frctweak_cmd_t;

frc_list_t frctweak_cmd_head;

int parse_loopback(char *str, uint8_t *value);
int parse_log(char *str, uint8_t *value);
void fr_u8_string(uint8_t val, char str[18]);
void fr_u16_string(uint16_t val, char str[18]);
void fr_u32_string(uint32_t val, char str[18]);
void fr_proto_string(uint16_t proto, char str[18]);
void fr_port_string(uint16_t port, char str[18]);
void fr_ipv4_string(uint32_t ip, char str[18]);
int parse_protocal(char *str, uint16_t *protocal);
#if FRC_CONFIG_TWO_TUPLE
int parse_acl_type(char *str, uint16_t *acl_type);
#endif
#if FRC_CONFIG_VLAN_CHECK
int parse_vlan_check_type(char *str, uint8_t *type);
#endif

int parse_bool(char *str, uint8_t *value);
int parse_mac64(char *str, uint64_t *val);
int parse_u64(char *str, uint64_t *value);
int parse_u32(char *str, uint32_t *value);
int parse_u32_inc(char *str, uint32_t *value, uint32_t *inc);
int parse_u16(char *str, uint16_t *value);
int parse_u16_inc(char *str, uint16_t *value, uint16_t *inc);
int parse_u8(char *str, uint8_t *value);
int parse_ipv4(char *str, uint32_t *ip);
int parse_ipv4_inc(char *str, uint32_t *ip, uint32_t *inc);
void frctweak_parent_string(frctweak_cmd_t *cmd, char pstr);
int u8_string(uint8_t val, char *str);
int mac_string(uint64_t val, char *str);
int u64_string(uint64_t val, char *str);
int u16_string(uint16_t val, char *str);

frctweak_cmd_t* frctweak_cmd_register(frctweak_cmd_t *parent, char *name, char *summary, frctweak_fn_t fn, frctweak_usage_fn_t usage);

int frctweak_misc_cmd_init(frctweak_cmd_t *cmd);
int frctweak_port_cmd_init(frctweak_cmd_t *cmd);
//int frctweak_debug_cmd_init(frctweak_cmd_t *cmd);
int frctweak_test_cmd_init(frctweak_cmd_t *cmd);
int frctweak_phy_cmd_init(frctweak_cmd_t *cmd);
int frctweak_chan_cmd_init(frctweak_cmd_t *cmd);
int frctweak_fr_cmd_init(frctweak_cmd_t *cmd);
int frctweak_pr_cmd_init(frctweak_cmd_t *cmd);
int frctweak_rule_cmd_init(frctweak_cmd_t *cmd);
int frctweak_bmm_cmd_init(frctweak_cmd_t *cmd);
int frctweak_stat_cmd_init(frctweak_cmd_t *cmd);
int frctweak_file_cmd_init(frctweak_cmd_t *cmd);
#if FRC_CONFIG_TWO_TUPLE
int frctweak_acl_cmd_init(frctweak_cmd_t *cmd);
#endif
#if FRC_CONFIG_UDP
int frctweak_udp_cmd_init(frctweak_cmd_t *cmd);
#endif
#if FRC_CONFIG_VLAN_CHECK
int frctweak_vlan_check_cmd_init(frctweak_cmd_t *cmd);
#endif
#if FRC_CONFIG_MAC_STATISTICS
int frctweak_mac_stat_cmd_init(frctweak_cmd_t *cmd);
#endif
#if FRC_CONFIG_TIMESTAMP_CHECK
int frctweak_timestamp_check_cmd_init(frctweak_cmd_t *cmd);
#endif
#if FRC_CONFIG_IPC
int frctweak_ipc_cmd_init(frctweak_cmd_t *cmd);
#endif

extern frctweak_cmd_t *frctweak_cmd;
extern char *program;

#if FRC_DEBUG_TWEAK
#   define FRCTWEAK_ERROR(_fmt, _args...)  printf("[ERROR] %s.%d:" _fmt, __func__, __LINE__, ##_args)
#   define FRCTWEAK_DEBUG(_fmt, _args...)  printf("[DEBUG] %s.%d:" _fmt, __func__, __LINE__, ##_args)
#else
#   define FRCTWEAK_ERROR(_fmt, _args...)
#   define FRCTWEAK_DEBUG(_fmt, _args...)
#endif

void frctweak_version_get(void *ptr);




#endif /* !__FRCTWEAK_H__ */

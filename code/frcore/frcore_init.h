
/**
 * @file
 *
 * MC functions for init.
 *
 * <hr>$Revision: 403 $<hr>
 */
#ifndef _FRCORE_INIT_H_
#define _FRCORE_INIT_H_

#include "frcore_filter.h"

#define MC_ERROR                -1
#define MC_OK                   1

/**
 * bootmem named block
 */
/**
 * bootmem named block
 */
#define MC_BOOTMEM_SHARED_DATA_NAME  "shared_data"
#define MC_BOOTMEM_CROSSLINK_NAME    "crosslink  "
#define MC_BOOTMEM_SSN_NAME          "session    "
#define MC_BOOTMEM_SSN_ADDR_NAME     "session_addr"
#define MC_BOOTMEM_BLC_NAME          "balancer   "
#define MC_BOOTMEM_CPSER_NAME        "copytoserver"
#define MC_BOOTMEM_IP_TREE_NAME      "ip_mac_tree"
#define MC_BOOTMEM_IP_TREE_CHILDREN_NAME      "ip_mac_tree_children"
#define MC_BOOTMEM_EPIF_NAME         "egress     "
#define MC_BOOTMEM_VLAN_NAME         "vlan       "
#define MC_BOOTMEM_IFSTAT_NAME       "ifstat     "
#define MC_BOOTMEM_IFSTAT_TIM_NAME   "ifstat_tim "
#define MC_BOOTMEM_IF_QUEUE_TIM_NAME   "if_queue_tim"
#define MC_BOOTMEM_QOS_QUEUE_TIM_NAME   "qos_queue_tim"
#define MC_BOOTMEM_QOS_ID_NAME       "qos_id     "
#define MC_BOOTMEM_QOS_ID_WCQ_NAME   "qos_id_wcq"
#define MC_BOOTMEM_TIM_NAME          "timer      "
#define MC_BOOTMEM_IF_WORK_CQ_NAME   "epif_work_cq"
#define MC_BOOTMEM_SYNFLD_CTL        "synfld_ctl"
#define MC_BOOTMEM_TCP_FLOW_REC      "tcp_flow_recovery"
#define MC_BOOTMEM_FPA_INFO_NAME        "fpa_info"

#define MC_BOOTMEM_DATA_NAME         "data"
#define MC_BOOTMEM_IPFRAG_NAME       "ipfrag"
#define MC_BOOTMEM_NTPTS_NAME        "ntpts"
#define MC_BOOTMEM_RFC_NAME          "rfc_sharemem_data"
#define MC_BOOTMEM_SPI_CNTER_NAME    "spi_cnter"
#define MC_BOOTMEM_GSN_INFO_NAME     "gsn_info"
#define MC_BOOTMEM_ACL_RULE          "acl_rule"
#define MC_BOOTMEM_TWO_TUPLE_ACL_RULE     "two_tuple_acl_rule"
#define MC_BOOTMEM_TWO_TUPLE_ACL_HASH_TABLE     "two_tuple_acl_hash_table"
#define MC_BOOTMEM_ACL_INDEX         "acl_index"
#define MC_BOOTMEM_SM                "sm"
#define MC_BOOTMEM_PORT_NAME         "port"
#define MC_BOOTMEM_PORT_MONITOR_NAME "port_monitor"
#define MC_BOOTMEM_IPFRAG_INFO_NAME  "ipfrag"

enum mpp_running_status{
    MPP_INIT=0,
    MPP_SW_INIT_DONE,
    MPP_SW_START,
    MPP_SW_END
};

enum mpp_init_config{
    MPP_INTEGREATED_LINUX=0,
    MPP_ACCELARATED_LINUX,
    MPP_FCASTING,
    MPP_TCASTING,
};

typedef struct mpp_ntp_tsync{
    union{
        struct{
            uint64_t            time_ns;
            uint64_t            cycles_st;
            uint64_t            cpu_hz;
            uint64_t            unused;
        };
        uint64_t        data_64[4];
    };
} mpp_ntp_tsync;

typedef struct mc_ctrl_sw{
    union{
        struct {
            uint32_t    profile;
            uint32_t    ssn_max;
            uint32_t    rsv0;
            uint32_t    rsv : 16;
            uint32_t    flag_start_md:  2;
            uint32_t    ssn_stat:    1;
            uint32_t    flag_test_max_pktrate:1;
            uint32_t    flag_flow_scale:1;
            uint32_t    flag_3gpp_gsn_learn:1;
            uint32_t    flag_ntp_time:1;    //reg ts enable/disable
            uint32_t    flag_mod_dmac:1;
            uint32_t    flag_proc_ipfrag:1;
            uint32_t    flag_insp_3g_app:1;
            uint32_t    flag_pif_af:1;//reg interface af enable/disable
            uint32_t    flag_sp_qos_sw :1;
            uint32_t    flag_pkt_monitor_sw :1;
            uint32_t    flag_pkt_stats_sw :1;
            uint32_t    flag_arp_learn:1;
            uint32_t    rsv1 : 1;
            uint64_t    ipfrag_pkt;
            uint64_t    ipfrag_pkt_inter;
        };
        uint64_t        data_64[4];
    };
}mc_ctrl_sw;

typedef struct mpp_global_param{
    struct mc_ctrl_sw   param_ctrl_sw;
    struct mpp_ntp_tsync nts;
    uint16_t  TMP_MASK;
}mpp_global_param;

typedef struct mpp_shared_data{
    struct mpp_global_param *gparam;
    struct mpp_acl          *acl;
    #if  FRC_CONFIG_TWO_TUPLE
    struct frcore_acl       *two_tuple_acl;
    struct frcore_acl_hash_table_t *acl_hash_table;
    #endif
    //struct mpp_acsm         *sm;
    //struct mpp_crosslink    *crslk;
    //struct mpp_epif         *epif;
    //struct mpp_vlan_ifnum   *vlan_ifnum;

    //struct mpp_if_stat_tim  *if_stat_tim;
    //struct mpp_if_queue_tim *if_queue_tim;
    //struct mpp_qos_queue_tim *qos_queue_tim;
    //struct mpp_if_stat      *if_stat;
    //struct mpp_qos_id       *qos_id;
    //struct mpp_qos_id_wcq   *qos_id_wcq;
    //struct mpp_epif_workcq  *if_work_cq;
    //struct mpp_synfld_ctl   *synfld_ctl;
    //struct tcp_flow_rec     *tcp_flow_rec;

    //struct mpp_balance      *blc;
    //struct mpp_copy_to_server *cpser;
    //struct mpp_ip_tree *ip_macs;

    struct mpp_ssn          *ssn;
    //struct frc_ssn_submit_addr *ssn_addr;

    //struct mpp_port         *port;
    //struct mpp_pim_ipfrag   *ipfg; //ip fragment
    //struct mpp_port_monitor    *p_mon;

}CVMX_CACHE_LINE_ALIGNED mpp_shared_data;


/*
 * global shared data in bootmem
 */
CVMX_SHARED extern mpp_shared_data gsdata;
int frcore_ssn_main_prepare(void);
#endif


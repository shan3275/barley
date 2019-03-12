/*
 *
 * OCTEON SDK
 *
 * Copyright (c) 2010 Cavium Networks. All rights reserved.
 *
 * This file, which is part of the OCTEON SDK which also includes the
 * OCTEON SDK Package from Cavium Networks, contains proprietary and
 * confidential information of Cavium Networks and in some cases its
 * suppliers.
 *
 * Any licensed reproduction, distribution, modification, or other use of
 * this file or the confidential information or patented inventions
 * embodied in this file is subject to your license agreement with Cavium
 * Networks. Unless you and Cavium Networks have agreed otherwise in
 * writing, the applicable license terms "OCTEON SDK License Type 5" can be found
 * under the directory: $OCTEON_ROOT/components/driver/licenses/
 *
 * All other use and disclosure is prohibited.
 *
 * Contact Cavium Networks at info@caviumnetworks.com for more information.
 *
 */



#include "cvmcs-common.h"
#include "frcore.h"
#include "cvmx-helper.h"
#include "cvm-pci-loadstore.h"
#include "frcore_init.h"
#include "frcore_ssn.h"
#include "frcore_acl.h"

extern CVMX_SHARED uint32_t            cvm_first_buf_size_after_skip;
extern CVMX_SHARED uint32_t            cvm_subs_buf_size_after_skip;

extern CVMX_SHARED cvmx_sysinfo_t     *appinfo;
extern CVMX_SHARED octnic_dev_t       *octnic;


static int pciport = FIRST_PCI_PORT;



void
frcore_init_pkt_skip_sizes(void)
{
    cvm_first_buf_size_after_skip = (((cvmx_read_csr(CVMX_IPD_PACKET_MBUFF_SIZE) & 0xfff) - ((cvmx_read_csr(CVMX_IPD_1ST_MBUFF_SKIP) & 0x3f) + 1) ) * 8);
    cvm_subs_buf_size_after_skip = ((cvmx_read_csr(CVMX_IPD_PACKET_MBUFF_SIZE) & 0xfff) - ((cvmx_read_csr(CVMX_IPD_NOT_1ST_MBUFF_SKIP) & 0x3f) + 1)) * 8;

    printf("First MBUF size: %u\n Subsequent MBUF size: %u\n", cvm_first_buf_size_after_skip, cvm_subs_buf_size_after_skip);
}




static int
__get_pciport(int  gmxport)
{
    int  port_to_use = pciport;

    if(octnic->numpciqs > 1) {
        pciport++;
        if(pciport > LAST_PCI_PORT)
            pciport = FIRST_PCI_PORT;
    }

    return port_to_use;
}





/* NIC includes all ports from firstport to lastport (inclusive) */
static void
__add_port_to_nic(octnic_dev_t  *nic, int firstport, int lastport)
{
    int i;

    for(i = firstport; i <= lastport; i++) {
        nic->port[i].present = 1;
        nic->nports++;
    }
}


static int
port_is_in_nic(octnic_dev_t  *nic, int idx)
{
    return nic->port[idx].present;
}


static int
next_active_port(octnic_dev_t  *nic, int idx)
{
    idx = (idx < 0)?0:(idx+1);

    while(idx < MAX_OCTEON_ETH_PORTS) {
        if(frcore_port_active(idx)) {
            return idx;
            break;
        }
        idx++;
    }

    return -1;
}



void
__auto_find_ports(octnic_dev_t  *nic)
{
    int i, j;

    for(i = 0; i < cvmx_helper_get_number_of_interfaces(); i++) {
        cvmx_helper_interface_mode_t  mode;
        mode = cvmx_helper_interface_get_mode(i);
        if(!((mode == CVMX_HELPER_INTERFACE_MODE_SGMII) ||
            (mode == CVMX_HELPER_INTERFACE_MODE_XAUI)))
            continue;

        for (j = 0; j < cvmx_helper_ports_on_interface(i); j++) {
            int port;
            port = cvmx_helper_get_ipd_port(i,j);
            __add_port_to_nic(nic, port, port);
        }
    }
}



int
frcore_init_board_info()
{
    int    i, ifidx=0;
    char   boardstring[32];

    octnic = cvmx_bootmem_alloc(sizeof(octnic_dev_t), CVMX_CACHE_LINE_SIZE);
    if(octnic == NULL) {
        printf("%s Allocation failed for octnic\n", __FUNCTION__);
        return 1;
    }


    memset(octnic, 0, sizeof(octnic_dev_t));

    if(appinfo) {
        switch(appinfo->board_type) {

            case CVMX_BOARD_TYPE_EBT3000:
                strcpy(boardstring, "EBT3000");
                __add_port_to_nic(octnic, 16, 19);
                break;
            case CVMX_BOARD_TYPE_EBT5800:
                strcpy(boardstring, "EBT5800");
                __add_port_to_nic(octnic, 16, 19);
                break;
            case CVMX_BOARD_TYPE_THUNDER:
                strcpy(boardstring, "THUNDER");
                __add_port_to_nic(octnic, 16, 19);
                break;
            case CVMX_BOARD_TYPE_NICPRO2:
                strcpy(boardstring, "NICPRO2");
                __add_port_to_nic(octnic, 16, 19);
                break;
            case CVMX_BOARD_TYPE_NIC_XLE_4G:
                strcpy(boardstring, "XLE_4G");
                __add_port_to_nic(octnic, 16, 19);
                break;
            case CVMX_BOARD_TYPE_NIC_XLE_10G:
                strcpy(boardstring, "XLE_10G");
                __add_port_to_nic(octnic, 0, 0);
                __add_port_to_nic(octnic, 16, 16);
                break;
            default:
                __auto_find_ports(octnic);
                sprintf(boardstring, "Unknown (board_type: %d)", appinfo->board_type);
                break;
        }
    }

    printf("\n boardtype: %s \n", boardstring);

#ifdef USE_MULTIPLE_OQ
    octnic->numpciqs = 4; // Multiple Queues are required for NAPI support in host.
#else
    octnic->numpciqs = 1;
#endif

    for(i = 0; i < MAX_OCTEON_ETH_PORTS; i++) {
        if(port_is_in_nic(octnic, i)) {
            octnic->port[i].state         = CVM_NIC_IF_STATE_ACTIVE;
            octnic->port[i].rx_on         = 0;
            octnic->port[i].ifidx         = ifidx++;
            octnic->port[i].linfo.gmxport = i;
            octnic->port[i].linfo.pciport = __get_pciport(i);
            printf("Packets from port %d will be sent to PCI on port %d\n",
                   octnic->port[i].linfo.gmxport, octnic->port[i].linfo.pciport);
        } else {
            octnic->port[i].state = CVM_NIC_IF_STATE_INACTIVE;
        }
    }

    return 0;
}

void
frcore_init_mtu(void)
{
    int   i, lastport = -1, port = -1;

    port = next_active_port(octnic, port);

    if(port == -1) {
        printf("%s No active ports found\n", __FUNCTION__);
        return;
    }

    if (!OCTEON_IS_MODEL(OCTEON_CN3XXX) && !OCTEON_IS_MODEL(OCTEON_CN58XX)) {
        cvmx_pip_frm_len_chkx_t frm_len_chk;

        /* Set CN56XX to truncate packets larger than the MTU and
        smaller the 64 bytes */
        frm_len_chk.u64 = 0;
        frm_len_chk.s.minlen = 64;
        frm_len_chk.s.maxlen = OCTNET_MAX_FRM_SIZE;

        cvmx_write_csr(CVMX_PIP_FRM_LEN_CHKX(INTERFACE(port)), frm_len_chk.u64);

        lastport = port;
        port = next_active_port(octnic, lastport);
        while(port > 0) {
            if(INTERFACE(port) != INTERFACE(lastport)) {
                cvmx_write_csr(CVMX_PIP_FRM_LEN_CHKX(INTERFACE(port)),
                       frm_len_chk.u64);
            }
            lastport = port;
            port = next_active_port(octnic, lastport);
        }
    }

    for(i = 0 ; i < MAX_OCTEON_ETH_PORTS; i++) {
        if(frcore_port_active(i)) {
            cvmx_write_csr(CVMX_GMXX_RXX_JABBER(INDEX(i), INTERFACE(i)),
                       ((OCTNET_DEFAULT_FRM_SIZE + 7) & ~7));
        }
    }
}

int
frcore_setup_interfaces(void)
{
    int        i, port;
    uint8_t   *oct_mac_addr_base;

    /* Get the board type and determine the number of ports, the first usable
       port etc. */
    if(frcore_init_board_info()) {
        printf("%s Board Init failed\n", __FUNCTION__);
        return 1;
    }

    /* Initialize the MTU value for all ports to their defaults. */
    frcore_init_mtu();

    oct_mac_addr_base = cvmcs_app_get_macaddr_base();

    octnic->macaddrbase = 0;
    for(i = 0; i < 6; i++, octnic->macaddrbase <<= 8)
        octnic->macaddrbase |= oct_mac_addr_base[i];

    octnic->macaddrbase >>=8;

    for(port = 0, i = 0; port < MAX_OCTEON_ETH_PORTS; port++) {
        if(frcore_port_active(port)) {
            uint64_t  hw_addr;
            hw_addr = ((octnic->macaddrbase & 0x0000ffffffffffffULL) + (i++));
            octnic->port[port].linfo.hw_addr = hw_addr;
            frcore_change_mac_address((uint8_t *)&hw_addr, port);
            octnic->port[port].ifflags = OCTNET_IFFLAG_MULTICAST;
            frcore_change_multicast_list(port);
        }
    }

    /* Get the skip sizes for first and subsequent wqe pkt buffers. */
    frcore_init_pkt_skip_sizes();

    CVMX_SYNCW;

    return 0;
}

/**
 * @brief get pointer of a name bootmem block
 *
 * Gets a pointer to a named bootmem allocated block,
 * allocating it if necessary.  This function is called
 * by all cores, and they will all get the same address.
 *
 * @param size   size of block to allocate
 * @param name   name of block
 *
 * @return Pointer to shared memory (physical address)
 *         NULL on failure
 */
static void *get_shared_named_block(uint64_t size, char *name)
{
    void *ptr;

    if (cvmx_bootmem_find_named_block(name)){
        ptr = cvmx_phys_to_ptr(cvmx_bootmem_find_named_block(name)->base_addr);
    }else{
        ptr = cvmx_bootmem_alloc_named(size, CVMX_CACHE_LINE_SIZE, name);
        if(ptr) {
            if (cvmx_sysinfo_get()->board_type != CVMX_BOARD_TYPE_SIM)
            {
                //printf("#########################################\n");
                memset(ptr, 0, size);
            }
        }
    }
    return(ptr);
}

/**
 * @brief free the allocated bootmem
 *
 * @param name   name of block

static void free_shared_named_block(char *name)
{
    if(cvmx_bootmem_free_named(name)==0)
    {
        MC_PRINTF_ERR("failed to free the named block, %s.\n", name);
    }else{
        MC_PRINTF_INFO("successed to free the named block, %s.\n", name);
    }
}
 */

int frcore_ssn_init_bootmem(int init)
{
//    int i;
    mpp_shared_data *sdata = &gsdata;
    int total_size=0;

    /* global parameters */
    sdata->gparam = (mpp_global_param*)get_shared_named_block(
              sizeof(mpp_global_param),
              MC_BOOTMEM_SHARED_DATA_NAME);
    if (!sdata->gparam)
    {
        MC_PRINTF_ERR("Unable to allocate memory for %s\n",
                      MC_BOOTMEM_SHARED_DATA_NAME);
        return -1;
    }
    MC_PRINTF_INFO("Get shared block %s, size: 0x%lx, ptr: %p\n",
              MC_BOOTMEM_SHARED_DATA_NAME,
              sizeof(mpp_global_param),
              sdata->gparam);
    total_size += sizeof(mpp_global_param);

    if(sdata->gparam->param_ctrl_sw.flag_start_md == MPP_SW_INIT_DONE)
    {
        MC_PRINTF_INFO("Current bootmem blocks have been initialized, skipping...\n");
        return 0;
    }
    if(init)
        sdata->gparam->param_ctrl_sw.flag_start_md = MPP_INIT;

    /* acl && rule */
    sdata->acl = (mpp_acl *)get_shared_named_block(sizeof(mpp_acl), MC_BOOTMEM_ACL_RULE);
    if (!sdata->acl){
        MC_PRINTF_ERR("Unable to allocate memory for %s\n", MC_BOOTMEM_ACL_RULE);
        return -1;
    }
    printf("Get shared block %s, size: 0x%lx, ptr: %p\n",
              MC_BOOTMEM_ACL_RULE,
              sizeof(mpp_acl),
              sdata->acl);
    total_size += sizeof(mpp_acl);

    #if FRC_CONFIG_TWO_TUPLE
    /* two tuple acl && rule */
    sdata->two_tuple_acl = (frcore_acl *)get_shared_named_block(sizeof(frcore_acl),
                                         MC_BOOTMEM_TWO_TUPLE_ACL_RULE);
    if (!sdata->two_tuple_acl){
        MC_PRINTF_ERR("Unable to allocate memory for %s\n", MC_BOOTMEM_TWO_TUPLE_ACL_RULE);
        return -1;
    }
    printf("Get shared block %s, size: 0x%lx, ptr: %p\n",
              MC_BOOTMEM_TWO_TUPLE_ACL_RULE,
              sizeof(frcore_acl),
              sdata->two_tuple_acl);
    total_size += sizeof(frcore_acl);

    /* acl hash table */
    sdata->acl_hash_table = (struct frcore_acl_hash_table_t *) get_shared_named_block(
            FRCORE_ACL_HASH_TABLE_SIZE * sizeof(struct frcore_acl_hash_table_t),
            MC_BOOTMEM_TWO_TUPLE_ACL_HASH_TABLE);
    if (!sdata->acl_hash_table)
    {
        MC_PRINTF_ERR("Unable to allocate memory for %s, "
                      "total size %lu\n", MC_BOOTMEM_TWO_TUPLE_ACL_HASH_TABLE,
                      FRCORE_ACL_HASH_TABLE_SIZE * sizeof(struct frcore_acl_hash_table_t));
        MC_PRINTF_ERR("Unable to allocate memory for %s, "
                      "hash bucket size %lu, mpp ssn size %llu\n",
                      MC_BOOTMEM_TWO_TUPLE_ACL_HASH_TABLE, (unsigned long)FRCORE_ACL_HASH_TABLE_SIZE,
                       (ULL)sizeof(struct frcore_acl_hash_table_t));
        return -1;
    }
    printf("Get shared block %s, unit size: 0x%lx, "
              "total size: 0x%lx, ptr: %p\n",
              MC_BOOTMEM_TWO_TUPLE_ACL_HASH_TABLE,
              sizeof(struct frcore_acl_hash_table_t),
              FRCORE_ACL_HASH_TABLE_SIZE * sizeof(struct frcore_acl_hash_table_t),
              sdata->acl_hash_table);
    total_size += FRCORE_ACL_HASH_TABLE_SIZE*sizeof(struct frcore_acl_hash_table_t);
    #endif

    #if FRC_CONFIG_SSN
    /* ssn cell */
    sdata->ssn = (struct mpp_ssn *)get_shared_named_block(
            SSN_HASH_BUCKET_SIZE*sizeof(struct mpp_ssn),
            MC_BOOTMEM_SSN_NAME);
    if (!sdata->ssn)
    {
        MC_PRINTF_ERR("Unable to allocate memory for mpp_ssn, "
                      "total size %lu\n",
                      SSN_HASH_BUCKET_SIZE*sizeof(struct mpp_ssn));
        MC_PRINTF_ERR("Unable to allocate memory for mpp_ssn, "
                      "bit num %d, hash bucket size %lu, mpp ssn size %llu\n",
                      (int)SSN_HASH_BUCKET_SIZE_BITS,
                      (unsigned long)SSN_HASH_BUCKET_SIZE, (ULL)sizeof(struct mpp_ssn));
        return -1;
    }
    printf("Get shared block %s, unit size: 0x%lx, "
              "total size: 0x%lx, ptr: %p\n",
              MC_BOOTMEM_SSN_NAME,
              sizeof(struct mpp_ssn),
              SSN_HASH_BUCKET_SIZE*sizeof(struct mpp_ssn),
              sdata->ssn);
    total_size += SSN_HASH_BUCKET_SIZE*sizeof(struct mpp_ssn);

    /* ssn submit addr cell
    sdata->ssn_addr = (struct frc_ssn_submit_addr *)get_shared_named_block(
            SSN_HASH_BUCKET_SIZE*sizeof(struct frc_ssn_submit_addr),
            MC_BOOTMEM_SSN_ADDR_NAME);
    if (!sdata->ssn_addr)
    {
        MC_PRINTF_ERR("Unable to allocate memory for frc_ssn_submit_addr, "
                      "total size %lu\n",
                      SSN_HASH_BUCKET_SIZE*sizeof(struct frc_ssn_submit_addr));
        MC_PRINTF_ERR("Unable to allocate memory for frc_ssn_submit_addr, "
                      "bit num %d, hash bucket size %lu, mpp ssn size %lu\n",
                      SSN_HASH_BUCKET_SIZE_BITS,
                      SSN_HASH_BUCKET_SIZE, sizeof(struct frc_ssn_submit_addr));
        return -1;
    }
    printf("Get shared block %s, unit size: 0x%lx, "
              "total size: 0x%lx, ptr: %p\n",
              MC_BOOTMEM_SSN_ADDR_NAME,
              sizeof(struct frc_ssn_submit_addr),
              SSN_HASH_BUCKET_SIZE*sizeof(struct frc_ssn_submit_addr),
              sdata->ssn_addr);
    total_size += SSN_HASH_BUCKET_SIZE*sizeof(struct frc_ssn_submit_addr);*/

    /* tcp flow recovery
    sdata->tcp_flow_rec = (struct tcp_flow_rec *)get_shared_named_block(
                           sizeof(struct tcp_flow_rec),
                           MC_BOOTMEM_TCP_FLOW_REC);
    if (!sdata->tcp_flow_rec)
    {
        MC_PRINTF_ERR("Unable to allocate memory for tcp_flow_rec, "
                      "total size %lu\n", sizeof(struct tcp_flow_rec));
        return -1;
    }
    printf("Get shared block %s, unit size: 0x%lx, "
              "total size: 0x%lx, ptr: %p\n",
              MC_BOOTMEM_TCP_FLOW_REC,
              sizeof(struct tcp_flow_data),
              sizeof(struct tcp_flow_rec),
              sdata->tcp_flow_rec);
    total_size += sizeof(struct tcp_flow_rec); */

    #endif /* end of FRC_CONFIG_SSN */

    if(init){
        /* notify other cores to know ALL the bootmem blocks are writable */
        /*  suppose somebody else call start functions                    */
        sdata->gparam->param_ctrl_sw.flag_start_md = MPP_SW_INIT_DONE;
    }

    MC_PRINTF("Total size of  shared mem is: 0x%lx(%d) Bytes!\n", total_size, total_size);
    return 0;
}

/*
static int _populate_one_fpa_pool(uint64_t pool_num, uint64_t buffer_size,
                        int num_buffers, const char *pool_name)
{
    void *memory; // returned by cvmx_bootmem_alloc
    uint64_t current_num;
    int result;

    if (num_buffers == 0)
    {
        return 0;
    }

    current_num = cvmx_read_csr(CVMX_FPA_QUEX_AVAILABLE(pool_num));
    if (current_num)
    {
        cvmx_dprintf("ERROR:  pool number %lu (%s) already has %lu buffers."
                    " Skipping setup.\n", pool_num, pool_name, current_num);
        // note this is not considered an error
        return 0;
    }

    // allocated memory aligned on CVMX_CACHE_LINE_SIZE
    memory = cvmx_bootmem_alloc(buffer_size * num_buffers,
                                      CVMX_CACHE_LINE_SIZE);
    if (memory == NULL)
    {
        cvmx_dprintf("ERROR:   Out of memory initializing pool %lu(%s).\n",
                 pool_num, pool_name);
        return -1;
    }

    result = cvmx_fpa_setup_pool(pool_num, pool_name, memory, buffer_size,
                                num_buffers);
    return result;
}
*/

int frcore_ssn_init_app_fpa_global(void)
{
    int ret;

    #if FRC_CONFIG_SSN

    /*ret = _populate_one_fpa_pool(CVMX_FPA_SSN_POOL,
                                 CVMX_FPA_SSN_POOL_SIZE,
                                 MC_FPA_SSN_BUF_NUMS,
                                 "SSN Command Buffers");*/

    ret = cvmcs_app_mem_alloc("SSN Command Buffers",
                              CVMX_FPA_SSN_POOL,
                              CVMX_FPA_SSN_POOL_SIZE,
                              MC_FPA_SSN_BUF_NUMS);
    if(ret)
    {
        printf("!!!!!!!!!!!!!!!!!!!!!!!!!Error to populate FPA for SSN\n");
        return -1;
    }

    /*ret = _populate_one_fpa_pool(CVMX_FPA_TIMER_POOL,
                                 CVMX_FPA_TIMER_POOL_SIZE,
                                 MC_FPA_TIM_BUF_NUMS,
                                 "Octeon Hardware Timer Buffers");
    if(ret < 0)
    {
        printf("!!!!!!!!!!!!!!!!!!!!!!!!!Error to populate FPA for TIM MC_FPA_TIM_BUF_NUMS=%u\n",
               MC_FPA_TIM_BUF_NUMS);
        return -1;
    }*/

    /*ret = _populate_one_fpa_pool(CVMX_FPA_TCPFLOWREC_POOL,
                                 CVMX_FPA_TCPFLOWREC_POOL_SIZE,
                                 TCP_FLOW_REC_QE_MAX/2,
                                 "Tcp Flow Recovery Queue Entry Buffers");*/
    ret = cvmcs_app_mem_alloc("Tcp Flow Recovery Queue Entry Buffers",
                              CVMX_FPA_TCPFLOWREC_POOL,
                              CVMX_FPA_TCPFLOWREC_POOL_SIZE,
                              TCP_FLOW_REC_QE_MAX);
    if (ret)
    {
        printf("!!!!!!!!!!!!!!!!!!!!!!!!!Error to populate FPA for TCP FLOW RECOVERY QUEUE ENTRY == %u\n",
               TCP_FLOW_REC_QE_MAX);
        return -1;
    }
    else
    printf("===============SUCCESS to populate FPA for TCP FLOW RECOVERY QUEUE ENTRY == %u\n",
               TCP_FLOW_REC_QE_MAX);
    #endif

    #if FRC_CONFIG_TWO_TUPLE
    ret = cvmcs_app_mem_alloc("Two Tuple ACL Hash Cell Entry Buffers",
                              CVMX_FPA_ACL_HASH_CELL_POOL,
                              CVMX_FPA_ACL_HASH_CELL_POOL_SIZE,
                              FRCORE_ACL_HASH_CELL_BUF_NUMS);
    if (ret)
    {
        printf("!!!!!!!!!!!!!!!!!!Error to populate FPA for Two Tuple ACL Hash Cell Entry == %u\n",
               FRCORE_ACL_HASH_CELL_BUF_NUMS);
        return -1;
    }
    else
    printf("===============SUCCESS to populate FPA for Two Tuple ACL Hash Cell Entry == %u\n",
               FRCORE_ACL_HASH_CELL_BUF_NUMS);
    #endif

    MC_PRINTF_INFO("success to init fpa buffer.\n");

    return 0;
}


int mc_start_global()
{
  //  int i;

    printf("start global...\n");

    //mc_init_app_fpa_global();

    /* init bootmem name */
    if(frcore_ssn_init_bootmem(1)){
        MC_PRINTF_ERR("====================== boot mem init error=================\n");
        return -1;
    }

    if(frcore_ssn_init_app_fpa_global()) {
        return -1;
    }
    printf("\tglobal shared memory allocated\n");
    return 0;
}

static void mc_init_param()
{
    mpp_global_param *pdata = gsdata.gparam;

    pdata->param_ctrl_sw.ssn_stat = 1;

    pdata->param_ctrl_sw.flag_pif_af = 0;
    pdata->param_ctrl_sw.flag_proc_ipfrag = 1;
    pdata->param_ctrl_sw.flag_insp_3g_app = 1;
    pdata->param_ctrl_sw.flag_3gpp_gsn_learn = 0;
    pdata->param_ctrl_sw.ipfrag_pkt_inter = 1400;
    pdata->param_ctrl_sw.ipfrag_pkt = 0;
    pdata->param_ctrl_sw.flag_ntp_time = 1;
    pdata->nts.cycles_st = cvmx_get_cycle();
    pdata->nts.cpu_hz = cvmx_sysinfo_get()->cpu_clock_hz/10000000;
    return;
}

int mc_init_data(enum mpp_init_config profile)
{
    int i;
    gsdata.gparam->TMP_MASK = 0x3f;
    /* setting up default crosslink entries */
    struct mpp_ssn *ssn = gsdata.ssn;

    mc_init_param();
    gsdata.gparam->param_ctrl_sw.profile = profile;

    #if FRC_CONFIG_SSN
    gsdata.gparam->param_ctrl_sw.ssn_max = SSN_TOTAL_NUM;

    /* init spin_lock per ssn cell */
    for(i=0; i<SSN_HASH_BUCKET_SIZE; i++)
    {
        ssn[i].ssn_index = SSN_INDEX_INVALID;
        ssn[i].ssn_status = SSN_STATUS_EMPTY;
        ssn[i].grp = GROUP_TO_DATA_PLANE_AGING;
        ssn[i].qos = 1;
        ssn[i].next_sc = NULL;
        ssn[i].ssn_addr.addr_flag = 0;
        ssn[i].tcp_flow_positive_data.rcv_nxt = 0;
        ssn[i].tcp_flow_positive_data.tsegqe_len = 0;
        ssn[i].tcp_flow_positive_data.head.lh_first = NULL;
        ssn[i].tcp_flow_positive_data.rev_flow_data = NULL;
        ssn[i].tcp_flow_positive_data.enable = 1;
        ssn[i].tcp_flow_negative_data.rcv_nxt = 0;
        ssn[i].tcp_flow_negative_data.tsegqe_len = 0;
        ssn[i].tcp_flow_negative_data.head.lh_first = NULL;
        ssn[i].tcp_flow_negative_data.rev_flow_data = NULL;
        ssn[i].tcp_flow_negative_data.enable = 1;
    }

    /* init ssn addr cell
    struct frc_ssn_submit_addr *ssn_addr;
    ssn_addr = gsdata.ssn_addr;
    for(i = 0; i < SSN_HASH_BUCKET_SIZE; i++) {
        ssn_addr[i].addr_flag = 0;
    }*/

    cvmx_spinlock_init(&ssn_spinlock);

    /* tcp flow recovery
    gsdata.tcp_flow_rec->index = 0;
    struct tcp_flow_data *tcpflowdata;
    tcpflowdata = gsdata.tcp_flow_rec->tcp_flow_data;
    for (i = 0; i < TCP_FLOW_REC_MAX; i++) {
         tcpflowdata[i].rcv_nxt = 0;
         tcpflowdata[i].tsegqe_len = 0;
         tcpflowdata[i].head.lh_first = NULL;
         if (i < 10)
         {
             //MC_PRINTF_ERR("tcpflowdata[i].head.lh_first == %p\n", tcpflowdata[i].head.lh_first);
         }
         tcpflowdata[i].rev_flow_data = NULL;
         tcpflowdata[i].enable = 1;
    }*/

    #endif /* end of FRC_CONFIG_SSN */

    #if FRC_CONFIG_RULE
    /* setting up default acl entries*/
    acl_init();
    #endif /* end of FRC_CONFIG_RULE */

    #if FRC_CONFIG_TWO_TUPLE
    frcore_filter_init();
    #endif
    gsdata.gparam->param_ctrl_sw.flag_start_md = MPP_SW_START;

    printf("\tglobal started\n");

    return 0;
}


int frcore_ssn_main_prepare(void)
{
    printf("ssn starting global...\n");
    if(mc_start_global()) {
        return -1;
    }
    printf("ssn starting global OK\n");

    printf("initializing ssn data with profile %d...\n", MPP_ACCELARATED_LINUX);
    mc_init_data(MPP_ACCELARATED_LINUX);
    printf("ssn data initialized with profile %d\n", MPP_ACCELARATED_LINUX);

    return 0;
}

#include <string.h>

#include "frcore.h"
#include "frcore_ipc.h"
#include "frcore_proto.h"
#include "frcore_stat.h"
#include "frcore_vlan_check.h"

#if FRC_CONFIG_IPC

#define MAX_MAC_NUM 512

CVMX_SHARED ipc_dpkt_hd_t ipc_exp_data;
CVMX_SHARED ipc_dpkt_hd_t ipc_exp_mask;  
CVMX_SHARED ipc_cur_t ipc_cur;
CVMX_SHARED ipc_misc_t ipc_misc;
CVMX_SHARED ipc_instr_t ipc_instr;

CVMX_SHARED uint64_t *ovid_cnt;
CVMX_SHARED uint64_t *ivid_cnt;

int frcore_cmd_ipc_cur_set(uint16_t plen, void *param, uint16_t *olen, void *outbuf)
{
    if (sizeof(ipc_cur_t) != plen)
    {
        return FRE_FAIL;
    }

    ipc_cur_t *curp = (ipc_cur_t *) param;
    printf("curp->smac = %llx, pktid = %lld\n", curp->smac, curp->pktid);

    cvmx_atomic_set64(&ipc_cur.smac, curp->smac);
    cvmx_atomic_set64(&ipc_cur.pktid, curp->pktid);

    *olen = 0;

    return FRE_SUCCESS;
}

int frcore_cmd_ipc_cur_get(uint16_t plen, void *param, uint16_t *olen, void *outbuf)
{
    ipc_cur_t *curp = (ipc_cur_t*) outbuf;

    curp->smac  = cvmx_atomic_get64(&ipc_cur.smac);
    curp->pktid = cvmx_atomic_get64(&ipc_cur.pktid);
    printf("%s.%d: curp->smac=0x%llx, 0x%llx\n", __func__, __LINE__, curp->smac, ipc_cur.smac);
    printf("%s.%d: curp->pktid=0x%lld, 0x%lld\n", __func__, __LINE__, curp->pktid, ipc_cur.pktid);

    *olen = sizeof(ipc_cur_t);

    return FRE_SUCCESS;
}

int frcore_cmd_ipc_exp_set(uint16_t plen, void *param, uint16_t *olen, void *outbuf)
{
    if (sizeof(ipc_exp_t) != plen)
    {
        return FRE_FAIL;
    }

    ipc_exp_t *expp = (ipc_exp_t *)param; 
    printf("SET:ouid = %d, iifd = %d, ouim = %d, iifm = %d\n", expp->ouid, expp->iifd, expp->ouim, expp->iifm);
    printf("SET:pidd = 0x%llx, pidm = 0x%llx\n", expp->pidd, expp->pidm);

    ipc_exp_data.field.pid_h      = (uint16_t)(expp->pidd >> 32 & 0xffff);
    ipc_exp_data.field.pid_l      = expp->pidd >>  0 & 0xffffffff;
    ipc_exp_data.field.ori_vid    = (uint16_t)((expp->pidd >> 32 & 0xffff0000) >> 16);
    ipc_exp_data.field.oui        = expp->ouid;
    ipc_exp_data.field.chassis    = ((uint8_t)(expp->iifd >>  8 & 0xf0)) >> 4;
    ipc_exp_data.field.resv       = (uint8_t)(expp->iifd >>  8 & 0xf);
    //ipc_exp_data.field.slot       = expp->iifd >>  3 & 0x1f;
    ipc_exp_data.field.slot       = ((uint8_t)(expp->iifd & 0xf8)) >> 3;
    ipc_exp_data.field.card       = ((uint8_t)(expp->iifd & 0x4)) >> 2;
    ipc_exp_data.field.ifid       = (uint8_t)(expp->iifd & 0x3);
#if 0
    ipc_exp_mask.field.pid_h      = expp->pidm >> 32 & 0xffffffff;
    ipc_exp_mask.field.pid_l      = expp->pidm >>  0 & 0xffffffff;
    ipc_exp_mask.field.oui        = expp->ouim;
    ipc_exp_mask.field.chassis    = expp->iifm >>  8 & 0x7;
    ipc_exp_mask.field.slot       = expp->iifm >>  3 & 0x1f;
    ipc_exp_mask.field.card       = expp->iifm >>  2 & 0x1;
    ipc_exp_mask.field.ifid       = expp->iifm >>  0 & 0x3;
#endif

    ipc_exp_mask.field.pid_h      = (uint16_t)(expp->pidm >> 32 & 0xffff);
    ipc_exp_mask.field.pid_l      = expp->pidm >>  0 & 0xffffffff;
    ipc_exp_mask.field.ori_vid    = (uint16_t)((expp->pidm >> 32 & 0xffff0000) >> 16);
    ipc_exp_mask.field.oui        = expp->ouim;
    ipc_exp_mask.field.chassis    = ((uint8_t)(expp->iifm >>  8 & 0xf0)) >> 4;
    ipc_exp_mask.field.resv       = (uint8_t)(expp->iifm >>  8 & 0xf);
    //ipc_exp_data.field.slot       = expp->iifd >>  3 & 0x1f;
    ipc_exp_mask.field.slot       = ((uint8_t)(expp->iifm & 0xf8)) >> 3;
    ipc_exp_mask.field.card       = ((uint8_t)(expp->iifm & 0x4)) >> 2;
    ipc_exp_mask.field.ifid       = (uint8_t)(expp->iifm & 0x3);
    *olen = 0;

    printf("SETMASK:pid_h = 0x%x, pid_l = 0x%x, vid = 0x%x\n", ipc_exp_mask.field.pid_h, ipc_exp_mask.field.pid_l, ipc_exp_mask.field.ori_vid);
    printf("SETDATA:pid_h = 0x%x, pid_l = 0x%x, vid = 0x%x\n", ipc_exp_data.field.pid_h, ipc_exp_data.field.pid_l, ipc_exp_data.field.ori_vid);
    return FRE_SUCCESS;
}

int frcore_cmd_ipc_exp_get(uint16_t plen, void *param, uint16_t *olen, void *outbuf)
{

    ipc_exp_t *expp = (ipc_exp_t *)outbuf; 
    printf("GETMASK:chassis = %x, slot = %x, card = %x, ifid = %x\n", ipc_exp_mask.field.chassis, ipc_exp_mask.field.slot, ipc_exp_mask.field.card, ipc_exp_mask.field.ifid);
    printf("GETDATA:chassis = %x, slot = %x, card = %x, ifid = %x\n", ipc_exp_data.field.chassis, ipc_exp_data.field.slot, ipc_exp_data.field.card, ipc_exp_data.field.ifid);

    expp->pidd = ((uint64_t)((ipc_exp_data.field.ori_vid << 16) | ipc_exp_data.field.pid_h) << 32) | ipc_exp_data.field.pid_l;
    expp->ouid = ipc_exp_data.field.oui;
    expp->iifd = (ipc_exp_data.field.chassis << 12)  | (ipc_exp_data.field.resv << 8) | (ipc_exp_data.field.slot << 3) | 
                 ipc_exp_data.field.card << 2 | ipc_exp_data.field.ifid;

    expp->pidm = ((uint64_t)((ipc_exp_mask.field.ori_vid << 16) | ipc_exp_mask.field.pid_h) << 32) | ipc_exp_mask.field.pid_l;
    expp->ouim = ipc_exp_mask.field.oui;
    expp->iifm = (ipc_exp_mask.field.chassis << 12)  | (ipc_exp_mask.field.resv << 8) | (ipc_exp_mask.field.slot << 3) | 
                 (ipc_exp_mask.field.card << 2) | ipc_exp_mask.field.ifid;

    printf("GETMASK: pidm = 0x%llx\n", expp->pidm);
    *olen = sizeof(ipc_exp_t);
    printf("chassis = %x, slot = %x, card = %x, ifid = %x\n", ipc_exp_mask.field.chassis, ipc_exp_mask.field.slot, ipc_exp_mask.field.card, ipc_exp_mask.field.ifid);

    return FRE_SUCCESS;
}


int frcore_cmd_ipc_misc_set(uint16_t plen, void *param, uint16_t *olen, void *outbuf)
{
    if (sizeof(ipc_misc_t) != plen)
    {
        return FRE_FAIL;
    }

    ipc_misc_t *miscp = (ipc_misc_t *)param; 
    printf("SET:smac_check = %d,  pktid_check = %d\n", miscp->smac_check, miscp->pktid_check);
    printf("SET:exp_check = %d,  invalid_dump = %d, instr_send=%d\n", miscp->exp_check, miscp->invalid_dump, miscp->instr_send);

    /* !!! */
    ipc_misc.smac_check   = miscp->smac_check;
    ipc_misc.pktid_check  = miscp->pktid_check;
    ipc_misc.exp_check    = miscp->exp_check;
    ipc_misc.invalid_dump  = miscp->invalid_dump;
    ipc_misc.instr_send    = miscp->instr_send;

    *olen = 0;

    return FRE_SUCCESS;
}

int frcore_cmd_ipc_misc_get(uint16_t plen, void *param, uint16_t *olen, void *outbuf)
{
    ipc_misc_t *miscp = (ipc_misc_t *)outbuf; 

    miscp->smac_check  = ipc_misc.smac_check;
    miscp->pktid_check = ipc_misc.pktid_check;
    miscp->exp_check   = ipc_misc.exp_check;
    miscp->invalid_dump = ipc_misc.invalid_dump;
    miscp->instr_send = ipc_misc.instr_send;
    printf("smac_check = %d,  pktid_check = %d\n", ipc_misc.smac_check, ipc_misc.pktid_check);

    *olen = sizeof(ipc_misc_t);

    return FRE_SUCCESS;
}



int frcore_cmd_ipc_instr_payload_set(uint16_t plen, void *param, uint16_t *olen, void *outbuf)
{
    int j = 0;
    uint64_t i = 0;
    
    if (sizeof(ipc_payload_set_in_t) != plen)
    {
        return FRE_FAIL;
    }

    ipc_payload_set_in_t *payload_set = (ipc_payload_set_in_t *)param;
    swap_buff(INSTR_PAYLOAD_SZ >> 3, payload_set->data);
    for (j = 0; j < INSTR_PAYLOAD_SZ; j++)
    {
        //printf("data[%d] = 0x%x\n", j, payload_set->data[j]);
    }
    printf("index = %d\n", payload_set->index);
    i = payload_set->index;
    memcpy(&ipc_instr.payload[i * INSTR_PAYLOAD_SZ], payload_set->data, INSTR_PAYLOAD_SZ);
    
    *olen = 0;

    return FRE_SUCCESS;
}


int frcore_cmd_ipc_instr_set(uint16_t plen, void *param, uint16_t *olen, void *outbuf)
{
    if (sizeof(ipc_instr_cfg_t) != plen)
    {
        return FRE_FAIL;
    }

    ipc_instr_cfg_t *instrp = (ipc_instr_cfg_t *)param; 
    printf("sip_data = 0x%x, sip_mask=0x%x, dip_data = 0x%x, dip_mask=0x%x, ip_len = %d\n", 
           instrp->sip_data, instrp->sip_mask, instrp->dip_data, instrp->dip_mask,instrp->ip_len);

    if (instrp->ip_len > IPC_IP_LEN_MAX)
    {
        return FRE_PARAM;
    }

    ipc_instr.instr_cfg.dip_data = instrp->dip_data;
    ipc_instr.instr_cfg.dip_mask = instrp->dip_mask;
    ipc_instr.instr_cfg.sip_data = instrp->sip_data;
    ipc_instr.instr_cfg.sip_mask = instrp->sip_mask;
    ipc_instr.instr_cfg.type     = instrp->type;
    ipc_instr.instr_cfg.ip_len   = instrp->ip_len;
    ipc_instr.instr_cfg.tx_port   = instrp->tx_port;
    ipc_instr.instr_cfg.action   = instrp->action;

    //memcpy(ipc_instr.payload, instrp->payload, instrp->ip_len);

    *olen = 0;

    return FRE_SUCCESS;
}

int frcore_cmd_ipc_instr_get(uint16_t plen, void *param, uint16_t *olen, void *outbuf)
{
    ipc_instr_cfg_t *instrp = (ipc_instr_cfg_t *)outbuf; 

    instrp->dip_data = ipc_instr.instr_cfg.dip_data;
    instrp->dip_mask = ipc_instr.instr_cfg.dip_mask;
    instrp->sip_data = ipc_instr.instr_cfg.sip_data;
    instrp->sip_mask = ipc_instr.instr_cfg.sip_mask;
    instrp->type     = ipc_instr.instr_cfg.type    ;
    instrp->ip_len   = ipc_instr.instr_cfg.ip_len  ;
    instrp->tx_port   = ipc_instr.instr_cfg.tx_port;
    instrp->action   = ipc_instr.instr_cfg.action;

    //memcpy(instrp->payload, ipc_instr.payload, ipc_instr.ip_len);

    *olen = sizeof(ipc_instr_cfg_t);

    return FRE_SUCCESS;
}



void frcore_dump_buff(const char *func, int line_num, int len, uint8_t *buff)
{
    int i;
    char *p, line[DUMP_LINE_SIZE];

    p = line;
    printf("%s.%d: BUFF DUMP %d bytes:\n", func, line_num, len);
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
            printf("%s\n", line);
        }
    }
    if ((i % 16) != 0)
    {
        line[DUMP_LINE_SIZE - 1] = 0;
        printf("%s\n", line);
    }
    printf("\n");
}




void frcore_ipc_dpkt_dump(ipc_dpkt_hd_t *dpkt)
{
    if (NULL == dpkt)
    {
        FRCORE_ERROR("Instr is NULL!\n");
    }

    FRCORE_INFO("    WORD:  0x%.8x  0x%.8x  0x%.8x\n", dpkt->word[0], dpkt->word[1], dpkt->word[2]);

    FRCORE_INFO("    16%s :  0x%x", "oui(16)", dpkt->field.oui);
    FRCORE_INFO("    16%s :  0x%x", "pid_h(32)", dpkt->field.pid_h);
    FRCORE_INFO("    16%s :  0x%x", "pid_l(32)", dpkt->field.pid_l);
    FRCORE_INFO("    16%s :  0x%x", "chassis(3)", dpkt->field.chassis);
    FRCORE_INFO("    16%s :  0x%x", "resv(5)", dpkt->field.resv);
    FRCORE_INFO("    16%s :  0x%x", "slot(5)", dpkt->field.slot);
    FRCORE_INFO("    16%s :  0x%x", "card(1)", dpkt->field.card);
    FRCORE_INFO("    16%s :  0x%x", "ifid(2)", dpkt->field.ifid);
}


CVMX_SHARED ipc_instr_entry_t *ipc_instr_table = NULL;
int frcore_ipc_instr_table_init()
{
    int i;
    char *output_data = NULL;
    ipc_instr_hd_t *hd = NULL;
    ipc_instr_entry_t *entry = NULL;

    printf("%s:%d\n",__func__, __LINE__);

    ipc_instr_table = cvmx_bootmem_alloc(sizeof(ipc_instr_entry_t) * IPC_IIF_MAX, 16);

    printf("%s:%d\n",__func__, __LINE__);
    if (NULL == ipc_instr_table)
    {
        printf("%s inst table alloc fail\n", __FUNCTION__);
        return 1;
    }

    memset(ipc_instr_table, 0, sizeof(ipc_instr_entry_t) * IPC_IIF_MAX);

    for (i = 0; i < IPC_IIF_MAX; i++)
    {
        output_data = cvmx_bootmem_alloc(2048, 0);
        if (output_data == NULL)
        {
            printf("netflow output buffer alloc fail!.\n");
            return 1;
        }
      
        hd = (ipc_instr_hd_t *) output_data;

        hd->oui = IPC_INSTR_OUI;

        hd->protocol = IPC_INSTR_PROTOCOL;

        hd->iif = i;    
        
        entry = &ipc_instr_table[i];
        entry->outbuf = output_data;
        cvmx_spinlock_init(&entry->lock);

    }

    return FRE_SUCCESS;



    output_data = cvmx_bootmem_alloc(4096, 0);
    if (output_data == NULL)
    {
        printf("netflow output buffer alloc fail!.\n");
        return 1;
    }
}

#if 0
CVMX_SHARED ipc_instr_entry_t *ipc_instr_table = NULL;

int frcore_ipc_instr_table_init()
{
    int i;
    ipc_instr_hd_t *hd = NULL;
    cvmx_wqe_t *wqe = NULL;

    printf("%s:%d\n",__func__, __LINE__);
    cvmcs_app_mem_alloc("WQE OF IPC INSTR TABLE", IPC_INSTR_POOL, CVMX_FPA_OUTPUT_BUFFER_POOL_SIZE, IPC_IIF_MAX);

    printf("%s:%d\n",__func__, __LINE__);
    ipc_instr_table = cvmx_bootmem_alloc(sizeof(ipc_instr_entry_t) * IPC_IIF_MAX, 16);

    printf("%s:%d\n",__func__, __LINE__);
    if (NULL == ipc_instr_table)
    {
        printf("%s inst table alloc fail\n", __FUNCTION__);
        return 1;
    }

    memset(ipc_instr_table, 0, sizeof(ipc_instr_entry_t) * IPC_IIF_MAX);

    for (i = 0; i < IPC_IIF_MAX; i++)
    {
        printf("%s:%d\n",__func__, __LINE__);
        wqe = cvmx_fpa_alloc(IPC_INSTR_POOL);
        printf("%s:%d\n",__func__, __LINE__);
    	if(cvmx_unlikely(wqe == NULL)) 
        {
    		printf("%s wqe alloc failed\n", __FUNCTION__);
    		return 1;
    	}

        printf("%s:%d\n",__func__, __LINE__);
    	memset(wqe, 0, sizeof(cvmx_wqe_t));
      
        printf("%s:%d\n",__func__, __LINE__);
    	wqe->tag_type = CVMX_POW_TAG_TYPE_ORDERED;
        printf("%s:%d\n",__func__, __LINE__);
        hd = (ipc_instr_hd_t *) wqe->packet_ptr.ptr;
        printf("%s:%d\n",__func__, __LINE__);
        hd->oui = IPC_INSTR_OUI;
        printf("%s:%d\n",__func__, __LINE__);
        hd->protocol = IPC_INSTR_PROTOCOL;
        printf("%s:%d\n",__func__, __LINE__);
        hd->iif = i;    
        
        printf("%s:%d\n",__func__, __LINE__);
        wqe->len = sizeof(ipc_instr_hd_t);

        ipc_instr_entry_t *entry = NULL;
        entry = &ipc_instr_table[i];
        entry->wqe = wqe;

        wqe->packet_ptr.s.addr = buff;

        cvmx_spinlock_init(&entry->lock);
        printf("%s:%d\n",__func__, __LINE__);
    }

    return FRE_SUCCESS;
}

#endif

void frcore_send_instr_packet(uint64_t output_len, char *obuf)
{
    uint64_t        port = 16;
    cvmx_buf_ptr_t  packet_ptr;
    cvmx_pko_command_word0_t pko_command;

    printf("%s:%d\n",__func__, __LINE__);

    if (NULL == obuf)
    {
        return;
    }

    printf("%s:%d\n",__func__, __LINE__);
    int queue = cvmx_pko_get_base_queue(port);
    cvmx_pko_send_packet_prepare(port, queue, CVMX_PKO_LOCK_ATOMIC_TAG);

    printf("%s:%d\n",__func__, __LINE__);

    //pko_command.s.ipoffp1 = 17;
    pko_command.s.total_bytes = output_len;
    pko_command.s.dontfree =1;
    pko_command.s.segs = 1;

    packet_ptr.u64 = 0;
    packet_ptr.s.pool = CVMX_FPA_PACKET_POOL;
    packet_ptr.s.size = 0xffff;
    packet_ptr.s.addr = cvmx_ptr_to_phys(obuf);

    printf("%s:%d\n",__func__, __LINE__);
    /*
     * Send the packet and wait for the tag switch to complete before
     * accessing the output queue. This ensures the locking required
     * for the queue.
     *
     */
//#ifdef ENABLE_LOCKLESS_PKO
#if 0
    if (cvmx_pko_send_packet_finish(port, queue, pko_command, packet_ptr, CVMX_PKO_LOCK_NONE))
#else
    if (cvmx_pko_send_packet_finish(port, queue, pko_command, packet_ptr, CVMX_PKO_LOCK_ATOMIC_TAG))
#endif
    {
        printf("%s:%d\n",__func__, __LINE__);
        //FRCORE_STAT_INC(cnt_tx_errs);
    }
    else 
    {
        printf("%s:%d\n",__func__, __LINE__);
        //printf("%s.%d, packet_length=%lld\n", __func__, __LINE__, (ULL)pkt_len);
        //FRCORE_STAT_INC(cnt_tx_pkts);
        //FRCORE_STAT_ADD(cnt_tx_bytes, pkt_len);
    }

    printf("%s:%d\n",__func__, __LINE__);

}


#if 0
void frcore_ipc_ipv4_instr_send(ipc_instr_entry_t *entry)
{
    char *p = NULL;

    frcore_forward_packet_to_wire(entry->wqe, ipc_instr.instr_cfg.tx_port, 0);

    /* Clear all old cmds after wqe send*/
    p = entry->wqe->packet_ptr.ptr + sizeof(ipc_instr_hd_t);

    memset(p, 0, entry->wqe->len - sizeof(ipc_instr_hd_t));

    entry->wqe->len = sizeof(ipc_instr_hd_t);
}
#endif


int frcore_ipc_cmd_insert(uint16_t iif , ipc_cmd_t *cmd, uint8_t *payload)
{
    char *p = NULL;
    int cmd_len = 0;
    uint64_t output_len = 0;

    //printf("iif = %d\n", iif);
    ipc_instr_entry_t *entry = &ipc_instr_table[iif];

    //printf("%s:%d\n",__func__, __LINE__);
    cvmx_spinlock_lock(&entry->lock);

    if (0 == cmd->dtype)
    {
        cmd_len = 16; 
    }
    else
    {
        cmd_len = sizeof(ipc_cmd_t);
    }

    /* Send instr if no size to insert this cmd */
    //output_len = sizeof(ipc_instr_hd_t) + cmd_len + cmd->ip_len;
#if 0
    if (output_len > ipc_instr.instr_cfg.pkt_len)
    {
        printf("%s:%d\n",__func__, __LINE__);
        frcore_send_instr_packet(output_len, entry->outbuf);
    }
#endif
    //printf("%s:%d\n",__func__, __LINE__);
    p = entry->outbuf + sizeof(ipc_instr_hd_t);

    //printf("%s:%d, p = %p\n",__func__, __LINE__, p);
    memcpy(p, cmd, cmd_len);
    //printf("%s:%d\n",__func__, __LINE__);
    p = p + cmd_len;
    if (cmd->ip_len > 0)
    {
        printf("%s:%d\n",__func__, __LINE__);
        memcpy(p, payload, cmd->ip_len); 
        p = p + cmd->ip_len;
    }

    output_len = p - entry->outbuf;
    frcore_send_instr_packet(output_len, entry->outbuf);

    //printf("%s:%d\n",__func__, __LINE__);
    cvmx_spinlock_unlock(&entry->lock);

    return FRE_SUCCESS;
}

int frcore_ipc_ipv4_instr_sample(ipc_dpkt_hd_t *dpkt, uint32_t sip, uint32_t dip)
{

    ipc_cmd_t cmd;
    uint16_t iif = 0;
    if ( ((sip & ipc_instr.instr_cfg.sip_mask) & (ipc_instr.instr_cfg.sip_data & ipc_instr.instr_cfg.sip_mask)) ||
         ((dip & ipc_instr.instr_cfg.dip_mask) & (ipc_instr.instr_cfg.dip_data & ipc_instr.instr_cfg.dip_mask)) )
    {
        printf("%s:%d\n",__func__, __LINE__);
        if (ipc_instr.instr_cfg.type & ipc_instr.instr_cfg.action)
        {
            cmd.pktid = 0xffffffffffffffff;
        }
        else
        {
            cmd.pktid  = ((uint64_t)dpkt->field.pid_h) << 32 | dpkt->field.pid_l;
        }
        cmd.dtype   = ipc_instr.instr_cfg.type;
        cmd.ip_len = ipc_instr.instr_cfg.ip_len; 
        iif = (dpkt->field.chassis << 8)  | (dpkt->field.slot << 3) | 
              (dpkt->field.card << 2) | dpkt->field.ifid;

        frcore_ipc_cmd_insert(iif, &cmd, ipc_instr.payload); 
    }

    return FRCORE_ACT_FREE;
}


int frcore_ipc_ipv4_process(ipc_dpkt_hd_t *dpkt, uint8_t *p)
{
   ipc_ipv4_hd_t hd;

   memset(&hd, 0, sizeof(ipc_ipv4_hd_t));

   UNPACK_U32(p, hd.word[0]);
   UNPACK_U32(p, hd.word[1]);
   UNPACK_U32(p, hd.word[2]);
   UNPACK_U32(p, hd.word[3]);
   UNPACK_U32(p, hd.word[4]);

   /* ih = 20 or ih = 5 ???*/
   if (hd.field.ih != 5)
   {
       //printf("%s:%d, len = %d\n",__func__, __LINE__, hd.field.ih);
       FRCORE_STAT_INC(stat_ip_option);
       FRCORE_DROP(stat_drop_ip_option);
   }

   //printf("%s:%d, instr_send = %d\n",__func__, __LINE__, ipc_misc.instr_send);
   if (ipc_misc.instr_send)
   {
       return frcore_ipc_ipv4_instr_sample(dpkt, hd.field.saddr, hd.field.daddr);
   }
   else
   {
       return FRCORE_ACT_FREE;
   }
}

int frcore_ipc_ipv6_process(ipc_dpkt_hd_t *dpkt, uint8_t *p)
{
    FRCORE_STAT_INC(stat_ip_v6);
    FRCORE_DROP(stat_drop_ip_v6);
}

int frcore_ipc_work_process(cvmx_wqe_t *work)
{
    uint8_t *ip_ptr, *ether_ptr;
    int rv;
    uint64_t psmac = 0, pdmac = 0, csmac = 0;
    uint8_t *p;
    uint16_t protocol;

    kinds_of_pkt_len_stat(work);
//printf("%s:%d\n",__func__, __LINE__);
    if(!octnic->port[work->ipprt].rx_on) 
    {
        FRCORE_DROP(stat_drop_port_off);
    }
//printf("%s:%d\n",__func__, __LINE__);
    if (work->word2.s.rcv_error) 
    {
        FRCORE_STAT_INC(stat_rx_errs);
        FRCORE_DROP(stat_drop_rx_error);
    }
//printf("%s:%d\n",__func__, __LINE__);
    FRCORE_STAT_INC(stat_rx_pkts);
    FRCORE_STAT_ADD(stat_rx_bytes, (work->len + 4));

    if (work->word2.s.bufs == 0)
    {
        ip_ptr    = work->packet_data + work->word2.s.ip_offset + 6;
        ether_ptr = work->packet_data;
    } 
    else 
    {
        ether_ptr = cvmx_phys_to_ptr(work->packet_ptr.s.addr);
        ip_ptr    = cvmx_phys_to_ptr(work->packet_ptr.s.addr + work->word2.s.ip_offset);
    }
//printf("%s:%d\n",__func__, __LINE__);
    p = ether_ptr;

    if (ipc_misc.invalid_dump & 8)
    {
        printf("Start capture:     \n");
        FRCORE_INFO("    PKT DATA:\n");
        frcore_dump_buff(__func__, __LINE__, work->len, p);
    }

    if (ipc_misc.smac_check)
    {
        memcpy(&pdmac, ether_ptr + 0, 6); 
        memcpy(&psmac, ether_ptr + 6, 6);

        swap_buff(1, &pdmac);
        swap_buff(1, &psmac);

        if (0 == psmac)
        {
            FRCORE_STAT_INC(stat_rx_errs);
            FRCORE_DROP(stat_ipc_drop_smac_zero);
        }

        csmac = cvmx_atomic_get64(&ipc_cur.smac);

        if (0 == csmac) 
        {
            cvmx_atomic_set64(&ipc_cur.smac, psmac);
            FRCORE_STAT_INC(stat_ipc_smac_ordered);
        }
        else if (psmac == (csmac + 1))
        {
            cvmx_atomic_set64(&ipc_cur.smac, psmac);
            FRCORE_STAT_INC(stat_ipc_smac_ordered);
        }
        else
        {
            FRCORE_STAT_INC(stat_ipc_smac_disorder);
        }
    }

    
    ipc_dpkt_hd_t dpkt;
    uint8_t vare = 0, iifv = 0;
    memset(&dpkt, 0, sizeof(ipc_dpkt_hd_t));

    UNPACK_U16(p, dpkt.field.oui);
    UNPACK_U32(p, dpkt.field.pid_l);
    UNPACK_U8(p, vare);
    UNPACK_U16(p, dpkt.field.ori_vid);
    UNPACK_U16(p, dpkt.field.pid_h);
    UNPACK_U8(p, iifv);

    dpkt.field.chassis = (vare & 0xf0) >> 4;
    dpkt.field.slot = (iifv & 0xf8) >> 3;
    dpkt.field.card = (iifv & 0x4) >> 2;
    dpkt.field.ifid = iifv & 0x3;

    //printf("oui = 0x%x, pid_l = 0x%x, vare = 0x%x, iifv = 0x%x\n", dpkt.field.oui, dpkt.field.pid_l, vare, iifv);
    //printf("vid = 0x%x, pid_h = 0x%x\n", dpkt.field.ori_vid, dpkt.field.pid_h);
    //printf("chs = 0x%x, slot = 0x%x, card = 0x%x, if=0x%x\n", dpkt.field.chassis, dpkt.field.slot, dpkt.field.card, dpkt.field.ifid);
    //UNPACK_U32(p, dpkt.word[0]);
    //UNPACK_U32(p, dpkt.word[1]);
    //UNPACK_U32(p, dpkt.word[2]);

    if (ipc_misc.exp_check)
    {
        int dpkt_valid = 0;
        if ((ipc_exp_data.field.oui & ipc_exp_mask.field.oui) != 
            (dpkt.field.oui & ipc_exp_mask.field.oui) )
        {
            FRCORE_STAT_INC(stat_ipc_head_invalid);
        } 
        else if ((ipc_exp_data.field.chassis & ipc_exp_mask.field.chassis)  != 
                 (dpkt.field.chassis & ipc_exp_mask.field.chassis) )
        {
            FRCORE_STAT_INC(stat_ipc_head_invalid);
        } 
        else if ((ipc_exp_data.field.pid_h & ipc_exp_mask.field.pid_h)  != 
                 (dpkt.field.pid_h & ipc_exp_mask.field.pid_h) )
        {
            FRCORE_STAT_INC(stat_ipc_head_invalid);
        } 
        else if ((ipc_exp_data.field.pid_l & ipc_exp_mask.field.pid_l)  != 
                 (dpkt.field.pid_l & ipc_exp_mask.field.pid_l) )
        {
            FRCORE_STAT_INC(stat_ipc_head_invalid);
        }
        else if ((ipc_exp_data.field.slot & ipc_exp_mask.field.slot)  != 
                 (dpkt.field.slot & ipc_exp_mask.field.slot) )
        {
            FRCORE_STAT_INC(stat_ipc_head_invalid);
        }
        else if ((ipc_exp_data.field.card & ipc_exp_mask.field.card)  != 
                 (dpkt.field.card & ipc_exp_mask.field.card) )
        {
            FRCORE_STAT_INC(stat_ipc_head_invalid);
        }
        else if ((ipc_exp_data.field.ifid & ipc_exp_mask.field.ifid)  != 
                 (dpkt.field.ifid  & ipc_exp_mask.field.ifid) )
        {
            FRCORE_STAT_INC(stat_ipc_head_invalid);
        }
        else if ((ipc_exp_data.field.ori_vid & ipc_exp_mask.field.ori_vid)  != 
                 (dpkt.field.ori_vid  & ipc_exp_mask.field.ori_vid))
        {
            FRCORE_STAT_INC(stat_ipc_head_invalid);
        }
        else
        {
            FRCORE_STAT_INC(stat_ipc_head_valid);
            dpkt_valid = 1;
        }

        if (0 == dpkt_valid)
        {
            if (ipc_misc.invalid_dump & 1)
            {
                FRCORE_INFO("    PKT DPKT:\n");
                frcore_ipc_dpkt_dump(&dpkt); 
            }

            if (ipc_misc.invalid_dump & 2)
            {
                FRCORE_INFO("    EXP DATA:\n");
                frcore_ipc_dpkt_dump(&ipc_exp_data);

                FRCORE_INFO("    EXP MASK:\n");
                frcore_ipc_dpkt_dump(&ipc_exp_mask);
            }

            if (ipc_misc.invalid_dump & 4)
            {
                FRCORE_INFO("    PKT DATA:\n");
                frcore_dump_ptrs(&work->packet_ptr, work->len);
            }
            
        }
    }
    

    if (ipc_misc.pktid_check)
    {
        uint64_t ppid = 0, cpid = 0;
        ppid = (((uint64_t) dpkt.field.pid_h << 32) |   dpkt.field.pid_l);

        cpid = cvmx_atomic_get64(&ipc_cur.pktid);

        //printf("pktid = %lld, cur_pktid = %lld\n", ppid, cpid);
        if (0 == cpid)
        {
            if (ppid == cpid)
            {
                FRCORE_STAT_INC(stat_ipc_pktid_repeat);
            }
            else
            {
                FRCORE_STAT_INC(stat_ipc_pktid_ordered);
                cvmx_atomic_set64(&ipc_cur.pktid, ppid);
            }          
        }
        else if (ppid == cpid)
        {
            FRCORE_STAT_INC(stat_ipc_pktid_repeat);
        }
        else if (ppid > cpid)
        {
            cvmx_atomic_set64(&ipc_cur.pktid, ppid);
            FRCORE_STAT_INC(stat_ipc_pktid_ordered);
        }
        else
        {
            FRCORE_STAT_INC(stat_ipc_pktid_disorder);
        }
    }

    uint16_t ovid, ivid;
    int vlan_dist = 0;

    p = ether_ptr + 12;
    UNPACK_U16(p, protocol);
    //printf("%s.%d:protocol = 0x%x\n", __func__, __LINE__, protocol);

    printf("%s.%d: pro = 0x%x\n", protocol);
    if (PROTO_VLAN == protocol)
    {
        vlan_dist = 1;
        FRCORE_STAT_INC(stat_vlan);
        UNPACK_U16(p, ovid);
        UNPACK_U16(p, protocol);

        if (PROTO_VLAN == protocol)
        {
            UNPACK_U16(p, ivid);
            UNPACK_U16(p, protocol);
            //FRCORE_PORT_VLAN_STAT_INC(work->ipprt, xe0_stat_inner_vlan);
        }
        //FRCORE_PORT_VLAN_STAT_INC(work->ipprt, xe1_stat_vlan_id_total);
    }

    printf("vlan_dist = %d, protocol = %d\n", vlan_dist, protocol);
    switch (protocol)
    {
    case PROTO_IPV4:
        if (vlan_dist)
        {
            rv = frcore_vlan_check_v4(work, ether_ptr, ip_ptr, psmac, pdmac);
        }
        rv = frcore_ipc_ipv4_process(&dpkt, p);
        break;
    case PROTO_IPV6:
        if (vlan_dist)
        {
            rv = frcore_vlan_check_v6(work, ether_ptr, ip_ptr, psmac, pdmac);
        }
        rv = frcore_ipc_ipv6_process(&dpkt, p);
        break;
    default:
        rv = FRCORE_ACT_FREE;
        break;
    }

    FRCORE_PKT("smac 0x%llx, dmac 0x%llx\n", (ULL)smac, (ULL)dmac); 

    return rv;
}


int frcore_ipc_init()
{
    printf("%s:%d\n",__func__, __LINE__);
    frcore_ipc_instr_table_init(); 
    FRCORE_REGISTER_CMD(CMD_TYPE_USER, USER_CMD_IPC_CUR_SET, frcore_cmd_ipc_cur_set);
    FRCORE_REGISTER_CMD(CMD_TYPE_USER, USER_CMD_IPC_CUR_GET, frcore_cmd_ipc_cur_get);

    FRCORE_REGISTER_CMD(CMD_TYPE_USER, USER_CMD_IPC_MISC_SET, frcore_cmd_ipc_misc_set);
    FRCORE_REGISTER_CMD(CMD_TYPE_USER, USER_CMD_IPC_MISC_GET, frcore_cmd_ipc_misc_get);

    FRCORE_REGISTER_CMD(CMD_TYPE_USER, USER_CMD_IPC_EXP_SET, frcore_cmd_ipc_exp_set);
    FRCORE_REGISTER_CMD(CMD_TYPE_USER, USER_CMD_IPC_EXP_GET, frcore_cmd_ipc_exp_get);

    FRCORE_REGISTER_CMD(CMD_TYPE_USER, USER_CMD_IPC_INSTR_SET, frcore_cmd_ipc_instr_set);
    FRCORE_REGISTER_CMD(CMD_TYPE_USER, USER_CMD_IPC_INSTR_PAYLOAD_SET, frcore_cmd_ipc_instr_payload_set);
    FRCORE_REGISTER_CMD(CMD_TYPE_USER, USER_CMD_IPC_INSTR_GET, frcore_cmd_ipc_instr_get);

    return FRE_SUCCESS;
}

#endif

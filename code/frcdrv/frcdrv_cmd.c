

#include "cavium_sysdep.h"
#include "cavium_defs.h"
#include "cavium_kernel_defs.h"
#include "cavium_release.h"
#include "octeon-opcodes.h"

#include "frcdrv.h"
#include "frcdrv_cmd.h"
#include "frcdrv_network.h"
#include "frc_pack.h"

extern struct  frcdev_props_t  *frcprops[MAX_OCTEON_DEVICES];





void
frcdrv_cmd_callback(octeon_req_status_t status, void *sif_ptr)
{
	
	octeon_soft_instruction_t  *si = (octeon_soft_instruction_t  *)sif_ptr;
	frcdrv_cmd_pkt_t          *pkt;

	pkt = (frcdrv_cmd_pkt_t *)((uint8_t *)si + OCT_SOFT_INSTR_SIZE);

	FRCDRV_DEBUG("status = %d, si = %p, pkt=%p.\n", status, si, pkt);
#if 0
	/* Call the callback function if status is OK.
	   Status is OK only if a response was expected and core returned success.
	   If no response was expected, status is OK if the command was posted
	   successfully. */
	if(!status && pkt->cb_fn)
		pkt->cb_fn(pkt);
#endif
	cavium_free_dma(si);
}




static inline octeon_soft_instruction_t  *
frcdrv_alloc_cmd_si(octeon_device_t  *oct, frcdrv_cmd_pkt_t  *cpkt, void *param)
{
	octeon_soft_instruction_t  *si = NULL;
	uint8_t                    *data, *p;
	uint32_t                    datasize = 0;
    
    FRCDRV_DEBUG("cpkt->wait_time %lu\n", cpkt->wait_time);

	datasize = sizeof(frcdrv_cmd_pkt_t) + (cpkt->wait_time?16:0);

	/* Additional 8 bytes to align rptr to a 8 byte boundary. */
	datasize += sizeof(frcore_cmd_t) + (cpkt->cmd.len) + 8;

	si = cavium_malloc_dma( (OCT_SOFT_INSTR_SIZE + datasize),
	                        __CAVIUM_MEM_ATOMIC);
	if(si == NULL){
        FRCDRV_ERROR("malloc is fail!\n");
		return NULL;
    }

	cavium_memset(si, 0, (OCT_SOFT_INSTR_SIZE + datasize) );

	cavium_memcpy(((uint8_t *)si + OCT_SOFT_INSTR_SIZE), cpkt,
	              sizeof(frcdrv_cmd_pkt_t));

	si->ih.fsz     = 16;
	si->ih.tagtype = ORDERED_TAG;
	si->ih.tag     = 0x11111111;
    si->ih.grp     = FRC_CMD_GRP;
	si->ih.raw     = 1;
	si->ih.rs      = 1;
	si->irh.opcode = FRC_CMD_REQUEST_OP;
	SET_SOFT_INSTR_DMA_MODE(si, OCTEON_DMA_DIRECT);
	SET_SOFT_INSTR_OCTEONID(si, oct->octeon_id);
	SET_SOFT_INSTR_IQ_NO(si, 0);
	SET_SOFT_INSTR_CALLBACK(si, frcdrv_cmd_callback);
	SET_SOFT_INSTR_CALLBACK_ARG(si, (void *)si);

	data            = (uint8_t *)si + OCT_SOFT_INSTR_SIZE +
	                              sizeof(frcdrv_cmd_pkt_t);
	

	cavium_memcpy(data, &cpkt->cmd, sizeof(frcore_cmd_t));
	octeon_swap_8B_data((uint64_t *)data, (sizeof(frcore_cmd_t) >> 3));

    if (cpkt->cmd.len && param)
    {   
        p = data + sizeof(frcore_cmd_t);
        cavium_memcpy(p, param, cpkt->cmd.len);
        octeon_swap_8B_data((uint64_t *)p, (cpkt->cmd.len >> 3));
    }

    si->dptr        = data;
	si->ih.dlengsz  = sizeof(frcore_cmd_t) + cpkt->cmd.len;

    //frc_dump_buff(si->ih.dlengsz, si->dptr);

	if(cpkt->wait_time) {
		si->rptr           = ((uint8_t *)si->dptr + si->ih.dlengsz);
		if((unsigned long)si->rptr & 0x7) {
			si->rptr = (void *)(((unsigned long)si->rptr + 8) & ~0x7);
		}
		si->irh.rlenssz    = 16;
		si->status_word    = (uint64_t *)((uint8_t *)si->rptr + 8);
		*(si->status_word) =  COMPLETION_WORD_INIT;

		SET_SOFT_INSTR_RESP_ORDER(si, OCTEON_RESP_ORDERED);
		SET_SOFT_INSTR_RESP_MODE(si, OCTEON_RESP_NON_BLOCKING);
		SET_SOFT_INSTR_TIMEOUT(si, cpkt->wait_time);
	} else {
		SET_SOFT_INSTR_RESP_ORDER(si, OCTEON_RESP_NORESPONSE);
	}

	SET_SOFT_INSTR_ALLOCFLAGS(si, OCTEON_SOFT_INSTR_DB_NOW);

	cavium_print(PRINT_FLOW,
	             "%s si @ %p datasize: %d dptr: %p rptr: %p\n",
	             __CVM_FUNCTION__, si, datasize, si->dptr, si->rptr);

    //FRCDRV_DEBUG("cmdsize %d, datasize %d, dptr: %p rptr: %p\n", cmdsize, datasize, si->dptr, si->rptr);
    //FRCDRV_DEBUG("si = %p, si->dptr = %p, si->ih.dlengsz  =%d.\n", si, si->dptr, si->ih.dlengsz);

	return si;
}


int
frcdrv_send_cmd_pkt(octeon_device_t *oct, frcdrv_cmd_pkt_t  *cpkt, void *param)
{
	octeon_instr_status_t      retval;
	octeon_soft_instruction_t  *si = NULL;

    if (cpkt->wait_time == 0)
    {
        cpkt->wait_time     = 6000;
    }

    FRCDRV_DEBUG("cpkt->cmd.type = 0x%.2x cpkt->cmd.cmd = 0x%.2x, cpkt->cmd.len = 0x%.4x\n", 
                 cpkt->cmd.type, cpkt->cmd.cmd, cpkt->cmd.len);
#ifdef FRC_DEBUG_DRV
    if (cpkt->cmd.len && param)
    {
        //frc_dump_buff(cpkt->cmd.len, param);
    }
#else
#   error "Undefined FRC_DEBUG_DRV"
#endif
	si = frcdrv_alloc_cmd_si(oct, cpkt, param);
	if(si == NULL) {
		FRCDRV_ERROR("soft instr alloc failed\n");
		return FRE_FAIL;
	}

	retval = octeon_process_instruction(oct, si, NULL);
	if(retval.u64 == OCTEON_REQUEST_FAILED) {
		FRCDRV_ERROR("soft instr send failed\n");
		return FRE_FAIL;
	}

	return FRE_SUCCESS;
}



unsigned long frcdrv_cmd_respond_bytes = 0;
unsigned long frcdrv_cmd_respond_pkts = 0;
unsigned long frcdrv_cmd_respond_min  = 128;
unsigned long frcdrv_cmd_respond_max  = 0;

unsigned long  frcdrv_cmd_respond_print_jiffies=0;

int  frcdrv_core_respond_dispatch(octeon_recv_info_t *recv_info, void *arg)
{
   int   i;
   octeon_recv_pkt_t  *recv_pkt = recv_info->recv_pkt;
   uint8_t *p, *data;
   uint32_t size, sum = 0;
   frc_cmd_respond_t respond;

   FRCDRV_DEBUG("recv_info = %p, arg = %p\n", recv_info, arg);

   memset(&respond, 0, sizeof(respond));

   frcdrv_cmd_respond_bytes += recv_pkt->length;

   if(recv_pkt->length < frcdrv_cmd_respond_min)
	frcdrv_cmd_respond_min = recv_pkt->length;

   if(recv_pkt->length > frcdrv_cmd_respond_max)
	frcdrv_cmd_respond_max = recv_pkt->length;
#ifdef FRCDRV_DUMP_RESPOND_STAT
   FRCDRV_DEBUG("--- droq_dispatch pkt #%lu---\n", frcdrv_cmd_respond_pkts);
   FRCDRV_DEBUG("opcode: %x \n", recv_pkt->resp_hdr.opcode);
   FRCDRV_DEBUG("request_id: %x \n", recv_pkt->resp_hdr.request_id);
   FRCDRV_DEBUG("octeon_id: %d \n", recv_pkt->octeon_id);
   FRCDRV_DEBUG("length: %d \n", recv_pkt->length);
   FRCDRV_DEBUG("buf_count: %d\n", recv_pkt->buffer_count);


   /* Limit the prints to 1 per second */
   if(jiffies <= (frcdrv_cmd_respond_print_jiffies + HZ)) {
       frcdrv_cmd_respond_print_jiffies = jiffies;
       printk("frcdrv_cmd_respond_bytes = %d\n", frcdrv_cmd_respond_bytes);
       printk("frcdrv_cmd_respond_pkts  = %d\n", frcdrv_cmd_respond_pkts);
       printk("frcdrv_cmd_respond_min   = %d\n", frcdrv_cmd_respond_min);
       printk("frcdrv_cmd_respond_max   = %d\n", frcdrv_cmd_respond_max);
   }
#endif      

   


#ifdef FRCDRV_DUMP_RESPOND_DATA

  
    /* Enable this section if you want to print the buffer 
     contents. Note that for big packets and for tests with a high
     packet frequency, enabling this section will cause the kernel
     to get busy printing all the time. 
    */
    for(i = 0 ; i < recv_pkt->buffer_count; i++) {
      data = (uint8_t *)(((struct sk_buff*)recv_pkt->buffer_ptr[i])->data);
      size = recv_pkt->buffer_size[i];
      
      printk("Data buffer #%d\n", i);
      cavium_error_print(data, size);
    }

#endif

   p = (uint8_t *) &respond;
    sum = 0;
   for(i = 0 ; i < recv_pkt->buffer_count; i++) {
        data = (uint8_t *)(((struct sk_buff*)recv_pkt->buffer_ptr[i])->data);
        size = recv_pkt->buffer_size[i];
        //FRCDRV_DEBUG("size = %u, sizeof(respond) = %lu\n", size, sizeof(respond));
        if ((sum + size) <= sizeof(respond))
        {
            cavium_memcpy(p, data, size);
            p   += size;
            sum += size;
        } else {
            break;
        }
   }
#if 0
   octeon_swap_8B_data((uint64_t *) &respond, sizeof(respond) >> 3);
#else
   octeon_swap_8B_data((uint64_t *) &respond, 1);
#endif
   //FRCDRV_DEBUG("!!! RESPOND GOT: RET 0x%x, LEN 0x%x, SEQ 0x%x.\n", respond.ret, respond.len, respond.seq);
   if (frcdrv_core_req_respond(&respond)) {
       FRCDRV_DEBUG("RESPOND after timeout: seq 0x%.8x.\n", respond.seq);
   }

   for(i = 0 ; i < recv_pkt->buffer_count; i++) {
        free_recv_buffer(recv_pkt->buffer_ptr[i]);
   }
   cavium_free_dma(recv_info);
   
   frcdrv_cmd_respond_pkts++;
   return 0;
}

static frc_list_t frcdrv_core_req_head;
static spinlock_t frcdrv_core_req_head_lock = SPIN_LOCK_UNLOCKED;
static uint32_t frcdrv_core_requset_seq = 0;

void frcdrv_core_req_add(frcdrv_core_req_t *core_req)
{
    spin_lock(&frcdrv_core_req_head_lock);
    frc_list_add_tail(&core_req->node, &frcdrv_core_req_head);
    frcdrv_core_requset_seq++;
    core_req->seq = frcdrv_core_requset_seq;
    core_req->state = CORE_REQ_WAITING;
    FRCDRV_DEBUG("respond.len = %d\n", core_req->respond.len);
    spin_unlock(&frcdrv_core_req_head_lock);
}

void frcdrv_core_req_remove(frcdrv_core_req_t *core_req)
{
    spin_lock(&frcdrv_core_req_head_lock);
    frc_list_del(&core_req->node);
    spin_unlock(&frcdrv_core_req_head_lock);
}

int frcdrv_core_req_respond(frc_cmd_respond_t *respond)
{
    frc_list_t *node;
    frcdrv_core_req_t *core_req;
    int timeout = 1;

    if (respond == NULL) {
         FRCDRV_ERROR("respond is NULL\n");
         return 1;
    }

    spin_lock(&frcdrv_core_req_head_lock);
    frc_list_for_each(node, &frcdrv_core_req_head) {
        core_req = (frcdrv_core_req_t *) node;

        if (core_req->seq == respond->seq)
        {
            memcpy(&core_req->respond, respond, sizeof(frc_cmd_respond_t));
            core_req->state = CORE_REQ_RESPOND;
            timeout = 0;
        }
    
    }

    spin_unlock(&frcdrv_core_req_head_lock);

    return timeout;
}

void
frcdrv_core_req_completion(void  *pkt_ptr)
{
//	frcdrv_cmd_pkt_t   *pkt = (frcdrv_cmd_pkt_t  *)pkt_ptr;

    //FRCDRV_DEBUG("CORE COMMAND %d %d dma completion.\n", pkt->cmd.type, pkt->cmd.cmd);
}
 
int frcdrv_core_cmd(octeon_device_t *oct, uint16_t type, uint16_t cmd, uint16_t ilen, void *input, uint16_t *olen, void *output)
{
    int rv, i;
    frcdrv_cmd_pkt_t *cpkt = NULL;
    frcdrv_core_req_t *core_req = NULL;

    int out_len = 0;
    if (ilen > FRC_CORE_CMD_ARG_MAX)
    {
        return FRE_PARAM;
    }

    FRCDRV_DEBUG("oct %p, type %d, cmd %d, ilen %d, input %p, olen %p, output %p\n",
                 oct, type, cmd, ilen, input, olen, output);

    if (olen != NULL)
    {
        out_len = *olen;
    }

    if (ilen > FRCORE_CMD_INPUT_MAX)
    {
        FRCDRV_ERROR("ilen(%d) > FRCORE_CMD_INPUT_MAX(%d).\n", ilen, FRCORE_CMD_INPUT_MAX);
        return FRE_PARAM;
    }
    
    core_req = frcdrv_malloc(sizeof(frcdrv_core_req_t));
    
    if (core_req == NULL)
    {
        FRCDRV_ERROR("Can't malloc core_req.\n");
        rv = FRE_MEMORY;
        goto frcdrv_core_cmd_error;
    }
    memset(core_req, 0, sizeof(frcdrv_core_req_t));

    cpkt = frcdrv_malloc(sizeof(frcdrv_cmd_pkt_t));
    if (cpkt == NULL)
    {
        FRCDRV_ERROR("malloc frcdrv_cmd_pkt_t fail!\n");
        rv = FRE_MEMORY;
        goto frcdrv_core_cmd_error;
    }
    memset(cpkt, 0, sizeof(frcdrv_cmd_pkt_t));

    frcdrv_core_req_add(core_req);
    FRCDRV_DEBUG("respond.len = %d\n", core_req->respond.len);
    cpkt->cmd.type     = type;
    cpkt->wait_time     = 6000;
    cpkt->cb_fn         = NULL;

    cpkt->cmd.seq  = core_req->seq;
    cpkt->cmd.cmd  = cmd;

    cpkt->cmd.len = ilen;

    FRCDRV_DEBUG("ilen = %d, cpkt->cmd.len = %d\n", ilen, cpkt->cmd.len);

    if (frcdrv_send_cmd_pkt(oct, cpkt, input)) {
        FRCDRV_ERROR("Send ssh DMA ctrl addr fail!\n");
        rv = FRE_FAIL;
        goto frcdrv_core_cmd_error;
    }

    FRCDRV_DEBUG("respond.len = %d\n", core_req->respond.len);

    for (i = 0; i < FRCDRV_CORE_REQ_TIMEOUT; i++)
    {
        cavium_udelay(RECDRV_CORE_REQ_TIMEOUT_STEP);
        if (core_req->state == CORE_REQ_RESPOND)
        {
            break;
        }
    }

    FRCDRV_DEBUG("respond.len = %d\n", core_req->respond.len);
    if (i == FRCDRV_CORE_REQ_TIMEOUT)
    {   
        FRCDRV_ERROR("request timeout\n");
        rv = FRE_TIMEOUT;
        goto frcdrv_core_cmd_error;
    } 

    FRCDRV_DEBUG("respond.len = %d\n", core_req->respond.len);

    if (out_len < core_req->respond.len)
    {
        rv = FRE_EXCEED;
        goto frcdrv_core_cmd_error;
    }

    rv = core_req->respond.ret;

    if ((rv == FRE_SUCCESS) && (olen != NULL) && (output != NULL))
    {
        *olen = core_req->respond.len;
        memcpy(output, core_req->respond.data, *olen);
    }

frcdrv_core_cmd_error:
    if (core_req != NULL)
    {
        frcdrv_core_req_remove(core_req);
        frcdrv_free(core_req);
    }

    if (cpkt != NULL)
    {
        frcdrv_free(cpkt);
    }

    return rv;
}

int
frcdrv_cmd_init(int octeon_id)
{
    FRCDRV_DEBUG("frcdrv_core_respond_dispatch = %p\n", frcdrv_core_respond_dispatch);

    if(octeon_register_dispatch_fn(octeon_id, FRC_CMD_RESPOND_OP, frcdrv_core_respond_dispatch, NULL)) {
        FRCDRV_ERROR("Registration failed for opcode: FRC_CMD_RESPOND_OP\n");
        return FRE_FAIL;
    }

    FRC_INIT_LIST_HEAD(&frcdrv_core_req_head);

    return FRE_SUCCESS;
}

int
frcdrv_cmd_destroy(int octeon_id)
{
    FRCDRV_DEBUG("octeon_id = %d.\n", octeon_id);

    if(octeon_unregister_dispatch_fn(octeon_id, FRC_CMD_RESPOND_OP)) {
        FRCDRV_ERROR("Unregistration failed for opcode: FRC_CMD_RESPOND_OP\n");
        return FRE_FAIL;
    }

    return FRE_SUCCESS;
}

/* End of file */

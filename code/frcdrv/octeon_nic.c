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


#include "cavium_sysdep.h"
#include "cn3xxx_device.h"
#include "cn56xx_device.h"
#include "octeon_macros.h"
#include "octeon_nic.h"



int
octnet_send_nic_data_pkt(octeon_device_t *oct, octnic_data_pkt_t  *ndata)
{
	return octeon_send_noresponse_command(oct, ndata->q_no, 1, &ndata->cmd,
	                         ndata->buf, ndata->datasize, ndata->buftype);
}






void
octnet_link_ctrl_callback(octeon_req_status_t status, void *sif_ptr)
{
	octeon_soft_instruction_t  *si = (octeon_soft_instruction_t  *)sif_ptr;
	octnic_ctrl_pkt_t          *nctrl;

	nctrl = (octnic_ctrl_pkt_t *)((uint8_t *)si + OCT_SOFT_INSTR_SIZE);

	/* Call the callback function if status is OK.
	   Status is OK only if a response was expected and core returned success.
	   If no response was expected, status is OK if the command was posted
	   successfully. */
	if(!status && nctrl->cb_fn)
		nctrl->cb_fn(nctrl);

	cavium_free_dma(si);
}






static inline octeon_soft_instruction_t  *
octnic_alloc_ctrl_pkt_si(octeon_device_t  *oct, octnic_ctrl_pkt_t  *nctrl)
{
	octeon_soft_instruction_t  *si = NULL;
	uint8_t                    *data;
	uint32_t                    uddsize = 0, datasize = 0;

	uddsize  = (nctrl->ncmd.s.more * 8);

	datasize = OCTNET_CMD_SIZE +  uddsize + (nctrl->wait_time?16:0);

	/* Additional 8 bytes to align rptr to a 8 byte boundary. */
	datasize += sizeof(octnic_ctrl_pkt_t) + 8;

	si = cavium_malloc_dma( (OCT_SOFT_INSTR_SIZE + datasize),
	                        __CAVIUM_MEM_ATOMIC);
	if(si == NULL)
		return NULL;

	cavium_memset(si, 0, (OCT_SOFT_INSTR_SIZE + datasize) );

	cavium_memcpy(((uint8_t *)si + OCT_SOFT_INSTR_SIZE), nctrl,
	              sizeof(octnic_ctrl_pkt_t));

	si->ih.fsz     = 16;
	si->ih.tagtype = ORDERED_TAG;
	si->ih.tag     = 0x11111111;
	si->ih.raw     = 1;
	si->ih.rs      = 1;
    si->ih.grp     = FRC_CMD_GRP;
	si->irh.opcode = OCT_NW_CMD_OP;
	SET_SOFT_INSTR_DMA_MODE(si, OCTEON_DMA_DIRECT);
	SET_SOFT_INSTR_OCTEONID(si, oct->octeon_id);
	SET_SOFT_INSTR_IQ_NO(si, 0);
	SET_SOFT_INSTR_CALLBACK(si, octnet_link_ctrl_callback);
	SET_SOFT_INSTR_CALLBACK_ARG(si, (void *)si);

	data            = (uint8_t *)si + OCT_SOFT_INSTR_SIZE +
	                              sizeof(octnic_ctrl_pkt_t);
	si->dptr        = data;
	si->ih.dlengsz  = OCTNET_CMD_SIZE + uddsize;

	cavium_memcpy(data, &nctrl->ncmd, OCTNET_CMD_SIZE);
	octeon_swap_8B_data( (uint64_t *)data, (OCTNET_CMD_SIZE >> 3));

	if(uddsize) {
		/* Endian-Swap for UDD should have been done by caller. */
		cavium_memcpy(data + OCTNET_CMD_SIZE, nctrl->udd, uddsize);
	}

	if(nctrl->wait_time) {
		si->rptr           = ((uint8_t *)si->dptr + si->ih.dlengsz);
		if((unsigned long)si->rptr & 0x7) {
			si->rptr = (void *)(((unsigned long)si->rptr + 8) & ~0x7);
		}
		si->irh.rlenssz    = 16;
		si->status_word    = (uint64_t *)((uint8_t *)si->rptr + 8);
		*(si->status_word) =  COMPLETION_WORD_INIT;

		SET_SOFT_INSTR_RESP_ORDER(si, OCTEON_RESP_ORDERED);
		SET_SOFT_INSTR_RESP_MODE(si, OCTEON_RESP_NON_BLOCKING);
		SET_SOFT_INSTR_TIMEOUT(si, nctrl->wait_time);
	} else {
		SET_SOFT_INSTR_RESP_ORDER(si, OCTEON_RESP_NORESPONSE);
	}

	SET_SOFT_INSTR_ALLOCFLAGS(si, OCTEON_SOFT_INSTR_DB_NOW);

	cavium_print(PRINT_FLOW,
	             "%s si @ %p uddsize: %d datasize: %d dptr: %p rptr: %p\n",
	             __CVM_FUNCTION__, si, uddsize, datasize, si->dptr, si->rptr);

	return si;
}







int
octnet_send_nic_ctrl_pkt(octeon_device_t *oct, octnic_ctrl_pkt_t  *nctrl)
{
	octeon_instr_status_t      retval;
	octeon_soft_instruction_t  *si = NULL;

	si = octnic_alloc_ctrl_pkt_si(oct, nctrl);
	if(si == NULL) {
		cavium_error("OCTNIC: %s soft instr alloc failed\n", __CVM_FUNCTION__);
		return 1;
	}

	retval = octeon_process_instruction(oct, si, NULL);
	if(retval.u64 == OCTEON_REQUEST_FAILED) {
		cavium_error("OCTNIC: %s soft instr send failed\n", __CVM_FUNCTION__);
		return 1;
	}

	return 0;
}



/* $Id: octeon_nic.c 45683 2009-11-03 03:07:24Z panicker $ */

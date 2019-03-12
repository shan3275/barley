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

/*!  \file octeon_nic.h
     \brief Host NIC Driver: Routine to send network data & control packet to Octeon.
*/

#ifndef  __CAVIUM_NIC_H__
#define  __CAVIUM_NIC_H__

#include "frc.h"
#include "cavium-list.h"
#include "octeon_device.h"
#include "octeon-nic-common.h"

/* Maximum of 1 8-byte words can be sent in a NIC control message.
   There is support for upto 7 in the control command sent to Octeon but we
   restrict ourselves to what we need in the NIC module.
 */
#define  MAX_NCTRL_UDD  1




typedef   void (*octnic_ctrl_pkt_cb_fn_t)(void *);
 

/** Structure of control information passed by the NIC module to the OSI
	layer when sending control commands to Octeon device software. */
typedef struct {

	/** Command to be passed to the Octeon device software. */
	octnet_cmd_t   ncmd;

	/** Additional data that may be needed by some commands. */
	uint64_t       udd[MAX_NCTRL_UDD];

	/** Time to wait for Octeon software to respond to this control command.
	    If wait_time is 0, OSI assumes no response is expected. */
	unsigned long  wait_time;

	/** The network device that issued the control command. */
	unsigned long  netpndev;

	/** Callback function called when the command has been fetched by
	    Octeon. */ 
	octnic_ctrl_pkt_cb_fn_t  cb_fn;

	unsigned long   rsvd;

} octnic_ctrl_pkt_t;




/** Structure of data information passed by the NIC module to the OSI
	layer when forwarding data to Octeon device software. */
typedef struct {

	/** Pointer to information maintained by NIC module for this packet. The
	    OSI layer passes this as-is to the driver. */
	void                          *buf;

	/** Type of buffer passed in "buf" aboce. */
	int                            buftype;

	/** Total data bytes to be transferred in this command. */
	int                            datasize;

	/** Command to be passed to the Octeon device software. */
	octeon_instr_32B_t             cmd;

	/** Input queue to use to send this command. */
	int                            q_no;

} octnic_data_pkt_t;





/** Structure passed by NIC module to OSI layer to prepare a command to send
	network data to Octeon. */
typedef union {

	struct {
		uint32_t     gmxport:8;
		uint32_t     cksum_offset:7;
		uint32_t     gather:1;
		uint32_t     rsvd:16;
		union {
			uint32_t     datasize;
			uint32_t     gatherptrs;
		}u;
	}s;

	uint64_t   u64;

} octnic_cmd_setup_t;








static inline int
octnet_iq_is_full(octeon_device_t *oct, int q_no)
{
	return (cavium_atomic_read(&oct->instr_queue[q_no]->instr_pending)
                            >= (oct->instr_queue[q_no]->max_count - 2));
}


static inline int
octnet_iq_bp_on(octeon_device_t *oct, int q_no)
{
	return IQ_CHECK_BP_ON((octeon_iq_t *)&oct->instr_queue[q_no]);
}




static inline void
octnet_prepare_pci_cmd(octeon_instr_32B_t  *cmd,
                       octnic_cmd_setup_t  *setup)
{
	volatile octeon_instr_ih_t     *ih;
	volatile octeon_instr_irh_t    *irh;

	cmd->ih      = 0;
	ih           = (octeon_instr_ih_t *)&cmd->ih;

	ih->fsz      = 16;
	ih->tagtype  = ORDERED_TAG;
#if FRC_CONFIG_NIC_GRP
    ih->grp      = FRC_PKT_GRP;
#else
	ih->grp      = OCTNET_POW_GRP;
#endif
	ih->tag      = 0x11111111 + setup->s.gmxport;
	ih->raw      = 1;


	if(!setup->s.gather) {
		ih->dlengsz  = setup->s.u.datasize;
	} else {
		ih->gather   = 1;
		ih->dlengsz  = setup->s.u.gatherptrs;
	}

	cmd->rptr    = 0;
	cmd->irh     = 0;
	irh          = (octeon_instr_irh_t *)&cmd->irh;

	if(setup->s.cksum_offset)
		irh->rlenssz = setup->s.cksum_offset;

	irh->opcode  = OCT_NW_PKT_OP;
	irh->param   = setup->s.gmxport;
}



int
octnet_send_nic_data_pkt(octeon_device_t *oct, octnic_data_pkt_t  *ndata);


int
octnet_send_nic_ctrl_pkt(octeon_device_t *oct, octnic_ctrl_pkt_t  *nctrl);

#endif


/* $Id: octeon_nic.h 52638 2010-09-03 01:11:19Z panicker $ */

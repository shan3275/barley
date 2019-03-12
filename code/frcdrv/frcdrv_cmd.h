#ifndef __FRCDRV_CMD_H__
#define __FRCDRV_CMD_H__

#include "frc_cmd.h"
#include "frcore_ctrl.h"
#include "frc_list.h"

#define FRCDRV_CORE_REQ_TIMEOUT         300
#define RECDRV_CORE_REQ_TIMEOUT_STEP    10000

typedef   void (*frcdrv_cmd_pkt_cb_fn_t)(void *);

/** Structure of control information passed by the NIC module to the OSI
	layer when sending control commands to Octeon device software. */
typedef struct {

	/** Command to be passed to the Octeon device software. */
	frcore_cmd_t   cmd;

	/** Time to wait for Octeon software to respond to this control command.
	    If wait_time is 0, OSI assumes no response is expected. */
	unsigned long  wait_time;

	/** Callback function called when the command has been fetched by
	    Octeon. */ 
	frcdrv_cmd_pkt_cb_fn_t  cb_fn;

	unsigned long   rsvd;

} frcdrv_cmd_pkt_t;

typedef struct {
    frc_list_t node;
    uint32_t seq;
    uint32_t state;
    frc_cmd_respond_t respond;
} frcdrv_core_req_t;

#define CORE_REQ_WAITING    0
#define CORE_REQ_RESPOND    1

#define FRC_CTRL_CMD_SIZE   (sizeof(frcdrv_cmd_pkt_t))

void frcdrv_cmd_completion(void  *ctrl_ptr);

int frcdrv_send_cmd_pkt(octeon_device_t *oct, frcdrv_cmd_pkt_t  *cpkt, void *param);
int frcdrv_cmd_init(int octeon_id);
int frcdrv_cmd_destroy(int octeon_id);

int frcdrv_core_req_respond(frc_cmd_respond_t *respond);
int frcdrv_core_cmd(octeon_device_t *oct, uint16_t type, uint16_t cmd, uint16_t ilen, void *input, uint16_t *olen, void *output);
#define frcore_set(_oct, _type, _cmd, _ilen, _input) frcdrv_core_cmd(_oct, _type, _cmd, _ilen, _input, NULL, NULL)

//#define FRCDRV_DUMP_RESPOND_DATA

#endif /* !__FRCDRV_CMD_H__ */

#include "cvmcs-common.h"
#include  <cvmx-atomic.h>
#include "frc.h"
#include "frcore.h"
#include "frcore_dma.h"
#include "frcore_cmd.h"
#include "frc_util.h"

CVMX_SHARED frcore_cmd_set_t *frcore_cmdset;
CVMX_SHARED frcore_respond_buf_t *frcore_cmd_respond_buf = NULL;

#define FRCORE_RESPOND_BUF_GET(_core_id) frcore_cmd_respond_buf + (FRC_CMD_RESPOND_BUF_SZ * (_core_id))

int 
frcore_cmd_init(void)
{
    frcore_cmdset = cvmx_bootmem_alloc(sizeof(frcore_cmd_set_t) * CMD_TYPE_MAX, sizeof(frcore_cmd_set_t));
    if (frcore_cmdset == NULL)
    {
        FRCORE_ERROR("Alloc cmd set fail!\n");
        return FRE_FAIL;
    }
    memset(frcore_cmdset, 0, sizeof(sizeof(frcore_cmd_set_t) * CMD_TYPE_MAX));


    // TEST CMD
    frcore_cmdset[CMD_TYPE_TEST].vector = cvmx_bootmem_alloc(sizeof(frcore_cmd_cb_t) * TEST_CMD_MAX, sizeof(frcore_cmd_cb_t));
    if (frcore_cmdset[CMD_TYPE_TEST].vector == NULL)
    {
        FRCORE_ERROR("Alloc test cmd vector fail!\n");
        return FRE_FAIL;
    }
    memset(frcore_cmdset[CMD_TYPE_TEST].vector, 0, sizeof(frcore_cmd_cb_t) * TEST_CMD_MAX);
    frcore_cmdset[CMD_TYPE_TEST].max = TEST_CMD_MAX;
    frcore_cmdset[CMD_TYPE_TEST].num = 0;

    // CTRL CMD
    frcore_cmdset[CMD_TYPE_CTRL].vector = cvmx_bootmem_alloc(sizeof(frcore_cmd_cb_t) * CTRL_CMD_MAX, sizeof(frcore_cmd_cb_t));
    if (frcore_cmdset[CMD_TYPE_CTRL].vector == NULL)
    {
        FRCORE_ERROR("Alloc test cmd vector fail!\n");
        return FRE_FAIL;
    }
    memset(frcore_cmdset[CMD_TYPE_CTRL].vector, 0, sizeof(frcore_cmd_cb_t) * CTRL_CMD_MAX);
    frcore_cmdset[CMD_TYPE_CTRL].max = CTRL_CMD_MAX;
    frcore_cmdset[CMD_TYPE_CTRL].num = 0;

    // USER CMD
    frcore_cmdset[CMD_TYPE_USER].vector = cvmx_bootmem_alloc(sizeof(frcore_cmd_cb_t) * USER_CMD_MAX, sizeof(frcore_cmd_cb_t));
    if (frcore_cmdset[CMD_TYPE_USER].vector == NULL)
    {
        FRCORE_ERROR("Alloc test cmd vector fail!\n");
        return FRE_FAIL;
    }
    memset(frcore_cmdset[CMD_TYPE_USER].vector, 0, sizeof(frcore_cmd_cb_t) * USER_CMD_MAX);
    frcore_cmdset[CMD_TYPE_USER].max = USER_CMD_MAX;
    frcore_cmdset[CMD_TYPE_USER].num = 0;

    
    frcore_cmd_respond_buf = cvmx_bootmem_alloc(sizeof(frcore_respond_buf_t) * FRC_CORE_MAX, 0);

    if (frcore_cmd_respond_buf == NULL)
    {
        FRCORE_ERROR("Alloc cmd respond buffer fail!\n");
        return FRE_FAIL;
    }

    return FRE_SUCCESS;
}

int
frcore_cmd_register(uint16_t type, uint16_t cmd, frcore_cmd_fn_t fn)
{
    frcore_cmd_set_t       *cmd_set;
    frcore_cmd_cb_t           *cb;

    if (type >= CMD_TYPE_MAX)
    {
        return FRE_UNSUPPORT;
    }

    cmd_set = &frcore_cmdset[type];

    if (cmd >= cmd_set->max)
    {
        return FRE_UNSUPPORT;
    }

    cb = &cmd_set->vector[cmd];

    if (cb->fn)
    {
        return FRE_EXIST;
    }

    cb->type = type;
    cb->cmd = cmd;
    cb->fn = fn;
    cmd_set->num++;

    return FRE_SUCCESS;
}

void
frcore_process_frc_cmd(cvmx_wqe_t    *wqe)
{

    cvmx_raw_inst_front_t  *f = (cvmx_raw_inst_front_t *)wqe->packet_data;
	frcore_cmd_set_t       *cmd_set;
    frcore_cmd_cb_t        *cb;
    frcore_cmd_t           *cmd;
    uint8_t *param = NULL;

    frcore_respond_buf_t *resp_buf;
    int retval;

    uint32_t             port = 32;
    cvmx_buf_ptr_t        lptr;
    uint16_t olen = 0;

    if(octdev_get_pko_state() != CVM_DRV_PKO_READY){
        FRCORE_ERROR("PKO not ready!\n");
		return;
    }

    if (frcore_cmd_respond_buf == NULL)
    {
        FRCORE_ERROR("frcore_cmd_respond_buf is NLL\n");
        return;
    }

    resp_buf = &frcore_cmd_respond_buf[cvmx_get_core_num()];

    memset(resp_buf, 0, sizeof(frcore_respond_buf_t));

    //FRCORE_DEBUG("!!!!!!! resp_buf = %p\n", resp_buf);

	cmd = (frcore_cmd_t  *) ((uint8_t *)f + CVM_RAW_FRONT_SIZE);

    FRCORE_CMD("============cmd->seq = %d cmd->type = %d, cmd->cmd = %d, cmd->len = %d\n", cmd->seq, cmd->type, cmd->cmd, cmd->len);
    resp_buf->respond.seq = cmd->seq;

    if (cmd->len)
    {
        param = (uint8_t *) cmd + sizeof(frcore_cmd_t);
        //frc_dump_buff(wqe->len - CVM_RAW_FRONT_SIZE, (uint8_t *) cmd);
    }

    if (cmd->type >= CMD_TYPE_MAX)
    {
        retval = FRE_UNSUPPORT;
        goto send_error;
    }

    cmd_set = &frcore_cmdset[cmd->type];

    if (cmd->cmd >= cmd_set->max)
    {
        retval = FRE_UNSUPPORT;
        goto send_error;
    }

    cb = &cmd_set->vector[cmd->cmd];

    if (cb->fn)
    {
        retval = cb->fn(cmd->len, param, &olen, (void *) resp_buf->respond.data);
    } 
    else 
    {
        retval = FRE_UNREGISTER;
    }

send_error:    
    lptr.u64    = 0;
    lptr.s.addr = CVM_DRV_GET_PHYS(resp_buf);
    lptr.s.size = sizeof(frcore_respond_buf_t);
/*   
#if !defined(MAX_PKT_REUSE_BUF)
    lptr.s.i    = 1;
#endif*/ 

    resp_buf->hdr.u64 = 0;
    resp_buf->hdr.s.opcode    = FRC_CMD_RESPOND_OP;
    resp_buf->hdr.s.sport     = port;
    resp_buf->hdr.s.destqport = (port - 32);

    resp_buf->respond.ret = retval;
    resp_buf->respond.len = olen;

    /* Send a single buffer to PKO */

    if (cvm_pko_send_direct(lptr, CVM_DIRECT_DATA, 1, sizeof(frcore_respond_buf_t), port)) {
        FRCORE_ERROR("Send respond pkt fail!\n");
    }

    //FRCORE_DEBUG("============RESPOND: SEQ 0x%.8x, RET %d LEN %d.\n", 
    //             resp_buf->respond.seq, resp_buf->respond.ret, resp_buf->respond.len);
}

/* End of file */

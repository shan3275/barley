#ifndef __FRCORE_CMD_H__
#define __FRCORE_CMD_H__

#include "frc_cmd.h"
#include "frcore_ctrl.h"



typedef   int (*frcore_cmd_fn_t)(uint16_t plen, void *param, uint16_t *olen, void *outbuf);

typedef struct frcore_cmd {
    uint32_t reigster;
    uint16_t type;
    uint16_t cmd;
    frcore_cmd_fn_t fn;
} frcore_cmd_cb_t;

typedef struct frcore_cmd_set {
    uint32_t max;
    uint32_t num;
    frcore_cmd_cb_t *vector;
} frcore_cmd_set_t;

typedef struct {
    cvmx_resp_hdr_t hdr;
    frc_cmd_respond_t respond;
} frcore_respond_buf_t;

int frcore_cmd_register(uint16_t type, uint16_t cmd, frcore_cmd_fn_t fn);
int frcore_cmd_respond(int rv, int size, void *resp_buf);


#define FRCORE_REGISTER_CMD(_type, _cmd, _fn) \
{ \
    int _rv; \
    _rv = frcore_cmd_register(_type, _cmd, _fn); \
    if (_rv) \
    { \
        FRCORE_ERROR("REGISTER COMMAND %s %s fail!\n", #_type, #_cmd); \
        return _rv; \
    } \
}


int frcore_cmd_init(void);

#endif /* !__FRCORE_CMD_H__ */

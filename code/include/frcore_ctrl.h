#ifndef __FRCORE_CTRL_H__
#define __FRCORE_CTRL_H__


#define FRC_CORE_CMD_ARG_MAX        512
#define FRC_CORE_CMD_PARAM_MAX      (FRC_CORE_CMD_ARG_MAX / 8)

typedef struct {
#if __FRC_BYTE_ORDER == __FRC_LITTLE_ENDIAN
    uint64_t seq:32;
    uint64_t len:16;
    uint64_t type:4;
    uint64_t cmd:12;
#else
    uint64_t cmd:12;
    uint64_t type:4;
    uint64_t len:16;
    uint64_t seq:32;
#endif
} frcore_cmd_t;


typedef struct {
#if __FRC_BYTE_ORDER == __FRC_LITTLE_ENDIAN
    uint64_t ret:16;
    uint64_t len:16;
    uint64_t seq:32;
#else
    uint64_t seq:32;
    uint64_t len:16;
    uint64_t ret:16;
#endif
    uint8_t data[FRC_CMD_RESPOND_DATA_SZ];
} frc_cmd_respond_t;

typedef enum {
    CTRL_CMD_RESV,
    CTRL_CMD_SETUP_QUEUE,
    CTRL_CMD_SETUP_DMA_LOOP_BUFF,
    CTRL_CMD_PORT_ENABLE,
    CTRL_CMD_SETUP_SIMPLE_PACKAGE_CHAN,
    CTRL_CMD_SETUP_SSN_CHAN,
    CTRL_CMD_MAX
} frcore_ctrl_cmd_e;

#endif /* !__FRCORE_CTRL_H__ */

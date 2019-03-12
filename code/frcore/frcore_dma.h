#ifndef __FRCORE_DMA_H__
#define __FRCORE_DMA_H__

#include "frc_dma.h"
#include "frcore.h"

#define DMA_USE_COMPLETION_WORD
#define DMA_WAIT_INTERVAL           100000

#define FRCORE_MAX_DMA_POINTERS     CN56XX_PASS2_MAX_DMA_LOCAL_POINTERS
#define FRCORE_DMA_BUFF_MAX         (FRCORE_MAX_DMA_POINTERS * MAX_PCI_DMA_LOCAL_BUF_SIZE)

#define DMA_DIR_RECV  0
#define DMA_DIR_SEND  1

#define FRCORE_QUEUE_RING_SIZE      (FRCORE_MAX_DMA_POINTERS * 1024)
#define FRCORE_QUEUE_TIMEOUT        (FRCORE_TIM_TICKS_PSEC / 100)
#define FRCORE_QUEUE_POST_PERIOD    (cvmx_sysinfo_get()->cpu_clock_hz / 10)
#define FRCORE_QUEUE_POST_LWM       (FRCORE_MAX_DMA_POINTERS * 16)

#define FRCORE_CHAN_TIMEOUT        (FRCORE_TIM_TICKS_PSEC / 100)  /* 10ms */
//#define FRCORE_CHAN_TIMEOUT        (FRCORE_TIM_TICKS_PSEC / 200)    /* 5ms */
#define FRCORE_CHAN_POST_PERIOD    (cvmx_sysinfo_get()->cpu_clock_hz / 10)
#define FRCORE_CHAN_POST_LWM       (FRCORE_MAX_DMA_POINTERS * 16)

#define FRCORE_DMA_LOOP_TIMEOUT     FRCORE_TIM_TICKS * 10

typedef struct {
    int number;
    int size[FRCORE_MAX_DMA_POINTERS];
    uint8_t *local_ptr[FRCORE_MAX_DMA_POINTERS];
    uint64_t remote_addr[FRCORE_MAX_DMA_POINTERS];
} frcore_dma_scatter_desc_t;

typedef struct {
    cvmx_spinlock_t lock;
    //frc_dma_ctrl_t ctrl;
    uint32_t type;
    uint32_t queue_id;
    uint64_t ctrl_addr;
    uint64_t avail_size;
    uint64_t avail_widx;
    uint64_t avail_ridx;
    uint64_t compl_size;
    uint64_t compl_widx;
    uint64_t compl_ridx;
    uint64_t local_widx;
    uint64_t local_ridx;
    uint64_t rx_pkts;
    uint64_t tx_pkts;
    uint64_t post_cycle;
    uint64_t idx_buff[RING_BUFF_SIZE];
    uint64_t pkt_size[FRCORE_QUEUE_RING_SIZE];
    frc_dma_pkt_t pkts[FRCORE_QUEUE_RING_SIZE];
    cvmx_wqe_t wqe;
    frc_dma_buff_pool_t pool;
} frcore_queue_t;


typedef struct {
    cvmx_spinlock_t lock;
    frc_dma_ctrl_t ctrl;
    frc_dma_queue_desc_t desc;
    uint64_t rx_pkts;
    uint64_t tx_pkts;
    cvmx_wqe_t wqe;
} frcore_dma_ssn_queue_t;

typedef struct {
    cvmx_spinlock_t avail_lock;
    cvmx_spinlock_t compl_lock;
    uint64_t type;
    uint64_t ctrl_addr;
    uint64_t avail_widx;
    uint64_t avail_ridx;
    uint64_t compl_widx;
    uint64_t compl_ridx;
    uint64_t rx_pkts;
    uint64_t tx_pkts;
} frcore_simple_package_chan_t;

#define FRCORE_CORE_FIFO_NUM       (FRCORE_MAX_DMA_POINTERS * 16)
//#define FRCORE_CORE_FIFO_NUM       (FRCORE_MAX_DMA_POINTERS * 1024)
typedef struct frcore_simple_package_fifo_cell{
    uint16_t pkt_size;
    uint8_t  data[FRC_DMA_SIMPLE_PACKAGE_DATA_SIZE];
} frcore_simple_package_fifo_cell_t;
typedef struct frcore_simple_package_fifo{
    cvmx_spinlock_t lock;
    uint64_t rx_pkts;    /* fifo rx pkts */
    uint64_t tx_pkts;    /* fifo tx pkts */
    uint64_t local_widx; /* fifo tail index */
    uint64_t local_ridx; /* fifo head index */
    uint64_t post_cycle;
    cvmx_wqe_t wqe;
    uint64_t idx_buff[FRCORE_CORE_FIFO_NUM];
    frcore_simple_package_fifo_cell_t fifo_cell[FRCORE_CORE_FIFO_NUM];
}frcore_simple_package_fifo_t;

#define FRC_SIMPLE_PACKAGE_SIMPLE_CHAN_FIFO_NUM 10
#define FRC_SIMPLE_PACKAGE_FIFO_NUM  (FRC_SIMPLE_PACKAGE_SIMPLE_CHAN_FIFO_NUM + FRC_SIMPLE_PACKAGE_SIMPLE_CHAN_FIFO_NUM)

#define FRC_SSN_SIMPLE_CHAN_FIFO_NUM 10
#define FRC_SSN_FIFO_NUM (FRC_SSN_SIMPLE_CHAN_FIFO_NUM * 5)
#define FRCORE_SSN_AVAIL_BUFF_GET_SIZE   1000
typedef struct {
    cvmx_spinlock_t avail_lock;
    cvmx_spinlock_t compl_lock;
    uint64_t type;
    uint64_t avail_addr;
    uint64_t compl_addr;
    uint64_t avail_widx;
    uint64_t avail_ridx;
    uint64_t compl_widx;
    uint64_t compl_ridx;
    uint64_t rx_pkts;
    uint64_t tx_pkts;
    uint64_t avail_buff[FRCORE_SSN_AVAIL_BUFF_GET_SIZE];
    uint64_t compl_buff[FRCORE_SSN_AVAIL_BUFF_GET_SIZE];
} frcore_ssn_chan_t;

typedef enum {
    FRC_SSN_RESV,
    FRC_SSN_DMA_HEAD,
    FRC_SSN_DMA_PAYLOAD,
    FRC_SSN_DMA_MAX,
} frc_ssn_dma_type_e;

#if FRC_CONFIG_SSN_CHAN_TEST
#define FRC_PAYLOAD_LEN_MAX   8192
typedef struct frcore_ssn_fifo_cell{
    /*FRC_SSN_DMA_HEAD submit head;FRC_SSN_DMA_PAYLOAD submit info and payload */
    frc_ssn_dma_type_e type;
    frc_dma_hdr_t      head;
    frc_dma_pkt_info_t info;
    uint8_t  payload[FRC_PAYLOAD_LEN_MAX];
    uint16_t payload_size;
    uint64_t block_addr;    /* memory block address */
    uint16_t info_offset;
    uint16_t payload_offset;
} frcore_ssn_fifo_cell_t;
#else
#define FRC_PAYLOAD_LEN_MAX   1600
typedef struct frcore_ssn_fifo_cell{
    /*FRC_SSN_DMA_HEAD submit head;FRC_SSN_DMA_PAYLOAD submit info and payload */
    frc_ssn_dma_type_e type;
    frc_dma_hdr_t      head;
    frc_dma_pkt_info_t info;
    #if FRC_CONFIG_SSN_WQE_TEST /* for test */
    uint8_t  *payload;
    #else
    uint8_t  payload[FRC_PAYLOAD_LEN_MAX];
    #endif
    uint16_t payload_size;
    uint64_t block_addr;    /* memory block address */
    uint16_t info_offset;
    uint16_t payload_offset;
    cvmx_wqe_t  *wqe;
} frcore_ssn_fifo_cell_t;
#endif
typedef struct frcore_ssn_fifo{
    cvmx_spinlock_t lock;
    uint64_t rx_pkts;    /* fifo rx pkts */
    uint64_t tx_pkts;    /* fifo tx pkts */
    uint64_t local_widx; /* fifo tail index */
    uint64_t local_ridx; /* fifo head index */
    uint64_t post_cycle;
    cvmx_wqe_t wqe;
#if FRC_CONFIG_SSN_AVAIL_BUFF_GET
    uint64_t avail_rdx;
    uint64_t avail_buff[FRCORE_SSN_AVAIL_BUFF_GET_SIZE];
#endif
    frcore_ssn_fifo_cell_t  fifo_cell[FRCORE_CORE_FIFO_NUM];
}frcore_ssn_fifo_t;

#if FRC_CONFIG_SIMPLE_PACKAGE || FRC_CONFIG_SSN_CHAN
int frcore_dma_scatter(int dir, frcore_dma_scatter_desc_t *desc);
int frcore_dma_gather(int dir, void *local, uint64_t remote_addr, int size);
int frcore_copy_from_remote(int size, void *local, uint64_t remote_addr);
int frcore_copy_to_remote(int size, void *local, uint64_t remote_addr);
#endif
#if FRC_CONFIG_DMA_TEST
int frcore_dma_init();
#endif

int frcore_forward_tcp_pkt_to_host(frc_dma_hdr_t *hdr, frc_dma_pkt_info_t *info, void *payload);
int frcore_forward_udp_pkt_to_host(frc_dma_hdr_t *hdr, frc_dma_pkt_info_t *info, void *payload);


int frcore_tcp_dma_queue_process(cvmx_wqe_t *wqe);
int frcore_udp_dma_queue_process(cvmx_wqe_t *wqe);

int frcore_dma_loop(cvmx_wqe_t *wqe);

#endif

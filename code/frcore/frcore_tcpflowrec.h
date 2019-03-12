#include "cvmx-wqe.h"
#include "cvmx-pow.h"
#include "cvmx-spinlock.h"
#include "cvmx-pko.h"
#include "frcore.h"
#include "frcore_init.h"
#include "frcore_queue.h"
#include "frcore_ssn.h"

#ifndef __FRCORE_TCP_FLOW_REC_H__
#define __FRCORE_TCP_FLOW_REC_H__

#if FRC_CONFIG_SSN_CHAN
#define SEQ_GT(a,b) ((int)((a)-(b)) > 0)
#define TCP_SEG_QUEUE_MAXNUM 5

#define TCP_FLOW_REC_MAX   300000
//#define TCP_FLOW_REC_MAX 65536
//#define TCP_FLOW_REC_MAX 1024

#define TCP_FLOW_REC_QE_MAX (TCP_FLOW_REC_MAX * TCP_SEG_QUEUE_MAXNUM * 2)

/* tcp segment queue entry */
typedef struct tseg_qent {
    union {
        struct {
              LIST_ENTRY(tseg_qent)  tqe_q;
              uint64_t               seq        :32 ;
              uint64_t               ack_num    :32 ;
              uint64_t               nxt_seq;
              cvmx_wqe_t             *work;
              uint8_t                *data;
              uint64_t               data_len;
              uint64_t               direction;
        };
        uint64_t data_64[8];
    };
}tseg_qent;//64Byte

LIST_HEAD(tsegqe_head, tseg_qent);

typedef struct tcp_flow_data {
    union {
        struct {
            uint64_t rcv_nxt;
            struct tsegqe_head head;
            uint32_t tsegqe_len;
            uint32_t enable;
            struct tcp_flow_data *rev_flow_data;
        };
        uint64_t data_64[4];
        uint32_t data_32[8];
    };
}tcp_flow_data;//128B

typedef struct tcp_flow_rec {
    int32_t index;
    struct tcp_flow_data tcp_flow_data[TCP_FLOW_REC_MAX];
}tcp_flow_rec;

int tcp_flow_flush_lock(struct mpp_ssn *ssn);
void tcp_flow_recovery(cvmx_wqe_t *work, struct mpp_ssn *ssn, struct mpp_control *mpp, uint8_t direction);
int tcpflowrec_ddoq_send(uint32_t  ddoq_id, tcp_flow_data *flow_data,
                         struct mpp_ssn *ssn, int reverse);
int md_dump_tcpflowrec(struct mpp_ssn *ssn);
/* submit dma header to fifo */
int sbumit_dma_header_to_fifo(struct mpp_ssn *ssn);

#endif /* end of FRC_CONFIG_SSN_CHAN*/
#endif

#ifndef __FRC_DMA_H__
#define __FRC_DMA_H__

#if !__KERNEL__
#include <string.h>
#endif

#include "frc.h"


#define FRC_MEM_PAGE_SIZE           4096
#define FRC_DMA_PKT_SIZE            2048

#define FRC_DMA_BUFF_PAGES_ORDER    (10)
#define FRC_DMA_BUFF_PAGES          1024
#define FRC_DMA_BUFF_SIZE           (FRC_MEM_PAGE_SIZE * FRC_DMA_BUFF_PAGES)

#define FRC_DMA_LOOP_ENTRY_SIZE     1024

#define FRC_TCP_DMA_QUEUE_NUM       FRC_DAT_CORE_NUM
#define FRC_TCP_DMA_BUFF_NUM        1

#define FRC_UDP_DMA_QUEUE_NUM       FRC_DAT_CORE_NUM
#define FRC_UDP_DMA_BUFF_NUM        1

#define FRC_DMA_QUEUE_NUM           ((FRC_TCP_DMA_QUEUE_NUM) + (FRC_UDP_DMA_QUEUE_NUM))
#define FRC_TCP_QUEUE_START         0
#define FRC_UDP_QUEUE_START         (FRC_TCP_DMA_QUEUE_NUM)

#define FRC_QUEUE_UNKOWN            0
#define FRC_TCP_QUEUE               1
#define FRC_UDP_QUEUE               2

#define FRC_DMA_BUFF_MAX            12


#define RING_BUFF_CELL_SIZE         sizeof(uint64_t)
#define RING_BUFF_SIZE              (2048  * RING_BUFF_CELL_SIZE)

#define FRC_SSN_POSIVTE_FLOW         0
#define FRC_SSN_NEGATIVE_FLOW        1
/*
 * for single packet memory block, data_offset is the offset that payload refers to dmac, and also equale the sum of all bytes before payload.
 * payload_len is the length of effective payload, not including other filled bytes,such as tailer and so on                                 .
 * direction : for all memory block within a session, the packets at same direction,their direction is same.                                                                                                                                          .
*/
typedef struct frc_dma_pkt_info {
#if __FRC_BYTE_ORDER == __FRC_BIG_ENDIAN
  uint32_t ack_seq;  //udp = 0
  uint32_t sequence; //udp = 0

  uint16_t direction; // 0 for positive; 1 for negative
  uint16_t payload_len;
  uint32_t data_offset;

  uint64_t smac;
  uint64_t dmac;
#else
  uint32_t sequence; //udp = 0
  uint32_t ack_seq;  //udp = 0

  uint32_t data_offset;
  uint16_t payload_len;
  uint16_t direction;// 0 for positive; 1 for negative

  uint64_t smac; // the actual smac, low 6 bytes
  uint64_t dmac; // the actual dmac, low 6 bytes
#endif
} frc_dma_pkt_info_t;

typedef enum {
    DMA_BUFF_UNUSED,
    DMA_BUFF_AVAILABLE,
    DMA_BUFF_RESTORING,
    DMA_BUFF_COMPLETED,
    DMA_BUFF_REVERTING,
} dma_stat_e;

typedef enum {
    DMA_TYPE_RESV,
    DMA_TYPE_UDP,
    DMA_TYPE_TCP,
    DMA_TYPE_MAX
} dma_type_e;


/*
 * illustrate
 * for a new    session memory block, start_sec=1,start_usec=0,stop_sec=0,stop_usec=0
 * for a ending session memory block, start_sec=0,start_usec=0,stop_sec=1,stop_usec=0
 * for a middle session memory block, start_sec=0,start_usec=0,stop_sec=0,stop_usec=0
 * for a session, all blocks with the same five tuples, refring to first packet
 *
 * for single packet memory block, total_paylen equals the original packet length;
 * for tcp flow recovery memory block, total_paylen equals the sum of all payload length.
 * r1,r1 are reserved, used with special comment
 */
typedef struct frc_dma_hdr {
#if __FRC_BYTE_ORDER == __FRC_BIG_ENDIAN
  uint32_t dip;
  uint32_t sip;

  uint16_t stop_sec;
  uint16_t protocol;
  uint16_t dport;
  uint16_t sport;

  uint32_t hash;
  uint16_t total_paylen;
  uint16_t pkt_num;

  uint32_t pppoe_sess_id;
  uint32_t teid;

  uint16_t vlan_type;
  uint16_t vlan_id;
  uint16_t pppoe_paylen;
  uint16_t pppoe_protocol;
#else
  uint32_t sip;
  uint32_t dip;

  uint16_t sport;
  uint16_t dport;
  uint16_t protocol;
  uint16_t stop_sec;

  uint16_t pkt_num;
  uint16_t total_paylen;
  uint32_t hash;

  uint32_t teid;
  uint32_t pppoe_sess_id;

  uint16_t pppoe_protocol; //IPCP/CHAP/LCP/PAP/IP
  uint16_t pppoe_paylen;
  uint16_t vlan_id;
  uint16_t vlan_type; //IP/PPPoe
#endif
} frc_dma_hdr_t;

#define FRC_DMA_PKT_HEAD_SIZE       sizeof(frc_dma_hdr_t)
#define FRC_DMA_PKT_PAYLOAD_OFFSET  sizeof(frc_dma_hdr_t)
#define FRC_DMA_PKT_INFO_SIZE       sizeof(frc_dma_pkt_info_t)
#define FRC_DMA_PKT_INFO_OFFSET     sizeof(frc_dma_hdr_t)
#define FRC_DMA_PKT_DATA_OFFSET     ((FRC_DMA_PKT_INFO_OFFSET) + sizeof(frc_dma_pkt_info_t))
#define FRC_DMA_PKT_DATA_SIZE       ((FRC_DMA_PKT_SIZE) - (FRC_DMA_PKT_DATA_OFFSET))

typedef struct frc_dma_pkt {
    frc_dma_hdr_t      header;
    frc_dma_pkt_info_t info;
    uint8_t            data[FRC_DMA_PKT_DATA_SIZE];
} frc_dma_pkt_t;
typedef struct frc_simple_pkt {
    frc_dma_hdr_t      header;
    uint8_t            payload[FRC_DMA_PKT_DATA_SIZE];
    frc_dma_pkt_info_t info;
} frc_simple_pkt_t;

typedef struct frc_simple_rule_pkt {
    frc_dma_hdr_t      header;
    uint16_t           rule_id;
    uint8_t            payload[FRC_DMA_PKT_DATA_SIZE];
    frc_dma_pkt_info_t info;
} frc_simple_rule_pkt_t;
#define FRC_DMA_LOOP_HD_SIZE        16
#define FRC_DMA_LOOP_BUFF_SIZE  (FRC_DMA_BUFF_SIZE - FRC_DMA_LOOP_HD_SIZE)

typedef struct {
    uint64_t   widx;        /* data is added at offset (widx % size) */
    uint64_t   ridx;        /* data is extracted from off. (ridx % size) */
    uint8_t    buff[FRC_DMA_LOOP_BUFF_SIZE];      /* the buff holding the data */
} frc_dma_loop_buff_t;


typedef struct {
    uint64_t   size;        /* the size of the allocated buff */
    uint64_t   resv;
    uint64_t   widx;        /* data is added at offset (widx % size) */
    uint64_t   ridx;        /* data is extracted from off. (ridx % size) */
    uint8_t    buff[RING_BUFF_SIZE];      /* the buff holding the data */
} frc_ring_buff_t;

typedef struct frc_ssn_dma_ctrl {
    frc_ring_buff_t available_ring;
    frc_ring_buff_t completed_ring;
} frc_dma_ctrl_t;

#define FRC_DMA_AVAIL_OFFSET        0
#define FRC_DMA_AVAIL_SIZE_OFFSET   (FRC_DMA_AVAIL_OFFSET +  0)
#define FRC_DMA_AVAIL_WIDX_OFFSET   (FRC_DMA_AVAIL_OFFSET + 16)
#define FRC_DMA_AVAIL_RIDX_OFFSET   (FRC_DMA_AVAIL_OFFSET + 24)
#define FRC_DMA_AVAIL_BUFF_OFFSET   (FRC_DMA_AVAIL_OFFSET + 32)

#define FRC_DMA_COMPL_OFFSET        (RING_BUFF_SIZE + 32)
#define FRC_DMA_COMPL_SIZE_OFFSET   (FRC_DMA_COMPL_OFFSET +  0)
#define FRC_DMA_COMPL_WIDX_OFFSET   (FRC_DMA_COMPL_OFFSET + 16)
#define FRC_DMA_COMPL_RIDX_OFFSET   (FRC_DMA_COMPL_OFFSET + 24)
#define FRC_DMA_COMPL_BUFF_OFFSET   (FRC_DMA_COMPL_OFFSET + 32)

typedef struct {
#if __FRC_BYTE_ORDER == __FRC_LITTLE_ENDIAN
    uint32_t type;
    uint32_t queue_id;
#else
    uint32_t queue_id;
    uint32_t type;
#endif
    uint64_t ctrl_size;
    uint64_t ctrl_addr;
    uint64_t pool_addr;
} frc_dma_queue_desc_t;

typedef struct {
    uint64_t buff_numb;
    uint64_t buff_size;
    uint64_t buff_addr[FRC_DMA_BUFF_MAX];
    uint64_t block_numb;
    uint64_t block_size;
} frc_dma_buff_pool_t;

typedef struct {
    frc_dma_queue_desc_t desc;
    frc_dma_ctrl_t *dma_ctrl;
    frc_dma_buff_pool_t *pool;
} frc_dma_queue_t;

#define SWAP_8_BYTE(_i) \
  ((((((uint64_t)(_i)) >>  0) & (uint64_t)0xff) << 56) | \
   (((((uint64_t)(_i)) >>  8) & (uint64_t)0xff) << 48) | \
   (((((uint64_t)(_i)) >> 16) & (uint64_t)0xff) << 40) | \
   (((((uint64_t)(_i)) >> 24) & (uint64_t)0xff) << 32) | \
   (((((uint64_t)(_i)) >> 32) & (uint64_t)0xff) << 24) | \
   (((((uint64_t)(_i)) >> 40) & (uint64_t)0xff) << 16) | \
   (((((uint64_t)(_i)) >> 48) & (uint64_t)0xff) <<  8) | \
   (((((uint64_t)(_i)) >> 56) & (uint64_t)0xff) <<  0))


#define SWAP_4_BYTE(_i) \
    ((((uint32_t)(_i)) & 0xff000000) >> 24) | \
   ((((uint32_t)(_i)) & 0x00ff0000) >>  8) | \
   ((((uint32_t)(_i)) & 0x0000ff00) <<  8) | \
   ((((uint32_t)(_i)) & 0x000000ff) << 24)

#define SWAP_2_BYTE(_i) \
   ((((uint16_t)(_i)) & 0xff00) >>  8) | \
   ((((uint16_t)(_i)) & 0x00ff) <<  8)

#define frc_min(_x, _y) ((_x) < (_y) ? (_x) : (_y))


static inline void swap_copy(void *dest, void *src, int size)
{
    int i;
    uint64_t *d64, *s64;
    d64 = (uint64_t *) dest;
    s64 = (uint64_t *) src;
    for (i = 0; i < size; i++) {
        d64[i] = SWAP_8_BYTE(s64[i]);
    }
}

static inline void swap_buff(int len, void *buff)
{
    int i;
    uint64_t *p64;
    p64 = (uint64_t *) buff;
    for (i = 0; i < len; i++) {
        p64[i] = SWAP_8_BYTE(p64[i]);
    }
}

static inline uint64_t ring_ridx(frc_ring_buff_t *ring)
{
#if __FRC_BYTE_ORDER == __FRC_LITTLE_ENDIAN
    return ring->ridx;
#else
    return SWAP_8_BYTE(ring->ridx);
#endif
}

static inline uint64_t ring_widx(frc_ring_buff_t *ring)
{
#if __FRC_BYTE_ORDER == __FRC_LITTLE_ENDIAN
    return ring->widx;
#else
    return SWAP_8_BYTE(ring->widx);
#endif
}

static inline uint64_t ring_size(frc_ring_buff_t *ring)
{
#if __FRC_BYTE_ORDER == __FRC_LITTLE_ENDIAN
    return ring->size;
#else
    return SWAP_8_BYTE(ring->size);
#endif
}


/*
* __ring_put - puts some data into the FIFO, no locking version
* Note that with only one concurrent reader and one concurrent
* writer, you don't need extra locking to use these functions.
*/

static inline uint64_t
__ring_put(frc_ring_buff_t *ring, uint8_t *buff, uint64_t len)
{
    uint64_t l, pl;
    uint64_t size, ridx, widx;
    uint8_t *p;

    size = ring_size(ring);
    ridx = ring_ridx(ring);
    widx = ring_widx(ring);

    //
    len = frc_min(len, size - widx + ridx);
    //printk("%s.%d: len = %lld.\n", __func__, __LINE__, (ULL) len);
    /* first put the data starting from ring->widx to buff end */
    l = frc_min(len, size - (widx & (size - 1)));

    //l<=len
    p  = ring->buff + (widx & (size - 1));
    pl = l;
    memcpy(p, buff, pl);
#if __FRC_BYTE_ORDER == __FRC_BIG_ENDIAN
    swap_buff(pl >> 3, p);
#endif

    /* then put the rest (if any) at the beginning of the buff */
    p  = ring->buff;
    pl = len - l;
    memcpy(p, buff + l, pl);
#if __FRC_BYTE_ORDER == __FRC_BIG_ENDIAN
    swap_buff(pl >> 3, p);
#endif
    widx += len;

#if __FRC_BYTE_ORDER == __FRC_BIG_ENDIAN
    ring->widx = SWAP_8_BYTE(widx);
#else
    ring->widx = widx;
#endif
    //printk("%s.%d: len = %lld.\n", __func__, __LINE__, (ULL) len);
    return len;
}

 /*
 * __ring_get - gets some data from the FIFO, no locking version
 * Note that with only one concurrent reader and one concurrent
 * writer, you don't need extra locking to use these functions.
 */

static inline uint64_t
__ring_get(frc_ring_buff_t *ring, uint8_t *buff, uint64_t len)
{
    uint64_t l, pl;
    uint64_t size, ridx, widx;
    uint8_t *p;

    size = ring_size(ring);
    ridx = ring_ridx(ring);
    widx = ring_widx(ring);

    //
    len = frc_min(len, widx - ridx);

    /* first get the data from ridx until the end of the buff */
    l = frc_min(len, size - (ridx & (size - 1)));

    p  = buff;
    pl = l;
    memcpy(p, ring->buff + (ridx & (size - 1)), pl);
#if __FRC_BYTE_ORDER == __FRC_BIG_ENDIAN
    swap_buff(pl >> 3, p);
#endif
    /* then get the rest (if any) from the beginning of the buff */
    p  = buff + l;
    pl = len - l;
    memcpy(p, ring->buff, pl);
#if __FRC_BYTE_ORDER == __FRC_BIG_ENDIAN
    swap_buff(pl >> 3, p);
#endif

    ridx += len;

#if __FRC_BYTE_ORDER == __FRC_BIG_ENDIAN
    ring->ridx = SWAP_8_BYTE(ridx);
#else
    ring->ridx = ridx;
#endif

    return len;
}

#define AVAIL_RING_INIT(_dma_ctrl, _size) \
{ \
    memset((_dma_ctrl).available_ring.buff, 0, _size); \
    (_dma_ctrl).available_ring.size = (_size >> 3) << 3; \
    (_dma_ctrl).available_ring.resv = 0; \
    (_dma_ctrl).available_ring.widx = 0; \
    (_dma_ctrl).available_ring.ridx = 0; \
}

#define COMPL_RING_INIT(_dma_ctrl, _size) \
{ \
    memset((_dma_ctrl).completed_ring.buff, 0, _size); \
    (_dma_ctrl).completed_ring.size = (_size >> 3) << 3; \
    (_dma_ctrl).completed_ring.resv = 0; \
    (_dma_ctrl).completed_ring.widx = 0; \
    (_dma_ctrl).completed_ring.ridx = 0; \
}

#define AVAIL_RIDX_VAL(_dma_ctrl)   (ring_ridx(&(_dma_ctrl).available_ring))
#define AVAIL_RIDX_PTR(_dma_ctrl)   (&(_dma_ctrl).available_ring.ridx)
#define AVAIL_RIDX_SIZE(_dma_ctrl)  sizeof((_dma_ctrl).available_ring.ridx)


#define AVAIL_SIZE_VAL(_dma_ctrl)   (ring_size(&(_dma_ctrl).available_ring))
#define AVAIL_WIDX_VAL(_dma_ctrl)   (ring_widx(&(_dma_ctrl).available_ring))
#define AVAIL_WIDX_PTR(_dma_ctrl)   (&(_dma_ctrl).available_ring.widx)
#define AVAIL_WIDX_SIZE(_dma_ctrl)  sizeof((_dma_ctrl).available_ring.widx)

#define AVAIL_BUFF(_dma_ctrl)       (_dma_ctrl).available_ring.buff
#define AVAIL_BUFF_PTR(_dma_ctrl)   (&(_dma_ctrl).available_ring.buff)


#define AVAIL_RING_GET(_dma_ctrl, _index, _size) \
    (__ring_get(&((_dma_ctrl).available_ring), (uint8_t *) &(_index), _size) == _size)
#define AVAIL_RING_PUT(_dma_ctrl, _index, _size) \
    (__ring_put(&((_dma_ctrl).available_ring), (uint8_t *) &(_index), _size) == _size)

#define COMPL_SIZE_VAL(_dma_ctrl)   (ring_size(&(_dma_ctrl).completed_ring))
#define COMPL_RIDX_VAL(_dma_ctrl)   (ring_ridx(&(_dma_ctrl).completed_ring))
#define COMPL_RIDX_PTR(_dma_ctrl)   (&(_dma_ctrl).completed_ring.ridx)
#define COMPL_RIDX_SIZE(_dma_ctrl)  sizeof((_dma_ctrl).completed_ring.ridx)

#define COMPL_WIDX_VAL(_dma_ctrl)   (ring_widx(&(_dma_ctrl).completed_ring))
#define COMPL_WIDX_PTR(_dma_ctrl)   (&(_dma_ctrl).completed_ring.widx)
#define COMPL_WIDX_SIZE(_dma_ctrl)  sizeof((_dma_ctrl).completed_ring.widx)

#define COMPL_BUFF(_dma_ctrl)   (_dma_ctrl).completed_ring.buff
#define COMPL_BUFF_PTR(_dma_ctrl)   (&(_dma_ctrl).completed_ring.buff)

#define COMPL_RING_GET(_dma_ctrl, _index, _size) \
    (__ring_get(&((_dma_ctrl).completed_ring), (uint8_t *) &(_index), _size) == _size)
#define COMPL_RING_PUT(_dma_ctrl, _index, _size) \
    (__ring_put(&((_dma_ctrl).completed_ring), (uint8_t *) &(_index), _size) == _size)


#define FRC_DMA_PKT_PTR(_ctrl, _block, _index) (frc_dma_pkt_t *)((_ctrl)->block[(_block)] + ((_index) * FRC_DMA_PKT_SIZE))

typedef struct session_node {
  uint64_t prev_ptr;
  uint64_t next_ptr;
  uint64_t head_work;//
  uint64_t tail_work;/* */
  int32_t  block_index;
  int32_t  update;//session update time at last
  frc_dma_hdr_t dma_hdr;
} frc_session_node_t ;

#define genoffset(TYPE, MEMBER) ((size_t) &((TYPE *)0)->MEMBER)

//#define FRC_DMA_OFFSET  genoffset(frc_session_node_t, dma_hdr)
#define FRC_DMA_OFFSET (40)
#define FRC_DMA_SIMPLE_PACKAGE_BLOCK_SIZE     (2*K)
#define FRC_DMA_SIMPLE_PACKAGE_DATA_SIZE (FRC_DMA_SIMPLE_PACKAGE_BLOCK_SIZE - FRC_DMA_OFFSET)
#define FRC_DMA_SIMPLE_PACKAGE_RING_BUFF_SIZE (2*K*10)
#define FRC_SIMPLE_RING_BUFF_SIZE             FRC_DMA_SIMPLE_PACKAGE_RING_BUFF_SIZE
typedef enum {
    FRC_WORK_UDP,
    FRC_WORK_RULE,
    FRC_WORK_SSN,
    FRC_WORK_MAX
}frc_work_e;

typedef struct {
    uint64_t type;
    uint64_t ctrl_addr;
    uint64_t ctrl_size;
    uint64_t pool_addr;
    uint64_t pool_size;
} frc_dma_chan_desc_t;

typedef struct {
    uint64_t   widx;        /* physical address, big endian */
    uint64_t   ridx;        /* physical address, big endian */
    uint64_t   buff[FRC_DMA_SIMPLE_PACKAGE_RING_BUFF_SIZE];      /* the buff holding the data */
} frc_simple_package_ring_buff_t;

typedef struct frc_dma_simple_package_chan_ctrl {
    frc_simple_package_ring_buff_t available_ring;
    frc_simple_package_ring_buff_t completed_ring;
} frc_dma_simple_package_chan_ctrl_t;

typedef struct frc_simple_package_block{
    uint8_t reserverd[FRC_DMA_OFFSET];
    uint8_t data[FRC_DMA_SIMPLE_PACKAGE_DATA_SIZE];
}frc_simple_package_block_t;

static inline uint64_t simple_package_ring_ridx(frc_simple_package_ring_buff_t *ring)
{
#if __FRC_BYTE_ORDER == __FRC_BIG_ENDIAN
    return ring->ridx;
#else
    return SWAP_8_BYTE(ring->ridx);
#endif
}

static inline uint64_t simple_package_ring_widx(frc_simple_package_ring_buff_t *ring)
{
#if __FRC_BYTE_ORDER == __FRC_BIG_ENDIAN
    return ring->widx;
#else
    return SWAP_8_BYTE(ring->widx);
#endif
}

/*
* __simple_package_ring_put - puts some data into the FIFO, no locking version
* Note that with only one concurrent reader and one concurrent
* writer, you don't need extra locking to use these functions.
*/

static inline uint64_t
__simple_package_ring_put(frc_simple_package_ring_buff_t *ring, uint64_t buff)
{
    uint64_t len;
    uint64_t ridx, widx;

    ridx = simple_package_ring_ridx(ring);
    widx = simple_package_ring_widx(ring);

    if ((widx >= ridx) && (widx - ridx < FRC_DMA_SIMPLE_PACKAGE_RING_BUFF_SIZE)) {
    #if __FRC_BYTE_ORDER == __FRC_LITTLE_ENDIAN
        ring->buff[widx%FRC_DMA_SIMPLE_PACKAGE_RING_BUFF_SIZE] = SWAP_8_BYTE(buff);
    #else
        ring->buff[widx%FRC_DMA_SIMPLE_PACKAGE_RING_BUFF_SIZE] = buff;
    #endif
        widx += 1;
    #if __FRC_BYTE_ORDER == __FRC_LITTLE_ENDIAN
        ring->widx = SWAP_8_BYTE(widx);
    #else
        ring->widx = widx;
    #endif
        len = 1;
    }else{
        len = 0;
    }
    return len;
}

 /*
 * __simple_package_ring_get - gets some data from the FIFO, no locking version
 * Note that with only one concurrent reader and one concurrent
 * writer, you don't need extra locking to use these functions.
 */

static inline uint64_t
__simple_package_ring_get(frc_simple_package_ring_buff_t *ring, uint64_t *buff)
{
    uint64_t len;
    uint64_t ridx, widx;

    ridx = simple_package_ring_ridx(ring);
    widx = simple_package_ring_widx(ring);

    if ((widx > ridx) && (widx - ridx <= FRC_DMA_SIMPLE_PACKAGE_RING_BUFF_SIZE)) {
    #if __FRC_BYTE_ORDER == __FRC_LITTLE_ENDIAN
        *buff = SWAP_8_BYTE(ring->buff[ridx%FRC_DMA_SIMPLE_PACKAGE_RING_BUFF_SIZE]);
    #else
        *buff = ring->buff[ridx%FRC_DMA_SIMPLE_PACKAGE_RING_BUFF_SIZE];
    #endif
        ridx += 1;
    #if __FRC_BYTE_ORDER == __FRC_LITTLE_ENDIAN
        ring->ridx = SWAP_8_BYTE(ridx);
    #else
        ring->ridx = ridx;
    #endif
        len = 1;
    }else{
        len = 0;
    }
    return len;
}

#define SIMPLE_PACKAGE_AVAIL_RING_GET(_dma_ctrl, _buff) \
    (__simple_package_ring_get(&((_dma_ctrl).available_ring), (uint64_t *) &(_buff)) == 1)
#define SIMPLE_PACKAGE_AVAIL_RING_PUT(_dma_ctrl, _buff) \
    (__simple_package_ring_put(&((_dma_ctrl).available_ring), (uint64_t )  (_buff)) == 1)

#define SIMPLE_PACKAGE_COMPL_RING_GET(_dma_ctrl, _buff) \
    (__simple_package_ring_get(&((_dma_ctrl).completed_ring), (uint64_t *) &(_buff)) == 1)
#define SIMPLE_PACKAGE_COMPL_RING_PUT(_dma_ctrl, _buff) \
    (__simple_package_ring_put(&((_dma_ctrl).completed_ring), (uint64_t  )  (_buff)) == 1)

#define FRC_DMA_SIMPLE_PACKAGE_CHAN_POOL_NUM  10
#define FRC_DMA_SIMPLE_PACKAGE_CHAN_NUM       2
#define FRC_DMA_POOL_SIZE                     (FRC_MEM_PAGE_SIZE * FRC_DMA_BUFF_PAGES)
#define FRC_DMA_SIMPLE_PACKAGE_POOL_NUM_MAX  FRC_DMA_SIMPLE_PACKAGE_CHAN_POOL_NUM
typedef struct {
    uint64_t pool_num;
    uint64_t pool_addr[FRC_DMA_SIMPLE_PACKAGE_CHAN_POOL_NUM];
} frc_dma_simple_package_pool_t;

typedef struct {
    frc_dma_chan_desc_t desc;
    frc_dma_simple_package_chan_ctrl_t *dma_ctrl;
    frc_dma_simple_package_pool_t *pool;
} frc_dma_simple_package_chan_t;

#define FRC_DMA_SIMPLE_CHAN_CHAN_AVAIL_OFFSET        0
#define FRC_DMA_SIMPLE_CHAN_AVAIL_WIDX_OFFSET   (FRC_DMA_SIMPLE_CHAN_CHAN_AVAIL_OFFSET + 0)
#define FRC_DMA_SIMPLE_CHAN_AVAIL_RIDX_OFFSET   (FRC_DMA_SIMPLE_CHAN_CHAN_AVAIL_OFFSET + 8)
#define FRC_DMA_SIMPLE_CHAN_AVAIL_RING_OFFSET   (FRC_DMA_SIMPLE_CHAN_CHAN_AVAIL_OFFSET + 16)

#define FRC_DMA_SIMPLE_CHAN_COMPL_OFFSET        sizeof(frc_simple_package_ring_buff_t)
#define FRC_DMA_SIMPLE_CHAN_COMPL_WIDX_OFFSET   (FRC_DMA_SIMPLE_CHAN_COMPL_OFFSET + 0)
#define FRC_DMA_SIMPLE_CHAN_COMPL_RIDX_OFFSET   (FRC_DMA_SIMPLE_CHAN_COMPL_OFFSET + 8)
#define FRC_DMA_SIMPLE_CHAN_COMPL_RING_OFFSET   (FRC_DMA_SIMPLE_CHAN_COMPL_OFFSET + 16)

/* SSN chan */
//#define FRC_DMA_SSN_RING_BUFF_SIZE  (500224)
//#define FRC_DMA_SSN_RING_BUFF_SIZE  (300544)
#define FRC_DMA_SSN_RING_BUFF_SIZE  (400896)
//#define FRC_DMA_SSN_RING_BUFF_SIZE  (100352)
#define FRC_DMA_SSN_BLOCK_SIZE      (8*K)
#define FRC_DMA_SSN_CHAN_POOL_NUM   (FRC_DMA_SSN_RING_BUFF_SIZE/(FRC_DMA_POOL_SIZE/FRC_DMA_SSN_BLOCK_SIZE))
#define FRC_DMA_SSN_POOL_NUM_MAX     FRC_DMA_SSN_CHAN_POOL_NUM
#define FRC_DMA_SSN_CHAN_NUM        1
#define FRC_DMA_SSN_DATA_SIZE (FRC_DMA_SSN_BLOCK_SIZE - FRC_DMA_OFFSET)
typedef struct {
    uint64_t   widx;        /* physical address, big endian */
    uint64_t   ridx;        /* physical address, big endian */
    uint64_t   buff[FRC_DMA_SSN_RING_BUFF_SIZE];      /* the buff holding the data */
} frc_ssn_ring_buff_t;

typedef struct {
    uint64_t pool_num;
    uint64_t pool_addr[FRC_DMA_SSN_CHAN_POOL_NUM];
} frc_dma_ssn_pool_t;

typedef struct {
    uint64_t type;
    uint64_t avail_addr;
    uint64_t avail_size;
    uint64_t compl_addr;
    uint64_t compl_size;
    uint64_t pool_addr;
    uint64_t pool_size;
} frc_dma_ssn_chan_desc_t;

typedef struct {
    frc_dma_ssn_chan_desc_t desc;
    frc_ssn_ring_buff_t *available_ring;
    frc_ssn_ring_buff_t *completed_ring;
    frc_dma_ssn_pool_t *pool;
} frc_dma_ssn_chan_t;

static inline uint64_t ssn_ring_ridx(frc_ssn_ring_buff_t *ring)
{
#if __FRC_BYTE_ORDER == __FRC_BIG_ENDIAN
    return ring->ridx;
#else
    return SWAP_8_BYTE(ring->ridx);
#endif
}

static inline uint64_t ssn_ring_widx(frc_ssn_ring_buff_t *ring)
{
#if __FRC_BYTE_ORDER == __FRC_BIG_ENDIAN
    return ring->widx;
#else
    return SWAP_8_BYTE(ring->widx);
#endif
}

/*
* __ssn_ring_put - puts some data into the FIFO, no locking version
* Note that with only one concurrent reader and one concurrent
* writer, you don't need extra locking to use these functions.
*/

static inline uint64_t
__ssn_ring_put(frc_ssn_ring_buff_t *ring, uint64_t buff)
{
    uint64_t len;
    uint64_t ridx, widx;

    ridx = ssn_ring_ridx(ring);
    widx = ssn_ring_widx(ring);

    if ((widx >= ridx) && (widx - ridx < FRC_DMA_SSN_RING_BUFF_SIZE)) {
    #if __FRC_BYTE_ORDER == __FRC_LITTLE_ENDIAN
        ring->buff[widx%FRC_DMA_SSN_RING_BUFF_SIZE] = SWAP_8_BYTE(buff);
    #else
        ring->buff[widx%FRC_DMA_SSN_RING_BUFF_SIZE] = buff;
    #endif
        widx += 1;
    #if __FRC_BYTE_ORDER == __FRC_LITTLE_ENDIAN
        ring->widx = SWAP_8_BYTE(widx);
    #else
        ring->widx = widx;
    #endif
        len = 1;
    }else{
        len = 0;
    }
    return len;
}

 /*
 * __simple_package_ring_get - gets some data from the FIFO, no locking version
 * Note that with only one concurrent reader and one concurrent
 * writer, you don't need extra locking to use these functions.
 */

static inline uint64_t
__ssn_ring_get(frc_ssn_ring_buff_t *ring, uint64_t *buff)
{
    uint64_t len;
    uint64_t ridx, widx;

    ridx = ssn_ring_ridx(ring);
    widx = ssn_ring_widx(ring);
    /*if (widx - ridx <= 50000) {
        return 0;
    }*/
    if ((widx > ridx) && (widx - ridx <= FRC_DMA_SSN_RING_BUFF_SIZE)) {
    #if __FRC_BYTE_ORDER == __FRC_LITTLE_ENDIAN
        *buff = SWAP_8_BYTE(ring->buff[ridx%FRC_DMA_SSN_RING_BUFF_SIZE]);
    #else
        *buff = ring->buff[ridx%FRC_DMA_SSN_RING_BUFF_SIZE];
    #endif
        ridx += 1;
    #if __FRC_BYTE_ORDER == __FRC_LITTLE_ENDIAN
        ring->ridx = SWAP_8_BYTE(ridx);
    #else
        ring->ridx = ridx;
    #endif
        len = 1;
    }else{
        len = 0;
    }
    return len;
}

#define SSN_AVAIL_RING_GET(_dma_chan, _buff) \
    (__ssn_ring_get(((_dma_chan).available_ring), (uint64_t *) &(_buff)) == 1)
#define SSN_AVAIL_RING_PUT(_dma_chan, _buff) \
    (__ssn_ring_put(((_dma_chan).available_ring), (uint64_t )  (_buff)) == 1)

#define SSN_COMPL_RING_GET(_dma_chan, _buff) \
    (__ssn_ring_get(((_dma_chan).completed_ring), (uint64_t *) &(_buff)) == 1)
#define SSN_COMPL_RING_PUT(_dma_chan, _buff) \
    (__ssn_ring_put(((_dma_chan).completed_ring), (uint64_t  )  (_buff)) == 1)

#define FRC_DMA_SSN_CHAN_AVAIL_OFFSET   0
#define FRC_DMA_SSN_AVAIL_WIDX_OFFSET   (FRC_DMA_SSN_CHAN_AVAIL_OFFSET + 0)
#define FRC_DMA_SSN_AVAIL_RIDX_OFFSET   (FRC_DMA_SSN_CHAN_AVAIL_OFFSET + 8)
#define FRC_DMA_SSN_AVAIL_RING_OFFSET   (FRC_DMA_SSN_CHAN_AVAIL_OFFSET + 16)

#define FRC_DMA_SSN_COMPL_OFFSET        0
#define FRC_DMA_SSN_COMPL_WIDX_OFFSET   (FRC_DMA_SSN_COMPL_OFFSET + 0)
#define FRC_DMA_SSN_COMPL_RIDX_OFFSET   (FRC_DMA_SSN_COMPL_OFFSET + 8)
#define FRC_DMA_SSN_COMPL_RING_OFFSET   (FRC_DMA_SSN_COMPL_OFFSET + 16)

typedef struct frc_ssn_block{
    uint8_t reserverd[FRC_DMA_OFFSET];
    uint8_t data[FRC_DMA_SSN_DATA_SIZE];
}frc_ssn_block_t;

#endif /* !__FRC_DMA_H__ */

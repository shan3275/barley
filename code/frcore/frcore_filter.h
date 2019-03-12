#ifndef _FRCORE_FILTER_H
#define _FRCORE_FILTER_H

#include "cvmx-config.h"
#include "cvmx.h"
#include "cvmx-wqe.h"
#include "cvmx-fau.h"
#include "cvmx-atomic.h"
#include "cvmx-spinlock.h"

#include "frcore_init.h"
#include "frcore_config.h"
#include "frcore_debug.h"
#include "frcore_queue.h"
#include "frc_types.h"

struct TwoTupleFilter
{
    uint16_t enable;    /* 1 for effictive; 0 for no effective */
    uint16_t acl_type;  /* four acl type */
    uint16_t action;    /* reserved */
    uint16_t op;        /* reserved */
    uint16_t acl_source; /* reserved */
    uint16_t proto;     /* 1 for TCP; 0 for UDP */
    uint32_t one_tuple; /* one of SIP/DIP/SP/DP */
    uint32_t index;     /* acl index, from 1 to 2000 */
    uint32_t hash;      /* acl hash value */
    uint64_t pkt_num;
    uint64_t pkt_bytes;
};

#define FRCORE_MAX_FILTERS (2*K) // maximum amount of acl filters

struct TwoTupleFiltSet
{
    unsigned int numFilters;
    struct TwoTupleFilter filterArr[FRCORE_MAX_FILTERS];
};

typedef struct frcore_acl{
    struct TwoTupleFiltSet acl_data;
}frcore_acl;



#define CVMX_FPA_ACL_HASH_CELL_POOL  (7)   /* for acl hash cell */
#define CVMX_FPA_ACL_HASH_CELL_POOL_SIZE   CVMX_CACHE_LINE_SIZE
#define FRCORE_ACL_HASH_CELL_BUF_NUMS  FRCORE_MAX_FILTERS
#define FRCORE_ACL_HASH_MASK (FRCORE_ACL_HASH_CELL_BUF_NUMS - 1)
#define FRCORE_ACL_HASH_TABLE_SIZE (FRCORE_ACL_HASH_CELL_BUF_NUMS * 4)

/* acl hash table cell */
typedef struct acl_hash_cell {
    union {
        struct {
            LIST_ENTRY(acl_hash_cell) tqe_q;
            uint64_t acl_index;
        };
        uint64_t data_64[3];
    };
}acl_hash_cell;//24Byte

LIST_HEAD(bucket_head, acl_hash_cell);
typedef struct frcore_acl_hash_table_t {
    union {
        struct {
            struct bucket_head head;
            uint64_t bucket_depth;
            uint64_t total_cell;
            uint64_t del_cell;
            cvmx_spinlock_t lock;
            uint32_t reserved;
        };
        uint64_t data_64[5];
        uint32_t data_32[10];
    };
}frcore_acl_hash_table_t;//32B

int frcore_filter_init(void);

/**
 *
 *
 * @author shan (6/13/2013)
 *
 * @param gid
 *
 * @return uint16_t
 */
uint16_t acl_get_filter_count(uint16_t *acl_num);
/**
 * Get one acl from acl table by index
 *
 * @author shan (6/11/2013)
 *
 * @param index 1 to 2000
 * @param one_tuple
 * @param protocol
 * @param op
 * @param action
 * @param acl_type
 * @param acl_source
 *
 * @return int
 */
int acl_get_one_filter(uint16_t index, uint32_t *one_tuple, uint16_t *proto,uint16_t *op,
                       uint16_t *action,uint16_t *acl_type, uint16_t *acl_source,
                       uint32_t *pkt_num, uint32_t *bytes_num, uint32_t *hash);

/**
 *
 *
 * @author shan (6/15/2013)
 *
 * @param acl_type
 * @param tagid  from 0 to 1999
 * @param bucket_depth
 * @param total_cell
 * @param del_cell
 *
 * @return int
 */
int acl_get_one_hash_table(uint16_t acl_type, uint16_t tagid, uint32_t *bucket_depth,
                           uint32_t *total_cell, uint32_t *del_cell);

/**
 *
 *
 * @author shan (6/11/2013)
 *
 * @param index  from 1 to 2000
 * @param one_tuple
 * @param protocol
 * @param op
 * @param action
 * @param acl_type
 * @param acl_source
 */
int acl_set_filter(uint16_t index, uint32_t one_tuple, uint16_t protocol,uint16_t op,
                    uint16_t action, uint16_t acl_type, uint16_t acl_source);

/**
 *
 *
 * @author shan (6/11/2013)
 *
 * @param index from 1 to 2000
 *
 * @return int
 */
int acl_del_filter(uint16_t index);

/**
 *
 *
 * @author shan (6/13/2013)
 *
 * @param tuple
 *
 * @return uint16_t
 */
uint16_t filter_lookup_by_tuple(struct mpp_tuple tuple);

inline void filter_statistic_hit(uint16_t index);
inline void filter_init_statistic_hit(uint16_t index);
inline uint64_t filter_get_statistic_hit(uint16_t index);
inline void filter_pkt_bytes_add(uint16_t index, uint16_t pkt_len);
inline void filter_init_pkt_bytes(uint16_t index);
inline uint64_t filter_get_pkt_bytes(uint16_t index);

/**
 *
 *
 * @author shan (6/14/2013)
 *
 * @param index from 1 to 2000
 */
void filter_init_filter_statistics(uint16_t index);
#endif /* end of */

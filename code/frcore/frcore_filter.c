#include "frcore.h"
#include "frcore_cmd.h"
#include "frcore_misc.h"
#include "frc_pack.h"
#include "frcore_proto.h"
#include "cvmx-mdio.h"
#include "frcore_init.h"
#include "frcore_acl.h"
#include "frcore_pkt.h"
#include "frc_types.h"
#include "frcore_stat.h"
#include "frcore_chan.h"
#include "frcore_filter.h"

#if FRC_CONFIG_TWO_TUPLE
CVMX_SHARED extern mpp_shared_data gsdata;

#define INDEX_ET(a,b) ((a) == (b))

#define FRCORE_ACL_HASH_TABLE_LOCK(_hash_table) cvmx_spinlock_lock(&((_hash_table)->lock));\
                                                FRCORE_INFO("lock acl hash table\n")
#define FRCORE_ACL_HASH_TABLE_UNLOCK(_hash_table) cvmx_spinlock_unlock(&((_hash_table)->lock));\
                                                FRCORE_INFO("unlock acl hash table\n")
/* get two tuple acl table pointer */
/**
 *
 *
 * @author shan (6/11/2013)
 *
 * @return frcore_acl*
 */
frcore_acl* acl_get_acl_array(void)
{
   return gsdata.two_tuple_acl;
}

/* get two tuple acl has table */
/**
 *
 *
 * @author shan (6/11/2013)
 *
 * @param acl_type
 * @param hash
 *
 * @return frcore_acl_hash_table_t*
 */
frcore_acl_hash_table_t *acl_get_hash_table(uint16_t acl_type, uint32_t hash)
{
   frcore_acl_hash_table_t *hash_table = NULL;

   if(acl_type <= FRC_ACL_UNKNOWN || acl_type > FRC_ACL_DP) {
      /* error handle and add statistics */
      FRCORE_STAT_INC(stat_acl_type_error);
      return hash_table;
   }

   if(hash >= FRCORE_MAX_FILTERS) {
      /* error handle and add statistcis */
      FRCORE_STAT_INC(stat_acl_hash_error);
      return hash_table;
   }

   hash_table = &gsdata.acl_hash_table[FRCORE_MAX_FILTERS * (acl_type - 1) + hash];
   return hash_table;
}

/**
 * The following code is for filter only by two tuple
 *
 * @author shan (6/11/2013)
 *
 * @param one_tuple
 * @param proto
 *
 * @return uint32_t
 */
static inline uint32_t acl_hash_crc(uint32_t one_tuple, uint16_t proto)
{
    int idx;
    CVMX_MT_CRC_POLYNOMIAL (0x1edc6f41);
    CVMX_MT_CRC_IV (0);
    CVMX_MT_CRC_WORD (one_tuple);
    CVMX_MT_CRC_HALF (proto);
    CVMX_MF_CRC_IV (idx);
    return idx & FRCORE_ACL_HASH_MASK;
}

/*
 * init two tuple acl and hash table
 * @author shan (6/11/2013)
 *
 * @return int
 */
int frcore_filter_init(void)
{
   int i;

   /* init two tuple acl */
   frcore_acl *acl = acl_get_acl_array();
   acl->acl_data.numFilters = 0;
   for(i = 0; i < FRCORE_MAX_FILTERS; i++) {
      acl->acl_data.filterArr[i].enable = 0;
      acl->acl_data.filterArr[i].acl_type = 0;
      acl->acl_data.filterArr[i].action = 0;
      acl->acl_data.filterArr[i].op = 0;
      acl->acl_data.filterArr[i].acl_source = 0;
      acl->acl_data.filterArr[i].proto = 0;
      acl->acl_data.filterArr[i].one_tuple = 0;
      acl->acl_data.filterArr[i].index = 0;
      acl->acl_data.filterArr[i].hash = 0;
      acl->acl_data.filterArr[i].pkt_num = 0;
      acl->acl_data.filterArr[i].pkt_bytes = 0;
   }
   printf("two tuple acl init finished!\n");

   /* init two tuple acl hash table */
   frcore_acl_hash_table_t *acl_hash_table;
   acl_hash_table = gsdata.acl_hash_table;
   for(i = 0; i < FRCORE_ACL_HASH_TABLE_SIZE; i++) {
      acl_hash_table[i].head.lh_first = NULL;
      acl_hash_table[i].bucket_depth = 0;
      acl_hash_table[i].total_cell = 0;
      acl_hash_table[i].del_cell = 0;
      cvmx_spinlock_init(&acl_hash_table[i].lock);
      acl_hash_table[i].reserved = 0;
   }
   printf("two tuple acl hash table init finished!\n");

   return FRE_SUCCESS;
}

inline void filter_statistic_hit(uint16_t index)
{
    cvmx_atomic_fetch_and_add64((int64_t *)(&gsdata.two_tuple_acl->acl_data.filterArr[index-1].pkt_num), 1);
}

inline void filter_init_statistic_hit(uint16_t index)
{
    cvmx_atomic_set64((int64_t *)(&gsdata.two_tuple_acl->acl_data.filterArr[index-1].pkt_num), 0);
}

inline uint64_t filter_get_statistic_hit(uint16_t index)
{
    return cvmx_atomic_fetch_and_add64((int64_t *)(&gsdata.two_tuple_acl->acl_data.filterArr[index-1].pkt_num),
                                       0);
}

inline void filter_pkt_bytes_add(uint16_t index, uint16_t pkt_len)
{
    cvmx_atomic_fetch_and_add64((int64_t *)(&gsdata.two_tuple_acl->acl_data.filterArr[index-1].pkt_bytes),
                                pkt_len);
}

inline void filter_init_pkt_bytes(uint16_t index)
{
    cvmx_atomic_set64((int64_t *)(&gsdata.two_tuple_acl->acl_data.filterArr[index-1].pkt_bytes), 0);
}

inline uint64_t filter_get_pkt_bytes(uint16_t index)
{
    return cvmx_atomic_fetch_and_add64((int64_t *)(&gsdata.two_tuple_acl->acl_data.filterArr[index-1].pkt_bytes),
                                        0);
}

/**
 *
 *
 * @author shan (6/14/2013)
 *
 * @param index from 1 to 2000
 */
void filter_init_filter_statistics(uint16_t index)
{
    filter_init_statistic_hit(index);
    filter_init_pkt_bytes(index);
}
/**
 *
 *
 * @author shan (6/13/2013)
 *
 * @param gid
 *
 * @return uint16_t
 */
uint16_t acl_get_filter_count(uint16_t *acl_num)
{
    uint16_t num;
    frcore_acl *acl_entry = NULL;

    /* get two tuple acl array */
    acl_entry = acl_get_acl_array();
    if(acl_entry == NULL) {
       return FRE_FAIL;
    }

    num = (uint16_t) (acl_entry->acl_data.numFilters);

    *acl_num = num;
    return FRE_SUCCESS;
}
/**
 * Get hash from acl table by index
 *
 * @author shan (6/11/2013)
 *
 * @param index
 * @param hash
 *
 * @return int
 */
int acl_get_hash(uint16_t index, uint32_t * hash)
{
   frcore_acl *acl_entry = NULL;

   /* get two tuple acl array */
   acl_entry = acl_get_acl_array();
   if(acl_entry == NULL) {
      return FRE_FAIL;
   }

   /* get hash */
   *hash       = acl_entry->acl_data.filterArr[index -1].hash;

   return FRE_SUCCESS;
}

/**
 * Get hash from acl table by index
 *
 * @author shan (6/11/2013)
 *
 * @param index from 1 to 2000
 * @param acl_type
 *
 * @return int
 */
int acl_get_type(uint16_t index, uint16_t * acl_type)
{
   frcore_acl *acl_entry = NULL;

   /* get two tuple acl array */
   acl_entry = acl_get_acl_array();
   if(acl_entry == NULL) {
      return FRE_FAIL;
   }

   /* get hash */
   *acl_type       = acl_entry->acl_data.filterArr[index -1].acl_type;

   return FRE_SUCCESS;
}
/**
 * Get acl status from acl table by index
 *
 * @author shan (6/11/2013)
 *
 * @param index from 1 to 2000
 * @param acl_status
 *
 * @return int
 */
int acl_get_status(uint16_t index, uint16_t * acl_status)
{
   frcore_acl *acl_entry = NULL;

   /* get two tuple acl array */
   acl_entry = acl_get_acl_array();
   if(acl_entry == NULL) {
      return FRE_FAIL;
   }

   /* get hash */
   *acl_status       = acl_entry->acl_data.filterArr[index -1].enable;

   return FRE_SUCCESS;
}
/* Delete one acl by index, don't do other things */
/**
 *
 *
 * @author shan (6/15/2013)
 *
 * @param index  from 1 to 2000
 *
 * @return int
 */
int acl_delete_one_filter(uint16_t index)
{
   frcore_acl *acl_entry = NULL;

   /* get two tuple acl array */
   acl_entry = acl_get_acl_array();
   if(acl_entry == NULL) {
      return FRE_FAIL;
   }

   /*cler acl entry content */
   acl_entry->acl_data.numFilters--;
   acl_entry->acl_data.filterArr[index-1].enable = 0;
   acl_entry->acl_data.filterArr[index-1].acl_type = FRC_ACL_UNKNOWN;
   acl_entry->acl_data.filterArr[index-1].action = 0;
   acl_entry->acl_data.filterArr[index-1].op = 0;
   acl_entry->acl_data.filterArr[index-1].acl_source = 0;
   acl_entry->acl_data.filterArr[index-1].proto = 0;
   acl_entry->acl_data.filterArr[index-1].one_tuple = 0;
   acl_entry->acl_data.filterArr[index-1].index = 0;
   acl_entry->acl_data.filterArr[index-1].hash = 0;
   //acl_entry->acl_data.filterArr[index-1].pkt_num = 0;
   //acl_entry->acl_data.filterArr[index-1].pkt_bytes = 0;
   filter_init_filter_statistics(index);
   FRCORE_STAT_INC(stat_acl_del_rule_num);
   return FRE_SUCCESS;

}

/**
 * Update acl, only write acl to acl table and cal hash to add into acl, don't do other things
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
int acl_add_one_filter(uint16_t index, uint32_t one_tuple, uint16_t proto,uint16_t op,
                       uint16_t action, uint16_t acl_type, uint16_t acl_source,
                       uint32_t hash)
{
   frcore_acl *acl_entry = NULL;


   /* get two tuple acl array */
   acl_entry = acl_get_acl_array();
   if(acl_entry == NULL) {
      return FRE_FAIL;
   }

   /* write to acl entry */
   acl_entry->acl_data.filterArr[index -1].acl_type = acl_type;
   acl_entry->acl_data.filterArr[index -1].action = action;
   acl_entry->acl_data.filterArr[index -1].op = op;
   acl_entry->acl_data.filterArr[index -1].acl_source = acl_source;
   acl_entry->acl_data.filterArr[index -1].proto = proto;
   acl_entry->acl_data.filterArr[index -1].one_tuple = one_tuple;
   acl_entry->acl_data.filterArr[index -1].index = index;
   acl_entry->acl_data.filterArr[index -1].hash = hash;
   acl_entry->acl_data.filterArr[index -1].enable = 1;
   acl_entry->acl_data.numFilters++;
   FRCORE_STAT_INC(stat_acl_add_rule_num);

   return FRE_SUCCESS;
}

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
                       uint32_t *pkt_num, uint32_t *bytes_num, uint32_t *hash_value)
{
   int rv;
   uint32_t hash;
   uint16_t type;
   frcore_acl_hash_table_t *hash_table = NULL;
   frcore_acl *acl_entry = NULL;

   /* get two tuple acl array */
   acl_entry = acl_get_acl_array();
   if(acl_entry == NULL) {
      return FRE_FAIL;
   }

   if(acl_entry->acl_data.filterArr[index - 1].enable == 0) {
      return FRE_INIT;
   }

   /* get hash and acl_type */
   rv = acl_get_hash(index, &hash);
   if(rv) {
      FRCORE_ERROR("acl_get_hash\n");
      return rv;
   }

   FRCORE_INFO("index:%d, hash:%d\n", index, hash);

   rv = acl_get_type(index, &type);
   if(rv) {
      FRCORE_ERROR("acl_get_type\n");
      return rv;
   }

   /* find out the hash table head by acl_type */
   hash_table = acl_get_hash_table(type, hash);
   if(hash_table == NULL) {
      /* error handling */
      FRCORE_ERROR("acl_get_hash_table");
      return FRE_FAIL;
   }

   /* lock hash table */
   FRCORE_ACL_HASH_TABLE_LOCK(hash_table);

   /* get acl entry */
   *acl_type   = acl_entry->acl_data.filterArr[index -1].acl_type;
   *action     = acl_entry->acl_data.filterArr[index -1].action;
   *op         = acl_entry->acl_data.filterArr[index -1].op;
   *acl_source = acl_entry->acl_data.filterArr[index -1].acl_source;
   *proto      = acl_entry->acl_data.filterArr[index -1].proto;
   *one_tuple  = acl_entry->acl_data.filterArr[index -1].one_tuple;
   *hash_value = acl_entry->acl_data.filterArr[index -1].hash;
   *pkt_num    = filter_get_statistic_hit(index);
   *bytes_num  = filter_get_pkt_bytes(index);
   FRCORE_ACL_HASH_TABLE_UNLOCK(hash_table);

   return FRE_SUCCESS;
}

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
                           uint32_t *total_cell, uint32_t *del_cell)
{
   frcore_acl_hash_table_t *hash_table = NULL;

   /* find out the hash table head by acl_type */
   hash_table = acl_get_hash_table(acl_type, (uint32_t)tagid);
   if(hash_table == NULL) {
      /* error handling */
      FRCORE_ERROR("acl_get_hash_table");
      return FRE_FAIL;
   }

   FRCORE_ACL_HASH_TABLE_LOCK(hash_table);
   *bucket_depth = hash_table->bucket_depth;
   *total_cell   = hash_table->total_cell;
   *del_cell     = hash_table->del_cell;
   FRCORE_ACL_HASH_TABLE_UNLOCK(hash_table);
   return FRE_SUCCESS;
}

/**
 * Delete one acl index from hash table
 *
 * @author shan (6/11/2013)
 *
 * @param index from 1 to 2000
 *
 * @return int
 */
int acl_delete_one_filter_from_hash_table(uint16_t index, frcore_acl_hash_table_t *hash_table)
{

   acl_hash_cell *q;

   /* as a  exception test case */
   if(hash_table->bucket_depth == 0 ||
      LIST_EMPTY(&hash_table->head)) {
      FRCORE_STAT_INC(stat_acl_entry_exception);
      return FRE_FAIL;
   }

   /*traverse the list to find out the index, then del index */
   LIST_FOREACH(q, &hash_table->head, tqe_q){
      if(INDEX_ET(q->acl_index, index)) {
         break;
      }
   }

   if(q) {
      /* have found out the index */
      FRCORE_STAT_INC(stat_acl_del_hash_cell);
      LIST_REMOVE(q, tqe_q);
      cvmx_fpa_free(q, CVMX_FPA_ACL_HASH_CELL_POOL, 0);
      hash_table->bucket_depth--;
      hash_table->del_cell++;
      CVMX_SYNCWS;
   }else {
      /* not find out the index, so exception */
      FRCORE_STAT_INC(stat_acl_not_found_index);
      return FRE_FAIL;
   }

   return FRE_SUCCESS;
}

/**
 * Add one acl index to hash table
 *
 * @author shan (6/11/2013)
 *
 * @param index from 1 to 2000
 *
 * @return int
 */
int acl_add_one_filter_to_hash_table(uint16_t index)
{

   frcore_acl_hash_table_t *hash_table;
   uint32_t hash;
   uint16_t acl_type;
   acl_hash_cell *q;
   acl_hash_cell *p  = NULL;
   acl_hash_cell *te = NULL;
   int rv;

   /* get hash and acl_type */
   rv = acl_get_hash(index, &hash);
   if(rv) {
      FRCORE_ERROR("acl_get_hash\n");
      return rv;
   }

   rv = acl_get_type(index, &acl_type);
   if(rv) {
      FRCORE_ERROR("acl_get_type\n");
      return rv;
   }

   /* find out the hash table head by acl_type */
   hash_table = acl_get_hash_table(acl_type, hash);
   if(hash_table == NULL) {
      /* error handling */
      FRCORE_ERROR("acl_get_hash_table");
      return FRE_FAIL;
   }


   /*
    * Allocate a new cell entry. If we can't, or hit the FPA limit
    * just drop the pkt.
    *
    */
   te = (struct acl_hash_cell *)cvmx_fpa_alloc(CVMX_FPA_ACL_HASH_CELL_POOL);
   if (te == NULL) {
       FRCORE_ERROR("tcp reasseble alloc failed\n");
       FRCORE_STAT_INC(stat_acl_hash_cell_alloc_fail);
       return FRE_FAIL;
   } else {
       te->data_64[0] = 0;
       te->data_64[1] = 0;
       te->data_64[2] = 0;
   }

   /* write index to cell */
   te->acl_index = index;

   /*traverse the list*/
   LIST_FOREACH(q, &hash_table->head, tqe_q){
      p = q;
   }

   if (p == NULL) {
       LIST_INSERT_HEAD(&hash_table->head, te, tqe_q);
   } else {
       LIST_INSERT_AFTER(p, te, tqe_q);
   }
   hash_table->bucket_depth++;
   hash_table->total_cell++;
   FRCORE_STAT_INC(stat_acl_add_hash_cell);
   CVMX_SYNCWS;/* must sync */

   return FRE_SUCCESS;
}

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
int acl_set_filter(uint16_t index, uint32_t one_tuple, uint16_t proto,uint16_t op,
                    uint16_t action, uint16_t acl_type, uint16_t acl_source)
{
   int rv;
   uint32_t hash;
   uint16_t acl_type_old;
   uint16_t acl_status = 0;
   frcore_acl_hash_table_t *hash_table = NULL;

   /* parameter check */
   if(index < ACL_INDEX_START || index > ACL_MAX) {
      FRCORE_ERROR("index exceed!!\n");
      return FRE_PARAM;
   }

   if(acl_type < FRC_ACL_SIP || acl_type > FRC_ACL_DP) {
      FRCORE_ERROR("acl_type error!!\n");
      return FRE_PARAM;
   }

   /* get old acl status */
   rv = acl_get_status(index, &acl_status);
   if(rv) {
      FRCORE_ERROR("acl_get_status\n");
      return rv;
   }

   if(acl_status) {

      /* get hash and acl_type */
      rv = acl_get_hash(index, &hash);
      if(rv) {
         FRCORE_ERROR("acl_get_hash\n");
         return rv;
      }

      rv = acl_get_type(index, &acl_type_old);
      if(rv) {
         FRCORE_ERROR("acl_get_type\n");
         return rv;
      }

      /* find out the hash table head by acl_type */
      hash_table = acl_get_hash_table(acl_type_old, hash);
      if(hash_table == NULL) {
         /* error handling */
         FRCORE_ERROR("acl_get_hash_table");
         return FRE_FAIL;
      }

      /* lock hash table */
      FRCORE_ACL_HASH_TABLE_LOCK(hash_table);

      /* delete acl from hash table*/
      rv = acl_delete_one_filter_from_hash_table(index, hash_table);
      if(rv){
         FRCORE_ERROR("acl_delete_one_filter_from_hash_table\n");
         FRCORE_ACL_HASH_TABLE_UNLOCK(hash_table);
         return rv;
      }

      /* delete acl from acl table */
      rv = acl_delete_one_filter(index);
      if(rv){
         FRCORE_ERROR("acl_delete_one_filter\n");
         FRCORE_ACL_HASH_TABLE_UNLOCK(hash_table);
         return rv;
      }

      /* unlock the hash table */
      FRCORE_ACL_HASH_TABLE_UNLOCK(hash_table);

   }
   /* cal hash value */
   hash = acl_hash_crc(one_tuple, proto);

   FRCORE_INFO("index:%d, hash:%d\n", index, hash);

   /* find out the hash table head by acl_type */
   hash_table = acl_get_hash_table(acl_type, hash);
   if(hash_table == NULL) {
      /* error handling */
      FRCORE_ERROR("acl_get_hash_table");
      return FRE_FAIL;
   }

   /* lock the hash table */
   FRCORE_ACL_HASH_TABLE_LOCK(hash_table);
   /* add acl to acl table */
   rv = acl_add_one_filter(index, one_tuple, proto, op, action , acl_type, acl_source, hash);
   if(rv){
      /* error handling */
      FRCORE_ERROR("acl_add_one_filter\n");
      FRCORE_ACL_HASH_TABLE_UNLOCK(hash_table);
      return rv;
   }

   /* add acl to acl hash table */
   rv = acl_add_one_filter_to_hash_table(index);
   if(rv){
      /* error handling */
      FRCORE_ERROR("acl_add_one_filter_to_hash_table\n");
      FRCORE_ACL_HASH_TABLE_UNLOCK(hash_table);
      return rv;
   }
   /*unlock */
   FRCORE_ACL_HASH_TABLE_UNLOCK(hash_table);

   return FRE_SUCCESS;
}

/**
 *
 *
 * @author shan (6/11/2013)
 *
 * @param index from 1 to 2000
 *
 * @return int
 */
int acl_del_filter(uint16_t index)
{
    uint32_t hash;
    uint16_t acl_type;
    uint16_t acl_status = 0;
    frcore_acl_hash_table_t *hash_table = NULL;
    int rv;

   /* parameter check */
   if(index < ACL_INDEX_START || index > ACL_MAX) {
      FRCORE_ERROR("index exceed!!\n");
      return FRE_PARAM;
   }

   /* get old acl status */
   rv = acl_get_status(index, &acl_status);
   if(rv) {
      FRCORE_ERROR("acl_get_status\n");
      return rv;
   }

   if(!acl_status) {
      return FRE_INIT;
   }

   /* get hash and acl_type */
   rv = acl_get_hash(index, &hash);
   if(rv) {
      FRCORE_ERROR("acl_get_hash\n");
      return rv;
   }

   FRCORE_INFO("index:%d, hash:%d\n", index, hash);

   rv = acl_get_type(index, &acl_type);
   if(rv) {
      FRCORE_ERROR("acl_get_type\n");
      return rv;
   }

   /* find out the hash table head by acl_type */
   hash_table = acl_get_hash_table(acl_type, hash);
   if(hash_table == NULL) {
      /* error handling */
      FRCORE_ERROR("acl_get_hash_table");
      return FRE_FAIL;
   }

   /* lock hash table */
   FRCORE_ACL_HASH_TABLE_LOCK(hash_table);

   /* delete acl from hash table*/
   rv = acl_delete_one_filter_from_hash_table(index, hash_table);
   if(rv){
      FRCORE_ERROR("acl_delete_one_filter_from_hash_table\n");
      FRCORE_ACL_HASH_TABLE_UNLOCK(hash_table);
      return rv;
   }

   /* delete acl from acl table */
   rv = acl_delete_one_filter(index);
   if(rv){
      FRCORE_ERROR("acl_delete_one_filter\n");
      FRCORE_ACL_HASH_TABLE_UNLOCK(hash_table);
      return rv;
   }

   /* unlock hash table */
   FRCORE_ACL_HASH_TABLE_UNLOCK(hash_table);

   return FRE_SUCCESS;
}

/**
 *
 *
 * @author shan (6/13/2013)
 *
 * @param tuple
 *
 * @return uint16_t
 */
uint16_t filter_lookup_by_tuple(struct mpp_tuple tuple)
{
   uint32_t one_tuple[4] = {0};
   frcore_acl_hash_table_t *hash_table = NULL;
   uint32_t hash;
   uint16_t acl_type;
   acl_hash_cell *q;
   int i;

   frcore_acl *acl_entry = NULL;

   /* get two tuple acl array */
   acl_entry = acl_get_acl_array();
   if(acl_entry == NULL) {
      return FRE_FAIL;
   }

   /* store first tuple to array */
   one_tuple[0] = tuple.sip;
   one_tuple[1] = tuple.dip;
   one_tuple[2] = tuple.sp;
   one_tuple[3] = tuple.dp;

   /* start handling */
   for(i = 0; i < 4; i++)
   {
      /* cal hash */
      hash = acl_hash_crc(one_tuple[i], tuple.proto);

      acl_type = i + 1;

      /* find out the hash table head by acl_type */
      hash_table = acl_get_hash_table(acl_type, hash);
      if(hash_table == NULL) {
         /* error handling */
         FRCORE_ERROR("acl_get_hash_table");
         return FRE_FAIL;
      }

      /* lock the hash table */
      FRCORE_ACL_HASH_TABLE_LOCK(hash_table);

      /* if hash table is empty, jump out to continue */
      if(LIST_EMPTY(&hash_table->head) &&
         hash_table->bucket_depth == 0) {
         FRCORE_ACL_HASH_TABLE_UNLOCK(hash_table);
         continue;
      }

      /*traverse the list to find out the index, then del index */
      LIST_FOREACH(q, &hash_table->head, tqe_q){
         /* match */
         if(acl_entry->acl_data.filterArr[q->acl_index-1].one_tuple == one_tuple[i] &&
            acl_entry->acl_data.filterArr[q->acl_index-1].proto == tuple.proto ) {
            FRCORE_ACL_HASH_TABLE_UNLOCK(hash_table);
            return (uint16_t) (q->acl_index);
         }
      }

      /* unlock the hash table */
      FRCORE_ACL_HASH_TABLE_UNLOCK(hash_table);
   }
   /* not match */
   return 0;
}

#endif

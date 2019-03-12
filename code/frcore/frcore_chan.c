#include "frcore_pkt.h"
#include "frcore_cmd.h"
#include "frcore_dma.h"

#include "frc_dma.h"
#include "frc_util.h"

#include "frcore_stat.h"
#include "frc_crc8.h"

#if FRC_CONFIG_SIMPLE_PACKAGE
CVMX_SHARED frcore_simple_package_chan_t *frcore_simple_package_chans = NULL;
CVMX_SHARED frcore_simple_package_fifo_t *frcore_simple_package_fifos = NULL;

extern CVMX_SHARED  uint64_t            cpu_freq;
uint8_t  udp_block_exhaust[FRC_SIMPLE_PACKAGE_SIMPLE_CHAN_FIFO_NUM]  = {0};
uint64_t udp_last_display[FRC_SIMPLE_PACKAGE_SIMPLE_CHAN_FIFO_NUM]   = {0};
uint8_t  rule_block_exhaust[FRC_SIMPLE_PACKAGE_SIMPLE_CHAN_FIFO_NUM] = {0};
uint64_t rule_last_display[FRC_SIMPLE_PACKAGE_SIMPLE_CHAN_FIFO_NUM]  = {0};

#if FRC_CONFIG_SIMPLE_PACKAGE
#   define FRCORE_SIMPLE_PACKAGE_CHAN_AVAIL_LOCK(_chan)     cvmx_spinlock_lock(&((_chan)->avail_lock))
#   define FRCORE_SIMPLE_PACKAGE_CHAN_AVAIL_UNLOCK(_chan)   cvmx_spinlock_unlock(&((_chan)->avail_lock))
#   define FRCORE_SIMPLE_PACKAGE_CHAN_COMPL_LOCK(_chan)     cvmx_spinlock_lock(&((_chan)->compl_lock))
#   define FRCORE_SIMPLE_PACKAGE_CHAN_COMPL_UNLOCK(_chan)   cvmx_spinlock_unlock(&((_chan)->compl_lock))
#endif

int frcore_cmd_setup_simple_package_chans(uint16_t plen, void *param, uint16_t *olen, void *outbuf)
{
    int i;
    frcore_simple_package_chan_t *chan = NULL;
    frcore_simple_package_fifo_t *fifo = NULL;
    frc_dma_chan_desc_t *desc;
    *olen = 0;

    desc = (frc_dma_chan_desc_t *) param;

    FRCORE_CMD("chan_id %lld,  ctrl_size %llx, ctrl_addr 0x%llx.\n",
               (ULL) desc->type, (ULL) desc->ctrl_size, (ULL) desc->ctrl_addr);

    if (frcore_simple_package_chans == NULL) {
        FRCORE_ERROR("\n");
        return FRE_INIT;
    }

    if (desc->type >= FRC_DMA_SIMPLE_PACKAGE_CHAN_NUM) {
        FRCORE_ERROR("desc->type(%lld) >= FRC_DMA_SIMPLE_PACKAGE_CHAN_NUM(%d)\n", (ULL) desc->type, FRC_DMA_SIMPLE_PACKAGE_CHAN_NUM);
        return FRE_EXCEED;
    }

    if ((desc->type != FRC_WORK_UDP) && (desc->type != FRC_WORK_RULE))
    {
        FRCORE_ERROR("Unkown type %lld!\n", (ULL)desc->type);
        return FRE_PARAM;
    }

    chan = &frcore_simple_package_chans[desc->type];

    /* setup ctrl ring */
    frcore_dma_scatter_desc_t dma_arg;

    memset(&dma_arg, 0, sizeof(frcore_dma_scatter_desc_t));

    dma_arg.local_ptr[0] = (uint8_t *) &chan->avail_widx;
    dma_arg.remote_addr[0]  = desc->ctrl_addr + FRC_DMA_SIMPLE_CHAN_AVAIL_WIDX_OFFSET;
    dma_arg.size[0] = sizeof(chan->avail_widx);

    dma_arg.local_ptr[1] = (uint8_t *) &chan->avail_ridx;
    dma_arg.remote_addr[1]  = desc->ctrl_addr + FRC_DMA_SIMPLE_CHAN_AVAIL_RIDX_OFFSET;
    dma_arg.size[1] = sizeof(chan->avail_ridx);

    dma_arg.local_ptr[2] = (uint8_t *) &chan->compl_widx;
    dma_arg.remote_addr[2]  = desc->ctrl_addr + FRC_DMA_SIMPLE_CHAN_COMPL_WIDX_OFFSET;
    dma_arg.size[2] = sizeof(chan->compl_widx);

    dma_arg.local_ptr[3] = (uint8_t *) &chan->compl_ridx;
    dma_arg.remote_addr[3]  = desc->ctrl_addr + FRC_DMA_SIMPLE_CHAN_COMPL_RIDX_OFFSET;
    dma_arg.size[3] = sizeof(chan->compl_ridx);
    dma_arg.number = 4;

    if( frcore_dma_scatter(DMA_DIR_RECV, &dma_arg)) {
        FRCORE_ERROR("Copy ctrl arg from remote fail!\n");
        FRCORE_STAT_INC(stat_dma_errors);
        return FRE_DMA;
    }
    chan->ctrl_addr = desc->ctrl_addr;
    chan->type = desc->type;

    FRCORE_CMD("CHAN %lld(%s): CTRL_ADDR 0x%llx; AVAIL RIDX 0x%llX, WIDX 0x%llX; COMPL RIDX 0x%llX, WIDX 0x%llX.\n",
               (ULL) chan->type, (ULL) (chan->type == FRC_WORK_UDP) ? "UDP" : "RULE", (ULL) chan->ctrl_addr,
               (ULL) (chan->avail_ridx), (ULL) (chan->avail_widx),
               (ULL) (chan->compl_ridx), (ULL) (chan->compl_widx));


    /* Init chan lock*/
    cvmx_spinlock_init(&chan->avail_lock);
    cvmx_spinlock_init(&chan->compl_lock);
    fifo = &frcore_simple_package_fifos[desc->type * FRC_SIMPLE_PACKAGE_SIMPLE_CHAN_FIFO_NUM];
    FRCORE_CMD("fifo=%p\n", fifo);
    /* Init fifo timer */
   for (i = 0; i < FRC_SIMPLE_PACKAGE_SIMPLE_CHAN_FIFO_NUM; i++)
    {
        cvmx_spinlock_init(&fifo[i].lock);
        cvmx_wqe_t *wqe = &fifo[i].wqe;
        memset(wqe, 0, sizeof(cvmx_wqe_t));
        wqe->unused = FRCORE_WORK_CHAN;
        wqe->ipprt  = 0;
        wqe->grp    = i + 1;

        wqe->tag      = wqe->grp;
        wqe->tag_type = CVMX_POW_TAG_TYPE_ATOMIC;
        wqe->packet_data[0] = (uint8_t)desc->type; /* dup,rule or ssn */
        wqe->packet_data[1] = i;                   /* fifo id */
        if (cvmx_tim_add_entry(wqe, FRCORE_CHAN_TIMEOUT, NULL) != CVMX_TIM_STATUS_SUCCESS) {
            FRCORE_ERROR("dma chan fifo timer add fail!\n");
            return FRE_FAIL;
        }

    }

    return FRE_SUCCESS;
}

int frcore_cmd_setup_simple_package_chans_test(uint16_t plen, void *param, uint16_t *olen, void *outbuf)
{
    int i;
    uint64_t *type;
    *olen = 0;

    type = (uint64_t *) param;
    printf("%s %d type=%d\n",  __func__,  __LINE__, (int)*type);
    /* Init fifo timer */
    for (i = 0; i < 1; i++)
  //  for (i = 0; i < FRC_SIMPLE_PACKAGE_SIMPLE_CHAN_FIFO_NUM; i++)
    {
        cvmx_wqe_t *wqe = cvmx_bootmem_alloc(sizeof(cvmx_wqe_t), 0);;
        memset(wqe, 0, sizeof(cvmx_wqe_t));

        wqe->unused = FRCORE_WORK_CHAN_TEST;
        wqe->ipprt  = 0;
        wqe->grp    = i + 1;

        wqe->tag      = wqe->grp;
        wqe->tag_type = CVMX_POW_TAG_TYPE_ATOMIC;
        wqe->packet_data[0] = (uint8_t)*type; /* dup,rule or ssn */
        wqe->packet_data[1] = i;                   /* fifo id */

        if (cvmx_tim_add_entry(wqe, FRCORE_CHAN_TIMEOUT, NULL) != CVMX_TIM_STATUS_SUCCESS) {
            FRCORE_ERROR("dma chan test fifo timer add fail!\n");
            return FRE_FAIL;
        }
    }

    return FRE_SUCCESS;
}


int frcore_simple_package_chan_init()
{
    frcore_simple_package_chans = cvmx_bootmem_alloc(sizeof(frcore_simple_package_chan_t) * FRC_DMA_SIMPLE_PACKAGE_CHAN_NUM, 0);
    if (frcore_simple_package_chans == NULL)
    {
        FRCORE_ERROR("Alloc tcp dma queues fail!\n");
        return FRE_MEMORY;
    }

    memset(frcore_simple_package_chans, 0, sizeof(frcore_simple_package_chan_t) * FRC_DMA_SIMPLE_PACKAGE_CHAN_NUM);
    frcore_simple_package_chans[0].type = FRC_WORK_UDP;
    frcore_simple_package_chans[1].type = FRC_WORK_RULE;

    FRCORE_INIT("CHAN[%lld] addr=%p\n", (ULL)frcore_simple_package_chans[0].type, &frcore_simple_package_chans[0]);
    FRCORE_INIT("CHAN[%lld] addr=%p\n", (ULL)frcore_simple_package_chans[1].type, &frcore_simple_package_chans[1]);
    printf("Simple packet channel addr=%p, size=0x%llx\n", frcore_simple_package_chans,
           (ULL)(sizeof(frcore_simple_package_chan_t) * FRC_DMA_SIMPLE_PACKAGE_CHAN_NUM));
    frcore_simple_package_fifos = cvmx_bootmem_alloc(sizeof(frcore_simple_package_fifo_t) * FRC_SIMPLE_PACKAGE_FIFO_NUM, 0);
    if (frcore_simple_package_fifos == NULL)
    {
        FRCORE_ERROR("Alloc simple package fifo fail!\n");
        return FRE_MEMORY;
    }
    memset(frcore_simple_package_fifos, 0, sizeof(frcore_simple_package_fifo_t) * FRC_SIMPLE_PACKAGE_FIFO_NUM);
    printf("Simple packet FIFO addr=%p size=0x%llx\n", frcore_simple_package_fifos,
           (ULL)(sizeof(frcore_simple_package_fifo_t) * FRC_SIMPLE_PACKAGE_FIFO_NUM));
    FRCORE_REGISTER_CMD(CMD_TYPE_CTRL, CTRL_CMD_SETUP_SIMPLE_PACKAGE_CHAN,  frcore_cmd_setup_simple_package_chans);
    #if FRC_CONFIG_SIMPLE_PACKET_TEST || FRC_CONFIG_SSN_CHAN_TEST
    FRCORE_REGISTER_CMD(CMD_TYPE_TEST, TEST_CMD_SIMPLE_PACKET_CHAN,  frcore_cmd_setup_simple_package_chans_test);
    #endif

    return 0;
}

frcore_simple_package_chan_t *
frcore_simple_package_chan_get(uint32_t idx)
{
    frcore_simple_package_chan_t *chan;

    chan = &frcore_simple_package_chans[idx];

    return chan;
}

frcore_simple_package_fifo_t *
frcore_simple_package_fifo_get(uint8_t type, uint8_t idx)
{
    frcore_simple_package_fifo_t *fifo;
    fifo = &frcore_simple_package_fifos[type * FRC_SIMPLE_PACKAGE_SIMPLE_CHAN_FIFO_NUM + idx];
    return fifo;
}

int frcore_simple_package_chan_avail_get(frcore_simple_package_chan_t *chan, frcore_simple_package_fifo_t *fifo,
                                          uint64_t number)
{
    uint64_t ridx, widx, offset, len, len1, len2, alen = 0;
    uint8_t *local;
    uint64_t remote_addr;

    FRCORE_SIMPLE_PACKAGE_CHAN_AVAIL_LOCK(chan);
    local = (uint8_t *) &chan->avail_widx;
    remote_addr = chan->ctrl_addr + FRC_DMA_SIMPLE_CHAN_AVAIL_WIDX_OFFSET;
    FRCORE_CYCLE_RECORDING();
    if(frcore_copy_from_remote(sizeof(chan->avail_widx), local, remote_addr)) {
        FRCORE_ERROR("SYNC ridx & widx of avail ring from remote fail!\n");
        FRCORE_STAT_INC(stat_dma_errors);
        FRCORE_SIMPLE_PACKAGE_CHAN_AVAIL_UNLOCK(chan);
        return FRE_DMA;
    }
    FRCORE_CYCLE_RECORDING();
    ridx = chan->avail_ridx;
    widx = chan->avail_widx;

    len = number;

    alen = (widx - ridx);
    if (alen < len)
    {
        FRCORE_CHAN("widx %lld ridx %lld, alen %lld, len %lld.\n",
                     (ULL) widx, (ULL) ridx, (ULL) alen, (ULL) len);
        FRCORE_STAT_INC(stat_avail_get_no_space);
        FRCORE_SIMPLE_PACKAGE_CHAN_AVAIL_UNLOCK(chan);
        return FRE_NOSPACE;
    }

    offset = ridx % FRC_SIMPLE_RING_BUFF_SIZE;

    len1 = (FRC_SIMPLE_RING_BUFF_SIZE - offset) < len ? (FRC_SIMPLE_RING_BUFF_SIZE - offset) : len;

    local = (uint8_t *)fifo->idx_buff;
    remote_addr = chan->ctrl_addr + FRC_DMA_SIMPLE_CHAN_AVAIL_RING_OFFSET + offset * RING_BUFF_CELL_SIZE;

    if(frcore_copy_from_remote(len1 * RING_BUFF_CELL_SIZE, local, remote_addr)) {
        FRCORE_ERROR("Copy avail ring buff from remote fail!\n");
        FRCORE_STAT_INC(stat_dma_errors);
        FRCORE_SIMPLE_PACKAGE_CHAN_AVAIL_UNLOCK(chan);
        return FRE_DMA;
    }
    FRCORE_CYCLE_RECORDING();
    if (len1 < len)
    {
        len2 = len - len1;
        local = (uint8_t *) fifo->idx_buff + len1 * RING_BUFF_CELL_SIZE;
        remote_addr = chan->ctrl_addr + FRC_DMA_SIMPLE_CHAN_AVAIL_RING_OFFSET;

        if(frcore_copy_from_remote(len2* RING_BUFF_CELL_SIZE, local, remote_addr)) {
            FRCORE_ERROR("Copy avail ring buff from remote fail!\n");
            FRCORE_STAT_INC(stat_dma_errors);
            FRCORE_SIMPLE_PACKAGE_CHAN_AVAIL_UNLOCK(chan);
            return FRE_DMA;
        }
    }
    FRCORE_CYCLE_RECORDING();
    ridx += len;
    //FRCORE_STAT_ADD(stat_get_avail_bytes, len);

    chan->avail_ridx = ridx;
    local = (uint8_t *) &chan->avail_ridx;
    remote_addr = chan->ctrl_addr + FRC_DMA_SIMPLE_CHAN_AVAIL_RIDX_OFFSET;
    FRCORE_CYCLE_RECORDING();
    if(frcore_copy_to_remote(sizeof(chan->avail_ridx), local, remote_addr)) {
        FRCORE_ERROR("Copy ridx of avail ring to remote fail!\n");
        FRCORE_STAT_INC(stat_dma_errors);
        FRCORE_SIMPLE_PACKAGE_CHAN_AVAIL_UNLOCK(chan);
        return FRE_DMA;
    }
    FRCORE_CYCLE_RECORDING();
    FRCORE_SIMPLE_PACKAGE_CHAN_AVAIL_UNLOCK(chan);

    return FRE_SUCCESS;
}

int frcore_simple_package_chan_post_pkt(frcore_simple_package_fifo_t *fifo,
                                         uint64_t local_num)
{
    uint64_t cur = 0, cur_offset = 0;

    frcore_dma_scatter_desc_t dma_arg;
    uint64_t remote_addr;
    uint64_t pkt_remain = 0, pkt_posted = 0, pkt_numb = 0, dma_numb = 0, i;

    memset(&dma_arg, 0, sizeof(frcore_dma_scatter_desc_t));

    pkt_remain = local_num;

    while (pkt_remain)
    {
        if (pkt_remain > FRCORE_MAX_DMA_POINTERS)
        {
            pkt_numb = FRCORE_MAX_DMA_POINTERS;
        }
        else
        {
            pkt_numb = pkt_remain;
        }
        dma_numb = 0;
        for (i = 0; i < pkt_numb; i++)
        {
            cur_offset = (fifo->local_ridx + cur ) % FRCORE_CORE_FIFO_NUM;
            remote_addr = fifo->idx_buff[cur];
            dma_arg.remote_addr[dma_numb] = remote_addr + FRC_DMA_OFFSET;
            dma_arg.local_ptr[dma_numb] = (uint8_t *) fifo->fifo_cell[cur_offset].data;
            dma_arg.size[dma_numb] = fifo->fifo_cell[cur_offset].pkt_size;
            FRCORE_STAT_ADD(stat_dma_bytes, (dma_arg.size[dma_numb] - sizeof(frc_dma_hdr_t) - sizeof(frc_dma_pkt_info_t)));
            FRCORE_STAT_INC(stat_dma_pkts);
            FRCORE_STAT_INC(stat_dma_pkt_infos);
            //FRCORE_STAT_ADD(stat_udp_submit_bytes, (dma_arg.size[dma_numb] - sizeof(frc_dma_hdr_t) - sizeof(frc_dma_pkt_info_t)));
            dma_numb++;
            cur++;
        }
        dma_arg.number = dma_numb;

        if (frcore_dma_scatter(DMA_DIR_SEND, &dma_arg))
        {
            FRCORE_ERROR("frcore_dma_scatter DMA_DIR_SEND fail!\n");
            FRCORE_STAT_INC(stat_dma_errors);
            goto frcore_queue_post_pkt_err;
        }
        else
        {
            FRCORE_STAT_ADD(stat_dma_blocks, dma_numb);
            //FRCORE_STAT_ADD(stat_udp_submit_pkts, dma_numb);
        }
        FRCORE_CYCLE_RECORDING();
        pkt_remain -= pkt_numb;
        pkt_posted += pkt_numb;
    }

frcore_queue_post_pkt_err:
    return pkt_posted;
}


int frcore_simple_package_chan_compl_put(frcore_simple_package_chan_t *chan, frcore_simple_package_fifo_t *fifo,
                                          uint64_t number)
{
    uint64_t ridx, widx, offset, len, len1, len2;
    uint8_t *local;
    uint64_t remote_addr;

    FRCORE_SIMPLE_PACKAGE_CHAN_COMPL_LOCK(chan);
    local = (uint8_t *) &chan->compl_ridx;
    remote_addr = chan->ctrl_addr + FRC_DMA_SIMPLE_CHAN_COMPL_RIDX_OFFSET;

    if (frcore_copy_from_remote(sizeof(chan->compl_ridx), local, remote_addr)) {
        FRCORE_ERROR("SYNC ridx & widx of compl ring from remote fail!\n");
        FRCORE_STAT_INC(stat_dma_errors);
        FRCORE_SIMPLE_PACKAGE_CHAN_COMPL_UNLOCK(chan);
        return FRE_DMA;
    }

    ridx = chan->compl_ridx;
    widx = chan->compl_widx;

    len = number;

    if ((widx - ridx) > (FRC_SIMPLE_RING_BUFF_SIZE - len))
    {
        FRCORE_STAT_INC(stat_compl_put_no_space);
        FRCORE_SIMPLE_PACKAGE_CHAN_COMPL_UNLOCK(chan);
        return FRE_NOSPACE;
    }

    offset = widx % FRC_SIMPLE_RING_BUFF_SIZE;

    len1 = (FRC_SIMPLE_RING_BUFF_SIZE - offset) < len ? (FRC_SIMPLE_RING_BUFF_SIZE - offset) : len;

    local = (uint8_t *) fifo->idx_buff;
    remote_addr = chan->ctrl_addr + FRC_DMA_SIMPLE_CHAN_COMPL_RING_OFFSET + offset * RING_BUFF_CELL_SIZE;

    if (frcore_copy_to_remote(len1*RING_BUFF_CELL_SIZE, local, remote_addr)) {
        FRCORE_ERROR("Copy compl ring buff from remote fail!\n");
        FRCORE_STAT_INC(stat_dma_errors);
        FRCORE_SIMPLE_PACKAGE_CHAN_COMPL_UNLOCK(chan);
        return FRE_DMA;
    }

    if (len1 < len)
    {
        len2 = len - len1;
        local = (uint8_t *) fifo->idx_buff + len1*RING_BUFF_CELL_SIZE;
        remote_addr = chan->ctrl_addr + FRC_DMA_SIMPLE_CHAN_COMPL_RING_OFFSET;

        if(frcore_copy_to_remote(len2*RING_BUFF_CELL_SIZE, local, remote_addr)) {
            FRCORE_ERROR("Copy compl ring buff from remote fail!\n");
            FRCORE_STAT_INC(stat_dma_errors);
            FRCORE_SIMPLE_PACKAGE_CHAN_COMPL_UNLOCK(chan);
            return FRE_DMA;
        }
    }

    widx += len;
    //FRCORE_STAT_ADD(stat_put_compl_bytes, len);

    chan->compl_widx = widx;
    local = (uint8_t *) &chan->compl_widx;
    remote_addr = chan->ctrl_addr + FRC_DMA_SIMPLE_CHAN_COMPL_WIDX_OFFSET;

    if (frcore_copy_to_remote(sizeof(chan->compl_widx), local, remote_addr)) {
        FRCORE_ERROR("Copy ridx of compl ring to remote fail!\n");
        FRCORE_STAT_INC(stat_dma_errors);
        FRCORE_SIMPLE_PACKAGE_CHAN_COMPL_UNLOCK(chan);
        return FRE_DMA;
    }
    FRCORE_SIMPLE_PACKAGE_CHAN_COMPL_UNLOCK(chan);
    return FRE_SUCCESS;
}


/*
 * flag  for 1 ,submit 14x16
 *       for 0, submit free(> 0)
 */
void frcore_simple_package_fifo_post(frcore_simple_package_chan_t *chan, frcore_simple_package_fifo_t *fifo,
                                     uint32_t core_num, uint8_t flag)
{
    int rv;
    //uint64_t spend_cycle;
    uint64_t local_widx, local_ridx, local_num, posted_num, post_num;

    local_widx = fifo->local_widx;
    local_ridx = fifo->local_ridx;
    local_num  = local_widx - local_ridx;

    if (flag)
    {
        if (local_num != FRCORE_CHAN_POST_LWM)
        {
            return;
        }
    }else {
        if (local_num == 0)
        {
            return;
        }
    }
    #if 0
    spend_cycle = cvmx_get_cycle() - fifo->post_cycle;

    if ((spend_cycle  < FRCORE_CHAN_POST_PERIOD) && (local_num < FRCORE_CHAN_POST_LWM))
    {
        return;
    }
    #endif


    FRCORE_CYCLE_RECORDER_INIT();
    while (local_num)
    {

        if (local_num < FRCORE_CHAN_POST_LWM)
        {
            post_num = local_num;
        }
        else
        {
            post_num = FRCORE_CHAN_POST_LWM;
        }

        FRCORE_CYCLE_RECORDING();
        if (udp_block_exhaust[core_num]) {
            if (cvmx_get_cycle() - udp_last_display[core_num]
                < (cpu_freq / DMA_CHANNEL_REST_DIV)) {
                break;
            }
        }
        rv = frcore_simple_package_chan_avail_get(chan, fifo, post_num);
        FRCORE_CHAN("CHAN %lld: frcore_simple_package_chan_avail_get return %d, post_num %lld..\n",
                     (ULL)chan->type, rv, (ULL) post_num);
        if (FRE_NOSPACE == rv)
        {
            udp_block_exhaust[core_num] = 1;
            udp_last_display[core_num] = cvmx_get_cycle();
            break;
        }
        else if (rv)
        {
            FRCORE_STAT_INC(stat_avail_get_err);
            goto frcore_queue_post_err;
        }
        else
        {
            udp_block_exhaust[core_num] = 0;
            FRCORE_STAT_INC(stat_avail_get);
        }
        FRCORE_CYCLE_RECORDING();

        /* Trans dma pkts to x86 */
        posted_num = frcore_simple_package_chan_post_pkt(fifo, post_num);

        FRCORE_QUEUE("CHAN %lld: frcore_queue_post_pkt posted_num %lld, post_num %lld.\n",
                     (ULL)chan->type, (ULL) posted_num, (ULL) post_num);
        if (posted_num < post_num)
        {
            FRCORE_STAT_INC(stat_pkt_post_err);
            FRCORE_STAT_ADD(stat_post_err_pkts, (post_num - posted_num));
        }
        else
        {
            FRCORE_STAT_INC(stat_pkt_post);
        }

        FRCORE_CYCLE_RECORDING();

        rv = frcore_simple_package_chan_compl_put(chan, fifo,  post_num);
        FRCORE_CHAN("CHAN %lld: frcore_simple_package_chan_compl_put return %d, post_num %lld.\n",
                     (ULL)chan->type, rv, (ULL) post_num);
        if (rv) {
            FRCORE_STAT_INC(stat_compl_put_err);
            goto frcore_queue_post_err;
        } else {
            FRCORE_STAT_INC(stat_compl_put);
        }
        fifo->local_ridx += post_num;
        fifo->post_cycle = cvmx_get_cycle();
        local_num -= post_num;
    }
    FRCORE_CYCLE_RECORDING();

frcore_queue_post_err:
    FRCORE_CYCLE_RECORD_DUMP();
}


int frcore_simple_package_fifo_add(frcore_simple_package_fifo_t *fifo, frc_dma_hdr_t *hdr,
                                   frc_dma_pkt_info_t *info, void *payload)
{
    int rv = 0;
    uint8_t *buff = NULL;
    int hdr_size, info_size, paylen;
    uint64_t pkt_len;
    uint64_t idx_offset = 0;

    hdr_size = sizeof(frc_dma_hdr_t);
    info_size = sizeof(frc_dma_pkt_info_t);
    //paylen = info->payload_len;
    paylen = hdr->total_paylen;

    pkt_len = hdr_size + info_size + paylen;
    if (pkt_len > 2048)
    {
        FRCORE_ERROR("pkt_len %lld > 2048, hdr_size %d, info_size %d, paylen %d!\n",
                     (ULL) pkt_len, hdr_size, info_size, paylen);
        FRCORE_STAT_INC(stat_pkt_oversize);
        return FRE_EXCEED;
    }


    if ((fifo->local_widx - fifo->local_ridx) >= FRCORE_CORE_FIFO_NUM)
    {
        rv = FRE_NOSPACE;
        FRCORE_STAT_INC(stat_queue_no_space);
        goto frcore_simple_package_fifo_add_err;
    }

    idx_offset = fifo->local_widx % FRCORE_CORE_FIFO_NUM;

    fifo->fifo_cell[idx_offset].pkt_size = pkt_len;
    buff = fifo->fifo_cell[idx_offset].data;

    FRCORE_CYCLE_RECORDING();
    memcpy(buff, hdr, hdr_size);
    memcpy(buff+hdr_size, payload, paylen);
    memcpy(buff+hdr_size+paylen, info, info_size);
    FRCORE_CYCLE_RECORDING();

    fifo->local_widx++;

frcore_simple_package_fifo_add_err:

    return rv;
}


int frcore_forward_simple_pkt_to_fifo(uint32_t type, frc_dma_hdr_t *hdr, frc_dma_pkt_info_t *info, void *payload)
{
    #if FRC_CONFIG_UDP_CLOSE_SUBMIT_DATA
    return FRE_SUCCESS;
    #else
    int num, rv;
    frcore_simple_package_chan_t *chan = NULL;
    frcore_simple_package_fifo_t *fifo;
    uint32_t fifo_id = 0;

    chan = frcore_simple_package_chan_get(type);
    if (chan == NULL)
    {
        FRCORE_ERROR("Invaild chan_id %d.\n", type);
        return FRE_INIT;
    }

    fifo_id = cvmx_get_core_num();
    fifo = frcore_simple_package_fifo_get(type, fifo_id);
    if (fifo == NULL)
    {
        FRCORE_ERROR("Invaild type %d fifo_id %d.\n",  type, fifo_id);
        return FRE_INIT;
    }

    /* if fifo is full, submit fifo */
    frcore_simple_package_fifo_post(chan, fifo, fifo_id, 1);

    FRCORE_CYCLE_RECORDING();
    num = frcore_simple_package_fifo_add(fifo, hdr, info, payload);
    if (num != 0)
    {
        FRCORE_ERROR("Simple Package fifo exceed!\n");
        FRCORE_STAT_INC(stat_dma_enqueue_errs);
        rv = FRE_EXCEED;
        goto frcore_forward_pkt_to_host_err;
    }
    else
    {
        FRCORE_STAT_INC(stat_dma_enqueue_pkts);
    }

    rv = FRE_SUCCESS;

frcore_forward_pkt_to_host_err:
    return rv;
    #endif
}

#if FRC_CONFIG_RULE

/*
 * flag  for 1 ,submit 14x16
 *       for 0, submit free(> 0)
 */
void frcore_rule_fifo_post(frcore_simple_package_chan_t *chan, frcore_simple_package_fifo_t *fifo,
                           uint32_t core_num, uint8_t flag)
{
    int rv;
    //uint64_t spend_cycle;
    uint64_t local_widx, local_ridx, local_num, posted_num, post_num;

    local_widx = fifo->local_widx;
    local_ridx = fifo->local_ridx;
    local_num  = local_widx - local_ridx;

    if (flag)
    {
        if (local_num != FRCORE_CHAN_POST_LWM)
        {
            return;
        }
    }else {
        if (local_num == 0)
        {
            return;
        }
    }
    #if 0
    spend_cycle = cvmx_get_cycle() - fifo->post_cycle;

    if ((spend_cycle  < FRCORE_CHAN_POST_PERIOD) && (local_num < FRCORE_CHAN_POST_LWM))
    {
        return;
    }
    #endif


    FRCORE_CYCLE_RECORDER_INIT();
    while (local_num)
    {

        if (local_num < FRCORE_CHAN_POST_LWM)
        {
            post_num = local_num;
        }
        else
        {
            post_num = FRCORE_CHAN_POST_LWM;
        }

        FRCORE_CYCLE_RECORDING();
        if (rule_block_exhaust[core_num]) {
            if (cvmx_get_cycle() - rule_last_display[core_num]
                < (cpu_freq / DMA_CHANNEL_REST_DIV)) {
                break;
            }
        }
        rv = frcore_simple_package_chan_avail_get(chan, fifo, post_num);
        FRCORE_CHAN("CHAN %lld: frcore_simple_package_chan_avail_get return %d, post_num %lld..\n",
                     (ULL)chan->type, rv, (ULL) post_num);
        if (FRE_NOSPACE == rv)
        {
            rule_block_exhaust[core_num] = 1;
            rule_last_display[core_num] = cvmx_get_cycle();
            break;
        }
        else if (rv)
        {
            FRCORE_STAT_INC(stat_avail_get_err);
            goto frcore_queue_post_err;
        }
        else
        {
            rule_block_exhaust[core_num] = 0;
            FRCORE_STAT_INC(stat_avail_get);
        }
        FRCORE_CYCLE_RECORDING();

        /* Trans dma pkts to x86 */
        posted_num = frcore_simple_package_chan_post_pkt(fifo, post_num);

        FRCORE_QUEUE("CHAN %lld: frcore_queue_post_pkt posted_num %lld, post_num %lld.\n",
                     (ULL)chan->type, (ULL) posted_num, (ULL) post_num);
        if (posted_num < post_num)
        {
            FRCORE_STAT_INC(stat_pkt_post_err);
            FRCORE_STAT_ADD(stat_post_err_pkts, (post_num - posted_num));
        }
        else
        {
            FRCORE_STAT_INC(stat_pkt_post);
        }

        FRCORE_CYCLE_RECORDING();

        rv = frcore_simple_package_chan_compl_put(chan, fifo,  post_num);
        FRCORE_CHAN("CHAN %lld: frcore_simple_package_chan_compl_put return %d, post_num %lld.\n",
                     (ULL)chan->type, rv, (ULL) post_num);
        if (rv) {
            FRCORE_STAT_INC(stat_compl_put_err);
            goto frcore_queue_post_err;
        } else {
            FRCORE_STAT_INC(stat_compl_put);
        }
        fifo->local_ridx += post_num;
        fifo->post_cycle = cvmx_get_cycle();
        local_num -= post_num;
    }
    FRCORE_CYCLE_RECORDING();

frcore_queue_post_err:
    FRCORE_CYCLE_RECORD_DUMP();
}

int frcore_rule_packet_fifo_add(frcore_simple_package_fifo_t *fifo, frc_dma_hdr_t *hdr, frc_dma_pkt_info_t *info, void *payload, uint16_t *tagid)
{
    int rv = 0;
    uint8_t *buff = NULL;
    int hdr_size, info_size, paylen;
    uint64_t pkt_len;
    uint64_t idx_offset = 0;

    hdr_size = sizeof(frc_dma_hdr_t);
    info_size = sizeof(frc_dma_pkt_info_t);
    //paylen = info->payload_len;
    paylen = hdr->total_paylen;

    pkt_len = hdr_size + info_size + paylen +2; /* 2 bytes rule id */
    if (pkt_len > 2048)
    {
        FRCORE_ERROR("pkt_len %lld > 2048, hdr_size %d, info_size %d, paylen %d!\n",
                     (ULL) pkt_len, hdr_size, info_size, paylen);
        FRCORE_STAT_INC(stat_pkt_oversize);
        return FRE_EXCEED;
    }


    if ((fifo->local_widx - fifo->local_ridx) >= FRCORE_CORE_FIFO_NUM)
    {
        rv = FRE_NOSPACE;
        FRCORE_STAT_INC(stat_queue_no_space);
        goto frcore_simple_package_fifo_add_err;
    }

    idx_offset = fifo->local_widx % FRCORE_CORE_FIFO_NUM;

    fifo->fifo_cell[idx_offset].pkt_size = pkt_len;
    buff = fifo->fifo_cell[idx_offset].data;

    FRCORE_CYCLE_RECORDING();
    memcpy(buff, hdr, hdr_size);
    memcpy(buff+hdr_size, tagid, 2);
    memcpy(buff+hdr_size+2, payload, paylen);
    memcpy(buff+hdr_size+2+paylen, info, info_size);
    FRCORE_CYCLE_RECORDING();

    fifo->local_widx++;

frcore_simple_package_fifo_add_err:

    return rv;
}

int frcore_forward_rule_pkt_to_fifo(uint16_t *tagid, frc_dma_hdr_t *hdr, frc_dma_pkt_info_t *info, void *payload)
{
    int num, rv;
    frcore_simple_package_chan_t *chan = NULL;
    frcore_simple_package_fifo_t *fifo;
    uint32_t fifo_id = 0;
    uint32_t type = FRC_WORK_RULE;

    chan = frcore_simple_package_chan_get(type);
    if (chan == NULL)
    {
        FRCORE_ERROR("Invaild chan_id %d.\n", type);
        return FRE_INIT;
    }
    fifo_id = cvmx_get_core_num();
    fifo = frcore_simple_package_fifo_get(type, fifo_id);
    if (fifo == NULL)
    {
        FRCORE_ERROR("Invaild type %d fifo_id %d.\n",  type, fifo_id);
        return FRE_INIT;
    }
    /* if fifo is full, submit fifo */
    frcore_rule_fifo_post(chan, fifo, fifo_id, 1);

    FRCORE_CYCLE_RECORDING();
    num = frcore_rule_packet_fifo_add(fifo, hdr, info, payload, tagid);
    if (num != 0)
    {
        FRCORE_ERROR("Simple Package fifo exceed!\n");
        FRCORE_STAT_INC(stat_dma_enqueue_errs);
        rv = FRE_EXCEED;
        goto frcore_forward_pkt_to_host_err;
    }
    else
    {
        FRCORE_STAT_INC(stat_dma_enqueue_pkts);
    }

    rv = FRE_SUCCESS;

frcore_forward_pkt_to_host_err:
    return rv;
}
#endif

#endif


#if FRC_CONFIG_SSN_CHAN
CVMX_SHARED frcore_ssn_chan_t *frcore_ssn_chans = NULL;
CVMX_SHARED frcore_ssn_fifo_t *frcore_ssn_fifos = NULL;

#if 1
#define FRCORE_SSN_CHAN_AVAIL_LOCK(_chan)     cvmx_spinlock_lock(&((_chan)->avail_lock))
#define FRCORE_SSN_CHAN_AVAIL_UNLOCK(_chan)   cvmx_spinlock_unlock(&((_chan)->avail_lock))
#define FRCORE_SSN_CHAN_COMPL_LOCK(_chan)     cvmx_spinlock_lock(&((_chan)->compl_lock))
#define FRCORE_SSN_CHAN_COMPL_UNLOCK(_chan)   cvmx_spinlock_unlock(&((_chan)->compl_lock))
#define FRCORE_SSN_CHAN_FIFO_LOCK(_fifo)      cvmx_spinlock_lock(&((_fifo)->lock));\
                                              FRCORE_STAT_INC(stat_ssn_fifo_lock)
#define FRCORE_SSN_CHAN_FIFO_UNLOCK(_fifo)    cvmx_spinlock_unlock(&((_fifo)->lock));\
                                              FRCORE_STAT_INC(stat_ssn_fifo_unlock)
#else
#define FRCORE_SSN_CHAN_AVAIL_LOCK(_chan)
#define FRCORE_SSN_CHAN_AVAIL_UNLOCK(_chan)
#define FRCORE_SSN_CHAN_COMPL_LOCK(_chan)
#define FRCORE_SSN_CHAN_COMPL_UNLOCK(_chan)
#endif

int frcore_cmd_setup_ssn_chans(uint16_t plen, void *param, uint16_t *olen, void *outbuf)
{
    int i;
    frcore_ssn_chan_t *chan = NULL;
    frcore_ssn_fifo_t *fifo = NULL;
    frc_dma_ssn_chan_desc_t *desc;
    *olen = 0;

    desc = (frc_dma_ssn_chan_desc_t *) param;

    FRCORE_CMD("chan_id %lld, avail_size %llx, avail_addr 0x%llx, compl_size %llx,compl_addr 0x%llx.\n",
               (ULL) desc->type, (ULL) desc->avail_size, (ULL) desc->avail_addr,
               (ULL) desc->compl_size, (ULL) desc->compl_addr);

    if (frcore_ssn_chans == NULL) {
        FRCORE_ERROR("\n");
        return FRE_INIT;
    }

    if (desc->type > FRC_WORK_SSN) {
        FRCORE_ERROR("desc->type(%lld) > FRC_WORK_SSN(%d)\n", (ULL) desc->type, FRC_WORK_SSN);
        return FRE_EXCEED;
    }

    if (desc->type != FRC_WORK_SSN)
    {
        FRCORE_ERROR("Unkown type %lld!\n", (ULL)desc->type);
        return FRE_PARAM;
    }

    chan = &frcore_ssn_chans[desc->type - FRC_WORK_SSN];

    /* setup ctrl ring */
    frcore_dma_scatter_desc_t dma_arg;

    memset(&dma_arg, 0, sizeof(frcore_dma_scatter_desc_t));

    dma_arg.local_ptr[0] = (uint8_t *) &chan->avail_widx;
    dma_arg.remote_addr[0]  = desc->avail_addr + FRC_DMA_SSN_AVAIL_WIDX_OFFSET;
    dma_arg.size[0] = sizeof(chan->avail_widx);

    dma_arg.local_ptr[1] = (uint8_t *) &chan->avail_ridx;
    dma_arg.remote_addr[1]  = desc->avail_addr + FRC_DMA_SSN_AVAIL_RIDX_OFFSET;
    dma_arg.size[1] = sizeof(chan->avail_ridx);

    dma_arg.local_ptr[2] = (uint8_t *) &chan->compl_widx;
    dma_arg.remote_addr[2]  = desc->compl_addr + FRC_DMA_SSN_COMPL_WIDX_OFFSET;
    dma_arg.size[2] = sizeof(chan->compl_widx);

    dma_arg.local_ptr[3] = (uint8_t *) &chan->compl_ridx;
    dma_arg.remote_addr[3]  = desc->compl_addr + FRC_DMA_SSN_COMPL_RIDX_OFFSET;
    dma_arg.size[3] = sizeof(chan->compl_ridx);
    dma_arg.number = 4;

    if( frcore_dma_scatter(DMA_DIR_RECV, &dma_arg)) {
        FRCORE_ERROR("Copy ctrl arg from remote fail!\n");
        FRCORE_STAT_INC(stat_dma_errors);
        return FRE_DMA;
    }
    chan->avail_addr = desc->avail_addr;
    chan->compl_addr = desc->compl_addr;
    chan->type = desc->type;

    FRCORE_CMD("CHAN %lld(%s): AVAIL_ADDR 0x%llx;COMPL_ADDR 0x%llx; AVAIL RIDX 0x%llX, WIDX 0x%llX; COMPL RIDX 0x%llX, WIDX 0x%llX.\n",
               (ULL) chan->type, (ULL) (chan->type == FRC_WORK_SSN) ? "SSN" : "OTHERS", (ULL) chan->avail_addr,(ULL) chan->compl_addr,
               (ULL) (chan->avail_ridx), (ULL) (chan->avail_widx), (ULL) (chan->compl_ridx), (ULL) (chan->compl_widx));

    /* Init chan lock*/
    cvmx_spinlock_init(&chan->avail_lock);
    cvmx_spinlock_init(&chan->compl_lock);
    fifo = &frcore_ssn_fifos[(desc->type - FRC_WORK_SSN) * FRC_SSN_SIMPLE_CHAN_FIFO_NUM];
    FRCORE_CMD("fifo=%p\n", fifo);
    /* Init fifo timer */
   for (i = 0; i < FRC_SSN_FIFO_NUM; i++)
    {
        cvmx_spinlock_init(&fifo[i].lock);
        cvmx_wqe_t *wqe = &fifo[i].wqe;
        memset(wqe, 0, sizeof(cvmx_wqe_t));
        wqe->unused = FRCORE_WORK_CHAN;
        wqe->ipprt  = 0;
        wqe->grp    = i % FRC_DAT_CORE_NUM + 1;

        wqe->tag      = wqe->grp;
        wqe->tag_type = CVMX_POW_TAG_TYPE_ATOMIC;
        wqe->packet_data[0] = (uint8_t)desc->type; /* dup,rule or ssn */
        wqe->packet_data[1] = i;  /* fifo id */
        if (cvmx_tim_add_entry(wqe, FRCORE_CHAN_TIMEOUT, NULL) != CVMX_TIM_STATUS_SUCCESS) {
            FRCORE_ERROR("dma chan fifo timer add fail!\n");
            return FRE_FAIL;
        }

    }

    return FRE_SUCCESS;
}

int frcore_ssn_chan_init()
{
    frcore_ssn_chans = cvmx_bootmem_alloc(sizeof(frcore_ssn_chan_t) * FRC_DMA_SSN_CHAN_NUM, 0);
    if (frcore_ssn_chans == NULL)
    {
        FRCORE_ERROR("Alloc ssn dma chan fail!\n");
        return FRE_MEMORY;
    }

    memset(frcore_ssn_chans, 0, sizeof(frcore_ssn_chan_t) * FRC_DMA_SSN_CHAN_NUM);
    frcore_ssn_chans[0].type = FRC_WORK_SSN;

    printf("CHAN[%lld] addr=%p size=0x%x\n", (ULL)frcore_ssn_chans[0].type, &frcore_ssn_chans[0],
           (unsigned int)(sizeof(frcore_ssn_chan_t) * FRC_DMA_SSN_CHAN_NUM));
    frcore_ssn_fifos = cvmx_bootmem_alloc(sizeof(frcore_ssn_fifo_t) * FRC_SSN_FIFO_NUM, 0);
    if (frcore_ssn_fifos == NULL)
    {
        FRCORE_ERROR("Alloc ssn fifo fail!\n");
        return FRE_MEMORY;
    }
    memset(frcore_ssn_fifos, 0, sizeof(frcore_ssn_fifo_t) * FRC_SSN_FIFO_NUM);
    printf("SSN FIFO addr=%p size=0x%llx\n", frcore_ssn_fifos,
           (ULL)(sizeof(frcore_ssn_fifo_t) * FRC_SSN_FIFO_NUM));
    FRCORE_REGISTER_CMD(CMD_TYPE_CTRL, CTRL_CMD_SETUP_SSN_CHAN, frcore_cmd_setup_ssn_chans);
    return 0;
}

frcore_ssn_chan_t *
frcore_ssn_chan_get(uint32_t type)
{
    frcore_ssn_chan_t *chan;

    chan = &frcore_ssn_chans[type-FRC_WORK_SSN];

    return chan;
}

frcore_ssn_fifo_t *
frcore_ssn_fifo_get(uint8_t type, uint8_t idx)
{
    frcore_ssn_fifo_t *fifo;
    fifo = &frcore_ssn_fifos[(type-FRC_WORK_SSN) * FRC_SSN_SIMPLE_CHAN_FIFO_NUM + idx];
    return fifo;
}
#if FRC_CONFIG_SSN_AVAIL_BUFF_GET
int frcore_ssn_chan_avail_get(frcore_ssn_chan_t *chan, uint64_t number, uint64_t *buff)
{
    uint64_t ridx, widx, offset, len, len1, len2, alen = 0;
    uint8_t *local;
    uint64_t remote_addr;
    FRCORE_CHAN("chan->avail_widx=%lld, chan->avail_ridx=%lld\n", (ULL)chan->avail_widx, (ULL)chan->avail_ridx);
    FRCORE_SSN_CHAN_AVAIL_LOCK(chan);
    local = (uint8_t *) &chan->avail_widx;
    remote_addr = chan->avail_addr + FRC_DMA_SSN_AVAIL_WIDX_OFFSET;
    //FRCORE_CYCLE_RECORDING();
    if(frcore_copy_from_remote(sizeof(chan->avail_widx), local, remote_addr)) {
        FRCORE_ERROR("SYNC ridx & widx of avail ring from remote fail!\n");
        FRCORE_STAT_INC(stat_ssn_dma_errors);
        FRCORE_SSN_CHAN_AVAIL_UNLOCK(chan);
        return FRE_DMA;
    }
    //FRCORE_CYCLE_RECORDING();
    ridx = chan->avail_ridx;
    widx = chan->avail_widx;

    len = number;

    alen = (widx - ridx);
    if (alen < len)
    {
        FRCORE_CHAN("widx %lld ridx %lld, alen %lld, len %lld.\n",
                     (ULL) widx, (ULL) ridx, (ULL) alen, (ULL) len);
        FRCORE_STAT_INC(stat_ssn_avail_get_no_space);
        FRCORE_SSN_CHAN_AVAIL_UNLOCK(chan);
        return FRE_NOSPACE;
    }

    offset = ridx % FRC_DMA_SSN_RING_BUFF_SIZE;

    len1 = (FRC_DMA_SSN_RING_BUFF_SIZE - offset) < len ? (FRC_DMA_SSN_RING_BUFF_SIZE - offset) : len;

    local = (uint8_t *)chan->avail_buff;
    remote_addr = chan->avail_addr + FRC_DMA_SSN_AVAIL_RING_OFFSET + offset * RING_BUFF_CELL_SIZE;

    if(frcore_copy_from_remote(len1 * RING_BUFF_CELL_SIZE, local, remote_addr)) {
        FRCORE_ERROR("Copy avail ring buff from remote fail!\n");
        FRCORE_STAT_INC(stat_ssn_dma_errors);
        FRCORE_SSN_CHAN_AVAIL_UNLOCK(chan);
        return FRE_DMA;
    }
    //FRCORE_CYCLE_RECORDING();
    if (len1 < len)
    {
        len2 = len - len1;
        local = (uint8_t *) chan->avail_buff + len1 * RING_BUFF_CELL_SIZE;
        remote_addr = chan->avail_addr + FRC_DMA_SSN_AVAIL_RING_OFFSET;

        if(frcore_copy_from_remote(len2* RING_BUFF_CELL_SIZE, local, remote_addr)) {
            FRCORE_ERROR("Copy avail ring buff from remote fail!\n");
            FRCORE_STAT_INC(stat_ssn_dma_errors);
            FRCORE_SSN_CHAN_AVAIL_UNLOCK(chan);
            return FRE_DMA;
        }
    }
    //FRCORE_CYCLE_RECORDING();
    ridx += len;
    FRCORE_STAT_ADD(stat_ssn_avail_get, len);

    chan->avail_ridx = ridx;
    local = (uint8_t *) &chan->avail_ridx;
    remote_addr = chan->avail_addr + FRC_DMA_SSN_AVAIL_RIDX_OFFSET;
    //FRCORE_CYCLE_RECORDING();
    if(frcore_copy_to_remote(sizeof(chan->avail_ridx), local, remote_addr)) {
        FRCORE_ERROR("Copy ridx of avail ring to remote fail!\n");
        FRCORE_STAT_INC(stat_ssn_dma_errors);
        FRCORE_SSN_CHAN_AVAIL_UNLOCK(chan);
        return FRE_DMA;
    }
    //FRCORE_CYCLE_RECORDING();
    memcpy(buff, chan->avail_buff, len* RING_BUFF_CELL_SIZE);
    FRCORE_SSN_CHAN_AVAIL_UNLOCK(chan);
    return FRE_SUCCESS;
}

int frcore_ssn_get_one_block_addr(uint64_t *buff, uint32_t fifo_id)
{
    int rv;
    frcore_ssn_chan_t *chan = NULL;
    frcore_ssn_fifo_t *fifo = NULL;
    uint64_t block_addr = 0;
    uint32_t type = FRC_WORK_SSN;
    //uint32_t fifo_id = 0;
    //fifo_id = cvmx_get_core_num();

    chan = frcore_ssn_chan_get(type);
    if (chan == NULL)
    {
        FRCORE_ERROR("Invaild type %d.\n",  type);
        return FRE_INIT;
    }
    fifo = frcore_ssn_fifo_get(type, fifo_id);
    if (fifo == NULL)
    {
        FRCORE_ERROR("Invaild type %d fifo_id %d.\n",  type, fifo_id);
        return FRE_INIT;
    }

    FRCORE_SSN_CHAN_FIFO_LOCK(fifo);
    if (fifo->avail_rdx <= 0)
    {
        //printf("%s %d\n", __func__, __LINE__);
        /* no block addr space, so first request addr space */
        /* get block addr */
        if (frcore_ssn_chan_avail_get(chan, FRCORE_SSN_AVAIL_BUFF_GET_SIZE, fifo->avail_buff) == FRE_SUCCESS)
        {
            fifo->avail_rdx = FRCORE_SSN_AVAIL_BUFF_GET_SIZE;
        }else if (frcore_ssn_chan_avail_get(chan, 100, fifo->avail_buff) == FRE_SUCCESS)
        {
            fifo->avail_rdx = 100;
        }else if (frcore_ssn_chan_avail_get(chan, 1, fifo->avail_buff) == FRE_SUCCESS)
        {
            fifo->avail_rdx = 1;
        }else{
            FRCORE_ERROR("frcore_ssn_chan_avail_get failed!\n");
            rv =  FRE_NOSPACE;
            goto frcore_ssn_get_one_block_addr_err;
        }
    }
    /* have block addr space */
    block_addr = fifo->avail_buff[fifo->avail_rdx-1];
    fifo->avail_rdx -= 1;
    CVMX_SYNCWS;

    *buff = block_addr;
    rv = FRE_SUCCESS;

    frcore_ssn_get_one_block_addr_err:
    FRCORE_SSN_CHAN_FIFO_UNLOCK(fifo);
    return rv;
}
#else
int frcore_ssn_chan_avail_get(frcore_ssn_chan_t *chan, uint64_t number, uint64_t *buff)
{
    uint64_t ridx, widx, offset, len, len1, len2, alen = 0;
    uint8_t *local;
    uint64_t remote_addr;
    FRCORE_CHAN("chan->avail_widx=%lld, chan->avail_ridx=%lld\n", (ULL)chan->avail_widx, (ULL)chan->avail_ridx);
    FRCORE_SSN_CHAN_AVAIL_LOCK(chan);
    local = (uint8_t *) &chan->avail_widx;
    remote_addr = chan->avail_addr + FRC_DMA_SSN_AVAIL_WIDX_OFFSET;
    //FRCORE_CYCLE_RECORDING();
    if(frcore_copy_from_remote(sizeof(chan->avail_widx), local, remote_addr)) {
        FRCORE_ERROR("SYNC ridx & widx of avail ring from remote fail!\n");
        FRCORE_STAT_INC(stat_ssn_dma_errors);
        FRCORE_SSN_CHAN_AVAIL_UNLOCK(chan);
        return FRE_DMA;
    }
    //FRCORE_CYCLE_RECORDING();
    ridx = chan->avail_ridx;
    widx = chan->avail_widx;

    len = number;

    alen = (widx - ridx);
    if (alen < len)
    {
        FRCORE_CHAN("widx %lld ridx %lld, alen %lld, len %lld.\n",
                     (ULL) widx, (ULL) ridx, (ULL) alen, (ULL) len);
        FRCORE_STAT_INC(stat_ssn_avail_get_no_space);
        FRCORE_SSN_CHAN_AVAIL_UNLOCK(chan);
        return FRE_NOSPACE;
    }

    offset = ridx % FRC_DMA_SSN_RING_BUFF_SIZE;

    len1 = (FRC_DMA_SSN_RING_BUFF_SIZE - offset) < len ? (FRC_DMA_SSN_RING_BUFF_SIZE - offset) : len;

    local = (uint8_t *)chan->avail_buff;
    remote_addr = chan->avail_addr + FRC_DMA_SSN_AVAIL_RING_OFFSET + offset * RING_BUFF_CELL_SIZE;

    if(frcore_copy_from_remote(len1 * RING_BUFF_CELL_SIZE, local, remote_addr)) {
        FRCORE_ERROR("Copy avail ring buff from remote fail!\n");
        FRCORE_STAT_INC(stat_ssn_dma_errors);
        FRCORE_SSN_CHAN_AVAIL_UNLOCK(chan);
        return FRE_DMA;
    }
    //FRCORE_CYCLE_RECORDING();
    if (len1 < len)
    {
        len2 = len - len1;
        local = (uint8_t *) chan->avail_buff + len1 * RING_BUFF_CELL_SIZE;
        remote_addr = chan->avail_addr + FRC_DMA_SSN_AVAIL_RING_OFFSET;

        if(frcore_copy_from_remote(len2* RING_BUFF_CELL_SIZE, local, remote_addr)) {
            FRCORE_ERROR("Copy avail ring buff from remote fail!\n");
            FRCORE_STAT_INC(stat_ssn_dma_errors);
            FRCORE_SSN_CHAN_AVAIL_UNLOCK(chan);
            return FRE_DMA;
        }
    }
    //FRCORE_CYCLE_RECORDING();
    ridx += len;
    FRCORE_STAT_ADD(stat_ssn_avail_get, len);

    chan->avail_ridx = ridx;
    local = (uint8_t *) &chan->avail_ridx;
    remote_addr = chan->avail_addr + FRC_DMA_SSN_AVAIL_RIDX_OFFSET;
    //FRCORE_CYCLE_RECORDING();
    if(frcore_copy_to_remote(sizeof(chan->avail_ridx), local, remote_addr)) {
        FRCORE_ERROR("Copy ridx of avail ring to remote fail!\n");
        FRCORE_STAT_INC(stat_ssn_dma_errors);
        FRCORE_SSN_CHAN_AVAIL_UNLOCK(chan);
        return FRE_DMA;
    }
    //FRCORE_CYCLE_RECORDING();
    FRCORE_SSN_CHAN_AVAIL_UNLOCK(chan);
    memcpy(buff, chan->avail_buff, len* RING_BUFF_CELL_SIZE);
    return FRE_SUCCESS;
}

int frcore_ssn_get_one_block_addr(uint64_t *buff)
{
    int rv;
    frcore_ssn_chan_t *chan = NULL;
    uint64_t block_addr = 0;
    uint32_t type = FRC_WORK_SSN;


    chan = frcore_ssn_chan_get(type);
    if (chan == NULL)
    {
        FRCORE_ERROR("Invaild type %d.\n",  type);
        return FRE_INIT;
    }

    /* get block addr */
    if (frcore_ssn_chan_avail_get(chan, 1, &block_addr))
    {
        FRCORE_ERROR("frcore_ssn_chan_avail_get failed!\n");
        return FRE_NOSPACE;
    }
    *buff = block_addr;

    rv = FRE_SUCCESS;
    return rv;
}
#endif

int frcore_ssn_chan_compl_put(frcore_ssn_chan_t *chan, uint64_t number, uint64_t *buff)
{
    uint64_t ridx, widx, offset, len, len1, len2;
    uint8_t *local;
    uint64_t remote_addr;

    FRCORE_CHAN("chan->compl_widx=%lld, chan->compl_ridx=%lld\n", (ULL)chan->compl_widx, (ULL)chan->compl_ridx);
    FRCORE_SSN_CHAN_COMPL_LOCK(chan);
    local = (uint8_t *) &chan->compl_ridx;
    remote_addr = chan->compl_addr + FRC_DMA_SSN_COMPL_RIDX_OFFSET;

    if (frcore_copy_from_remote(sizeof(chan->compl_ridx), local, remote_addr)) {
        FRCORE_ERROR("SYNC ridx & widx of compl ring from remote fail!\n");
        FRCORE_STAT_INC(stat_ssn_dma_errors);
        FRCORE_SSN_CHAN_COMPL_UNLOCK(chan);
        return FRE_DMA;
    }

    ridx = chan->compl_ridx;
    widx = chan->compl_widx;

    len = number;

    if ((widx - ridx) > (FRC_DMA_SSN_RING_BUFF_SIZE - len))
    {
        FRCORE_STAT_INC(stat_ssn_compl_put_no_space);
        FRCORE_SSN_CHAN_COMPL_UNLOCK(chan);
        return FRE_NOSPACE;
    }

    memcpy(chan->compl_buff, buff, len*RING_BUFF_CELL_SIZE);
    offset = widx % FRC_DMA_SSN_RING_BUFF_SIZE;

    len1 = (FRC_DMA_SSN_RING_BUFF_SIZE - offset) < len ? (FRC_DMA_SSN_RING_BUFF_SIZE - offset) : len;

    local = (uint8_t *) chan->compl_buff;
    remote_addr = chan->compl_addr + FRC_DMA_SSN_COMPL_RING_OFFSET + offset * RING_BUFF_CELL_SIZE;

    if (frcore_copy_to_remote(len1*RING_BUFF_CELL_SIZE, local, remote_addr)) {
        FRCORE_ERROR("Copy compl ring buff from remote fail!\n");
        FRCORE_STAT_INC(stat_ssn_dma_errors);
        FRCORE_SSN_CHAN_COMPL_UNLOCK(chan);
        return FRE_DMA;
    }

    if (len1 < len)
    {
        len2 = len - len1;
        local = (uint8_t *) chan->compl_buff + len1*RING_BUFF_CELL_SIZE;
        remote_addr = chan->compl_addr + FRC_DMA_SSN_COMPL_RING_OFFSET;

        if(frcore_copy_to_remote(len2*RING_BUFF_CELL_SIZE, local, remote_addr)) {
            FRCORE_ERROR("Copy compl ring buff from remote fail!\n");
            FRCORE_STAT_INC(stat_ssn_dma_errors);
            FRCORE_SSN_CHAN_COMPL_UNLOCK(chan);
            return FRE_DMA;
        }
    }

    widx += len;
    FRCORE_STAT_ADD(stat_ssn_compl_put, len);

    chan->compl_widx = widx;
    local = (uint8_t *) &chan->compl_widx;
    remote_addr = chan->compl_addr + FRC_DMA_SSN_COMPL_WIDX_OFFSET;

    if (frcore_copy_to_remote(sizeof(chan->compl_widx), local, remote_addr)) {
        FRCORE_ERROR("Copy ridx of compl ring to remote fail!\n");
        FRCORE_STAT_INC(stat_ssn_dma_errors);
        FRCORE_SSN_CHAN_COMPL_UNLOCK(chan);
        return FRE_DMA;
    }
    FRCORE_SSN_CHAN_COMPL_UNLOCK(chan);
    return FRE_SUCCESS;
}
#if 0
int frcore_ssn_chan_post_pkt(frcore_ssn_chan_t *chan, frcore_ssn_fifo_t *fifo, uint64_t local_num)
{
    uint64_t cur = 0, cur_offset = 0;
    frcore_dma_scatter_desc_t dma_arg;
    uint64_t remote_addr;
    uint64_t pkt_remain = 0, pkt_posted = 0, dma_numb = 0;
    memset(&dma_arg, 0, sizeof(frcore_dma_scatter_desc_t));

    pkt_remain = local_num;

    dma_numb = 0;
    while (pkt_remain)
    {
        cur_offset = (fifo->local_ridx + cur ) % FRCORE_CORE_FIFO_NUM;
        if (fifo->fifo_cell[cur_offset].type == FRC_SSN_DMA_HEAD)
        {
            /* submit dma head and block addr to compl ring */
            remote_addr = fifo->fifo_cell[cur_offset].block_addr;
            dma_arg.remote_addr[dma_numb] = remote_addr + FRC_DMA_OFFSET;
            dma_arg.local_ptr[dma_numb] = (uint8_t *) &(fifo->fifo_cell[cur_offset].head);
            dma_arg.size[dma_numb] = sizeof(frc_dma_hdr_t);
            FRCORE_STAT_INC(stat_dma_blocks);
            dma_numb++;

            /* if have dma head ,submit dma  */
            dma_arg.number = dma_numb;
            if (frcore_dma_scatter(DMA_DIR_SEND, &dma_arg))
            {
                FRCORE_ERROR("frcore_dma_scatter DMA_DIR_SEND fail!\n");
                FRCORE_STAT_INC(stat_dma_errors);
                goto frcore_queue_post_pkt_err;
            }
            else
            {
                dma_numb = 0;
            }
            /* sbumit block addr to compl ring */
            if (frcore_ssn_chan_compl_put(chan, 1, &remote_addr))
            {
                FRCORE_ERROR("put addr to compl ring failed!\n");
                goto frcore_queue_post_pkt_err;
            }

        } else if (fifo->fifo_cell[cur_offset].type == FRC_SSN_DMA_PAYLOAD)
        {
            /* submit payload and pkt_info */
            /* submit payload */
            remote_addr = fifo->fifo_cell[cur_offset].block_addr;
            dma_arg.remote_addr[dma_numb] = remote_addr + fifo->fifo_cell[cur_offset].payload_offset;
            dma_arg.local_ptr[dma_numb] = (uint8_t *) fifo->fifo_cell[cur_offset].payload;
            dma_arg.size[dma_numb] = fifo->fifo_cell[cur_offset].payload_size;
            FRCORE_STAT_INC(stat_dma_pkts);
            dma_numb++;
            /* if dma_numb == FRCORE_MAX_DMA_POINTERS */
            if (dma_numb == FRCORE_MAX_DMA_POINTERS)
            {
                dma_arg.number = dma_numb;
                if (frcore_dma_scatter(DMA_DIR_SEND, &dma_arg))
                {
                    FRCORE_ERROR("frcore_dma_scatter DMA_DIR_SEND fail!\n");
                    FRCORE_STAT_INC(stat_dma_errors);
                    goto frcore_queue_post_pkt_err;
                }else
                {
                    dma_numb = 0;
                }
            }
            /* submit pkt_info */
            dma_arg.remote_addr[dma_numb] = remote_addr + fifo->fifo_cell[cur_offset].info_offset;
            dma_arg.local_ptr[dma_numb] = (uint8_t *) &(fifo->fifo_cell[cur_offset].info);
            dma_arg.size[dma_numb] = sizeof(frc_dma_pkt_info_t);
            FRCORE_STAT_INC(stat_dma_pkt_infos);
            dma_numb++;
            /* if dma_numb == FRCORE_MAX_DMA_POINTERS */
            if (dma_numb == FRCORE_MAX_DMA_POINTERS)
            {
                dma_arg.number = dma_numb;
                if (frcore_dma_scatter(DMA_DIR_SEND, &dma_arg))
                {
                    FRCORE_ERROR("frcore_dma_scatter DMA_DIR_SEND fail!\n");
                    FRCORE_STAT_INC(stat_dma_errors);
                    goto frcore_queue_post_pkt_err;
                }else
                {
                    dma_numb = 0;
                }
            }
        } else {
            FRCORE_ERROR("FIFO cell error type!\n");
            goto frcore_queue_post_pkt_err;
        }

        pkt_remain -= 1;
        /* the last time,so submit dma */
        if (pkt_remain == 0)
        {
            if (dma_numb > 0)
            {
                dma_arg.number = dma_numb;
                if (frcore_dma_scatter(DMA_DIR_SEND, &dma_arg))
                {
                    FRCORE_ERROR("frcore_dma_scatter DMA_DIR_SEND fail!\n");
                    FRCORE_STAT_INC(stat_dma_errors);
                    goto frcore_queue_post_pkt_err;
                }else
                {
                    dma_numb = 0;
                }
            }
        }
        cur++;
        pkt_posted += 1;
    }

frcore_queue_post_pkt_err:
    return pkt_posted;
}

void frcore_ssn_fifo_post(frcore_ssn_chan_t *chan, frcore_ssn_fifo_t *fifo)
{
    uint64_t spend_cycle;
    uint64_t local_widx, local_ridx, local_num, posted_num, post_num;

    local_widx = fifo->local_widx;
    local_ridx = fifo->local_ridx;
    local_num  = local_widx - local_ridx;

    if (local_num == 0)
    {
        return;
    }
    spend_cycle = cvmx_get_cycle() - fifo->post_cycle;

    if ((spend_cycle  < FRCORE_CHAN_POST_PERIOD) && (local_num < FRCORE_CHAN_POST_LWM))
    {
        return;
    }

    FRCORE_CHAN("CHAN %lld: SPEND %lld cycles, %lld pkts in local buffer ring.\n",
                 (ULL)chan->type, (ULL) spend_cycle, (ULL) local_num);

    FRCORE_CYCLE_RECORDER_INIT();
    while (local_num)
    {
        if (local_num < FRCORE_CHAN_POST_LWM)
        {
            post_num = local_num;
        }
        else
        {
            post_num = FRCORE_CHAN_POST_LWM;
        }
        FRCORE_CYCLE_RECORDING();
        /* Trans dma pkts to x86 */
        posted_num = frcore_ssn_chan_post_pkt(chan, fifo, post_num);
        FRCORE_CHAN("CHAN %lld: frcore_queue_post_pkt posted_num %lld, post_num %lld.\n",
                     (ULL)chan->type, (ULL) posted_num, (ULL) post_num);
        if (posted_num < post_num)
        {
            FRCORE_STAT_INC(stat_pkt_post_err);
            FRCORE_STAT_ADD(stat_post_err_pkts, (post_num - posted_num));
        }
        else
        {
            FRCORE_STAT_INC(stat_pkt_post);
        }
        FRCORE_CYCLE_RECORDING();
        fifo->local_ridx += posted_num;
        fifo->post_cycle = cvmx_get_cycle();
        local_num -= posted_num;
    }
    FRCORE_CYCLE_RECORDING();
    FRCORE_CYCLE_RECORD_DUMP();
}
#else
/*
 loal_num:   local fifo number to transmit
 block_num:  get block address num from transmitted fifo
 block_addr: get block address from transmitted fifo
*/
int frcore_ssn_chan_post_pkt(frcore_ssn_chan_t *chan, frcore_ssn_fifo_t *fifo, uint64_t local_num,
                             uint64_t *block_num, uint64_t *block_addr)
{
    uint64_t cur = 0, cur_offset = 0;
    frcore_dma_scatter_desc_t dma_arg;
    uint64_t remote_addr;
    uint64_t pkt_remain = 0, pkt_posted = 0, dma_numb = 0;
    uint64_t block_numb = 0; /* local block_num*/
    uint64_t *address = block_addr;
    #if FRC_CONFIG_SSN_WQE_TEST
    cvmx_wqe_t  *wqe[FRCORE_MAX_DMA_POINTERS];
    uint8_t wqe_num = 0; /* the wqe number to be freed */
    #endif
    memset(&dma_arg, 0, sizeof(frcore_dma_scatter_desc_t));

    pkt_remain = local_num;

    dma_numb = 0;
    while (pkt_remain)
    {
        cur_offset = (fifo->local_ridx + cur ) % FRCORE_CORE_FIFO_NUM;
        if (fifo->fifo_cell[cur_offset].type == FRC_SSN_DMA_HEAD)
        {
            /* submit dma head and block addr to compl ring */
            remote_addr = fifo->fifo_cell[cur_offset].block_addr;
            dma_arg.remote_addr[dma_numb] = remote_addr + FRC_DMA_OFFSET;
            #if 0
            //#if !FRC_CONFIG_SSN_CHAN_TEST
            dma_arg.local_ptr[dma_numb] = (uint8_t *) fifo->fifo_cell[cur_offset].payload;
            dma_arg.size[dma_numb] = fifo->fifo_cell[cur_offset].payload_size;
            #else
            dma_arg.local_ptr[dma_numb] = (uint8_t *) &(fifo->fifo_cell[cur_offset].head);
            dma_arg.size[dma_numb] = sizeof(frc_dma_hdr_t);
            #endif

            //FRCORE_STAT_INC(stat_dma_blocks);

            dma_numb++;

            /* if dma_numb == FRCORE_MAX_DMA_POINTERS */
            if (dma_numb == FRCORE_MAX_DMA_POINTERS)
            {
                dma_arg.number = dma_numb;
                if (frcore_dma_scatter(DMA_DIR_SEND, &dma_arg))
                {
                    FRCORE_ERROR("frcore_dma_scatter DMA_DIR_SEND fail!\n");
                    FRCORE_STAT_INC(stat_ssn_dma_errors);
                    goto frcore_queue_post_pkt_err;
                }else
                {
                    #if FRC_CONFIG_SSN_WQE_TEST
                    for (i = 0; i < wqe_num; i++)
                    {
                        frcore_work_free(wqe[i]);
                    }
                    //printf("%s %d free wqe_num=%d\n", __FUNCTION__, __LINE__, wqe_num);
                    wqe_num = 0;
                    #endif
                    dma_numb = 0;
                }
            }

            /* if have dma head ,submit dma
            dma_arg.number = dma_numb;
            if (frcore_dma_scatter(DMA_DIR_SEND, &dma_arg))
            {
                FRCORE_ERROR("frcore_dma_scatter DMA_DIR_SEND fail!\n");
                FRCORE_STAT_INC(stat_dma_errors);
                goto frcore_queue_post_pkt_err;
            }
            else
            {
                dma_numb = 0;
            } */
            /* sbumit block addr to compl ring
            if (frcore_ssn_chan_compl_put(chan, 1, &remote_addr))
            {
                FRCORE_ERROR("put addr to compl ring failed!\n");
                goto frcore_queue_post_pkt_err;
            }*/
            address[block_numb++] = remote_addr;

        } else if (fifo->fifo_cell[cur_offset].type == FRC_SSN_DMA_PAYLOAD)
        {
            /* submit payload and pkt_info */
            /* submit payload */
            remote_addr = fifo->fifo_cell[cur_offset].block_addr;
            dma_arg.remote_addr[dma_numb] = remote_addr + fifo->fifo_cell[cur_offset].payload_offset;
            dma_arg.local_ptr[dma_numb] = (uint8_t *) fifo->fifo_cell[cur_offset].payload;
            dma_arg.size[dma_numb] = fifo->fifo_cell[cur_offset].payload_size;
            FRCORE_STAT_INC(stat_ssn_dma_pkts);
            FRCORE_STAT_ADD(stat_ssn_dma_bytes, dma_arg.size[dma_numb]);
            dma_numb++;
            #if FRC_CONFIG_SSN_WQE_TEST
            /* free wqe */
            wqe[wqe_num++] = fifo->fifo_cell[cur_offset].wqe;
            #endif
            /* if dma_numb == FRCORE_MAX_DMA_POINTERS */
            if (dma_numb == FRCORE_MAX_DMA_POINTERS)
            {
                dma_arg.number = dma_numb;
                if (frcore_dma_scatter(DMA_DIR_SEND, &dma_arg))
                {
                    FRCORE_ERROR("frcore_dma_scatter DMA_DIR_SEND fail!\n");
                    FRCORE_STAT_INC(stat_ssn_dma_errors);
                    goto frcore_queue_post_pkt_err;
                }else
                {
                    #if FRC_CONFIG_SSN_WQE_TEST
                    for (i = 0; i < wqe_num; i++)
                    {
                        frcore_work_free(wqe[i]);
                    }
                    //printf("%s %d free wqe_num=%d\n", __FUNCTION__, __LINE__, wqe_num);
                    wqe_num = 0;
                    #endif
                    dma_numb = 0;
                }
            }
            /* submit pkt_info */
            dma_arg.remote_addr[dma_numb] = remote_addr + fifo->fifo_cell[cur_offset].info_offset;
            dma_arg.local_ptr[dma_numb] = (uint8_t *) &(fifo->fifo_cell[cur_offset].info);
            dma_arg.size[dma_numb] = sizeof(frc_dma_pkt_info_t);
            FRCORE_STAT_INC(stat_ssn_dma_pkt_infos);
            dma_numb++;
            /* if dma_numb == FRCORE_MAX_DMA_POINTERS */
            if (dma_numb == FRCORE_MAX_DMA_POINTERS)
            {
                dma_arg.number = dma_numb;
                if (frcore_dma_scatter(DMA_DIR_SEND, &dma_arg))
                {
                    FRCORE_ERROR("frcore_dma_scatter DMA_DIR_SEND fail!\n");
                    FRCORE_STAT_INC(stat_ssn_dma_errors);
                    goto frcore_queue_post_pkt_err;
                }else
                {
                    #if FRC_CONFIG_SSN_WQE_TEST
                    for (i = 0; i < wqe_num; i++)
                    {
                        frcore_work_free(wqe[i]);
                    }
                    //printf("%s %d free wqe_num=%d\n", __FUNCTION__, __LINE__, wqe_num);
                    wqe_num = 0;
                    #endif
                    dma_numb = 0;
                }
            }
        } else {
            printf("FIFO cell error type!\n");
            goto frcore_queue_post_pkt_err;
        }

        pkt_remain -= 1;
        /* the last time,so submit dma */
        if (pkt_remain == 0)
        {
            if (dma_numb > 0)
            {
                dma_arg.number = dma_numb;
                if (frcore_dma_scatter(DMA_DIR_SEND, &dma_arg))
                {
                    FRCORE_ERROR("frcore_dma_scatter DMA_DIR_SEND fail!\n");
                    FRCORE_STAT_INC(stat_ssn_dma_errors);
                    goto frcore_queue_post_pkt_err;
                }else
                {
                    #if FRC_CONFIG_SSN_WQE_TEST
                    for (i = 0; i < wqe_num; i++)
                    {
                        frcore_work_free(wqe[i]);
                    }
                    //printf("%s %d free wqe_num=%d\n", __FUNCTION__, __LINE__, wqe_num);
                    wqe_num = 0;
                    #endif
                    dma_numb = 0;
                }
            }
        }
        cur++;
        pkt_posted += 1;
    }

frcore_queue_post_pkt_err:
    *block_num = block_numb;
    return pkt_posted;
}

/*
 * flag  for 1 ,submit 14x16
 *       for 0, submit free(> 0)
 */
void frcore_ssn_fifo_post(frcore_ssn_chan_t *chan, frcore_ssn_fifo_t *fifo, uint8_t flag)
{
    uint64_t local_widx, local_ridx, local_num, posted_num, post_num;
    uint64_t block_addr[FRCORE_CHAN_POST_LWM], block_num;

    FRCORE_SSN_CHAN_FIFO_LOCK(fifo);
    local_widx = fifo->local_widx;
    local_ridx = fifo->local_ridx;
    local_num  = local_widx - local_ridx;

    if (flag)
    {
        if (local_num != FRCORE_CHAN_POST_LWM)
        {
            goto frcore_ssn_post_pkt_err;
        }
    }else {
        if (local_num == 0)
        {
            goto frcore_ssn_post_pkt_err;
        }
    }


    //FRCORE_CYCLE_RECORDER_INIT();
    while (local_num)
    {
        if (local_num < FRCORE_CHAN_POST_LWM)
        {
            post_num = local_num;
        }
        else
        {
            post_num = FRCORE_CHAN_POST_LWM;
        }
        //FRCORE_CYCLE_RECORDING();
        /* Trans dma pkts to x86 */
        posted_num = frcore_ssn_chan_post_pkt(chan, fifo, post_num, &block_num, block_addr);
        FRCORE_CHAN("CHAN %lld: frcore_queue_post_pkt posted_num %lld, post_num %lld.\n",
                     (ULL)chan->type, (ULL) posted_num, (ULL) post_num);
        if (posted_num < post_num)
        {
            FRCORE_STAT_INC(stat_ssn_pkt_post_err);
            FRCORE_STAT_ADD(stat_ssn_post_err_pkts, (post_num - posted_num));
        }
        else
        {
            FRCORE_STAT_INC(stat_ssn_pkt_post);
        }
        //FRCORE_CYCLE_RECORDING();
        /* sbumit block addr to compl ring*/
        if (frcore_ssn_chan_compl_put(chan, block_num, block_addr))
        {
            FRCORE_ERROR("put addr to compl ring failed!\n");
            goto frcore_ssn_post_pkt_err;
        }
        fifo->local_ridx += posted_num;
        CVMX_SYNCWS;
        fifo->post_cycle = cvmx_get_cycle();
        local_num -= posted_num;
    }
    //FRCORE_CYCLE_RECORDING();
frcore_ssn_post_pkt_err:
    FRCORE_SSN_CHAN_FIFO_UNLOCK(fifo);
     if (1)
     {
     }
    //FRCORE_CYCLE_RECORD_DUMP();
}

/*
 * flag  for 1 ,submit 14x16
 *       for 0, submit free(> 0)
 */
void frcore_ssn_fifo_post_no_lock(frcore_ssn_chan_t *chan, frcore_ssn_fifo_t *fifo, uint8_t flag)
{
    uint64_t local_widx, local_ridx, local_num, posted_num, post_num;
    uint64_t block_addr[FRCORE_CHAN_POST_LWM], block_num;

    //FRCORE_SSN_CHAN_FIFO_LOCK(fifo);
    local_widx = fifo->local_widx;
    local_ridx = fifo->local_ridx;
    local_num  = local_widx - local_ridx;

    if (flag)
    {
        if (local_num != FRCORE_CHAN_POST_LWM)
        {
            goto frcore_ssn_post_pkt_err;
        }
    }else {
        if (local_num == 0)
        {
            goto frcore_ssn_post_pkt_err;
        }
    }


    //FRCORE_CYCLE_RECORDER_INIT();
    while (local_num)
    {
        if (local_num < FRCORE_CHAN_POST_LWM)
        {
            post_num = local_num;
        }
        else
        {
            post_num = FRCORE_CHAN_POST_LWM;
        }
        //FRCORE_CYCLE_RECORDING();
        /* Trans dma pkts to x86 */
        posted_num = frcore_ssn_chan_post_pkt(chan, fifo, post_num, &block_num, block_addr);
        FRCORE_CHAN("CHAN %lld: frcore_queue_post_pkt posted_num %lld, post_num %lld.\n",
                     (ULL)chan->type, (ULL) posted_num, (ULL) post_num);
        if (posted_num < post_num)
        {
            FRCORE_STAT_INC(stat_ssn_pkt_post_err);
            FRCORE_STAT_ADD(stat_ssn_post_err_pkts, (post_num - posted_num));
        }
        else
        {
            FRCORE_STAT_INC(stat_ssn_pkt_post);
        }
        //FRCORE_CYCLE_RECORDING();
        /* sbumit block addr to compl ring*/
        if (frcore_ssn_chan_compl_put(chan, block_num, block_addr))
        {
            FRCORE_ERROR("put addr to compl ring failed!\n");
            goto frcore_ssn_post_pkt_err;
        }
        fifo->local_ridx += posted_num;
        CVMX_SYNCWS;
        fifo->post_cycle = cvmx_get_cycle();
        local_num -= posted_num;
    }
    //FRCORE_CYCLE_RECORDING();
frcore_ssn_post_pkt_err:
    //FRCORE_SSN_CHAN_FIFO_UNLOCK(fifo);
     if (1)
     {
     }
    //FRCORE_CYCLE_RECORD_DUMP();
}
#endif

#if !FRC_CONFIG_SSN_CHAN_TEST
int frcore_ssn_fifo_add(frcore_ssn_chan_t *chan,frcore_ssn_fifo_t *fifo, uint64_t dma_type,
                        frc_dma_hdr_t *hdr, frc_dma_pkt_info_t *info, void *payload,
                        uint64_t block_addr, uint16_t info_offset, uint16_t payload_offset, cvmx_wqe_t  *wqe)
{
    int rv = 0;
    int hdr_size, info_size, paylen;
    uint64_t idx_offset = 0;

    hdr_size = sizeof(frc_dma_hdr_t);
    info_size = sizeof(frc_dma_pkt_info_t);

    FRCORE_SSN_CHAN_FIFO_LOCK(fifo);
    /* if fifo if full, sbumit it */
    frcore_ssn_fifo_post_no_lock(chan, fifo, 1);

    if ((fifo->local_widx - fifo->local_ridx) >= FRCORE_CORE_FIFO_NUM)
    {
        rv = FRE_NOSPACE;
        FRCORE_STAT_INC(stat_ssn_queue_no_space);
        goto frcore_ssn_fifo_add_err;
    }

    idx_offset = fifo->local_widx % FRCORE_CORE_FIFO_NUM;
    //FRCORE_CYCLE_RECORDING();
    if (dma_type == FRC_SSN_DMA_HEAD)
    {
        memcpy(&fifo->fifo_cell[idx_offset].head, hdr, hdr_size);
    }else{
        paylen = info->payload_len;
        fifo->fifo_cell[idx_offset].info_offset    = info_offset;
        fifo->fifo_cell[idx_offset].payload_offset = payload_offset;
        fifo->fifo_cell[idx_offset].payload_size   = paylen;
        #if FRC_CONFIG_SSN_WQE_TEST
        fifo->fifo_cell[idx_offset].payload        = payload;
        fifo->fifo_cell[idx_offset].wqe            = wqe;
        #else
        memcpy(fifo->fifo_cell[idx_offset].payload, payload, paylen);
        frcore_work_free(wqe);
        #endif
        memcpy(&fifo->fifo_cell[idx_offset].info, info, info_size);
    }
    fifo->fifo_cell[idx_offset].block_addr     = block_addr;
    fifo->fifo_cell[idx_offset].type           = dma_type;

    //FRCORE_CYCLE_RECORDING();

    fifo->local_widx++;
    CVMX_SYNCWS;

frcore_ssn_fifo_add_err:
    FRCORE_SSN_CHAN_FIFO_UNLOCK(fifo);
    return rv;
}


int frcore_forward_ssn_pkt_to_fifo(uint32_t type,uint32_t index, uint64_t dma_type,
                                   frc_dma_hdr_t *hdr, frc_dma_pkt_info_t *info,
                                   void *payload,uint64_t block_addr, uint16_t info_offset,
                                   uint16_t payload_offset, cvmx_wqe_t  *wqe)
{
    int num, rv;
    frcore_ssn_chan_t *chan;
    frcore_ssn_fifo_t *fifo;
    uint32_t fifo_id = 0;

    chan = frcore_ssn_chan_get(type);
    if (chan == NULL)
    {
        FRCORE_ERROR("Invaild chan_id %d.\n", type);
        return FRE_INIT;
    }
    //fifo_id = cvmx_get_core_num();
    fifo_id = index % FRC_SSN_FIFO_NUM;
    fifo = frcore_ssn_fifo_get(type, fifo_id);
    if (fifo == NULL)
    {
        FRCORE_ERROR("Invaild type %d fifo_id %d.\n",  type, fifo_id);
        return FRE_INIT;
    }

    /* if fifo if full, sbumit it
    frcore_ssn_fifo_post(chan, fifo, 1);*/
    //FRCORE_CYCLE_RECORDING();
    num = frcore_ssn_fifo_add(chan, fifo, dma_type, hdr, info, payload, block_addr,
                              info_offset, payload_offset, wqe);
    if (num != 0)
    {
        FRCORE_ERROR("ssn fifo exceed!\n");
        FRCORE_STAT_INC(stat_ssn_dma_enqueue_errs);
        rv = FRE_EXCEED;
        goto frcore_forward_pkt_to_host_err;
    }
    else
    {
        FRCORE_STAT_INC(stat_ssn_dma_enqueue_pkts);
    }

    rv = FRE_SUCCESS;

frcore_forward_pkt_to_host_err:
    return rv;
}
#endif /* end of FRC_CONFIG_SSN_CHAN_TEST */

/* for ssn test */
#if FRC_CONFIG_SSN_CHAN_TEST
int frcore_ssn_chan_pkt_to_fifo_test(uint32_t type)
{
    int rv;
    frcore_ssn_chan_t *chan = NULL;
    frcore_ssn_fifo_t *fifo = NULL;
    uint64_t block_addr = 0;
    uint8_t  data[FRC_DMA_SSN_BLOCK_SIZE];
    uint32_t fifo_id = 0, idx_offset;
    uint32_t i;
    uint16_t payload_offset = FRC_DMA_OFFSET + sizeof(frc_dma_hdr_t), info_offset = FRC_DMA_SSN_BLOCK_SIZE - sizeof(frc_dma_pkt_info_t) ;
    #if FRC_CONFIG_SSN_CHAN_TEST_ONE_BLOCK_ONE_PAYLOD
    uint16_t paylen = 512;
    #else
    uint16_t paylen = (FRC_DMA_SSN_BLOCK_SIZE - sizeof(frc_dma_hdr_t) - FRC_DMA_OFFSET)/16 - sizeof(frc_dma_pkt_info_t); /* should 978 */
    #endif
    fifo_id = cvmx_get_core_num();

    chan = frcore_ssn_chan_get(type);
    if (chan == NULL)
    {
        FRCORE_ERROR("Invaild type %d.\n",  type);
        return FRE_INIT;
    }

    fifo = frcore_ssn_fifo_get(type, fifo_id);
    if (fifo == NULL)
    {
        FRCORE_ERROR("Invaild type %d fifo_id %d.\n",  type, fifo_id);
        return FRE_INIT;
    }

    /* get block addr */
    if (frcore_ssn_get_one_block_addr(&block_addr))
    {
        FRCORE_ERROR("frcore_ssn_chan_avail_get failed!\n");
        return FRE_NOSPACE;
    }

    //FRCORE_TEST("block_addr=0x%llx\n", block_addr);
        /* check if have enough space */
    if ((fifo->local_widx - fifo->local_ridx) >  FRCORE_CORE_FIFO_NUM - 9)
    {
        FRCORE_TEST("FIFO no enough space\n");
        return FRE_NOSPACE;
    }

    /* CALC BLOCK DATA */
    srand((uint32_t) cvmx_get_cycle());
    for (i = 0; i < FRC_DMA_SSN_DATA_SIZE - 1; i++)
    {
        data[i + FRC_DMA_OFFSET] = (uint8_t)rand();
    }
    data[i+ FRC_DMA_OFFSET] = cal_crc(&data[FRC_DMA_OFFSET], FRC_DMA_SSN_DATA_SIZE -1);

    /* print some struct type length */
    //FRCORE_TEST("FRC_DMA_SSN_BLOCK_SIZE: %lld\n", (ULL)FRC_DMA_SSN_BLOCK_SIZE); /* should be 8192 */
    //FRCORE_TEST("FRC_DMA_OFFSET: %ld\n", FRC_DMA_OFFSET); /* should be 40 */
    //FRCORE_TEST("sizeof(frc_dma_hdr_t): %ld\n", sizeof(frc_dma_hdr_t)); /* should be 72 */
    //FRCORE_TEST("sizeof(frc_dma_pkt_info_t): %ld\n", sizeof(frc_dma_pkt_info_t)); /* should be 32 */
    //FRCORE_TEST("paylen: %d\n", paylen); /* should be 978 */

    #if FRC_CONFIG_SSN_CHAN_TEST
    #if FRC_CONFIG_SSN_CHAN_TEST_ONE_BLOCK_ONE_PAYLOD
    idx_offset = fifo->local_widx % FRCORE_CORE_FIFO_NUM;
    fifo->fifo_cell[idx_offset].type = FRC_SSN_DMA_PAYLOAD;
    fifo->fifo_cell[idx_offset].block_addr = block_addr;
    memcpy(fifo->fifo_cell[idx_offset].payload, &data[payload_offset], paylen);
    fifo->fifo_cell[idx_offset].payload_size = paylen;
    fifo->fifo_cell[idx_offset].payload_offset = payload_offset;
    memcpy(&fifo->fifo_cell[idx_offset].info, &data[info_offset], sizeof(frc_dma_pkt_info_t));
    fifo->fifo_cell[idx_offset].info_offset = info_offset;
    fifo->local_widx++;
    #else
    /* fill in data to fifo */
    for (i = 0; i < 16; i++ ) /* one block 8 times fiied */
    {
        idx_offset = fifo->local_widx % FRCORE_CORE_FIFO_NUM;
        fifo->fifo_cell[idx_offset].type = FRC_SSN_DMA_PAYLOAD;
        fifo->fifo_cell[idx_offset].block_addr = block_addr;
        memcpy(fifo->fifo_cell[idx_offset].payload, &data[payload_offset], paylen);
        fifo->fifo_cell[idx_offset].payload_size = paylen;
        fifo->fifo_cell[idx_offset].payload_offset = payload_offset;
        memcpy(&fifo->fifo_cell[idx_offset].info, &data[info_offset], sizeof(frc_dma_pkt_info_t));
        fifo->fifo_cell[idx_offset].info_offset = info_offset;
        fifo->local_widx++;

        /* update payload_offset and info_offset */
        payload_offset += paylen;
        info_offset -= sizeof(frc_dma_pkt_info_t);
    }
    //FRCORE_TEST("payload_offset = %d\n", payload_offset);
    //FRCORE_TEST("info_offset = %d\n", info_offset);
    #endif /*end of FRC_CONFIG_SSN_CHAN_TEST_ONE_BLOCK_ONE_PAYLOD*/
    /* fill in the dma head */
    idx_offset = fifo->local_widx % FRCORE_CORE_FIFO_NUM;
    fifo->fifo_cell[idx_offset].type = FRC_SSN_DMA_HEAD;
    fifo->fifo_cell[idx_offset].block_addr = block_addr;
    memcpy(&fifo->fifo_cell[idx_offset].head, &data[FRC_DMA_OFFSET], sizeof(frc_dma_hdr_t));
    fifo->local_widx++;

    #else
    idx_offset = fifo->local_widx % FRCORE_CORE_FIFO_NUM;
    fifo->fifo_cell[idx_offset].type = FRC_SSN_DMA_HEAD;
    fifo->fifo_cell[idx_offset].block_addr = block_addr;
    memcpy(fifo->fifo_cell[idx_offset].payload, &data[FRC_DMA_OFFSET], FRC_DMA_SSN_BLOCK_SIZE-FRC_DMA_OFFSET);
    fifo->fifo_cell[idx_offset].payload_size = FRC_DMA_SSN_BLOCK_SIZE-FRC_DMA_OFFSET;
    fifo->local_widx++;
    #endif

    rv = FRE_SUCCESS;
    return rv;
}

int frcore_forward_ssn_pkt_to_fifos_test(uint32_t type)
{
    int num, rv;
    frcore_ssn_fifo_t *fifo;
    uint32_t fifo_id = 0;
    fifo_id = cvmx_get_core_num();

    fifo = frcore_ssn_fifo_get(type, fifo_id);
    if (fifo == NULL)
    {
        FRCORE_ERROR("Invaild type %d fifo_id %d.\n",  type, fifo_id);
        return FRE_INIT;
    }
    for (num = 0; num < 100; num++)
    {
        if ((fifo->local_widx - fifo->local_ridx) > FRCORE_CORE_FIFO_NUM - 9)
        {
            break;
        }
        frcore_ssn_chan_pkt_to_fifo_test(type);
    }

    rv = FRE_SUCCESS;
    return rv;
}
#endif /* end of FRC_CONFIG_SSN_CHAN_TEST*/


#endif /* end of FRC_CONFIG_SSN */



#if FRC_CONFIG_SIMPLE_PACKET_TEST
int frcore_forward_simple_pkt_to_fifo_test(uint32_t type)
{
    int num, rv;
    frcore_simple_package_fifo_t *fifo;
    uint32_t fifo_id = 0, idx_offset;
    uint32_t i;
    fifo_id = cvmx_get_core_num();

    fifo = frcore_simple_package_fifo_get(type, fifo_id);
    if (fifo == NULL)
    {
        FRCORE_ERROR("Invaild type %d fifo_id %d.\n",  type, fifo_id);
        return FRE_INIT;
    }
    for (num = 0; num < 10000; num++)
    {
        if ((fifo->local_widx - fifo->local_ridx) >= FRCORE_CORE_FIFO_NUM)
        {
            break;
        }
        srand((uint32_t) cvmx_get_cycle());
        idx_offset = fifo->local_widx % FRCORE_CORE_FIFO_NUM;
        fifo->fifo_cell[idx_offset].pkt_size = FRC_DMA_SIMPLE_PACKAGE_DATA_SIZE;
        for (i = 0; i < FRC_DMA_SIMPLE_PACKAGE_DATA_SIZE-1; i++)
        {
            fifo->fifo_cell[idx_offset].data[i] = (uint8_t)rand();
        }
        fifo->fifo_cell[idx_offset].data[i] = cal_crc(fifo->fifo_cell[idx_offset].data, FRC_DMA_SIMPLE_PACKAGE_DATA_SIZE-1);
        fifo->local_widx++;
    }


    rv = FRE_SUCCESS;
    return rv;
}
#endif

#if FRC_CONFIG_SIMPLE_PACKET_TEST || FRC_CONFIG_SSN_CHAN_TEST
int frcore_chan_test_process(cvmx_wqe_t *wqe)
{
    uint8_t fifo_id, type;
    unsigned int core_num;
    type    = wqe->packet_data[0];
    fifo_id = wqe->packet_data[1];

    core_num = cvmx_get_core_num();
    if (core_num != fifo_id)
    {
        FRCORE_ERROR("CHAN %lld(%s) core_num(%d) != fifo_id(%d).\n", (ULL)type,
                     (type == FRC_WORK_UDP ? "UDP":(type==FRC_WORK_RULE ? "RULE":"SSN")),
                     core_num, fifo_id);
    }
    if (type != FRC_WORK_UDP && type != FRC_WORK_RULE && type != FRC_WORK_SSN)
    {
        FRCORE_ERROR("CHAN %lld(%s)\n", (ULL)type,
                     (type == FRC_WORK_UDP ? "UDP":(type==FRC_WORK_RULE ? "RULE":(type==FRC_WORK_SSN)?"SSN":"OTHERS")));
    }

    if (type == FRC_WORK_UDP || type == FRC_WORK_RULE)
    {
        #if FRC_CONFIG_SIMPLE_PACKET_TEST
        frcore_forward_simple_pkt_to_fifo_test(type);
        #endif
    }else if (type == FRC_WORK_SSN)
    {
#if FRC_CONFIG_SSN_CHAN_TEST
        frcore_forward_ssn_pkt_to_fifos_test(type);
#endif
    }

    if (cvmx_tim_add_entry(wqe, FRCORE_CHAN_TIMEOUT, NULL) != CVMX_TIM_STATUS_SUCCESS) {
        FRCORE_ERROR("dma chan timer add fail!\n");
        return FRE_FAIL;
    }
    return FRCORE_ACT_UNFREE;
}
#endif

int frcore_chan_process(cvmx_wqe_t *wqe)
{
    uint8_t fifo_id, type;
    frcore_simple_package_chan_t *simple_chan = NULL;
    frcore_simple_package_fifo_t *simple_fifo = NULL;
    frcore_ssn_chan_t *ssn_chan = NULL;
    frcore_ssn_fifo_t *ssn_fifo = NULL;
    unsigned int core_num;
    type    = wqe->packet_data[0];
    fifo_id = wqe->packet_data[1];

    core_num = cvmx_get_core_num();
    if (core_num != fifo_id%FRC_DAT_CORE_NUM)
    {
        printf("CHAN %lld(%s) core_num(%d) != fifo_id(%d).\n", (ULL)type,
                     (type == FRC_WORK_UDP ? "UDP":(type==FRC_WORK_RULE ? "RULE":"SSN")),
                     core_num, fifo_id);
    }
    if (type != FRC_WORK_UDP && type != FRC_WORK_RULE && type != FRC_WORK_SSN)
    {
        printf("CHAN %lld(%s)\n", (ULL)type,
                     (type == FRC_WORK_UDP ? "UDP":(type==FRC_WORK_RULE ? "RULE":(type==FRC_WORK_SSN)?"SSN":"OTHERS")));
    }

    if (type == FRC_WORK_UDP )
    {
        #if FRC_CONFIG_SIMPLE_PACKAGE
        simple_chan = frcore_simple_package_chan_get(type);
        if (simple_chan == NULL)
        {
            printf("Invaild chan_id %d.\n", type);
            return FRE_FAIL;
        }

        simple_fifo = frcore_simple_package_fifo_get(type, fifo_id%FRC_DAT_CORE_NUM);
        if (simple_fifo == NULL)
        {
            printf("Invaild fifo_id %d.\n", fifo_id%FRC_DAT_CORE_NUM);
            return FRE_FAIL;
        }
        //FRCORE_INFO("simple_chan=%p\n", simple_chan);
        //FRCORE_INFO("simple_fifo=%p\n", simple_fifo);
        frcore_simple_package_fifo_post(simple_chan,simple_fifo, fifo_id%FRC_DAT_CORE_NUM, 0);
        #endif
    }else if (type == FRC_WORK_RULE)
    {
        #if FRC_CONFIG_SIMPLE_PACKAGE
        simple_chan = frcore_simple_package_chan_get(type);
        if (simple_chan == NULL)
        {
            printf("Invaild chan_id %d.\n", type);
            return FRE_FAIL;
        }

        simple_fifo = frcore_simple_package_fifo_get(type, fifo_id%FRC_DAT_CORE_NUM);
        if (simple_fifo == NULL)
        {
            printf("Invaild fifo_id %d.\n", fifo_id%FRC_DAT_CORE_NUM);
            return FRE_FAIL;
        }
        //FRCORE_INFO("simple_chan=%p\n", simple_chan);
        //FRCORE_INFO("simple_fifo=%p\n", simple_fifo);
        frcore_rule_fifo_post(simple_chan,simple_fifo, fifo_id%FRC_DAT_CORE_NUM, 0);
        #endif
    }else if (type == FRC_WORK_SSN)
    {
        #if FRC_CONFIG_SSN_CHAN
        ssn_chan = frcore_ssn_chan_get(type);
        if (ssn_chan == NULL)
        {
            printf("Invaild chan_id %d.\n", type);
            return FRE_FAIL;
        }

        ssn_fifo = frcore_ssn_fifo_get(type, fifo_id);
        if (ssn_fifo == NULL)
        {
            printf("Invaild fifo_id %d.\n", fifo_id);
            return FRE_FAIL;
        }
        //FRCORE_INFO("simple_chan=%p\n", simple_chan);
        //FRCORE_INFO("simple_fifo=%p\n", simple_fifo);
        frcore_ssn_fifo_post(ssn_chan,ssn_fifo,0);
        #endif
    }


    if (cvmx_tim_add_entry(wqe, FRCORE_CHAN_TIMEOUT, NULL) != CVMX_TIM_STATUS_SUCCESS) {
        printf("dma chan timer add fail!\n");
        return FRE_FAIL;
    }
    return FRCORE_ACT_DEBUG;
}

void frcore_print_block_usage(void)
{
    uint64_t avail_widx, avail_ridx;
    uint64_t compl_widx, compl_ridx;
    int i;
    frcore_simple_package_chan_t *simple_chan = NULL;
    frcore_ssn_chan_t *ssn_chan = NULL;

    /* print udp and rule channel block usage */
    for (i = 0; i < 2; i++) {
        #if FRC_CONFIG_SIMPLE_PACKAGE
        simple_chan = frcore_simple_package_chan_get(i);
        FRCORE_SIMPLE_PACKAGE_CHAN_AVAIL_LOCK(simple_chan);
        avail_widx = simple_chan->avail_widx;
        avail_ridx = simple_chan->avail_ridx;
        FRCORE_SIMPLE_PACKAGE_CHAN_AVAIL_UNLOCK(simple_chan);
        //FRCORE_SIMPLE_PACKAGE_CHAN_COMPL_LOCK(chan);
        //compl_widx = simple_chan->compl_widx;
        //compl_ridx = simple_chan->compl_ridx;
        //FRCORE_SIMPLE_PACKAGE_CHAN_COMPL_UNLOCK(chan);
        printf("%-4s: %6d blocks, %6d us, %6d id\n", (i==0)?"UDP":"RULE", FRC_SIMPLE_RING_BUFF_SIZE,
               FRC_SIMPLE_RING_BUFF_SIZE-(avail_widx - avail_ridx), avail_widx-avail_ridx);
        #endif
    }

    #if FRC_CONFIG_SSN_CHAN
    /* print ssn channel block usage */
    ssn_chan = frcore_ssn_chan_get(FRC_WORK_SSN);
    FRCORE_SSN_CHAN_AVAIL_LOCK(ssn_chan);
    avail_widx = ssn_chan->avail_widx;
    avail_ridx = ssn_chan->avail_ridx;
    FRCORE_SSN_CHAN_AVAIL_UNLOCK(ssn_chan);
    printf("SSN : %6d blocks, %6u us, %6u id\n", FRC_DMA_SSN_RING_BUFF_SIZE,
           FRC_DMA_SSN_RING_BUFF_SIZE-(avail_widx - avail_ridx), avail_widx-avail_ridx);
    /*FRCORE_SSN_CHAN_COMPL_LOCK(ssn_chan);
    compl_widx = ssn_chan->compl_widx;
    compl_ridx = ssn_chan->compl_ridx;
    FRCORE_SSN_CHAN_COMPL_UNLOCK(ssn_chan);
    printf("--- %6u us, %6u id\n",
           FRC_DMA_SSN_RING_BUFF_SIZE-(compl_widx - compl_ridx), compl_widx-compl_ridx);*/
    #endif

}

/* End of file */

#include "cvmcs-common.h"
#include  <cvmx-atomic.h>
#include "frc.h"
#include "frcore.h"
#include "frcore_config.h"

#include "frcore_stat.h"
#include "frcore_dma.h"
#include "frcore_cmd.h"
#include "frc_util.h"
#include "frc_cmd.h"
#include "frc_dma.h"
#if FRC_CONFIG_SIMPLE_PACKAGE || FRC_CONFIG_SSN_CHAN
int
frcore_dma_scatter(int dir, frcore_dma_scatter_desc_t *desc)
{
    int rv;

    int i;

    cvm_pci_dma_cmd_t      cmd;
    cvmx_buf_ptr_t          lptr[CN3XXX_MAX_DMA_LOCAL_POINTERS];
        cvm_dma_remote_ptr_t    rptr[CN3XXX_MAX_DMA_REMOTE_POINTERS];

#if defined(DMA_USE_COMPLETION_WORD)
        cvm_dma_comp_ptr_t     *comp_ptr = NULL;
        uint8_t                *word_to_check= NULL;
#else
#   error "Undefined DMA_USE_COMPLETION_WORD"
#endif

    if (desc->number == 0)
    {
        FRCORE_ERROR("desc->number == 0.\n");
        FRCORE_STAT_INC(stat_dma_errors);
        return FRE_PARAM;
    }

    if (desc->number > FRCORE_MAX_DMA_POINTERS)
    {
        FRCORE_ERROR("desc->number(%d) > FRCORE_MAX_DMA_POINTERS(%d).\n", desc->number, FRCORE_MAX_DMA_POINTERS);
        FRCORE_STAT_INC(stat_dma_errors);
        return FRE_PARAM;
    }

#if defined(DMA_USE_COMPLETION_WORD)
        /* If the DMA Completion uses a memory location, get a word from the
         * core PCI driver.
         */
        comp_ptr = cvm_get_dma_comp_ptr();
        if(comp_ptr == NULL) {
                FRCORE_ERROR("Discard. No comp_ptr allocated\n");
        FRCORE_STAT_INC(stat_dma_errors);
                return FRE_MEMORY;
        }
        word_to_check = &comp_ptr->comp_byte;
        *word_to_check = 0xff;
#endif

    for (i = 0; i < desc->number; i++)
    {
        if (desc->size[i] == 0)
        {
            FRCORE_ERROR("desc->size[%d] == 0)\n", i);
            rv = FRE_PARAM;
            goto frcore_dma_err;
        }

        if (desc->size[i] > MAX_PCI_DMA_LOCAL_BUF_SIZE)
        {
            FRCORE_ERROR("desc->size[%d](%d) > MAX_PCI_DMA_LOCAL_BUF_SIZE(%d)\n", i, desc->size[i], MAX_PCI_DMA_LOCAL_BUF_SIZE);
            rv = FRE_PARAM;
            goto frcore_dma_err;
        }

        lptr[i].u64 = 0;
        rptr[i].u64 = 0;
        lptr[i].s.i = 0;
        lptr[i].s.addr = CVM_DRV_GET_PHYS(desc->local_ptr[i]);
        rptr[i].s.addr = desc->remote_addr[i];
        lptr[i].s.size = rptr[i].s.size = desc->size[i];
    }

    cmd.u64  = 0;
    cmd.s.nl = cmd.s.nr = i;
        /* Reset all bytes so that unused fields don't have any value. */
#if defined(DMA_USE_COMPLETION_WORD)
        cmd.s.flags |= PCI_DMA_PUTWORD;
        cmd.s.ptr = cvmx_ptr_to_phys(word_to_check);
#endif

    if(dir == DMA_DIR_RECV)
    {
        rv = cvm_pci_dma_recv_data(&cmd, lptr, rptr);
    }
    else
    {
        rv = cvm_pci_dma_send_data(&cmd, lptr, rptr);
    }

        if(rv) {
        FRCORE_STAT_INC(stat_dma_errors);
        rv = FRE_DMA;
        goto frcore_dma_err;
    }

#if defined(DMA_USE_COMPLETION_WORD)
        i = 0;
        while((*word_to_check == 0xff) && (++i < DMA_WAIT_INTERVAL)) {
                cvmx_wait(10);
                CVMX_SYNCWS;
        }

        if(i == DMA_WAIT_INTERVAL) {
                printf("DMA FAILED interval: %d  Completion Word value: %x\n", i,
                       *word_to_check);
        }
#endif
frcore_dma_err:

#if defined(DMA_USE_COMPLETION_WORD)
        if(comp_ptr)
                cvm_release_dma_comp_ptr(comp_ptr);
#endif
    return rv;
}

int
_frcore_dma_gather(int size, void *local, uint64_t remote_addr, int dir)
{
    int cur = 0, rv;
    uint64_t local_phy_addr, remote_phy_addr;
    uint32_t i;

    cvm_pci_dma_cmd_t      cmd;
    cvmx_buf_ptr_t          lptr[CN3XXX_MAX_DMA_LOCAL_POINTERS];
        cvm_dma_remote_ptr_t    rptr[CN3XXX_MAX_DMA_REMOTE_POINTERS];

#if defined(DMA_USE_COMPLETION_WORD)
        cvm_dma_comp_ptr_t     *comp_ptr = NULL;
        uint8_t                *word_to_check= NULL;
#else
#   error "Undefined DMA_USE_COMPLETION_WORD"
#endif
    //FRCORE_DEBUG("remote_addr = 0x%llx\n", (ULL) remote_addr);

    if (size == 0)
    {
        FRCORE_ERROR("size == 0!\n");
        return FRE_PARAM;
    }

    if (size > (FRCORE_DMA_BUFF_MAX))
    {
        FRCORE_ERROR("size(%d) > FRCORE_DMA_BUFF_MAX(%d).\n", size, FRCORE_DMA_BUFF_MAX);
        return FRE_PARAM;
    }

#if defined(DMA_USE_COMPLETION_WORD)

        /* If the DMA Completion uses a memory location, get a word from the
         * core PCI driver.
         */
        comp_ptr = cvm_get_dma_comp_ptr();
        if(comp_ptr == NULL) {
                FRCORE_ERROR("Discard. No comp_ptr allocated\n");
                return FRE_MEMORY;
        }
        word_to_check = &comp_ptr->comp_byte;
        *word_to_check = 0xff;
#endif

    i = 0;
    local_phy_addr = CVM_DRV_GET_PHYS(local);
    remote_phy_addr = remote_addr;
    while(size)
    {
        if (size > MAX_PCI_DMA_LOCAL_BUF_SIZE)
        {
            cur = MAX_PCI_DMA_LOCAL_BUF_SIZE;
        } else {
            cur = size;
        }
        lptr[i].u64 = 0;
        rptr[i].u64 = 0;
        lptr[i].s.i = 0;
        lptr[i].s.addr = local_phy_addr;
        rptr[i].s.addr = remote_phy_addr;
        lptr[i].s.size = rptr[i].s.size = cur;
        local_phy_addr += cur;
        remote_phy_addr += cur;
        size -= cur;
        i++;

    };


    cmd.u64  = 0;
    cmd.s.nl = cmd.s.nr = i;
        /* Reset all bytes so that unused fields don't have any value. */
#if defined(DMA_USE_COMPLETION_WORD)
        cmd.s.flags |= PCI_DMA_PUTWORD;
        cmd.s.ptr = cvmx_ptr_to_phys(word_to_check);
#endif

    if(dir == DMA_DIR_RECV)
    {
        rv = cvm_pci_dma_recv_data(&cmd, lptr, rptr);
    }
    else
    {
        rv = cvm_pci_dma_send_data(&cmd, lptr, rptr);
    }

        if(rv) {
        rv = FRE_DMA;
        goto dma_gather_err;
    }

#if defined(DMA_USE_COMPLETION_WORD)
        i = 0;
        while((*word_to_check == 0xff) && (++i < DMA_WAIT_INTERVAL)) {
                cvmx_wait(10);
                CVMX_SYNCWS;
        }

        if(i == DMA_WAIT_INTERVAL) {
                printf("DMA FAILED interval: %d  Completion Word value: %x\n", i,
                       *word_to_check);
        }
#endif

dma_gather_err:

#if defined(DMA_USE_COMPLETION_WORD)
        if(comp_ptr)
                cvm_release_dma_comp_ptr(comp_ptr);
#endif
    return rv;
}

int
frcore_dma_gather(int dir, void *local, uint64_t remote_addr, int size)
{
    int cur, rv;
    while(size)
    {
        if (size > FRCORE_DMA_BUFF_MAX)
        {
            cur = FRCORE_DMA_BUFF_MAX;
        }
        else
        {
            cur = size;
        }

        //cvmx_spinlock_lock(&dma_lock);
        rv =  _frcore_dma_gather(cur, local, remote_addr, dir);
        //cvmx_spinlock_unlock(&dma_lock);
        if (rv)
        {
            FRCORE_ERROR("_frcore_copy_to_remote fail: rv %d\n", rv);
            return rv;
        }
        size        -= cur;
        local       += cur;
        remote_addr += cur;
    }

    return FRE_SUCCESS;
}

int frcore_copy_from_remote(int size, void *local, uint64_t remote_addr)
{
    FRCORE_DMA("SIZE %d, LOCAL %p, REMOTE_ADDR %llx.\n", size, local, (ULL) remote_addr);
    return frcore_dma_gather(DMA_DIR_RECV, local, remote_addr, size);
}

int frcore_copy_to_remote(int size, void *local, uint64_t remote_addr)
{
    FRCORE_DMA("SIZE %d, LOCAL %p, REMOTE_ADDR %llx.\n", size, local, (ULL) remote_addr);
    return frcore_dma_gather(DMA_DIR_SEND, local, remote_addr, size);
}

#if FRC_CONFIG_DMA_TEST
CVMX_SHARED uint64_t remote_dma_loop_rx_addr[FRC_DAT_CORE_NUM], remote_dma_loop_tx_addr[FRC_DAT_CORE_NUM];
CVMX_SHARED frc_dma_loop_buff_t *frcore_dma_loop_rx_buff[FRC_DAT_CORE_NUM];
CVMX_SHARED frc_dma_loop_buff_t *frcore_dma_loop_tx_buff[FRC_DAT_CORE_NUM];
CVMX_SHARED uint8_t *frcore_dma_loop_temp_buff[FRC_DAT_CORE_NUM];
CVMX_SHARED cvmx_wqe_t *frcore_dma_loop_wqe;
int frcore_cmd_dma_loop_start(uint16_t plen, void *param, uint16_t *olen, void *outbuf)
{
    int grp;
    uint64_t core_num = *((uint64_t *) param);
    cvmx_wqe_t *wqe;
    *olen = 0;

    wqe = &frcore_dma_loop_wqe[core_num];
    grp = core_num + 1;
    FRCORE_CMD("WQE[%lld] = %p.\n", (ULL) core_num, wqe);
    memset(wqe, 0, sizeof(cvmx_wqe_t));
    wqe->unused   = FRCORE_WORK_DMA_LOOP;
    wqe->ipprt    = 0;
    wqe->grp      = grp;
    wqe->tag      = grp;
    wqe->tag_type = CVMX_POW_TAG_TYPE_ATOMIC;
    wqe->packet_data[0] = grp;
    if(cvmx_tim_add_entry(wqe, FRCORE_DMA_LOOP_TIMEOUT, NULL)) {
        FRCORE_ERROR("Add dma loop timer fail!.\n");
        return FRE_FAIL;
    }

    return FRE_SUCCESS;
}

#define FRCORE_DMA_LOOP_SLEEP 100000
int frcore_dma_loop(cvmx_wqe_t *work)
{
    unsigned int core_num;
    int rv;
    frc_dma_loop_buff_t *rx_buff, *tx_buff;
    uint8_t *temp_buff;
    uint64_t remote_rx_addr, remote_tx_addr;
    uint64_t rx_ridx, rx_widx, tx_ridx, tx_widx;
    uint64_t rx_offset, tx_offset;
    uint64_t tx_len = 0, tlen1 = 0, tlen2 = 0;
    core_num = cvmx_get_core_num();
    int cnt = 0;

    rx_buff = frcore_dma_loop_rx_buff[core_num];
    tx_buff = frcore_dma_loop_tx_buff[core_num];
    temp_buff = frcore_dma_loop_temp_buff[core_num];
    remote_rx_addr = remote_dma_loop_rx_addr[core_num];
    remote_tx_addr = remote_dma_loop_tx_addr[core_num];

/*
    FRCORE_LOOP("work->grp %d, RX buff %p addr %llx, TX  buff %p addr %llx.\n", work->grp,
                rx_buff, (ULL) remote_rx_addr, tx_buff, (ULL) remote_tx_addr);*/

    while (1)
    {
        FRCORE_CYCLE_RECORDER_INIT();
        rv = frcore_dma_gather(DMA_DIR_RECV, &tx_buff->widx, remote_tx_addr + 0, sizeof(tx_buff->widx));
        if (rv)
        {
            FRCORE_ERROR("DMA ridx of tx buff %d from remote fail.\n", core_num);
            FRCORE_STAT_INC(stat_dma_errors);
            rv = FRE_DMA;
            goto frcore_dma_loop_err;
        }
        FRCORE_CYCLE_RECORDING();
        tx_ridx = tx_buff->ridx;
        tx_widx = tx_buff->widx;

        swap_buff(1, &tx_ridx);
        swap_buff(1, &tx_widx);

        tx_len = tx_widx - tx_ridx;
        FRCORE_CYCLE_RECORDING();
        if (tx_len == 0)
        {
            frcore_sleep(FRCORE_DMA_LOOP_SLEEP);
            cnt = 0;
            continue;
        }

        if ((cnt < 10) && (tx_len < FRCORE_DMA_BUFF_MAX)) {
           // FRCORE_CYCLE_RECORD_DUMP();
            frcore_sleep(FRCORE_DMA_LOOP_SLEEP);
            cnt++;
            continue;
        }
        if (tx_len > FRCORE_DMA_BUFF_MAX)
        {
            tx_len = FRCORE_DMA_BUFF_MAX;
        }
        //FRCORE_LOOP("tx_ridx 0x%.16llx, tx_widx 0x%.16llx, tx_len %lld\n", (ULL) tx_ridx, (ULL) tx_widx,  (ULL) tx_len);

        while (1)
        {

            rv = frcore_dma_gather(DMA_DIR_RECV, &rx_buff->ridx, remote_rx_addr + 8, sizeof(rx_buff->ridx));
            if (rv)
            {
                FRCORE_ERROR("DMA ridx of send buff %d from remote fail.\n", core_num);
                FRCORE_STAT_INC(stat_dma_errors);
                rv = FRE_DMA;
                goto frcore_dma_loop_err;
            }
            FRCORE_CYCLE_RECORDING();
            rx_ridx = rx_buff->ridx;
            rx_widx = rx_buff->widx;

            swap_buff(1, &rx_ridx);
            swap_buff(1, &rx_widx);

            FRCORE_CYCLE_RECORDING();
            //FRCORE_LOOP("rx_ridx 0x%.16llx, rx_widx 0x%.16llx\n", (ULL) rx_widx, (ULL) rx_widx);

            if ((rx_widx - rx_ridx) <= (FRC_DMA_LOOP_BUFF_SIZE - tx_len))
            {

                tx_offset = tx_ridx % FRC_DMA_LOOP_BUFF_SIZE;
                if ((tx_offset + tx_len) > FRC_DMA_LOOP_BUFF_SIZE)
                {
                    tlen1 = FRC_DMA_LOOP_BUFF_SIZE - tx_offset;
                    rv = frcore_dma_gather(DMA_DIR_RECV, temp_buff, remote_tx_addr + FRC_DMA_LOOP_HD_SIZE + tx_offset, tlen1);
                    if (rv)
                    {
                        FRCORE_ERROR("DMA tx buff fail.\n");
                        goto frcore_dma_loop_err;
                    }

                    tlen2 = tx_len - tlen1;
                    rv = frcore_dma_gather(DMA_DIR_RECV, temp_buff + tlen1, remote_tx_addr + FRC_DMA_LOOP_HD_SIZE, tlen2);
                    if (rv)
                    {
                        FRCORE_ERROR("DMA tx buff fail.\n");
                        goto frcore_dma_loop_err;
                    }
                }
                else
                {
                    rv = frcore_dma_gather(DMA_DIR_RECV, temp_buff, remote_tx_addr + FRC_DMA_LOOP_HD_SIZE + tx_offset, tx_len);
                    if (rv)
                    {
                        FRCORE_ERROR("DMA tx buff fail.\n");
                        goto frcore_dma_loop_err;
                    }
                }
                //frc_dump_buff(tx_len, tx_buff->buff + tx_offset);
                //memcpy(rx_buff->buff + rx_offset, tx_buff->buff + tx_offset, tx_len);
                FRCORE_CYCLE_RECORDING();
                rx_offset = rx_widx % FRC_DMA_LOOP_BUFF_SIZE;
                if ((rx_offset + tx_len) > FRC_DMA_LOOP_BUFF_SIZE)
                {
                    tlen1 = FRC_DMA_LOOP_BUFF_SIZE - rx_offset;
                    rv = frcore_dma_gather(DMA_DIR_SEND, temp_buff, remote_rx_addr + FRC_DMA_LOOP_HD_SIZE + rx_offset, tlen1);
                    if (rv)
                    {
                        FRCORE_ERROR("DMA send buff fail.\n");
                        FRCORE_STAT_INC(stat_dma_errors);
                        rv = FRE_DMA;
                        goto frcore_dma_loop_err;
                    }

                    tlen2 = tx_len - tlen1;
                    rv = frcore_dma_gather(DMA_DIR_SEND, temp_buff + tlen1, remote_rx_addr + FRC_DMA_LOOP_HD_SIZE + 0, tlen2);
                    if (rv)
                    {
                        FRCORE_ERROR("DMA send buff fail.\n");
                        FRCORE_STAT_INC(stat_dma_errors);
                        rv = FRE_DMA;
                        goto frcore_dma_loop_err;
                    }
                }
                else
                {
                    rv = frcore_dma_gather(DMA_DIR_SEND, temp_buff, remote_rx_addr + FRC_DMA_LOOP_HD_SIZE + rx_offset, tx_len);
                    if (rv)
                    {
                        FRCORE_ERROR("DMA send buff fail.\n");
                        FRCORE_STAT_INC(stat_dma_errors);
                        rv = FRE_DMA;
                        goto frcore_dma_loop_err;
                    }
                }
                FRCORE_CYCLE_RECORDING();
                tx_ridx += tx_len;
                rx_widx += tx_len;
                swap_buff(1, &tx_ridx);
                swap_buff(1, &rx_widx);
                tx_buff->ridx = tx_ridx;
                rx_buff->widx = rx_widx;
                FRCORE_CYCLE_RECORDING();
                rv = frcore_dma_gather(DMA_DIR_SEND, &tx_buff->ridx, remote_tx_addr + 8, sizeof(tx_buff->ridx));
                if (rv)
                {
                    FRCORE_ERROR("DMA ridx of tx buff %d to remote fail.\n", core_num);
                    FRCORE_STAT_INC(stat_dma_errors);
                    rv = FRE_DMA;
                    goto frcore_dma_loop_err;
                }
                FRCORE_CYCLE_RECORDING();
                rv = frcore_dma_gather(DMA_DIR_SEND, &rx_buff->widx, remote_rx_addr + 0, sizeof(rx_buff->widx));
                if (rv)
                {
                    FRCORE_ERROR("DMA widx of tx buff %d to remote fail.\n", core_num);
                    FRCORE_STAT_INC(stat_dma_errors);
                    rv = FRE_DMA;
                    goto frcore_dma_loop_err;
                }
                FRCORE_CYCLE_RECORDING();
                FRCORE_STAT_ADD(stat_dma_loop_bytes, tx_len);

                FRCORE_STAT_INC(stat_dma_loop_entries);
                break;
            }
            else
            {
                frcore_sleep(FRCORE_DMA_LOOP_SLEEP);
            }
        }
        FRCORE_CYCLE_RECORD_DUMP();
    }

frcore_dma_loop_err:
    if (rv)
    {
        FRCORE_STAT_INC(stat_dma_loop_errs);
        return FRCORE_ACT_UNFREE;
    }


    if(cvmx_tim_add_entry(work, FRCORE_DMA_LOOP_TIMEOUT, NULL) ) {
        FRCORE_ERROR("Add dma loop timer fail.\n");
        return FRCORE_ACT_UNFREE;
    }

    return FRCORE_ACT_UNFREE;
}

int frcore_cmd_setup_dma_loop_buff(uint16_t plen, void *param, uint16_t *olen, void *outbuf)
{
    *olen = 0;
    frc_dma_loop_buff_addr_t *buff_addr = (frc_dma_loop_buff_addr_t *) param;

    FRCORE_CMD("core_num %lld: rx_addr 0x%llx, tx_addr 0x%llx\n", (ULL) buff_addr->core_num, (ULL) buff_addr->rx_addr, (ULL) buff_addr->tx_addr);
    /* X86's tx buff is octeon's recv buff */
    remote_dma_loop_rx_addr[buff_addr->core_num] = buff_addr->rx_addr;
    /* X86's rx buff is octeon's send buff */
    remote_dma_loop_tx_addr[buff_addr->core_num] = buff_addr->tx_addr;
#if 1
    int rv;
    rv = frcore_dma_gather(DMA_DIR_RECV, frcore_dma_loop_rx_buff[buff_addr->core_num], remote_dma_loop_rx_addr[buff_addr->core_num], FRC_DMA_BUFF_SIZE);
    if (rv)
    {
        FRCORE_ERROR("DMA loop recv buff form remote fail: rv = %d.!\n", rv);
        FRCORE_STAT_INC(stat_dma_errors);
        return FRE_DMA;
    }

    rv = frcore_dma_gather(DMA_DIR_RECV, frcore_dma_loop_tx_buff[buff_addr->core_num], remote_dma_loop_tx_addr[buff_addr->core_num], FRC_DMA_BUFF_SIZE);
    if (rv)
    {
        FRCORE_ERROR("DMA loop recv buff form remote fail: rv = %d.!\n", rv);
        FRCORE_STAT_INC(stat_dma_errors);
        return FRE_DMA;
    }
#endif


    return FRE_SUCCESS;
}
#endif
int frcore_dma_init()
{
#if FRC_CONFIG_DMA_TEST
    frcore_dma_loop_wqe = cvmx_bootmem_alloc(sizeof(cvmx_wqe_t) * FRC_DAT_CORE_NUM, 128);
    if (frcore_dma_loop_wqe == NULL)
    {
        FRCORE_ERROR("Alloc frcore_dma_loop_wqe from bootmem fail!\n");
        return FRE_MEMORY;
    }

    FRCORE_INIT("frcore_dma_loop_wqe = %p.\n", frcore_dma_loop_wqe);

    int i;

    for (i = 0; i < FRC_DAT_CORE_NUM; i++)
    {
        remote_dma_loop_rx_addr[i] = 0;
        remote_dma_loop_tx_addr[i] = 0;
        frcore_dma_loop_rx_buff[i] = cvmx_bootmem_alloc(FRC_DMA_BUFF_SIZE, 0);
        if (frcore_dma_loop_rx_buff[i] == NULL)
        {
            FRCORE_ERROR("Alloc dma loop recv buff %d fail.\n", i);
            return FRE_MEMORY;
        }

        frcore_dma_loop_tx_buff[i] = cvmx_bootmem_alloc(FRC_DMA_BUFF_SIZE, 0);
        if (frcore_dma_loop_tx_buff[i]== NULL)
        {
            FRCORE_ERROR("Alloc dma loop send buff %d fail.\n", i);
            return FRE_MEMORY;
        }

        frcore_dma_loop_temp_buff[i] = cvmx_bootmem_alloc(FRC_DMA_BUFF_SIZE, 0);
        if (frcore_dma_loop_temp_buff[i]== NULL)
        {
            FRCORE_ERROR("Alloc dma loop send buff %d fail.\n", i);
            return FRE_MEMORY;
        }
        memset(frcore_dma_loop_temp_buff[i], 0, FRC_DMA_BUFF_SIZE);
    }


    FRCORE_INIT("Alloc dma loop recv/send buff success. Buffer size %ld.\n", sizeof(frc_dma_loop_buff_t));

    FRCORE_REGISTER_CMD(CMD_TYPE_TEST, TEST_CMD_DMA_LOOP_START, frcore_cmd_dma_loop_start);
    FRCORE_REGISTER_CMD(CMD_TYPE_CTRL, CTRL_CMD_SETUP_DMA_LOOP_BUFF, frcore_cmd_setup_dma_loop_buff);
#endif
    return 0;
}
#endif
/* End of file */

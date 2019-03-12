#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include <time.h>
#include <sys/mman.h>
#include <sys/time.h>
#include <pthread.h>

#include "frctweak.h"
#include "frc_dma.h"
#include "frc_crc8.h"
#include "frc_debug.h"
#include "frc_api.h"


void frctweak_cmd_chan_usage(void)
{
    printf("%s (udp|ssn|rule) OPTIONS\n", program);
}

int frctweak_cmd_chan(int argc, char **argv)
{
   printf("%s (udp|ssn|rule) OPTIONS\n", program);
   return 0;
}

struct timeval start_time;
#define DUMP_STAT_TITLE \
  printf("  %5s  %16s  %16s  %9s  %9s  %16s  %16s  %16s %16s\n", \
         "TIME", "RX_BLOCKS", "ERR_BLOCKS", "CUR SPEED", "AVG SPEED", "AVAIL_SPACE", "COMPL_SPACE", "AVAIL_W", "COMPL_W")

#if FRC_CONFIG_SIMPLE_PACKAGE
typedef struct {
    uint64_t rx_blocks;
    uint64_t err_blocks;
} simple_package_stat_t;

typedef struct {
    uint64_t type;
    simple_package_stat_t fr_stat;
    frc_dma_chan_desc_t desc;
    frc_dma_simple_package_chan_ctrl_t *dma_ctrl; /* app addr*/
    uint8_t *pool_addr; /* app addr */
} chan_chan_t;

static pthread_t chan_udp_stat_thread_id;
static pthread_t chan_udp_data_thread_id;


simple_package_stat_t stat_now, stat_old;


/* physical addr to application address */
frc_simple_package_block_t *
chan_udp_pkt_get(uint64_t block_addr, chan_chan_t *chan)
{
    uint64_t offset;
    frc_simple_package_block_t * dma_pkt = NULL;
    offset = block_addr - chan->desc.pool_addr;
    if (offset > chan->desc.pool_size)
    {
        return NULL;
    }
    dma_pkt = (frc_simple_package_block_t *)(chan->pool_addr + offset);
    return dma_pkt;
}

/* return 1 for right;
          0 for wrong */
int
chan_udp_pkt_check(frc_simple_package_block_t *dma_pkt)
{
    uint8_t crc;
    crc = cal_crc(dma_pkt->data, sizeof(dma_pkt->data));
    if (crc)
    {
        return 0;
    }
    return 1;
}

void *chan_udp_data_thread(void *arg)
{
    uint64_t block_addr; /* driver addr */
    frc_simple_package_block_t *dma_pkt = NULL;
    struct timespec sleeptime;
    sleeptime.tv_sec = 0;
    sleeptime.tv_nsec = 10;
    chan_chan_t *udp_chan = (chan_chan_t *)arg;
    while(1)
    {
        if (SIMPLE_PACKAGE_COMPL_RING_GET(*udp_chan->dma_ctrl, block_addr))
        {
            dma_pkt = chan_udp_pkt_get(block_addr, udp_chan);
            if (dma_pkt == NULL)
            {
                continue;
            }
            /* crc check */
            if (chan_udp_pkt_check(dma_pkt))
            {
                udp_chan->fr_stat.rx_blocks++;
            } else {
                udp_chan->fr_stat.err_blocks++;
            }

            while (!SIMPLE_PACKAGE_AVAIL_RING_PUT(*udp_chan->dma_ctrl, block_addr))
            {
                nanosleep(&sleeptime, &sleeptime);
            }
        }
        else
        {
            nanosleep(&sleeptime, &sleeptime);
        }

    }

    exit(1);
    return NULL;
}

void *chan_udp_stat_thread(void *arg)
{
    uint64_t cur_speed, avg_speed, spend;
    char cur_unit, avg_unit;
    chan_chan_t *udp_chan = (chan_chan_t *)arg;
    struct timeval now;
    gettimeofday(&start_time, NULL);
    DUMP_STAT_TITLE;
    while (1)
    {
        gettimeofday(&now, NULL);
        memcpy(&stat_now, &udp_chan->fr_stat, sizeof(simple_package_stat_t));
        spend = now.tv_sec - start_time.tv_sec;
        if (spend == 0)
        {
            spend = 1;
        }
        avg_speed = (stat_now.rx_blocks * 2)/ spend;
        if (avg_speed < 1024)
        {
            avg_unit = 'K';
        }else if (avg_speed < 1024*1024)
        {
            avg_unit = 'M';
            avg_speed /=1024;
        }

        cur_speed = (stat_now.rx_blocks - stat_old.rx_blocks) * 2;
        if (cur_speed < 1024)
        {
            cur_unit = 'K';
        }else if (avg_speed < 1024*1024)
        {
            cur_unit = 'M';
            cur_speed /=1024;
        }

        printf("\r  %5ld  %16lld  %16lld  %8lld%C  %8lld%C",now.tv_sec- start_time.tv_sec,
                (ULL) stat_now.rx_blocks,(ULL)stat_now.err_blocks, (ULL)cur_speed, cur_unit, (ULL)avg_speed, avg_unit);
        fflush(stdout);
        memcpy(&stat_old, &stat_now, sizeof(simple_package_stat_t));
        sleep(1);
    }

    return NULL;
}

void frctweak_cmd_chan_udp_usage(void)
{
    printf("%s \n", program);
    printf("PR for udp start:\n");
}

int frctweak_cmd_chan_udp_start(int argc, char **argv)
{
    int mfd = 0;
    //uint16_t olen;
    int rv;
    chan_chan_t *udp_chan = NULL;
    udp_chan = malloc(sizeof(chan_chan_t));
    memset(udp_chan, 0, sizeof(chan_chan_t));
    udp_chan->type = FRC_WORK_UDP;
    //olen = sizeof(frc_dma_chan_desc_t);
    //rv = frcapi(CMD_TYPE_DRV, DRV_CMD_GET_POOL_AND_RING_ADDR, 8, &udp_chan->type, &olen, &udp_chan->desc);

    rv = frcapi_chan_or_pr_rule_or_udp_start(udp_chan->type, &udp_chan->desc);
    if (rv != FRE_SUCCESS)
    {
        FRCTWEAK_ERROR("command execute fail: %d!\n", rv);
        goto frctweak_cmd_chan_udp_start_err;
    }

    FRCTWEAK_DEBUG("udp_chan=%lld, ctlr_addr=%llx, ctrl_size=%llx,"
                   "pool_addr=%llx, pool_size=%llx\n", (ULL)udp_chan->desc.type, (ULL)udp_chan->desc.ctrl_addr,(ULL)udp_chan->desc.ctrl_size,
                   (ULL)udp_chan->desc.pool_addr, (ULL)udp_chan->desc.pool_size);
    {
        mfd = open("/dev/mem",O_RDWR);
        if (mfd < 0)
        {
            FRCTWEAK_ERROR("Can't open /dev/mem !\n");
            rv = FRE_FAIL;
            goto frctweak_cmd_chan_udp_start_err;
        }


        udp_chan->dma_ctrl = mmap(0, udp_chan->desc.ctrl_size, PROT_READ|PROT_WRITE, MAP_SHARED, mfd, udp_chan->desc.ctrl_addr);
        FRCTWEAK_DEBUG("udp_chan->dma_ctrl=%p\n", udp_chan->dma_ctrl);
        if (udp_chan->dma_ctrl == NULL)
        {
            FRCTWEAK_ERROR("mmap udp avail ring fail!\n");
            rv = FRE_MEMORY;
            goto frctweak_cmd_chan_udp_start_err;
        }

        udp_chan->pool_addr = mmap(0, udp_chan->desc.pool_size, PROT_READ, MAP_SHARED, mfd, udp_chan->desc.pool_addr);
        FRCTWEAK_DEBUG("udp_chan->pool_addr=%p\n", udp_chan->pool_addr);
        if (udp_chan->pool_addr == NULL)
        {
            FRCTWEAK_ERROR("mmap udp pool fail!\n");
            rv = FRE_MEMORY;
            goto frctweak_cmd_chan_udp_start_err;
        }

    }

    chan_udp_stat_thread_id = pthread_create(&chan_udp_stat_thread_id, NULL, chan_udp_stat_thread, (void *)udp_chan);
    chan_udp_data_thread_id = pthread_create(&chan_udp_data_thread_id, NULL, chan_udp_data_thread, (void *)udp_chan);

    while(1)
    {
        sleep(30);
    }
frctweak_cmd_chan_udp_start_err:
    if (mfd > 0)
    {
        close(mfd);
    }
    if (rv)
    {
        return FRE_FAIL;
    }
    return FRE_SUCCESS;
}

void frctweak_cmd_chan_rule_usage(void)
{
    printf("%s \n", program);
    printf("PR for rule start:\n");
}

int frctweak_cmd_chan_rule_start(int argc, char **argv)
{
    int mfd = 0;
    //uint16_t olen;
    int rv;
    chan_chan_t *udp_chan = NULL;
    udp_chan = malloc(sizeof(chan_chan_t));
    memset(udp_chan, 0, sizeof(chan_chan_t));
    udp_chan->type = FRC_WORK_RULE;
    //memset(&phy_addr, 0, sizeof(frcdrv_single_chan_addr_t));
    //memset(&simple_package_stat, 0, sizeof(simple_package_stat_t));
    //olen = sizeof(frc_dma_chan_desc_t);
    //rv = frcapi(CMD_TYPE_DRV, DRV_CMD_GET_POOL_AND_RING_ADDR, 8, &udp_chan->type, &olen, &udp_chan->desc);

    rv = frcapi_chan_or_pr_rule_or_udp_start(udp_chan->type, &udp_chan->desc);
    if (rv != FRE_SUCCESS)
    {
        FRCTWEAK_ERROR("command execute fail: %d!\n", rv);
        goto frctweak_cmd_chan_udp_start_err;
    }

    {
        //memset(&app_addr, 0, sizeof(frc_chan_single_chan_addr_t));
        mfd = open("/dev/mem",O_RDWR);
        if (mfd < 0)
        {
            FRCTWEAK_ERROR("Can't open /dev/mem !\n");
            rv = FRE_FAIL;
            goto frctweak_cmd_chan_udp_start_err;
        }


        udp_chan->dma_ctrl = mmap(0, udp_chan->desc.ctrl_size, PROT_READ|PROT_WRITE, MAP_SHARED, mfd, udp_chan->desc.ctrl_addr);
        if (udp_chan->dma_ctrl == NULL)
        {
            FRCTWEAK_ERROR("mmap udp avail ring fail!\n");
            rv = FRE_MEMORY;
            goto frctweak_cmd_chan_udp_start_err;
        }

        udp_chan->pool_addr = mmap(0, udp_chan->desc.pool_size, PROT_READ, MAP_SHARED, mfd, udp_chan->desc.pool_addr);
        if (udp_chan->pool_addr == NULL)
        {
            FRCTWEAK_ERROR("mmap udp pool fail!\n");
            rv = FRE_MEMORY;
            goto frctweak_cmd_chan_udp_start_err;
        }

    }

    //memset(&simple_package_stat, 0, sizeof(simple_package_stat_t));
    //memset(&stat_now, 0, sizeof(simple_package_stat_t));
    //memset(&stat_old, 0, sizeof(simple_package_stat_t));
    //gettimeofday(&start_time, NULL);


    chan_udp_stat_thread_id = pthread_create(&chan_udp_stat_thread_id, NULL, chan_udp_stat_thread, (void *)udp_chan);
    chan_udp_data_thread_id = pthread_create(&chan_udp_data_thread_id, NULL, chan_udp_data_thread, (void *)udp_chan);

    while(1)
    {
        sleep(30);
    }
frctweak_cmd_chan_udp_start_err:
    if (mfd > 0)
    {
        close(mfd);
    }
    if (rv)
    {
        return FRE_FAIL;
    }
    return FRE_SUCCESS;
}
#endif

#if FRC_CONFIG_SSN_CHAN
typedef struct {
    uint64_t rx_blocks;
    uint64_t err_blocks;
} ssn_stat_t;

typedef struct {
    uint64_t type;
    ssn_stat_t fr_stat;
    frc_dma_ssn_chan_desc_t desc;
    frc_ssn_ring_buff_t *available_ring;
    frc_ssn_ring_buff_t *completed_ring;
    uint8_t *pool_addr; /* app addr */
} chan_ssn_chan_t;

static pthread_t chan_ssn_stat_thread_id;
static pthread_t chan_ssn_data_thread_id;
ssn_stat_t ssn_stat_now, ssn_stat_old;
/* physical addr to application address */
frc_ssn_block_t *
chan_ssn_pkt_get(uint64_t block_addr, chan_ssn_chan_t *chan)
{
    uint64_t offset;
    frc_ssn_block_t * dma_pkt = NULL;
    offset = block_addr - chan->desc.pool_addr;
    if (offset > chan->desc.pool_size)
    {
        return NULL;
    }
    dma_pkt = (frc_ssn_block_t *)(chan->pool_addr + offset);
    return dma_pkt;
}

/* return 1 for right;
          0 for wrong */
int
chan_ssn_pkt_check(frc_ssn_block_t *dma_pkt)
{
    uint8_t crc;
    crc = cal_crc(dma_pkt->data, sizeof(dma_pkt->data)-1);
    if (crc != dma_pkt->data[FRC_DMA_SSN_DATA_SIZE-1])
    {
        return 0;
    }
    return 1;
}

void *chan_ssn_data_thread(void *arg)
{
    uint64_t block_addr; /* driver addr */
    frc_ssn_block_t *dma_pkt = NULL;
    struct timespec sleeptime;
    sleeptime.tv_sec = 0;
    sleeptime.tv_nsec = 10;
    chan_ssn_chan_t *ssn_chan = (chan_ssn_chan_t *)arg;
    while(1)
    {
        if (SSN_COMPL_RING_GET((*ssn_chan), block_addr))
        {
            dma_pkt = chan_ssn_pkt_get(block_addr, ssn_chan);
            if (dma_pkt == NULL)
            {
                continue;
            }
            #if FRC_CONFIG_SSN_CHAN_TEST_ONE_BLOCK_ONE_PAYLOD
            ssn_chan->fr_stat.rx_blocks++;
            #else
            /* crc check */
            if (chan_ssn_pkt_check(dma_pkt))
            {
                ssn_chan->fr_stat.rx_blocks++;
            } else {
                ssn_chan->fr_stat.err_blocks++;
                //FRCTWEAK_ERROR("error block_addr=0x%llx\n", block_addr);
            }
            #endif

            while (!SSN_AVAIL_RING_PUT(*ssn_chan, block_addr))
            {
                nanosleep(&sleeptime, &sleeptime);
            }
        } else
        {
            nanosleep(&sleeptime, &sleeptime);
        }
    }
}

void *chan_ssn_stat_thread(void *arg)
{
    uint64_t cur_speed, avg_speed, spend;
    char cur_unit, avg_unit;
    chan_ssn_chan_t *ssn_chan = (chan_ssn_chan_t *)arg;
    struct timeval now;
    gettimeofday(&start_time, NULL);
    DUMP_STAT_TITLE;
    while (1)
    {
        gettimeofday(&now, NULL);
        memcpy(&ssn_stat_now, &ssn_chan->fr_stat, sizeof(ssn_stat_t));
        spend = now.tv_sec - start_time.tv_sec;
        if (spend == 0)
        {
            spend = 1;
        }
        #if FRC_CONFIG_SSN_CHAN_TEST_ONE_BLOCK_ONE_PAYLOD
        avg_speed = (ssn_stat_now.rx_blocks * 512)/ spend;
        if (avg_speed < 1024)
        {
            avg_unit = 'B';
        }
        else if (avg_speed > (1024 * 1024))
        {
            avg_speed /= 1024 * 1024;
            avg_unit = 'M';
        }
        else
        {
            avg_speed /= 1024;
            avg_unit = 'K';
        }

        cur_speed = (ssn_stat_now.rx_blocks - ssn_stat_old.rx_blocks) * 512;
        if (cur_speed < 1024)
        {
            cur_unit = 'B';
        }
        else if (cur_speed > (1024 * 1024))
        {
            cur_speed /= 1024 * 1024;
            cur_unit = 'M';
        }
        else
        {
            cur_speed /= 1024;
            cur_unit = 'K';
        }
        #else
        avg_speed = (ssn_stat_now.rx_blocks * 8)/ spend;
        if (avg_speed < 1024)
        {
            avg_unit = 'K';
        }else if (avg_speed < 1024*1024)
        {
            avg_unit = 'M';
            avg_speed /=1024;
        }

        cur_speed = (ssn_stat_now.rx_blocks - ssn_stat_old.rx_blocks) * 8;
        if (cur_speed < 1024)
        {
            cur_unit = 'K';
        }else if (avg_speed < 1024*1024)
        {
            cur_unit = 'M';
            cur_speed /=1024;
        }
        #endif /* END OF FRC_CONFIG_SSN_CHAN_TEST_ONE_BLOCK_ONE_PAYLOD*/
#if 0
        printf("\r  %5ld  %16lld  %16lld  %8lld%C  %8lld%C  %16lld  %16lld",now.tv_sec- start_time.tv_sec,
                (ULL) ssn_stat_now.rx_blocks,(ULL)ssn_stat_now.err_blocks, (ULL)cur_speed, cur_unit, (ULL)avg_speed, avg_unit,
               (ULL) (SWAP_8_BYTE(ssn_chan->available_ring->widx)-SWAP_8_BYTE(ssn_chan->available_ring->ridx)),
               (ULL) (SWAP_8_BYTE(ssn_chan->completed_ring->widx)-SWAP_8_BYTE(ssn_chan->completed_ring->ridx)));
#else
        printf("\r  %5ld  %16lld  %16lld  %8lld%C  %8lld%C  %16lld  %16lld  %16lld  %16lld",now.tv_sec- start_time.tv_sec,
                (ULL) ssn_stat_now.rx_blocks,(ULL)ssn_stat_now.err_blocks, (ULL)cur_speed, cur_unit, (ULL)avg_speed, avg_unit,
               (ULL) (SWAP_8_BYTE(ssn_chan->available_ring->widx)-SWAP_8_BYTE(ssn_chan->available_ring->ridx)),
               (ULL) (SWAP_8_BYTE(ssn_chan->completed_ring->widx)-SWAP_8_BYTE(ssn_chan->completed_ring->ridx)),
               (ULL) SWAP_8_BYTE(ssn_chan->available_ring->widx), (ULL)SWAP_8_BYTE(ssn_chan->completed_ring->widx));
#endif
        fflush(stdout);
        memcpy(&ssn_stat_old, &ssn_stat_now, sizeof(ssn_stat_t));
        sleep(1);
    }

    return NULL;
}


void frctweak_cmd_chan_ssn_usage(void)
{
    printf("%s \n", program);
    printf("PR for ssn start:\n");
}

int frctweak_cmd_chan_ssn_start(int argc, char **argv)
{
    int mfd = 0;
    //uint16_t olen;
    int rv;
    chan_ssn_chan_t *ssn_chan = NULL;
    ssn_chan = malloc(sizeof(chan_ssn_chan_t));
    memset(ssn_chan, 0, sizeof(chan_ssn_chan_t));
    ssn_chan->type = FRC_WORK_SSN;
    //olen = sizeof(frc_dma_ssn_chan_desc_t);
    //rv = frcapi(CMD_TYPE_DRV, DRV_CMD_GET_POOL_AND_RING_ADDR, 8, &ssn_chan->type, &olen, &ssn_chan->desc);

    rv = frcapi_chan_or_pr_ssn_start(ssn_chan->type, &ssn_chan->desc);
    if (rv != FRE_SUCCESS)
    {
        FRCTWEAK_ERROR("command execute fail: %d!\n", rv);
        goto frctweak_cmd_chan_ssn_start_err;
    }
    FRCTWEAK_DEBUG("ssn_chan->desc.avail_addr=0x%llx, avail_size=0x%llx\n", (ULL)ssn_chan->desc.avail_addr,
                   (ULL)ssn_chan->desc.avail_size);
    FRCTWEAK_DEBUG("ssn_chan->desc.compl_addr=0x%llx, compl_size=0x%llx\n",(ULL)ssn_chan->desc.compl_addr,
                   (ULL)ssn_chan->desc.compl_size);
    FRCTWEAK_DEBUG("ssn_chan->desc.pool_addr=0x%llx, pool_size=0x%llx\n",(ULL)ssn_chan->desc.pool_addr,
                   (ULL)ssn_chan->desc.pool_size);

    {
        //memset(&app_addr, 0, sizeof(frc_chan_single_chan_addr_t));
        mfd = open("/dev/mem",O_RDWR);
        if (mfd < 0)
        {
            FRCTWEAK_ERROR("Can't open /dev/mem !\n");
            rv = FRE_FAIL;
            goto frctweak_cmd_chan_ssn_start_err;
        }

        ssn_chan->available_ring = mmap(0, ssn_chan->desc.avail_size, PROT_READ|PROT_WRITE, MAP_SHARED, mfd, ssn_chan->desc.avail_addr);
        if (ssn_chan->available_ring == NULL)
        {
            FRCTWEAK_ERROR("mmap udp avail ring fail!\n");
            rv = FRE_MEMORY;
            goto frctweak_cmd_chan_ssn_start_err;
        }

        ssn_chan->completed_ring = mmap(0, ssn_chan->desc.compl_size, PROT_READ|PROT_WRITE, MAP_SHARED, mfd, ssn_chan->desc.compl_addr);
        if (ssn_chan->completed_ring == NULL)
        {
            FRCTWEAK_ERROR("mmap udp compl ring fail!\n");
            rv = FRE_MEMORY;
            goto frctweak_cmd_chan_ssn_start_err;
        }

        ssn_chan->pool_addr = mmap(0, ssn_chan->desc.pool_size, PROT_READ, MAP_SHARED, mfd, ssn_chan->desc.pool_addr);
        if (ssn_chan->pool_addr == NULL)
        {
            FRCTWEAK_ERROR("mmap udp pool fail!\n");
            rv = FRE_MEMORY;
            goto frctweak_cmd_chan_ssn_start_err;
        }

    }

    chan_ssn_stat_thread_id = pthread_create(&chan_ssn_stat_thread_id, NULL, chan_ssn_stat_thread, (void *)ssn_chan);
    chan_ssn_data_thread_id = pthread_create(&chan_ssn_data_thread_id, NULL, chan_ssn_data_thread, (void *)ssn_chan);

    while(1)
    {
        sleep(30);
    }
frctweak_cmd_chan_ssn_start_err:
    if (mfd > 0)
    {
        close(mfd);
    }
    if (rv)
    {
        return FRE_FAIL;
    }
    return FRE_SUCCESS;
}
#endif

int frctweak_chan_cmd_init(frctweak_cmd_t *cmd)
{
    frctweak_cmd_t *chan_cmd;

    chan_cmd = frctweak_cmd_register(cmd, "chan", "chan test", frctweak_cmd_chan, frctweak_cmd_chan_usage);
#if FRC_CONFIG_SIMPLE_PACKAGE
    frctweak_cmd_register(chan_cmd, "udp",  "chan udp test.", frctweak_cmd_chan_udp_start, frctweak_cmd_chan_udp_usage);
    frctweak_cmd_register(chan_cmd, "rule", "chan rule test.", frctweak_cmd_chan_rule_start, frctweak_cmd_chan_rule_usage);
#endif
#if FRC_CONFIG_SSN_CHAN
    frctweak_cmd_register(chan_cmd, "ssn", "chan ssn test.", frctweak_cmd_chan_ssn_start, frctweak_cmd_chan_ssn_usage);
#endif
    return 0;
}

/* End of file */

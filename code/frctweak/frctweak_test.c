#include <stdlib.h>
#include <string.h>

#include <time.h>
#include <sys/mman.h>
#include <sys/time.h>
#include <pthread.h>

#include "frctweak.h"
#include "frc_dma.h"



void frctweak_cmd_test_usage(void)
{
    printf("%s (pkt|dma|chan) OPTIONS\n", program);
}

int frctweak_cmd_test(int argc, char **argv)
{
   printf("%s (pkt|dma|chan) OPTIONS\n", program);
   return 0;
}
#if FRC_CONFIG_QUEUE_TEST
void frctweak_cmd_test_dma_pkt_usage(void)
{
    printf("%s (driver|core) (tcp|udp) NUMBER [LENGTH]\n", program);
    printf("\nOPTIONS:\n");
    printf("  %-16s --%s", "dirver", "Generate data from driver.\n");
    printf("  %-16s --%s", "core", "Generate data from driver.\n");
    printf("  %-16s --%s", "tcp", "Generate tcp data.\n");
    printf("  %-16s --%s", "udp", "Generate udp data.\n");
    printf("  %-16s --%s", "NUMBER", "Number of block.\n");
    printf("  %-16s --%s", "LENGTH", "Length of pakcet.\n");
}

int frctweak_cmd_test_dma_pkt(int argc, char **argv)
{
    int rv;
    uint16_t olen;
    frc_pkt_dma_test_t test;
    uint16_t type;
    uint16_t cmd;

    if ((argc != 4) || (argc != 5))
    {
        return FRE_PARAM;
    }

    if (!strcmp("driver", argv[1]))
    {
        type = CMD_TYPE_DRV;
        if (!strcmp("tcp", argv[2]))
        {
            cmd = DRV_CMD_TEST_TCP;
        }
        else if(!strcmp("udp", argv[2]))
        {
            cmd = DRV_CMD_TEST_UDP;
        }
        else
        {
            return FRE_PARAM;
        }
    }
    else if (!strcmp("core", argv[1]))
    {
        type = CMD_TYPE_TEST;
        if (!strcmp("tcp", argv[2]))
        {
            cmd = TEST_CMD_TCP_DMA;
        }
        else if(!strcmp("udp", argv[2]))
        {
            cmd = TEST_CMD_UDP_DMA;
        }
        else
        {
            return FRE_PARAM;
        }
    }
    else
    {
        return FRE_PARAM;
    }

    memset(&test, 0, sizeof(frc_pkt_dma_test_t));

    test.num = atoi(argv[3]);

    olen = sizeof(frc_pkt_dma_test_t);

    if (argc == 5)
    {
        test.len = atoi(argv[4]);
    }

    rv = frcapi(type, cmd, sizeof(frc_pkt_dma_test_t), &test, NULL, NULL);
    if (rv != FRE_SUCCESS)
    {
        FRCTWEAK_ERROR("command execute fail: %d!\n", rv);
        return rv;
    }

    return FRE_SUCCESS;
}
#endif
#if FRC_CONFIG_DMA_TEST
#define DMA_LOOP_VAILD_DATA 1
#define DMA_LOOP_SLEEP 1
typedef struct {
    uint64_t rx_entries;
    uint64_t rx_bytes;
    uint64_t tx_entries;
    uint64_t tx_bytes;
    uint64_t err_bytes;
} dma_loop_stat_t;

dma_loop_stat_t dma_loop_stat;
static uint32_t dma_loop_entries = 0;
static int core_num = 0;
static int pkt_len = 0;
frc_dma_loop_buff_t *frctweak_dma_loop_tx_buff = NULL, *frctweak_dma_loop_rx_buff = NULL;

#define DUMP_STAT_TITLE \
  printf("  %5s  %16s  %16s  %16s  %16s  %16s  %9s  %9s\n", \
         "TIME", "RX_ENTRIES", "RX_BYTES", "TX_ENTRIES", "TX_BYTES", "ERR_BYTES", "CUR SPEED", "AVG SPEED")

dma_loop_stat_t stat_now, stat_old;
struct timeval start_time;


void dma_stat_dump()
{
    uint64_t cur_speed, avg_speed, spend;
    char cur_unit, avg_unit;
    struct timeval now;
    gettimeofday(&now, NULL);

    memcpy(&stat_now, &dma_loop_stat, sizeof(dma_loop_stat_t));

    spend = now.tv_sec - start_time.tv_sec;
    if (spend == 0)
    {
        spend = 1;
    }
    avg_speed = stat_now.rx_bytes / spend;
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

    cur_speed = stat_now.rx_bytes - stat_old.rx_bytes;
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
    printf("\r  %5ld  %16lld  %16lld  %16lld  %16lld  %16lld  %8lld%C  %8lld%C", now.tv_sec- start_time.tv_sec,
           (ULL) stat_now.rx_entries, (ULL) stat_now.rx_bytes,
           (ULL) stat_now.tx_entries, (ULL) stat_now.tx_bytes,
           (ULL) stat_now.err_bytes,  (ULL) cur_speed, cur_unit, (ULL) avg_speed, avg_unit);
    fflush(stdout);
    memcpy(&stat_old, &stat_now, sizeof(dma_loop_stat_t));
}

void dma_stat_loop()
{

    DUMP_STAT_TITLE;

    while (1)
    {
        dma_stat_dump();
        if (dma_loop_stat.rx_entries == dma_loop_entries)
        {
            printf("\n");
            break;
        }
        sleep(1);
    }
}




void dma_rx(int len, uint8_t value)
{
    int i;

    frc_dma_loop_buff_t *rx_buff;
    uint64_t widx, ridx, offset;
    uint8_t *bufp;
    dma_loop_stat_t *stat;
    int len1, len2;

    struct timespec sleeptime;
    sleeptime.tv_sec = 0;
    sleeptime.tv_nsec = 10;

    rx_buff = frctweak_dma_loop_rx_buff;
    stat    = &dma_loop_stat;

    while (1)
    {
        widx = rx_buff->widx;
        ridx = rx_buff->ridx;

        if ((widx - ridx) >= len)
        {
        #if DMA_LOOP_VAILD_DATA
            offset = ridx % FRC_DMA_LOOP_BUFF_SIZE;

            if ((offset + len) > FRC_DMA_LOOP_BUFF_SIZE)
            {
                bufp = rx_buff->buff + offset;
                len1 = FRC_DMA_LOOP_BUFF_SIZE - offset;
                for (i = 0; i < len1; i++)
                {
                    if ((bufp[i] & 0xff) != value)
                    {
                        //printf("bufp[%d] = 0x%x, value = 0x%.x.\n", i, bufp[i], value);
                        stat->err_bytes++;
                    }
                }

                bufp = rx_buff->buff;
                len2 = len - len1;
                for (i = 0; i < len2; i++)
                {
                    if ((bufp[i] & 0xff) != value)
                    {
                        //printf("bufp[%d] = 0x%x, value = 0x%.x.\n", i, bufp[i], value);
                        stat->err_bytes++;
                    }
                }
            }
            else
            {
                bufp = rx_buff->buff + offset;
                for (i = 0; i < len; i++)
                {
                    if ((bufp[i] & 0xff) != value)
                    {
                        //printf("bufp[%d] = 0x%x, value = 0x%.x.\n", i, bufp[i], value);
                        stat->err_bytes++;
                    }
                }
            }
        #endif
            rx_buff->ridx    += len;
            stat->rx_bytes   += len;
            stat->rx_entries += 1;
            break;
        }
    #if DMA_LOOP_SLEEP
        else {
            nanosleep(&sleeptime, &sleeptime);
        }
    #endif
    }
}

void dma_tx(int len, uint8_t value)
{
    frc_dma_loop_buff_t *tx_buff;
    uint64_t widx, ridx, offset, len1, len2;
    uint8_t *bufp;
    dma_loop_stat_t *stat;

    struct timespec sleeptime;
    sleeptime.tv_sec = 0;
    sleeptime.tv_nsec = 10;

    tx_buff = frctweak_dma_loop_tx_buff;
    stat    = &dma_loop_stat;

    while (1)
    {
        widx = tx_buff->widx;
        ridx = tx_buff->ridx;

        if ((widx - ridx) < (FRC_DMA_LOOP_BUFF_SIZE - len))
        {
        #if DMA_LOOP_VAILD_DATA
            offset = widx % FRC_DMA_LOOP_BUFF_SIZE;

            bufp = tx_buff->buff + offset;
            if ((offset + len) > FRC_DMA_LOOP_BUFF_SIZE)
            {
                len1 = FRC_DMA_LOOP_BUFF_SIZE - offset;
                memset(bufp, value, len1);
                len2 = len - len1;
                bufp = tx_buff->buff;
                memset(bufp, value, len2);
            }
            else
            {
                memset(bufp, value, len);
            }
        #endif
            tx_buff->widx    = widx + len;
            stat->tx_bytes   += len;
            stat->tx_entries += 1;
            break;
        }
    #if DMA_LOOP_SLEEP
        else
        {
            nanosleep(&sleeptime, &sleeptime);
        }
    #endif
    }
}

void dma_test_loop(int len, uint32_t entires)
{
    uint32_t i;
    uint8_t value;

    long old_sec;
    struct timeval now;
    gettimeofday(&now, NULL);
    old_sec = now.tv_sec;
    DUMP_STAT_TITLE;
    for (i = 0; i < entires; i++)
    {
        if (i % 2)
        {
            value = 0X5A;
        }
        else
        {
            value = 0xA5;
        }

        dma_tx(len, value);
        dma_rx(len, value);

        if ((i & 0xf) == 0)
        {
            gettimeofday(&now, NULL);
            if (now.tv_sec - old_sec)
            {
                dma_stat_dump();
                old_sec = now.tv_sec;
            }
        }
    }
    dma_stat_dump();
    printf(".\n");
}

void *dma_rx_thread(void* arg)
{
    uint32_t remain = dma_loop_entries;
    uint8_t value;

    while (remain)
    {
        if (remain & 0x1)
        {
            value = 0x5a;
        }
        else
        {
            value = 0xa5;
        }
        dma_rx(pkt_len, value);
        remain--;
    }

    return NULL;
}

void *dma_tx_thread(void* arg)
{
    uint32_t remain = dma_loop_entries;
    uint32_t value = 0;
    while (remain)
    {
        if (remain & 0x1)
        {
            value = 0x5a;
        }
        else
        {
            value = 0xa5;
        }
        dma_tx(pkt_len, value);
        remain--;

    }

    return NULL;
}

void frctweak_cmd_test_dma_loop_usage(void)
{
    printf("%s (LEN ENTRIES CORE)\n", program);
    printf("\nOPTIONS:\n");
    printf("  %-16s --%s", "LEN", "Len of packet.\n");
    printf("  %-16s --%s", "ENTRIES", "Number of entries data loop. Entry size 1024 Bytes.\n");
    printf("  %-16s --%s", "CORE", "Index of core.\n");
}

int frctweak_cmd_test_dma_loop(int argc, char **argv)
{
    int mfd = 0;
    frc_dma_loop_arg_t loop_arg;
    uint16_t olen;
    int rv;
    uint64_t cid;

    if (argc != 4)
    {
        return FRE_PARAM;
    }

    pkt_len = atoi(argv[1]);
    dma_loop_entries = atoi(argv[2]);
    core_num = atoi(argv[3]);

    cid = core_num;
    olen = 0;
    rv = frcapi(CMD_TYPE_TEST, TEST_CMD_DMA_LOOP_START, 8, &cid, &olen, NULL);
    if (rv != FRE_SUCCESS)
    {
        FRCTWEAK_ERROR("command execute fail: %d!\n", rv);
        return 1;
    }


    memset(&loop_arg, 0, sizeof(frc_dma_loop_arg_t));

    olen = sizeof(frc_dma_loop_arg_t);
    rv = frcapi(CMD_TYPE_DRV, DRV_CMD_DMA_LOOP_BUFF_GET, 0, NULL, &olen, &loop_arg);
    if (rv != FRE_SUCCESS)
    {
        FRCTWEAK_ERROR("command execute fail: %d!\n", rv);
        goto frctweak_cmd_test_dma_loop_err;
    }

    {

        mfd = open("/dev/mem",O_RDWR);
        if (mfd < 0)
        {
            FRCTWEAK_ERROR("Can't open /dev/mem !\n");
            rv = FRE_FAIL;
            goto frctweak_cmd_test_dma_loop_err;
        }


        frctweak_dma_loop_rx_buff = mmap(0, FRC_DMA_BUFF_SIZE, PROT_READ|PROT_WRITE, MAP_SHARED, mfd, loop_arg.rx_addr[core_num]);
        if (frctweak_dma_loop_rx_buff == NULL)
        {
            FRCTWEAK_ERROR("mmap dma loop rx buff %d fail!\n", core_num);
            rv = FRE_MEMORY;
            goto frctweak_cmd_test_dma_loop_err;
        }

        frctweak_dma_loop_tx_buff = mmap(0, FRC_DMA_BUFF_SIZE, PROT_READ|PROT_WRITE, MAP_SHARED, mfd, loop_arg.tx_addr[core_num]);
        if (frctweak_dma_loop_tx_buff == NULL)
        {
            FRCTWEAK_ERROR("mmap dma loop tx buff %d fail!\n", core_num);
            rv = FRE_MEMORY;
            goto frctweak_cmd_test_dma_loop_err;
        }

    }

    memset(&dma_loop_stat, 0, sizeof(dma_loop_stat_t));
    memset(&stat_now, 0, sizeof(dma_loop_stat_t));
    memset(&stat_old, 0, sizeof(dma_loop_stat_t));
    gettimeofday(&start_time, NULL);
#if 0
    dma_test_loop(pkt_len, dma_loop_entries);
#else
    pthread_t tx_thread, rx_thread;
    rx_thread = pthread_create(&rx_thread, NULL, dma_rx_thread, &dma_loop_entries);
    tx_thread = pthread_create(&tx_thread, NULL, dma_tx_thread, &dma_loop_entries);

    dma_stat_loop();
#endif
frctweak_cmd_test_dma_loop_err:
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

#if FRC_CONFIG_SIMPLE_PACKET_TEST || FRC_CONFIG_SSN_CHAN_TEST
void frctweak_cmd_test_chan_usage(void)
{
    printf("%s (CHAN)\n", program);
    printf("\nOPTIONS:\n");
    printf("  %-16s --%s", "CHAN", "CHAN ID, 0 for UDP, 1 for RULE, 2 for SSN.\n");
}

int frctweak_cmd_test_chan(int argc, char **argv)
{
    uint16_t olen;
    int rv;
    uint64_t chan;

    if (argc != 2)
    {
        return FRE_PARAM;
    }

    chan = atoi(argv[1]);

    rv = frcapi_chan_test_start(&chan, &olen);
    if (rv != FRE_SUCCESS)
    {
        FRCTWEAK_ERROR("command execute fail: %d!\n", rv);
        return 1;
    }
    return FRE_SUCCESS;
}
#endif

int frctweak_test_cmd_init(frctweak_cmd_t *cmd)
{
    #if (FRC_CONFIG_QUEUE_TEST | FRC_CONFIG_DMA_TEST | FRC_CONFIG_SIMPLE_PACKET_TEST | FRC_CONFIG_SSN_CHAN_TEST )
    frctweak_cmd_t *test_cmd;

    test_cmd = frctweak_cmd_register(cmd, "test", "Do some test", frctweak_cmd_test, frctweak_cmd_test_usage);
    #endif
#if FRC_CONFIG_QUEUE_TEST
    frctweak_cmd_register(test_cmd, "pkt", "Test dma pkt transfer.", frctweak_cmd_test_dma_pkt, frctweak_cmd_test_dma_pkt_usage);
#endif
#if FRC_CONFIG_DMA_TEST
    frctweak_cmd_register(test_cmd, "dma", "DMA loop test.", frctweak_cmd_test_dma_loop, frctweak_cmd_test_dma_loop_usage);
#endif
#if FRC_CONFIG_SIMPLE_PACKET_TEST || FRC_CONFIG_SSN_CHAN_TEST
    frctweak_cmd_register(test_cmd, "chan", "DMA loop test.", frctweak_cmd_test_chan, frctweak_cmd_test_chan_usage);
#endif
    return 0;
}

/* End of file */

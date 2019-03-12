#include <stdlib.h>
#include <string.h>

#include <time.h>
#include <sys/mman.h>
#include <sys/time.h>
#include <pthread.h>

#include "frctweak.h"
#include "frc_dma.h"
#include "frc_crc8.h"
#include "frc_util.h"
#include "frc_api.h"

#define FRC_MAC_STRING_SIZE 40

void frctweak_cmd_pr_usage(void)
{
    printf("%s (udp|ssn|rule) OPTIONS\n", program);
}

int frctweak_cmd_pr(int argc, char **argv)
{
   printf("%s (udp|ssn|rule) OPTIONS\n", program);
   return 0;
}

void ip_string(uint32_t ip, char *str)
{
    uint8_t *ipc = (uint8_t *)&ip;
    sprintf(str, "%u.%u.%u.%u", ipc[3], ipc[2], ipc[1], ipc[0]);
}

void proto_string(uint16_t proto, char *str)
{
    switch (proto)
    {
    case PROTO_TCP:
        sprintf(str, "TCP");
        break;
    case PROTO_UDP:
        sprintf(str, "UDP");
        break;
    case PROTO_SCTP:
        sprintf(str, "SCTP");
        break;
    case PROTO_ICMP:
        sprintf(str, "ICMP");
        break;
    default:
        sprintf(str, "UNKOWN");
        break;
    }
}

void
frc_dma_header_dump(uint64_t chan_id, frc_dma_hdr_t *header)
{
    char sip[20], dip[20], proto[8];
    ip_string(header->sip, sip);
    ip_string(header->dip, dip);
    proto_string(header->protocol, proto);

    printf("  CHAN %lld,%u  %s %s->%s %d:%d; %d pkts %d bytes, hash 0x%x.\n",
           (ULL) chan_id, header->stop_sec, proto, sip, dip, header->sport, header->dport,
           header->pkt_num, header->total_paylen, header->hash);
}

void
frc_mac_string(uint64_t mac, char *str)
{
    uint8_t *cmac;
    cmac = (uint8_t *) &mac;
    memset(str, 0, FRC_MAC_STRING_SIZE);

    sprintf(str, "%.2x:%.2x:%.2x:%.2x:%.2x:%.2x",
            cmac[0], cmac[1], cmac[2], cmac[3], cmac[4], cmac[5]);
}

void
frc_pkt_info_dump(frc_dma_pkt_info_t *pkt_info)
{
    char smac_str[FRC_MAC_STRING_SIZE];
    char dmac_str[FRC_MAC_STRING_SIZE];

    frc_mac_string(pkt_info->smac, smac_str);
    frc_mac_string(pkt_info->dmac, dmac_str);
    printf("INFO: %s -> %s 0x%.8x:0x%.8x offset %d paylen %d dir %d\n",
           smac_str, dmac_str,pkt_info->sequence, pkt_info->ack_seq,
           pkt_info->data_offset, pkt_info->payload_len, pkt_info->direction);
}

void
frc_pkt_payload_dump(int paylen, uint8_t *payload)
{
    int i;
    printf("PAYLOAD: %d bytes.\n", paylen);

    for (i = 0; i < paylen; i++)
    {
        if ((i % 16) == 0)
        {
            printf(" %.4x:", i);
        }
        printf(" %.2x", payload[i]);
        if ((i % 16) == 15)
        {
            printf("\n");
        }
    }
    printf("\n");
}

void frc_speed_str(uint64_t val, char *str, char *prefix)
{

    if (val > (10 * 1024 * 1024))
    {
        sprintf(str, "%lldM%s", (ULL) (val / (1024 * 1024)), prefix);
    }
    else if (val > (10 *1024))
    {
        sprintf(str, "%lldK%s", (ULL) (val / 1024), prefix);
    }
    else
    {
        sprintf(str, "%lld%s", (ULL) val, prefix);
    }
}
#define DUMP_STAT_TITLE \
  printf("  %5s  %16s  %16s  %9s  %9s  %16s  %16s  %16s %16s\n", \
         "TIME", "RX_BLOCKS", "ERR_BLOCKS", "CUR SPEED", "AVG SPEED", "AVAIL_SPACE", "COMPL_SPACE", "AVAIL_W", "COMPL_W")
struct timeval start_time;
static int dump_block   = 0;
static int dump_info    = 0;
static int dump_packet  = 0;
static int dump_err_addr= 0;

#if FRC_CONFIG_SIMPLE_PACKAGE
typedef struct {
    uint64_t rx_blocks;
    uint64_t rx_pkts;
    uint64_t rx_bytes;
    uint64_t rx_loss;
    uint64_t rx_positive;
    uint64_t rx_reverse;
    uint64_t rx_errors;

    uint64_t tx_block;
    uint64_t tx_pkts;
    uint64_t tx_bytes;
    uint64_t tx_error;
    uint64_t tx_positive;
    uint64_t tx_reverse;
} simple_package_stat_t;

typedef struct {
    uint64_t type;
    simple_package_stat_t fr_stat;
    frc_dma_chan_desc_t desc;
    frc_dma_simple_package_chan_ctrl_t *dma_ctrl; /* app addr*/
    uint8_t *pool_addr; /* app addr */
} pr_chan_t;

static pthread_t pr_udp_stat_thread_id;
static pthread_t pr_udp_data_thread_id;

simple_package_stat_t stat_now, stat_old;


/* physical addr to application address */
frc_simple_package_block_t *
pr_udp_pkt_get(uint64_t block_addr, pr_chan_t *chan)
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

/*
   return 0 for success;
   return 1 for failed
 */
int
pr_simple_pkt_get(frc_simple_package_block_t *dma_pkt, frc_simple_pkt_t *simple_pkt)
{
    if (dma_pkt == NULL || simple_pkt == NULL)
    {
        return FRE_FAIL;
    }
    memset(simple_pkt, 0, sizeof(frc_simple_pkt_t));
    memcpy(&simple_pkt->header, dma_pkt->data, FRC_DMA_PKT_HEAD_SIZE);
    swap_buff(FRC_DMA_PKT_HEAD_SIZE >>3, &simple_pkt->header);
    memcpy(simple_pkt->payload, &dma_pkt->data[FRC_DMA_PKT_PAYLOAD_OFFSET], simple_pkt->header.total_paylen);
    memcpy(&simple_pkt->info, &dma_pkt->data[FRC_DMA_PKT_PAYLOAD_OFFSET + simple_pkt->header.total_paylen], FRC_DMA_PKT_INFO_SIZE);
    swap_buff(FRC_DMA_PKT_INFO_SIZE >>3, &simple_pkt->info);
    return FRE_SUCCESS;
}

void
pr_simple_pkt_process(pr_chan_t *chan, frc_simple_pkt_t *simple_pkt)
{
    frc_dma_pkt_info_t *pkt_info;

    chan->fr_stat.rx_blocks++;

    if (dump_block)
    {
        frc_dma_header_dump(chan->desc.type, &simple_pkt->header);
    }

    pkt_info = &simple_pkt->info;

    chan->fr_stat.rx_pkts++;
    chan->fr_stat.rx_bytes += simple_pkt->header.total_paylen;

    if (dump_info)
    {
         frc_pkt_info_dump(pkt_info);
    }

    if (dump_packet)
    {
         frc_pkt_payload_dump(simple_pkt->header.total_paylen, simple_pkt->payload);
    }
}

void *pr_udp_data_thread(void *arg)
{
    uint64_t block_addr; /* driver addr */
    frc_simple_package_block_t *dma_pkt = NULL;
    frc_simple_pkt_t  simple_pkt;
    struct timespec sleeptime;
    sleeptime.tv_sec = 0;
    sleeptime.tv_nsec = 10;
    pr_chan_t *udp_chan = (pr_chan_t *)arg;
    while(1)
    {
        if (SIMPLE_PACKAGE_COMPL_RING_GET(*udp_chan->dma_ctrl, block_addr))
        {
            dma_pkt = pr_udp_pkt_get(block_addr, udp_chan);
            if (dma_pkt == NULL)
            {
                udp_chan->fr_stat.rx_errors++;
                continue;
            }
            /**/if (pr_simple_pkt_get(dma_pkt, &simple_pkt))
            {
                udp_chan->fr_stat.rx_errors++;
                continue;
            }
            #if FRC_DEBUG_PKT_LEN
                    frc_dump_buff(FRC_DMA_SIMPLE_PACKAGE_DATA_SIZE, dma_pkt->data);
                    frc_dump_dma_hdr(&simple_pkt.header);
                    frc_dump_dma_pkt_info(&simple_pkt.info);
                    frc_dump_buff(simple_pkt.header.total_paylen, simple_pkt.payload);

            #endif
            pr_simple_pkt_process(udp_chan, &simple_pkt);

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

void *pr_udp_stat_thread(void *arg)
{
    int sec = 0;
    printf("  %5s  %10s  %10s %8s  %10s %8s  %10s\n", "TIME", "RX_BLOCKS", "RX_PKTS", "SPEED", "RX_BYTES", "SPEED", "RX_ERRORS");
    uint64_t rx_pkts, rx_bytes, rx_blocks, rx_errors;
    uint64_t rx_bytes_old = 0, rx_pkts_old =0, rx_bytes_speed = 0, rx_pkts_speed = 0;
    char rx_bytes_speed_str[10];
    char rx_pkts_speed_str[10];
    pr_chan_t *chan = (pr_chan_t *) arg;

    while (1)
    {
        rx_blocks = chan->fr_stat.rx_blocks;
        rx_pkts   = chan->fr_stat.rx_pkts;
        rx_bytes  = chan->fr_stat.rx_bytes;
        rx_errors = chan->fr_stat.rx_errors;

        rx_bytes_speed = rx_bytes - rx_bytes_old;
        rx_pkts_speed = rx_pkts - rx_pkts_old;

        frc_speed_str(rx_bytes_speed, rx_bytes_speed_str, "Bps");
        frc_speed_str(rx_pkts_speed, rx_pkts_speed_str, "pps");
        printf("\r  %5d  %10lld  %10lld %8s  %10lld %8s  %10lld", sec,
               (ULL) rx_blocks, (ULL) rx_pkts, rx_pkts_speed_str,
               (ULL) rx_bytes, rx_bytes_speed_str, (ULL) rx_errors);
        fflush(stdout);

        sec++;
        rx_bytes_old = rx_bytes;
        rx_pkts_old  = rx_pkts;
        sleep(1);
    }

    return NULL;
}

void frctweak_cmd_pr_udp_usage(void)
{
    printf("Usage: %s [-bipch].\n", program);
    printf("          -b        -- Dump block info.\n");
    printf("          -i        -- Dump pkt info.\n");
    printf("          -p        -- Dump packet data.\n");
    printf("          -c        -- Dump counter.\n");
    printf("          -h        -- Printf help message.\n");

    exit(0);
}

int frctweak_cmd_pr_udp_start(int argc, char **argv)
{
    int mfd = 0;
    //uint16_t olen;
    int rv;
    pr_chan_t *udp_chan = NULL;
    int opt;
    int dump_count = 0;
    if (argc == 1)
    {
        frctweak_cmd_pr_udp_usage();
        return FRE_SUCCESS;
    }
    while ((opt = getopt(argc, argv, "bfipcvo:TURh?")) != -1) {
        switch (opt) {
        case 'b':
            dump_block  = 1;
            break;
        case 'i':
            dump_info  = 1;
            break;
        case 'p':
            dump_packet = 1;
            break;
        case 'c':
            dump_count = 1;
            break;
        case 'h':
        case '?':
            frctweak_cmd_pr_udp_usage();
            return FRE_SUCCESS;
            break;
        }
    }

    udp_chan = malloc(sizeof(pr_chan_t));
    memset(udp_chan, 0, sizeof(pr_chan_t));
    udp_chan->type = FRC_WORK_UDP;
    //olen = sizeof(frc_dma_chan_desc_t);
    //rv = frcapi(CMD_TYPE_DRV, DRV_CMD_GET_POOL_AND_RING_ADDR, 8, &udp_chan->type, &olen, &udp_chan->desc);

    rv = frcapi_chan_or_pr_rule_or_udp_start(udp_chan->type, &udp_chan->desc);
    if (rv != FRE_SUCCESS)
    {
        FRCTWEAK_ERROR("command execute fail: %d!\n", rv);
        goto frctweak_cmd_pr_udp_start_err;
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
            goto frctweak_cmd_pr_udp_start_err;
        }


        udp_chan->dma_ctrl = mmap(0, udp_chan->desc.ctrl_size, PROT_READ|PROT_WRITE, MAP_SHARED, mfd, udp_chan->desc.ctrl_addr);
        FRCTWEAK_DEBUG("udp_chan->dma_ctrl=%p\n", udp_chan->dma_ctrl);
        if (udp_chan->dma_ctrl == NULL)
        {
            FRCTWEAK_ERROR("mmap udp avail ring fail!\n");
            rv = FRE_MEMORY;
            goto frctweak_cmd_pr_udp_start_err;
        }

        udp_chan->pool_addr = mmap(0, udp_chan->desc.pool_size, PROT_READ, MAP_SHARED, mfd, udp_chan->desc.pool_addr);
        FRCTWEAK_DEBUG("udp_chan->pool_addr=%p\n", udp_chan->pool_addr);
        if (udp_chan->pool_addr == NULL)
        {
            FRCTWEAK_ERROR("mmap udp pool fail!\n");
            rv = FRE_MEMORY;
            goto frctweak_cmd_pr_udp_start_err;
        }

    }

    if (dump_count && !dump_block && !dump_info && !dump_packet)
    {
        pr_udp_stat_thread_id = pthread_create(&pr_udp_stat_thread_id, NULL, pr_udp_stat_thread, (void *)udp_chan);
    }
    pr_udp_data_thread_id = pthread_create(&pr_udp_data_thread_id, NULL, pr_udp_data_thread, (void *)udp_chan);

    while(1)
    {
        sleep(30);
    }
frctweak_cmd_pr_udp_start_err:
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


/*
   return 0 for success;
   return 1 for failed
 */
int
pr_simple_rule_pkt_get(frc_simple_package_block_t *dma_pkt, frc_simple_rule_pkt_t *simple_pkt)
{
    if (dma_pkt == NULL || simple_pkt == NULL)
    {
        return FRE_FAIL;
    }
    memset(simple_pkt, 0, sizeof(frc_simple_rule_pkt_t));
    memcpy(&simple_pkt->header, dma_pkt->data, FRC_DMA_PKT_HEAD_SIZE);
    swap_buff(FRC_DMA_PKT_HEAD_SIZE >>3, &simple_pkt->header);
    memcpy(&simple_pkt->rule_id, &dma_pkt->data[FRC_DMA_PKT_PAYLOAD_OFFSET], 2);
    simple_pkt->rule_id = SWAP_2_BYTE(simple_pkt->rule_id);
    memcpy(simple_pkt->payload, &dma_pkt->data[FRC_DMA_PKT_PAYLOAD_OFFSET+2], simple_pkt->header.total_paylen);
    memcpy(&simple_pkt->info, &dma_pkt->data[FRC_DMA_PKT_PAYLOAD_OFFSET + simple_pkt->header.total_paylen +2], FRC_DMA_PKT_INFO_SIZE);
    swap_buff(FRC_DMA_PKT_INFO_SIZE >>3, &simple_pkt->info);
    return FRE_SUCCESS;
}

void
pr_simple_rule_pkt_process(pr_chan_t *chan, frc_simple_rule_pkt_t *simple_pkt)
{
    frc_dma_pkt_info_t *pkt_info;

    chan->fr_stat.rx_blocks++;

    if (dump_block)
    {
        frc_dma_header_dump(chan->desc.type, &simple_pkt->header);
    }

    pkt_info = &simple_pkt->info;

    chan->fr_stat.rx_pkts++;
    chan->fr_stat.rx_bytes += simple_pkt->header.total_paylen;

    if (dump_info)
    {
         frc_pkt_info_dump(pkt_info);
    }

    if (dump_packet)
    {
         printf("Rule Index: %d\n", simple_pkt->rule_id);
         frc_pkt_payload_dump(simple_pkt->header.total_paylen, simple_pkt->payload);
    }
}


void *pr_rule_data_thread(void *arg)
{
    uint64_t block_addr; /* driver addr */
    frc_simple_package_block_t *dma_pkt = NULL;
    frc_simple_rule_pkt_t  simple_pkt;
    struct timespec sleeptime;
    sleeptime.tv_sec = 0;
    sleeptime.tv_nsec = 10;
    pr_chan_t *udp_chan = (pr_chan_t *)arg;
    while(1)
    {
        if (SIMPLE_PACKAGE_COMPL_RING_GET(*udp_chan->dma_ctrl, block_addr))
        {
            dma_pkt = pr_udp_pkt_get(block_addr, udp_chan);
            if (dma_pkt == NULL)
            {
                udp_chan->fr_stat.rx_errors++;
                continue;
            }
            /**/if (pr_simple_rule_pkt_get(dma_pkt, &simple_pkt))
            {
                udp_chan->fr_stat.rx_errors++;
                continue;
            }
            #if FRC_DEBUG_PKT_LEN
                    frc_dump_buff(FRC_DMA_SIMPLE_PACKAGE_DATA_SIZE, dma_pkt->data);
                    frc_dump_dma_hdr(&simple_pkt.header);
                    frc_dump_dma_pkt_info(&simple_pkt.info);
                    frc_dump_buff(simple_pkt.header.total_paylen, simple_pkt.payload);

            #endif
            pr_simple_rule_pkt_process(udp_chan, &simple_pkt);

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

void *pr_rule_stat_thread(void *arg)
{
    int sec = 0;
    printf("  %5s  %10s  %10s %8s  %10s %8s  %10s\n", "TIME", "RX_BLOCKS", "RX_PKTS", "SPEED", "RX_BYTES", "SPEED", "RX_ERRORS");
    uint64_t rx_pkts, rx_bytes, rx_blocks, rx_errors;
    uint64_t rx_bytes_old = 0, rx_pkts_old =0, rx_bytes_speed = 0, rx_pkts_speed = 0;
    char rx_bytes_speed_str[10];
    char rx_pkts_speed_str[10];
    pr_chan_t *chan = (pr_chan_t *) arg;

    while (1)
    {
        rx_blocks = chan->fr_stat.rx_blocks;
        rx_pkts   = chan->fr_stat.rx_pkts;
        rx_bytes  = chan->fr_stat.rx_bytes;
        rx_errors = chan->fr_stat.rx_errors;

        rx_bytes_speed = rx_bytes - rx_bytes_old;
        rx_pkts_speed = rx_pkts - rx_pkts_old;

        frc_speed_str(rx_bytes_speed, rx_bytes_speed_str, "Bps");
        frc_speed_str(rx_pkts_speed, rx_pkts_speed_str, "pps");
        printf("\r  %5d  %10lld  %10lld %8s  %10lld %8s  %10lld", sec,
               (ULL) rx_blocks, (ULL) rx_pkts, rx_pkts_speed_str,
               (ULL) rx_bytes, rx_bytes_speed_str, (ULL) rx_errors);
        fflush(stdout);

        sec++;
        rx_bytes_old = rx_bytes;
        rx_pkts_old  = rx_pkts;
        sleep(1);
    }

    return NULL;
}

void frctweak_cmd_pr_rule_usage(void)
{
    printf("Usage: %s [-bipch].\n", program);
    printf("          -b        -- Dump block info.\n");
    printf("          -i        -- Dump pkt info.\n");
    printf("          -p        -- Dump packet data.\n");
    printf("          -c        -- Dump counter.\n");
    printf("          -h        -- Printf help message.\n");

    exit(0);
}

int frctweak_cmd_pr_rule_start(int argc, char **argv)
{
    int mfd = 0;
    //uint16_t olen;
    int rv;
    pr_chan_t *udp_chan = NULL;
    int opt;
    int dump_count = 0;
    if (argc == 1)
    {
        frctweak_cmd_pr_udp_usage();
        return FRE_SUCCESS;
    }
    while ((opt = getopt(argc, argv, "bfipcvo:TURh?")) != -1) {
        switch (opt) {
        case 'b':
            dump_block  = 1;
            break;
        case 'i':
            dump_info  = 1;
            break;
        case 'p':
            dump_packet = 1;
            break;
        case 'c':
            dump_count = 1;
            break;
        case 'h':
        case '?':
            frctweak_cmd_pr_udp_usage();
            return FRE_SUCCESS;
            break;
        }
    }

    udp_chan = malloc(sizeof(pr_chan_t));
    memset(udp_chan, 0, sizeof(pr_chan_t));
    udp_chan->type = FRC_WORK_RULE;
    //olen = sizeof(frc_dma_chan_desc_t);
    //rv = frcapi(CMD_TYPE_DRV, DRV_CMD_GET_POOL_AND_RING_ADDR, 8, &udp_chan->type, &olen, &udp_chan->desc);

    rv = frcapi_chan_or_pr_rule_or_udp_start(udp_chan->type, &udp_chan->desc);
    if (rv != FRE_SUCCESS)
    {
        FRCTWEAK_ERROR("command execute fail: %d!\n", rv);
        goto frctweak_cmd_pr_udp_start_err;
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
            goto frctweak_cmd_pr_udp_start_err;
        }


        udp_chan->dma_ctrl = mmap(0, udp_chan->desc.ctrl_size, PROT_READ|PROT_WRITE, MAP_SHARED, mfd, udp_chan->desc.ctrl_addr);
        FRCTWEAK_DEBUG("udp_chan->dma_ctrl=%p\n", udp_chan->dma_ctrl);
        if (udp_chan->dma_ctrl == NULL)
        {
            FRCTWEAK_ERROR("mmap udp avail ring fail!\n");
            rv = FRE_MEMORY;
            goto frctweak_cmd_pr_udp_start_err;
        }

        udp_chan->pool_addr = mmap(0, udp_chan->desc.pool_size, PROT_READ, MAP_SHARED, mfd, udp_chan->desc.pool_addr);
        FRCTWEAK_DEBUG("udp_chan->pool_addr=%p\n", udp_chan->pool_addr);
        if (udp_chan->pool_addr == NULL)
        {
            FRCTWEAK_ERROR("mmap udp pool fail!\n");
            rv = FRE_MEMORY;
            goto frctweak_cmd_pr_udp_start_err;
        }

    }

    if (dump_count && !dump_block && !dump_info && !dump_packet)
    {
        pr_udp_stat_thread_id = pthread_create(&pr_udp_stat_thread_id, NULL, pr_rule_stat_thread, (void *)udp_chan);
    }
    pr_udp_data_thread_id = pthread_create(&pr_udp_data_thread_id, NULL, pr_rule_data_thread, (void *)udp_chan);

    while(1)
    {
        sleep(30);
    }
frctweak_cmd_pr_udp_start_err:
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
    uint64_t rx_pkts;
    uint64_t rx_bytes;
    uint64_t rx_loss;
    uint64_t rx_positive;
    uint64_t rx_reverse;
    uint64_t rx_errors;

    uint64_t tx_block;
    uint64_t tx_pkts;
    uint64_t tx_bytes;
    uint64_t tx_error;
    uint64_t tx_positive;
    uint64_t tx_reverse;
} ssn_stat_t;

typedef struct {
    uint64_t type;
    ssn_stat_t fr_stat;
    frc_dma_ssn_chan_desc_t desc;
    frc_ssn_ring_buff_t *available_ring;
    frc_ssn_ring_buff_t *completed_ring;
    uint8_t *pool_addr; /* app addr */
} pr_ssn_chan_t;

static pthread_t pr_ssn_stat_thread_id;
static pthread_t pr_ssn_data_thread_id;
ssn_stat_t ssn_stat_now, ssn_stat_old;


typedef struct {
    uint64_t   num;
    uint64_t   buff[FRC_DMA_SSN_RING_BUFF_SIZE];      /* the buff holding the data */
} pr_ssn_ring_t;

pr_ssn_ring_t *avail_ring = NULL;
pr_ssn_ring_t *compl_ring = NULL;

int pr_ssn_disable_oct0(void)
{
    int rv;
    frc_bdd_phy_t input;
    frc_phy_op_t output;
    memset(&input, 0, sizeof(frc_bdd_phy_t));
    memset(&output, 0, sizeof(frc_phy_op_t));
    input.phy.port = 0;
    input.phy.devad = 1;
    input.phy.addr  = 0;
    input.phy.value = 0x2041;
    input.op = 1;

    rv =frcapi_bdd_phy(&input, &output);
    if (rv != FRE_SUCCESS)
    {
        printf("Get or set PHY  fail: %d!\n",  rv);
    }
    return rv;
}

int pr_ssn_enable_oct0(void)
{
    int rv;
    frc_bdd_phy_t input;
    frc_phy_op_t output;
    memset(&input, 0, sizeof(frc_bdd_phy_t));
    memset(&output, 0, sizeof(frc_phy_op_t));
    input.phy.port = 0;
    input.phy.devad = 1;
    input.phy.addr  = 0;
    input.phy.value = 0x2040;
    input.op = 1;

    rv =frcapi_bdd_phy(&input, &output);
    if (rv != FRE_SUCCESS)
    {
        printf("Get or set PHY  fail: %d!\n",  rv);
    }
    return rv;
}
static inline void pr_ssn_ring_get(frc_ssn_ring_buff_t *ring, pr_ssn_ring_t *buff)
{
    uint64_t ridx, widx;
    int i;

    ridx = ssn_ring_ridx(ring);
    widx = ssn_ring_widx(ring);

    for (i = 0; ridx < widx; ridx++, i++)
    {
        if ((widx >= ridx) && (widx - ridx <= FRC_DMA_SSN_RING_BUFF_SIZE)) {
        #if __FRC_BYTE_ORDER == __FRC_LITTLE_ENDIAN
            buff->buff[i] = SWAP_8_BYTE(ring->buff[ridx%FRC_DMA_SSN_RING_BUFF_SIZE]);
        #else
            buff->buff[i] = ring->buff[ridx%FRC_DMA_SSN_RING_BUFF_SIZE];
        #endif
        }
    }
    buff->num = i;
    printf("pr_ssn_ring size=%d\n", i);
}
/* physical addr to application address */
frc_ssn_block_t *
pr_ssn_pkt_get(uint64_t block_addr, pr_ssn_chan_t *chan)
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

/*
   return 0 for success;
   return 1 for failed
 */
int
pr_ssn_dma_head_get(frc_ssn_block_t *dma_pkt, frc_dma_hdr_t *dma_hdr)
{
    if (dma_pkt == NULL)
    {
        return FRE_FAIL;
    }
    memset(dma_hdr, 0, FRC_DMA_PKT_HEAD_SIZE);
    memcpy(dma_hdr, dma_pkt->data, FRC_DMA_PKT_HEAD_SIZE);
    swap_buff(FRC_DMA_PKT_HEAD_SIZE >>3, dma_hdr);
    return FRE_SUCCESS;
}

void
pr_ssn_pkt_process(pr_ssn_chan_t *chan, frc_ssn_block_t *dma_pkt, frc_dma_hdr_t *dma_hdr,
                   uint64_t block_addr)
{
    int i;
    frc_dma_pkt_info_t pkt_info;
    uint8_t *payload = NULL;
    uint16_t paylen = 0;
    uint16_t payload_offset = FRC_DMA_PKT_PAYLOAD_OFFSET;
    uint16_t pktinfo_offset = FRC_DMA_SSN_DATA_SIZE - FRC_DMA_PKT_INFO_SIZE;
    uint8_t pkt_num;
    uint8_t flag = 0;
    uint64_t dmac = 0;
    if (dump_block)
    {
        frc_dma_header_dump(chan->desc.type, dma_hdr);
    }

    #if FRC_DEBUG_PKT_LEN
    frc_dump_buff(FRC_DMA_SSN_DATA_SIZE, dma_pkt->data);
    frc_dump_dma_hdr(dma_hdr);
    #endif
    pkt_num = dma_hdr->pkt_num;
    //printf("pkt_num:%d\n", pkt_num);

    for (i = 0; i < pkt_num; i++ )
    {
        memset(&pkt_info, 0, FRC_DMA_PKT_INFO_SIZE);
        memcpy(&pkt_info, (frc_dma_pkt_info_t *)&dma_pkt->data[pktinfo_offset], FRC_DMA_PKT_INFO_SIZE);
        swap_buff(FRC_DMA_PKT_INFO_SIZE>>3, &pkt_info);
        payload  = &dma_pkt->data[payload_offset];
        paylen  = pkt_info.payload_len;
        chan->fr_stat.rx_pkts++;
        chan->fr_stat.rx_bytes += pkt_info.payload_len;
        //printf("chan->fr_stat.rx_bytes=%llu, pkt_info.payload_len=%u\n", chan->fr_stat.rx_bytes, pkt_info.payload_len);
        pktinfo_offset -= FRC_DMA_PKT_INFO_SIZE;
        payload_offset += pkt_info.payload_len;

        if (i == 0)
        {
            dmac = pkt_info.dmac & 0xffffffffffff;
        } else {
            if ((dmac != (pkt_info.dmac & 0xffffffffffff)) &&
                (dmac != (pkt_info.smac & 0xffffffffffff)))
            {
                flag |= 1;
                break;
            }
        }

        if (dump_info)
        {
             frc_pkt_info_dump(&pkt_info);
        }

        if (dump_packet)
        {
             frc_pkt_payload_dump(paylen, payload);
        }
        #if FRC_DEBUG_PKT_LEN
        frc_dump_dma_pkt_info(&pkt_info);
        //frc_dump_buff(simple_pkt.info.payload_len, simple_pkt.payload);
        #endif
    }

    if (flag)
    {
        chan->fr_stat.rx_errors++;
        if (dump_err_addr)
        {
            frc_dma_header_dump(chan->desc.type, dma_hdr);
            printf("block_addr=%llx\n", (ULL)block_addr);
            frc_dump_buff(FRC_DMA_SSN_DATA_SIZE, dma_pkt->data);
            if (pr_ssn_disable_oct0())
            {
                printf("disable oct0 failed\n");
            }
            sleep(5);
            /*get avail ring */
            pr_ssn_ring_get(chan->available_ring, avail_ring);
            pr_ssn_ring_get(chan->completed_ring, compl_ring);
            printf("avail_ring->num + compl_ring->num = %llu\n", (ULL)(avail_ring->num+compl_ring->num));

            /* match */
            /* find avail_ring*/
            for (i = 0; i < avail_ring->num; i++)
            {
                if (block_addr == avail_ring->buff[i])
                {
                    printf("find avail_ring addr i=%d\n", i);
                }
            }
            /* find compl_ring*/
            for (i = 0; i < compl_ring->num; i++)
            {
                if (block_addr == compl_ring->buff[i])
                {
                    printf("find compl_ring addr i=%d\n", i);
                }
            }
            if (pr_ssn_enable_oct0())
            {
                printf("disable oct0 failed\n");
            }

        }
    }

    memset(dma_pkt, 0, sizeof(frc_ssn_block_t));
}

void *pr_ssn_data_thread(void *arg)
{
    uint64_t block_addr; /* driver addr */
    frc_ssn_block_t *dma_pkt = NULL;
    frc_dma_hdr_t dma_head;
    struct timespec sleeptime;
    sleeptime.tv_sec = 0;
    sleeptime.tv_nsec = 10;
    pr_ssn_chan_t *ssn_chan = (pr_ssn_chan_t *)arg;
    while(1)
    {
        if (SSN_COMPL_RING_GET((*ssn_chan), block_addr))
        {
            #if 1
            dma_pkt = pr_ssn_pkt_get(block_addr, ssn_chan);
            if (dma_pkt == NULL)
            {
                continue;
            }
            /* get dma header */

            /* crc check */
            if (!pr_ssn_dma_head_get(dma_pkt, &dma_head))
            {
                ssn_chan->fr_stat.rx_blocks++;
            } /*else {
                ssn_chan->fr_stat.rx_errors++;
                //FRCTWEAK_ERROR("error block_addr=0x%llx\n", block_addr);
            }*/
            #else
             ssn_chan->fr_stat.rx_blocks++;
            #endif

            pr_ssn_pkt_process(ssn_chan, dma_pkt, &dma_head, block_addr);
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

void *pr_ssn_stat_thread(void *arg)
{
    uint64_t cur_speed, avg_speed, spend;
    char cur_unit, avg_unit;
    pr_ssn_chan_t *ssn_chan = (pr_ssn_chan_t *)arg;
    struct timeval now;
    gettimeofday(&start_time, NULL);
    //DUMP_STAT_TITLE;
    printf("  %5s  %16s  %16s  %9s  %9s  %16s  %16s  %16s  %16s\n", \
           "TIME", "RX_BLOCKS", "ERR_BLOCKS", "CUR SPEED", "AVG SPEED",
           "PKT", "PAYLEN", "AVAIL_SPACE", "COMPL_SPACE");
    while (1)
    {
        gettimeofday(&now, NULL);
        memcpy(&ssn_stat_now, &ssn_chan->fr_stat, sizeof(ssn_stat_t));
        spend = now.tv_sec - start_time.tv_sec;
        if (spend == 0)
        {
            spend = 1;
        }
       avg_speed = (ssn_stat_now.rx_bytes )/ spend;
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

        cur_speed = ssn_stat_now.rx_bytes - ssn_stat_old.rx_bytes;
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
#if 0
        printf("\r  %5ld  %16lld  %16lld  %8lld%C  %8lld%C  %16lld  %16lld",now.tv_sec- start_time.tv_sec,
                (ULL) ssn_stat_now.rx_blocks,(ULL)ssn_stat_now.rx_errors, (ULL)cur_speed, cur_unit, (ULL)avg_speed, avg_unit,
               (ULL) (SWAP_8_BYTE(ssn_chan->available_ring->widx)-SWAP_8_BYTE(ssn_chan->available_ring->ridx)),
               (ULL) (SWAP_8_BYTE(ssn_chan->completed_ring->widx)-SWAP_8_BYTE(ssn_chan->completed_ring->ridx)));
#else
        #if 0
        printf("\r  %5ld  %16lld  %16lld  %8lld%C  %8lld%C  %16lld  %16lld  %16lld  %16lld",now.tv_sec- start_time.tv_sec,
                (ULL) ssn_stat_now.rx_blocks,(ULL)ssn_stat_now.rx_errors, (ULL)cur_speed, cur_unit, (ULL)avg_speed, avg_unit,
               (ULL) (SWAP_8_BYTE(ssn_chan->available_ring->widx)-SWAP_8_BYTE(ssn_chan->available_ring->ridx)),
               (ULL) (SWAP_8_BYTE(ssn_chan->completed_ring->widx)-SWAP_8_BYTE(ssn_chan->completed_ring->ridx)),
               (ULL) SWAP_8_BYTE(ssn_chan->available_ring->widx), (ULL)SWAP_8_BYTE(ssn_chan->completed_ring->widx));

        #else
        printf("\r  %5ld  %16lld  %16lld  %8lld%C  %8lld%C  %16lld  %16lld  %16lld %16lld",
               now.tv_sec- start_time.tv_sec,(ULL) ssn_stat_now.rx_blocks,
               (ULL)ssn_stat_now.rx_errors, (ULL)cur_speed, cur_unit,
               (ULL)avg_speed, avg_unit,(ULL) ssn_stat_now.rx_pkts,
               (ULL) ssn_stat_now.rx_bytes,
               (ULL) (SWAP_8_BYTE(ssn_chan->available_ring->widx)-SWAP_8_BYTE(ssn_chan->available_ring->ridx)),
               (ULL) (SWAP_8_BYTE(ssn_chan->completed_ring->widx)-SWAP_8_BYTE(ssn_chan->completed_ring->ridx)));
        #endif
#endif
        fflush(stdout);
        memcpy(&ssn_stat_old, &ssn_stat_now, sizeof(ssn_stat_t));
        sleep(1);
    }

    return NULL;
}


void frctweak_cmd_pr_ssn_usage(void)
{
    printf("Usage: %s [-bipceh].\n", program);
    printf("          -b        -- Dump block info.\n");
    printf("          -i        -- Dump pkt info.\n");
    printf("          -p        -- Dump packet data.\n");
    printf("          -c        -- Dump counter.\n");
    printf("          -e        -- Dump error address.\n");
    printf("          -h        -- Printf help message.\n");
}

int frctweak_cmd_pr_ssn_start(int argc, char **argv)
{
    int mfd = 0;
    //uint16_t olen;
    int rv;
    pr_ssn_chan_t *ssn_chan = NULL;
    int dump_count = 0;
    int opt;
    if (argc == 1)
    {
        frctweak_cmd_pr_ssn_usage();
        exit(0);
    }
    while ((opt = getopt(argc, argv, "befipcvo:TURh?")) != -1) {
        switch (opt) {
        case 'b':
            dump_block  = 1;
            break;
        case 'i':
            dump_info  = 1;
            break;
        case 'p':
            dump_packet = 1;
            break;
        case 'c':
            dump_count = 1;
            break;
        case 'e':
            dump_err_addr = 1;
            break;
        case 'h':
        case '?':
            frctweak_cmd_pr_ssn_usage();
            return FRE_SUCCESS;
            break;
        }
    }

    /* start test */
    avail_ring = malloc(sizeof(pr_ssn_ring_t));
    if (avail_ring == NULL)
    {
        printf("%s %d avail_ring malloc failed!\n", __FUNCTION__, __LINE__);
    }
    memset(avail_ring, 0, sizeof(pr_ssn_ring_t));

    compl_ring = malloc(sizeof(pr_ssn_ring_t));
    if (compl_ring == NULL)
    {
        printf("%s %d compl_ring malloc failed!\n", __FUNCTION__, __LINE__);
    }

    memset(avail_ring, 0, sizeof(pr_ssn_ring_t));
    ssn_chan = malloc(sizeof(pr_ssn_chan_t));
    memset(ssn_chan, 0, sizeof(pr_ssn_chan_t));
    ssn_chan->type = FRC_WORK_SSN;
    //olen = sizeof(frc_dma_ssn_chan_desc_t);
    //rv = frcapi(CMD_TYPE_DRV, DRV_CMD_GET_POOL_AND_RING_ADDR, 8, &ssn_chan->type, &olen, &ssn_chan->desc);
    rv = frcapi_chan_or_pr_ssn_start(ssn_chan->type, &ssn_chan->desc);

    if (rv != FRE_SUCCESS)
    {
        FRCTWEAK_ERROR("command execute fail: %d!\n", rv);
        goto frctweak_cmd_pr_ssn_start_err;
    }
    FRCTWEAK_DEBUG("ssn_chan->desc.avail_addr=0x%llx, avail_size=0x%llx\n", (ULL)ssn_chan->desc.avail_addr,
                   (ULL)ssn_chan->desc.avail_size);
    FRCTWEAK_DEBUG("ssn_chan->desc.compl_addr=0x%llx, compl_size=0x%llx\n",(ULL)ssn_chan->desc.compl_addr,
                   (ULL)ssn_chan->desc.compl_size);
    FRCTWEAK_DEBUG("ssn_chan->desc.pool_addr=0x%llx, pool_size=0x%llx\n",(ULL)ssn_chan->desc.pool_addr,
                   (ULL)ssn_chan->desc.pool_size);

    {
        //memset(&app_addr, 0, sizeof(frc_pr_single_chan_addr_t));
        mfd = open("/dev/mem",O_RDWR);
        if (mfd < 0)
        {
            FRCTWEAK_ERROR("Can't open /dev/mem !\n");
            rv = FRE_FAIL;
            goto frctweak_cmd_pr_ssn_start_err;
        }

        ssn_chan->available_ring = mmap(0, ssn_chan->desc.avail_size, PROT_READ|PROT_WRITE, MAP_SHARED, mfd, ssn_chan->desc.avail_addr);
        if (ssn_chan->available_ring == NULL)
        {
            FRCTWEAK_ERROR("mmap udp avail ring fail!\n");
            rv = FRE_MEMORY;
            goto frctweak_cmd_pr_ssn_start_err;
        }

        ssn_chan->completed_ring = mmap(0, ssn_chan->desc.compl_size, PROT_READ|PROT_WRITE, MAP_SHARED, mfd, ssn_chan->desc.compl_addr);
        if (ssn_chan->completed_ring == NULL)
        {
            FRCTWEAK_ERROR("mmap udp compl ring fail!\n");
            rv = FRE_MEMORY;
            goto frctweak_cmd_pr_ssn_start_err;
        }

        ssn_chan->pool_addr = mmap(0, ssn_chan->desc.pool_size, PROT_READ|PROT_WRITE, MAP_SHARED, mfd, ssn_chan->desc.pool_addr);
        if (ssn_chan->pool_addr == NULL)
        {
            FRCTWEAK_ERROR("mmap udp pool fail!\n");
            rv = FRE_MEMORY;
            goto frctweak_cmd_pr_ssn_start_err;
        }

    }
    if (dump_count && !dump_block && !dump_info && !dump_packet)
    {
        pr_ssn_stat_thread_id = pthread_create(&pr_ssn_stat_thread_id, NULL, pr_ssn_stat_thread, (void *)ssn_chan);
    }
    pr_ssn_data_thread_id = pthread_create(&pr_ssn_data_thread_id, NULL, pr_ssn_data_thread, (void *)ssn_chan);

    while(1)
    {
        sleep(30);
    }
frctweak_cmd_pr_ssn_start_err:
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

int frctweak_pr_cmd_init(frctweak_cmd_t *cmd)
{
    frctweak_cmd_t *pr_cmd;

    pr_cmd = frctweak_cmd_register(cmd, "pr", "pr get data", frctweak_cmd_pr, frctweak_cmd_pr_usage);
#if FRC_CONFIG_SIMPLE_PACKAGE
    frctweak_cmd_register(pr_cmd, "udp", "pr udp get data.", frctweak_cmd_pr_udp_start, frctweak_cmd_pr_udp_usage);
    frctweak_cmd_register(pr_cmd, "rule", "pr rule get data.", frctweak_cmd_pr_rule_start, frctweak_cmd_pr_rule_usage);
#endif
#if FRC_CONFIG_SSN_CHAN
    frctweak_cmd_register(pr_cmd, "ssn", "pr ssn get data.", frctweak_cmd_pr_ssn_start, frctweak_cmd_pr_ssn_usage);
#endif
    return 0;
}

/* End of file */

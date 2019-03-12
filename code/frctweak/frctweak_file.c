
#include <stdlib.h>
#include <string.h>

#include <time.h>
#include <sys/mman.h>
#include <sys/time.h>
#include <pthread.h>

#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>


#include "frctweak.h"
#include "frc_dma.h"
#include "frc_crc8.h"
#include "frc_util.h"
#include "frc_debug.h"
#include "frc_api.h"

#define FRC_MAC_STRING_SIZE 40

void frctweak_cmd_file_usage(void)
{
    printf("%s (udp|ssn|rule) OPTIONS\n", program);
}

int frctweak_cmd_file(int argc, char **argv)
{
   printf("%s (udp|ssn|rule) OPTIONS\n", program);
   return 0;
}



struct pcap_file_header {
    uint32_t magic;
    uint16_t version_major;
    uint16_t version_minor;
    uint32_t thiszone; /* gmt to local correction */
    uint32_t sigfigs;    /* accuracy of timestamps */
    uint32_t snaplen;    /* max length saved portion of each pkt */
    uint32_t linktype;   /* data link type (LINKTYPE_*) */
};

struct pcap_pkthdr {
    uint32_t second;  /* time stamp */
    uint32_t micresec; /* time stamp */
    uint32_t caplen; /* length of portion present */
    uint32_t len;    /* length this packet (off wire) */
};


typedef struct {
    int infd; /* input file descriptor */
    int outfd; /* output file descriptor*/
}pcap_file_struct;

//#define DUMP_LINE_SIZE      128
static inline void pcap_dump_buff(int len, uint8_t *buff)
{
#if 0
    int i;
    char *p, line[DUMP_LINE_SIZE];

    p = line;
    printf("BUFF DUMP %d bytes:\n", len);
    for (i = 0; i < len; i++)
    {
        if ((i % 16) == 0)
        {
            memset(line, 0, DUMP_LINE_SIZE);
            p = line;
            p += sprintf(p, "%.4d:", i);
        }

        if ((i % 16) == 8)
        {
            p += sprintf(p, " -");
        }
        p += sprintf(p, " %.2x", buff[i]);

        if ((i % 16) == 15)
        {
            line[DUMP_LINE_SIZE - 1] = 0;
            printf("%s\n", line);
        }
    }
    if ((i % 16) != 0)
    {
        line[DUMP_LINE_SIZE - 1] = 0;
        printf("%s\n", line);
    }
    printf("\n");
#endif
    int i;
    printf("PAYLOAD: %d bytes.\n", len);

    for (i = 0; i < len; i++)
    {
        if ((i % 16) == 0)
        {
            printf(" %.4x:", i);
        }
        printf(" %.2x", buff[i]);
        if ((i % 16) == 15)
        {
            printf("\n");
        }
    }
    printf("\n");
}




int pcap_file_init(char* in_file, char* out_file, pcap_file_struct *file_des)
{
    int in_fd, out_fd; /* file description */
    uint32_t numbytes;
    struct pcap_file_header pcap_header;
    int flags = O_CREAT | O_TRUNC | O_WRONLY;

    memset(&pcap_header, 0, sizeof(struct pcap_file_header));
    /* open file */
    if ((in_fd = open(in_file, O_RDONLY, 0644)) < 0) {
        perror("open");
        return EXIT_FAILURE;
    }else{
        printf("opened %s\n",in_file);
        printf("descriptor is %d\n", in_fd);
    }

    /* read pacap header */
    if (read(in_fd, &pcap_header, sizeof(struct pcap_file_header))
        != sizeof(struct pcap_file_header))
    {
        printf("read pcap header failed!\n");
        close(in_fd);
    }else {
        printf("pcap_header.magic = 0x%x\n", pcap_header.magic);
        printf("pcap_header.version_major = 0x%x\n", pcap_header.version_major);
        printf("pcap_header.version_minor = 0x%x\n", pcap_header.version_minor);
        printf("pcap_header.thiszone = 0x%x\n", pcap_header.thiszone);
        printf("pcap_header.sigfigs = 0x%x\n", pcap_header.sigfigs);
        printf("pcap_header.snaplen = 0x%x\n", pcap_header.snaplen);
        printf("pcap_header.linktype = 0x%x\n", pcap_header.linktype);
    }

    /* open outfile */
    if ((out_fd = open(out_file, flags, 0644)) < 0) {
        perror("open");
        close(in_fd); /* have to close both of these now */
        close(out_fd);
        return EXIT_FAILURE;
    }

    /*write pcap header */
    numbytes = write(out_fd, &pcap_header, sizeof(struct pcap_file_header));
    if (numbytes != sizeof(struct pcap_file_header))
    {
        printf("write pcap header failed!\n");
        close(in_fd);
        close(out_fd);
        return EXIT_FAILURE;
    }

    file_des->infd = in_fd;
    file_des->outfd = out_fd;
    return 0;
}


int pcap_file_add_packet(char *buff, pcap_file_struct *file_des)
{
    int in_fd, out_fd; /* file description */
    uint32_t numbytes;
    struct pcap_pkthdr pkt_header;
    uint32_t packet_len;

    in_fd = file_des->infd;
    out_fd = file_des->outfd;

    memset(&pkt_header,  0, sizeof(struct pcap_pkthdr));

    /* read packet header*/
    if (read(in_fd, &pkt_header, sizeof(struct pcap_pkthdr))
        != sizeof(struct pcap_pkthdr))
    {
        printf("read pcap packet header failed!\n");
    }else {
        //printf("pkt_header.second = 0x%x\n", pkt_header.second);
        //printf("pkt_header.micresec = 0x%x\n", pkt_header.micresec);
        //printf("pkt_header.caplen = 0x%x\n", pkt_header.caplen);
        //printf("pkt_header.len = 0x%x\n", pkt_header.len);
    }

    packet_len = pkt_header.caplen;
    //printf("%d pkt_header.caplen = 0x%x\n", __LINE__, pkt_header.caplen);

    /* write packet header */
    if (write(out_fd, &pkt_header, sizeof(struct pcap_pkthdr)) !=
        sizeof(struct pcap_pkthdr))
    {
        printf("write packet header faile\n");
        //close(in_fd);
        //close(out_fd);
        return EXIT_FAILURE;
    }

    /* write packet */
    //printf("%d pkt_header.caplen = 0x%x\n", __LINE__, pkt_header.caplen);
    //printf("packet_len = 0x%x\n", packet_len);
    numbytes = write(out_fd, buff, packet_len);
    if (numbytes != packet_len)
    {
        printf("write packet buff failed ! 0x%x\n", numbytes);
        //close(in_fd);
        //close(out_fd);
        return EXIT_FAILURE;
    }

    //printf("%s.%d\n", __func__, __LINE__);
    //pcap_dump_buff(packet_len, (uint8_t *)buff);
    //printf("%s.%d\n", __func__, __LINE__);
        /* update */
    if (lseek(in_fd, packet_len, SEEK_CUR) < 0)
    {
        printf("file end!\n");
        //close(in_fd);
        //close(out_fd);
        return EXIT_FAILURE;
    }
    //printf("%s.%d\n", __func__, __LINE__);
    return EXIT_SUCCESS;
}

int pcap_file_close(pcap_file_struct *file_des)
{
    int in_fd, out_fd; /* file description */

    in_fd = file_des->infd;
    out_fd = file_des->outfd;

    if (close(in_fd) < 0) {
        perror("close");
        return EXIT_FAILURE;
    }
    if (close(out_fd) < 0) {
        perror("close");
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}



struct timeval start_time;


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

//static pthread_t pr_udp_stat_thread_id;
//static pthread_t pr_udp_data_thread_id;

simple_package_stat_t stat_now, stat_old;


/* physical addr to application address */
static frc_simple_package_block_t *
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
static int
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

static int
pr_simple_pkt_process(pr_chan_t *chan, frc_simple_pkt_t *simple_pkt, pcap_file_struct *file_des)
//pr_simple_pkt_process(pcap_file_struct *file_des)
{
    //int i;
    //char buf[1024];

    //frc_dma_pkt_info_t *pkt_info;

    //chan->fr_stat.rx_blocks++;

    //pkt_info = &simple_pkt->info;

    //chan->fr_stat.rx_pkts++;
    //chan->fr_stat.rx_bytes += pkt_info->payload_len;
#if 0
    for (i = 0; i<1024; i++)
    {
        buf[i]=i+1;
    }
#endif
    if (pcap_file_add_packet((char *)simple_pkt->payload, file_des))
    //if (pcap_file_add_packet(buf, file_des))
    {
        FRCTWEAK_ERROR("pcap_file_add_packet failed\n");
        pcap_file_close(file_des);
        return FRE_PARAM;
    }
    return FRE_SUCCESS;
}

void pr_udp_data(pr_chan_t *udp_chan, pcap_file_struct *file_des, int recv_num)
{
    int i=0;
    uint64_t block_addr; /* driver addr */
    frc_simple_package_block_t *dma_pkt = NULL;
    frc_simple_pkt_t  simple_pkt;
    struct timespec sleeptime;
    sleeptime.tv_sec = 0;
    sleeptime.tv_nsec = 10;
    //pr_chan_t *udp_chan = (pr_chan_t *)arg;
    while(1)
    {
        //printf("%s.%d, i = %d\n", __func__, __LINE__, i);
        if (SIMPLE_PACKAGE_COMPL_RING_GET(*udp_chan->dma_ctrl, block_addr))
        {
            i ++;
            //printf("%s.%d, i = %d\n", __func__, __LINE__, i);
            #if 1
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
                    frc_dump_buff(simple_pkt.info.payload_len, simple_pkt.payload);

            #endif
            #endif

            //pcap_dump_buff(simple_pkt.info.payload_len, simple_pkt.payload);
            pr_simple_pkt_process(udp_chan, &simple_pkt, file_des);
            //pr_simple_pkt_process(file_des);

            while (!SIMPLE_PACKAGE_AVAIL_RING_PUT(*udp_chan->dma_ctrl, block_addr))
            {
                printf("%s.%d, i = %d\n", __func__, __LINE__, i);
                nanosleep(&sleeptime, &sleeptime);
            }
        }
        else
        {
            //printf("%s.%d\n", __func__, __LINE__);
            nanosleep(&sleeptime, &sleeptime);
        }

        if (i == recv_num)
        {
            printf("i = %d\n", i);
            pcap_file_close(file_des);
            break;
        }

    }

}



void frctweak_cmd_file_udp_usage(void)
{
    printf("%s capture udp INPUT OUTPUT RECV_NUM\n", program);
    printf("OPTIONS:\n");
    printf("   INPUT    --The path of sending pcap file\n");
    printf("   OUTPUT    --The path of receiving pcap file\n");
    printf("   RECV_NUM  --The number of delivered pkts\n");
}

int frctweak_cmd_file_udp_start(int argc, char **argv)
{
    int mfd = 0;
    int recv_num;
    int rv;
    pcap_file_struct file_des;
    pr_chan_t *udp_chan = NULL;
    //int opt;


    udp_chan = malloc(sizeof(pr_chan_t));
    memset(udp_chan, 0, sizeof(pr_chan_t));
    memset(&file_des, 0 ,sizeof(pcap_file_struct));
    udp_chan->type = FRC_WORK_UDP;

    if (argc < 3)
    {
        frctweak_cmd_file_udp_usage();
        return FRE_SUCCESS;
    }


    sscanf(argv[3], "%d", &recv_num);
    //printf("num=%d\n", recv_num);
    if (!recv_num)
    {
        FRCTWEAK_ERROR("invalid packet length, must > 0\n");
        return FRE_FAIL;
    }

    if (pcap_file_init(argv[1], argv[2], &file_des))
    {
        FRCTWEAK_ERROR("pcap_file_init failed!\n");
        return FRE_FAIL;
    }

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


    pr_udp_data(udp_chan, &file_des, recv_num);
    //pr_udp_data_thread_id = pthread_create(&pr_udp_data_thread_id, NULL, pr_udp_data_thread, (void *)udp_chan);

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

static int
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

static int
pr_simple_rule_pkt_process(pr_chan_t *chan, frc_simple_rule_pkt_t *simple_pkt, pcap_file_struct *file_des)
{

    if (pcap_file_add_packet((char *)simple_pkt->payload, file_des))
    {
        FRCTWEAK_ERROR("pcap_file_add_packet failed\n");
        pcap_file_close(file_des);
        return FRE_PARAM;
    }

    return FRE_SUCCESS;
}


void pr_rule_data(pr_chan_t *udp_chan, pcap_file_struct *file_des, int recv_num)
{
    int i = 0;
    uint64_t block_addr; /* driver addr */
    frc_simple_package_block_t *dma_pkt = NULL;
    frc_simple_rule_pkt_t  simple_pkt;
    struct timespec sleeptime;
    sleeptime.tv_sec = 0;
    sleeptime.tv_nsec = 10;
    //pr_chan_t *udp_chan = (pr_chan_t *)arg;
    while(1)
    {
        //printf("%s.%d\n", __func__, __LINE__);
        if (SIMPLE_PACKAGE_COMPL_RING_GET(*udp_chan->dma_ctrl, block_addr))
        {
            i ++;
            //printf("i = %d\n", i);

            dma_pkt = pr_udp_pkt_get(block_addr, udp_chan);
            if (dma_pkt == NULL)
            {
                printf("%s.%d\n", __func__, __LINE__);
                udp_chan->fr_stat.rx_errors++;
                continue;
            }
            /**/if (pr_simple_rule_pkt_get(dma_pkt, &simple_pkt))
            {
                printf("%s.%d\n", __func__, __LINE__);
                udp_chan->fr_stat.rx_errors++;
                continue;
            }
            #if FRC_DEBUG_PKT_LEN
                    frc_dump_buff(FRC_DMA_SIMPLE_PACKAGE_DATA_SIZE, dma_pkt->data);
                    frc_dump_dma_hdr(&simple_pkt.header);
                    frc_dump_dma_pkt_info(&simple_pkt.info);
                    frc_dump_buff(simple_pkt.info.payload_len, simple_pkt.payload);

            #endif
            pr_simple_rule_pkt_process(udp_chan, &simple_pkt, file_des);

            while (!SIMPLE_PACKAGE_AVAIL_RING_PUT(*udp_chan->dma_ctrl, block_addr))
            {
                printf("%s.%d\n", __func__, __LINE__);
                nanosleep(&sleeptime, &sleeptime);
            }
        }
        else
        {
            //printf("%s.%d\n", __func__, __LINE__);
            nanosleep(&sleeptime, &sleeptime);
        }

        if (i == recv_num)
        {
            printf("i = %d\n", i);
            pcap_file_close(file_des);
            break;
        }

    }

}



void frctweak_cmd_file_rule_usage(void)
{
    printf("%s capture rule INPUT OUTPUT RECV_NUM\n", program);
    printf("OPTIONS:\n");
    printf("   INPUT    --The path of sending pcap file\n");
    printf("   OUTPUT    --The path of receiving pcap file\n");
    printf("   RECV_NUM  --The number of delivered pkts\n");

}

int frctweak_cmd_file_rule_start(int argc, char **argv)
{
    int mfd = 0;
    //uint16_t olen;
    int rv;
    int recv_num;
    pcap_file_struct file_des;
    pr_chan_t *udp_chan = NULL;

    udp_chan = malloc(sizeof(pr_chan_t));
    memset(udp_chan, 0, sizeof(pr_chan_t));
    memset(&file_des, 0 ,sizeof(pcap_file_struct));
    udp_chan->type = FRC_WORK_RULE;

    if (argc < 3)
    {
        frctweak_cmd_file_rule_usage();
        return FRE_SUCCESS;
    }

    sscanf(argv[3], "%d", &recv_num);
    //printf("num=%d\n", recv_num);
    if (!recv_num)
    {
        FRCTWEAK_ERROR("invalid packet length, must > 0\n");
        return FRE_FAIL;
    }

    if (pcap_file_init(argv[1], argv[2], &file_des))
    {
        FRCTWEAK_ERROR("pcap_file_init failed!\n");
        return FRE_FAIL;
    }
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

    pr_rule_data(udp_chan, &file_des, recv_num);
    //pr_udp_data_thread_id = pthread_create(&pr_udp_data_thread_id, NULL, pr_rule_data_thread, (void *)udp_chan);

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

#if 1
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

typedef struct {
    uint32_t sip;
    uint32_t dip;
    uint16_t sp;
    uint16_t dp;
    uint16_t block_num;
    uint16_t block_len;
} frc_tuple_block_t;

typedef struct {
    uint16_t valid;
    frc_tuple_block_t ssn_block[100];
} frc_tuple_block_use_t;

//static pthread_t pr_ssn_stat_thread_id;
//static pthread_t pr_ssn_data_thread_id;
ssn_stat_t ssn_stat_now, ssn_stat_old;
/* physical addr to application address */
static frc_ssn_block_t *
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
int static
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




void static
pr_ssn_pkt_process(frc_ssn_block_t *dma_pkt, frc_dma_hdr_t *dma_hdr, char *filename,  uint16_t block_len, char *recv_dir)
{
    int i = 0;
    //frc_dma_pkt_info_t pkt_info;
    FILE *fp;
    uint8_t *payload = NULL;
    uint16_t paylen = 0, block_offset = 0;
    uint16_t payload_offset = FRC_DMA_PKT_PAYLOAD_OFFSET;
    //uint16_t pktinfo_offset = FRC_DMA_SSN_DATA_SIZE - FRC_DMA_PKT_INFO_SIZE;
    //uint8_t pkt_num;


    sprintf(filename, "%s/%x_%x_%x_%x.txt", recv_dir,
            dma_hdr->sip, dma_hdr->dip, dma_hdr->sport, dma_hdr->dport);

    fp = fopen(filename, "a+");
    if (fp == NULL)
    {
        return;
    }


    payload  = &dma_pkt->data[payload_offset];
    paylen  = dma_hdr->total_paylen;
    block_offset = block_len - paylen;
    #if 0
    if (block_offset)
    {
        printf("%d\n", block_offset);
    }
    #endif
    for (i =0; i< paylen; i++)
    {


        if ((((block_offset % 32) + i) % 32) == 0)
        {
            //printf("%.2x ", buff[i]);
            fprintf(fp, "%.2X", payload[i]);
        }
        else
        {
            fprintf(fp, " %.2X", payload[i]);
        }

        if ((((block_offset % 32) + i) % 32) == 31)
        {
            //printf("%.2x", buff[i]);
            //printf("\n");
            fprintf(fp, "\n");
        }
    }
    fclose(fp);
}



#if 0
void static
pr_ssn_pkt_process(frc_ssn_block_t *dma_pkt, frc_dma_hdr_t *dma_hdr, char *filename, uint16_t block_num)
{
    int i, j = 0;
    frc_dma_pkt_info_t pkt_info;
    FILE *fp;
    uint8_t *payload = NULL;
    uint16_t paylen = 0;
    uint16_t payload_offset = FRC_DMA_PKT_PAYLOAD_OFFSET;
    uint16_t pktinfo_offset = FRC_DMA_SSN_DATA_SIZE - FRC_DMA_PKT_INFO_SIZE;
    uint8_t pkt_num;


    sprintf(filename, "/root/fengying/%x_%x_%x_%x.txt",
            dma_hdr->sip, dma_hdr->dip, dma_hdr->sport, dma_hdr->dport);

    fp = fopen(filename, "a+");
    if (fp == NULL)
    {
        return;
    }


    pkt_num = dma_hdr->pkt_num;
    //printf("pkt_num:%d\n", pkt_num);

    for (i = 0; i < pkt_num; i++ )
    {
        //printf("%s.%d\n", __func__, __LINE__);
        memset(&pkt_info, 0, FRC_DMA_PKT_INFO_SIZE);
        memcpy(&pkt_info, (frc_dma_pkt_info_t *)&dma_pkt->data[pktinfo_offset], FRC_DMA_PKT_INFO_SIZE);
        swap_buff(FRC_DMA_PKT_INFO_SIZE>>3, &pkt_info);
        payload  = &dma_pkt->data[payload_offset];
        paylen  = pkt_info.payload_len;
        //chan->fr_stat.rx_pkts++;
        //chan->fr_stat.rx_bytes += pkt_info.payload_len;
        //printf("chan->fr_stat.rx_bytes=%llu, pkt_info.payload_len=%u\n", chan->fr_stat.rx_bytes, pkt_info.payload_len);
        pktinfo_offset -= FRC_DMA_PKT_INFO_SIZE;
        payload_offset += pkt_info.payload_len;


        for (j =0; j< paylen; j++)
        {


            if ((((paylen * pkt_num * (block_num - 1)) +(i * paylen) + j) % 32) == 0)
            {
                //printf("%.2x ", buff[i]);
                fprintf(fp, "%.2x", payload[j]);
            }
            else
            {
                fprintf(fp, " %.2x", payload[j]);
            }

            if ((((paylen * pkt_num * (block_num - 1)) +(i * paylen) + j) % 32) == 31)
            {
                //printf("%.2x", buff[i]);
                //printf("\n");
                fprintf(fp, "\n");
            }
        }

    }
    fclose(fp);
}
#endif

uint16_t five_tuple_create(frc_tuple_block_use_t *tuple_block, frc_dma_hdr_t *dma_hdr)
{
    uint16_t index, block_len;
    index = tuple_block->valid;
    tuple_block->ssn_block[index].sip = dma_hdr->sip;
    tuple_block->ssn_block[index].dip = dma_hdr->dip;
    tuple_block->ssn_block[index].sp = dma_hdr->sport;
    tuple_block->ssn_block[index].dp = dma_hdr->dport;
    tuple_block->ssn_block[index].block_len = dma_hdr->total_paylen;
    tuple_block->ssn_block[index].block_num = 1;
    block_len = tuple_block->ssn_block[index].block_len;
    //printf("sip=%x,dip=%x,sp=%x,dp=%x\n", tuple_block->ssn_block[index].sip,
                   //tuple_block->ssn_block[index].dip,tuple_block->ssn_block[index].sp,tuple_block->ssn_block[index].dp);
    tuple_block->valid ++;

    return block_len;
}

uint16_t five_tuple_match_process(frc_tuple_block_use_t *tuple_block, frc_dma_hdr_t *dma_hdr)
{
    int i;
    uint16_t block_len;
    //uint16_t num;
    if (tuple_block->valid)
    {
        for (i = 0; i < tuple_block->valid; i++)
        {
            if (dma_hdr->sip == tuple_block->ssn_block[i].sip &&
                dma_hdr->dip == tuple_block->ssn_block[i].dip &&
                dma_hdr->sport  == tuple_block->ssn_block[i].sp  &&
                dma_hdr->dport  == tuple_block->ssn_block[i].dp)
            {
                tuple_block->ssn_block[i].block_len += dma_hdr->total_paylen;
                tuple_block->ssn_block[i].block_num ++;
                //num = tuple_block->ssn_block[i].block_num;
                block_len = tuple_block->ssn_block[i].block_len;
                //pr_ssn_pkt_process(dma_pkt, dma_hdr, filename, tuple_block->ssn_block[i].block_num);

                return block_len;
            }
        }
    }

    return five_tuple_create(tuple_block, dma_hdr);
}

void pr_ssn_data(pr_ssn_chan_t *ssn_chan, char *recv_dir)
{
    uint64_t block_addr; /* driver addr */
    uint16_t block_len;
    frc_ssn_block_t *dma_pkt = NULL;
    frc_dma_hdr_t dma_head;
    frc_tuple_block_use_t tuple_block;
    char filename[128];
    struct timespec sleeptime;
    struct timeval start, end;
    sleeptime.tv_sec = 0;
    sleeptime.tv_nsec = 10;
    memset(&tuple_block, 0, sizeof(frc_tuple_block_use_t));
    gettimeofday(&start, NULL);

    //pr_ssn_chan_t *ssn_chan = (pr_ssn_chan_t *)arg;
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
                block_len = five_tuple_match_process(&tuple_block, &dma_head);
                ssn_chan->fr_stat.rx_blocks++;
            } else {
                ssn_chan->fr_stat.rx_errors++;
                //FRCTWEAK_ERROR("error block_addr=0x%llx\n", block_addr);
            }
    #if 0
            if (block_num > 1)
            {
                printf("%d\n", block_num);
            }
     #endif
            #else
             ssn_chan->fr_stat.rx_blocks++;
            #endif

            pr_ssn_pkt_process(dma_pkt, &dma_head, filename, block_len, recv_dir);
            while (!SSN_AVAIL_RING_PUT(*ssn_chan, block_addr))
            {
                nanosleep(&sleeptime, &sleeptime);
            }

        } else
        {
            nanosleep(&sleeptime, &sleeptime);
        }
        gettimeofday(&end, NULL);

        if ((end.tv_sec - start.tv_sec)>30)
        {
            break;
        }
    }

}


void frctweak_cmd_file_ssn_usage(void)
{
    printf("%s capture ssn RECVDIR\n", program);
    printf(" RECVDIR  -- The directory which is used for the dumped files\n" );
}

int frctweak_cmd_file_ssn_start(int argc, char **argv)
{
    int mfd = 0;
    //uint16_t olen;
    int rv;
    char recv_dir[128];
    pr_ssn_chan_t *ssn_chan = NULL;
    //int dump_count = 0;

    if (argc < 2)
    {
        frctweak_cmd_file_ssn_usage();
        return FRE_SUCCESS;
    }
    sscanf(argv[1], "%s", recv_dir);

    //printf("recv_dir = %s\n", recv_dir);
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

        ssn_chan->pool_addr = mmap(0, ssn_chan->desc.pool_size, PROT_READ, MAP_SHARED, mfd, ssn_chan->desc.pool_addr);
        if (ssn_chan->pool_addr == NULL)
        {
            FRCTWEAK_ERROR("mmap udp pool fail!\n");
            rv = FRE_MEMORY;
            goto frctweak_cmd_pr_ssn_start_err;
        }

    }

    pr_ssn_data(ssn_chan, recv_dir);
    //pr_ssn_data_thread_id = pthread_create(&pr_ssn_data_thread_id, NULL, pr_ssn_data_thread, (void *)ssn_chan);

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

int frctweak_file_cmd_init(frctweak_cmd_t *cmd)
{
    frctweak_cmd_t *file_cmd;

    file_cmd = frctweak_cmd_register(cmd, "capture", "capture the dma packets in pcap", frctweak_cmd_file, frctweak_cmd_file_usage);
#if FRC_CONFIG_SIMPLE_PACKAGE
    frctweak_cmd_register(file_cmd, "udp", "capture udp get data.", frctweak_cmd_file_udp_start, frctweak_cmd_file_udp_usage);
    frctweak_cmd_register(file_cmd, "rule", "capture rule get data.", frctweak_cmd_file_rule_start, frctweak_cmd_file_rule_usage);
#endif
#if FRC_CONFIG_SSN_CHAN
    frctweak_cmd_register(file_cmd, "ssn", "capture ssn get data.", frctweak_cmd_file_ssn_start, frctweak_cmd_file_ssn_usage);
#endif
    return 0;
}

/* End of file */

a.    8139too.c中加入的代码
/*add_by_liangjian for zero_copy*/
#include <linux/wrapper.h>
#include <asm/page.h>
#include <linux/slab.h>
#include <linux/proc_fs.h>
#define PAGES_ORDER 9
#define PAGES 512
#define MEM_WIDTH    1500
/*added*/
/*add_by_liangjian for zero_copy*/
struct MEM_DATA
{
    //int key;
    unsigned short width;/*缓冲区宽度*/
    unsigned short length;/*缓冲区长度*/
    //unsigned short wtimes;/*写进程记数,预留，为以后可以多个进程写*/
    //unsigned short rtimes;/*读进程记数,预留，为以后可以多个进程读*/
    unsigned short wi;/*写指针*/
    unsigned short ri;/*读指针*/
} * mem_data;
struct MEM_PACKET
{
    unsigned int len;
    unsigned char packetp[MEM_WIDTH - 4];/*sizeof(unsigned int) == 4*/
};
unsigned long su1_2;/*缓存地址*/
/*added*/
/*add_by_liangjian for zero_copy*/
//删除缓存
void del_mem()
{
    int pages = 0;
    char *addr;
    addr = (char *)su1_2;
    while (pages <=PAGES -1)
    {
        mem_map_unreserve(virt_to_page(addr));
        addr = addr + PAGE_SIZE;
        pages++;
    }
    free_pages(su1_2,PAGES_ORDER);    
}    
void init_mem()
/********************************************************
*                  初始化缓存
*       输入:   aMode:    缓冲区读写模式:  r,w        *
*       返回:   00:     失败                        *
*               >0:     缓冲区地址                  *
********************************************************/
{
    int i;
    int pages = 0;
    char *addr;
    char *buf;
    struct MEM_PACKET * curr_pack;
    
    su1_2 = __get_free_pages(GFP_KERNEL,PAGES_ORDER);
    printk("[%x]\n",su1_2);
    addr = (char *)su1_2;
    while (pages <= PAGES -1)
    {
        mem_map_reserve(virt_to_page(addr));//需使缓存的页面常驻内存
        addr = addr + PAGE_SIZE;
        pages++;
    }
    mem_data = (struct MEM_DATA *)su1_2;
    mem_data[0].ri = 1;
          mem_data[0].wi = 1;
          mem_data[0].length = PAGES*4*1024 / MEM_WIDTH;
          mem_data[0].width = MEM_WIDTH;
    /* initial su1_2 */
    for(i=1;i<=mem_data[0].length;i++)
    {
        buf = (void *)((char *)su1_2 + MEM_WIDTH * i);
        curr_pack = (struct MEM_PACKET *)buf;
        curr_pack->len = 0;
    }    
}


int put_mem(char *aBuf,unsigned int pack_size)
/****************************************************************
*                 写缓冲区子程序                                *
*       输入参数    :   aMem:   缓冲区地址                      *
*                       aBuf:   写数据地址                      *
*       输出参数    :   <=00 :  错误                            *
*                       XXXX :  数据项序号                      *
*****************************************************************/
{
    register int s,i,width,length,mem_i;
    char *buf;
    struct MEM_PACKET * curr_pack;
    s = 0;
    mem_data = (struct MEM_DATA *)su1_2;
    width  = mem_data[0].width;
    length = mem_data[0].length;
    mem_i  = mem_data[0].wi;
    buf = (void *)((char *)su1_2 + width * mem_i);
    for (i=1;i<length;i++){
        curr_pack = (struct MEM_PACKET *)buf;
            if  (curr_pack->len == 0){
                    memcpy(curr_pack->packetp,aBuf,pack_size);
                    curr_pack->len = pack_size;;
                s = mem_i;
            mem_i++;
                    if  (mem_i >= length)
                        mem_i = 1;
                mem_data[0].wi = mem_i;
                break;
            }
            mem_i++;
            if  (mem_i >= length){
                    mem_i = 1;
                    buf = (void *)((char *)su1_2 + width);
            }
            else buf = (char *)su1_2 + width*mem_i;
        }
    if(i >= length)
            s = 0;
    return s;
}
// proc文件读函数
int read_procaddr(char *buf,char **start,off_t offset,int count,int *eof,void *data)
{
    sprintf(buf,"%u\n",__pa(su1_2));
    *eof = 1;
    return 9;
}
/*added*/
在8139too.c的rtl8139_init_module()函数中加入以下代码：
/*add_by_liangjian for zero_copy*/
    put_pkt2mem_n = 0;
    init_mem();
    put_mem("data1dfadfaserty",16);
    put_mem("data2zcvbnm",11);
    put_mem("data39876543210poiuyt",21);
    create_proc_read_entry("nf_addr",0,NULL,read_procaddr,NULL);
/*added */    
在8139too.c的rtl8139_cleanup_module()函数中加入以下代码：
/*add_by_liangjian for zero_copy*/
    del_mem();
    remove_proc_entry("nf_addr",NULL);
/*added*/    
b．用户空间读取缓存代码
#include <stdio.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>
#define PAGES 512
#define MEM_WIDTH 1500
struct MEM_DATA
{
    //int key;
    unsigned short width;/*缓冲区宽度*/
    unsigned short length;/*缓冲区长度*/
    //unsigned short wtimes;/*写进程记数,预留，为以后可以多个进程写*/
    //unsigned short rtimes;/*读进程记数,预留，为以后可以多个进程读*/
    unsigned short wi;/*写指针*/
    unsigned short ri;/*读指针*/
} * mem_data;
struct MEM_PACKET
{
    unsigned int len;
    unsigned char packetp[MEM_WIDTH - 4];/*sizeof(unsigned int) == 4*/
};
int get_mem(char *aMem,char *aBuf,unsigned int *size)
/****************************************************************
*                 读缓冲区子程序                                *
*       输入参数    :   aMem:   缓冲区地址                      *
*                       aBuf:   返回数据地址, 其数据区长度应大于*
*                               缓冲区宽度                      *
*       输出参数    :   <=00 :  错误                            *
*                       XXXX :  数据项序号                      *
*****************************************************************/
{
    register int i,s,width,length,mem_i;
    char     *buf;
    struct MEM_PACKET * curr_pack;
    s = 0;
    mem_data = (void *)aMem;
    width  = mem_data[0].width;
    length = mem_data[0].length;
    mem_i  = mem_data[0].ri;
    buf = (void *)(aMem + width * mem_i);
    curr_pack = (struct MEM_PACKET *)buf;
    if  (curr_pack->len != 0){/*第一个字节为0说明该部分为空*/
            memcpy(aBuf,curr_pack->packetp,curr_pack->len);
            *size = curr_pack->len;
            curr_pack->len = 0;
            s = mem_data[0].ri;
            mem_data[0].ri++;
            if(mem_data[0].ri >= length)
                    mem_data[0].ri = 1;
            goto ret;
        }
    
    for (i=1;i<length;i++){
            mem_i++;/*继续向后找，最糟糕的情况是把整个缓冲区都找一遍*/
            if  (mem_i >= length)
                mem_i = 1;
            buf = (void *)(aMem + width*mem_i);
            curr_pack = (struct MEM_PACKET *)buf;
            if  (curr_pack->len == 0)
                    continue;
            memcpy(aBuf,curr_pack->packetp,curr_pack->len);
            *size = curr_pack->len;
            curr_pack->len = 0;
            s = mem_data[0].ri = mem_i;
            mem_data[0].ri++;
            if(mem_data[0].ri >= length)
            mem_data[0].ri = 1;
            break;
        }
    ret:
    return s;
}
int main()
{
    char *su1_2;
    char receive[1500];
    int i,j;
    int fd;
    int fd_procaddr;
    unsigned int size;
    char addr[9];
    unsigned long ADDR;
    
    j = 0;
    /*open device 'mem' as a media to access the RAM*/
    fd=open("/dev/mem",O_RDWR);    
    fd_procaddr = open("/proc/nf_addr",O_RDONLY);
    read(fd_procaddr,addr,9);
    ADDR = atol(addr);
    close(fd_procaddr);
    printf("%u[%8lx]\n",ADDR,ADDR);
    /*Map the address in kernel to user space, use mmap function*/
    su1_2 = mmap(0,PAGES*4*1024, PROT_READ|PROT_WRITE, MAP_SHARED, fd, ADDR);
    perror("mmap");
    while(1)
    {
        bzero(receive,1500);
        i = get_mem(su1_2,receive,&size);
        if (i != 0)
        {
            j++;
            printf("%d:%s[size = %d]\n",j,receive,size);
        }    
        else
        {
            printf("there have no data\n");
            munmap(su1_2,PAGES*4*1024);
            close(fd);
            break;
        }
    }
    while(1);
}


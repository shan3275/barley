#include "frcore.h"
#include "frc_util.h"

#include "frcore_stat.h"
#include "frcore_debug.h"
#include "frcore_cmd.h"
extern CVMX_SHARED  uint64_t            cpu_freq;

void frcore_stat_add64(int cnt, int64_t value)
{
    cvmx_fau_atomic_add64(FRCORE_STAT_REG(cnt), value);
}

void frcore_stat_set64(int cnt, int64_t value)
{
    cvmx_fau_atomic_write64(FRCORE_STAT_REG(cnt), value);
}

int64_t frcore_stat_fetch_and_add64(int cnt, uint64_t value)
{
    return cvmx_fau_fetch_and_add64(FRCORE_STAT_REG(cnt), value);
}



void frcore_stat_dump()
{
    uint64_t v64;
    int i;
    char stat_name[FRC_STAT_NAME_SIZE];
    uint64_t seconds;
    //frcore_console_lock();
    seconds = cvmx_get_cycle() / cpu_freq;
    printf("CYCLE %lld up time:%.5d:%.2d:%.2d\n", (ULL) cvmx_get_cycle(),
           (int)seconds/3600, (int)(seconds%3600)/60, (int)seconds%60);

    for (i = 0; i < stat_max; i++)
    {
        v64 = 0;
        v64 = FRCORE_STAT_VAL(i);
        if (v64 > 0)  {
            frc_stat_name_get(i, stat_name);
            printf("%-20s: %lld\n", stat_name, (ULL) v64);
        }
    }

    //frcore_console_unlock();
}

void frcore_stat_clear()
{
    int i;

    for (i = 0; i < stat_max; i++)
    {
       #if FRC_CONFIG_SSN
       if (i == stat_ssn_active_num) {
          continue;
       }
       #endif
        cvmx_fau_atomic_write64(FRCORE_STAT_REG(i), 0);
    }
}

int frcore_cmd_get_stat(uint16_t plen, void *param, uint16_t *olen, void *outbuf)
{
    int i;
    frc_stat_op_in_t *input = (frc_stat_op_in_t *) param;
    uint64_t *v64p;

    FRCORE_CMD("plen %d, param %p, *olne %d, outbuf %p.\n", plen, param, *olen, outbuf);

    v64p = outbuf;
    for (i = 0; i < input->num; i++, v64p++)
    {
        *v64p = FRCORE_STAT_VAL(input->index + i);
    }

    *olen = input->num * sizeof(uint64_t);

    return 0;
}

int frcore_cmd_clear_stat(uint16_t plen, void *param, uint16_t *olen, void *outbuf)
{
    *olen = 0;
    frcore_stat_clear();
    return FRE_SUCCESS;
}

int frcore_stat_init()
{
    frcore_stat_clear();

    FRCORE_REGISTER_CMD(CMD_TYPE_USER, USER_CMD_GET_STAT, frcore_cmd_get_stat);
    FRCORE_REGISTER_CMD(CMD_TYPE_USER, USER_CMD_CLEAR_STAT, frcore_cmd_clear_stat);

    return 0;
}

char *pool_name[] = {
        "Packet Data Buffer",
        "Test              ",
        "WQE               ",
        "PKO               ",
        "Timer             ",
};

void frcore_print_pool_count_stats(void)
{
        printf("Pool stats: \n");
        printf("[Pool %d <%18s>: size %.4d entry %.7d free %.7llu ]\n", 0, pool_name[0],
               CVMX_FPA_POOL_0_SIZE, FPA_PACKET_POOL_COUNT,cast64(cvmx_read_csr(CVMX_FPA_QUEX_AVAILABLE(0))));
        printf("[Pool %d <%18s>: size %.4d entry %.7d free %.7llu ]\n", 1, pool_name[1],
               CVMX_FPA_POOL_1_SIZE, 0,cast64(cvmx_read_csr(CVMX_FPA_QUEX_AVAILABLE(1))));
        printf("[Pool %d <%18s>: size %.4d entry %.7d free %.7llu ]\n", 2, pool_name[2],
               CVMX_FPA_POOL_2_SIZE, FPA_WQE_POOL_COUNT,cast64(cvmx_read_csr(CVMX_FPA_QUEX_AVAILABLE(2))));
        printf("[Pool %d <%18s>: size %.4d entry %.7d free %.7llu ]\n", 3, pool_name[3],
               CVMX_FPA_POOL_3_SIZE, FPA_OQ_POOL_COUNT,cast64(cvmx_read_csr(CVMX_FPA_QUEX_AVAILABLE(3))));
        printf("[Pool %d <%18s>: size %.4d entry %.7d free %.7llu ]\n", 4, pool_name[4],
               CVMX_FPA_POOL_4_SIZE, FPA_TIMER_POOL_COUNT,cast64(cvmx_read_csr(CVMX_FPA_QUEX_AVAILABLE(4))));
}


void
frcore_core_print_pow(void)
{
  int k = 0;
  printf("-- POW/SSO queue info --\n");
  for (k = 0; k < 8; k++)
    {
      cvmx_pow_iq_cntx_t iq_cnt;
      iq_cnt.u64 = cvmx_read_csr(CVMX_POW_IQ_CNTX(k));
      printf(" QoS [%d], in queue using %d\n", k, iq_cnt.s.iq_cnt);
    }
  {
    cvmx_pow_iq_com_cnt_t com_cnt;
    com_cnt.u64 = cvmx_read_csr(CVMX_POW_IQ_COM_CNT);
    printf(" IQ All %d\n", com_cnt.s.iq_cnt);
  }
  {
    cvmx_pow_nos_cnt_t nos_cnt;
    nos_cnt.u64 = cvmx_read_csr(CVMX_POW_NOS_CNT);
    printf(" Noschedule Count %u\n", nos_cnt.s.nos_cnt);
  }

  for (k = 0; k < 8; k++)
    {
      cvmx_pow_qos_thrx_t thrx;
      thrx.u64 = cvmx_read_csr(CVMX_POW_QOS_THRX(k));
      printf(" threshold, QoS %d, DES_CNT %04d, BUF_CNT %04d, FREE_CNT %04d, MAX_THR %04d, MIN_THR %04d\n",
             k, thrx.s.des_cnt, thrx.s.buf_cnt, thrx.s.free_cnt, thrx.s.max_thr, thrx.s.min_thr);
    }
}


void
frcore_core_print_pip_pko_stat(void)
{
  cvmx_pip_port_status_t  stat;
  cvmx_pko_port_status_t  stat_pko;
  int i, first_port = 0, num_ports = 6;
  int port[32] = {0, 16, 32, 33, 34, 35};


  printf("\nPIP Stat:\n");
  for(i = 0; i < num_ports; i++) {
    cvmx_pip_get_port_status(port[i], 0, &stat);
    if (stat.inb_packets)
      printf("port %02d, [recv total %u drop %u, err %u], proc %u,mcast %u,bcast %u,crc err %u,min err %u,max err %u\n",
             port[i], stat.inb_packets, stat.dropped_packets, stat.inb_errors, stat.packets, stat.multicast_packets,
             stat.broadcast_packets, stat.fcs_align_err_packets, stat.runt_packets, stat.oversize_packets);
  }

  printf("\nPKO Stat:\n");
  for(i = 0; i < num_ports; i++) {
    cvmx_pko_get_port_status(port[i], 0, &stat_pko);
    if(stat_pko.packets || stat_pko.doorbell)
      printf("Port %02d: doorbell: %llu send pkts: %u \n", port[i], cast64(stat_pko.doorbell), stat_pko.packets);
  }

}

/* End of file */

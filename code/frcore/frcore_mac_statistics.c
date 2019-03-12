#include "frcore_mac_statistics.h"
#include "frcore_proto.h"
#include <string.h>
#if FRC_CONFIG_MAC_STATISTICS

#define MAX_MAC_NUM 512
CVMX_SHARED uint16_t mac_num = 0;
CVMX_SHARED uint16_t hash_mode = HASH_SIP;
CVMX_SHARED uint16_t heart_beat = HEART_BEAT_ON;
CVMX_SHARED struct frc_mac_stat global_mac_stat[MAX_MAC_NUM];
CVMX_SHARED int64_t ip_tos = -1;
CVMX_SHARED int64_t ip_ttl = -1;

uint16_t frcore_mac_statistics_hash(uint32_t sip, uint32_t dip, uint16_t sp, uint16_t dp, uint8_t proto)
{
    uint16_t hash = MAX_MAC_NUM + 1;
    if (mac_num == 0)
    {
        return hash;
    }
    switch (hash_mode)
    {
    case HASH_SIP:
                  hash = ((uint16_t)((sip >> 16) + (sip & 0xffff))) % mac_num;
                  break;
        case HASH_DIP:
                  hash = ((uint16_t)((dip >> 16) + (dip & 0xffff))) % mac_num;
                  break;
        case HASH_SIP_DIP:
                  hash = ((((uint16_t)((sip >> 16) + (sip & 0xffff)))) + (((uint16_t)((dip >> 16) + (dip & 0xffff))))) % mac_num;
                  break;
        case HASH_FIVE_TUPLE:
                  hash = ((((uint16_t)((sip >> 16) + (sip & 0xffff)))) + (((uint16_t)((dip >> 16) + (dip & 0xffff)))) + sp + dp + proto) % mac_num;
                  break;
        default:
                  break;;
    }
    return hash;
}

int frcore_mac_statistics_add_mac(uint16_t plen, void *param, uint16_t *olen, void *outbuf)
{
    frc_mac_stat_in_t *input = (frc_mac_stat_in_t *) param;
    uint64_t mac = 0;
    mac = input->mac;
    if (mac == 0)
    {
        return FRE_FAIL;
    }
    *olen = 0;
    if (mac_num == MAX_MAC_NUM)
    {
        return FRE_NOSPACE;
    }
    global_mac_stat[mac_num].mac = mac;
    global_mac_stat[mac_num].total = 0;
    global_mac_stat[mac_num].errors = 0;
    mac_num++;
    return FRE_SUCCESS;
}

int frcore_mac_statistics_del_mac(uint16_t plen, void *param, uint16_t *olen, void *outbuf)
{
    frc_mac_stat_in_t *input = (frc_mac_stat_in_t *) param;
    uint64_t mac = 0;
    mac = input->mac;
    *olen = 0;
    uint16_t i;

    if (mac == 0)
    {
        return FRE_FAIL;
    }

    for (i = 0; i < mac_num; i++)
    {
        if (global_mac_stat[i].mac == mac)
        {
            global_mac_stat[i].mac == 0;
            global_mac_stat[i].total = 0;
            global_mac_stat[i].errors = 0;
            mac_num--;

            for (; i < mac_num; i++)
            {
                    global_mac_stat[i].mac = global_mac_stat[i+1].mac;
                    global_mac_stat[i].total = global_mac_stat[i+1].total;
                    global_mac_stat[i].errors = global_mac_stat[i+1].errors;
            }
            return FRE_SUCCESS;
        }
        else
        {
            return FRE_NOTFOUND;
        }
    }
    return FRE_FAIL;
}

int frcore_mac_statistics_del_all(uint16_t plen, void *param, uint16_t *olen, void *outbuf)
{
    uint16_t i;
    for (i = 0; i < mac_num; i++)
    {
        memset(&(global_mac_stat[i]), 0, sizeof(struct frc_mac_stat));
    }
    mac_num = 0;
    return FRE_SUCCESS;
}

int frcore_mac_statistics_clear_counter(uint16_t plen, void *param, uint16_t *olen, void *outbuf)
{
    frc_mac_stat_in_t *input = (frc_mac_stat_in_t *) param;
    uint64_t mac = 0;
    mac = input->mac;
    uint16_t i;
    for (i = 0; i < mac_num; i++)
    {
        if (global_mac_stat[i].mac == mac)
        {
            global_mac_stat[i].total = 0;
            global_mac_stat[i].errors = 0;
            return FRE_SUCCESS;
        }
    }
    return FRE_NOTFOUND;
}

int frcore_mac_statistics_clear_all_counter(uint16_t plen, void *param, uint16_t *olen, void *outbuf)
{
    uint16_t i;

    for (i = 0; i < mac_num; i++)
    {
        global_mac_stat[i].total = 0;
        global_mac_stat[i].errors = 0;
    }
    return FRE_SUCCESS;
}

int frcore_mac_statistics_show_by_mac(uint16_t plen, void *param, uint16_t *olen, void *outbuf)
{
    frc_mac_stat_in_t *input = (frc_mac_stat_in_t *) param;
    uint64_t mac = 0;
    mac = input->mac;
    *olen = sizeof(frc_mac_stat_out_t);
    frc_mac_stat_out_t *output = outbuf;

    if (mac == 0)
    {
        return FRE_PARAM;
    }
    uint16_t i;
    for (i = 0; i < mac_num; i++)
    {
        if (global_mac_stat[i].mac == mac)
        {
            output->mac = mac;
            output->total = global_mac_stat[i].total;
            output->errors = global_mac_stat[i].errors;
            return FRE_SUCCESS;
        }
    }
    return FRE_NOTFOUND;
}

int frcore_mac_statistics_show_by_index(uint16_t plen, void *param, uint16_t *olen, void *outbuf)
{
    frc_mac_stat_in_t *input = (frc_mac_stat_in_t *)param;
    uint64_t index = input->mac;
    frc_mac_stat_out_t *out = outbuf;
    *olen = sizeof(frc_mac_stat_out_t);

    if (index >= mac_num)
    {
        return FRE_FAIL;
    }

    out->mac = global_mac_stat[index].mac;
    out->total = global_mac_stat[index].total;
    out->errors = global_mac_stat[index].errors;
    return FRE_SUCCESS;
}

int frcore_mac_statistics_set_hash_mode(uint16_t plen, void *param, uint16_t *olen, void *outbuf)
{
    frc_mac_stat_in_t *input = (frc_mac_stat_in_t *)param;
    hash_mode = input->mac;
    return FRE_SUCCESS;
}
int frcore_mac_statistics_set_heart_beat(uint16_t plen, void *param, uint16_t *olen, void *outbuf)
{
    frc_mac_stat_in_t *input = (frc_mac_stat_in_t *)param;
    heart_beat = (uint16_t)(input->mac);
    return FRE_SUCCESS;
}

int frcore_mac_statistics_set_ip(uint16_t plen, void *param, uint16_t *olen, void *outbuf)
{
    frc_mac_stat_in_t *input = (frc_mac_stat_in_t *)param;
    uint64_t type = input->mac;

    if (type == IP_TOS)
    {
        ip_tos = (int64_t)(input->counter);
    }
    else if (type == IP_TTL)
    {
        ip_ttl = (int64_t)(input->counter);
    }
    else
    {
        return FRE_FAIL;
    }
    return FRE_SUCCESS;
}

static inline uint16_t check_sum(uint8_t *addr, uint16_t count)
{
    uint32_t sum = 0;

    while (count > 1)
    {
        sum += *(uint16_t *)addr++;
        count -= 2;
    }

    if (count > 0)
    {
        sum += *(uint16_t *) addr;
    }

    sum = (sum & 0xffff) + (sum >> 16);
    sum += (sum >> 16);
    return ~sum;
}

static inline void frcore_mac_statistics_set_ip_pkt(struct iphdr *ip_header)
{
    if (ip_tos >= 0)
    {
        ip_header->tos = ip_tos;
    }
    if (ip_ttl >=0)
    {
        ip_header->ttl = ip_ttl;
    }
    ip_header->check = 0;
    ip_header->check = check_sum((uint8_t *)ip_header, ip_header->tot_len);
}

void pkt_dump(uint8_t *ether, int length)
{
    printf("Packet length: %d\n", length);

    int i;
    for(i = 0; i < length; i++)
    {
         printf(" %02x", ether[i]);
        if( (i + 1) % 16 == 0 )
        {
            printf("\n");
        }
    }
    printf("\n\n");
}

int frcore_mac_statistics_counter_inc(uint8_t *ether)
{
    uint64_t mac = (*(uint64_t *)(ether)) >> 16;
    //skip the user definition length
    struct ethhdr *ether_ptr = (struct ethhdr *)(ether + 16);

//    printf("ether mac = %llx====================proto = %x\n", mac, ether_ptr->h_proto);
    //only deal with ip packet
    if (ether_ptr->h_proto != ETH_P_IP)
    {
        return FRE_FAIL;
    }

//    printf("ip====================\n");
    struct iphdr *ip_ptr = (struct iphdr *)((uint8_t *)ether_ptr + sizeof(struct ethhdr));
    frcore_mac_statistics_set_ip_pkt(ip_ptr);
    //only deal with ipv4
    if (ip_ptr->version != 0x4)
    {
        return FRE_FAIL;
    }
    uint32_t sip = ip_ptr->saddr;
    uint32_t dip = ip_ptr->daddr;
    uint8_t proto = ip_ptr->protocol;
    uint16_t sp;
    uint16_t dp;
    if (proto == IP_P_TCP)
    {
//        printf("tcp=================================\n");
        struct tcphdr *tcp_ptr = (struct tcphdr *)((uint8_t *)ip_ptr + (ip_ptr->ihl) * 4);
        sp = tcp_ptr->source;
        dp = tcp_ptr->dest;
    }
    else if (proto == IP_P_UDP)
    {
//        printf("udp================================\n");
        struct udphdr *udp_ptr = (struct tcphdr *)((uint8_t *)ip_ptr + (ip_ptr->ihl) * 4);
        sp = udp_ptr->source;
        dp = udp_ptr->dest;
    }
    else
    {
        return FRE_FAIL;
    }

    uint16_t hash = frcore_mac_statistics_hash(sip, dip, sp, dp, proto);
//    printf("sip = %x dip = %x sp = %d dp = %d proto = %d  hash = %d \n", sip, dip, sp, dp, proto, hash);
//    printf("tos = %d  ttl = %d======================\n", ip_ptr->tos, ip_ptr->ttl);
    if (hash == (MAX_MAC_NUM + 1))
    {
        return FRE_FAIL;
    }
    if (global_mac_stat[hash].mac == 0)
    {
        return FRE_FAIL;
    }
    if (global_mac_stat[hash].mac == mac)
    {
        cvmx_atomic_fetch_and_add64(&(global_mac_stat[hash].total), 1);
    }
    else
    {
        cvmx_atomic_fetch_and_add64(&(global_mac_stat[hash].total), 1);
        cvmx_atomic_fetch_and_add64(&(global_mac_stat[hash].errors), 1);
    }
    return FRE_SUCCESS;
}

int frcore_mac_statistics_init()
{
    FRCORE_REGISTER_CMD(CMD_TYPE_USER, USER_CMD_MAC_STAT_ADD_MAC, frcore_mac_statistics_add_mac);
    FRCORE_REGISTER_CMD(CMD_TYPE_USER, USER_CMD_MAC_STAT_DEL_MAC, frcore_mac_statistics_del_mac);
    FRCORE_REGISTER_CMD(CMD_TYPE_USER, USER_CMD_MAC_STAT_DEL_ALL, frcore_mac_statistics_del_all);
    FRCORE_REGISTER_CMD(CMD_TYPE_USER, USER_CMD_MAC_STAT_CLEAR_COUNTER, frcore_mac_statistics_clear_counter);
    FRCORE_REGISTER_CMD(CMD_TYPE_USER, USER_CMD_MAC_STAT_CLEAR_ALL_COUNTER, frcore_mac_statistics_clear_all_counter);
    FRCORE_REGISTER_CMD(CMD_TYPE_USER, USER_CMD_MAC_STAT_SHOW_COUNTER, frcore_mac_statistics_show_by_mac);
    FRCORE_REGISTER_CMD(CMD_TYPE_USER, USER_CMD_MAC_STAT_SHOW_ALL_COUNTER, frcore_mac_statistics_show_by_index);
    FRCORE_REGISTER_CMD(CMD_TYPE_USER, USER_CMD_MAC_STAT_SET_HASH_MODE, frcore_mac_statistics_set_hash_mode);
    FRCORE_REGISTER_CMD(CMD_TYPE_USER, USER_CMD_MAC_STAT_HEART_BEAT, frcore_mac_statistics_set_heart_beat);
    FRCORE_REGISTER_CMD(CMD_TYPE_USER, USER_CMD_MAC_STAT_SET_IP, frcore_mac_statistics_set_ip);
}

#endif

#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include "frctweak.h"
#include "frc_pack.h"
#include "frc_dma.h"
#include "cvmx-swap.h"
#include "frc_api.h"
#include "frc_debug.h"
#include <arpa/inet.h>


#include "frctweak_arg_parser.h"

#if FRC_CONFIG_IPC

void frctweak_cmd_ipc_usage()
{
    printf("Usage: %s ipc clear\n", program);
    printf("          ipc misc [smac_check=y/n] [pktid_check=y/n] [exp_check=y/n]\n");
    printf("                   [invaild_dump=0xHH] [instr_send=y/n]\n");
    printf("          ipc misc clear\n");
    printf("          ipc instr [sipd=A.B.C.D] [sipm=A.B.C.D] [dipd=A.B.C.D] [dipm=A.B.C.D]\n"); 
    printf("                    [type=0|1|2|3|4|5] [action=0|1] [payload=FILE]\n");
    printf("          ipc instr clear\n");
    printf("          ipc exp [ouid=0xHH] [ouim=0xHH] [pidd=0xHH] [pidm=0xHH]  [iifd=0xHH] [iifm=0xHH]\n");
    printf("          ipc exp clear\n");
    printf("          ipc cur [smac=HH:HH:HH:HH:HH:HH] [pktid=0xHH]\n");
    printf("          ipc cur clear\n");
    printf("          ipc vlan_hash [v4/v6] sip/dip A.B.C.D/AA::BB\n");
}


#define FRCTWEAK_IPC_FILED_DUMP(_val, _field, _func) \
    { \
        if (NULL == _func) \
        { \
            printf("  %16s : 0x%x\n", #_field, _val._field); \
        } \
        else \
        { \
            char _str[256]; \
            _func(_val._field, _str); \
            printf("  %16s : %s\n", #_field, _str); \
        } \
    }

int frctweak_ipc_misc_clear()
{
    int rv;
    ipc_misc_t misc;

    memset(&misc, 0, sizeof(ipc_misc_t));

    rv = frcapi_ipc_misc_set(&misc);

     if (FRE_SUCCESS == rv)
    {
        printf("IPC misc reset success.\n");
    }
    else
    {
        printf("Error: IPC misc reset fail!(rv=%d)\n", rv);
    }

    return rv;
}

int frctweak_cmd_ipc_misc(int argc, char **argv)
{
    int rv = 0;
    ipc_misc_t misc;
    arg_parser_t parser = {};

    rv = frcapi_ipc_misc_get(&misc);
    

    if (FRE_SUCCESS != rv)
    {
        printf("ERROR: ipc misc get fail!(rv=%d)\n", rv);
        return rv;
    }

    swap_buff(sizeof(ipc_misc_t) >> 3, &misc);
    printf("%s.%d:smac_check = %d, pktid_check = %d\n", __func__, __LINE__, misc.smac_check, misc.pktid_check);

    if (1 == argc)
    {
        printf("IPC MISC:\n"); 

        FRCTWEAK_IPC_FILED_DUMP(misc, smac_check, u8_string);
        FRCTWEAK_IPC_FILED_DUMP(misc, pktid_check, u8_string);
        FRCTWEAK_IPC_FILED_DUMP(misc, exp_check, u8_string);
        FRCTWEAK_IPC_FILED_DUMP(misc, invalid_dump, u8_string);
        FRCTWEAK_IPC_FILED_DUMP(misc, instr_send, u8_string);
    }
    else if (!strcasecmp("clear", argv[1]))
    {
        rv = frctweak_ipc_misc_clear();
    }
    else
    {
        frctweak_arg_parser_init(&parser);

        frctweak_arg_parser_add(&parser,"smac_check", parse_u8, &misc.smac_check);
        frctweak_arg_parser_add(&parser,"pktid_check", parse_u8, &misc.pktid_check);
        frctweak_arg_parser_add(&parser,"exp_check", parse_u8, &misc.exp_check);
        frctweak_arg_parser_add(&parser,"invalid_dump", parse_u8, &misc.invalid_dump);
        frctweak_arg_parser_add(&parser,"instr_send", parse_u8, &misc.instr_send);
        printf("argc = %d, argv[0] = %s\n", argc, argv[0]);

        rv = frctweak_arg_parsing(&parser, --argc, ++argv);

        frctweak_arg_parser_free(&parser);

        if (FRE_SUCCESS != rv)
        {
            frctweak_cmd_ipc_usage();
            return FRE_PARAM;
        }

        rv = frcapi_ipc_misc_set(&misc);

        if (FRE_SUCCESS == rv)
        {
            printf("IPC misc set success.\n");
        }
        else
        {
            printf("Error: IPC misc set fail!(rv=%d)\n", rv);
        }
    }

    return rv;
   
}

int frctweak_ipc_cur_clear()
{
    int rv;
    ipc_cur_t cur;

    memset(&cur, 0, sizeof(ipc_cur_t));

    rv = frcapi_ipc_cur_set(&cur);

     if (FRE_SUCCESS == rv)
    {
        printf("IPC cur reset success.\n");
    }
    else
    {
        printf("Error: IPC cur reset fail!(rv=%d)\n", rv);
    }

    return rv;
}

int frctweak_cmd_ipc_cur(int argc, char **argv)
{
    int rv = 0;
    ipc_cur_t cur;
    arg_parser_t parser;

    if (1 == argc)
    {
        rv = frcapi_ipc_cur_get(&cur);

        if (FRE_SUCCESS != rv)
        {
            printf("ERROR: ipc cur get fail!(rv=%d)\n", rv);
            return rv;
        }

        printf("IPC CUR:\n"); 
        swap_buff(2, &cur);

        FRCTWEAK_IPC_FILED_DUMP(cur, smac, mac_string);
        FRCTWEAK_IPC_FILED_DUMP(cur, pktid, u64_string);

        rv = FRE_SUCCESS;
    }
    else if (!strcasecmp("clear", argv[1]))
    {
        rv = frctweak_ipc_cur_clear();
    }
    else 
    {
        rv = frcapi_ipc_cur_get(&cur);

        if (FRE_SUCCESS != rv)
        {
            printf("ERROR: ipc cur get fail!(rv=%d)\n", rv);
            return rv;
        }
        swap_buff(2, &cur);

        frctweak_arg_parser_init(&parser);
        frctweak_arg_parser_add(&parser, "smac", parse_mac64, (void *)&cur.smac);
        frctweak_arg_parser_add(&parser, "pktid", parse_u64, (void *)&cur.pktid);

        rv = frctweak_arg_parsing(&parser, argc, argv);

        frctweak_arg_parser_free(&parser);

        if (FRE_SUCCESS != rv)
        {
            frctweak_cmd_ipc_usage();
            return FRE_PARAM;
        }

        rv = frcapi_ipc_cur_set(&cur);

        if (FRE_SUCCESS == rv)
        {
            printf("IPC cur set success.\n");
        }
        else
        {
            printf("Error: IPC cur set fail!(rv=%d)\n", rv);
        }
    }

    return rv;
}

int parse_hash_type(char *str, uint32_t *type)
{
    if (!strcmp(str, "sip"))
    {
        *type = FRC_VLAN_CHECK_SIP; 
    }
    else if (!strcmp(str, "dip"))
    {
        *type = FRC_VLAN_CHECK_DIP; 
    }
    else
    {
        return FRE_PARAM;
    }
    return FRE_SUCCESS;
}

int frctweak_cmd_ipc_vlan_hash(int argc, char **argv)
{
    int i;
    int rv = 0;
    frc_vlan_hash_mask_t hash_mask = {};

    if (1 == argc)
    {
        #if 0
        //rv = frcapi_ipc_cur_get(&cur);

        if (FRE_SUCCESS != rv)
        {
            printf("ERROR: ipc cur get fail!(rv=%d)\n", rv);
            return rv;
        }

        printf("IPC CUR:\n"); 
        swap_buff(2, &cur);

        FRCTWEAK_IPC_FILED_DUMP(cur, smac, mac_string);
        FRCTWEAK_IPC_FILED_DUMP(cur, pktid, u64_string);

        rv = FRE_SUCCESS;
        #endif
    }
    else if (!strcasecmp("v4", argv[1]))
    {
        if (parse_hash_type(argv[2], &hash_mask.hash_type))
        {
            return FRE_PARAM;
        }
        if (parse_ipv4(argv[3], &hash_mask.ip4))
        {
            return FRE_PARAM;
        }
        printf("ip4=0x%x\n", hash_mask.ip4);
        rv = frctweak_ipc_hash4_mask_set(&hash_mask);
        
    }
    else if (!strcasecmp("v6", argv[1]))
    {
        if (parse_hash_type(argv[2], &hash_mask.hash_type))
        {
            return FRE_PARAM;
        }
        if (inet_pton(AF_INET6, argv[3], hash_mask.ip6) <= 0)
        {
            return FRE_PARAM;
        }
        for (i = 0; i < 16; i++)
        {
            printf("%d, ", hash_mask.ip6[i]);
        }
        printf("\n");
        rv = frctweak_ipc_hash6_mask_set(&hash_mask);
    }

    if (FRE_SUCCESS == rv)
    {
        printf("IPC hash mask set success.\n");
    }
    else
    {
        printf("Error: IPC hash mask set fail!(rv=%d)\n", rv);
    }

    return rv;
}




int frctweak_ipc_exp_clear()
{
    int rv;
    ipc_exp_t exp;
    memset(&exp, 0, sizeof(ipc_exp_t));

    rv = frcapi_ipc_exp_set(&exp);

     if (FRE_SUCCESS == rv)
    {
        printf("IPC exp reset success.\n");
    }
    else
    {
        printf("Error: IPC exp reset fail!(rv=%d)\n", rv);
    }

    return rv;
}

int frctweak_cmd_ipc_exp(int argc, char **argv)
{
    int rv = 0;
    ipc_exp_t exp;
    arg_parser_t parser;

    if (1 == argc)
    {
        rv = frcapi_ipc_exp_get(&exp);

        if (FRE_SUCCESS != rv)
        {
            printf("ERROR: ipc exp get fail!(rv=%d)\n", rv);
            return rv;
        }
        swap_buff(sizeof(ipc_exp_t)>>3, &exp);

        printf("IPC exp:\n");
        
        printf("N: iifd = 0x%x, iifm=0x%x\n", exp.iifd, exp.iifm); 

        FRCTWEAK_IPC_FILED_DUMP(exp, ouid, u16_string);
        FRCTWEAK_IPC_FILED_DUMP(exp, ouim, u16_string);

        FRCTWEAK_IPC_FILED_DUMP(exp, pidd, u64_string);
        FRCTWEAK_IPC_FILED_DUMP(exp, pidm, u64_string);

        FRCTWEAK_IPC_FILED_DUMP(exp, iifd, u16_string);
        FRCTWEAK_IPC_FILED_DUMP(exp, iifm, u16_string);

    }
    else if (!strcasecmp("clear", argv[1]))
    {
        rv = frctweak_ipc_exp_clear();
    }
    else 
    {
        rv = frcapi_ipc_exp_get(&exp);

        if (FRE_SUCCESS != rv)
        {
            printf("ERROR: ipc exp get fail!(rv=%d)\n", rv);
            return rv;
        }
        swap_buff(sizeof(ipc_exp_t)>>3, &exp);

        frctweak_arg_parser_init(&parser);

        frctweak_arg_parser_add(&parser,"ouid", parse_u16, &exp.ouid);
        frctweak_arg_parser_add(&parser,"pidd", parse_u64, &exp.pidd);
        frctweak_arg_parser_add(&parser,"iifd", parse_u16, &exp.iifd);

        frctweak_arg_parser_add(&parser,"ouim", parse_u16, &exp.ouim);
        frctweak_arg_parser_add(&parser,"pidm", parse_u64, &exp.pidm);
        frctweak_arg_parser_add(&parser,"iifm", parse_u16, &exp.iifm);

        rv = frctweak_arg_parsing(&parser, argc, argv);

        frctweak_arg_parser_free(&parser);

        if (FRE_SUCCESS != rv)
        {
            frctweak_cmd_ipc_usage();
            return FRE_PARAM;
        }
        printf("RRRR :ouid =%d, ouim=%d, iifd=%d, iifm=%d\n", exp.ouid, exp.ouim, exp.iifd, exp.iifm);

        rv = frcapi_ipc_exp_set(&exp);

        if (FRE_SUCCESS == rv)
        {
            printf("IPC exp set success.\n");
        }
        else
        {
            printf("Error: IPC exp set fail!(rv=%d)\n", rv);
        }
    }

    return rv;
   
}

int frctweak_ipc_instr_clear()
{
    int rv;
    ipc_instr_t instr;

    memset(&instr, 0, sizeof(ipc_instr_t));

    rv = frcapi_ipc_instr_set(&instr);

     if (FRE_SUCCESS == rv)
    {
        printf("IPC instr reset success.\n");
    }
    else
    {
        printf("Error: IPC instr reset fail!(rv=%d)\n", rv);
    }

    return rv;
}

#define LINE_SZ 32

void payload_dump(int len, char *buff)
{
    int i; 

    for (i = 0; i < len; i++)
    {
        if (0 == (i % LINE_SZ))
        {
            printf("        ");
        }
        else if (((LINE_SZ - 1) == (i % LINE_SZ)) || (i == (len - 1)))
        {
            printf("\n");
        }
        else 
        {
            printf(" %.2x", buff[i]);
        }
    }
}

int parse_payload(char *file, char *buff)
{
    int len = 0;
    int c = 0;
    char line[256];
    FILE *fp = NULL;

    int rv = system("dir payload.txt");
    if (rv)
    {
        printf("cao\n");
    }
    printf("file=%s\n", file);
    fp = fopen(file , "r");

    if (NULL == fp)
    {
        return FRE_OPEN;
    }

    memset(line, 0, 256);
    while (fgets(line, 256, fp) != NULL)
    {
        //printf("%s.%d, line = %s\n", __func__, __LINE__, line);
        len = strlen(line);

        char *p; 
        p=strtok(line," "); 
        while(p) 
        { 
            //printf("%s.%d, p = %s\n", __func__, __LINE__, p);
            if (c >= IPC_IP_LEN_MAX)
            {
                return FRE_FAIL;
            }
            if (parse_u8(p, &buff[c++])) 
            {
                return FRE_FAIL;
            }
            p=strtok(NULL," "); 
        } 
    }

    return FRE_SUCCESS;
}

int frctweak_cmd_ipc_instr(int argc, char **argv)
{
    uint64_t i = 0, n = 0;
    int rv = 0;
    ipc_instr_t instr = {};
    ipc_payload_set_in_t payload_set = {};
    arg_parser_t parser;

    if (1 == argc)
    {
        rv = frcapi_ipc_instr_get(&instr.instr_cfg);

        if (FRE_SUCCESS != rv)
        {
            printf("ERROR: ipc instr get fail!(rv=%d)\n", rv);
            return rv;
        }
        swap_buff(sizeof(ipc_instr_cfg_t) >> 3, &instr.instr_cfg);

        printf("IPC instr:\n"); 

        /*
            uint32_t sip_data;
            uint32_t sip_mask;
            uint32_t dip_data;
            uint32_t dip_mask;     
            uint16_t pkt_len;
            uint16_t type   : 4;
                     ip_len : 12;
            uint8_t  payload[IPC_CMD_PAYLEN_MAX]; 
        */

        FRCTWEAK_IPC_FILED_DUMP(instr.instr_cfg, sip_data, fr_ipv4_string);
        FRCTWEAK_IPC_FILED_DUMP(instr.instr_cfg, sip_mask, fr_ipv4_string);
        FRCTWEAK_IPC_FILED_DUMP(instr.instr_cfg, dip_data, fr_ipv4_string);
        FRCTWEAK_IPC_FILED_DUMP(instr.instr_cfg, dip_mask, fr_ipv4_string);
        FRCTWEAK_IPC_FILED_DUMP(instr.instr_cfg, type,     fr_u32_string);
        FRCTWEAK_IPC_FILED_DUMP(instr.instr_cfg, ip_len,   fr_u32_string);
        FRCTWEAK_IPC_FILED_DUMP(instr.instr_cfg, tx_port,   fr_u32_string);
        FRCTWEAK_IPC_FILED_DUMP(instr.instr_cfg, action,     fr_u32_string);
        //FRCTWEAK_IPC_FILED_DUMP(instr, payload,  parse_payload);

        payload_dump(instr.instr_cfg.ip_len, instr.payload);

        rv = FRE_SUCCESS;
    }
    else if (!strcasecmp("clear", argv[1]))
    {
        rv = frctweak_ipc_instr_clear();
    }
    else 
    {
        rv = frcapi_ipc_instr_get(&instr.instr_cfg);

        if (FRE_SUCCESS != rv)
        {
            printf("ERROR: ipc instr get fail!(rv=%d)\n", rv);
            return rv;
        }

        swap_buff(sizeof(ipc_instr_cfg_t) >> 3, &instr.instr_cfg);

        frctweak_arg_parser_init(&parser);
        frctweak_arg_parser_add(&parser,"sipd", parse_ipv4, &instr.instr_cfg.sip_data);
        frctweak_arg_parser_add(&parser,"sipm",  parse_ipv4, &instr.instr_cfg.sip_mask);
        frctweak_arg_parser_add(&parser,"dipd",  parse_ipv4, &instr.instr_cfg.dip_data);
        frctweak_arg_parser_add(&parser,"dipm",  parse_ipv4, &instr.instr_cfg.dip_mask);
        frctweak_arg_parser_add(&parser,"type",  parse_u16, &instr.instr_cfg.type);
        frctweak_arg_parser_add(&parser,"ip_len",  parse_u16, &instr.instr_cfg.ip_len);
        frctweak_arg_parser_add(&parser,"tx_port",  parse_u16, &instr.instr_cfg.tx_port);
        frctweak_arg_parser_add(&parser,"action",  parse_u16, &instr.instr_cfg.action);
        frctweak_arg_parser_add(&parser,"payload",  parse_payload, instr.payload);

        rv = frctweak_arg_parsing(&parser, argc, argv);

        frctweak_arg_parser_free(&parser);

        for (i = 0; i < INSTR_PAYLOAD_SZ; i++)
        {
            //printf("payload[%lld] = 0x%x\n", i, instr.payload[i]);
        }

        if (FRE_SUCCESS != rv)
        {
            frctweak_cmd_ipc_usage();
            return FRE_PARAM;
        }

        rv = frcapi_ipc_instr_set(&instr.instr_cfg);

        if (FRE_SUCCESS == rv)
        {
            printf("IPC instr set success.\n");
        }
        else
        {
            printf("Error: IPC instr set fail!(rv=%d)\n", rv);
            return rv;
        }
        n = IPC_IP_LEN_MAX / INSTR_PAYLOAD_SZ;
        printf("n = %d\n", n);
        for (i = 0; i < n; i++)
        {
            memcpy(payload_set.data, &instr.payload[i * INSTR_PAYLOAD_SZ], INSTR_PAYLOAD_SZ);
            payload_set.index  = i;
            rv = frcapi_ipc_instr_payload_set(&payload_set);
            if (FRE_SUCCESS != rv)
            {
                printf("Error: IPC instr set fail!(rv=%d)\n", rv);
                return rv;
            }
        }
    }

    return rv;
}


int frctweak_cmd_ipc_clear()
{
    frctweak_ipc_misc_clear();
    frctweak_ipc_cur_clear();
    frctweak_ipc_exp_clear();
    frctweak_ipc_instr_clear();
    return 0;
}

int frctweak_cmd_ipc(int argc, char **argv)
{
    int rv = 0;
    if (argc < 2)
    {
       frctweak_cmd_ipc_usage();
       return FRE_SUCCESS;
    }
   if (!strcmp("misc", argv[1]))
   {
       rv = frctweak_cmd_ipc_misc(--argc, ++argv);
   }
   else if (!strcmp("instr", argv[1]))
   {
       rv = frctweak_cmd_ipc_instr(--argc, ++argv);
   }
   else if (!strcmp("exp", argv[1]))
   {
       rv = frctweak_cmd_ipc_exp(--argc, ++argv);
   }
   else if (!strcmp("cur", argv[1]))
   {
       rv = frctweak_cmd_ipc_cur(--argc, ++argv);
   }
   else if (!strcmp("vlan_hash", argv[1]))
   {
       rv = frctweak_cmd_ipc_vlan_hash(--argc, ++argv);
   }
   else if (!strcmp("clear", argv[1]))
   {
       rv = frctweak_cmd_ipc_clear();
   }
   else
   {
       frctweak_cmd_ipc_usage();
       return FRE_SUCCESS;
   }

   return rv;
}

int frctweak_ipc_cmd_init(frctweak_cmd_t *cmd)
{
    frctweak_cmd_register(cmd, "ipc", "FS9000 IPC setting", frctweak_cmd_ipc, frctweak_cmd_ipc_usage);
    return 0;
}

#endif

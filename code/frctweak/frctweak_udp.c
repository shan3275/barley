#include <stdlib.h>
#include <string.h>

#include "frctweak.h"
#include "frc_pack.h"
#include "frc_dma.h"
#include "cvmx-swap.h"
#include "frc_api.h"
#include "frc_debug.h"

#if FRC_CONFIG_UDP

typedef enum{
    FRCTWEAK_UDP_STATUS_CMD,
    FRCTWEAK_UDP_ENABLE_CMD,
    FRCTWEAK_UDP_DISABLE_CMD,
}frctweak_udp_t;

void frctweak_cmd_udp_usage()
{
    printf("Usage: %s udp SUBCOMMAND OPTION\n",program);
    printf("       %s udp status\n",program);
    printf("       %s udp enable\n",program);
    printf("       %s udp disable\n",program);
    printf("\n  SUBCOMMAND:\n");
    printf("    status          -- Get udp module status:(enable|disable).\n");
    printf("    enable          -- Enable udp module.\n");
    printf("    disable         -- Disable udp module.\n");
}


int frctweak_cmd_udp(int argc, char **argv)
{
    int rv;
    int op;
    uint8_t enable;


    if (argc < 2)
    {
        frctweak_cmd_udp_usage();
        return FRE_SUCCESS;
    }

    if (!(strcmp("status", argv[1])))
    {
        op = FRCTWEAK_UDP_STATUS_CMD;
    }

    else if (!strcmp("enable", argv[1]))
    {
        op = FRCTWEAK_UDP_ENABLE_CMD;
    }

    else if (!strcmp("disable", argv[1]))
    {
        op = FRCTWEAK_UDP_DISABLE_CMD;
    }
    else
    {
        frctweak_cmd_udp_usage();
        return 0;
    }

    switch (op)
    {
    case FRCTWEAK_UDP_STATUS_CMD:
        rv = frcapi_udp_status_get(&enable);
        printf("UDP function status: %s\n", enable?"enable":"disable");
        break;
    case FRCTWEAK_UDP_ENABLE_CMD:
    case FRCTWEAK_UDP_DISABLE_CMD:
        if (parse_bool(argv[1], &enable))
        {
            return FRE_PARAM;
        }
        //udp_enable = enable;
        rv = frcapi_udp_enable(enable);
        break;
    default:
        return FRE_PARAM;
    }

    return rv;

}

int frctweak_udp_cmd_init(frctweak_cmd_t *cmd)
{
    frctweak_cmd_register(cmd, "udp", "Get or set for udp module", frctweak_cmd_udp, frctweak_cmd_udp_usage);

    return 0;

}
#endif /* end of FRC_CONFIG_UDP*/

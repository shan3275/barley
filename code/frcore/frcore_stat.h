#include "frc_types.h"
#include "frcore_config.h"
#include "frc_config.h"

#define FRCORE_STAT_FAU_OFFSET        0x100

#define FRCORE_STAT_REG(_idx)   \
    (cvmx_fau_reg_64_t)(CVMX_FAU_REG_AVAIL_BASE + FRCORE_STAT_FAU_OFFSET + (_idx) * sizeof(uint64_t))

#define FRCORE_STAT_VAL(_idx) \
    (uint64_t) cvmx_fau_fetch_and_add64(FRCORE_STAT_REG(_idx), 0)

void frcore_stat_add64(int cnt, int64_t value);
int64_t frcore_stat_fetch_and_add64(int cnt, uint64_t value);

#define FRCORE_STAT_ADD(_cnt, _val) \
    frcore_stat_add64(_cnt, _val)

#define FRCORE_STAT_INC(_cnt) frcore_stat_add64((_cnt),  1)

#define FRCORE_STAT_FETCH_AND_ADD(_cnt, _val) \
    frcore_stat_fetch_and_add64(_cnt, _val)

int frcore_stat_init();
void frcore_stat_dump();
void frcore_print_pool_count_stats(void);
void frcore_core_print_pow(void);
void frcore_core_print_pip_pko_stat(void);

#if FRCORE_CFG_DUMP_CYCLE

#define FRCORE_CYCLE_REC_NUM   100

typedef struct {
    char func[64];
    int line;
    uint64_t cycle;
} frcore_cycle_step_t;

typedef struct {
    int idx;
    uint64_t start_cycle;
    uint64_t last_cycle;
    frcore_cycle_step_t steps[FRCORE_CYCLE_REC_NUM];
} frcore_cycle_rec_t;

extern frcore_cycle_rec_t frcore_cycle_recoder;

#   define FRCORE_CYCLE_RECORDER_INIT() \
    { \
        memset(&frcore_cycle_recoder, 0, sizeof(frcore_cycle_rec_t)); \
        frcore_cycle_recoder.start_cycle = frcore_cycle_recoder.last_cycle = cvmx_get_cycle(); \
    }

#   define FRCORE_CYCLE_RECORDING() \
    { \
        frcore_cycle_recoder.steps[frcore_cycle_recoder.idx].cycle = cvmx_get_cycle() - frcore_cycle_recoder.last_cycle; \
        strcpy(frcore_cycle_recoder.steps[frcore_cycle_recoder.idx].func, __func__); \
        frcore_cycle_recoder.steps[frcore_cycle_recoder.idx].line = __LINE__;\
        frcore_cycle_recoder.idx++; \
        frcore_cycle_recoder.last_cycle = cvmx_get_cycle(); \
    }

#   define FRCORE_CYCLE_RECORD_DUMP() \
    { \
        int i; \
        uint64_t all_cycle = 0; \
        for (i = 0; i < frcore_cycle_recoder.idx; i++) { \
            printf("%32s.%-.4d: take %-8lld cycles.\n", \
                         frcore_cycle_recoder.steps[i].func, frcore_cycle_recoder.steps[i].line, \
                         (unsigned long long) frcore_cycle_recoder.steps[i].cycle); \
            all_cycle += frcore_cycle_recoder.steps[i].cycle; \
        } \
        printf("%d steps takes %lld cycles.\n", frcore_cycle_recoder.idx, \
                     (unsigned long long) all_cycle); \
    }
#else
#   define FRCORE_CYCLE_RECORDER_INIT()
#   define FRCORE_CYCLE_RECORDING()
#   define FRCORE_CYCLE_RECORD_DUMP()
#endif
/* End of file */

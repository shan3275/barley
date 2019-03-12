#include "frcore_alg.h"

#if FRC_CONFIG_VLAN_CHECK
static int ip4b8[8] = {0, 2, 4, 6, 8, 12, 16, 24};
static int ip4b4[4] = {0, 4, 8, 16};
static int ip6b8[8] = {0, 2, 4, 8, 16, 24, 32, 48};
static int ip6b4[4] = {0, 4, 8, 16};

#define SDIP_MAX_NUM 256
/********************************************************
    h4,    l4,   val,   vid
    0,     0,     0,     0
    0,     1,     1,     1
    0,     2,     2,     2
    0,     3,     3,     3
    0,     4,     4,     4
    0,     5,     5,     5
    0,     6,     6,     6
    0,     7,     7,     7
    0,     8,     8,     8
    0,     9,     9,     9
    0,    10,    10,    10
    0,    11,    11,    11
    0,    12,    12,    12
    0,    13,    13,    13
    0,    14,    14,    14
    0,    15,    15,    15
    1,     0,     1,     1
    1,     1,    17,    16
    1,     2,    18,    17
    1,     3,    19,    18
    1,     4,    20,    19
    1,     5,    21,    20
    1,     6,    22,    21
    1,     7,    23,    22
    1,     8,    24,    23
    1,     9,    25,    24
    1,    10,    26,    25
    1,    11,    27,    26
    1,    12,    28,    27
    1,    13,    29,    28
    1,    14,    30,    29
    1,    15,    31,    30
    2,     0,     2,     2
    2,     1,    18,    17
    2,     2,    34,    31
    2,     3,    35,    32
    2,     4,    36,    33
    2,     5,    37,    34
    2,     6,    38,    35
    2,     7,    39,    36
    2,     8,    40,    37
    2,     9,    41,    38
    2,    10,    42,    39
    2,    11,    43,    40
    2,    12,    44,    41
    2,    13,    45,    42
    2,    14,    46,    43
    2,    15,    47,    44
    3,     0,     3,     3
    3,     1,    19,    18
    3,     2,    35,    32
    3,     3,    51,    45
    3,     4,    52,    46
    3,     5,    53,    47
    3,     6,    54,    48
    3,     7,    55,    49
    3,     8,    56,    50
    3,     9,    57,    51
    3,    10,    58,    52
    3,    11,    59,    53
    3,    12,    60,    54
    3,    13,    61,    55
    3,    14,    62,    56
    3,    15,    63,    57
    4,     0,     4,     4
    4,     1,    20,    19
    4,     2,    36,    33
    4,     3,    52,    46
    4,     4,    68,    58
    4,     5,    69,    59
    4,     6,    70,    60
    4,     7,    71,    61
    4,     8,    72,    62
    4,     9,    73,    63
    4,    10,    74,    64
    4,    11,    75,    65
    4,    12,    76,    66
    4,    13,    77,    67
    4,    14,    78,    68
    4,    15,    79,    69
    5,     0,     5,     5
    5,     1,    21,    20
    5,     2,    37,    34
    5,     3,    53,    47
    5,     4,    69,    59
    5,     5,    85,    70
    5,     6,    86,    71
    5,     7,    87,    72
    5,     8,    88,    73
    5,     9,    89,    74
    5,    10,    90,    75
    5,    11,    91,    76
    5,    12,    92,    77
    5,    13,    93,    78
    5,    14,    94,    79
    5,    15,    95,    80
    6,     0,     6,     6
    6,     1,    22,    21
    6,     2,    38,    35
    6,     3,    54,    48
    6,     4,    70,    60
    6,     5,    86,    71
    6,     6,   102,    81
    6,     7,   103,    82
    6,     8,   104,    83
    6,     9,   105,    84
    6,    10,   106,    85
    6,    11,   107,    86
    6,    12,   108,    87
    6,    13,   109,    88
    6,    14,   110,    89
    6,    15,   111,    90
    7,     0,     7,     7
    7,     1,    23,    22
    7,     2,    39,    36
    7,     3,    55,    49
    7,     4,    71,    61
    7,     5,    87,    72
    7,     6,   103,    82
    7,     7,   119,    91
    7,     8,   120,    92
    7,     9,   121,    93
    7,    10,   122,    94
    7,    11,   123,    95
    7,    12,   124,    96
    7,    13,   125,    97
    7,    14,   126,    98
    7,    15,   127,    99
    8,     0,     8,     8
    8,     1,    24,    23
    8,     2,    40,    37
    8,     3,    56,    50
    8,     4,    72,    62
    8,     5,    88,    73
    8,     6,   104,    83
    8,     7,   120,    92
    8,     8,     0,     0
    8,     9,   137,   100
    8,    10,   138,   101
    8,    11,   139,   102
    8,    12,   140,   103
    8,    13,   141,   104
    8,    14,   142,   105
    8,    15,   143,   106
    9,     0,     9,     9
    9,     1,    25,    24
    9,     2,    41,    38
    9,     3,    57,    51
    9,     4,    73,    63
    9,     5,    89,    74
    9,     6,   105,    84
    9,     7,   121,    93
    9,     8,   137,   100
    9,     9,    17,    16
    9,    10,   154,   107
    9,    11,   155,   108
    9,    12,   156,   109
    9,    13,   157,   110
    9,    14,   158,   111
    9,    15,   159,   112
   10,     0,    10,    10
   10,     1,    26,    25
   10,     2,    42,    39
   10,     3,    58,    52
   10,     4,    74,    64
   10,     5,    90,    75
   10,     6,   106,    85
   10,     7,   122,    94
   10,     8,   138,   101
   10,     9,   154,   107
   10,    10,    34,    31
   10,    11,   171,   113
   10,    12,   172,   114
   10,    13,   173,   115
   10,    14,   174,   116
   10,    15,   175,   117
   11,     0,    11,    11
   11,     1,    27,    26
   11,     2,    43,    40
   11,     3,    59,    53
   11,     4,    75,    65
   11,     5,    91,    76
   11,     6,   107,    86
   11,     7,   123,    95
   11,     8,   139,   102
   11,     9,   155,   108
   11,    10,   171,   113
   11,    11,    51,    45
   11,    12,   188,   118
   11,    13,   189,   119
   11,    14,   190,   120
   11,    15,   191,   121
   12,     0,    12,    12
   12,     1,    28,    27
   12,     2,    44,    41
   12,     3,    60,    54
   12,     4,    76,    66
   12,     5,    92,    77
   12,     6,   108,    87
   12,     7,   124,    96
   12,     8,   140,   103
   12,     9,   156,   109
   12,    10,   172,   114
   12,    11,   188,   118
   12,    12,    68,    58
   12,    13,   205,   122
   12,    14,   206,   123
   12,    15,   207,   124
   13,     0,    13,    13
   13,     1,    29,    28
   13,     2,    45,    42
   13,     3,    61,    55
   13,     4,    77,    67
   13,     5,    93,    78
   13,     6,   109,    88
   13,     7,   125,    97
   13,     8,   141,   104
   13,     9,   157,   110
   13,    10,   173,   115
   13,    11,   189,   119
   13,    12,   205,   122
   13,    13,    85,    70
   13,    14,   222,   125
   13,    15,   223,   126
   14,     0,    14,    14
   14,     1,    30,    29
   14,     2,    46,    43
   14,     3,    62,    56
   14,     4,    78,    68
   14,     5,    94,    79
   14,     6,   110,    89
   14,     7,   126,    98
   14,     8,   142,   105
   14,     9,   158,   111
   14,    10,   174,   116
   14,    11,   190,   120
   14,    12,   206,   123
   14,    13,   222,   125
   14,    14,   102,    81
   14,    15,   239,   127
   15,     0,    15,    15
   15,     1,    31,    30
   15,     2,    47,    44
   15,     3,    63,    57
   15,     4,    79,    69
   15,     5,    95,    80
   15,     6,   111,    90
   15,     7,   127,    99
   15,     8,   143,   106
   15,     9,   159,   112
   15,    10,   175,   117
   15,    11,   191,   121
   15,    12,   207,   124
   15,    13,   223,   126
   15,    14,   239,   127
   15,    15,   119,    91
 * ******************************************************/
uint16_t sdip_vlan_index[SDIP_MAX_NUM] = {
/*
 * h4,    l4,   val,   vid   */
/*    0,     0,     0, */    0, 
/*    0,     1,     1, */    1,
/*    0,     2,     2, */    2,
/*    0,     3,     3, */    3,
/*    0,     4,     4, */    4,
/*    0,     5,     5, */    5,
/*    0,     6,     6, */    6,
/*    0,     7,     7, */    7,
/*    0,     8,     8, */    8,
/*    0,     9,     9, */    9,
/*    0,    10,    10, */   10,
/*    0,    11,    11, */   11,
/*    0,    12,    12, */   12,
/*    0,    13,    13, */   13,
/*    0,    14,    14, */   14,
/*    0,    15,    15, */   15,
/*    1,     0,     1, */    1,
/*    1,     1,    17, */   16,
/*    1,     2,    18, */   17,
/*    1,     3,    19, */   18,
/*    1,     4,    20, */   19,
/*    1,     5,    21, */   20,
/*    1,     6,    22, */   21,
/*    1,     7,    23, */   22,
/*    1,     8,    24, */   23,
/*    1,     9,    25, */   24,
/*    1,    10,    26, */   25,
/*    1,    11,    27, */   26,
/*    1,    12,    28, */   27,
/*    1,    13,    29, */   28,
/*    1,    14,    30, */   29,
/*    1,    15,    31, */   30,
/*    2,     0,     2, */    2,
/*    2,     1,    18, */   17,
/*    2,     2,    34, */   31,
/*    2,     3,    35, */   32,
/*    2,     4,    36, */   33,
/*    2,     5,    37, */   34,
/*    2,     6,    38, */   35,
/*    2,     7,    39, */   36,
/*    2,     8,    40, */   37,
/*    2,     9,    41, */   38,
/*    2,    10,    42, */   39,
/*    2,    11,    43, */   40,
/*    2,    12,    44, */   41,
/*    2,    13,    45, */   42,
/*    2,    14,    46, */   43,
/*    2,    15,    47, */   44,
/*    3,     0,     3, */    3,
/*    3,     1,    19, */   18,
/*    3,     2,    35, */   32,
/*    3,     3,    51, */   45,
/*    3,     4,    52, */   46,
/*    3,     5,    53, */   47,
/*    3,     6,    54, */   48,
/*    3,     7,    55, */   49,
/*    3,     8,    56, */   50,
/*    3,     9,    57, */   51,
/*    3,    10,    58, */   52,
/*    3,    11,    59, */   53,
/*    3,    12,    60, */   54,
/*    3,    13,    61, */   55,
/*    3,    14,    62, */   56,
/*    3,    15,    63, */   57,
/*    4,     0,     4, */    4,
/*    4,     1,    20, */   19,
/*    4,     2,    36, */   33,
/*    4,     3,    52, */   46,
/*    4,     4,    68, */   58,
/*    4,     5,    69, */   59,
/*    4,     6,    70, */   60,
/*    4,     7,    71, */   61,
/*    4,     8,    72, */   62,
/*    4,     9,    73, */   63,
/*    4,    10,    74, */   64,
/*    4,    11,    75, */   65,
/*    4,    12,    76, */   66,
/*    4,    13,    77, */   67,
/*    4,    14,    78, */   68,
/*    4,    15,    79, */   69,
/*    5,     0,     5, */    5,
/*    5,     1,    21, */   20,
/*    5,     2,    37, */   34,
/*    5,     3,    53, */   47,
/*    5,     4,    69, */   59,
/*    5,     5,    85, */   70,
/*    5,     6,    86, */   71,
/*    5,     7,    87, */   72,
/*    5,     8,    88, */   73,
/*    5,     9,    89, */   74,
/*    5,    10,    90, */   75,
/*    5,    11,    91, */   76,
/*    5,    12,    92, */   77,
/*    5,    13,    93, */   78,
/*    5,    14,    94, */   79,
/*    5,    15,    95, */   80,
/*    6,     0,     6, */    6,
/*    6,     1,    22, */   21,
/*    6,     2,    38, */   35,
/*    6,     3,    54, */   48,
/*    6,     4,    70, */   60,
/*    6,     5,    86, */   71,
/*    6,     6,   102, */   81,
/*    6,     7,   103, */   82,
/*    6,     8,   104, */   83,
/*    6,     9,   105, */   84,
/*    6,    10,   106, */   85,
/*    6,    11,   107, */   86,
/*    6,    12,   108, */   87,
/*    6,    13,   109, */   88,
/*    6,    14,   110, */   89,
/*    6,    15,   111, */   90,
/*    7,     0,     7, */    7,
/*    7,     1,    23, */   22,
/*    7,     2,    39, */   36,
/*    7,     3,    55, */   49,
/*    7,     4,    71, */   61,
/*    7,     5,    87, */   72,
/*    7,     6,   103, */   82,
/*    7,     7,   119, */   91,
/*    7,     8,   120, */   92,
/*    7,     9,   121, */   93,
/*    7,    10,   122, */   94,
/*    7,    11,   123, */   95,
/*    7,    12,   124, */   96,
/*    7,    13,   125, */   97,
/*    7,    14,   126, */   98,
/*    7,    15,   127, */   99,
/*    8,     0,     8, */    8,
/*    8,     1,    24, */   23,
/*    8,     2,    40, */   37,
/*    8,     3,    56, */   50,
/*    8,     4,    72, */   62,
/*    8,     5,    88, */   73,
/*    8,     6,   104, */   83,
/*    8,     7,   120, */   92,
/*    8,     8,     0, */    0,
/*    8,     9,   137, */  100,
/*    8,    10,   138, */  101,
/*    8,    11,   139, */  102,
/*    8,    12,   140, */  103,
/*    8,    13,   141, */  104,
/*    8,    14,   142, */  105,
/*    8,    15,   143, */  106,
/*    9,     0,     9, */    9,
/*    9,     1,    25, */   24,
/*    9,     2,    41, */   38,
/*    9,     3,    57, */   51,
/*    9,     4,    73, */   63,
/*    9,     5,    89, */   74,
/*    9,     6,   105, */   84,
/*    9,     7,   121, */   93,
/*    9,     8,   137, */  100,
/*    9,     9,    17, */   16,
/*    9,    10,   154, */  107,
/*    9,    11,   155, */  108,
/*    9,    12,   156, */  109,
/*    9,    13,   157, */  110,
/*    9,    14,   158, */  111,
/*    9,    15,   159, */  112,
/*   10,     0,    10, */   10,
/*   10,     1,    26, */   25,
/*   10,     2,    42, */   39,
/*   10,     3,    58, */   52,
/*   10,     4,    74, */   64,
/*   10,     5,    90, */   75,
/*   10,     6,   106, */   85,
/*   10,     7,   122, */   94,
/*   10,     8,   138, */  101,
/*   10,     9,   154, */  107,
/*   10,    10,    34, */   31,
/*   10,    11,   171, */  113,
/*   10,    12,   172, */  114,
/*   10,    13,   173, */  115,
/*   10,    14,   174, */  116,
/*   10,    15,   175, */  117,
/*   11,     0,    11, */   11,
/*   11,     1,    27, */   26,
/*   11,     2,    43, */   40,
/*   11,     3,    59, */   53,
/*   11,     4,    75, */   65,
/*   11,     5,    91, */   76,
/*   11,     6,   107, */   86,
/*   11,     7,   123, */   95,
/*   11,     8,   139, */  102,
/*   11,     9,   155, */  108,
/*   11,    10,   171, */  113,
/*   11,    11,    51, */   45,
/*   11,    12,   188, */  118,
/*   11,    13,   189, */  119,
/*   11,    14,   190, */  120,
/*   11,    15,   191, */  121,
/*   12,     0,    12, */   12,
/*   12,     1,    28, */   27,
/*   12,     2,    44, */   41,
/*   12,     3,    60, */   54,
/*   12,     4,    76, */   66,
/*   12,     5,    92, */   77,
/*   12,     6,   108, */   87,
/*   12,     7,   124, */   96,
/*   12,     8,   140, */  103,
/*   12,     9,   156, */  109,
/*   12,    10,   172, */  114,
/*   12,    11,   188, */  118,
/*   12,    12,    68, */   58,
/*   12,    13,   205, */  122,
/*   12,    14,   206, */  123,
/*   12,    15,   207, */  124,
/*   13,     0,    13, */   13,
/*   13,     1,    29, */   28,
/*   13,     2,    45, */   42,
/*   13,     3,    61, */   55,
/*   13,     4,    77, */   67,
/*   13,     5,    93, */   78,
/*   13,     6,   109, */   88,
/*   13,     7,   125, */   97,
/*   13,     8,   141, */  104,
/*   13,     9,   157, */  110,
/*   13,    10,   173, */  115,
/*   13,    11,   189, */  119,
/*   13,    12,   205, */  122,
/*   13,    13,    85, */   70,
/*   13,    14,   222, */  125,
/*   13,    15,   223, */  126,
/*   14,     0,    14, */   14,
/*   14,     1,    30, */   29,
/*   14,     2,    46, */   43,
/*   14,     3,    62, */   56,
/*   14,     4,    78, */   68,
/*   14,     5,    94, */   79,
/*   14,     6,   110, */   89,
/*   14,     7,   126, */   98,
/*   14,     8,   142, */  105,
/*   14,     9,   158, */  111,
/*   14,    10,   174, */  116,
/*   14,    11,   190, */  120,
/*   14,    12,   206, */  123,
/*   14,    13,   222, */  125,
/*   14,    14,   102, */   81,
/*   14,    15,   239, */  127,
/*   15,     0,    15, */   15,
/*   15,     1,    31, */   30,
/*   15,     2,    47, */   44,
/*   15,     3,    63, */   57,
/*   15,     4,    79, */   69,
/*   15,     5,    95, */   80,
/*   15,     6,   111, */   90,
/*   15,     7,   127, */   99,
/*   15,     8,   143, */  106,
/*   15,     9,   159, */  112,
/*   15,    10,   175, */  117,
/*   15,    11,   191, */  121,
/*   15,    12,   207, */  124,
/*   15,    13,   223, */  126,
/*   15,    14,   239, */  127,
/*   15,    15,   119, */   91,
};


/*
int fal_ip4data_get(uint8_t idx, bcm_ip_t *sip_data,
                bcm_ip_t *dip_data, HASH_EM dvlan)
{
        uint32_t i;

        if (NULL == sip_data || NULL == dip_data || FS_HSEL_NULL == dvlan)
        {
                return(-1);
        }

        if (FS_HSEL_SIP == dvlan)
        {
                *sip_data = 0;
                for (i = 0; i < 8; i++)
                {
                        BIT_SET(*sip_data, ip4b8[i], BIT_GET(idx, i));
                }
        }
        else if (FS_HSEL_DIP == dvlan)
        {
                *dip_data = 0;
                for (i = 0; i < 8; i++)
                {
                        BIT_SET(*dip_data, ip4b8[i], BIT_GET(idx, i));
                }
        }
        else if (FS_HSEL_SDIP == dvlan)
        {
                *sip_data = 0;
                *dip_data = 0;
                for (i = 0; i < 4; i++)
                {
                        BIT_SET(*sip_data, ip4b4[i], BIT_GET(idx, i));
                        BIT_SET(*dip_data, ip4b4[i], BIT_GET(idx, i));
                }
        }
        else
        {
                return(-1);
        }

        return(0);
}
*/
/*
int fal_ip4mask_get(bcm_ip_t *sip_mask,
                bcm_ip_t * dip_mask, HASH_EM dvlan)
{
        uint32_t i;

        if (NULL == sip_mask || NULL == dip_mask || FS_HSEL_NULL == dvlan)
        {
                return(-1);
        }

        if (FS_HSEL_SIP == dvlan)
        {
                *sip_mask = 0;
                for (i = 0; i < 8; i++)
                {
                        BIT_SET(*sip_mask, ip4b8[i], 1);
                }
        }
        else if (FS_HSEL_DIP == dvlan)
        {
                *dip_mask = 0;
                for (i = 0; i < 8; i++)
                {
                        BIT_SET(*dip_mask, ip4b8[i], 1);
                }
        }
        else if (FS_HSEL_SDIP == dvlan)
        {
                *sip_mask = 0;
                *dip_mask = 0;
                for (i = 0; i < 4; i++)
                {
                        BIT_SET(*sip_mask, ip4b4[i], 1);
                        BIT_SET(*dip_mask, ip4b4[i], 1);
                }
        }
        else
        {
                return(-1);
        }

        return(0);
}

int fal_ip6data_get(uint8_t idx, bcm_ip6_t *sip_data,
                bcm_ip6_t *dip_data, HASH_EM dvlan)
{
        uint32_t i, *sip32 = NULL, *dip32 = NULL;

        if (FS_HSEL_NULL == dvlan)
        {
                return(-1);
        }

        if (FS_HSEL_SIP == dvlan)
        {
                memset(sip_data, 0, sizeof(bcm_ip6_t));
                sip32 = (uint32_t*)sip_data;
                for (i = 0; i < 8; i++)
                {
                        BIT_SET(sip32[3 - ip6b8[i] / 32], ip6b8[i] % 32, BIT_GET(idx, i));
                }
        }
        else if (FS_HSEL_DIP == dvlan)
        {
                memset(dip_data, 0, sizeof(bcm_ip6_t));
                dip32 = (uint32_t*)dip_data;
                for (i = 0; i < 8; i++)
                {
                        BIT_SET(dip32[3 - ip6b8[i] / 32], ip6b8[i] % 32, BIT_GET(idx, i));
                }
        }
        else if (FS_HSEL_SDIP == dvlan)
        {
                memset(sip_data, 0, sizeof(bcm_ip6_t));
                memset(dip_data, 0, sizeof(bcm_ip6_t));
                sip32 = (uint32_t*)sip_data;
                dip32 = (uint32_t*)dip_data;
                for (i = 0; i < 4; i++)
                {
                        BIT_SET(sip32[3 - ip6b4[i] / 32], ip6b4[i]%32, BIT_GET(idx, i));
                }
                for (i = 0; i < 4; i++)
                {
                        BIT_SET(dip32[3 - ip6b4[i] / 32], ip6b4[i]%32, BIT_GET(idx, i));
                }
        }
        else
        {
                return(-1);
        }

        return(0);
}
*/
/*
int fal_ip6mask_get(bcm_ip6_t *sip_mask,
                bcm_ip6_t *dip_mask, HASH_EM dvlan)
{
        uint32_t i, *sip32 = NULL, *dip32 = NULL;

        if (NULL == sip_mask || NULL == dip_mask || FS_HSEL_NULL == dvlan)
        {
                return(-1);
        }

        if (FS_HSEL_SIP == dvlan)
        {
                memset(sip_mask, 0, sizeof(bcm_ip6_t));
                sip32 = (uint32_t*)sip_mask;
                for (i = 0; i < 8; i++)
                {
                        BIT_SET(sip32[3 - ip6b8[i] / 32], ip6b8[i] % 32, 1);
                }
        }
        else if (FS_HSEL_DIP == dvlan)
        {
                memset(dip_mask, 0, sizeof(bcm_ip6_t));
                dip32 = (uint32_t*)dip_mask;
                for (i = 0; i < 8; i++)
                {
                        BIT_SET(dip32[3 - ip6b8[i] / 32], ip6b8[i] % 32, 1);
                }
        }
        else if (FS_HSEL_SDIP == dvlan)
        {
                memset(sip_mask, 0, sizeof(bcm_ip6_t));
                memset(dip_mask, 0, sizeof(bcm_ip6_t));
                sip32 = (uint32_t*)sip_mask;
                dip32 = (uint32_t*)dip_mask;
                for (i = 0; i < 4; i++)
                {
                        BIT_SET(sip32[3 - ip6b4[i] / 32], ip6b4[i] % 32, 1);
                }
                for (i = 0; i < 4; i++)
                {
                        BIT_SET(dip32[3 - ip6b4[i] / 32], ip6b4[i] % 32, 1);
                }
        }
        else
        {
                return(-1);
        }

        return(0);
}
*/
uint32_t ip6_to_eid(ip6_pkt_t *ip6_pkt, HASH_EM hash)
{
        int eid = 0, i;
        uint32_t *ip32 = NULL;
        bcm_ip6_t sip_data = {}, dip_data = {};
        //bcm_ip6_t sip_mask = {}, dip_mask = {};
        memcpy(sip_data, ip6_pkt->sip6_data, sizeof(sip_data));
        memcpy(dip_data, ip6_pkt->dip6_data, sizeof(dip_data));

        //fal_ip6mask_get(&sip_mask, &dip_mask, hash);

        //ip32 = (uint32_t *)sip_mask;
        //for(j=0;j<4;j++)
        //{
        //      printf("ip32[%d]=%08x\n",3-j,ip32[3-j]);
        //      for(i=0;i<32;i++)
        //              printf("%d%s",BIT_GET(ip32[3- j], 31 - i), i==31?"\n":"");
        //}
        //printf("\n");
        switch (hash)
        {
                case FS_HSEL_NULL:
                        eid = -1;
                        break;

                case FS_HSEL_SIP:
                        {
                                ip32 = (uint32_t*)sip_data;
                                /*
                                for(j=0;j<4;j++)
                                {
                                        printf("ip32[%d]=%08x\n",3-j,ip32[3-j]);
                                        for(i=0;i<32;i++)
                                                printf("%d%s",BIT_GET(ip32[3- j], 31 - i), i==31?"\n":"");
                                }
                                printf("\n");*/

                                for (i = 0; i < 7; i++)
                                {
                                        BIT_SET(eid, i, BIT_GET(ip32[3 - ip6b8[i] / 32], ip6b8[i]%32));
                                }
                                //printf("eid=%d\n",eid);
                        }
                        break;

                case FS_HSEL_DIP:
                        {
                                ip32 = (uint32_t*)dip_data;
                                for (i = 0; i < 7; i++)
                                {
                                        BIT_SET(eid, i, BIT_GET(ip32[3 - ip6b8[i] / 32], ip6b8[i]%32));
                                }
                        }
                        break;

                case FS_HSEL_SDIP:
                        {
                                ip32 = (uint32_t*)sip_data;
                                for (i = 0; i < 4; i++)
                                {
                                        BIT_SET(eid, i, BIT_GET(ip32[3 - ip6b4[i] / 32], ip6b4[i]%32));
                                }
                                eid <<= 4;
                                ip32 = (uint32_t*)dip_data;
                                for (i = 0; i < 4; i++)
                                {
                                        BIT_SET(eid, i, BIT_GET(ip32[3 - ip6b4[i] / 32], ip6b4[i]%32));
                                }
                                eid = sdip_vlan_index[eid % SDIP_MAX_NUM];
                        }
                        break;

                default:
                        break;
        }

        return eid;
}


#if FRC_CONFIG_VLAN_IV
extern CVMX_SHARED uint32_t hash_sip4_mask;
extern CVMX_SHARED uint32_t hash_dip4_mask;
extern CVMX_SHARED bcm_ip6_t hash_sip6_mask;
extern CVMX_SHARED bcm_ip6_t hash_dip6_mask;

uint32_t ip6_to_eid_new(ip6_pkt_t *ip6_pkt, HASH_EM hash)
{
        int eid = 0, i;
        bcm_ip_t sw_sip = 0, sw_dip = 0;
        uint32_t sip32[4] = {}, dip32[4] = {};
        bcm_ip6_t sip_data = {}, dip_data = {};
        char *p = NULL , *q = NULL;
        //bcm_ip6_t sip_mask = {}, dip_mask = {};
        memcpy(sip_data, ip6_pkt->sip6_data, sizeof(sip_data));
        memcpy(dip_data, ip6_pkt->dip6_data, sizeof(dip_data));
        for (i = 0; i < 16; i++)
        {
            sip_data[i] &= hash_sip6_mask[i];
            dip_data[i] &= hash_dip6_mask[i];
        }

        p = (char *)sip_data;
        q = (char *)dip_data;

        for (i = 0; i < 4; i++)
        {
            UNPACK_U32(p, sip32[i]);
            UNPACK_U32(q, dip32[i]);
            if (i)
            {
                sip32[i] ^= sip32[i-1];
                dip32[i] ^= dip32[i-1];
            }
        }
        sw_sip = sip32[i-1];
        sw_dip = dip32[i-1];
        

        switch (hash)
        {
                case FS_HSEL_NULL:
                        eid = -1;
                        break;

                case FS_HSEL_SIP:
                        {
                                eid = sw_sip;
                        }
                        break;

                case FS_HSEL_DIP:
                        {
                                eid = sw_dip;
                        }
                        break;

                case FS_HSEL_SDIP:
                        {
                                eid = sw_sip + sw_dip;
                        }
                        break;

                default:
                        break;
        }

        return eid;
}


uint32_t ip4_to_eid_new(ip4_pkt_t *ip4_pkt, HASH_EM hash)
{
        int eid = 0;
        bcm_ip_t sip_data = ip4_pkt->sip_data;
        bcm_ip_t dip_data = ip4_pkt->dip_data;

        //printf("smask = 0x%x, sdata = 0x%x\n", hash_sip4_mask, sip_data);
        //printf("dmask = 0x%x, ddata = 0x%x\n", hash_dip4_mask, dip_data);
        //printf("hash = %d\n", hash);   
        sip_data &= hash_sip4_mask;
        dip_data &= hash_dip4_mask;
        switch (hash)
        {
                case FS_HSEL_NULL:
                        eid = -1;
                        break;

                case FS_HSEL_SIP:
                        {
                                eid = sip_data;
                        }
                        break;

                case FS_HSEL_DIP:
                        {
                                eid = dip_data;
                        }
                        break;

                case FS_HSEL_SDIP:
                        {
                                eid = sip_data + dip_data;
                        }
                        break;

                default:
                        break;
        }

        return eid;
}
#endif

uint32_t ip4_to_eid(ip4_pkt_t *ip4_pkt, HASH_EM hash)
{
        int eid = 0, i;
        bcm_ip_t sip_data = ip4_pkt->sip_data;
        bcm_ip_t dip_data = ip4_pkt->dip_data;

        switch (hash)
        {
                case FS_HSEL_NULL:
                        eid = -1;
                        break;

                case FS_HSEL_SIP:
                        {
                                for (i = 0; i < 7; i++)
                                {
                                        BIT_SET(eid, i, BIT_GET(sip_data, ip4b8[i]));
                                }
                        }
                        break;

                case FS_HSEL_DIP:
                        {
                                for (i = 0; i < 7; i++)
                                {
                                        BIT_SET(eid, i, BIT_GET(dip_data, ip4b8[i]));
                                }
                        }
                        break;

                case FS_HSEL_SDIP:
                        {
                                for (i = 0; i < 4; i++)
                                {
                                        BIT_SET(eid, i, BIT_GET(sip_data, ip4b4[i]));
                                }
                                eid <<= 4;
                                for (i = 0; i < 4; i++)
                                {
                                        BIT_SET(eid, i, BIT_GET(dip_data, ip4b4[i]));
                                }
                                eid = sdip_vlan_index[eid % SDIP_MAX_NUM];
                        }
                        break;

                default:
                        break;
        }

        return eid;
}



int eid_to_vlan_new(uint32_t eid, IP_TYPE IPTP, uint32_t vlan_n, uint32_t vlan_0, HASH_EM hash)
{
    uint32_t vlanid = -1;
    //printf("vlan_n = %d, vlan_0 = %d\n", vlan_n, vlan_0);
    vlanid = eid % vlan_n + vlan_0;
    return vlanid;
}


int eid_to_vlan(uint32_t eid, IP_TYPE IPTP, uint32_t vlan_n, uint32_t vlan_0, HASH_EM hash)
{
        uint32_t vlanid = -1; // not valid vlanid
        if(FS_HSEL_SDIP == hash)
	{
	  vlanid = eid % vlan_n + vlan_0;
	  return vlanid;
	}

        if (IPV4 == IPTP)
        {
                vlanid = (eid + E0) % vlan_n + vlan_0;
        }
        else if (IPV6 == IPTP)
        {
                vlanid = (eid + E0 + EMAX4) % vlan_n + vlan_0;
        }

        return vlanid;
}

uint32_t ip4_calc_vlan(ip4_pkt_t *ip4_pkt, HASH_EM hash, uint32_t vlan_n, uint32_t vlan_0)
{
        int eid, vlanid;
#if FRC_CONFIG_VLAN_IV
        eid = ip4_to_eid_new(ip4_pkt, hash);
#else
        eid = ip4_to_eid(ip4_pkt, hash);
#endif
        if (-1 == eid)
        {
                return -1;
        }
#if FRC_CONFIG_VLAN_IV
        vlanid = eid_to_vlan_new(eid, IPV4, vlan_n, vlan_0, hash);
#else
        vlanid = eid_to_vlan(eid, IPV4, vlan_n, vlan_0, hash);
#endif
        if (vlanid > 4095)
        {
                return -1;
        }

        return vlanid;
}

uint32_t ip6_calc_vlan(ip6_pkt_t *ip6_pkt, HASH_EM hash, uint32_t vlan_n, uint32_t vlan_0)
{
        int eid, vlanid;

#if FRC_CONFIG_VLAN_IV
        eid = ip6_to_eid_new(ip6_pkt, hash);
#else
        eid = ip6_to_eid(ip6_pkt, hash);
#endif
        if (-1 == eid)
        {
                return -1;
        }
#if FRC_CONFIG_VLAN_IV
        vlanid = eid_to_vlan_new(eid, IPV6, vlan_n, vlan_0, hash);
#else
        vlanid = eid_to_vlan(eid, IPV6, vlan_n, vlan_0, hash);
#endif

        if (vlanid > 4095)
        {
                return -1;
        }

        return vlanid;
}
#endif

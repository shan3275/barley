#ifndef __FRC_CONFIG_H__
#define __FRC_CONFIG_H__

#define FRC_CONFIG_FR                 1
#define FRC_CONFIG_NIC                1

#define FRC_CONFIG_TCP                0
#define FRC_CONFIG_QUEUE_TEST         0
#define FRC_CONFIG_DMA_TEST           0
#define FRC_CONFIG_UDP                0
#define FRC_CONFIG_UDP_CLOSE_SUBMIT_DATA  0
#define FRC_CONFIG_NIC_GRP            0
#define FRC_CONFIG_LOCK_NET_XMIT      1
#define FRC_CONFIG_SIMPLE_PACKAGE     0
#define FRC_CONFIG_SIMPLE_PACKET_TEST 0
#define FRC_CONFIG_RULE               0
#define FRC_CONFIG_SSN_CHAN           0
#define FRC_CONFIG_SSN_CHAN_TEST      0
#define FRC_CONFIG_SSN                0
#define FRC_CONFIG_AGE                0
#define FRC_CONFIG_SSN_WQE_TEST       0
#define FRC_CONFIG_SSN_SIMPLE_PACKET_TEST 0 
#define FRC_CONFIG_SSN_ATOMIC             0
#define FRC_CONFIG_SSN_AVAIL_BUFF_GET     0  /* if 1 every time get 1000 buff addr else get 1 buff addr */
#define FRC_CONFIG_SSN_CLOSE_SUBMIT_DATA  0
#define FRC_CONFIG_SSN_CHAN_TEST_ONE_BLOCK_ONE_PAYLOD 0  /* if 1 every block 1 payload */

#define FRC_CONFIG_TWO_TUPLE 0  /* two tuple rule macro switch */
#define FRC_CONFIG_TWO_TUPLE_TSET 0 /* two tuple rule test */

#define FRC_CONFIG_GET_SYSINFO 0
#define FRC_CONFIG_VLAN_CHECK 1 
#define FRC_CONFIG_TIMESTAMP_CHECK 0
#define FRC_CONFIG_MAC_STATISTICS 0
#define FRC_CONFIG_IPC     1
#define FRC_CONFIG_VLAN_IV     1
#endif /* !__FRC_CONFIG_H__ */

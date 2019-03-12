#ifndef __FRCORE_QUEUE_H__
#define __FRCORE_QUEUE_H__

#if FRC_CONFIG_SIMPLE_PACKAGE
int frcore_simple_package_chan_init();
int frcore_forward_simple_pkt_to_fifo(uint32_t type, frc_dma_hdr_t *hdr, frc_dma_pkt_info_t *info, void *payload);
#if FRC_CONFIG_RULE
int frcore_forward_rule_pkt_to_fifo(uint16_t *tagid, frc_dma_hdr_t *hdr, frc_dma_pkt_info_t *info, void *payload);
#endif
#endif

#if FRC_CONFIG_SSN_CHAN
int frcore_ssn_chan_init();
int frcore_forward_ssn_pkt_to_fifo(uint32_t type,uint32_t index, uint64_t dma_type,
                                   frc_dma_hdr_t *hdr, frc_dma_pkt_info_t *info,
                                   void *payload,uint64_t block_addr, uint16_t info_offset,
                                   uint16_t payload_offset, cvmx_wqe_t  *wqe);
int frcore_ssn_get_one_block_addr(uint64_t *buff, uint32_t fifo_id);
#endif

#if FRC_CONFIG_SIMPLE_PACKET_TEST
int frcore_forward_simple_pkt_to_fifo_test(uint32_t type);
int frcore_chan_test_process(cvmx_wqe_t *wqe);
#endif

int frcore_chan_process(cvmx_wqe_t *wqe);
void frcore_print_block_usage(void);
#endif /* __FRCORE_QUEUE_H__ */

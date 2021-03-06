#ifndef __FRCORE_TWOTUPLE_H__
#define __FRCORE_TWOTUPLE_H__
#if FRC_CONFIG_TWO_TUPLE
int frcore_acl_init();
int frcore_acl_process(cvmx_wqe_t *work, uint8_t *eth_ptr, uint8_t *ip_ptr,
                       uint8_t *udp_ptr, uint16_t ip_paylen, uint64_t smac, uint64_t dmac);
#endif
#endif /* !__FRCORE_FR_H__ */

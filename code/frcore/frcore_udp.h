#ifndef __FRCORE_UDP_H__
#define __FRCORE_UDP_H__

#define UDP_HDR_SZ  8

int frcore_udp_process(cvmx_wqe_t *work, uint8_t *eth_ptr, uint8_t *ip_ptr, uint8_t *udp_ptr, uint16_t ip_paylen, uint64_t smac, uint64_t dmac);

int
frcore_udp_init();
#endif /* !__FRCORE_UDP_H__ */

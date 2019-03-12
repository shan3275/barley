#ifndef _FRCORE_TCP_H_
#define _FRCORE_TCP_H_

#define TCP_FLAG(_val, _flag)   (_val & _flag)

#define TCP_FLAG_URG(_val)      TCP_FLAG((_val), 0x20)
#define TCP_FLAG_ACK(_val)      TCP_FLAG((_val), 0x10)
#define TCP_FLAG_PSH(_val)      TCP_FLAG((_val), 0x08)
#define TCP_FLAG_RST(_val)      TCP_FLAG((_val), 0x04)
#define TCP_FLAG_SYN(_val)      TCP_FLAG((_val), 0x02)
#define TCP_FLAG_FIN(_val)      TCP_FLAG((_val), 0x01)

int frcore_tcp_process(cvmx_wqe_t *work, uint8_t *eth_ptr, uint8_t *ip_ptr, uint8_t *tcp_ptr, uint16_t ip_paylen, uint64_t smac, uint64_t dmac);
#if FRC_CONFIG_AGE
int frcore_ssn_age_process(cvmx_wqe_t *work);
#endif
#endif /* !_FRCORE_TCP_H_ */

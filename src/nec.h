#ifndef __NEC_H__
#define __NEC_H__
#include <stdint.h>


void nec_init(void);
void nec_start(void);
void nec_stop(void);
uint8_t nec_read(uint32_t *data);

#endif


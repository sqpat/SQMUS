#ifndef __SQMUSMPU_H_
#define __SQMUSMPU_H_

#include "sqcommon.h"

#define MPU401PORT	0x330
#define DRV_MPU401	0x0004


int8_t MPU401detectHardware(uint16_t port, uint8_t irq, uint8_t dma);
int8_t MPU401initHardware(uint16_t port, uint8_t irq, uint8_t dma);

extern struct driverBlock MPU401driver;


#endif

#ifndef __SQMUSSBM_H_
#define __SQMUSSBM_H_

#include "sqcommon.h"

#define DRV_SBMIDI	0x0005
#define SBMIDIPORT	0x220

int8_t SBMIDIdetectHardware(uint16_t port, uint8_t irq, uint8_t dma);
int8_t SBMIDIinitHardware(uint16_t port, uint8_t irq, uint8_t dma);

extern struct driverBlock SBMIDIdriver;


#endif

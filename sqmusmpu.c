#include "sqcommon.h"
#include "sqmusmpu.h"
#include "sqmusmid.h"
#include <dos.h>
#include <conio.h>

#include <stdio.h>
#include <stdlib.h>






uint16_t MPU401port = MPU401PORT;
uint8_t runningStatus = 0;

/* MPU401 commands: (out: port 331h) */
#define MPU401_SET_UART	(uint8_t)0x3F	// set UART mode
#define MPU401_RESET	(uint8_t)0xFF	// reset MIDI device

// MPU401 status codes: (in: port 331h)
#define MPU401_BUSY	  0x40	// 1:busy, 0:ready to receive a command
#define MPU401_EMPTY  0x80	// 1:empty, 0:buffer is full
#define MPU401_ACK	  0xFE	// command acknowledged


/* write one byte to MPU-401 data port */
int8_t MPU401sendByte(uint8_t value){
    uint16_t timeout = 10000;
    uint16_t statusport = MPU401port + 1;

    /* wait until the device is ready */
    for(;;) {
        uint8_t delay;
        uint8_t status = inp(statusport);	/* read status port */
        if ((status & MPU401_BUSY) == 0){
            break;
        }
        if (status & MPU401_EMPTY){
            continue;
        }
        _enable();
        for (delay = 100; delay; delay--){

        }
        inp(MPU401port);		/* discard incoming data */
        if (--timeout == 0){
            return -1;			/* port is not responding: timeout */
        }
    }

    outp(MPU401port, value);		/* write value to data port */
    return 0;
}

/* write block of bytes to MPU-401 port */
int8_t MPU401sendBlock(uint8_t *block, uint16_t length){
    runningStatus = 0;			/* clear the running status byte */

    _disable();
    while (length--){
	    MPU401sendByte(*block++);
    }
    _enable();
    return 0;
}

/* write one byte to MPU-401 command port */
uint8_t MPU401sendCommand(uint8_t value){
    uint16_t timeout;
    uint16_t statusport = MPU401port + 1;

    runningStatus = 0;			/* clear the running status byte */

    /* wait until the device is ready */
    for(timeout = 0xFFFF;;) {
        if ((inp(statusport) & MPU401_BUSY) == 0){ /* read status port */
            break;
        }
        if (--timeout == 0){
            return -1;			/* port is not responding: timeout */
        }
    }

    outp(statusport, value);		/* write value to command port */

    /* wait for acknowledging the command */
    for(timeout = 0xFFFF;;) {
        if ((inp(statusport) & MPU401_EMPTY) == 0){ /* read status port */
            if (inp(MPU401port) == MPU401_ACK){
               break;
            }
        }
        if (--timeout == 0){
            return -1;			/* port is not responding: timeout */
        }
    }

    return 0;
}

/* reset MPU-401 port */
int8_t MPU401reset(void){
    runningStatus = 0;			/* clear the running status byte */

    if (!MPU401sendCommand(MPU401_RESET)){	/* first trial */
	    return 0;
    }
    return MPU401sendCommand(MPU401_RESET);	/* second trial */
}


/* send MIDI command */
int8_t MPU401sendMIDI(uint8_t command, uint8_t par1, uint8_t par2){
    uint8_t event = command & MIDI_EVENT_MASK;

    if (event == MIDI_NOTE_ON){

    } else if (event == MIDI_NOTE_OFF) {

        /* convert NOTE_OFF to NOTE_ON with zero velocity */
        command = (command & 0x0F) | MIDI_NOTE_ON;
        par2 = 0; /* velocity */
    }

    _disable();
    if (runningStatus != command){
	    MPU401sendByte(runningStatus = command);
    }
    MPU401sendByte(par1);
    if (event != MIDI_PATCH && event != MIDI_CHAN_TOUCH){
	    MPU401sendByte(par2);
    }
    _enable();
    return 0;
}

int8_t MPU401driverParam(uint16_t message, uint16_t param1, void *param2){
    switch (message) {
        case DP_SYSEX:
            MPU401sendBlock((uint8_t *)param2, param1);
            break;
    }
    return 0;
}


int8_t MPU401loadBank(int16_t fd, uint8_t bankNumber){
    return 0;
}



int8_t MPU401detectHardware(uint16_t port, uint8_t irq, uint8_t dma){
    int16_t savedMPU401port = MPU401port;
    int8_t result;

    MPU401port = port;
    result = MPU401reset() + 1;
    MPU401port = savedMPU401port;

    return result;
}

int8_t MPU401initHardware(uint16_t port, uint8_t irq, uint8_t dma){
    MPU401port = port;
    if (MPU401reset()){
	    return -1;
    }
    return MPU401sendCommand(MPU401_SET_UART);	/* set UART mode */
}

int8_t MPU401deinitHardware(void){
    return MPU401sendCommand(MPU401_RESET);
}



struct driverBlock MPU401driver = {
	DRV_MPU401,			// driverID
	sizeof(struct MIDIdata),	// datasize
	MIDIinitDriver,
	MIDIdeinitDriver,
	MPU401driverParam,
	MPU401loadBank,
	MPU401detectHardware,
	MPU401initHardware,
	MPU401deinitHardware,

	MIDIplayNote,
	MIDIreleaseNote,
	MIDIpitchWheel,
	MIDIchangeControl,
	MIDIplayMusic,
	MIDIstopMusic,
	MIDIchangeSystemVolume,
	MIDIpauseMusic,
	MIDIunpauseMusic,
	MPU401sendMIDI
};

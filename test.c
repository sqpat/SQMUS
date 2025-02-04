#include <dos.h>
#include <conio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>
#include <graph.h>

#include <i86.h>
#include "test.h"
#include <sys/types.h>
#include <string.h>
#include "DMX.H"





//#define LOCKMEMORY
//#define NOINTS
//#define USE_USRHOOKS







typedef uint8_t byte;
#define I_Error printf
// REGS stuff used for int calls
union REGS regs;
struct SREGS sregs;

#define intx86(a, b, c) int86(a, b, c)

static uint16_t pageframebase;

#define _outbyte(x,y) (outp(x,y))
#define _outhword(x,y) (outpw(x,y))

#define _inbyte(x) (inp(x))
#define _inhword(x) (inpw(x))
 
 


#define FREAD_BUFFER_SIZE 512

void  far_fread(void __far* dest, uint16_t elementsize, uint16_t elementcount, FILE * fp) {
	// cheating with size/element count
	uint16_t totalsize = elementsize * elementcount;
	uint16_t totalreadsize = 0;
	uint16_t copysize;
	uint16_t remaining;
	byte stackbuffer[FREAD_BUFFER_SIZE];
	byte __far* stackbufferfar = (byte __far *)stackbuffer;
	byte __far* destloc = dest;
	while (totalreadsize < totalsize) {

		remaining = totalsize - totalreadsize;
		copysize = (FREAD_BUFFER_SIZE > remaining) ? remaining : FREAD_BUFFER_SIZE;
		fread(stackbuffer, copysize, 1, fp);
		_fmemcpy(destloc, stackbufferfar, copysize);

		destloc += copysize;
		totalreadsize += copysize;
	}

}

uint16_t currentsong_looping;
uint16_t currentsong_start_offset;
uint16_t currentsong_playing_offset;
uint16_t currentsong_length;
uint16_t currentsong_primary_channels;
uint16_t currentsong_secondary_channels;
uint16_t currentsong_num_instruments;       // 0-127


#define MUS_SEGMENT 	0x6000
#define MUS_LOCATION    (byte __far *) MK_FP(MUS_SEGMENT, 0x000)

int16_t MUS_Parseheader(byte __far *data){
    int16_t __far *  worddata = (int16_t __far *)data;
    if (worddata[0] == 0x554D && worddata[1] == 0x1A53 ){     // MUS file header
        currentsong_length              = worddata[2];  // how do larger that 64k files work?
        currentsong_start_offset        = worddata[3];  // varies
        currentsong_primary_channels    = worddata[4];  // max at  0x07?
        currentsong_secondary_channels  = worddata[5];  // always 0??
        currentsong_num_instruments     = worddata[6];  // varies..  but 0-127
        // reserved   

		return 1; 
    } else {
		return - 1;
	}


}

#define MUS_INTERRUPT_RATE 100 
volatile int16_t called = 0;
void MUS_ServiceRoutine(){
	called = 1;
}




int16_t main(void) {

		int16_t result;
		FILE* fp = fopen("DEMO1.MUS", "rb");
		uint16_t filesize;
		if (fp){
			fseek(fp, 0, SEEK_END);
			filesize = ftell(fp);
			fseek(fp, 0, SEEK_SET);
			// where we're going, we don't need DOS's permission...
			far_fread(MUS_LOCATION, filesize, 1, fp);
			printf("Loaded DEMO1.MUS into memory location 0x%lx successfully...\n", MUS_LOCATION);

			printf("Scheduling interrupt\n");

			TS_Startup();
			TS_ScheduleTask(MUS_ServiceRoutine, MUS_INTERRUPT_RATE);
			TS_Dispatch();
			printf("Interrupt scheduled at %i interrupts per second\n", MUS_INTERRUPT_RATE);

			printf("Now looping until keypress\n");

			while ( !kbhit()){
				if (called){
					putchar('.');
					called = 0;
				}
			}

			printf("Detected keypress, shutting down interrupt...\n");


			TS_SetTimerToMaxTaskRate();
			TS_Shutdown();

			printf("Shut down interrupt, exiting program...\n");






			result = MUS_Parseheader(MUS_LOCATION);
		} else {
			printf("Error: Could not find DEMO1.MUS");
		}





        return 0;
}

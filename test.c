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




#define TRUE (1 == 1)
#define FALSE (!TRUE)

//#define LOCKMEMORY
//#define NOINTS
//#define USE_USRHOOKS

#include <dos.h>
#include <conio.h>
#include <stdio.h>
#include <stdlib.h>




#define BYTES_TO_ALLOCATE (4*1024*1024)
#define PAGE_FRAME_SIZE (16*1024)
#define EMS_INT 0x67

#define false 0
#define true 1


#ifndef __FIXEDTYPES__
#define __FIXEDTYPES__
typedef signed char				int8_t;
typedef unsigned char			uint8_t;
typedef short					int16_t;
typedef unsigned short			uint16_t;
typedef long					int32_t;
typedef unsigned long			uint32_t;
typedef long long				int64_t;
typedef unsigned long long		uint64_t;
#endif



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


int16_t main(void)
  {

		int16_t result;
		FILE* fp = fopen("DEMO1.MUS", "rb");
		uint16_t filesize;
		if (fp){
			fseek(fp, 0, SEEK_END);
			filesize = ftell(fp);
			fseek(fp, 0, SEEK_SET);
			
			far_fread(MUS_LOCATION, filesize, 1, fp);
			printf("Loaded DEMO1.MUS into memory location 0x%lx successfully...\n", MUS_LOCATION);
			

			result = MUS_Parseheader(MUS_LOCATION);
		} else {
			printf("Error: Could not find DEMO1.MUS");
		}





        return 0;
}

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

		currentsong_playing_offset = currentsong_start_offset;
		return 1; 
    } else {
		return - 1;
	}


}

#define MUS_INTERRUPT_RATE 100 
volatile int16_t called = 0;
void MUS_ServiceRoutine(){
	
	// ok lets actually process events....
	int16_t increment = 1; // 1 for the event
	byte __far* currentlocation = MK_FP(MUS_SEGMENT, currentsong_playing_offset);
	byte eventbyte = currentlocation[0];
	byte event     = (eventbyte & 0x70) >> 4;
	byte channel   = (eventbyte & 0x0F);
	byte lastflag  = (eventbyte & 0x80);

	printf("%x: Channel %hhi, Event %hhi:\t", currentsong_playing_offset, channel, event);

	switch (event){
		case 0:
			// Release Note
			{
				byte value 			  = currentlocation[1];
				byte notenumber		  = value & 0x7F;

				// todo release notenumber
				printf("release note 0x%hhx\n", notenumber);
			}
			break;
		case 1:
			// Play Note
			{
				byte value 			  = currentlocation[1];
				byte volume;
				byte notenumber		  = value & 0x7F;
				if (value & 0x80){
					volume = currentlocation[2] & 0x7F;
					increment++;
				} else {
					volume = 0;		// todo: previous volume for the channel? stored?
				}

				// todo play notenumber
				printf("play note 0x%hhx\n", notenumber);
				increment++;

			}

			break;
		case 2:
			// Pitch Bend
			{
				byte value 			  = currentlocation[1];

				// todo bend note
				printf("bend channel by 0x%hhx\n", value);
				increment++;

			}
			break;
		case 3:
			// System Event
			{
				byte controllernumber = currentlocation[1] & 0x7F;
				if (channel == 10 && controllernumber == 120){
					// turn all sounds off no fade
					printf("%hhx: turn all notes off no fade", controllernumber);
				} else if (channel == 11 && controllernumber == 123){
					// turn all sounds off with fade
					printf("%hhx: turn all notes off with fade", controllernumber);
				} else if (channel == 12 && controllernumber == 126){
					// mono (one note on channel)
					printf("%hhx: turn channel mono", controllernumber);
				} else if (channel == 13 && controllernumber == 127){
					// poly (multiple notes on channel)
					printf("%hhx: turn channel poly", controllernumber);
				} else if (channel == 14 && controllernumber == 121){
					// reset all controllers for this channel
					printf("%hhx: reset all channel controllers", controllernumber);
				} else if (channel == 15){
					// never implemented?
					printf("%hhx: unimplemented event?", controllernumber);
				} else {
					printf("BAD SYSTEM EVENT?? 0x%hhx", controllernumber);

				}
								
				printf("\n");
				increment++;
			}


			// TODO handle below cases with data byte 0

			
		case 4:
			// Controller
			{
				byte controllernumber = currentlocation[1]; // values above 127 used for instrument change & 0x7F;
				byte value 			  = currentlocation[2]; // values above 127 used for instrument change & 0x7F; ?
				
				if (channel == 0){
					// change instrument
					printf("%hhx %hhx: change instrument?", controllernumber, value);
				} else if (channel == 1 && (controllernumber == 0 || controllernumber == 32)){
					// bank select, 0 by default
					printf("%hhx %hhx: bank select?", controllernumber, value);
				} else if (channel == 2 && controllernumber == 1){
					// modulation (vibrato frequency)
					printf("%hhx %hhx: modulation", controllernumber, value);
				} else if (channel == 3 && controllernumber == 7){
					// volume 0 - 127. 100 is normal?
					printf("%hhx %hhx: volume", controllernumber, value);
				} else if (channel == 4 && controllernumber == 10){
					// pan (balance) 0 left 64 center 127 right
					printf("%hhx %hhx: pan", controllernumber, value);
				} else if (channel == 5 && controllernumber == 11){
					// expression
					printf("%hhx %hhx: expression", controllernumber, value);
				} else if (channel == 6 && controllernumber == 91){
					// reverb depth
					printf("%hhx %hhx: reverb", controllernumber, value);
				} else if (channel == 7 && controllernumber == 93){
					// chorus depth
					printf("%hhx %hhx: chorus", controllernumber, value);
				} else if (channel == 8 && controllernumber == 64){
					// sustain pedal
					printf("%hhx %hhx: sustain pedal", controllernumber, value);
				} else if (channel == 9 && controllernumber == 67){
					// soft pedal
					printf("%hhx %hhx: soft pedal", controllernumber, value);
				} else {
					printf("%hhx %hhx: BAD CONTROLLER?", controllernumber, value);

				}
				
				printf("\n");
				// todo silently skip event 3 cases

				increment++;
				increment++;
			}
			break;
		case 5:
			// End of Measure
			// do nothing..
			printf("End of Measure\n");

			break;
		case 6:
			// Finish
			printf("Song over\n");
			if (currentsong_looping){
				// is this right?
				printf("LOOP SONG!\n");
				currentsong_playing_offset = currentsong_start_offset;
				return;
			}
			break;
		case 7:
			// Unused
			printf("UNUSED EVENT 7?\n");
			increment++;   // advance for one data byte
			break;
	}

	
	currentsong_playing_offset += increment;
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
					called = 0;
				}
			}

			printf("Detected keypress, shutting down interrupt...\n");


			TS_Shutdown();

			printf("Shut down interrupt, exiting program...\n");






			result = MUS_Parseheader(MUS_LOCATION);
		} else {
			printf("Error: Could not find DEMO1.MUS");
		}





        return 0;
}

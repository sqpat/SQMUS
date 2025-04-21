#include <dos.h>
#include <conio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>
#include <graph.h>

#include <i86.h>
#include "sqcommon.h"
#include "sqmusopl.h"
#include "sqmusmpu.h"
#include "sqmussbm.h"
#include "sqmusmid.h"
#include "sqmussfx.h"
#include <sys/types.h>
#include <string.h>
#include "DMX.H"
#include <signal.h>
#include <bios.h>
#include <ctype.h>
#include <malloc.h>



void donothing(){

}



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






// todo
// x dynamic load of num instruments
// x parse filename
// x parse sound engine choice
// 3. test sbmidi
// x. test looping
// n locallib printf?
// use playingstate
struct driverBlock	*playingdriver = &OPL2driver;

uint16_t 			currentsong_looping;
uint16_t 			currentsong_start_offset;
uint16_t 			currentsong_playing_offset;
uint16_t 			currentsong_length;
int16_t 			currentsong_primary_channels;
int16_t 			currentsong_secondary_channels;
uint16_t 			currentsong_num_instruments;       // 0-127

uint16_t 			currentsong_play_timer;
uint32_t 			currentsong_int_count;
int16_t 			currentsong_ticks_to_process = 0;
byte __far*  		muslocation;







int8_t				playingpcspeakersfx;
uint16_t			sfxlength;
uint16_t 			sfxlength_currentsample;


uint8_t				playingstate = ST_PLAYING;			
uint16_t			playingpercussMask = 1 << PERCUSSION;	// todo #define? or should other instruments be forced into percussion?
int16_t     		playingvolume = DEFAULT_VOLUME;
volatile uint32_t 	playingtime = 0;
volatile int16_t 	called = 0;
volatile int16_t 	finishplaying = 0;
OP2instrEntry 		AdLibInstrumentList[MAX_INSTRUMENTS_PER_TRACK];
uint8_t 			instrumentlookup[MAX_INSTRUMENTS];
int8_t				loops_enabled = false;
int16_t opl_valid_instruments = 0;

int16_t 	myargc;
int8_t**	myargv;

int16_t MUS_Parseheader(byte __far *data){
    int16_t __far *  worddata = (int16_t __far *)data;
	int8_t i;
    if (worddata[0] == 0x554D && worddata[1] == 0x1A53 ){     // MUS file header
        currentsong_length              = worddata[2];  // how do larger that 64k files work?
        currentsong_start_offset        = worddata[3];  // varies
        currentsong_primary_channels    = worddata[4];  // max at  0x07?
        currentsong_secondary_channels  = worddata[5];  // always 0??
        currentsong_num_instruments     = worddata[6];  // varies..  but 0-127
        // reserved   
		printmessage("Parsed values: \n");
		printmessage("length:             0x%x\n",currentsong_length);
		printmessage("start offset:       0x%x\n",currentsong_start_offset);
		printmessage("primary channels:   0x%x\n",currentsong_primary_channels);
		printmessage("secondary channels: 0x%x\n",currentsong_secondary_channels);
		printmessage("num instruments:    0x%x\n",currentsong_num_instruments);	// todo dynamically load the data from the main bank at startup to take less memory?

		if (currentsong_num_instruments > MAX_INSTRUMENTS_PER_TRACK){
			printerror("Too many instruments! %i vs max of %i", currentsong_num_instruments, MAX_INSTRUMENTS_PER_TRACK);
		}

		currentsong_playing_offset = currentsong_start_offset;
		currentsong_play_timer = 0;
		currentsong_int_count = 0;
		currentsong_ticks_to_process = 0;
		

		// parse instruements
		memset(instrumentlookup, 0xFF, MAX_INSTRUMENTS);
		for (i = 0; i < currentsong_num_instruments; i++){
			uint16_t instrument = worddata[8+i];
			if (instrument > 127){
				instrument -= 7;
			}
			// printf("Setup %03i %03i  ", i, instrument);
			if (instrument <= 174){
				opl_valid_instruments++;
			}
			instrumentlookup[instrument] = i;	// this instrument is index i in AdLibInstrumentList
		}
		
		// currentsong_num_instruments = real_counted_instruments;
		//printf("valid opl instruments %i\n", currentsong_num_instruments);

		return 1; 
    } else {
		printerror("Bad header %x %x", worddata[0], worddata[1]);
		return - 1;
	}


}


#define MUS_INTERRUPT_RATE 140
#define SPEAKER_INTERRUPT_RATE 140


uint16_t pc_speaker_freq_table[128] = {
	   0, 6818, 6628, 6449, 6279, 6087, 5906, 5736, 
	5575, 5423, 5279, 5120, 4971, 4830, 4697, 4554, 
	4435, 4307, 4186, 4058, 3950, 3836, 3728, 3615, 
	3519, 3418, 3323, 3224, 3131, 3043, 2960, 2875, 
	2794, 2711, 2633, 2560, 2485, 2415, 2348, 2281, 
	2213, 2153, 2089, 2032, 1975, 1918, 1864, 1810, 
	1757, 1709, 1659, 1612, 1565, 1521, 1478, 1435, 
	1395, 1355, 1316, 1280, 1242, 1207, 1173, 1140, 
	1107, 1075, 1045, 1015,  986,  959,  931,  905, 
	 879,  854,  829,  806,  783,  760,  739,  718, 
	 697,  677,  658,  640,  621,  604,  586,  570,
     553,  538,  522,  507,  493,  479,  465,  452, 
	 439,  427,  415,  403,  391,  380,  369,  359, 
	 348,  339,  329,  319,  310,  302,  293,  285,
     276,  269,  261,  253,  246,  239,  232,  226, 
     219,  213,  207,  201,  195,  190,  184,  179

	};



void playpcspeakernote(uint8_t samplebyte){
	uint16_t value = pc_speaker_freq_table[samplebyte];

	if (value){
		outp (0x43, 0xB6);
		outp (0x42, value &0xFF);
		outp (0x42, value >> 8);
		outp (0x61, inp(0x61) | 3);
		
		// if (status != status | 3){
		// 	outp (0x61, status | 3);
		// }

	} else {
		outp(0x61, inp(0x61) & 0xFC);
	}



}

void stoppcspeaker(){
	uint8_t tmp = inp(0x61) & 0xFC;
	outp(0x61, tmp);
}

int8_t next = 0;
uint16_t lastvalue = 0;

void MUS_ServiceRoutine(){
	if (finishplaying){
		return;
	}	
	return;
/*
	if (playingpcspeakersfx) {

		// todo inline the plus 4 or whatever? 
		// if (next == 10){
			//playpcspeakernote(sfxlocation[4+sfxlength_currentsample]);

			uint16_t value = pc_speaker_freq_table[sfxlocation[4+sfxlength_currentsample]];

			if (value){
				if (value != lastvalue){
					outp (0x43, 0xB6);
					outp (0x42, value);
					outp (0x42, value >> 8);
					outp (0x61, inp(0x61) | 3);
					lastvalue = value;
				}
				// if (status != status | 3){
				// 	outp (0x61, status | 3);
				// }

			// } else {
				// outp(0x61, inp(0x61) & 0xFC);
			// }
	
			sfxlength_currentsample++;
			next = 0;
		} else {
			next++;
		}
		if (sfxlength_currentsample == sfxlength){
			sfxlength_currentsample = 0;
			// or playingsfx = false?
			stoppcspeaker();
			playingpcspeakersfx = false;

		}
	}
*/


	currentsong_ticks_to_process ++;
	while (currentsong_ticks_to_process >= 0){

		// ok lets actually process events....
		int16_t increment 			= 1; // 1 for the event
		byte doing_loop 			= false;
		byte __far* currentlocation = muslocation + currentsong_playing_offset;
		uint8_t eventbyte 			= currentlocation[0];
		uint8_t event     			= (eventbyte & 0x70) >> 4;
		int8_t  channel   			= (eventbyte & 0x0F);
		byte lastflag  				= (eventbyte & 0x80);
		int16_t_union delay_amt		= {0};


		// // todo is this the right way...?
		// if (channel > currentsong_primary_channels && channel < 10 && channel != PERCUSSION_CHANNEL){
		// 	printf("primary changed channel to percussion %i", channel);
		// 	channel = PERCUSSION_CHANNEL;
		// } else if ((channel - 10) > currentsong_secondary_channels && channel != PERCUSSION_CHANNEL){
		// 	//todo get rid of secondaries?
		// 	printf("secondary changed channel to percussion %i", channel);
		// 	channel = PERCUSSION_CHANNEL;
		// }

		switch (event){
			case 0:
				// Release Note
				{
					byte value 			  = currentlocation[1];
					byte key		  = value & 0x7F;
					playingdriver->releaseNote(channel, value);

				}
				increment++;
				break;
			case 1:
				// Play Note
				{
					uint8_t value 			  = currentlocation[1];
					byte volume = -1;  		// -1 means repeat..
					uint8_t key		  = value & 0x7F;
					if (value & 0x80){
						volume = currentlocation[2] & 0x7F;
						increment++;
					}
					playingdriver->playNote(channel, key, volume);
					increment++;
				}

				break;
			case 2:
				// Pitch Bend
				{
					byte value 			  = currentlocation[1];
					increment++;
					playingdriver->pitchWheel(channel, value);

				}
				break;
			case 3:
				// System Event
				{
					byte controllernumber = currentlocation[1] & 0x7F;
					playingdriver->changeControl(channel, controllernumber, 0);
					increment++;
				}

				break;
				
			case 4:
				// Controller
				{
					uint8_t controllernumber  = currentlocation[1] & 0x7F; // values above 127 used for instrument change & 0x7F;
					uint8_t value 			  = currentlocation[2] & 0x7F; // values above 127 used for instrument change & 0x7F; ?

					playingdriver->changeControl(channel, controllernumber, value);
					increment++;
					increment++;
				}
				break;
			case 5:
				// End of Measure
				// do nothing..
				//printf("End of Measure\n");

				break;
			case 6:
				// Finish
				printmessage("\nSong over\n");
				if (currentsong_looping){
					// is this right?
					doing_loop = true;
				} else {
					if (loops_enabled){
						doing_loop = true;
					} else {
						finishplaying = 1;
					}
				}
				if (doing_loop){
					printmessage("LOOPING SONG!\n");
				}
				break;
			case 7:
				// Unused
				printmessage("UNUSED EVENT 7?\n");
				increment++;   // advance for one data byte
				break;
		}

		currentsong_playing_offset += increment;

		while (lastflag){
			// i dont think delays > 32768 are valid..
			currentlocation = muslocation + currentsong_playing_offset;
			delay_amt.hu <<= 7;
			//delay_amt.bu.bytehigh >>= 1;	// shift 128.
			lastflag = currentlocation[0] ;
			delay_amt.bu.bytelow += (lastflag & 0x7F);

			lastflag &= 0x80;
			currentsong_playing_offset++;
		}
		//printf("%i %i %hhx\n", currentsong_ticks_to_process, currentsong_ticks_to_process - delay_amt.hu, delay_amt.hu);
		currentsong_ticks_to_process -= delay_amt.hu;

		//todo how to handle loop/end song plus last flag?
		if (doing_loop){
			// todo do we have to reset or something?
			currentsong_playing_offset = currentsong_start_offset;
		}
		if (finishplaying){
			break;
		}


	}
	
	called = 1;
	currentsong_int_count++;
	playingtime++;
}

#define driver_type_any 	0
#define driver_type_opl2 	1
#define driver_type_opl3    2
#define driver_type_mpu401  3
#define driver_type_sbmidi  4
#define driver_type_none	5

int8_t tryloaddrivertype(int8_t type){
	switch (type){
	
		case driver_type_opl2:
			if (OPL2detectHardware(ADLIBPORT, 0, 0)){
				printmessage("OPL2 Detected...\n");
				playingdriver = &OPL2driver;
				playingdriver->initHardware(ADLIBPORT, 0, 0);
				playingdriver->initDriver();
				printmessage("OPL2 Enabled...\n");
				return 1;
			}
			return 0;
		case driver_type_opl3:

			if (OPL3detectHardware(ADLIBPORT, 0, 0)){
				playingdriver = &OPL3driver;
				printmessage("OPL3 Detected...\n");
				playingdriver->initHardware(ADLIBPORT, 0, 0);
				playingdriver->initDriver();
				printmessage("OPL3 Enabled...\n");
				return 1;
			} 
			return 0;
		case driver_type_mpu401:
			if (MPU401detectHardware(MPU401PORT, 0, 0)){
				playingdriver = &MPU401driver;
				printmessage("MPU-401 Detected...\n");
				playingdriver->initHardware(MPU401PORT, 0, 0);
				playingdriver->initDriver();
				printmessage("MPU-401 Enabled...\n");
				return 1;
			}
			return 0;
		case driver_type_sbmidi:
			if (SBMIDIdetectHardware(SBMIDIPORT, 0, 0)){
				printmessage("SB MIDI Detected...\n");
				playingdriver = &SBMIDIdriver;
				playingdriver->initHardware(SBMIDIPORT, 0, 0);
				playingdriver->initDriver();
				printmessage("SB MIDI Enabled...\n");
				return 1;
			}
			return 0;

	}
	return 0;
}

int8_t attemptDetectingAnyHardware(){
	int8_t i = 1;
	for (i = 1; i < driver_type_none; i++){
		if (tryloaddrivertype(i)){
			return 1;
		}	
	}

	printerror("ERROR! No SB sound hardware detected!\n");
	return 0;
}

void sigint_catcher(int sig) {
	TS_Shutdown();
	if (playingdriver){
		playingdriver->stopMusic();
		playingdriver->deinitHardware();
	}
	exit(sig);
}


void locallib_strlwr(int8_t *str){
	int16_t i = 0;
	while (str[i] != '\0'){
		if ((str[i] >= 'A') && (str[i] <= 'Z')){
			str[i] += 32;
		}
		i++;
	}
}

int16_t locallib_strcmp(int8_t *str1, int8_t *str2){
	int16_t i = 0;
	while (str1[i]){
		int16_t b  = str1[i] - str2[i];
		if (b){
			return b;
		}
		i++;
	}
	return str1[i] - str2[i];
}

/*
void locallib_strupr(int8_t  *str){
	int i = 0;
	while (str[i] != '\0'){
		if ((str[i] >= 'a') && (str[i] <= 'z')){
			str[i] -= 32;
		}
		i++;
	}
}
*/

uint8_t  locallib_toupper(uint8_t ch){
	if (ch >=  0x61 && ch <= 0x7A){
		return ch - 0x20;
	}
	return ch;
}


int16_t checkparm (int8_t *check) {
    int16_t		i;
	// ASSUMES *check is LOWERCASE. dont pass in uppercase!
	// myargv must be tolower()
	// trying to avoid strcasecmp dependency.
    for (i = 1;i<myargc;i++) {
		// technically this runs over and over for myargv, 
		// but its during initialization so who cares speed-wise. 
		// code is smaller to stick it here rather than make a loop elsewhere (i think)
		locallib_strlwr(myargv[i]);
		if ( !locallib_strcmp(check, myargv[i]) )
			return i;
		}

    return 0;
}

int8_t* findfilenameparm(){
	int16_t i;
	for (i = 1;i<myargc;i++) {
		int16_t j;
		locallib_strlwr(myargv[i]);
		for (j = 0; myargv[i][j] != '\0'; j++){ 
			// find end 
		}
		j-=4;
		if (( *((uint16_t *) &(myargv[i][j+0]))   == 0x6D2E) &&	//.m
			  *((uint16_t *) &(myargv[i][j+2]))   == 0x7375	//us
		){
			return myargv[i];
		}
	}
    return 0;
}
 








int16_t main(int16_t argc, int8_t** argv) {
	int16_t result;
	int8_t* filename;
	// int8_t* sfxfilename = "DSBAREXP.lmp";
	// int8_t* sfxfilename = "DSBDOPN.lmp";

	// int8_t* pcsfxfilename = "DPBDCLS.lmp";
	//int8_t* pcsfxfilename = "DPBDOPN.lmp";
	int8_t* pcsfxfilename = "DPITEMUP.lmp";
	FILE* fp;
	uint16_t filesize;
	int16_t param;
	int16_t userspecifieddriver = 0;
	myargc = argc;
	myargv = argv;

	filename = findfilenameparm();
	if (!filename){
		printerror("Couldn't find MUS file parameter!");
		return 0;
	}

	param = checkparm("-d");
	if (param){
		userspecifieddriver = myargv[param + 1][0] - '0';
	}

	param = checkparm("-loop");
	if (param){
		loops_enabled = true;
	}

	fp  = fopen(filename, "rb");

	if (fp){
		int8_t i;
		fseek(fp, 0, SEEK_END);
		filesize = ftell(fp);
		fseek(fp, 0, SEEK_SET);
		// where we're going, we don't need DOS's permission...
		muslocation = _fmalloc(filesize);
		if (!muslocation){
			printerror("Couldn't malloc %u bytes", filesize);
			return 0;
		}

		far_fread(muslocation, filesize, 1, fp);
		fclose(fp);
		
		result = MUS_Parseheader(muslocation);
		printmessage("Loaded mus %s into memory location 0x%lx successfully...\n", filename, muslocation);

		// load pcm sfx!
		for (i = 0; i < NUM_SFX_LUMPS; i++){
			fp  = fopen(sfxfilename[i], "rb");
			if (fp){
				int16_t_union word_read;
				
				// process header
				fgetc(fp);	// supposed to be 00
				fgetc(fp);	// supposed to be 00
				word_read.bu.bytelow  = fgetc(fp); // sample rate lo
				word_read.bu.bytehigh = fgetc(fp); // sample rate hi
				sb_sfx_info[i].samplerate =  (word_read.hu == SAMPLE_RATE_22_KHZ_UINT) ? 1 : 0;
				word_read.bu.bytelow  = fgetc(fp);	// sample count lo
				word_read.bu.bytehigh = fgetc(fp);  // sample count hi  (TODO: in theory 4 bytes)
				sb_sfx_info[i].length =  word_read.hu - 32;	// subtract padding
				fseek(fp, 0x18, SEEK_SET);

 				sb_sfx_info[i].location = (uint8_t __far*) _fmalloc(sb_sfx_info[i].length);
				
				// cut out the header and reload the rest
				far_fread(sb_sfx_info[i].location, sb_sfx_info[i].length, 1, fp);
				fclose(fp);
				
			
				//printmessage("Loaded pcm sfx %s into memory location 0x%lx successfully...\n", sfxfilename[i], sb_sfx_info[i].location);



			} else {
				printmessage("\n\nBAD SFX FILE? %s\n\n", sfxfilename[i]);

			}

		}

		if (SB_InitCard() == SB_OK){
			if (SB_SetupPlayback() == SB_OK){
				printf("\nSound Blaster SFX Engine Initailized!.. ");

			} else {
				printf("Error B\n");
			}

		} else {
			printf("\nSB INIT Error A\n");
		}

		// load speaker sfx!

		// fp  = fopen(pcsfxfilename, "rb");
		// if (fp){
		// 	uint16_t sfxfilesize;
		// 	fseek(fp, 0, SEEK_END);
		// 	sfxfilesize = ftell(fp);
		// 	fseek(fp, 0, SEEK_SET);
		// 	sfxlocation = _fmalloc(sfxfilesize);

		// 	far_fread(sfxlocation, sfxfilesize, 1, fp);
		// 	fclose(fp);

		// 	sfxlength = ((uint16_t __far*) sfxlocation)[1];	// number of samples
		// 	sfxlength_currentsample = 0;
		
		// 	printmessage("Loaded pc speaker sfx %s into memory location 0x%lx successfully... %x\n", pcsfxfilename, sfxlocation);

		// 	// playingpcspeakersfx = true;
		// 	playingpcspeakersfx = false;
		// }


		// todo only if OPL?

		fp = fopen("genmidi.lmp", "rb");
		if (fp){
			// todo read based on numinstruments
			uint8_t i;
			for (i = 0; i < MAX_INSTRUMENTS; i++){
				uint8_t instrumentindex = instrumentlookup[i];
				if (instrumentindex != 0xFF){
					uint16_t offset = sizeof(OP2instrEntry) * i;

					fseek(fp, offset, SEEK_SET);
					far_fread(&AdLibInstrumentList[instrumentindex], sizeof(OP2instrEntry), 1, fp);
				}
			}
			// printmessage("Read instrument data!\n");
			fclose(fp);
		} else {
			printerror("Error reading genmidi.lmp!\n");
			_ffree(muslocation);
			return 0;
		}

		printmessage("Enabling Sound...\n");

		if (userspecifieddriver){
			if (!tryloaddrivertype(userspecifieddriver)){
				printerror("Could not find that driver!\n");
				_ffree(muslocation);
				return 0;
			}
		} else {
			if (!attemptDetectingAnyHardware()){
				_ffree(muslocation);
				return 0;
			}
		}

		printmessage("Driver song setup...\n");
		playingdriver->stopMusic();
		playingdriver->playMusic();

		printmessage("Scheduling interrupt\n");
		signal(SIGINT, sigint_catcher);

		TS_Startup();
		TS_ScheduleTask(MUS_ServiceRoutine, MUS_INTERRUPT_RATE);
		TS_Dispatch();
		
		printmessage("Interrupt scheduled at %i interrupts per second\n", MUS_INTERRUPT_RATE);

		printmessage("Now looping until ESC keypress\n");

		while (true){
			if (_bios_keybrd(_KEYBRD_READY)){
				uint16_t key = _bios_keybrd(_KEYBRD_READ);
				if (key == 0x011B){		/* Esc - exit to DOS */
					break;
				}
			    if (key & 0xFF00){		/* if not extended key */
					uint8_t usedkey = locallib_toupper(key & 0x00FF);
					switch (usedkey) {
						case '=':
						case '+':		/* '+' - increase volume */
							if (playingvolume <= 512-4 ){
								playingvolume += 4;
								playingdriver->changeSystemVolume(playingvolume);
							}
							break;
						case '-':		/* '-' - decrease volume */
							if (playingvolume >= 4){
								playingvolume -= 4;
								playingdriver->changeSystemVolume(playingvolume);
							} else {
								playingdriver->changeSystemVolume(0);
							}
							break;
						case '*':		/* '*' - normal volume */
							playingdriver->changeSystemVolume(DEFAULT_VOLUME);
							break;
						case '/':		/* '/' - mute volume */
							playingdriver->changeSystemVolume(MUTE_VOLUME);
							break;
						case '0':
						case '1':
						case '2':
						case '3':
						case '4':
						case '5':
						case '6':
						case '7':
						case '8':
						case '9':
							SB_PlaySoundEffect(usedkey - '0');

							break;

					}
				}

			}

			if (called){

				uint16_t val1 = currentsong_int_count / (60 * MUS_INTERRUPT_RATE);
				uint16_t val2 = (currentsong_int_count / (MUS_INTERRUPT_RATE)) % 60;
				uint16_t val3 = (1000L * (currentsong_int_count % (MUS_INTERRUPT_RATE))) / (MUS_INTERRUPT_RATE);

				cprintf("\rPlaying %s ... %2i:%02i.%03i   Vol: %i (%li) ", filename,
					val1, 
					val2,
					val3,
					playingvolume,
					currentsong_int_count
					);

			}
			if (finishplaying){
				break;
			}
		}

		if (finishplaying){
			printmessage("\nSong Finished, shutting down interrupt...\n");
		} else {
			printmessage("\nDetected ESC keypress, shutting down interrupt...\n");
		}

		TS_Shutdown();
		SB_Shutdown();
		printmessage("Shut down interrupt, flushing sound hardware...\n");


		playingdriver->stopMusic();
		playingdriver->deinitHardware();

		printmessage("Sound hardware state cleared, freeing memory...\n");

		_ffree(muslocation);
		printmessage("Exiting program...\n");

	} else {
		printerror("Error: Could not find %s", filename);
	}

	return 0;

}

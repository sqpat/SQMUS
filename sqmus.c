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
byte __far*  		sfxlocation;
int8_t				playingpcmsfx;
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

uint16_t pcm_samplerate;

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
 


#define SB_DSP_Set_DA_Rate 0x41
#define SB_DSP_Set_AD_Rate 0x42

#define SB_Ready 0xAA

#define SB_MixerAddressPort  0x4
#define SB_MixerDataPort 	 0x5
#define SB_ResetPort 		 0x6
#define SB_ReadPort 		 0xA
#define SB_WritePort 		 0xC
#define SB_DataAvailablePort 0xE

// hacked settings for now

//todo! configure these!
#define UNDEFINED_DMA -1

#define FIXED_SB_PORT 0x220
#define FIXED_SB_DMA_8 1
#define FIXED_SB_DMA_16 UNDEFINED_DMA
#define FIXED_SB_IRQ 7

#define SB_STEREO 1
#define SB_SIXTEEN_BIT 2


// actual variables that get set.
// todo: set from environment variable.
int16_t sb_port = -1;
int16_t sb_dma  = -1;
int16_t sb_int  = -1;
int16_t sb_irq  = -1;

int8_t sb_dma_16 = UNDEFINED_DMA;
int8_t sb_dma_8  = UNDEFINED_DMA;


void( __interrupt __far *SB_OldInt)(void);
char __far* SB_DMABuffer;
char __far* SB_DMABufferEnd;
char __far* SB_CurrentDMABuffer;
uint16_t 	SB_TotalDMABufferSize;

uint16_t SB_MixMode = SB_STEREO;  // | SB_SIXTEEN_BIT;

uint8_t SB_Mixer_Status;


byte __far* SB_BUFFERS[2] = {
	(byte __far*)0x84000000,	// todo change
	(byte __far*)0x88000000
};

// todo what does this mean
#define MixBufferSize 256

#define NumberOfBuffers 16
#define TotalBufferSize (MixBufferSize * NumberOfBuffers)

#define SB_TransferLength MixBufferSize


#define MIXER_MPU401_INT 0x4
#define MIXER_16BITDMA_INT 0x2
#define MIXER_8BITDMA_INT 0x1


void __interrupt __far SB_ServiceInterrupt(void) {

    // Acknowledge interrupt
    // Check if this is this an SB16 or newer
    // if (SB_Version >= DSP_Version4xx) {
        outp(sb_port + SB_MixerAddressPort, 0x82);  //  MIXER_DSP4xxISR_Ack);

        SB_Mixer_Status = inp(sb_port + SB_MixerDataPort);

        // Check if a 16-bit DMA interrupt occurred
        if (SB_Mixer_Status & MIXER_16BITDMA_INT) {
            // Acknowledge 16-bit transfer interrupt
            inp(sb_port + 0x0F);	// SB_16BitDMAAck
        }
        else if (SB_Mixer_Status & MIXER_8BITDMA_INT) {
            inp(sb_port + SB_DataAvailablePort);
        } else {


			// Wasn't our interrupt.  Call the old one.
			_chain_intr(SB_OldInt);
		
	    }
    // } else {
    //     // Older card - can't detect if an interrupt occurred.
    //     inp(sb_port + SB_DataAvailablePort);
    // }

    // Keep track of current buffer
    SB_CurrentDMABuffer += SB_TransferLength;

    if (SB_CurrentDMABuffer >= SB_DMABufferEnd) {
        SB_CurrentDMABuffer = SB_DMABuffer;
    }

    // Continue playback on cards without autoinit mode
    // if (SB_Version < DSP_Version2xx) {
    //     if (SB_SoundPlaying) {
    //         SB_DSP1xx_BeginPlayback(SB_TransferLength);
    //     }
    // }

    // Call the caller's callback function
    // if (SB_CallBack != NULL) {
    //     MV_ServiceVoc();
    // }

    // send EOI to Interrupt Controller
    if (sb_irq > 7){
        outp(0xA0, 0x20);
    }

    outp(0x20, 0x20);
}


int16_t     SB_IntController1Mask;
int16_t     SB_IntController2Mask;


enum SB_ERRORS
{
    SB_Warning = -2,
    SB_Error = -1,
    SB_OK = 0,
    SB_EnvNotFound,
    SB_AddrNotSet,
    SB_DMANotSet,
    SB_DMA16NotSet,
    SB_InvalidParameter,
    SB_CardNotReady,
    SB_NoSoundPlaying,
    SB_InvalidIrq,
    SB_UnableToSetIrq,
    SB_DmaError,
    SB_NoMixer,
    SB_DPMI_Error,
    SB_OutOfMemory
};

void SB_WriteDSP(byte value)
{
    int16_t port = sb_port + SB_WritePort;
    uint16_t count = 0xFFFF;

    while (count) {
        if ((inp(port) & 0x80) == 0) {
            outp(port, value);
			return;
        }
        count--;
    }
}

int16_t SB_ReadDSP(void) {
    int16_t port = sb_port + SB_DataAvailablePort;
    uint16_t count;

    count = 0xFFFF;

    while (count) {
        if (inp(port) & 0x80) {
            return inp(sb_port + SB_ReadPort);
        }
        count--;
    }

    return SB_Error;
}

int16_t SB_ResetBlaster(){
    volatile uint8_t count;
    int16_t port = sb_port + SB_ResetPort;

    outp(port, 1);

    count = 0xFF;
    while (count){
		count--;
	}

    outp(port, 0);
    count = 100;

    while (count) {
        if (SB_ReadDSP() == SB_Ready) {
            return SB_OK;
            break;
        }
        count--;
    } 

    return SB_CardNotReady;
}

void SB_SetPlaybackRate(int16_t sample_rate){
	// Set playback rate
	SB_WriteDSP(SB_DSP_Set_DA_Rate);
	SB_WriteDSP(sample_rate >> 8);
	SB_WriteDSP(sample_rate & 0xFF);

	// Set recording rate
	SB_WriteDSP(SB_DSP_Set_AD_Rate);
	SB_WriteDSP(sample_rate >> 8);
	SB_WriteDSP(sample_rate & 0xFF);
}

void SB_SetMixMode(){
	// todo: some soundblasters (sb pro but not sb16?) 
	// require setting mix mode for mono/stereo

}

#define DMA_8_MAX_CHANNELS 4
#define VALID_IRQ(irq) (((irq) >= 0) && ((irq) <= 15))

#define INVALID_IRQ 0xFF



// todo this is 16 bit 
// need to handle 8 bit case too...
uint8_t IRQ_TO_INTERRUPT_MAP[16] =
    {
        INVALID_IRQ, INVALID_IRQ, 0x0A, 	   0x0B,
        INVALID_IRQ, 0x0D, 		  INVALID_IRQ, 0x0F,
        INVALID_IRQ, INVALID_IRQ, 0x72, 	   0x73,
        0x74, 		 INVALID_IRQ, INVALID_IRQ, 0x77};



void	SB_StopPlayback(){
	// todo if sb is currentlty active stop it, clean up dma etc
}



#define SB_DSP_SignedBit 0x10
#define SB_DSP_StereoBit 0x20

#define SB_DSP_UnsignedMonoData 	0x00
#define SB_DSP_SignedMonoData 		(SB_DSP_SignedBit)
#define SB_DSP_UnsignedStereoData 	(SB_DSP_StereoBit)
#define SB_DSP_SignedStereoData 	(SB_DSP_SignedBit | SB_DSP_StereoBit)

#define SB_DSP_Halt8bitTransfer 		0xD0
#define SB_DSP_Continue8bitTransfer 	0xD4
#define SB_DSP_Halt16bitTransfer 		0xD5
#define SB_DSP_Continue16bitTransfer 	0xD6
#define SB_DSP_Reset 					0xFFFF

uint16_t SB_HaltTransferCommand;


void SB_DSP4xx_BeginPlayback() {
    int8_t TransferCommand;
    int8_t TransferMode;
    int16_t SampleLength = MixBufferSize;

    if (SB_MixMode & SB_SIXTEEN_BIT) {
        TransferCommand = 0xB6; // 16 bit DAC
        SampleLength >>= 1;
        SB_HaltTransferCommand = SB_DSP_Halt16bitTransfer;
        if (SB_MixMode & SB_STEREO) {
            TransferMode = SB_DSP_SignedStereoData;
        } else {
            TransferMode = SB_DSP_SignedMonoData;
        }
    } else {
        TransferCommand = 0xC6; // 8 bit dac
        
        SB_HaltTransferCommand = SB_DSP_Halt8bitTransfer;
        if (SB_MixMode & SB_STEREO) {
            TransferMode = SB_DSP_UnsignedStereoData;
        } else {
            TransferMode = SB_DSP_UnsignedMonoData;
        }
    }

	SampleLength--;


    // Program DSP to play sound
    SB_WriteDSP(TransferCommand);
    SB_WriteDSP(TransferMode);
    SB_WriteDSP(SampleLength&0xFF);
    SB_WriteDSP(SampleLength>>8);


}

#define DMA_INVALID
#define DMA_VALID

typedef struct
{
    //int valid;	// 2 and 4 invalid
    // int Mask;	0x0A, 0xD4
    // int Mode;	0x0B, 0xD6
    // int Clear;	0x0C, 0xD8
    uint8_t page;
    uint8_t address;
    uint8_t length;
} DMA_PORT;

#define DMA_MaxChannel_16_BIT 7

DMA_PORT DMA_PortInfo[8] =
    {
        {0x87, 0x00, 0x01},
        {0x83, 0x02, 0x03},
        {0x81, 0x04, 0x05},
        {0x82, 0x06, 0x07},
        {0x8F, 0xC0, 0xC2},
        {0x8B, 0xC4, 0xC6},
        {0x89, 0xC8, 0xCA},
        {0x8A, 0xCC, 0xCE},
};

#define DMA_ERROR 0
#define DMA_OK 1

int8_t SB_DMA_VerifyChannel(uint8_t channel) {
    if (channel > DMA_MaxChannel_16_BIT) {
        return DMA_ERROR;
    } else if (channel == 2 || channel == 4) {	// invalid dma channels
        return DMA_ERROR;
    }

    return DMA_OK;
}



int16_t DMA_SetupTransfer(uint8_t channel, byte __far* address, uint16_t length) {
    
    if (SB_DMA_VerifyChannel(channel) == DMA_OK) {


    	DMA_PORT __near* port = &DMA_PortInfo[channel];
        uint8_t  channel_select = channel & 0x3;
		uint32_t addr = (uint32_t)address;
		uint8_t  page;
		uint8_t  hi_byte;
		uint8_t  lo_byte;
    	uint16_t transfer_length;


        if (channel > 3) {	// 16 bit port
            page = 	  (addr >> 16) & 255;
            hi_byte = (addr >> 9) & 255;
            lo_byte = (addr >> 1) & 255;

            // Convert the length in bytes to the length in words
            transfer_length = (length + 1) >> 1;

            // The length is always one less the number of bytes or words
            // that we're going to send
            transfer_length--;
        } else {			// 8 bit port
            page = 	  (addr >> 16) & 255;
            hi_byte = (addr >> 8) & 255;
            lo_byte =  addr & 255;

            // The length is always one less the number of bytes or words
            // that we're going to send
            transfer_length = length - 1;
        }

        // Mask off DMA channel
        outp(channel < 4 ? 	0x0A: 0xD4, 4 | channel_select);

        // Clear flip-flop to lower byte with any data
        outp(channel < 4 ? 	0x0C: 0xD8, 0);

        // Set DMA mode
        // switch (DMA_AutoInitRead) {
		// 	case DMA_SingleShotRead:
		// 		outp(port->mode, 0x48 | channel_select);
		// 		break;
		// 	case DMA_SingleShotWrite:
		// 		outp(port->mode, 0x44 | channel_select);
		// 		break;
		//	case DMA_AutoInitRead:
				outp(channel < 4 ? 	0x0B: 0xD6, 0x58 | channel_select);
		//		break;
		// 	case DMA_AutoInitWrite:
		// 		outp(port->mode, 0x54 | channel_select);
		// 		break	;
        // }

        // Send address
        outp(port->address, lo_byte);
        outp(port->address, hi_byte);

        // Send page
        outp(port->page, page);

        // Send length
        outp(port->length, transfer_length);		// lo
        outp(port->length, transfer_length >> 8);	// hi

        // enable DMA channel
        outp(channel < 4 ? 	0x0A: 0xD4, channel_select);

	    return DMA_OK;
    } else {
		return DMA_ERROR;
	}

}


int8_t SB_SetupDMABuffer( byte __far *buffer, uint16_t buffer_size) {
    int8_t dma_channel;
    int8_t dma_status;

    if (SB_MixMode & SB_SIXTEEN_BIT) {
        dma_channel = sb_dma_16;
    } else {
        dma_channel = sb_dma_8;
    }

    if (dma_channel == UNDEFINED_DMA) {
        return SB_Error;
    }

    if (DMA_SetupTransfer(dma_channel, buffer, buffer_size) == DMA_ERROR) {
        return SB_Error;
    }

    sb_dma = dma_channel;

    SB_DMABuffer = buffer;
    SB_CurrentDMABuffer = buffer;
    SB_TotalDMABufferSize = buffer_size;
    SB_DMABufferEnd = buffer + buffer_size;

    return SB_OK;
}



void SB_EnableInterrupt() {
    uint8_t irq;
    uint8_t mask;

    // Unmask system interrupt
    irq = sb_irq;
    if (irq < 8) {
        mask = inp(0x21) & ~(1 << irq);
        outp(0x21, mask);
    } else {
        mask = inp(0xA1) & ~(1 << (irq - 8));
        outp(0xA1, mask);

        mask = inp(0x21) & ~(1 << 2);
        outp(0x21, mask);
    }
}


int8_t SB_SetupPlayback(){
    uint16_t sample_rate = 11025;
	uint16_t buffer_size = TotalBufferSize;
	SB_StopPlayback();
    SB_SetMixMode();

    if (SB_SetupDMABuffer(SB_BUFFERS[0], buffer_size)){
        return SB_Error;
    }

    SB_SetPlaybackRate(sample_rate);

    SB_EnableInterrupt();

	// Turn on Speaker
    SB_WriteDSP(0xD1);

    //SB_TransferLength = MixBufferSize; 
    
    //  Program the sound card to start the transfer.
    
	// if (SB_Version < DSP_Version2xx) {
    //     SB_DSP1xx_BeginPlayback(SB_TransferLength);
    // } else if (SB_Version < DSP_Version4xx) {
    //     SB_DSP2xx_BeginPlayback(SB_TransferLength);
    // } else {
        SB_DSP4xx_BeginPlayback();
    // }

    return SB_OK;


}



int16_t SB_InitCard(){
    uint16_t sample_rate = 11025;
	int8_t status;

	//todo get these from environment variables
	sb_irq  = FIXED_SB_IRQ;
	sb_dma_8  = FIXED_SB_DMA_8;
	sb_dma_16  = FIXED_SB_DMA_16;
	sb_port = FIXED_SB_PORT;
    // Save the interrupt masks
    SB_IntController1Mask = inp(0x21);
    SB_IntController2Mask = inp(0xA1);
	status = SB_ResetBlaster();

    if (status == SB_OK) {
        SB_SetPlaybackRate(sample_rate);
        SB_SetMixMode();

        // if (SB_Config.Dma16 != UNDEFINED)
        // {
        //     status = SB_DMA_VerifyChannel(SB_Config.Dma16);
        //     if (status == DMA_Error)
        //     {
        //         return (SB_Error);
        //     }
        // }

		if (SB_DMA_VerifyChannel(sb_dma)) {
			return SB_Error;
		}

        // Install our interrupt handler
        
        if (!VALID_IRQ(sb_irq)) {
            return (SB_Error);
        }

		// todo make IRQ_TO_INTERRUPT_MAP logic handle 8 bit (single dma controller etc) machines right
        sb_int = IRQ_TO_INTERRUPT_MAP[sb_irq];
        if (sb_int == INVALID_IRQ) {
            return SB_Error;
        }

        // StackSelector = allocateTimerStack(kStackSize);
        // if (StackSelector == NULL) {
        //     return SB_Error;
        // }

        // Leave a little room at top of stack just for the hell of it...
        // StackPointer = kStackSize - sizeof(long);

        SB_OldInt = _dos_getvect(sb_int);
        if (sb_irq < 8) {
			// 8 bit logic?
			printf("\n\nSet the interrupt!\n\n");
            _dos_setvect(sb_int, SB_ServiceInterrupt);
        } else {
			// 16 bit logic?
            // status = IRQ_SetVector(Interrupt, SB_ServiceInterrupt);
            // if (status != IRQ_Ok)
            // {
            //     deallocateTimerStack(StackSelector);
            //     StackSelector = NULL;
            //     return (SB_Error);
            // }
        }

        return  SB_OK;
    }


	return status;

}


int16_t main(int16_t argc, int8_t** argv) {
	int16_t result;
	int8_t* filename;
	int8_t* sfxfilename = "DSBDOPN.lmp";
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
		fp  = fopen(sfxfilename, "rb");
		if (fp){
			uint16_t sfxfilesize;
			fseek(fp, 0, SEEK_END);
			sfxfilesize = ftell(fp);
			fseek(fp, 0, SEEK_SET);
			sfxlocation = (byte __far*) SB_BUFFERS[0];

			far_fread(sfxlocation, sfxfilesize, 1, fp);
			fclose(fp);

			pcm_samplerate = ((uint16_t __far*) sfxlocation)[1];

			sfxlength = ((uint16_t __far*) sfxlocation)[2];	// number of samples
			sfxlength_currentsample = 0;
		
			printmessage("Loaded pcm sfx %s into memory location 0x%lx successfully... %x\n", sfxfilename, sfxlocation);

			playingpcmsfx = true;

			if (SB_InitCard() == SB_OK){
				printf("\\nSB nINIT OK!!!\n");
				if (SB_SetupPlayback() == SB_OK){
					printf("\\nSB PLAYBACK OK!!!\n");

				} else {
					printf("Error B\n");
				}

			} else {
				printf("\nSB INIT Error A\n");
			}

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
			printmessage("Read instrument data!\n");
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
					switch (locallib_toupper(key & 0x00FF)) {
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

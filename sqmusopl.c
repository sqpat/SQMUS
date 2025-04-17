#include "sqcommon.h"
#include "sqmusopl.h"
#include <string.h>
#include <conio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>
#include <graph.h>
#include <i86.h>
#include <mem.h>
#include <malloc.h>
#include <io.h>
#include <dos.h>


// todo move this out?
extern OP2instrEntry AdLibInstrumentList[MAX_INSTRUMENTS_PER_TRACK];
extern uint8_t instrumentlookup[MAX_INSTRUMENTS];


uint16_t OPLport = ADLIBPORT;
uint8_t OPLchannels = OPL2CHANNELS;
uint8_t OPL3mode = 0;
extern volatile uint32_t	playingtime;

/*
 * Direct write to any OPL2/OPL3 FM synthesizer register.
 *   reg - register number (range 0x001-0x0F5 and 0x101-0x1F5). When high byte
 *         of reg is zero, data go to port OPLport, otherwise to OPLport+2
 *   data - register value to be written
 */



/* Watcom C */
uint8_t _OPL2writeReg(uint16_t port, uint16_t reg, uint8_t data);
uint8_t _OPL3writeReg(uint16_t port, uint16_t reg, uint8_t data);

 
#pragma aux _OPL2writeReg =	\
	"out	dx,al"		\
	"mov	cx,6"		\
"loop1:	 in	al,dx"		\
	"loop	loop1"		\
	"inc	dx"		\
	"mov	al,bl"		\
	"out	dx,al"		\
	"dec	dx"		\
	"mov	cx,36"		\
"loop2:	 in	al,dx"		\
	"loop	loop2"		\
	parm [DX][AX][BL]	\
	modify exact [AL CX DX] nomemory	\
	value [AL];

#pragma aux _OPL3writeReg =	\
	"or	ah,ah"		\
	"jz	bank0"		\
	"inc	dx"		\
	"inc	dx"		\
"bank0:	 out	dx,al"		\
	"in	al,dx"		\
	"mov	ah,al"		\
	"inc	dx"		\
	"mov	al,bl"		\
	"out	dx,al"		\
	parm [DX][AX][BL]	\
	modify exact [AX DX] nomemory	\
	value [AH];

// void printerfunc(uint16_t reg, uint8_t data){
//     FILE* fp = fopen("outp.txt", "ab");
//     fprintf(fp,"%04x %02x\n", reg, data);
//     fclose(fp);
// }

uint8_t OPLwriteReg(uint16_t reg, uint8_t data){
	// printerfunc(reg, data);
    if (OPL3mode){
		return _OPL3writeReg(OPLport, reg, data);
	} else{
		return _OPL2writeReg(OPLport, reg, data);
	}
}
 

uint16_t op_num[] = {
	0x000, 0x001, 0x002, 0x008, 0x009, 0x00A, 0x010, 0x011, 0x012,
	0x100, 0x101, 0x102, 0x108, 0x109, 0x10A, 0x110, 0x111, 0x112};

/*
 * Write to an operator pair. To be used for register bases of 0x20, 0x40,
 * 0x60, 0x80 and 0xE0.
 */
void OPLwriteChannel(uint8_t regbase, uint8_t channel, uint8_t data1, uint8_t data2){
    
    register uint16_t reg = regbase+op_num[channel];
    OPLwriteReg(reg, data1);
    OPLwriteReg(reg+3, data2);
}

uint16_t reg_num[] = {
	0x000, 0x001, 0x002, 0x003, 0x004, 0x005, 0x006, 0x007, 0x008,
	0x100, 0x101, 0x102, 0x103, 0x104, 0x105, 0x106, 0x107, 0x108};

/*
 * Write to channel a single value. To be used for register bases of
 * 0xA0, 0xB0 and 0xC0.
 */

void OPLwriteValue(uint8_t regbase, uint8_t channel, uint8_t value){
    
    OPLwriteReg(reg_num[channel] + regbase, value);
}

/*
 * Write frequency/octave/keyon data to a channel
 */
void OPLwriteFreq(uint8_t channel, uint16_t freq, uint8_t octave, uint8_t keyon){
    OPLwriteValue(0xA0, channel, freq & 0xFF);
    OPLwriteValue(0xB0, channel, (freq >> 8) | (octave << 2) | (keyon << 5));
}


int8_t noteVolumetable[128] = {
	  0,   1,   3,   5,   6,   8,  10,  11,
	 13,  14,  16,  17,  19,  20,  22,  23,
	 25,  26,  27,  29,  30,  32,  33,  34,
	 36,  37,  39,  41,  43,  45,  47,  49,
	 50,  52,  54,  55,  57,  59,  60,  61,
	 63,  64,  66,  67,  68,  69,  71,  72,
	 73,  74,  75,  76,  77,  79,  80,  81,
	 82,  83,  84,  84,  85,  86,  87,  88,
	 89,  90,  91,  92,  92,  93,  94,  95,
	 96,  96,  97,  98,  99,  99, 100, 101,
	101, 102, 103, 103, 104, 105, 105, 106,
	107, 107, 108, 109, 109, 110, 110, 111,
	112, 112, 113, 113, 114, 114, 115, 115,
	116, 117, 117, 118, 118, 119, 119, 120,
	120, 121, 121, 122, 122, 123, 123, 123,
	124, 124, 125, 125, 126, 126, 127, 127};





/*
 * Adjust volume value (register 0x40)
 */
int8_t OPLconvertVolume(uint8_t data, int8_t noteVolume){
	int16_t_union volumevalue;
	volumevalue.hu = FastMul8u8u(noteVolumetable[noteVolume & 0x7F], (0x3F - data));
	volumevalue.hu <<= 1;
	return 0x3F - volumevalue.bu.bytehigh;
}

int8_t OPLpanVolume(int8_t noteVolume, int8_t pan){
    if (pan >= 0){
		return noteVolume;
	} else{
		return (((int16_t)noteVolume * (pan + 64)) / 64) & 0x7F;
	}
}

/*
 * Write volume data to a channel
 */
void OPLwriteVolume(uint8_t channel, struct OPL2instrument *instr, int8_t noteVolume){
    OPLwriteChannel(0x40, channel, ((instr->feedback & 1) ?
	OPLconvertVolume(instr->level_1, noteVolume) : instr->level_1) | instr->scale_1,
	OPLconvertVolume(instr->level_2, noteVolume) | instr->scale_2);
}

/*
 * Write pan (balance) data to a channel
 */
void OPLwritePan(uint8_t channel, struct OPL2instrument *instr, int8_t pan){
    uint8_t bits;
    if (pan < -36) {
		bits = 0x10;		// left
	} else if (pan > 36){
		bits = 0x20;	// right
	} else {
		bits = 0x30;			// both
	}

    OPLwriteValue(0xC0, channel, instr->feedback | bits);
}

/*
 * Write an instrument to a channel
 *
 * Instrument layout:
 *
 *   Operator1  Operator2  Descr.
 *    data[0]    data[7]   reg. 0x20 - tremolo/vibrato/sustain/KSR/multi
 *    data[1]    data[8]   reg. 0x60 - attack rate/decay rate
 *    data[2]    data[9]   reg. 0x80 - sustain level/release rate
 *    data[3]    data[10]  reg. 0xE0 - waveform select
 *    data[4]    data[11]  reg. 0x40 - key scale level
 *    data[5]    data[12]  reg. 0x40 - output level (bottom 6 bits only)
 *          data[6]        reg. 0xC0 - feedback/AM-FM (both operators)
 */
void OPLwriteInstrument(uint8_t channel, struct OPL2instrument *instr){
    OPLwriteChannel(0x40, channel, 0x3F, 0x3F);		// no volume
    OPLwriteChannel(0x20, channel, instr->trem_vibr_1, instr->trem_vibr_2);
    OPLwriteChannel(0x60, channel, instr->att_dec_1,   instr->att_dec_2);
    OPLwriteChannel(0x80, channel, instr->sust_rel_1,  instr->sust_rel_2);
    OPLwriteChannel(0xE0, channel, instr->wave_1,      instr->wave_2);
    OPLwriteValue  (0xC0, channel, instr->feedback | 0x30);
}

/*
 * Stop all sounds
 */
void OPLshutup(void){
    uint8_t i;

    for(i = 0; i < OPLchannels; i++) {
		OPLwriteChannel(0x40, i, 0x3F, 0x3F);	// turn off volume
		OPLwriteChannel(0x60, i, 0xFF, 0xFF);	// the fastest attack, decay
		OPLwriteChannel(0x80, i, 0x0F, 0x0F);	// ... and release
		OPLwriteValue(0xB0, i, 0);		// KEY-OFF
    }
}

/*
 * Initialize hardware upon startup
 */
void OPLinit(uint16_t port, uint8_t OPL3){
    OPLport = port;
    if ( (OPL3mode = OPL3) != 0) {
		OPLchannels = OPL3CHANNELS;
		OPLwriteReg(0x105, 0x01);	// enable YMF262/OPL3 mode
		OPLwriteReg(0x104, 0x00);	// disable 4-operator mode
    } else {
		OPLchannels = OPL2CHANNELS;
	}
	OPLwriteReg(0x01, 0x20);		// enable Waveform Select
	OPLwriteReg(0x08, 0x40);		// turn off CSW mode
	OPLwriteReg(0xBD, 0x00);		// set vibrato/tremolo depth to low, set melodic mode

    OPLshutup();
}

/*
 * Deinitialize hardware before shutdown
 */
void OPLdeinit(void){
    OPLshutup();
    if (OPL3mode) {
		OPLwriteReg(0x105, 0x00);		// disable YMF262/OPL3 mode
		OPLwriteReg(0x104, 0x00);		// disable 4-operator mode
    }
    OPLwriteReg(0x01, 0x20);			// enable Waveform Select
    OPLwriteReg(0x08, 0x00);			// turn off CSW mode
    OPLwriteReg(0xBD, 0x00);			// set vibrato/tremolo depth to low, set melodic mode
}

/*
 * Detect Adlib card (OPL2)
 */
int16_t OPL2detect(uint16_t port){
    uint16_t origPort = OPLport;
    uint8_t stat1, stat2, i;

    OPLport = port;
    OPLwriteReg(0x04, 0x60);
    OPLwriteReg(0x04, 0x80);
    stat1 = inp(port) & 0xE0;
    OPLwriteReg(0x02, 0xFF);
    OPLwriteReg(0x04, 0x21);
    for (i = 255; --i;){
		inp(port);
	}
    stat2 = inp(port) & 0xE0;
    OPLwriteReg(0x04, 0x60);
    OPLwriteReg(0x04, 0x80);
    OPLport = origPort;

    return (stat1 == 0 && stat2 == 0xC0);
}

/*
 * Detect Sound Blaster Pro II (OPL3)
 *
 * Status register contents (inp(port) & 0x06):
 *   OPL2:	6
 *   OPL3:	0
 *   OPL4:	2
 */
int16_t OPL3detect(uint16_t port){
    if (!OPL2detect(port)){
		return 0;
	}

    if (inp(port) & 4){
		return 0;
	}
    return 1;
}



/* Internal variables */
struct OPLdata {
	uint8_t	channelInstr[CHANNELS];		// instrument #
	uint8_t	channelVolume[CHANNELS];	// volume
	uint8_t	channelLastVolume[CHANNELS];	// last volume
	int8_t	channelPan[CHANNELS];		// pan, 0=normal
	int8_t	channelPitch[CHANNELS];		// pitch wheel, 0=normal
	uint8_t	channelSustain[CHANNELS];	// sustain pedal value
	uint8_t	channelModulation[CHANNELS];	// modulation pot value
};

/* Driver descriptor */

struct OPLdata OPL2driverdata;

struct driverBlock OPL2driver = {
	DRV_OPL2,			// driverID
	sizeof(struct OPLdata),		// datasize
	OPLinitDriver,
	OPL2detectHardware,
	OPL2initHardware,
	OPL2deinitHardware,

	OPLplayNote,
	OPLreleaseNote,
	OPLpitchWheel,
	OPLchangeControl,
	OPLplayMusic,
	OPLstopMusic,
	OPLchangeSystemVolume
};


struct driverBlock OPL3driver = {
	DRV_OPL3,			// driverID
	sizeof(struct OPLdata),		// datasize
	OPLinitDriver,
	OPL3detectHardware,
	OPL3initHardware,
	OPL3deinitHardware,

	OPLplayNote,
	OPLreleaseNote,
	OPLpitchWheel,
	OPLchangeControl,
	OPLplayMusic,
	OPLstopMusic,
	OPLchangeSystemVolume
};




uint8_t	OPLsinglevoice = 0;


/* OPL channel (voice) data */
struct channelEntry {
	uint8_t	channel;		/* MUS channel number */
	uint8_t	note;			/* note number */
	uint8_t	flags;			/* see CH_xxx below */
	uint8_t	realnote;		/* adjusted note number */
	uint8_t	pitchwheel;		/* pitch-wheel value */
	int8_t	finetune;		/* frequency fine-tune */
	int8_t  noteVolume;		/* note volume */
	int8_t	realvolume;		/* adjusted note volume */
	struct OPL2instrument *instr;	/* current instrument */
	uint32_t time;			/* note start time */
} channels[MAXCHANNELS];

/* Flags: */
#define CH_SECONDARY	0x01
#define CH_SUSTAIN	0x02
#define CH_VIBRATO	0x04		/* set if modulation >= MOD_MIN */
#define CH_FREE		0x80

#define MOD_MIN		40		/* vibrato threshold */



// for low 7 notes
uint16_t freqtable[] = {
	345, 365, 387, 410, 435, 460, 488
};
// for the rest.
uint16_t freqtable2[] = {
	517, 547, 580, 615, 651, 690, 731, 774, 820, 869, 921, 975
};


//#define HIGHEST_NOTE 102
#define HIGHEST_NOTE 127

//todo how to calculate...?

// uint16_t pitchwheeltable2[] = {				    /* pitch wheel */
// 	 29193U,29219U,29246U,29272U,29299U,29325U,29351U,29378U,  /* -128 */
// 	 29405U,29431U,29458U,29484U,29511U,29538U,29564U,29591U,  /* -120 */
// 	 29618U,29644U,29671U,29698U,29725U,29752U,29778U,29805U,  /* -112 */
// 	 29832U,29859U,29886U,29913U,29940U,29967U,29994U,30021U,  /* -104 */
// 	 30048U,30076U,30103U,30130U,30157U,30184U,30212U,30239U,  /*  -96 */
// 	 30266U,30293U,30321U,30348U,30376U,30403U,30430U,30458U,  /*  -88 */
// 	 30485U,30513U,30541U,30568U,30596U,30623U,30651U,30679U,  /*  -80 */
// 	 30706U,30734U,30762U,30790U,30817U,30845U,30873U,30901U,  /*  -72 */
// 	 30929U,30957U,30985U,31013U,31041U,31069U,31097U,31125U,  /*  -64 */
// 	 31153U,31181U,31209U,31237U,31266U,31294U,31322U,31350U,  /*  -56 */
// 	 31379U,31407U,31435U,31464U,31492U,31521U,31549U,31578U,  /*  -48 */
// 	 31606U,31635U,31663U,31692U,31720U,31749U,31778U,31806U,  /*  -40 */
// 	 31835U,31864U,31893U,31921U,31950U,31979U,32008U,32037U,  /*  -32 */
// 	 32066U,32095U,32124U,32153U,32182U,32211U,32240U,32269U,  /*  -24 */
// 	 32298U,32327U,32357U,32386U,32415U,32444U,32474U,32503U,  /*  -16 */
// 	 32532U,32562U,32591U,32620U,32650U,32679U,32709U,32738U,  /*   -8 */
// 	 32768U,32798U,32827U,32857U,32887U,32916U,32946U,32976U,  /*    0 */
// 	 33005U,33035U,33065U,33095U,33125U,33155U,33185U,33215U,  /*    8 */
// 	 33245U,33275U,33305U,33335U,33365U,33395U,33425U,33455U,  /*   16 */
// 	 33486U,33516U,33546U,33576U,33607U,33637U,33667U,33698U,  /*   24 */
// 	 33728U,33759U,33789U,33820U,33850U,33881U,33911U,33942U,  /*   32 */
// 	 33973U,34003U,34034U,34065U,34095U,34126U,34157U,34188U,  /*   40 */
// 	 34219U,34250U,34281U,34312U,34343U,34374U,34405U,34436U,  /*   48 */
// 	 34467U,34498U,34529U,34560U,34591U,34623U,34654U,34685U,  /*   56 */
// 	 34716U,34748U,34779U,34811U,34842U,34874U,34905U,34937U,  /*   64 */
// 	 34968U,35000U,35031U,35063U,35095U,35126U,35158U,35190U,  /*   72 */
// 	 35221U,35253U,35285U,35317U,35349U,35381U,35413U,35445U,  /*   80 */
// 	 35477U,35509U,35541U,35573U,35605U,35637U,35669U,35702U,  /*   88 */
// 	 35734U,35766U,35798U,35831U,35863U,35895U,35928U,35960U,  /*   96 */
// 	 35993U,36025U,36058U,36090U,36123U,36155U,36188U,36221U,  /*  104 */
// 	 36254U,36286U,36319U,36352U,36385U,36417U,36450U,36483U,  /*  112 */
// 	 36516U,36549U,36582U,36615U,36648U,36681U,36715U,36748U}; /*  120 */
	 

// 32767 * 
int8_t pitchwheeltable[256] = { 
	14,14,14,14,14,14,14,14,14,14,13,13,13,13,13,13,
	13,13,13,12,12,12,12,12,12,12,12,12,12,11,11,11,
	11,11,11,11,11,11,10,10,10,10,10,10,10,10,10,10,
	9,9,9,9,9,9,9,9,9,8,8,8,8,8,8,8,
	8,8,7,7,7,7,7,7,7,7,7,6,6,6,6,6,
	6,6,6,6,5,5,5,5,5,5,5,5,5,4,4,4,
	4,4,4,4,4,4,3,3,3,3,3,3,3,3,3,2,
	2,2,2,2,2,2,2,2,1,1,1,1,1,1,1,1,
	0,0,0,0,0,0,0,0,0,-1,-1,-1,-1,-1,-1,-1,
	-1,-1,-2,-2,-2,-2,-2,-2,-2,-2,-3,-3,-3,-3,-3,-3,
	-3,-3,-3,-4,-4,-4,-4,-4,-4,-4,-4,-5,-5,-5,-5,-5,
	-5,-5,-5,-6,-6,-6,-6,-6,-6,-6,-6,-7,-7,-7,-7,-7,
	-7,-7,-7,-7,-8,-8,-8,-8,-8,-8,-8,-8,-9,-9,-9,-9,
	-9,-9,-9,-9,-10,-10,-10,-10,-10,-10,-10,-10,-11,-11,-11,-11,
	-11,-11,-11,-11,-12,-12,-12,-12,-12,-12,-12,-12,-13,-13,-13,-13,
	-13,-13,-13,-14,-14,-14,-14,-14,-14,-14,-14,-15,-15,-15,-15,-15
	};



void writeFrequency(uint8_t slot, uint8_t note, uint8_t pitchwheel, uint8_t keyOn){
	uint16_t freq;
    uint8_t octave;

	if (note < 7){
		freq = freqtable[note];
		octave = 0;
	} else {
		int16_t_union div_result = FastDiv16u_8u(note-7, 12);
		freq = freqtable2[div_result.b.bytehigh];
		octave = div_result.b.bytelow;
	}

    if (pitchwheel!= DEFAULT_PITCH_BEND) {
		fixed_t_union product;
		int16_t pitchshiftval = 128 - pitchwheeltable[pitchwheel];
		product.wu = FastMul16u16u(freq, pitchshiftval);
		freq = (product.productresult_mid.usemid << 1) + ((product.productresult_mid.throwawaylow & 0x80) ? 1 : 0);
		//product2.wu = FastMul16u16u(freqcopy, pitchwheeltable2[pitchwheel]);
		// need to shift 7 right... or instead:
		//freq = product2.hu.intbits << 1;

		if (freq >= 1024) {
			freq >>= 1;
			octave++;
		}
    } 
	
    if (octave > 7){
		octave = 7;
	}
    OPLwriteFreq(slot, freq, octave, keyOn);
}

void writeModulation(uint8_t slot, struct OPL2instrument *instr, uint8_t state){
    if (state){
		state = 0x40;	/* enable Frequency Vibrato */
	}
    OPLwriteChannel(0x20, slot,
	(instr->feedback & 1) ? (instr->trem_vibr_1 | state) : instr->trem_vibr_1,
	instr->trem_vibr_2 | state);
}

int8_t calcVolumeOPL(uint8_t channelVolume, uint16_t systemVolume, int8_t noteVolume){
	fixed_t_union volume_product;
	int16_t_union intermediate;
    intermediate.hu = FastMul8u8u(channelVolume, noteVolume);
	volume_product.wu = FastMul16u16u(intermediate.hu, systemVolume);
	// divide by 256...
	intermediate.bu.bytelow = volume_product.bu.fracbytehigh;
	intermediate.bu.bytehigh = volume_product.bu.intbytelow;
	// divide by 127
	intermediate = FastDiv16u_8u(intermediate.hu, 127);
	
	if (intermediate.bu.bytelow > 0x7F){
		return 0x7F;
	} else {
		return intermediate.bu.bytelow;
	}
	
}

uint8_t occupyChannel(uint8_t slot, uint8_t channel,
	uint8_t note, int8_t noteVolume, OP2instrEntry *instrument, uint8_t secondary){
    struct OPL2instrument *instr;
	int16_t pitchadder;
    struct channelEntry *ch = &channels[slot];

    //playingChannels++;

    ch->channel = channel;

    ch->note = note;
    ch->flags = secondary ? CH_SECONDARY : 0;
    if (OPL2driverdata.channelModulation[channel] >= MOD_MIN){
		ch->flags |= CH_VIBRATO;
	}

    ch->time = playingtime;

    if (noteVolume == -1){
		noteVolume = OPL2driverdata.channelLastVolume[channel];
	} else{
		OPL2driverdata.channelLastVolume[channel] = noteVolume;
	}

	ch->noteVolume = noteVolume;
    ch->realvolume = calcVolumeOPL(OPL2driverdata.channelVolume[channel], playingvolume, noteVolume);
    
	
	if (instrument->flags & FL_FIXED_PITCH){
		note = instrument->note;
	} else if (channel == PERCUSSION){
		note = 60;			// C-5
	}
	
	if (secondary && (instrument->flags & FL_DOUBLE_VOICE)){
		ch->finetune = instrument->finetune;
	} else {
		ch->finetune = DEFAULT_PITCH_BEND;
	}

    pitchadder = ch->finetune + OPL2driverdata.channelPitch[channel];
	ch->pitchwheel = pitchadder & 0xFF;
	
    if (secondary) {
		instr = &instrument->instr[1];
	} else { 
		instr = &instrument->instr[0];
	}
    ch->instr = instr;
	note += instr->basenote;
	note &= 0x7F;
	// todo divide or modulo 127?
    /*
	if ( (usenote += instr->basenote) < 0){
		while ((usenote += 12) < 0){

		}
	} else if (usenote > HIGHEST_NOTE){
		while ((usenote -= 12) > HIGHEST_NOTE){

		}
	}
	*/

    ch->realnote = note;

    OPLwriteInstrument(slot, instr);
    if (ch->flags & CH_VIBRATO){
		writeModulation(slot, instr, 1);
	}
    OPLwritePan(slot, instr, OPL2driverdata.channelPan[channel]);
    OPLwriteVolume(slot, instr, ch->realvolume);
    writeFrequency(slot, note, ch->pitchwheel, 1);
    return slot;
}

void releaseChannel(uint8_t slot, uint8_t killed){
    struct channelEntry *ch = &channels[slot];
    //playingChannels--;
    writeFrequency(slot, ch->realnote, ch->pitchwheel, 0);
    ch->channel |= CH_FREE;
    ch->flags = CH_FREE;
    if (killed) {
		OPLwriteChannel(0x80, slot, 0x0F, 0x0F);  // release rate - fastest
		OPLwriteChannel(0x40, slot, 0x3F, 0x3F);  // no volume
    }
}

void releaseSustain(uint8_t channel){
    uint8_t i;
    uint8_t id = channel;

    for(i = 0; i < OPLchannels; i++) {
		if (channels[i].channel == id && channels[i].flags & CH_SUSTAIN){
			releaseChannel(i, 0);
		}
	}
}

int8_t findFreeChannel(uint8_t flag){
    static uint8_t last = 0xFF;
    uint8_t i;
    uint8_t oldest = 0xFF;
    uint32_t oldesttime = playingtime;

    /* find free channel */
    for(i = 0; i < OPLchannels; i++) {
		if (++last == OPLchannels){	/* use cyclic `Next Fit' algorithm */
			last = 0;
		}
		if (channels[last].flags & CH_FREE){
			return last;
		}
    }

    if (flag & 1){
		return -1;			/* stop searching if bit 0 is set */
	}

    /* find some 2nd-voice channel and determine the oldest */
    for(i = 0; i < OPLchannels; i++) {
		if (channels[i].flags & CH_SECONDARY) {
			
			releaseChannel(i, -1);
			return i;
		} else
			if (channels[i].time < oldesttime) {
			oldesttime = channels[i].time;
			oldest = i;
		}
    }

    /* if possible, kill the oldest channel */
    if ( !(flag & 2) && oldest != 0xFF) {
		releaseChannel(oldest, -1);
		return oldest;
    }

    /* can't find any free channel */
    return -1;
}

extern 	int16_t opl_valid_instruments;


OP2instrEntry *getInstrument(uint8_t channel, uint8_t note) {
    uint8_t instrnumber;
    uint8_t instrindex;

    if (playingpercussMask & (1 << channel)) {
		if (note < 35 || note > 81){
			return NULL;		/* wrong percussion number */
		}
		instrnumber = note + (128-35);
    } else { 
		instrnumber = OPL2driverdata.channelInstr[channel];
	}

	if (instrnumber > 174){
		printmessage("\nBad OPL instrument %i!", instrnumber);
		return NULL;
	} else {
		// printmessage("\nPlaying %03i %03i!", instrumentlookup[instrnumber], instrnumber);

	}

	instrindex = instrumentlookup[instrnumber];

	if (instrindex == 0xFF){
		printerror("Bad instrument index %i %i!!\n", instrnumber, instrindex);
		return NULL;
	}
	return &AdLibInstrumentList[instrindex];
}


// code 1: play note
void OPLplayNote(uint8_t channel, uint8_t note, int8_t noteVolume){
    int8_t i;
    OP2instrEntry *instr = getInstrument(channel, note);

    if (instr == NULL){
		printerror( "null instrument? %i %i\n", channel, note);
		return;
	}


    if ( (i = findFreeChannel((channel == PERCUSSION) ? 2 : 0)) != -1) {
		occupyChannel(i, channel, note, noteVolume, instr, 0);
		if (!OPLsinglevoice && instr->flags == FL_DOUBLE_VOICE) {
			if ( (i = findFreeChannel((channel == PERCUSSION) ? 3 : 1)) != -1){
				occupyChannel(i, channel, note, noteVolume, instr, 1);
			}
		}
    } else {
		printmessage("no voice found!\n");
	}
}

// code 0: release note
void OPLreleaseNote(uint8_t channel, uint8_t note){
    uint8_t i;
    uint8_t id = channel;

    uint8_t sustain = OPL2driverdata.channelSustain[channel];

    for(i = 0; i < OPLchannels; i++){
		if (channels[i].channel == id && channels[i].note == note) {
			if (sustain < 0x40){
				releaseChannel(i, 0);
			} else {
				channels[i].flags |= CH_SUSTAIN;
			}
		}
	}
}

// code 2: change pitch wheel (bender)
void OPLpitchWheel(uint8_t channel, uint8_t pitch){
    uint8_t i;
    uint8_t id = channel;

	pitch -= DEFAULT_PITCH_BEND;
    OPL2driverdata.channelPitch[channel] = pitch;
    for(i = 0; i < OPLchannels; i++) {
		struct channelEntry *ch = &channels[i];
		if (ch->channel == id) {
			int16_t pitchadder;
			ch->time = playingtime;
			pitchadder = (int16_t)ch->finetune + pitch;
			ch->pitchwheel = (pitchadder & 0xFF);
			writeFrequency(i, ch->realnote, ch->pitchwheel, 1);
		}
    }
}

// code 4: change control
void OPLchangeControl(uint8_t channel, uint8_t controller, uint8_t value){
    uint8_t i;
    uint8_t id = channel;

    switch (controller) {
		case 0:			/* change instrument */
			OPL2driverdata.channelInstr[channel] = value;
			// printf("\nchange instrument %i %i", channel, value);
			break;
		case 2:
			OPL2driverdata.channelModulation[channel] = value;
			for(i = 0; i < OPLchannels; i++) {
				struct channelEntry *ch = &channels[i];
				if (ch->channel == id) {
					uint8_t flags = ch->flags;
					ch->time = playingtime;
					if (value >= MOD_MIN) {
						ch->flags |= CH_VIBRATO;
						if (ch->flags != flags){
							writeModulation(i, ch->instr, 1);
						}
					} else {
						ch->flags &= ~CH_VIBRATO;
						if (ch->flags != flags){
							writeModulation(i, ch->instr, 0);
						}
					}
				}
			}
			break;
		case 3:		/* change volume */
			OPL2driverdata.channelVolume[channel] = value;
			for(i = 0; i < OPLchannels; i++) {
				struct channelEntry *ch = &channels[i];
				if (ch->channel == id) {
					ch->time = playingtime;
					ch->realvolume = calcVolumeOPL(value, playingvolume, ch->noteVolume);
					OPLwriteVolume(i, ch->instr, ch->realvolume);
				}
			}
			break;
		case 4:			/* change pan (balance) */
			OPL2driverdata.channelPan[channel] = value -= 64;
			for(i = 0; i < OPLchannels; i++) {
				struct channelEntry *ch = &channels[i];
				if (ch->channel == id) {
					ch->time = playingtime;
					OPLwritePan(i, ch->instr, value);
				}
			}
			break;
		case 8:		/* change sustain pedal (hold) */
			OPL2driverdata.channelSustain[channel] = value;
			if (value < 0x40){
				releaseSustain(channel);
			}
			break;
    }
}


void OPLplayMusic(){
    uint8_t i;

    for (i = 0; i < CHANNELS; i++) {
		OPL2driverdata.channelVolume[i] = 127;	/* default volume 127 (full volume) */
		OPL2driverdata.channelSustain[i] = OPL2driverdata.channelLastVolume[i] = 0;
    }
}

void OPLstopMusic(){
    uint8_t i;
    for(i = 0; i < OPLchannels; i++){
		if (!(channels[i].flags & CH_FREE)){
			releaseChannel(i, -1);
		}
	}
}

void OPLchangeSystemVolume(int16_t systemVolume){
    uint8_t *channelVolume = OPL2driverdata.channelVolume;
    uint8_t i;
    for(i = 0; i < OPLchannels; i++) {
		struct channelEntry *ch = &channels[i];
		ch->realvolume = calcVolumeOPL(channelVolume[ch->channel & 0xF], systemVolume, ch->noteVolume);
		if (playingstate == ST_PLAYING){
			OPLwriteVolume(i, ch->instr, ch->realvolume);
		}
    }
}
 

int8_t OPLinitDriver(void){
    int8_t i;
	memset(channels, 0xFF, sizeof channels);
    for(i = 0; i < OPLchannels; i++) {
		struct channelEntry *ch = &channels[i];
		ch->pitchwheel = DEFAULT_PITCH_BEND;
	}
    //OPLinstruments = NULL;
    return 0;
}
 
 

int8_t OPL2detectHardware(uint16_t port, uint8_t irq, uint8_t dma){
    return OPL2detect(port);
}

int8_t OPL3detectHardware(uint16_t port, uint8_t irq, uint8_t dma){
    return OPL3detect(port);
}

int8_t OPL2initHardware(uint16_t port, uint8_t irq, uint8_t dma){
    OPLinit(port, 0);
    return 0;
}

int8_t OPL3initHardware(uint16_t port, uint8_t irq, uint8_t dma){
    OPLinit(port, 1);
    return 0;
}

int8_t OPL2deinitHardware(void){
    OPLdeinit();
    return 0;
}

int8_t OPL3deinitHardware(void){
    OPLdeinit();
    return 0;
}


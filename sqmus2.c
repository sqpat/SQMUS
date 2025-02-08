#include "sqmus2.h"
#include "test.h"
#include <string.h>
#include <conio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>
#include <graph.h>

#include <i86.h>



/*
 *	Name:		OPL2/OPL3 Music driver
 *	Project:	MUS File Player Library
 *	Version:	1.67
 *	Author:		Vladimir Arnost (QA-Software)
 *	Last revision:	May-2-1996
 *	Compiler:	Borland C++ 3.1, Watcom C/C++ 10.0
 *
 */

/*
 * Revision History:
 *
 *	Aug-8-1994	V1.00	V.Arnost
 *		Written from scratch
 *	Aug-9-1994	V1.10	V.Arnost
 *		Some minor changes to improve sound quality. Tried to add
 *		stereo sound capabilities, but failed to -- my SB Pro refuses
 *		to switch to stereo mode.
 *	Aug-13-1994	V1.20	V.Arnost
 *		Stereo sound fixed. Now works also with Sound Blaster Pro II
 *		(chip OPL3 -- gives 18 "stereo" (ahem) channels).
 *		Changed code to handle properly notes without volume.
 *		(Uses previous volume on given channel.)
 *		Added cyclic channel usage to avoid annoying clicking noise.
 *	Aug-28-1994	V1.40	V.Arnost
 *		Added Adlib and SB Pro II detection.
 *	Apr-16-1995	V1.60	V.Arnost
 *		Moved into separate source file MUSLIB.C
 *	Jul-12-1995	V1.62	V.Arnost
 *		Module created and source code copied from MUSLIB.C
 *	Aug-08-1995	V1.63	V.Arnost
 *		Modified to follow changes in MLOPL_IO.C
 *	Aug-13-1995	V1.64	V.Arnost
 *		Added OPLsendMIDI() function
 *	Sep-8-1995	V1.65	V.Arnost
 *		Added sustain pedal support.
 *		Improved pause/unpause functions. Now all notes are made
 *		silent (even released notes, which haven't sounded off yet).
 *	Mar-1-1996	V1.66	V.Arnost
 *		Cleaned up the source
 *	May-2-1996	V1.67	V.Arnost
 *		Added modulation wheel (vibrato) support
 */


#include <mem.h>
#include <malloc.h>
#include <stdio.h>
#include <io.h>



/*
 *	Name:		Low-level OPL2/OPL3 I/O interface
 *	Project:	MUS File Player Library
 *	Version:	1.64
 *	Author:		Vladimir Arnost (QA-Software)
 *	Last revision:	Mar-1-1996
 *	Compiler:	Borland C++ 3.1, Watcom C/C++ 10.0
 *
 */

/*
 * Revision History:
 *
 *	Aug-8-1994	V1.00	V.Arnost
 *		Written from scratch
 *	Aug-9-1994	V1.10	V.Arnost
 *		Added stereo capabilities
 *	Aug-13-1994	V1.20	V.Arnost
 *		Stereo capabilities made functional
 *	Aug-24-1994	V1.30	V.Arnost
 *		Added Adlib and SB Pro II detection
 *	Oct-30-1994	V1.40	V.Arnost
 *		Added BLASTER variable parsing
 *	Apr-14-1995	V1.50	V.Arnost
 *              Some declarations moved from adlib.h to deftypes.h
 *	Jul-22-1995	V1.60	V.Arnost
 *		Ported to Watcom C
 *		Simplified WriteChannel() and WriteValue()
 *	Jul-24-1995	V1.61	V.Arnost
 *		DetectBlaster() moved to MLMISC.C
 *	Aug-8-1995	V1.62	V.Arnost
 *		Module renamed to MLOPL_IO.C and functions renamed to OPLxxx
 *		Mixer-related functions moved to module MLSBMIX.C
 *	Sep-8-1995	V1.63	V.Arnost
 *		OPLwriteReg() routine sped up on OPL3 cards
 *	Mar-1-1996	V1.64	V.Arnost
 *		Cleaned up the source
 */

#include <dos.h>
#include <conio.h>
#include <stdio.h>

#define MAX_INSTRUMENTS 175


extern struct OP2instrEntry AdLibInstrumentList[MAX_INSTRUMENTS];

uint OPLport = ADLIBPORT;
uint OPLchannels = OPL2CHANNELS;
uint OPL3mode = 0;
extern volatile ulong	MLtime;

/*
 * Direct write to any OPL2/OPL3 FM synthesizer register.
 *   reg - register number (range 0x001-0x0F5 and 0x101-0x1F5). When high byte
 *         of reg is zero, data go to port OPLport, otherwise to OPLport+2
 *   data - register value to be written
 */


#ifdef __WATCOMC__

/* Watcom C */
uchar _OPL2writeReg(uint port, uint reg, uchar data);
uchar _OPL3writeReg(uint port, uint reg, uchar data);

#ifdef __386__

#pragma aux _OPL2writeReg =	\
	"out	dx,al"		\
	"mov	ecx,6"		\
"loop1:	 in	al,dx"		\
	"loop	loop1"		\
	"inc	edx"		\
	"mov	al,bl"		\
	"out	dx,al"		\
	"dec	edx"		\
	"mov	ecx,36"		\
"loop2:	 in	al,dx"		\
	"loop	loop2"		\
	parm [EDX][EAX][BL]	\
	modify exact [AL ECX EDX] nomemory	\
	value [AL];

#pragma aux _OPL3writeReg =	\
	"or	ah,ah"		\
	"jz	bank0"		\
	"inc	edx"		\
	"inc	edx"		\
"bank0:	 out	dx,al"		\
	"in	al,dx"		\
	"mov	ah,al"		\
	"inc	edx"		\
	"mov	al,bl"		\
	"out	dx,al"		\
	parm [EDX][EAX][BL]	\
	modify exact [AX EDX] nomemory	\
	value [AH];
#else /* !__386__ */
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
#endif

uint OPLwriteReg(uint reg, uchar data)
{
	// FILE* fp = fopen ("log1.txt", "ab");
	//  fprintf(fp, "outp %03x, %02x outp %03x, %02x\n", OPLport, reg, OPLport+1, data);
	// fclose(fp);


    if (OPL3mode)
	return _OPL3writeReg(OPLport, reg, data);
    else
	return _OPL2writeReg(OPLport, reg, data);
}

#else

/* Borland C */
uint OPLwriteReg(uint reg, uchar data)
{


#define I asm
    if (OPL3mode)		/* OPL3 mode: no delay loops */
    {
I	mov	dx,OPLport
I	mov	ax,reg
I	or	ah,ah
I	jz	bank0
I	inc	dx
I	inc	dx
bank0:
I	out	dx,al
I	in	al,dx		/* short delay */
I	mov	ah,al
I	inc	dx
I	mov	al,data
I	out	dx,al
I	mov	al,ah
    } else {			/* OPL2 mode: with delays, first bank only */
I	mov	dx,OPLport
I	mov	ax,reg
I	out	dx,al
I	mov	cx,6
loop1:
I	in	al,dx
I	loop	loop1

I	inc	dx
I	mov	al,data
I	out	dx,al
I	dec	dx
I	mov	cx,36
loop2:
I	in	al,dx
I	loop	loop2
    }
    return _AL;
}
#endif

/*
 * Write to an operator pair. To be used for register bases of 0x20, 0x40,
 * 0x60, 0x80 and 0xE0.
 */
void OPLwriteChannel(uint regbase, uint channel, uchar data1, uchar data2)
{
    static uint op_num[] = {
	0x000, 0x001, 0x002, 0x008, 0x009, 0x00A, 0x010, 0x011, 0x012,
	0x100, 0x101, 0x102, 0x108, 0x109, 0x10A, 0x110, 0x111, 0x112};

    register uint reg = regbase+op_num[channel];
    OPLwriteReg(reg, data1);
    OPLwriteReg(reg+3, data2);
}

/*
 * Write to channel a single value. To be used for register bases of
 * 0xA0, 0xB0 and 0xC0.
 */
void OPLwriteValue(uint regbase, uint channel, uchar value)
{
    static uint reg_num[] = {
	0x000, 0x001, 0x002, 0x003, 0x004, 0x005, 0x006, 0x007, 0x008,
	0x100, 0x101, 0x102, 0x103, 0x104, 0x105, 0x106, 0x107, 0x108};

    OPLwriteReg(regbase + reg_num[channel], value);
}

/*
 * Write frequency/octave/keyon data to a channel
 */
void OPLwriteFreq(uint channel, uint freq, uint octave, uint keyon)
{
    OPLwriteValue(0xA0, channel, (BYTE)freq);
    OPLwriteValue(0xB0, channel, (freq >> 8) | (octave << 2) | (keyon << 5));
}

/*
 * Adjust volume value (register 0x40)
 */
uint OPLconvertVolume(uint data, uint volume)
{
    static uchar volumetable[128] = {
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

#if 0
    uint n;

    if (volume > 127)
	volume = 127;
    n = 0x3F - (data & 0x3F);
    n = (n * (uint)volumetable[volume]) >> 7;
    return (0x3F - n) | (data & 0xC0);
#else
    return 0x3F - (((0x3F - data) *
	(uint)volumetable[volume <= 127 ? volume : 127]) >> 7);
#endif
}

uint OPLpanVolume(uint volume, int pan)
{
    if (pan >= 0)
	return volume;
    else
	return (volume * (pan + 64)) / 64;
}

/*
 * Write volume data to a channel
 */
void OPLwriteVolume(uint channel, struct OPL2instrument *instr, uint volume)
{
    OPLwriteChannel(0x40, channel, ((instr->feedback & 1) ?
	OPLconvertVolume(instr->level_1, volume) : instr->level_1) | instr->scale_1,
	OPLconvertVolume(instr->level_2, volume) | instr->scale_2);
}

/*
 * Write pan (balance) data to a channel
 */
void OPLwritePan(uint channel, struct OPL2instrument *instr, int pan)
{
    uchar bits;
    if (pan < -36) bits = 0x10;		// left
    else if (pan > 36) bits = 0x20;	// right
    else bits = 0x30;			// both

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
void OPLwriteInstrument(uint channel, struct OPL2instrument *instr)
{
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
void OPLshutup(void)
{
    uint i;

    for(i = 0; i < OPLchannels; i++)
    {
	OPLwriteChannel(0x40, i, 0x3F, 0x3F);	// turn off volume
	OPLwriteChannel(0x60, i, 0xFF, 0xFF);	// the fastest attack, decay
	OPLwriteChannel(0x80, i, 0x0F, 0x0F);	// ... and release
	OPLwriteValue(0xB0, i, 0);		// KEY-OFF
    }
}

/*
 * Initialize hardware upon startup
 */
void OPLinit(uint port, uint OPL3)
{
    OPLport = port;
    if ( (OPL3mode = OPL3) != 0)
    {
	OPLchannels = OPL3CHANNELS;
	OPLwriteReg(0x105, 0x01);	// enable YMF262/OPL3 mode
	OPLwriteReg(0x104, 0x00);	// disable 4-operator mode
    } else
	OPLchannels = OPL2CHANNELS;
    OPLwriteReg(0x01, 0x20);		// enable Waveform Select
    OPLwriteReg(0x08, 0x40);		// turn off CSW mode
    OPLwriteReg(0xBD, 0x00);		// set vibrato/tremolo depth to low, set melodic mode

    OPLshutup();
}

/*
 * Deinitialize hardware before shutdown
 */
void OPLdeinit(void)
{
    OPLshutup();
    if (OPL3mode)
    {
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
int OPL2detect(uint port)
{
    uint origPort = OPLport;
    uint stat1, stat2, i;

    OPLport = port;
    OPLwriteReg(0x04, 0x60);
    OPLwriteReg(0x04, 0x80);
    stat1 = inp(port) & 0xE0;
    OPLwriteReg(0x02, 0xFF);
    OPLwriteReg(0x04, 0x21);
    for (i = 512; --i; )
	inp(port);
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
int OPL3detect(uint port)
{
    if (!OPL2detect(port))
	return 0;
#if 0
    if (!SBdetectMixer(port))
	return 0;
    if (OPL2detect(port + 2))
	return 0;
#else
    if (inp(port) & 4)
	return 0;
#endif
    return 1;
}



/* Internal variables */
struct OPLdata {
	uint	channelInstr[CHANNELS];		// instrument #
	uchar	channelVolume[CHANNELS];	// volume
	uchar	channelLastVolume[CHANNELS];	// last volume
	schar	channelPan[CHANNELS];		// pan, 0=normal
	schar	channelPitch[CHANNELS];		// pitch wheel, 0=normal
	uchar	channelSustain[CHANNELS];	// sustain pedal value
	uchar	channelModulation[CHANNELS];	// modulation pot value
};

/* Driver descriptor */
static char OPL2name[] = "OPL2 FM";

struct OPLdata OPL2driverdata;

struct driverBlock OPL2driver = {
	NULL,				// next
	DRV_OPL2,			// driverID
	OPL2name,			// name
	sizeof(struct OPLdata),		// datasize
	OPLinitDriver,
	OPLdeinitDriver,
	OPLdriverParam,
	OPLloadBank,
	OPL2detectHardware,
	OPL2initHardware,
	OPL2deinitHardware,

	OPLplayNote,
	OPLreleaseNote,
	OPLpitchWheel,
	OPLchangeControl,
	OPLplayMusic,
	OPLstopMusic,
	OPLchangeVolume,
	OPLpauseMusic,
	OPLunpauseMusic,
	OPLsendMIDI};


//struct musicBlock *mainmusicblock[MAXMUSBLOCK] = {NULL};
//struct driverBlock *MLdriverList = &DUMMYdriver;


uint	playingstate = ST_PLAYING;			
uint	playingvolume = 256;
uint	playingloopcount;
uint	playingchannelMask = 0xFFFF;
uint	playingpercussMask = 1 << PERCUSSION;

ulong	playingtime;
ulong	playingticks;
uint	playingplayingcount;
uint	playingchannelcount;


struct driverBlock	*playingdriver = &OPL2driver;

/*

static char OPL3name[] = "OPL3 FM";

struct driverBlock OPL3driver = {
	NULL,				// next
	DRV_OPL3,			// driverID
	OPL3name,			// name
	sizeof(struct OPLdata),		// datasize
	OPLinitDriver,
	OPLdeinitDriver,
	OPLdriverParam,
	OPLloadBank,
	OPL3detectHardware,
	OPL3initHardware,
	OPL3deinitHardware,

	OPLplayNote,
	OPLreleaseNote,
	OPLpitchWheel,
	OPLchangeControl,
	OPLplayMusic,
	OPLstopMusic,
	OPLchangeVolume,
	OPLpauseMusic,
	OPLunpauseMusic};
*/

static uint	OPLsinglevoice = 0;


/* OPL channel (voice) data */
static struct channelEntry {
	uchar	channel;		/* MUS channel number */
	uchar	note;			/* note number */
	uchar	flags;			/* see CH_xxx below */
	uchar	realnote;		/* adjusted note number */
	schar	finetune;		/* frequency fine-tune */
	sint	pitch;			/* pitch-wheel value */
	uint	volume;			/* note volume */
	uint	realvolume;		/* adjusted note volume */
	struct OPL2instrument *instr;	/* current instrument */
	ulong	time;			/* note start time */
} channels[MAXCHANNELS];

/* Flags: */
#define CH_SECONDARY	0x01
#define CH_SUSTAIN	0x02
#define CH_VIBRATO	0x04		/* set if modulation >= MOD_MIN */
#define CH_FREE		0x80

#define MOD_MIN		40		/* vibrato threshold */

#define CHANNEL_ID(ch) (*(uint8_t *)&(ch))

#ifdef __WATCOMC__
  WORD MAKE_ID(uchar ch, uchar mus);
  #pragma aux MAKE_ID = parm[DL][DH] value[DX] modify exact [] nomemory;
#else
  #define MAKE_ID(ch, mus) (_AL=(uchar)(ch), _AH=(uchar)(mus), _AX)
#endif


static WORD freqtable[] = {					 /* note # */
	345, 365, 387, 410, 435, 460, 488, 517, 547, 580, 615, 651,  /*  0 */
	690, 731, 774, 820, 869, 921, 975, 517, 547, 580, 615, 651,  /* 12 */
	690, 731, 774, 820, 869, 921, 975, 517, 547, 580, 615, 651,  /* 24 */
	690, 731, 774, 820, 869, 921, 975, 517, 547, 580, 615, 651,  /* 36 */
	690, 731, 774, 820, 869, 921, 975, 517, 547, 580, 615, 651,  /* 48 */
	690, 731, 774, 820, 869, 921, 975, 517, 547, 580, 615, 651,  /* 60 */
	690, 731, 774, 820, 869, 921, 975, 517, 547, 580, 615, 651,  /* 72 */
	690, 731, 774, 820, 869, 921, 975, 517, 547, 580, 615, 651,  /* 84 */
	690, 731, 774, 820, 869, 921, 975, 517, 547, 580, 615, 651,  /* 96 */
	690, 731, 774, 820, 869, 921, 975, 517, 547, 580, 615, 651, /* 108 */
	690, 731, 774, 820, 869, 921, 975, 517};		    /* 120 */

static BYTE octavetable[] = {					 /* note # */
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,			     /*  0 */
	0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1,			     /* 12 */
	1, 1, 1, 1, 1, 1, 1, 2, 2, 2, 2, 2,			     /* 24 */
	2, 2, 2, 2, 2, 2, 2, 3, 3, 3, 3, 3,			     /* 36 */
	3, 3, 3, 3, 3, 3, 3, 4, 4, 4, 4, 4,			     /* 48 */
	4, 4, 4, 4, 4, 4, 4, 5, 5, 5, 5, 5,			     /* 60 */
	5, 5, 5, 5, 5, 5, 5, 6, 6, 6, 6, 6,			     /* 72 */
	6, 6, 6, 6, 6, 6, 6, 7, 7, 7, 7, 7,			     /* 84 */
	7, 7, 7, 7, 7, 7, 7, 8, 8, 8, 8, 8,			     /* 96 */
	8, 8, 8, 8, 8, 8, 8, 9, 9, 9, 9, 9,			    /* 108 */
	9, 9, 9, 9, 9, 9, 9,10};				    /* 120 */

//#define HIGHEST_NOTE 102
#define HIGHEST_NOTE 127

static WORD pitchtable[] = {				    /* pitch wheel */
	 29193U,29219U,29246U,29272U,29299U,29325U,29351U,29378U,  /* -128 */
	 29405U,29431U,29458U,29484U,29511U,29538U,29564U,29591U,  /* -120 */
	 29618U,29644U,29671U,29698U,29725U,29752U,29778U,29805U,  /* -112 */
	 29832U,29859U,29886U,29913U,29940U,29967U,29994U,30021U,  /* -104 */
	 30048U,30076U,30103U,30130U,30157U,30184U,30212U,30239U,  /*  -96 */
	 30266U,30293U,30321U,30348U,30376U,30403U,30430U,30458U,  /*  -88 */
	 30485U,30513U,30541U,30568U,30596U,30623U,30651U,30679U,  /*  -80 */
	 30706U,30734U,30762U,30790U,30817U,30845U,30873U,30901U,  /*  -72 */
	 30929U,30957U,30985U,31013U,31041U,31069U,31097U,31125U,  /*  -64 */
	 31153U,31181U,31209U,31237U,31266U,31294U,31322U,31350U,  /*  -56 */
	 31379U,31407U,31435U,31464U,31492U,31521U,31549U,31578U,  /*  -48 */
	 31606U,31635U,31663U,31692U,31720U,31749U,31778U,31806U,  /*  -40 */
	 31835U,31864U,31893U,31921U,31950U,31979U,32008U,32037U,  /*  -32 */
	 32066U,32095U,32124U,32153U,32182U,32211U,32240U,32269U,  /*  -24 */
	 32298U,32327U,32357U,32386U,32415U,32444U,32474U,32503U,  /*  -16 */
	 32532U,32562U,32591U,32620U,32650U,32679U,32709U,32738U,  /*   -8 */
	 32768U,32798U,32827U,32857U,32887U,32916U,32946U,32976U,  /*    0 */
	 33005U,33035U,33065U,33095U,33125U,33155U,33185U,33215U,  /*    8 */
	 33245U,33275U,33305U,33335U,33365U,33395U,33425U,33455U,  /*   16 */
	 33486U,33516U,33546U,33576U,33607U,33637U,33667U,33698U,  /*   24 */
	 33728U,33759U,33789U,33820U,33850U,33881U,33911U,33942U,  /*   32 */
	 33973U,34003U,34034U,34065U,34095U,34126U,34157U,34188U,  /*   40 */
	 34219U,34250U,34281U,34312U,34343U,34374U,34405U,34436U,  /*   48 */
	 34467U,34498U,34529U,34560U,34591U,34623U,34654U,34685U,  /*   56 */
	 34716U,34748U,34779U,34811U,34842U,34874U,34905U,34937U,  /*   64 */
	 34968U,35000U,35031U,35063U,35095U,35126U,35158U,35190U,  /*   72 */
	 35221U,35253U,35285U,35317U,35349U,35381U,35413U,35445U,  /*   80 */
	 35477U,35509U,35541U,35573U,35605U,35637U,35669U,35702U,  /*   88 */
	 35734U,35766U,35798U,35831U,35863U,35895U,35928U,35960U,  /*   96 */
	 35993U,36025U,36058U,36090U,36123U,36155U,36188U,36221U,  /*  104 */
	 36254U,36286U,36319U,36352U,36385U,36417U,36450U,36483U,  /*  112 */
	 36516U,36549U,36582U,36615U,36648U,36681U,36715U,36748U}; /*  120 */


static void writeFrequency(uint slot, uint note, int pitch, uint keyOn)
{
    uint freq = freqtable[note];
    uint octave = octavetable[note];

    if (pitch)
    {
#ifdef DEBUG
	printf("DEBUG: pitch: N: %d  P: %d\n", note, pitch);
#endif
	if (pitch > 127) pitch = 127;
	else if (pitch < -128) pitch = -128;
	freq = ((ulong)freq * pitchtable[pitch + 128]) >> 15;
	if (freq >= 1024)
	{
	    freq >>= 1;
	    octave++;
	}
    }
    if (octave > 7)
	octave = 7;
    OPLwriteFreq(slot, freq, octave, keyOn);
}

static void writeModulation(uint slot, struct OPL2instrument *instr, int state)
{
    if (state)
	state = 0x40;	/* enable Frequency Vibrato */
    OPLwriteChannel(0x20, slot,
	(instr->feedback & 1) ? (instr->trem_vibr_1 | state) : instr->trem_vibr_1,
	instr->trem_vibr_2 | state);
}

static uint calcVolume(uint channelVolume, uint MUSvolume, uint noteVolume)
{
    noteVolume = ((ulong)channelVolume * MUSvolume * noteVolume) / (256*127);
    if (noteVolume > 127)
	return 127;
    else
	return noteVolume;
}

static int occupyChannel(uint slot, uint channel,
	int note, int volume, struct OP2instrEntry *instrument, uchar secondary)
{
    struct OPL2instrument *instr;

    struct channelEntry *ch = &channels[slot];

    //playingChannels++;

    ch->channel = channel;

    ch->note = note;
    ch->flags = secondary ? CH_SECONDARY : 0;
    if (OPL2driverdata.channelModulation[channel] >= MOD_MIN){
		ch->flags |= CH_VIBRATO;
	}
    ch->time = MLtime;
    if (volume == -1){
		volume = OPL2driverdata.channelLastVolume[channel];
	} else{
		OPL2driverdata.channelLastVolume[channel] = volume;
	}
    ch->realvolume = calcVolume(OPL2driverdata.channelVolume[channel], playingvolume, ch->volume = volume);
    
	
	if (instrument->flags & FL_FIXED_PITCH){
		note = instrument->note;
	} else if (channel == PERCUSSION){
		note = 60;			// C-5
	}
	if (secondary && (instrument->flags & FL_DOUBLE_VOICE)){
	ch->finetune = instrument->finetune - 0x80;
	} else {
	ch->finetune = 0;
	}
    ch->pitch = ch->finetune + OPL2driverdata.channelPitch[channel];
    if (secondary) {
	instr = &instrument->instr[1];
	} else { 
		instr = &instrument->instr[0];
	}
    ch->instr = instr;
    if ( (note += instr->basenote) < 0)
	while ((note += 12) < 0);
    else if (note > HIGHEST_NOTE)
	while ((note -= 12) > HIGHEST_NOTE);
    ch->realnote = note;

    OPLwriteInstrument(slot, instr);
    if (ch->flags & CH_VIBRATO)
	writeModulation(slot, instr, 1);
    OPLwritePan(slot, instr, OPL2driverdata.channelPan[channel]);
    OPLwriteVolume(slot, instr, ch->realvolume);
    writeFrequency(slot, note, ch->pitch, 1);
    return slot;
}

#pragma argsused
static int releaseChannel(uint slot, uint killed)
{
    struct channelEntry *ch = &channels[slot];
#ifdef DEBUG
    printf("\nDEBUG: Release  Ch: %d  Adl: %d  %04X\n", channel, slot, Adlibchannel[slot]);
#endif
    //playingChannels--;
    writeFrequency(slot, ch->realnote, ch->pitch, 0);
    ch->channel |= CH_FREE;
    ch->flags = CH_FREE;
    if (killed)
    {
	OPLwriteChannel(0x80, slot, 0x0F, 0x0F);  // release rate - fastest
	OPLwriteChannel(0x40, slot, 0x3F, 0x3F);  // no volume
    }
    return slot;
}

static int releaseSustain(uint channel)
{
    uint i;
    uint8_t id = channel;

    for(i = 0; i < OPLchannels; i++)
    {
	if (CHANNEL_ID(channels[i]) == id && channels[i].flags & CH_SUSTAIN)
	    releaseChannel(i, 0);
    }
    return 0;
}

static int findFreeChannel(uint flag)
{
    static uint last = -1U;
    uint i;
    uint oldest = -1U;
    ulong oldesttime = MLtime;

    /* find free channel */
    for(i = 0; i < OPLchannels; i++) {
		//printf ("check %i ", i);
		if (++last == OPLchannels){	/* use cyclic `Next Fit' algorithm */
			last = 0;
		}
		if (channels[last].flags & CH_FREE){
		//printf("return a %i", last);
			return last;
		}
    }

    if (flag & 1){
		return -1;			/* stop searching if bit 0 is set */
	}

    /* find some 2nd-voice channel and determine the oldest */
    for(i = 0; i < OPLchannels; i++) {
		if (channels[i].flags & CH_SECONDARY) {
			#ifdef DEBUG
					printf("\nDEBUG: Kill 2nd %04X\n", Adlibchannel[i]);
			#endif
			releaseChannel(i, -1);
			return i;
		} else
			if (channels[i].time < oldesttime) {
			oldesttime = channels[i].time;
			oldest = i;
		}
    }

    /* if possible, kill the oldest channel */
    if ( !(flag & 2) && oldest != -1U)
    {
#ifdef DEBUG
	printf("DEBUG: Kill %04X !!!\n", Adlibchannel[oldest]);
#endif
	releaseChannel(oldest, -1);
	return oldest;
    }

    /* can't find any free channel */
#ifdef DEBUG
    printf("DEBUG: Full!!!\n");
#endif
    return -1;
}

static struct OP2instrEntry *getInstrument(uint channel, uchar note)
{
    uint instrnumber;

    if (playingpercussMask & (1 << channel))
    {
	if (note < 35 || note > 81)
	    return NULL;		/* wrong percussion number */
	instrnumber = note + (128-35);
    } else
	instrnumber = OPL2driverdata.channelInstr[channel];

	return &AdLibInstrumentList[instrnumber];
}


// code 1: play note
void OPLplayNote(uint channel, uchar note, int volume)
{
    int i;
    struct OP2instrEntry *instr;


	//printf("\nvals %i %i %i", channel, note, volume);

    if ( (instr = getInstrument(channel, note)) == NULL ){
		printf( "PLAY NOTE caught? %i %i", channel, note);
		return;
	}
#ifdef DEBUG
    cprintf("\rDEBUG: play: Ch: %d  N: %d  V: %d (%d)  I: %d (%s)  Fi: %d\r\n",
	channel, note, volume, OPL2driver.channelVolume[channel], instrnumber,
	instrumentName[instrnumber], (instr->flags & FL_DOUBLE_VOICE) ?
	(instr->finetune - 0x80) : 0);
#endif

    if ( (i = findFreeChannel((channel == PERCUSSION) ? 2 : 0)) != -1) {

		// FILE* fp = fopen ("log1.txt", "ab");
		// fclose(fp);

		occupyChannel(i, channel, note, volume, instr, 0);
		if (!OPLsinglevoice && instr->flags == FL_DOUBLE_VOICE) {
			if ( (i = findFreeChannel((channel == PERCUSSION) ? 3 : 1)) != -1){
				occupyChannel(i, channel, note, volume, instr, 1);
			}
		}
    } else {
		printf("dont play\n");

	}
	//printf ("\n");
}

// code 0: release note
void OPLreleaseNote(uint channel, uchar note)
{
    uint i;
    uint8_t id = channel;

    uint sustain = OPL2driverdata.channelSustain[channel];

#ifdef DEBUG
    printf("DEBUG: release: Ch: %d  N: %d\n", channel, note);
#endif
    for(i = 0; i < OPLchannels; i++){
		if (CHANNEL_ID(channels[i]) == id && channels[i].note == note) {
			if (sustain < 0x40){
				releaseChannel(i, 0);
			} else {
				channels[i].flags |= CH_SUSTAIN;
			}
		}
	}
}

// code 2: change pitch wheel (bender)
void OPLpitchWheel(uint channel, int pitch)
{
    uint i;
    uint8_t id = channel;

#ifdef DEBUG
    printf("DEBUG: pitch: Ch: %d  P: %d\n", channel, pitch);
#endif
    OPL2driverdata.channelPitch[channel] = pitch;
    for(i = 0; i < OPLchannels; i++)
    {
	struct channelEntry *ch = &channels[i];
	if (CHANNEL_ID(*ch) == id)
	{
	    ch->time = MLtime;
	    ch->pitch = ch->finetune + pitch;
	    writeFrequency(i, ch->realnote, ch->pitch, 1);
	}
    }
}

// code 4: change control
void OPLchangeControl(uint channel, uchar controller, int value)
{
    uint i;
    uint8_t id = channel;
#ifdef DEBUG
    printf("DEBUG: ctrl: Ch: %d  C: %d  V: %d\n", channel, controller, value);
#endif

    switch (controller) {
	case ctrlPatch:			/* change instrument */
	    OPL2driverdata.channelInstr[channel] = value;
	    break;
	case ctrlModulation:
	    OPL2driverdata.channelModulation[channel] = value;
	    for(i = 0; i < OPLchannels; i++)
	    {
		struct channelEntry *ch = &channels[i];
		if (CHANNEL_ID(*ch) == id)
		{
		    uchar flags = ch->flags;
		    ch->time = MLtime;
		    if (value >= MOD_MIN)
		    {
			ch->flags |= CH_VIBRATO;
			if (ch->flags != flags)
			    writeModulation(i, ch->instr, 1);
		    } else {
			ch->flags &= ~CH_VIBRATO;
			if (ch->flags != flags)
			    writeModulation(i, ch->instr, 0);
		    }
		}
	    }
	    break;
	case ctrlVolume:		/* change volume */
	    OPL2driverdata.channelVolume[channel] = value;
	    for(i = 0; i < OPLchannels; i++)
	    {
		struct channelEntry *ch = &channels[i];
		if (CHANNEL_ID(*ch) == id)
		{
		    ch->time = MLtime;
		    ch->realvolume = calcVolume(value, playingvolume, ch->volume);
		    OPLwriteVolume(i, ch->instr, ch->realvolume);
		}
	    }
	    break;
	case ctrlPan:			/* change pan (balance) */
	    OPL2driverdata.channelPan[channel] = value -= 64;
	    for(i = 0; i < OPLchannels; i++)
	    {
		struct channelEntry *ch = &channels[i];
		if (CHANNEL_ID(*ch) == id)
		{
		    ch->time = MLtime;
		    OPLwritePan(i, ch->instr, value);
		}
	    }
	    break;
	case ctrlSustainPedal:		/* change sustain pedal (hold) */
	    OPL2driverdata.channelSustain[channel] = value;
	    if (value < 0x40)
		releaseSustain(channel);
	    break;
    }
}


void OPLplayMusic()
{
    uint i;

    for (i = 0; i < CHANNELS; i++)
    {
	OPL2driverdata.channelVolume[i] = 127;	/* default volume 127 (full volume) */
	OPL2driverdata.channelSustain[i] = OPL2driverdata.channelLastVolume[i] = 0;
    }
}

void OPLstopMusic()
{
    uint i;
    for(i = 0; i < OPLchannels; i++)
	if (!(channels[i].flags & CH_FREE)){
	    releaseChannel(i, -1);
	}
}

void OPLchangeVolume(uint volume)
{
    uchar *channelVolume = OPL2driverdata.channelVolume;
    uint i;
    for(i = 0; i < OPLchannels; i++)
    {
	struct channelEntry *ch = &channels[i];
	if (true)
	{
	    ch->realvolume = calcVolume(channelVolume[ch->channel & 0xF], volume, ch->volume);
	    if (playingstate == ST_PLAYING)
		OPLwriteVolume(i, ch->instr, ch->realvolume);
	}
    }
}

#pragma argsused
void OPLpauseMusic()
{
    uint i;
    for(i = 0; i < OPLchannels; i++)
    {
	struct channelEntry *ch = &channels[i];
	if (true)
	{
	    struct OPL2instrument *instr = ch->instr;
	    if (OPL3mode)
		OPLwriteValue(0xC0, i, instr->feedback);
	    OPLwriteVolume(i, instr, 0);
	    OPLwriteChannel(0x60, i, 0, 0);	// attack, decay
	    OPLwriteChannel(0x80, i, instr->sust_rel_1 & 0xF0,
		instr->sust_rel_2 & 0xF0);	// sustain, release
	}
    }
}

#pragma argsused
void OPLunpauseMusic()
{
    uint i;
    for(i = 0; i < OPLchannels; i++)
    {
	struct channelEntry *ch = &channels[i];
	if (true)
	{
	    struct OPL2instrument *instr = ch->instr;

	    OPLwriteChannel(0x60, i, instr->att_dec_1,  instr->att_dec_2);
	    OPLwriteChannel(0x80, i, instr->sust_rel_1, instr->sust_rel_2);
	    OPLwriteVolume(i, instr, ch->realvolume);
	    if (OPL3mode)
		OPLwritePan(i, instr, OPL2driverdata.channelPan[ch->channel & 0xF]);
	}
    }
}


int OPLinitDriver(void)
{
    memset(channels, 0xFF, sizeof channels);
    //OPLinstruments = NULL;
    return 0;
}

int OPLdeinitDriver(void)
{
    //free(OPLinstruments);
    //OPLinstruments = NULL;
    return 0;
}

#pragma argsused
int OPLdriverParam(uint message, uint param1, void *param2)
{
    switch (message) {
	case DP_SINGLE_VOICE:
	    OPLsinglevoice = param1;
	    break;
    }
    return 0;
}

#pragma argsused
int OPLloadBank(int fd, uint bankNumber)
{
	/*
    static uchar masterhdr[8] = "#OPL_II#";
    uchar hdr[8];
    struct OP2instrEntry *instruments;

    if (read(fd, &hdr, sizeof hdr) != sizeof hdr)
	return -1;
    if (memcmp(hdr, masterhdr, sizeof hdr))
	return -2;
    if ( (instruments = (struct OP2instrEntry *)calloc(OP2INSTRCOUNT, OP2INSTRSIZE)) == NULL)
	return -3;
    if (read(fd, instruments, OP2INSTRSIZE * OP2INSTRCOUNT) != OP2INSTRSIZE * OP2INSTRCOUNT)
    {
	free(instruments);
	return -1;
    }
    free(OPLinstruments);
    OPLinstruments = instruments;
	*/
    return 0;
}

#pragma argsused
int OPL2detectHardware(uint port, uchar irq, uchar dma)
{
    return OPL2detect(port);
}

#pragma argsused
int OPL3detectHardware(uint port, uchar irq, uchar dma)
{
    return OPL3detect(port);
}

#pragma argsused
int OPL2initHardware(uint port, uchar irq, uchar dma)
{
    OPLinit(port, 0);
    return 0;
}

#pragma argsused
int OPL3initHardware(uint port, uchar irq, uchar dma)
{
    OPLinit(port, 1);
    return 0;
}

int OPL2deinitHardware(void)
{
    OPLdeinit();
    return 0;
}

int OPL3deinitHardware(void)
{
    OPLdeinit();
    return 0;
}

#pragma argsused
int OPLsendMIDI(uint command, uint par1, uint par2)
{
    return 0;
}

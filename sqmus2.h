
/*
 *	Name:		Main header include file
 *	Project:	MUS File Player Library
 *	Version:	1.75
 *	Author:		Vladimir Arnost (QA-Software)
 *	Last revision:	Mar-9-1996
 *	Compiler:	Borland C++ 3.1, Watcom C/C++ 10.0
 *
 */

#ifndef __MUSLIB_H_
#define __MUSLIB_H_


#include "test.h"

/* Global Definitions */

#define MLVERSION	0x0175
#define MLVERSIONSTR	"1.75"
extern char MLversion[];
extern char MLcopyright[];

#define CHANNELS	16		// total channels 0..CHANNELS-1
#define PERCUSSION	15		// percussion channel

/* MUS file header structure */
struct MUSheader {
	char	ID[4];			// identifier "MUS" 0x1A
	uint16_t	scoreLen;		// score length
	uint16_t	scoreStart;		// score start
	uint16_t	channels;		// primary channels
	uint16_t	sec_channels;		// secondary channels (??)
	uint16_t    instrCnt;		// used instrument count
	uint16_t	dummy;
//	uint16_t	instruments[...];	// table of used instruments
};


/* OPL2 instrument */
struct OPL2instrument {
/*00*/	uint8_t    trem_vibr_1;	/* OP 1: tremolo/vibrato/sustain/KSR/multi */
/*01*/	uint8_t	att_dec_1;	/* OP 1: attack rate/decay rate */
/*02*/	uint8_t	sust_rel_1;	/* OP 1: sustain level/release rate */
/*03*/	uint8_t	wave_1;		/* OP 1: waveform select */
/*04*/	uint8_t	scale_1;	/* OP 1: key scale lesvel */
/*05*/	uint8_t	level_1;	/* OP 1: output level */
/*06*/	uint8_t	feedback;	/* feedback/AM-FM (both operators) */
/*07*/	uint8_t    trem_vibr_2;	/* OP 2: tremolo/vibrato/sustain/KSR/multi */
/*08*/	uint8_t	att_dec_2;	/* OP 2: attack rate/decay rate */
/*09*/	uint8_t	sust_rel_2;	/* OP 2: sustain level/release rate */
/*0A*/	uint8_t	wave_2;		/* OP 2: waveform select */
/*0B*/	uint8_t	scale_2;	/* OP 2: key scale level */
/*0C*/	uint8_t	level_2;	/* OP 2: output level */
/*0D*/	uint8_t	unused;
/*0E*/	int16_t	basenote;	/* base note offset */
};

/* OP2 instrument file entry */
struct OP2instrEntry {
/*00*/	uint16_t	flags;			// see FL_xxx below
/*02*/	uint8_t	finetune;		// finetune value for 2-voice sounds
/*03*/	uint8_t	note;			// note # for fixed instruments
/*04*/	struct OPL2instrument instr[2];	// instruments
};

#define FL_FIXED_PITCH	0x0001		// note has fixed pitch (see below)
#define FL_UNKNOWN	0x0002		// ??? (used in instrument #65 only)
#define FL_DOUBLE_VOICE	0x0004		// use two voices instead of one


#define OP2INSTRSIZE	sizeof(struct OP2instrEntry) // instrument size (36 uint8_ts)
#define OP2INSTRCOUNT	(128 + 81-35+1)	// instrument count



#define BT_EMPTY	0
#define BT_CONV		1		// conventional memory buffer
#define BT_EMS		2		// EMS memory buffer
#define BT_XMS		3		// XMS memory buffer
 


 
#define TIMER_CNT18_2	0		// INT 08h: system timer (18.2 Hz)
#define TIMER_CNT140	1		// INT 08h: system timer (140 Hz)
#define TIMER_RTC1024	2		// INT 70h: RTC periodic interrupt (1024 Hz)
#define TIMER_RTC512	3		// RTC: 512 Hz
#define TIMER_RTC256	4		// RTC: 256 Hz
#define TIMER_RTC128	5		// RTC: 128 Hz
#define TIMER_RTC64	6		// RTC: 64 Hz

#define TIMER_MIN	TIMER_CNT18_2
#define TIMER_MAX	TIMER_RTC64
 

void	MLplayerInterrupt(void);

int	MLinitTimer(int mode);
int	MLshutdownTimer(void);

extern	volatile uint32_t	MLtime;

#ifdef __WINDOWS__
extern	/*HINSTANCE*/ uint MLinstance;
#endif


#define MAXMUSBLOCK 4



#define ST_EMPTY	0		// music block is empty
#define ST_STOPPED	1		// music block is used but not playing
#define ST_PLAYING	2		// music block is used and playing
#define ST_PAUSED	3		// music block is used and paused
					// any number >= 3 means `paused'

//#define MF_xxx		1		//

struct driverBlock {
	int8_t	driverID;
	uint16_t	datasize;

	int8_t	(*initDriver)(void);
	int8_t	(*deinitDriver)(void);
	int8_t	(*driverParam)(uint16_t message, uint16_t param1, void *param2);
	int8_t	(*loadBank)(int16_t fd, uint8_t bankNumber);
	int8_t	(*detectHardware)(uint16_t port, uint8_t irq, uint8_t dma);
	int8_t	(*initHardware)(uint16_t port, uint8_t irq, uint8_t dma);
	int8_t	(*deinitHardware)(void);

	void	(*playNote)(uint8_t channel, uint8_t note, int8_t volume);
	void	(*releaseNote)(uint8_t channel, uint8_t note);
	void	(*pitchWheel)(uint8_t channel, int8_t pitch);
	void	(*changeControl)(uint8_t channel, uint8_t controller, uint8_t value);
	void	(*playMusic)();
	void	(*stopMusic)();
	void	(*changeVolume)(int8_t volume);
	void	(*pauseMusic)();
	void	(*unpauseMusic)();
	int8_t	(*sendMIDI)(uint8_t command, uint8_t par1, uint8_t par2);

};

/* driverParam message codes */
#define DP_NOTHING		0x0000	/* no action */
#define DP_SYSEX		0x0001	/* send SYStem EXclusive message */
#define DP_SINGLE_VOICE		0x0002	/* OPLx: disabling double-voice mode */
  /* DP_SINGLE_VOICE: param1 codes */
  #define DPP_SINGLE_VOICE_OFF	0	/* default: off */
  #define DPP_SINGLE_VOICE_ON	1

#if 0 /* disable non-functional AWE32 reverb/chorus control */

#define DP_AWE32_REVERB		0x0003
  /* DP_AWE32_REVERB: param1 codes */
  #define DPP_REVERB_ROOM1	0	/* room 1 */
  #define DPP_REVERB_ROOM2	1	/* room 2 */
  #define DPP_REVERB_ROOM3	2	/* room 3 */
  #define DPP_REVERB_HALL1	3	/* hall 1 */
  #define DPP_REVERB_HALL2	4	/* hall 2 */
  #define DPP_REVERB_PLATE	5	/* plate */
  #define DPP_REVERB_DELAY	6	/* delay */
  #define DPP_REVERB_PANDELAY	7	/* panning delay */
#define DP_AWE32_CHORUS		0x0004
  /* DP_AWE32_CHORUS: param1 codes */
  #define DPP_CHORUS_1		0	/* chorus 1 */
  #define DPP_CHORUS_2		1	/* chorus 2 */
  #define DPP_CHORUS_3		2	/* chorus 3 */
  #define DPP_CHORUS_4		3	/* chorus 4 */
  #define DPP_CHORUS_FEEDBACK	4	/* feedback chorus */
  #define DPP_CHORUS_FLANGER	5	/* flanger */
  #define DPP_CHORUS_DELAY	6	/* short delay */
  #define DPP_CHORUS_DELAYFB	7	/* short delay (FB) */

#endif /* 0 */
    
/* From MLOPL.C */
#define DRV_OPL2    0x01
#define DRV_OPL3	0x02

extern struct driverBlock OPL2driver;
extern struct driverBlock OPL3driver;

void	OPLplayNote(uint8_t channel, uint8_t note, int8_t volume);
void	OPLreleaseNote(uint8_t channel, uint8_t note);
void    OPLpitchWheel(uint8_t channel, int8_t pitch);
void	OPLchangeControl(uint8_t channel, uint8_t controller, uint8_t value);
void	OPLplayMusic();
void	OPLstopMusic();
void	OPLchangeVolume(int8_t volume);
void	OPLpauseMusic();
void	OPLunpauseMusic();

int8_t	OPLinitDriver(void);
int8_t OPLdeinitDriver(void);
int8_t	OPLdriverParam(uint16_t message, uint16_t param1, void *param2);
int8_t OPLloadBank(int16_t fd, uint8_t bankNumber);

int8_t	OPL2detectHardware(uint16_t port, uint8_t irq, uint8_t dma);
int8_t	OPL2initHardware(uint16_t port, uint8_t irq, uint8_t dma);
int8_t	OPL2deinitHardware(void);

int8_t	OPL3detectHardware(uint16_t port, uint8_t irq, uint8_t dma);
int8_t	OPL3initHardware(uint16_t port, uint8_t irq, uint8_t dma);
int8_t	OPL3deinitHardware(void);

int8_t OPLsendMIDI(uint8_t command, uint8_t par1, uint8_t par2);


/* From MLOPL_IO.C */
#define ADLIBPORT	0x388
#define SBPORT		0x228
#define SBPROPORT	0x220
#define OPL2PORT	0x388		/* universal port number */
#define OPL3PORT	0x388

#define OPL2CHANNELS	9
#define OPL3CHANNELS	18
#define MAXCHANNELS	18

extern uint16_t OPLport;
extern uint8_t OPLchannels;
extern uint8_t OPL3mode;

uint8_t OPLwriteReg(uint16_t reg, uint8_t data);
void	OPLwriteChannel(uint8_t regbase, uint8_t channel, uint8_t data1, uint8_t data2);
void	OPLwriteValue(uint8_t regbase, uint8_t channel, uint8_t value);
void	OPLwriteFreq(uint8_t channel, uint16_t freq, uint8_t octave, uint8_t keyon);
int8_t	OPLconvertVolume(uint8_t data, int8_t volume);
int8_t	OPLpanVolume(int8_t volume, int8_t pan);
void	OPLwriteVolume(uint8_t channel, struct OPL2instrument *instr, int8_t volume);
void	OPLwritePan(uint8_t channel, struct OPL2instrument *instr, int8_t pan);
void	OPLwriteInstrument(uint8_t channel, struct OPL2instrument *instr);
void	OPLshutup(void);
void	OPLinit(uint16_t port, uint8_t OPL3);
void	OPLdeinit(void);
int16_t	OPL2detect(uint16_t port);
int16_t	OPL3detect(uint16_t port);
 

/* From MLMIDI.C */

/* MIDI events: description,  <parameters> */
/* the lower 4 bits contain channel number */
#define MIDI_NOTE_OFF	0x80	// release key,   <note#>, <velocity>
#define MIDI_NOTE_ON	0x90	// press key,     <note#>, <velocity>
#define MIDI_NOTE_TOUCH	0xA0	// key after-touch, <note#>, <velocity>
#define MIDI_CONTROL	0xB0	// control change, <controller>, <value>
#define MIDI_PATCH	0xC0	// patch change,  <patch#>
#define MIDI_CHAN_TOUCH	0xD0	// channel after-touch (??), <channel#>
#define MIDI_PITCH_WHEEL 0xE0	// pitch wheel,   <bottom>, <top 7 bits>
#define MIDI_EVENT_MASK	0xF0	// value to mask out the event number, not a command!

/* the following events contain no channel number */
#define MIDI_SYSEX	0xF0	// start of System Exclusive sequence
#define MIDI_SYSEX2	0xF7	// System Exclusive sequence continue
#define MIDI_TIMING	0xF8	// timing clock used when synchronization
				// is required
#define MIDI_START	0xFA	// start current sequence
#define MIDI_CONTINUE	0xFB	// continue a stopped sequence
#define MIDI_STOP	0xFC	// stop a sequence

enum MUSctrl {
    ctrlPatch = 0,
    ctrlBank,
    ctrlModulation,
    ctrlVolume,
    ctrlPan,
    ctrlExpression,
    ctrlReverb,
    ctrlChorus,
    ctrlSustainPedal,
    ctrlSoftPedal,
    _ctrlCount_,
    ctrlSoundsOff = _ctrlCount_,
    ctrlNotesOff,
    ctrlMono,
    ctrlPoly,
    ctrlResetCtrls
};

struct MIDIdata {
	uint8_t	controllers[_ctrlCount_][CHANNELS]; // MUS controllers
	uint8_t	channelLastVolume[CHANNELS];	// last volume
	int8_t	pitchWheel[CHANNELS];		// pitch wheel value
	int8_t	realChannels[CHANNELS];		// real MIDI output channels
	uint8_t	percussions[128/8];		// bit-map of used percussions
};

/*

void	MIDIplayNote(uint16_t channel, uint8_t note, int8_t volume);
void	MIDIreleaseNote(uint16_t channel, uint8_t note);
void	MIDIpitchWheel(uint16_t channel, int pitch);
void	MIDIchangeControl(uint16_t channel, uint8_t controller, uint8_t value);
void	MIDIplayMusic();
void	MIDIstopMusic();
void	MIDIchangeVolume(int8_t volume);
void	MIDIpauseMusic();
void	MIDIunpauseMusic();
int	MIDIinitDriver(void);
int	MIDIdeinitDriver(void);


#define DRV_SBMIDI	0x0005
#define SBMIDIPORT	0x220

extern struct driverBlock SBMIDIdriver;

int	SBMIDIsendMIDI(uint command, uint par1, uint par2);
int	SBMIDIdriverParam(uint message, uint param1, void *param2);
int	SBMIDIloadBank(int fd, uint bankNumber);

int	SBMIDIdetectHardware(uint port, uint8_t irq, uint8_t dma);
int	SBMIDIinitHardware(uint port, uint8_t irq, uint8_t dma);
int	SBMIDIdeinitHardware(void);
*/


 

void	SBsetMixer(uint16_t SBport, uint8_t index, uint8_t data);
int16_t	SBgetMixer(uint16_t SBport, uint8_t index);
int16_t	SBdetectMixer(uint16_t port);



#endif // __MUSLIB_H_

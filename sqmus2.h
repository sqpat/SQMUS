
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

#ifndef __DEFTYPES_H_
  #include "deftypes.h"
#endif
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

#ifndef __WINDOWS__

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
/*0E*/	sshort	basenote;	/* base note offset */
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

#endif /* __WINDOWS__ */

#ifdef __cplusplus
  extern "C" {
#endif


#define BT_EMPTY	0
#define BT_CONV		1		// conventional memory buffer
#define BT_EMS		2		// EMS memory buffer
#define BT_XMS		3		// XMS memory buffer

uint	MEMdetect(void);
int	MEMload(int fd, ulong length, struct memoryBlock *block, uint memory);
int	MEMfree(struct memoryBlock *block);
int	MEMgetchar(struct memoryBlock *block);
int	MEMrewind(struct memoryBlock *block);


/* From MLKERNEL.C */
#ifndef __WINDOWS__

#define TIMER_CNT18_2	0		// INT 08h: system timer (18.2 Hz)
#define TIMER_CNT140	1		// INT 08h: system timer (140 Hz)
#define TIMER_RTC1024	2		// INT 70h: RTC periodic interrupt (1024 Hz)
#define TIMER_RTC512	3		// RTC: 512 Hz
#define TIMER_RTC256	4		// RTC: 256 Hz
#define TIMER_RTC128	5		// RTC: 128 Hz
#define TIMER_RTC64	6		// RTC: 64 Hz

#define TIMER_MIN	TIMER_CNT18_2
#define TIMER_MAX	TIMER_RTC64

#else

#define TIMER_WIN35	0x100		// Windows Timer: low resolution (35Hz)
#define TIMER_WIN70	0x101		// Windows Timer: medium resolution (70Hz)
#define TIMER_WIN140	0x102		// Windows Timer: high resolution (140Hz)

#define TIMER_MIN	TIMER_WIN35
#define TIMER_MAX	TIMER_WIN140

#endif /* __WINDOWS__ */

void	MLplayerInterrupt(void);

int	MLinitTimer(int mode);
int	MLshutdownTimer(void);

extern	volatile ulong	MLtime;
extern	volatile uint	playingChannels;

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
	struct driverBlock *next;
	uint	driverID;
	char	*name;
	uint	datasize;

	int	(*initDriver)(void);
	int	(*deinitDriver)(void);
	int	(*driverParam)(uint message, uint param1, void *param2);
	int	(*loadBank)(int fd, uint bankNumber);
	int	(*detectHardware)(uint port, uchar irq, uchar dma);
	int	(*initHardware)(uint port, uchar irq, uchar dma);
	int	(*deinitHardware)(void);

	void	(*playNote)(uint channel, uchar note, int volume);
	void	(*releaseNote)(uint channel, uchar note);
	void	(*pitchWheel)(uint channel, int pitch);
	void	(*changeControl)(uint channel, uchar controller, int value);
	void	(*playMusic)();
	void	(*stopMusic)();
	void	(*changeVolume)(uint volume);
	void	(*pauseMusic)();
	void	(*unpauseMusic)();
	int	(*sendMIDI)(uint command, uint par1, uint par2);

	uint	state;		/* 0-not initialized, <>0-initialized */
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

/* From MLAPI.C */
extern struct musicBlock *MLmusicBlocks[];
extern struct driverBlock *MLdriverList;

void	MLinit(uint instance);
void	MLdeinit(void);
struct driverBlock *MLfindDriver(uint driverID);
int	MLaddDriver(struct driverBlock *newDriver);
int	MLinitDriver(uint driverID);
int	MLdeinitDriver(uint driverID);
int	MLdriverParam(uint driverID, uint message, uint param1, void *param2);
int	MLloadBank(uint driverID, int fd, uint bankNumber);
int	MLdetectHardware(uint driverID, short port, short irq, short dma);
int	MLinitHardware(uint driverID, short port, short irq, short dma);
int	MLdeinitHardware(uint driverID);

int	MLallocHandle(uint driverID);
int	MLfreeHandle(uint musHandle);
int	MLloadMUS(uint musHandle, int fd, uint memoryFlags);
int	MLfreeMUS(uint musHandle);
int	MLplay(uint musHandle);
int	MLstop(uint musHandle);
int	MLsetVolume(uint musHandle, uint volume);
int	MLgetVolume(uint musHandle);
int	MLpause(uint musHandle);
int	MLunpause(uint musHandle);
int	MLgetState(uint musHandle);
struct musicBlock *MLgetBlock(uint musHandle);
int	MLsetLoopCount(uint musHandle, int count);


/* From MLMISC.C */
#ifndef __WINDOWS__

/*int	MLdetectBlaster(uint *port, ushort *irq, ushort *dma, ushort *type);*/
uint	MLparseBlaster(const char *format, ...);

#endif /* __WINDOWS__ */


/* From MLDUMMY.C */
#define DRV_DUMMY 0x0000

extern struct driverBlock DUMMYdriver;

void	DUMMYplayNote(uint channel, uchar note, int volume);
void	DUMMYreleaseNote(uint channel, uchar note);
void	DUMMYpitchWheel(uint channel, int pitch);
void	DUMMYchangeControl(uint channel, uchar controller, int value);
void	DUMMYplayMusic();
void	DUMMYstopMusic();
void	DUMMYchangeVolume(uint volume);
void	DUMMYpauseMusic();
void	DUMMYunpauseMusic();
int	DUMMYsendMIDI(uint command, uint par1, uint par2);

int	DUMMYinitDriver(void);
int	DUMMYdeinitDriver(void);
int	DUMMYdriverParam(uint message, uint param1, void *param2);
int	DUMMYloadBank(int fd, uint bankNumber);

int	DUMMYdetectHardware(uint port, uchar irq, uchar dma);
int	DUMMYinitHardware(uint port, uchar irq, uchar dma);
int	DUMMYdeinitHardware(void);


#ifndef __WINDOWS__

/* From MLOPL.C */
#define DRV_OPL2        0x0001
#define DRV_OPL3	0x0002

extern struct driverBlock OPL2driver;
extern struct driverBlock OPL3driver;

void	OPLplayNote(uint channel, uchar note, int volume);
void	OPLreleaseNote(uint channel, uchar note);
void	OPLpitchWheel(uint channel, int pitch);
void	OPLchangeControl(uint channel, uchar controller, int value);
void	OPLplayMusic();
void	OPLstopMusic();
void	OPLchangeVolume(uint volume);
void	OPLpauseMusic();
void	OPLunpauseMusic();
int	OPLsendMIDI(uint command, uint par1, uint par2);

int	OPLinitDriver(void);
int	OPLdeinitDriver(void);
int	OPLdriverParam(uint message, uint param1, void *param2);
int	OPLloadBank(int fd, uint bankNumber);

int	OPL2detectHardware(uint port, uchar irq, uchar dma);
int	OPL2initHardware(uint port, uchar irq, uchar dma);
int	OPL2deinitHardware(void);

int	OPL3detectHardware(uint port, uchar irq, uchar dma);
int	OPL3initHardware(uint port, uchar irq, uchar dma);
int	OPL3deinitHardware(void);


/* From MLOPL_IO.C */
#define ADLIBPORT	0x388
#define SBPORT		0x228
#define SBPROPORT	0x220
#define OPL2PORT	0x388		/* universal port number */
#define OPL3PORT	0x388

#define OPL2CHANNELS	9
#define OPL3CHANNELS	18
#define MAXCHANNELS	18

extern uint OPLport;
extern uint OPLchannels;
extern uint OPL3mode;

uint	OPLwriteReg(uint reg, uchar data);
void	OPLwriteChannel(uint regbase, uint channel, uchar data1, uchar data2);
void	OPLwriteValue(uint regbase, uint channel, uchar value);
void	OPLwriteFreq(uint channel, uint freq, uint octave, uint keyon);
uint	OPLconvertVolume(uint data, uint volume);
uint	OPLpanVolume(uint volume, int pan);
void	OPLwriteVolume(uint channel, struct OPL2instrument *instr, uint volume);
void	OPLwritePan(uint channel, struct OPL2instrument *instr, int pan);
void	OPLwriteInstrument(uint channel, struct OPL2instrument *instr);
void	OPLshutup(void);
void	OPLinit(uint port, uint OPL3);
void	OPLdeinit(void);
int	OPL2detect(uint port);
int	OPL3detect(uint port);

#endif /* __WINDOWS__ */


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
	uchar	controllers[_ctrlCount_][CHANNELS]; // MUS controllers
	uchar	channelLastVolume[CHANNELS];	// last volume
	schar	pitchWheel[CHANNELS];		// pitch wheel value
	schar	realChannels[CHANNELS];		// real MIDI output channels
	uchar	percussions[128/8];		// bit-map of used percussions
};

void	MIDIplayNote(uint channel, uchar note, int volume);
void	MIDIreleaseNote(uint channel, uchar note);
void	MIDIpitchWheel(uint channel, int pitch);
void	MIDIchangeControl(uint channel, uchar controller, int value);
void	MIDIplayMusic();
void	MIDIstopMusic();
void	MIDIchangeVolume(uint volume);
void	MIDIpauseMusic();
void	MIDIunpauseMusic();
int	MIDIinitDriver(void);
int	MIDIdeinitDriver(void);


#ifndef __WINDOWS__

/* From MLAWE32.C */
#define DRV_AWE32 	0x0003

#define AWE32PORT	0x620

extern struct driverBlock AWE32driver;

int	AWE32sendMIDI(uint command, uint par1, uint par2);
int	AWE32driverParam(uint message, uint param1, void *param2);
int	AWE32loadBank(int fd, uint bankNumber);

int	AWE32detectHardware(uint port, uchar irq, uchar dma);
int	AWE32initHardware(uint port, uchar irq, uchar dma);
int	AWE32deinitHardware(void);


/* From MLMPU401.C */
#define DRV_MPU401	0x0004

#define MPU401PORT	0x330

extern struct driverBlock MPU401driver;

int	MPU401sendMIDI(uint command, uint par1, uint par2);
int	MPU401driverParam(uint message, uint param1, void *param2);
int	MPU401loadBank(int fd, uint bankNumber);

int	MPU401detectHardware(uint port, uchar irq, uchar dma);
int	MPU401initHardware(uint port, uchar irq, uchar dma);
int	MPU401deinitHardware(void);


/* From MLSBMIDI.C */
#define DRV_SBMIDI	0x0005

#define SBMIDIPORT	0x220

extern struct driverBlock SBMIDIdriver;

int	SBMIDIsendMIDI(uint command, uint par1, uint par2);
int	SBMIDIdriverParam(uint message, uint param1, void *param2);
int	SBMIDIloadBank(int fd, uint bankNumber);

int	SBMIDIdetectHardware(uint port, uchar irq, uchar dma);
int	SBMIDIinitHardware(uint port, uchar irq, uchar dma);
int	SBMIDIdeinitHardware(void);

#endif /* __WINDOWS__ */


#ifdef __WINDOWS__

/* From MLWINMM.C */
#define DRV_WINDOWS	0x0100

extern struct driverBlock WINdriver;

int	WINsendMIDI(uint command, uint par1, uint par2);
int	WINdriverParam(uint message, uint param1, void *param2);
int	WINloadBank(int fd, uint bankNumber);

/*
 * `port' value selects Windows MMSYSTEM driver number in the range
 *  0..midiOutGetNumDevs()-1 or MIDI_MAPPER. `irq' and `dma' are ignored.
 */
int	WINdetectHardware(uint port, uchar irq, uchar dma);
int	WINinitHardware(uint port, uchar irq, uchar dma);
int	WINdeinitHardware(void);

#endif /* __WINDOWS__ */


/* From MLOS2.C */
#define DRV_OS2		0x0200

extern struct driverBlock OS2driver;

int	OS2sendMIDI(uint command, uint par1, uint par2);
int	OS2driverParam(uint message, uint param1, void *param2);
int	OS2loadBank(int fd, uint bankNumber);

/*
 * `port' value selects OS/2 driver number; `irq' and `dma' may be ignored.
 */
int	OS2detectHardware(uint port, uchar irq, uchar dma);
int	OS2initHardware(uint port, uchar irq, uchar dma);
int	OS2deinitHardware(void);


/* From MLSBMIX.C */
#ifndef __WINDOWS__

void	SBsetMixer(uint SBport, uchar index, uchar data);
int	SBgetMixer(uint SBport, uchar index);
int	SBdetectMixer(uint port);

#endif /* __WINDOWS__ */


#ifdef __cplusplus
  }
#endif


#endif // __MUSLIB_H_

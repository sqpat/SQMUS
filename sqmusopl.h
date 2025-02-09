#ifndef __SQMUSOPL_H_
#define __SQMUSOPL_H_



#include "sqcommon.h"

/* Global Definitions */



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
 


 






extern struct driverBlock OPL2driver;
extern struct driverBlock OPL3driver;

void	OPLplayNote(uint8_t channel, uint8_t note, int8_t volume);
void	OPLreleaseNote(uint8_t channel, uint8_t note);
void    OPLpitchWheel(uint8_t channel, uint8_t pitch);
void	OPLchangeControl(uint8_t channel, uint8_t controller, uint8_t value);
void	OPLplayMusic();
void	OPLstopMusic();
void	OPLchangeVolume(int8_t volume);
void	OPLpauseMusic();
void	OPLunpauseMusic();

int8_t	OPLinitDriver(void);
int8_t  OPLdeinitDriver(void);
int8_t	OPLdriverParam(uint16_t message, uint16_t param1, void *param2);
int8_t  OPLloadBank(int16_t fd, uint8_t bankNumber);

int8_t	OPL2detectHardware(uint16_t port, uint8_t irq, uint8_t dma);
int8_t	OPL2initHardware(uint16_t port, uint8_t irq, uint8_t dma);
int8_t	OPL2deinitHardware(void);

int8_t	OPL3detectHardware(uint16_t port, uint8_t irq, uint8_t dma);
int8_t	OPL3initHardware(uint16_t port, uint8_t irq, uint8_t dma);
int8_t	OPL3deinitHardware(void);

int8_t OPLsendMIDI(uint8_t command, uint8_t par1, uint8_t par2);


#define ADLIBPORT	0x388
#define SBPORT		0x228
#define SBPROPORT	0x220
#define OPL2PORT	0x388		/* universal port number */
#define OPL3PORT	0x388

#define OPL2CHANNELS	9
#define OPL3CHANNELS	18
#define MAXCHANNELS	18

extern uint16_t OPLport;
extern uint8_t  OPLchannels;
extern uint8_t  OPL3mode;

uint8_t OPLwriteReg(uint16_t reg, uint8_t data);
void	  OPLwriteChannel(uint8_t regbase, uint8_t channel, uint8_t data1, uint8_t data2);
void	  OPLwriteValue(uint8_t regbase, uint8_t channel, uint8_t value);
void	  OPLwriteFreq(uint8_t channel, uint16_t freq, uint8_t octave, uint8_t keyon);
int8_t	OPLconvertVolume(uint8_t data, int8_t volume);
int8_t	OPLpanVolume(int8_t volume, int8_t pan);
void	  OPLwriteVolume(uint8_t channel, struct OPL2instrument *instr, int8_t volume);
void	  OPLwritePan(uint8_t channel, struct OPL2instrument *instr, int8_t pan);
void	  OPLwriteInstrument(uint8_t channel, struct OPL2instrument *instr);
void	  OPLshutup(void);
void	  OPLinit(uint16_t port, uint8_t OPL3);
void	  OPLdeinit(void);
int16_t	OPL2detect(uint16_t port);
int16_t	OPL3detect(uint16_t port);
 

 


 

void	SBsetMixer(uint16_t SBport, uint8_t index, uint8_t data);
int16_t	SBgetMixer(uint16_t SBport, uint8_t index);
int16_t	SBdetectMixer(uint16_t port);



#endif

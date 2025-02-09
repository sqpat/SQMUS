
#ifndef __SQCOMMON_H_
#define __SQCOMMON_H_

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

typedef  char					byte;


typedef union _fixed_t_union {
	uint32_t wu;
	int32_t w;

	struct dual_int16_t {
		int16_t fracbits;
		int16_t intbits;
	} h;

	struct dual_uuint16_t {
		uint16_t fracbits;
		uint16_t intbits;
	} hu;

	struct quad_int8_t {
		int8_t fracbytelow;
		int8_t fracbytehigh;
		int8_t intbytelow;
		int8_t intbytehigh;
	} b;

	struct quad_uint8_t {
		uint8_t fracbytelow;
		uint8_t fracbytehigh;
		uint8_t intbytelow;
		uint8_t intbytehigh;
	} bu;

	struct productresult_mid_t {
		int8_t throwawayhigh;
		int16_t usemid;
		int8_t throwawaylow;
	} productresult_mid;

} fixed_t_union;


typedef union _int16_t_union {
	uint16_t hu;
	int16_t h;

	struct dual_int8_t {
		int8_t bytelow;
		int8_t bytehigh;
	} b;

	struct dual_uint8_t {
		uint8_t bytelow;
		uint8_t bytehigh;
	} bu;

} int16_t_union;


#define ctrlPatch 			0
#define ctrlBank 			1
#define ctrlModulation 		2
#define ctrlVolume 			3
#define ctrlPan 			4
#define ctrlExpression		5
#define ctrlReverb			6
#define ctrlChorus			7
#define ctrlSustainPedal	8
#define ctrlSoftPedal		9
#define ctrlSoundsOff		10
#define ctrlNotesOff		11
#define ctrlMono			12
#define ctrlPoly			13
#define ctrlResetCtrls		14


struct driverBlock {
	int8_t	driverID;
	uint16_t	datasize;

	int8_t	(*initDriver)(void);
    
	int8_t	(*detectHardware)(uint16_t port, uint8_t irq, uint8_t dma);
	int8_t	(*initHardware)(uint16_t port, uint8_t irq, uint8_t dma);
	int8_t	(*deinitHardware)(void);

	void	(*playNote)(uint8_t channel, uint8_t note, int8_t noteVolume);
	void	(*releaseNote)(uint8_t channel, uint8_t note);
	void	(*pitchWheel)(uint8_t channel, uint8_t pitch);
	void	(*changeControl)(uint8_t channel, uint8_t controller, uint8_t value);
	void	(*playMusic)();
	void	(*stopMusic)();
	void	(*changeSystemVolume)(int16_t volume);
	int8_t	(*sendMIDI)(uint8_t command, uint8_t par1, uint8_t par2);

};

#define CHANNELS	16		// total channels 0..CHANNELS-1
#define PERCUSSION	15		// percussion channel
#define MAX_INSTRUMENTS 175
#define MAX_INSTRUMENTS_PER_TRACK 0x1C // largest in doom1 or doom2
#define DEFAULT_PITCH_BEND 0x80
extern uint8_t	playingstate;
#define DEFAULT_VOLUME  256
#define MUTE_VOLUME  0
extern uint16_t	playingpercussMask;

extern struct driverBlock * playingdriver;
extern int16_t playingvolume;



/* DP_SINGLE_VOICE: param1 codes */
#define DPP_SINGLE_VOICE_OFF	0	/* default: off */
#define DPP_SINGLE_VOICE_ON	1


#define DRV_OPL2    0x01
#define DRV_OPL3	0x02

#define ST_EMPTY	0		// music block is empty
#define ST_STOPPED	1		// music block is used but not playing
#define ST_PLAYING	2		// music block is used and playing
#define ST_PAUSED	3		// music block is used and paused
					// any number >= 3 means `paused'

#define TRUE (1 == 1)
#define FALSE (!TRUE)

#define true (1 == 1)
#define false (!true)

extern	volatile uint32_t	playingtime;

#endif

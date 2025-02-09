#ifndef __SQMUSMID_H_
#define __SQMUSMID_H_

#include "sqcommon.h"


#define NUM_CONTROLLERS 10

struct MIDIdata {
	uint8_t	controllers[NUM_CONTROLLERS][CHANNELS]; // MUS controllers
	uint8_t	channelLastVolume[CHANNELS];	// last volume
	uint8_t	pitchWheel[CHANNELS];		// pitch wheel value
	int8_t	realChannels[CHANNELS];		// real MIDI output channels
	uint8_t	percussions[128/8];		// bit-map of used percussions
};


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



void	MIDIplayNote(uint8_t channel, uint8_t note, int8_t noteVolume);
void	MIDIreleaseNote(uint8_t channel, uint8_t note);
void	MIDIpitchWheel(uint8_t channel, uint8_t pitch);
void	MIDIchangeControl(uint8_t channel, uint8_t controller, uint8_t value);
void	MIDIplayMusic();
void	MIDIstopMusic();
void	MIDIchangeSystemVolume(int16_t noteVolume);
int8_t  MIDIinitDriver(void);


 

#endif

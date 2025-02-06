#include "test.h"

#define OCTAVE_COUNT 8
#define TUNE_COUNT 32u
#define VOICE_COUNT 9
#define CHANNEL_COUNT 16
#define MAX_VELOCITY 0x7F

// 107
#define MAX_NOTE ( OCTAVE_COUNT * 12 + 11 )


/* Number of slots for the voices on the chip */
#define NumChipSlots 18

#define FINETUNE_RANGE TUNE_COUNT
#define PITCHBEND_CENTER 1638400u


#define AL_MaxVolume             127
#define AL_DefaultChannelVolume  90
#define AL_DefaultPitchBendRange 200

enum octaves {
   OCTAVE_0 = 0x0000,
   OCTAVE_1 = 0x0400,
   OCTAVE_2 = 0x0800,
   OCTAVE_3 = 0x0C00,
   OCTAVE_4 = 0x1000,
   OCTAVE_5 = 0x1400,
   OCTAVE_6 = 0x1800,
   OCTAVE_7 = 0x1C00
};





typedef struct AdLibVoice   {
   struct AdLibVoice *next;
   struct AdLibVoice *prev;

   int8_t      num;
   uint8_t     key;
   uint8_t     velocity;
   uint8_t     channel;
   int16_t_union     pitchleft;
   int16_t_union     pitchright;
   uint8_t     timbre;
   uint16_t    port;
   int16_t     status;
} AdLibVoice;

typedef struct {
    AdLibVoice *start;
    AdLibVoice *end;
} AdLibVoiceList;


typedef struct {
   AdLibVoiceList   Voices;
   uint8_t          Timbre;
   uint8_t          Pitchbend;
   uint8_t          KeyOffset;
   uint8_t          KeyDetune;
   uint8_t          Volume;
   //uint8_t         EffectiveVolume;
   uint8_t          Pan;
   uint8_t          Detune;
   uint8_t          RPN;
   int16_t          PitchBendRange;
   int16_t          PitchBendSemiTones;
   int16_t          PitchBendHundreds;
} AdLibChannel;

typedef struct {
   uint8_t  SAVEK[2];
   uint8_t  Level[2];
   uint8_t  Env1[2];
   uint8_t  Env2[2];
   uint8_t  Wave[2];
   uint8_t  Feedback;
   int8_t   Transpose;
   int8_t   Velocity;
} AdLibTimbre;


#define NOTE_ON         0x2000  /* Used to turn note on or toggle note */
#define NOTE_OFF        0x0000

int16_t AL_Init();
int16_t AL_Detect();
void AL_NoteOn (uint8_t channel, uint8_t key, int8_t volume);
void AL_NoteOff (uint8_t channel, uint8_t key);
void AL_SetPitchBend(uint8_t channel, uint8_t lsb, uint8_t msb);
void AL_SetChannelPan (uint8_t channel, uint8_t pan);
void AL_SetChannelVolume(uint8_t channel, uint8_t volume);
void AL_AllNotesOff(uint8_t channel);
void AL_ResetVoices();
void AL_SetChannelDetune (uint8_t channel, uint8_t detune);
void AL_ProgramChange (uint8_t channel, uint8_t patch);



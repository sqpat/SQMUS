#include "sqmus.h"
#include "test.h"
#include <string.h>
#include <conio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>
#include <graph.h>

#include <i86.h>



static uint16_t OctavePitch[OCTAVE_COUNT] = {
    OCTAVE_0, OCTAVE_1, OCTAVE_2, OCTAVE_3,
    OCTAVE_4, OCTAVE_5, OCTAVE_6, OCTAVE_7,
};

// Pitch table

static uint16_t NotePitch[TUNE_COUNT][12] = {
      { 0x157, 0x16b, 0x181, 0x198, 0x1b0, 0x1ca, 0x1e5, 0x202, 0x220, 0x241, 0x263, 0x287 },
      { 0x157, 0x16b, 0x181, 0x198, 0x1b0, 0x1ca, 0x1e5, 0x202, 0x220, 0x242, 0x264, 0x288 },
      { 0x158, 0x16c, 0x182, 0x199, 0x1b1, 0x1cb, 0x1e6, 0x203, 0x221, 0x243, 0x265, 0x289 },
      { 0x158, 0x16c, 0x183, 0x19a, 0x1b2, 0x1cc, 0x1e7, 0x204, 0x222, 0x244, 0x266, 0x28a },
      { 0x159, 0x16d, 0x183, 0x19a, 0x1b3, 0x1cd, 0x1e8, 0x205, 0x223, 0x245, 0x267, 0x28b },
      { 0x15a, 0x16e, 0x184, 0x19b, 0x1b3, 0x1ce, 0x1e9, 0x206, 0x224, 0x246, 0x268, 0x28c },
      { 0x15a, 0x16e, 0x185, 0x19c, 0x1b4, 0x1ce, 0x1ea, 0x207, 0x225, 0x247, 0x269, 0x28e },
      { 0x15b, 0x16f, 0x185, 0x19d, 0x1b5, 0x1cf, 0x1eb, 0x208, 0x226, 0x248, 0x26a, 0x28f },
      { 0x15b, 0x170, 0x186, 0x19d, 0x1b6, 0x1d0, 0x1ec, 0x209, 0x227, 0x249, 0x26b, 0x290 },
      { 0x15c, 0x170, 0x187, 0x19e, 0x1b7, 0x1d1, 0x1ec, 0x20a, 0x228, 0x24a, 0x26d, 0x291 },
      { 0x15d, 0x171, 0x188, 0x19f, 0x1b7, 0x1d2, 0x1ed, 0x20b, 0x229, 0x24b, 0x26e, 0x292 },
      { 0x15d, 0x172, 0x188, 0x1a0, 0x1b8, 0x1d3, 0x1ee, 0x20c, 0x22a, 0x24c, 0x26f, 0x293 },
      { 0x15e, 0x172, 0x189, 0x1a0, 0x1b9, 0x1d4, 0x1ef, 0x20d, 0x22b, 0x24d, 0x270, 0x295 },
      { 0x15f, 0x173, 0x18a, 0x1a1, 0x1ba, 0x1d4, 0x1f0, 0x20e, 0x22c, 0x24e, 0x271, 0x296 },
      { 0x15f, 0x174, 0x18a, 0x1a2, 0x1bb, 0x1d5, 0x1f1, 0x20f, 0x22d, 0x24f, 0x272, 0x297 },
      { 0x160, 0x174, 0x18b, 0x1a3, 0x1bb, 0x1d6, 0x1f2, 0x210, 0x22e, 0x250, 0x273, 0x298 },
      { 0x161, 0x175, 0x18c, 0x1a3, 0x1bc, 0x1d7, 0x1f3, 0x211, 0x22f, 0x251, 0x274, 0x299 },
      { 0x161, 0x176, 0x18c, 0x1a4, 0x1bd, 0x1d8, 0x1f4, 0x212, 0x230, 0x252, 0x276, 0x29b },
      { 0x162, 0x176, 0x18d, 0x1a5, 0x1be, 0x1d9, 0x1f5, 0x212, 0x231, 0x254, 0x277, 0x29c },
      { 0x162, 0x177, 0x18e, 0x1a6, 0x1bf, 0x1d9, 0x1f5, 0x213, 0x232, 0x255, 0x278, 0x29d },
      { 0x163, 0x178, 0x18f, 0x1a6, 0x1bf, 0x1da, 0x1f6, 0x214, 0x233, 0x256, 0x279, 0x29e },
      { 0x164, 0x179, 0x18f, 0x1a7, 0x1c0, 0x1db, 0x1f7, 0x215, 0x235, 0x257, 0x27a, 0x29f },
      { 0x164, 0x179, 0x190, 0x1a8, 0x1c1, 0x1dc, 0x1f8, 0x216, 0x236, 0x258, 0x27b, 0x2a1 },
      { 0x165, 0x17a, 0x191, 0x1a9, 0x1c2, 0x1dd, 0x1f9, 0x217, 0x237, 0x259, 0x27c, 0x2a2 },
      { 0x166, 0x17b, 0x192, 0x1aa, 0x1c3, 0x1de, 0x1fa, 0x218, 0x238, 0x25a, 0x27e, 0x2a3 },
      { 0x166, 0x17b, 0x192, 0x1aa, 0x1c3, 0x1df, 0x1fb, 0x219, 0x239, 0x25b, 0x27f, 0x2a4 },
      { 0x167, 0x17c, 0x193, 0x1ab, 0x1c4, 0x1e0, 0x1fc, 0x21a, 0x23a, 0x25c, 0x280, 0x2a6 },
      { 0x168, 0x17d, 0x194, 0x1ac, 0x1c5, 0x1e0, 0x1fd, 0x21b, 0x23b, 0x25d, 0x281, 0x2a7 },
      { 0x168, 0x17d, 0x194, 0x1ad, 0x1c6, 0x1e1, 0x1fe, 0x21c, 0x23c, 0x25e, 0x282, 0x2a8 },
      { 0x169, 0x17e, 0x195, 0x1ad, 0x1c7, 0x1e2, 0x1ff, 0x21d, 0x23d, 0x260, 0x283, 0x2a9 },
      { 0x16a, 0x17f, 0x196, 0x1ae, 0x1c8, 0x1e3, 0x1ff, 0x21e, 0x23e, 0x261, 0x284, 0x2ab },
      { 0x16a, 0x17f, 0x197, 0x1af, 0x1c8, 0x1e4, 0x200, 0x21f, 0x23f, 0x262, 0x286, 0x2ac }
   };

// Slot numbers as a function of the voice and the operator.
// (melodic only)



static int8_t slotVoice[VOICE_COUNT] = {
    0 ,    // voice 0
    1 ,    // 1
    2 ,    // 2
    6 ,    // 3
    7 ,   // 4
    8 ,   // 5
    12 ,  // 6
    13 ,  // 7
    14 ,  // 8
};

// This table gives the offset of each slot within the chip.
// offset = fn(slot)

static int8_t offsetSlot[NumChipSlots] = {
    0,  1,  2,  3,  4,  5,
    8,  9, 10, 11, 12, 13,
   16, 17, 18, 19, 20, 21
};


OPL2InstrumentEntry AdLibInstrumentList[MAX_INSTRUMENTS];


#define ADLIB_PORT     0x388
#define AL_LeftPort    0x388
#define AL_RightPort   0x388

int8_t MAX_MUS_CHANNEL = 10;
#define VOICE_NOT_FOUND -1

AdLibVoice     AdLibVoices[VOICE_COUNT * 2];
AdLibChannel   AdLibChannels[CHANNEL_COUNT];
uint8_t        AdLibVoiceLevels[NumChipSlots][2];
uint8_t        AdLibVoiceKsls[NumChipSlots][2];
uint8_t        AdLibVoiceReserved[VOICE_COUNT * 2];
int8_t         AdLibStereoOn = 0;
AdLibVoiceList Voice_Pool;
//#define NULL 0
int8_t channelonoff[CHANNEL_COUNT];


void AL_Remove (AdLibVoiceList* listhead, AdLibVoice * item) {

    // head is prevmost
    // tail is nextmost

    if (item->prev == NULL) {   // was head
        listhead->start = item->next;
    } else {
        item->prev->next = item->next;
    }

    if (item->next == NULL){    // was tail
        listhead->end = item->prev;
    } else {
        item->next->prev = item->prev;
    }
    item->next = NULL;
    item->prev = NULL;

}

void AL_AddToTail(AdLibVoiceList* listhead, AdLibVoice* item) {

    // head is prevmost
    // tail is nextmost

    item->next = NULL;           // new tail has no next (tail is nextmost)
    item->prev = listhead->end;  // prev connects to current (soon to be prior) end...

    if (listhead->end) {
        // connect prior end to this (if it existed)
        (listhead->end)->next = item;
    } else {
        // list was empty
        listhead->start = item;
    }

    // this is the new tail
    listhead->end = item;

}






/*---------------------------------------------------------------------
   Function: AL_AllocVoice

   Retrieves a free voice from the voice pool.
---------------------------------------------------------------------*/

int8_t lastid = 0;

int8_t AL_AllocVoice() {

    int8_t i = lastid;
    
    lastid++;
    if (lastid >= VOICE_COUNT * 2){
        lastid = 0;
    }
    return i;


    for (i = 0; i < VOICE_COUNT * 2; i++){
            
        if (AdLibVoices[i].status == NOTE_OFF){
            AL_Remove(&Voice_Pool, &AdLibVoices[i]);
            return i;
        }
    }
/*
    int8_t voice;
    if (Voice_Pool.start) {
        AdLibVoice * ptr = Voice_Pool.start;
        while (ptr && ptr->status == NOTE_ON){
            printf("get next!\n");  // shouldnt happen
            ptr = ptr->next;
        }

        if (ptr){
            voice = Voice_Pool.start->num;
            AL_Remove(&Voice_Pool, &AdLibVoices[voice]);
            return voice;
        }

    }
    */

    printf("voice not found! A");

    return VOICE_NOT_FOUND;
}


/*---------------------------------------------------------------------
   Function: AL_GetVoice

   Determines which voice is associated with a specified note and
   MIDI channel.
---------------------------------------------------------------------*/



int8_t AL_GetVoice(uint8_t channel, uint8_t key) {
    AdLibVoice *voice;
    voice = AdLibChannels[channel].Voices.start;

    while(voice != NULL) {
        if (voice->key == key) {
            return voice->num;
        }
        voice = voice->next;
    }

    printf("voice not found!");
    return VOICE_NOT_FOUND;
}



int16_t adlib_device = 1; // todo


void AL_SendOutputToPort(int16_t port, int16_t reg, uint8_t data) {
    int8_t delay;
	//   FILE* fp = fopen ("log2.txt", "ab");
	//   fprintf(fp, "outp %03x, %02x outp %03x, %02x\n", port, reg, port+1, data);
 	//   fclose(fp);

    outp(port, reg);

    //   for(delay = 2; delay > 0 ; delay--)
    for (delay = 6; delay > 0; delay--) {
        inp(port);
    }

    outp(port + 1, data);

    // todo move delay outside?

    //   for(delay = 35; delay > 0 ; delay--)
    for (delay = 27; delay > 0; delay--){
    //   for(delay = 2; delay > 0 ; delay--)
        inp(port);
    }
}



void AL_SendOutput(uint8_t port, int16_t reg, uint8_t data){

    // todo use port in opl3 case?

    if(adlib_device){ //todo check for opl3?

        //chip->fm_writereg(usereg, data);
        //outp(usereg, data);
//        if (AdLibStereoOn){
//            AL_SendOutputToPort(AL_LeftPort, reg, data);
//            AL_SendOutputToPort(AL_RightPort, reg, data);
//        } else {
            //int16_t useport = (port == 0) ? AL_RightPort : AL_LeftPort;

//            int16_t usereg = reg;            
            // opl3 related???

            //if (port){
            //    usereg |= 0x100;
            //}

            AL_SendOutputToPort(ADLIB_PORT, reg, data);

//        }
    }
}

void AL_WriteChannel(uint8_t port, int16_t reg, uint8_t data1, uint8_t data2){

    if(adlib_device){ //todo check for opl3?
        AL_SendOutputToPort(ADLIB_PORT, reg+0, data1);
        AL_SendOutputToPort(ADLIB_PORT, reg+3, data2);
    }
}


static uint8_t calcVolume(uint8_t channelVolume, uint8_t MUSvolume, uint8_t noteVolume) {
    int16_t usevolume = ((uint32_t)channelVolume * MUSvolume * noteVolume) / (256*127);
    if (usevolume > 127){
	    return 127;
    } else {
	    return usevolume;
    }
}

/*---------------------------------------------------------------------
   Function: AL_SetVoicePan
   
   Sets the stereo voice panning of the specified AdLib voice.
---------------------------------------------------------------------*/

/*
void AL_SetVoicePan(uint8_t voiceindex) {
    AdLibVoice *voice;
    uint8_t channel;
    uint8_t pan;
    uint8_t port;
    uint8_t voc;
    uint8_t mask;
    AdLibTimbre *timbre;
   
    voice = &AdLibVoices[voiceindex];
    channel = voice->channel;
    pan = AdLibChannels[channel].Pan;
   
    port = voice->port;
    voc  = (voiceindex >= VOICE_COUNT) ? voiceindex - VOICE_COUNT : voiceindex;
    timbre = &ADLIB_TimbreBank[voice->timbre];
    
    // balance around 64?
    
    if (pan >= 96){
        mask = 0x10;
    } else if (pan <= 48){
        mask = 0x20;
    } else {
        mask = 0x30;
    }
    AL_SendOutput(port, 0xC0 + voc, (timbre->Feedback & 0x0F) | mask);
}
*/


/*---------------------------------------------------------------------
   Function: AL_SetVoicePitch

   Programs the pitch of the specified voice.
---------------------------------------------------------------------*/
/*
void AL_SetVoicePitch(int8_t voice){
    int16_t note16;
    int8_t  note8;
    uint8_t channel;
    uint8_t patch;
    uint8_t detune;
    uint8_t ScaleNote;
    uint8_t Octave;
    int16_t_union pitch;
    uint8_t port;
    uint8_t voc;

    port = AdLibVoices[voice].port;
    voc  = (voice >= VOICE_COUNT) ? voice - VOICE_COUNT : voice;
    channel = AdLibVoices[voice].channel;

    if (channel == PERCUSSION_CHANNEL){  // drum
        patch = AdLibVoices[voice].key + 128;
        note16  = ADLIB_TimbreBank[patch].Transpose;
    } else {
        patch = AdLibChannels[channel].Timbre;
        note16  = AdLibVoices[voice].key + ADLIB_TimbreBank[patch].Transpose;
    }

    note16 += AdLibChannels[channel].KeyOffset - 12;
    if (note16 > MAX_NOTE){
        note16 = MAX_NOTE;
    }
    if (note16 < 0){
        note16 = 0;
    }
    note8 = note16;

    detune = AdLibChannels[channel].KeyDetune;

    // lookups are probably slower? calc both in one div instruction
    ScaleNote = note8 % 12;
    Octave    = note8 / 12;

    pitch.hu = OctavePitch[Octave] | NotePitch[detune][ScaleNote];

    AdLibVoices[voice].pitchleft = pitch;

    pitch.hu |= AdLibVoices[voice].status; // just NOTE_ON?

    AL_SendOutput(port, 0xA0 + voc, pitch.b.bytelow);
    AL_SendOutput(port, 0xB0 + voc, pitch.b.bytehigh);
}*/


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



/*
 * Write frequency/octave/keyon data to a channel
 */
void OPLwriteFreq(uint channel, uint freq, uint octave, uint keyon)
{
    AL_SendOutput(port, 0xA0 + voc, 0);
    AL_SendOutput(port, 0xB0 + voc, 0);
    OPLwriteValue(0xA0, channel, (BYTE)freq);
    OPLwriteValue(0xB0, channel, (freq >> 8) | (octave << 2) | (keyon << 5));
}

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




/*---------------------------------------------------------------------
   Function: AL_SetVoiceVolume

   Sets the volume of the specified voice.
---------------------------------------------------------------------*/

void AL_SetVoiceVolume (int8_t voice) {
    uint8_t channel;
    uint8_t velocity;
    uint8_t slot;
    uint8_t port;
    uint8_t voc;
    uint32_t t1;
    uint8_t volume;
    AdLibTimbre *timbre;

    channel = AdLibVoices[voice].channel;

    timbre = &ADLIB_TimbreBank[AdLibVoices[voice].timbre];

    velocity = AdLibVoices[voice].velocity + timbre->Velocity;
    velocity = velocity & MAX_VELOCITY;

    voc  = (voice >= VOICE_COUNT) ? voice - VOICE_COUNT : voice;
    slot = slotVoice[voc] + 3;
    port = AdLibVoices[voice].port;

    // amplitude
    t1  = AdLibVoiceLevels[slot][port];
    t1 *= (((int16_t)velocity) + 0x80);
    t1  = (t1 * ((int16_t)AdLibChannels[channel].Volume)) >> 15;   // todo gross

    volume  = t1 ^ 63;
    volume |= AdLibVoiceKsls[slot][port];

    AL_SendOutput(port, 0x40 + offsetSlot[slot], volume);

   // Check if this timbre is Additive
    if (timbre->Feedback & 0x01) {
        uint32_t t2;
        slot -= 3;

        // amplitude
        t2  = AdLibVoiceLevels[slot][port];
        t2 *= (((int16_t)velocity) + 0x80);
        t2  = (t1 * ((int16_t)AdLibChannels[channel].Volume)) >> 15;

        volume  = t2 ^ 63;
        volume |= AdLibVoiceKsls[slot][port];

        AL_SendOutput(port, 0x40 + offsetSlot[slot], volume);
    }
}


/*---------------------------------------------------------------------
   Function: AL_SetVoiceTimbre

   Programs the specified voice's timbre.
---------------------------------------------------------------------*/

void AL_SetVoiceTimbre (int8_t voice) {
   uint8_t    off;
   uint8_t    slot;
   uint8_t    port;
   uint8_t    voc;
   uint8_t    patch;
   uint8_t    channel;
   OPL2Instrument *instr;

   channel = AdLibVoices[voice].channel;

    if (channel == PERCUSSION_CHANNEL) {
        patch = AdLibVoices[voice].key + 128;
    } else {
        patch = AdLibChannels[channel].Timbre;
    }

    if (AdLibVoices[voice].timbre == patch){
        return;       // TODO FIX: this should be enabled but it seems to cause some bad instruments
    }

    instr = &AdLibInstrumentList[patch].instr[0];  // secondary would be [1], but unused? 

    //printf ("\nplay with timbre %i %i %i\n", patch, channel, voice);

    AdLibVoices[voice].timbre = patch;


    port = AdLibVoices[voice].port;
    voc  = (voice >= VOICE_COUNT) ? voice - VOICE_COUNT : voice;
    slot = slotVoice[voc];
    off  = offsetSlot[slot];

    //AdLibVoiceLevels[slot][port] = 63 - (timbre->Level[0] & 0x3F);
    //AdLibVoiceKsls[slot][port]   = timbre->Level[0] & 0xC0;



/*
    // clear freq
    AL_SendOutput(port, 0xA0 + voc, 0);
    AL_SendOutput(port, 0xB0 + voc, 0);

    // Let voice clear the release
    AL_SendOutput(port, 0x80 + off, 0xFF);

    // instrument stuff
    AL_SendOutput(port, 0x60 + off, timbre->Env1[0]);       //attack
    AL_SendOutput(port, 0x80 + off, timbre->Env2[0]);       //sustain?
    AL_SendOutput(port, 0x20 + off, timbre->SAVEK[0]);      //trem
    AL_SendOutput(port, 0xE0 + off, timbre->Wave[0]);       //wave

    AL_SendOutput(port, 0x40 + off, timbre->Level[0]);      
    slot = slotVoice[voc][1];

    // play the note?
    AL_SendOutput(port, 0xC0 + voc, (timbre->Feedback & 0x0F) | 0x30);
    //AL_SendOutputToPort(ADLIB_PORT, 0xC0 + voice, timbre->Feedback);




    off = offsetSlot[slot];

    AdLibVoiceLevels[slot][port] = 63 - (timbre->Level[1] & 0x3F);
    AdLibVoiceKsls[slot][port]   = timbre->Level[1] & 0xC0;
    
    AL_SendOutput(port, 0x40 + off, 63);        // no volume

    // Let voice clear the release
    AL_SendOutput(port, 0x80 + off, 0xFF);

    AL_SendOutput(port, 0x60 + off, timbre->Env1[1]);
    AL_SendOutput(port, 0x80 + off, timbre->Env2[1]);
    AL_SendOutput(port, 0x20 + off, timbre->SAVEK[1]);
    AL_SendOutput(port, 0xE0 + off, timbre->Wave[1]);

    AL_SendOutput(port, 0x40 + off, timbre->Level[1]);  

*/



    AL_WriteChannel(0x40, channel, 0x3F, 0x3F);		// no volume
    AL_WriteChannel(0x20, channel, instr->trem_vibr_1, instr->trem_vibr_2);
    AL_WriteChannel(0x60, channel, instr->att_dec_1,   instr->att_dec_2);
    AL_WriteChannel(0x80, channel, instr->sust_rel_1,  instr->sust_rel_2);
    AL_WriteChannel(0xE0, channel, instr->wave_1,      instr->wave_2);
    //AL_SendOutput  (0xC0, channel, instr->feedback | 0x30);
    AL_SendOutput(port, 0xC0 + voc, instr->feedback | 0x30);

/*

    // clear freq
    AL_SendOutput(port, 0xA0 + voc, 0);
    AL_SendOutput(port, 0xB0 + voc, 0);

    // Let voice clear the release
    AL_WriteChannel(port, 0x40 + off + 0, 0x3F, 0x3F);
    AL_WriteChannel(port, 0x80 + off, 0xFF, 0xFF);

    // instrument stuff
    AL_WriteChannel(port, 0x20 + off, timbre->SAVEK[0] , timbre->SAVEK[1]);      //trem
    AL_WriteChannel(port, 0x60 + off, timbre->Env1[0]  , timbre->Env1[1]);      //attack
    AL_WriteChannel(port, 0x80 + off, timbre->Env2[0]  , timbre->Env2[1]);      //sustain?
    AL_WriteChannel(port, 0xE0 + off, timbre->Wave[0]  , timbre->Wave[1]);      //sustain?

    // play the note?
    AL_SendOutput(port, 0xC0 + voc, (timbre->Feedback & 0x0F) | 0x30);
    //    AL_SendOutput(port, 0xC0 + voc + 3, (timbre->Feedback & 0x0F) | 0x30);
    
    AL_WriteChannel(port, 0x40 + off, timbre->Level[0], 63);      */

    //AL_SendOutputToPort(ADLIB_PORT, 0xC0 + voice, timbre->Feedback);

    slot += 3;
    off = offsetSlot[slot];
    AdLibVoiceLevels[slot][port] = 63 - (timbre->Level[1] & 0x3F);
    AdLibVoiceKsls[slot][port]   = timbre->Level[1] & 0xC0;


}



/*---------------------------------------------------------------------
   Function: AL_ControlChange

   Sets the value of a controller on the specified MIDI channel.
---------------------------------------------------------------------*/

/*
void AL_ControlChange (uint8_t channel, uint8_t type, uint8_t data){
    // We only play channels 1 through 10
    if (channel > AL_MaxMidiChannel) {
        return;
    }

    switch(type) {



        case MIDI_DETUNE :
            AL_SetChannelDetune(channel, data);
            break;

        case MIDI_RPN_MSB :
            AdLibChannels[channel].RPN &= 0x00FF;
            AdLibChannels[channel].RPN |= (data & 0xFF) << 8;
            break;

        case MIDI_RPN_LSB :
            AdLibChannels[channel].RPN &= 0xFF00;
            AdLibChannels[channel].RPN |= data & 0xFF;
            break;

        case MIDI_DATAENTRY_MSB :
            if (AdLibChannels[channel].RPN == MIDI_PITCHBEND_RPN)
            {
                AdLibChannels[channel].PitchBendSemiTones = data;
                AdLibChannels[channel].PitchBendRange     =
                    AdLibChannels[channel].PitchBendSemiTones * 100 +
                    AdLibChannels[channel].PitchBendHundreds;
            }
            break;

        case MIDI_DATAENTRY_LSB :
            if (AdLibChannels[channel].RPN == MIDI_PITCHBEND_RPN) {
                AdLibChannels[channel].PitchBendHundreds = data;
                AdLibChannels[channel].PitchBendRange    =
                    AdLibChannels[channel].PitchBendSemiTones * 100 +
                    AdLibChannels[channel].PitchBendHundreds;
            }
            break;
    }
}

*/


void AL_SetPitchBend(uint8_t channel, uint8_t msb){    
    int16_t_union  pitchbend;
    uint32_t  TotalBend;
    AdLibVoice  *voice;

    // We only play channels 1 through 10
    if (channel > MAX_MUS_CHANNEL && channel != PERCUSSION_CHANNEL){
        return;
    }

    pitchbend.b.bytehigh = msb;
    pitchbend.b.bytelow = 0;

    AdLibChannels[channel].Pitchbend = pitchbend.hu;

    TotalBend  = pitchbend.hu * AdLibChannels[channel].PitchBendRange;
    TotalBend /= (PITCHBEND_CENTER / FINETUNE_RANGE);

    AdLibChannels[channel].KeyOffset  = (int)(TotalBend / FINETUNE_RANGE);
    AdLibChannels[channel].KeyOffset -= AdLibChannels[channel].PitchBendSemiTones;

    AdLibChannels[channel].KeyDetune = (TotalBend % FINETUNE_RANGE);

    voice = AdLibChannels[channel].Voices.start;
    while(voice != NULL) {
        AL_SetVoicePitch(voice->num);
        voice = voice->next;
    }
}


void AL_NoteOff (uint8_t channel, uint8_t key) {
    int8_t   voice;
    uint16_t port;
    uint8_t  voc;

    
    // We only play channels 1 through 10
    if (channel > MAX_MUS_CHANNEL && channel != PERCUSSION_CHANNEL) {
        printf("bad note off channel %i\n", channel);
        return;
    }

    voice = AL_GetVoice(channel, key);

    if (voice == VOICE_NOT_FOUND){
        //printf("couldn't find voice to note off!\n");
        return;
    }

    //printf("%i OFF\t", channel);
    channelonoff[channel] = 0;

    AdLibVoices[voice].status = NOTE_OFF;

    port = AdLibVoices[voice].port;
    voc  = (voice >= VOICE_COUNT) ? voice - VOICE_COUNT : voice;

    AL_SendOutput(port, 0xB0 + voc, AdLibVoices[voice].pitchleft.b.bytehigh);

    AL_Remove(&AdLibChannels[channel].Voices, &AdLibVoices[voice]);
    AL_AddToTail(&Voice_Pool, &AdLibVoices[voice]);
}

void AL_NoteOn (uint8_t channel, uint8_t key, int8_t volume) {
    int8_t voice;
   OPL2Instrument *instr;

    // We only play channels 1 through 10
    if (channel > MAX_MUS_CHANNEL && channel != PERCUSSION_CHANNEL) {
        printf("bad note on channel %i\n", channel);
        return;
    }

    // turn off note
    if (volume == 0) {
        printf("volume 0 note on?\n");
        AL_NoteOff(channel, key);
        return;
    }

    voice = AL_AllocVoice(); // removes voice from free voice pool

    if (voice == VOICE_NOT_FOUND){
        // kill oldest channel?
        
        if (AdLibChannels[PERCUSSION_CHANNEL].Voices.start) {
            AL_NoteOff(PERCUSSION_CHANNEL, AdLibChannels[PERCUSSION_CHANNEL].Voices.start->key);
            voice = AL_AllocVoice();
        }
        printf ("killed channel?");
        
        if (voice == VOICE_NOT_FOUND) {
            printf("voice not found! B");
            return;
        }
    } else {
        // FILE* fp = fopen ("log2.txt", "ab");
        // fprintf(fp, "PLAY NOTE %i %i %i %i \n", channel, key, volume, voice);
        // fclose(fp);    
    }

    //printf("%i ON\t", channel);
    channelonoff[channel] = 1;

    /*
    printf("\nfound voice %hhx %hhx %hhx (%hhx) %hhx\n", key, channel, volume, 
        volume == -1 ? AdLibChannels[channel].LastVolume : volume,
        voice);
        */


    AdLibVoices[voice].key      = key;
    AdLibVoices[voice].channel  = channel;
    // last volume if -1
    volume = volume == -1 ? AdLibChannels[channel].LastVolume : volume;

    AdLibChannels[channel].RealVolume = calcVolume([channel], 256, volume);


    AdLibVoices[voice].velocity = volume;
    AdLibVoices[voice].status   = NOTE_ON;
    AdLibChannels[channel].LastVolume = volume;

    AL_AddToTail(&AdLibChannels[channel].Voices, &AdLibVoices[voice]);

    AL_SetVoiceTimbre(voice);
    AL_SetVoiceVolume(voice);
    AL_SetVoicePitch(voice);    // set freq..
    //AL_SetVoicePan(voice);


}



/*---------------------------------------------------------------------
   Function: AL_AllNotesOff

   Turns off all currently playing voices.
---------------------------------------------------------------------*/

void AL_AllNotesOff(uint8_t channel){
    while (AdLibChannels[channel].Voices.start != NULL) {
        AL_NoteOff(channel, AdLibChannels[channel].Voices.start->key);
    }
}



/*---------------------------------------------------------------------
   Function: AL_SetChannelVolume

   Sets the volume of the specified MIDI channel.
---------------------------------------------------------------------*/

void AL_SetChannelVolume (uint8_t channel, uint8_t volume){
    AdLibVoice *voice;

    volume &= AL_MaxVolume;
    AdLibChannels[channel].LastVolume = AdLibChannels[channel].Volume;
    AdLibChannels[channel].Volume = volume;

    voice = AdLibChannels[channel].Voices.start;
    while(voice != NULL){
        AL_SetVoiceVolume(voice->num);
        voice = voice->next;
    }
}


/*---------------------------------------------------------------------
   Function: AL_SetChannelPan

   Sets the pan position of the specified MIDI channel.
---------------------------------------------------------------------*/

void AL_SetChannelPan (uint8_t channel, uint8_t pan){

    // Don't pan drum sounds
    if (channel != PERCUSSION_CHANNEL) {
        AdLibVoice *voice;
        AdLibChannels[channel].Pan = pan;
        voice = AdLibChannels[channel].Voices.start;
        while(voice != NULL){
            // todo impl
            //AL_SetVoicePan(voice->num);
            voice = voice->next;
        }
    }
}


/*---------------------------------------------------------------------
   Function: AL_SetChannelDetune

   Sets the stereo voice detune of the specified MIDI channel.
---------------------------------------------------------------------*/

void AL_SetChannelDetune (uint8_t channel, uint8_t detune){
   AdLibChannels[channel].Detune = detune;
}


/*---------------------------------------------------------------------
   Function: AL_ProgramChange

   Selects the instrument to use on the specified MIDI channel.
---------------------------------------------------------------------*/

void AL_ProgramChange (uint8_t channel, uint8_t patch){

   // We only play channels 1 through 10
    if (channel > CHANNEL_COUNT){
        return;
    }

    AdLibChannels[channel].Timbre  = patch;
}




/*---------------------------------------------------------------------
   Function: AL_ResetVoices

   Sets all voice info to the default state.
---------------------------------------------------------------------*/

void AL_ResetVoices(){
    uint8_t index;

    Voice_Pool.start = NULL;
    Voice_Pool.end   = NULL;

    for(index = 0; index < VOICE_COUNT * 2; index++) {
        if (AdLibVoiceReserved[index] == false){  // todo whats this
            AdLibVoices[index].num = index;
            AdLibVoices[index].key = 0;
            AdLibVoices[index].velocity = 0;
            AdLibVoices[index].channel = 0xFF;
            AdLibVoices[index].timbre = 0xFF;
            AdLibVoices[index].port = (index < VOICE_COUNT) ? 0 : 1;
            AdLibVoices[index].status = NOTE_OFF;
            AL_AddToTail(&Voice_Pool, &AdLibVoices[index]);
        }
    }

    for(index = 0; index < CHANNEL_COUNT; index++){
        AdLibChannels[index].Voices.start    = NULL;
        AdLibChannels[index].Voices.end      = NULL;
        AdLibChannels[index].Timbre          = 0;
        AdLibChannels[index].Pitchbend       = 0;
        AdLibChannels[index].KeyOffset       = 0;
        AdLibChannels[index].KeyDetune       = 0;
        AdLibChannels[index].Volume          = AL_DefaultChannelVolume;
        AdLibChannels[index].LastVolume      = AL_DefaultChannelVolume;
        AdLibChannels[index].Pan             = 64;
        AdLibChannels[index].RPN             = 0;
        AdLibChannels[index].PitchBendRange     = AL_DefaultPitchBendRange;
        AdLibChannels[index].PitchBendSemiTones = AL_DefaultPitchBendRange / 100;
        AdLibChannels[index].PitchBendHundreds  = AL_DefaultPitchBendRange % 100;
    }
}



/*---------------------------------------------------------------------
   Function: AL_FlushCard

   Sets all voices to a known (quiet) state.
---------------------------------------------------------------------*/

void AL_FlushCard(){

    int8_t i;

    for(i = 0 ; i < VOICE_COUNT; i++) {
        if (AdLibVoiceReserved[i] == false) {
            int16_t slot1 = offsetSlot[slotVoice[i]];

            AL_SendOutputToPort(ADLIB_PORT, 0xA0 + i, 0);
            AL_SendOutputToPort(ADLIB_PORT, 0xB0 + i, 0);   // key off

            AL_WriteChannel(0, 0xE0 + slot1, 0, 0);

            // Set the envelope to be fast and quiet
            AL_WriteChannel(0, 0x60 + slot1, 0xff, 0xff);    // attack/decay max
            AL_WriteChannel(0, 0x80 + slot1, 0xff, 0xff);    // release max

            // Maximum attenuation
            AL_WriteChannel(0, 0x40 + slot1, 0xff, 0xff);
        }
    }
}



/*---------------------------------------------------------------------
   Function: AL_StereoOn

   Sets the card send info in stereo.
---------------------------------------------------------------------*/

void AL_StereoOn() {
    // Set card to OPL3 operation
    AL_SendOutputToPort(AL_RightPort, 0x5, 1);
    AdLibStereoOn = true;
}


/*---------------------------------------------------------------------
   Function: AL_StereoOff

   Sets the card send info in mono.
---------------------------------------------------------------------*/

void AL_StereoOff() {
    // Set card back to OPL2 operation
    AL_SendOutputToPort(AL_RightPort, 0x5, 0);
    AdLibStereoOn = false;
}


/*---------------------------------------------------------------------
   Function: AL_Reset

   Sets the card to a known (quiet) state.
---------------------------------------------------------------------*/

void AL_Reset(void) {
    AL_SendOutputToPort(ADLIB_PORT, 1, 0x20);       // enable waveform select
    //AL_SendOutputToPort(ADLIB_PORT, 0x08, 0x40);    // turn off CSW mode
    AL_SendOutputToPort(ADLIB_PORT, 0x08, 0);     // (original apogee lib)

    // Set the values: AM Depth, VIB depth & Rhythm
    AL_SendOutputToPort(ADLIB_PORT, 0xBD, 0);

    AL_StereoOn();

    AL_FlushCard();
    //AL_FlushCard(AL_LeftPort);
    //AL_FlushCard(AL_RightPort);
}




int16_t AL_InitSynth() {
    memset(AdLibVoiceLevels,   0, sizeof(AdLibVoiceLevels));
    memset(AdLibVoiceKsls,     0, sizeof(AdLibVoiceKsls));
    memset(AdLibVoiceReserved, 0, sizeof(AdLibVoiceReserved));
    memset(AdLibVoices,        0, sizeof(AdLibVoices));
    memset(&Voice_Pool,        0, sizeof(Voice_Pool));
    memset(AdLibChannels,      0, sizeof(AdLibChannels));

    //AL_CalcPitchInfo();
    AL_Reset();
    AL_ResetVoices();

    return 1;
}

/*---------------------------------------------------------------------
   Function: AL_Init

   Begins use of the sound card.
---------------------------------------------------------------------*/

int16_t AL_Init (uint16_t rate) {

/*
    if (fm_init(rate)){
        return 0;
    }
*/


    // fat timbre contents are ADLIB_TimbreBank by default
    //memcpy(&ADLIB_TimbreBank, &FatTimbre, sizeof(FatTimbre));    
    //AL_LoadBank("APOGEE.TMB");
    
    return AL_InitSynth();
}


int16_t AL_Detect(){

    uint8_t status1;
    uint8_t status2;
    uint8_t i;

    AL_SendOutputToPort(ADLIB_PORT, 0x04, 0x60);
    AL_SendOutputToPort(ADLIB_PORT, 0x04, 0x80);
    status1 = inp(ADLIB_PORT) & 0xE0;
    AL_SendOutputToPort(ADLIB_PORT, 0x02, 0xFF);
    AL_SendOutputToPort(ADLIB_PORT, 0x04, 0x21);
    
    for (i = 0; i < 100; i++){
	    inp(ADLIB_PORT);          // delay
    }

    status2 = inp(ADLIB_PORT) & 0xE0;
    AL_SendOutputToPort(ADLIB_PORT, 0x04, 0x60);
    AL_SendOutputToPort(ADLIB_PORT, 0x04, 0x80);

    return (status1 == 0 && status2 == 0xC0);
}



// custom tmb handling

/*---------------------------------------------------------------------
   Function: AL_RegisterTimbreBank

   Copies user supplied timbres over the default timbre bank.
---------------------------------------------------------------------*/
/*
void AL_RegisterTimbreBank(uint8_t *timbres){

   
   int16_t i;

   for (i = 0; i < 256; i++) {
        ADLIB_TimbreBank[i].SAVEK[0]   = *(timbres++);
        ADLIB_TimbreBank[i].SAVEK[1]   = *(timbres++);
        ADLIB_TimbreBank[i].Level[0]   = *(timbres++);
        ADLIB_TimbreBank[i].Level[1]   = *(timbres++);
        ADLIB_TimbreBank[i].Env1[0]    = *(timbres++);
        ADLIB_TimbreBank[i].Env1[1]    = *(timbres++);
        ADLIB_TimbreBank[i].Env2[0]    = *(timbres++);
        ADLIB_TimbreBank[i].Env2[1]    = *(timbres++);
        ADLIB_TimbreBank[i].Wave[0]    = *(timbres++);
        ADLIB_TimbreBank[i].Wave[1]    = *(timbres++);
        ADLIB_TimbreBank[i].Feedback   = *(timbres++);
        ADLIB_TimbreBank[i].Transpose  = *(int8_t *)(timbres++);
        ADLIB_TimbreBank[i].Velocity   = *(int8_t *)(timbres++);
    }
}

bool AL_LoadBank(const char *filename) {
    FILE *tmb = fopen(filename,"rb");
    if (tmb) {
        fseek(tmb,0,SEEK_END);
        int size = ftell(tmb);
        rewind(tmb);
        if(size == 256*13) {
            unsigned char timbre[256*13];
            fread(timbre,1,256*13,tmb);
            AL_RegisterTimbreBank(timbre);
            fclose(tmb);
            return true;
        }
        fclose(tmb);
        return false;
    }
    return false;
}
*/

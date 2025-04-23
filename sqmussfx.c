#include <dos.h>
#include <conio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>
#include <graph.h>

#include <i86.h>
#include "sqcommon.h"
#include "sqmussfx.h"
#include <sys/types.h>
#include <string.h>
#include "DMX.H"
#include <signal.h>
#include <bios.h>
#include <ctype.h>
#include <malloc.h>

void SB_SetPlaybackRate(int16_t sample_rate);
void SB_DSP1xx_BeginPlayback();



void( __interrupt __far *SB_OldInt)(void);

SB_VoiceInfo        sb_voicelist[NUM_SFX_TO_MIX];

SB_VoiceInfo        sb_sfx_info[NUM_SFX_LUMPS];

int8_t* 			sfxfilename[NUM_SFX_LUMPS] = {"DSPODTH1.lmp", "DSBAREXP.lmp", "DSPODTH2.lmp", "DSPODTH3.lmp", "DSITMBK.lmp",
                                                  "DSBDCLS.lmp",  "DSBDOPN.lmp", "DSPLPAIN.lmp", "DSDMPAIN.lmp", "DSTELEPT.lmp"

                                                 };

#define MAX_VOLUME_SFX_FLAG 0x80




// actual variables that get set.
// todo: set from environment variable.
int16_t sb_port = -1;
int16_t sb_dma  = -1;
int16_t sb_irq  = -1;

int8_t sb_dma_16 = UNDEFINED_DMA;
int8_t sb_dma_8  = UNDEFINED_DMA;

int16_t     SB_IntController1Mask;
int16_t     SB_IntController2Mask;

byte __far* SB_DMABuffer;
uint16_t  SB_DMABufferSegment;



int8_t  SB_CardActive = false;
int16_t_union SB_DSP_Version;
uint8_t SB_MixerType = SB_TYPE_NONE;
uint8_t SB_OriginalMidiVolumeLeft = 255;
uint8_t SB_OriginalMidiVolumeRight = 255;
uint8_t SB_OriginalVoiceVolumeLeft = 255;
uint8_t SB_OriginalVoiceVolumeRight = 255;

byte __far * sbbuffer;


// uint16_t SB_MixMode = 0; //SB_STEREO;
// uint16_t SB_MixMode = SB_STEREO | SB_SIXTEEN_BIT;
// uint16_t SB_MixMode = SB_SIXTEEN_BIT;



uint8_t SB_Mixer_Status;




uint8_t 				current_sampling_rate = SAMPLE_RATE_11_KHZ_FLAG;
uint8_t 				last_sampling_rate	  = SAMPLE_RATE_11_KHZ_FLAG;
int8_t 					change_sampling_to_22_next_int = 0;
int8_t 					change_sampling_to_11_next_int = 0;

int8_t   				in_first_buffer  = true;




void SB_Service_Mix22Khz(){
	
	int8_t i;
	int8_t remaining_22khz = false;	

	int8_t sound_played = 0;	// first sound copies. 2nd and more add. if no sounds played, clear buffer.
    uint16_t extra_zero_length = 0;
    uint8_t __far *extra_zero_copy_target;

	for (i = 0; i < NUM_SFX_TO_MIX; i++){

		if (!sb_voicelist[i].playing){
			// printf("sound done!");
		} else if (sb_voicelist[i].volume == 0){
            // dont play mute sound
		} else {

			// Keep track of current buffer
			//printf("\nPlaying %lx size is %x", SB_CurrentDMABuffer, sfx_length);

			if (sb_voicelist[i].currentsample >= sb_voicelist[i].length){
				// sound done playing. 
				// printf(" end sound!");
				if (sb_voicelist[i].playing){
                    sb_voicelist[i].playing = false;
                }

			} else {
                uint16_t copy_length = SB_TransferLength;
                uint16_t copy_offset;
                int8_t   do_second_copy = false;

                if (sb_voicelist[i].samplerate){
                    remaining_22khz = true;
                }
                
                // if not the first copy, just copy to the next buffer
                if (sb_voicelist[i].currentsample){
                    // copy only to doubled buffer
                    if (in_first_buffer){
                        copy_offset = SB_TransferLength;    
                    } else {
                        copy_offset = 0;
                    }
                // if the first copy, copy two buffers worth.
                } else {
                    // double buffer for first write!
                    // detect the run-over over the dma end buffer...
                    
                    if (!in_first_buffer){
                        do_second_copy = true;
                        copy_offset = SB_TransferLength;
                    } else {
                        copy_length = SB_DoubleBufferLength;
                        copy_offset = 0;
                    }

                }

                // stupid c89
                {                
                    uint8_t __far * dma_buffer = MK_FP(SB_DMABufferSegment, copy_offset);
                    uint8_t __far * source  = sb_voicelist[i].location + sb_voicelist[i].currentsample;
                    uint16_t remaining_length = sb_voicelist[i].length - sb_voicelist[i].currentsample;
                    int8_t volume = sb_voicelist[i].volume;
                    while (true){

                        if (remaining_length < copy_length){
                            if (sound_played == 0){
                                extra_zero_length = copy_length - remaining_length;
                                extra_zero_copy_target = dma_buffer + remaining_length;
                            }
                            copy_length = remaining_length;
                        }



                        // MANUAL MIX?

                        // volume is 0-127. if 128+ then use full volume.                                         
                        if (volume & MAX_VOLUME_SFX_FLAG){
                            // max volume. just use the bytes directly
                            if (!sound_played){
                                // first sound copied...
                                
                                if (sb_voicelist[i].samplerate){
                                    _fmemcpy(dma_buffer, source, copy_length);
                                } else {
                                    int8_t j;
                                    for (j = 0; j < copy_length/2; j++){
                                        dma_buffer[2*j]   = source[j];
                                        dma_buffer[2*j+1] = source[j];
                                    }
                                }

                                if (extra_zero_length){
                                    printf("\nzeroing extra %i", extra_zero_length);
                                    _fmemset(dma_buffer + copy_length, 0x80, extra_zero_length);
                                }


                            } else {
                                uint16_t j;
                                // subsequent sounds added
                                // obviously needs imrpovement...
                                if (sb_voicelist[i].samplerate){
                                    for (j = 0; j < copy_length; j++){
                                        int16_t total = dma_buffer[j] + source[j];
                                        dma_buffer[j] = total >> 1;
                                    }
                                } else {
                                    for (j = 0; j < copy_length/2; j++){
                                        int16_t total = dma_buffer[2*j] + source[j];
                                        dma_buffer[2*j] = total >> 1;
                                        total = dma_buffer[2*j+1] + source[j];
                                        dma_buffer[2*j+1] = total >> 1;
                                    }
                                }
                            }
                        } else {
                            // DO VOLUME MIX

                            if (!sound_played){
                                // first sound copied...
                                uint16_t j;
                                
                                if (sb_voicelist[i].samplerate){


                                    for (j = 0; j < copy_length; j++){
                                        int16_t_union total;
                                        int8_t intermediate = (source[j] - 0x80);
                                        total.h = FastIMul8u8u(volume, intermediate) << 1;
                                        dma_buffer[j] = 0x80 + total.bu.bytehigh;

                                        // dma_buffer[j] = 0x80 + total.bu.bytehigh;   // divide by 256 means take the high byte
                                    }
                                } else {
                                    for (j = 0; j < copy_length/2; j++){
                                        int16_t_union total;
                                        int8_t intermediate = (source[j] - 0x80);
                                        uint8_t result;
                                        total.h = FastIMul8u8u(volume, intermediate) << 1;
                                        result = 0x80 + total.bu.bytehigh;
                                        // todo word copy
                                        dma_buffer[2*j] = result;
                                        dma_buffer[2*j+1] = result;

                                        // dma_buffer[j] = 0x80 + total.bu.bytehigh;   // divide by 256 means take the high byte
                                    }
                                }

                            } else {
                                uint16_t j;
                                // subsequent sounds added
                                // obviously needs imrpovement...
                                if (sb_voicelist[i].samplerate){

                                    for (j = 0; j < copy_length; j++){
                                        int16_t_union total;
                                        int8_t intermediate = (source[j] - 0x80);
                                        total.h = FastIMul8u8u(volume, intermediate) << 1;
                                        total.bu.bytehigh += 0x80;

                                        dma_buffer[j] = (dma_buffer[j] + total.bu.bytehigh) >> 1;
                                        

                                    }
                                } else {
                                    for (j = 0; j < copy_length/2; j++){
                                        int16_t_union total;
                                        int8_t intermediate = (source[j] - 0x80);
                                        total.h = FastIMul8u8u(volume, intermediate) << 1;
                                        total.bu.bytehigh += 0x80;

                                        dma_buffer[2*j] = (dma_buffer[2*j]    + total.bu.bytehigh) >> 1;
                                        dma_buffer[2*j+1] = (dma_buffer[2*j+1] + total.bu.bytehigh) >> 1;
                                        

                                    }
                                }

                            }
                        }

                        if (!do_second_copy){
                            break;
                        }

                        // for when we are doing the first copy, but it runs over the edge of buffer and 
                        // we must reset to write to the start of the buffer for the 2nd buffer's write
                        
                        // todo generalize to larger #s than transfer length?
                        do_second_copy = false;
        				// printf("\ncaught while true!");
                        remaining_length -= copy_length;
                        if (!remaining_length){
                            break;  // if the sfx was less than a sample long i guess.
                        }
                        dma_buffer = MK_FP(SB_DMABufferSegment, 0); // to start of buffer.
                        source     += copy_length;
                        sb_voicelist[i].currentsample += copy_length;
                    }
                }
                sound_played++;
				// printf(" %lx %x %i", MK_FP(SB_DMABufferSegment, SB_CurrentDMABufferOffset), SB_DMABufferEndOffset, SB_CurrentDMABufferOffset);
                if (sb_voicelist[i].samplerate){
                    sb_voicelist[i].currentsample += copy_length;
                } else {
                    sb_voicelist[i].currentsample += copy_length >> 1;
                }

			}


		}

		// Call the caller's callback function
		// if (SB_CallBack != NULL) {
		//     MV_ServiceVoc();
		// }

		// send EOI to Interrupt Controller

	}	// end for loop

    if (!sound_played){
        _fmemset(MK_FP(SB_DMABufferSegment, 0), 0x80, SB_TotalBufferSize);
    } else if ( sound_played == 1){
        if (extra_zero_length){
            printf("\nzeroing extra %i", extra_zero_length);
            _fmemset(extra_zero_copy_target, 0x80, extra_zero_length);
        }

    }


	if (!remaining_22khz){
		change_sampling_to_11_next_int = true;
		printf("\n turning on 11 khz next int");

	}

}


void SB_Service_Mix11Khz(){
	int8_t i;
	int8_t sound_played = 0;	// first sound copies. 2nd and more add. if no sounds played, clear buffer.
    uint16_t extra_zero_length = 0;
    uint8_t __far *extra_zero_copy_target;

	for (i = 0; i < NUM_SFX_TO_MIX; i++){

		if (!sb_voicelist[i].playing){
			// printf("sound done!");
		} else if (sb_voicelist[i].volume == 0){
            // dont play mute sound
		} else {

			
			// Keep track of current buffer
			//printf("\nPlaying %lx size is %x", SB_CurrentDMABuffer, sfx_length);

			if (sb_voicelist[i].currentsample >= sb_voicelist[i].length){
				// sound done playing. 
				// printf(" end sound!");
				if (sb_voicelist[i].playing){
                    sb_voicelist[i].playing = false;
                }

			} else {
                uint16_t copy_length = SB_TransferLength;
                uint16_t copy_offset;
                int8_t   do_second_copy = false;
                
                // if not the first copy, just copy to the next buffer
                if (sb_voicelist[i].currentsample){
                    // copy only to doubled buffer
                    if (in_first_buffer){
                        copy_offset = SB_TransferLength;    
                    } else {
                        copy_offset = 0;
                    }
                // if the first copy, copy two buffers worth.
                } else {
                    // double buffer for first write!
                    // detect the run-over over the dma end buffer...
                    
                    if (!in_first_buffer){
                        do_second_copy = true;
                        copy_offset = SB_TransferLength;
                    } else {
                        copy_length = SB_DoubleBufferLength;
                        copy_offset = 0;
                    }

                }

                // stupid c89
                {                
                    uint8_t __far * dma_buffer = MK_FP(SB_DMABufferSegment, copy_offset);
                    uint8_t __far * source  = sb_voicelist[i].location + sb_voicelist[i].currentsample;
                    uint16_t remaining_length = sb_voicelist[i].length - sb_voicelist[i].currentsample;
                    int8_t volume = sb_voicelist[i].volume;
                    while (true){

                        if (remaining_length < copy_length){
                            if (sound_played == 0){
                                extra_zero_length = copy_length - remaining_length;
                                extra_zero_copy_target = dma_buffer + remaining_length;
                            }
                            copy_length = remaining_length;
                        }



                        // MANUAL MIX?

                        // volume is 0-127. if 128+ then use full volume.                                         
                        if (volume & MAX_VOLUME_SFX_FLAG){
                            // max volume. just use the bytes directly
                            if (!sound_played){
                                // first sound copied...
                                _fmemcpy(dma_buffer, source, copy_length);
                            } else {
                                uint16_t j;
                                // subsequent sounds added
                                // obviously needs imrpovement...
                                for (j = 0; j < copy_length; j++){
                                    int16_t total = dma_buffer[j] + source[j];
                                    dma_buffer[j] = total >> 1;
                                }

                            }

                        } else {
                            if (!sound_played){
                                // first sound copied...
                                uint16_t j;
                                
                                for (j = 0; j < copy_length; j++){
                                    int16_t_union total;
                                    int8_t intermediate = (source[j] - 0x80);
                                    total.h = FastIMul8u8u(volume, intermediate) << 1;
                                    dma_buffer[j] = 0x80 + total.bu.bytehigh;

                                    // dma_buffer[j] = 0x80 + total.bu.bytehigh;   // divide by 256 means take the high byte
                                }

                            } else {
                                uint16_t j;
                                // subsequent sounds added
                                // obviously needs imrpovement...
                                for (j = 0; j < copy_length; j++){
                                    int16_t_union total;
                                    int8_t intermediate = (source[j] - 0x80);
                                    total.h = FastIMul8u8u(volume, intermediate) << 1;
                                    total.bu.bytehigh += 0x80;

                                    dma_buffer[j] = (dma_buffer[j] + total.bu.bytehigh) >> 1;
                                    

                                }

                            }
                        }

                        if (!do_second_copy){
                            break;
                        }

                        // for when we are doing the first copy, but it runs over the edge of buffer and 
                        // we must reset to write to the start of the buffer for the 2nd buffer's write
                        
                        // todo generalize to larger #s than transfer length?
                        do_second_copy = false;
        				// printf("\ncaught while true!");
                        remaining_length -= copy_length;
                        if (!remaining_length){
                            break;  // if the sfx was less than a sample long i guess.
                        }
                        dma_buffer = MK_FP(SB_DMABufferSegment, 0); // to start of buffer.
                        source     += copy_length;
                        sb_voicelist[i].currentsample += copy_length;
                    }
                }
                sound_played++;
				// printf(" %lx %x %i", MK_FP(SB_DMABufferSegment, SB_CurrentDMABufferOffset), SB_DMABufferEndOffset, SB_CurrentDMABufferOffset);
                sb_voicelist[i].currentsample += copy_length;

			}


		}

		// Call the caller's callback function
		// if (SB_CallBack != NULL) {
		//     MV_ServiceVoc();
		// }

		// send EOI to Interrupt Controller

	}	// end for loop

    if (!sound_played){
        _fmemset(MK_FP(SB_DMABufferSegment, 0), 0x80, SB_TotalBufferSize);
    } else if ( sound_played == 1){
        if (extra_zero_length){
            printf("\nzeroing extra %i", extra_zero_length);
            _fmemset(extra_zero_copy_target, 0x80, extra_zero_length);
        }

    }

}



void __interrupt __far SB_ServiceInterrupt(void) {
	int8_t i;
	int8_t sample_rate_this_instance;
    if (in_first_buffer){
        in_first_buffer = false;
    } else {
        in_first_buffer = true;
    }

	if (change_sampling_to_22_next_int){
		change_sampling_to_22_next_int = 0;
		change_sampling_to_11_next_int = 0;
		if (current_sampling_rate == SAMPLE_RATE_11_KHZ_FLAG){
			current_sampling_rate = SAMPLE_RATE_22_KHZ_FLAG;
		}
		printf("\n turned on 22 khz");
		SB_SetPlaybackRate(SAMPLE_RATE_22_KHZ_UINT);
	} else if (change_sampling_to_11_next_int){
		change_sampling_to_11_next_int = 0;
		if (current_sampling_rate == SAMPLE_RATE_22_KHZ_FLAG){
			current_sampling_rate = SAMPLE_RATE_11_KHZ_FLAG;
		}
		printf("\n turned on 11 khz");
		SB_SetPlaybackRate(SAMPLE_RATE_11_KHZ_UINT);

	}

	sample_rate_this_instance = current_sampling_rate;

    // Acknowledge interrupt
    // Check if this is this an SB16 or newer
     if (SB_DSP_Version.hu >= SB_DSP_Version4xx) {
        outp(sb_port + SB_MixerAddressPort, 0x82);  //  MIXER_DSP4xxISR_Ack);

        SB_Mixer_Status = inp(sb_port + SB_MixerDataPort);
		//printf("\nmixer status %i %i", sb_port, SB_Mixer_Status);
        // Check if a 16-bit DMA interrupt occurred
        if (SB_Mixer_Status & MIXER_16BITDMA_INT) {
            // Acknowledge 16-bit transfer interrupt
            // printf (" 16bit! ");
			inp(sb_port + 0x0F);	// SB_16BitDMAAck
        } else if (SB_Mixer_Status & MIXER_8BITDMA_INT) {
            // printf (" 8bit! ");
            inp(sb_port + SB_DataAvailablePort);
        } else {


			// Wasn't our interrupt.  Call the old one.
			_chain_intr(SB_OldInt);
		
	    }
    } else {
        // Older card - can't detect if an interrupt occurred.
        inp(sb_port + SB_DataAvailablePort);
    }


	// Continue playback on cards without autoinit mode
	if (SB_DSP_Version.hu < SB_DSP_Version2xx) {
        if (SB_CardActive){
            SB_DSP1xx_BeginPlayback();
        }
	}



	// printf("\n and playing %lx", MK_FP(SB_DMABufferSegment, SB_CurrentDMABufferOffset));

	

	if (sample_rate_this_instance == SAMPLE_RATE_22_KHZ_FLAG){

//  22 KHZ MODE LOOP
//  22 KHZ MODE LOOP
//  22 KHZ MODE LOOP

		SB_Service_Mix22Khz();

	} else {

//  11 KHZ MODE LOOP
//  11 KHZ MODE LOOP
//  11 KHZ MODE LOOP


		SB_Service_Mix11Khz();
	}

	last_sampling_rate = current_sampling_rate;


    if (sb_irq > 7){
        outp(0xA0, 0x20);
    }

    outp(0x20, 0x20);
}


uint8_t volnumber = 60;



void SB_PlaySoundEffect(int8_t sfx_id){
	int8_t i;
    for (i = 0; i < NUM_SFX_TO_MIX;i++){
        if (sb_voicelist[i].playing == false){

            sb_voicelist[i].currentsample = 0;
            sb_voicelist[i].playing = true;
            sb_voicelist[i].samplerate = sb_sfx_info[sfx_id].samplerate;
            sb_voicelist[i].location   = sb_sfx_info[sfx_id].location;
            sb_voicelist[i].length     = sb_sfx_info[sfx_id].length;
            sb_voicelist[i].volume     = volnumber;
            
            // volume is 0-127
            printf("\nuse vol %i", volnumber);
            volnumber += 5;
            if (volnumber > 160){
                volnumber = 40;
            }
            // sb_voicelist[i].volume     = 255;
            if (sb_voicelist[i].samplerate){
                if (!current_sampling_rate){
                    change_sampling_to_22_next_int = 1;
                    printf("\nturning 22 khz mode on next int");
                }
            }


            return;
        }
    }
    printf("\nNO CHANNELS FOUND");
    

}



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

uint8_t SB_ReadDSP() {
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

int16_t SB_ResetDSP(){
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
 
    if (SB_DSP_Version.hu < SB_DSP_Version4xx){

        // Set playback rate
        if (sample_rate == SAMPLE_RATE_22_KHZ_UINT){
            SB_WriteDSP(0x40);
            // SB_WriteDSP(0xE9);  // 22
            SB_WriteDSP(0xD2);  // 22khz
        } else {
            SB_WriteDSP(0x40);
            // SB_WriteDSP(0xD2);  // 11khz
            SB_WriteDSP(0xA5);  // 11khz
        }


    } else{
        int16_t_union sample_rate_bytes;
        sample_rate_bytes.hu = sample_rate;
        // Set playback rate
        SB_WriteDSP(SB_DSP_Set_DA_Rate);
        SB_WriteDSP(sample_rate_bytes.bu.bytehigh);
        SB_WriteDSP(sample_rate_bytes.bu.bytelow);

        // Set recording rate
        SB_WriteDSP(SB_DSP_Set_AD_Rate);
        SB_WriteDSP(sample_rate_bytes.bu.bytehigh);
        SB_WriteDSP(sample_rate_bytes.bu.bytelow);
    }
}

void SB_SetMixMode(){
    // todo is this even needed?
/*
    //todo sb pro check

    //sb pro needs to set mixer to mono?
    uint8_t data;

    outp(sb_port+SB_MixerAddressPort, 0x0E);
    // make sure stereo is off
    data = inp(sb_port+SB_MixerDataPort);
    data &= ~0x02;  // turn off stereo flag...
    outp(sb_port+SB_MixerDataPort, 0x0E);
    SB_SetPlaybackRate(SAMPLE_RATE_11_KHZ_UINT);
    */


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



// todo hardcode these params, writes
void SB_DSP1xx_BeginPlayback() {
    int16_t_union sample_length;
	sample_length.hu = SB_MixBufferSize - 1;

    // Program DSP to play sound
    SB_WriteDSP(0x14);	// SB DAC 8 bit init, no autoinit
    SB_WriteDSP(sample_length.bu.bytelow);
    SB_WriteDSP(sample_length.bu.bytehigh);

    

}

void SB_DSP2xx_BeginPlayback() {

    int16_t_union sample_length;
	sample_length.hu = SB_MixBufferSize - 1;

    SB_WriteDSP(0x48);	// set block length
    SB_WriteDSP(sample_length.bu.bytelow);
    SB_WriteDSP(sample_length.bu.bytehigh);


	SB_WriteDSP(0x1C);	// SB DAC init, 8 bit auto init



}

void SB_DSP4xx_BeginPlayback() {
    int16_t_union sample_length;
	sample_length.hu = SB_MixBufferSize - 1;

	

    // Program DSP to play sound
    SB_WriteDSP(0xC6);	// 8 bit dac
    SB_WriteDSP(SB_DSP_UnsignedMonoData);	// transfer mode
    SB_WriteDSP(sample_length.bu.bytelow);
    SB_WriteDSP(sample_length.bu.bytehigh);


}

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

// todo do we need 16bit ports...? 
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
    //printf("channel used %i", channel);
	if (channel > DMA_MaxChannel_16_BIT) {
        return DMA_ERROR;
    } else if (channel == 2 || channel == 4) {	// invalid dma channels i guess
        return DMA_ERROR;
    }

    return DMA_OK;
}



int16_t DMA_SetupTransfer(uint8_t channel, byte __far* address, uint16_t length) {
    
    if (SB_DMA_VerifyChannel(channel) == DMA_OK) {


    	DMA_PORT __near* port = &DMA_PortInfo[channel];
        uint8_t  channel_select = channel & 0x3;
    	uint16_t transfer_length;
		fixed_t_union addr;
		
		addr.wu = (uint32_t)address;
		addr.hu.fracbits = addr.hu.fracbits + (addr.hu.intbits << 4) & 0xFFFF;  // equals offset (?)
		addr.hu.intbits = (addr.hu.intbits >> 4) & 0xFF00;		// equals page


        if (channel > 3) {	// 16 bit port
			addr.hu.fracbits = addr.hu.fracbits >> 1;	// shift offset. high bit is wrong, but doesnt affect our impl.

            // Convert the length in bytes to the length in words
            transfer_length = (length + 1) >> 1;

            // The length is always one less the number of bytes or words
            // that we're going to send
        } else {			// 8 bit port

			// offset already set.
            // The length is always one less the number of bytes or words
            // that we're going to send
            transfer_length = length;
        }

		transfer_length--;

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

		printf("\ndma addr %lx %x %x %x", addr, addr.bu.intbytehigh, addr.bu.fracbytehigh, addr.bu.fracbytelow);

        outp(port->address, addr.bu.fracbytelow);
        outp(port->address, addr.bu.fracbytehigh);

        // Send page
        outp(port->page, addr.bu.intbytehigh);

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

    // if (SB_MixMode & SB_SIXTEEN_BIT) {
        // dma_channel = sb_dma_16;
    // } else {
        dma_channel = sb_dma_8;
    // }

    if (dma_channel == UNDEFINED_DMA) {
        return SB_Error;
    }

    if (DMA_SetupTransfer(dma_channel, buffer, buffer_size) == DMA_ERROR) {
        return SB_Error;
    }

    sb_dma = dma_channel;

    SB_DMABuffer 				= buffer;
	SB_DMABufferSegment     	= FP_SEG(SB_DMABuffer);
    
    return SB_OK;
}



void SB_EnableInterrupt() {
    uint8_t mask;

    // Unmask system interrupt
    if (sb_irq < 8) {
        mask = inp(0x21) & ~(1 << sb_irq);
        outp(0x21, mask);
    } else {

        mask = inp(0xA1) & ~(1 << (sb_irq - 8));
        outp(0xA1, mask);

        mask = inp(0x21) & ~(1 << 2);
        outp(0x21, mask);
    }
}


int8_t SB_SetupPlayback(){
	// todo double?
    fixed_t_union addr;
	SB_StopPlayback();
    SB_SetMixMode();

    // todo why does malloc create garbage noise???
    addr.wu = (uint32_t) _fmalloc(2*SB_TotalBufferSize + 15);  // add 15 bytes...
    // round up to seg

    if (addr.wu & 0x0F){
        addr.wu += 0x10;
        addr.wu -= (addr.wu & 0x0F);
    }
    

    addr.hu.intbits += (addr.hu.fracbits >> 4);
    addr.hu.fracbits = 0;

    sbbuffer = (byte __far*)addr.wu;
    printf ("\n buffer %lx", sbbuffer);
    if (SB_SetupDMABuffer(sbbuffer, SB_TotalBufferSize)){
        return SB_Error;
    }
    printf ("  %x", SB_DMABufferSegment);
    _fmemset(MK_FP(SB_DMABufferSegment, 0), 0x80, SB_TotalBufferSize);

    SB_SetPlaybackRate(SAMPLE_RATE_11_KHZ_UINT);

    SB_EnableInterrupt();


	// Turn on Speaker
    SB_WriteDSP(0xD1);

    //SB_TransferLength = MixBufferSize; 
    
    //  Program the sound card to start the transfer.
    
	if (SB_DSP_Version.hu < SB_DSP_Version2xx) {
		SB_DSP1xx_BeginPlayback();
    } else if (SB_DSP_Version.hu < SB_DSP_Version4xx) {
        SB_DSP2xx_BeginPlayback();
    } else {
        SB_DSP4xx_BeginPlayback();
    }
    SB_CardActive = true;

    return SB_OK;


}

int8_t SB_DMA_EndTransfer(int8_t channel) {

    if (SB_DMA_VerifyChannel(channel) == DMA_OK) {

    // int Mask;	0x0A, 0xD4
    // int Mode;	0x0B, 0xD6
    // int Clear;	0x0C, 0xD8

        // Mask off DMA channel
        outp(channel < 4 ? 	0x0A: 0xD4, 4 | (channel & 0x3));

        // Clear flip-flop to lower byte with any data
        outp(channel < 4 ? 	0x0C: 0xD8, 0);
        printf("\nDMA Clear Channel OK");
		return DMA_OK;
    }

    return DMA_ERROR;
}

void SB_DisableInterrupt(){
    int mask;

    // Restore interrupt mask
    if (sb_irq < 8) {
        mask = inp(0x21) & ~(1 << sb_irq);
        mask |= SB_IntController1Mask & (1 << sb_irq);
        outp(0x21, mask);
    } else {
        mask = inp(0x21) & ~(1 << 2);
        mask |= SB_IntController1Mask & (1 << 2);
        outp(0x21, mask);

        mask = inp(0xA1) & ~(1 << (sb_irq - 8));
        mask |= SB_IntController2Mask & (1 << (sb_irq - 8));
        outp(0xA1, mask);
    }
}

void SB_StopPlayback(){

	SB_DisableInterrupt();

    SB_WriteDSP(SB_DSP_Halt8bitTransfer);   // halt command

    // Disable the DMA channel
    // if (SB_MixMode & SB_SIXTEEN_BIT){
        // SB_DMA_EndTransfer(sb_dma_16);
    // } else {
        SB_DMA_EndTransfer(sb_dma_8);
    // }

	SB_WriteDSP(0xD3);	// speaker off

    // sfx_playing = false;
    SB_DMABuffer = NULL;
    SB_CardActive = false;

}

/*
int8_t IRQ_RestoreVector(int8_t vector) {
    // Restore original interrupt handlers
    // DPMI set real mode vector
    regs.w.ax = 0x0201;
    regs.w.bx = vector;
    regs.w.cx = IRQ_RealModeSegment;
    regs.w.dx = IRQ_RealModeOffset;
    int386(0x31, &regs, &regs);

    regs.w.ax = 0x0205;
    regs.w.bx = vector;
    regs.w.cx = IRQ_ProtectedModeSelector;
    regs.x.edx = IRQ_ProtectedModeOffset;
    int386(0x31, &regs, &regs);

    // Free callback
    regs.w.ax = 0x304;
    regs.w.cx = IRQ_CallBackSegment;
    regs.w.dx = IRQ_CallBackOffset;
    int386x(0x31, &regs, &regs, &segregs);

    if (regs.x.cflag) {
        return 1;
    }

    return 0;
}
*/

#define SB_MIXER_DSP4xxISR_Ack 0x82
#define SB_MIXER_DSP4xxISR_Enable 0x83
#define SB_MIXER_MPU401_INT 0x4
#define SB_MIXER_16BITDMA_INT 0x2
#define SB_MIXER_8BITDMA_INT 0x1
#define SB_MIXER_DisableMPU401Interrupts 0xB
#define SB_MIXER_SBProOutputSetting 0x0E
#define SB_MIXER_SBProStereoFlag 0x02
#define SB_MIXER_SBProVoice 0x04
#define SB_MIXER_SBProMidi 0x26
#define SB_MIXER_SB16VoiceLeft 0x32
#define SB_SBProVoice 0x04
#define SB_MIXER_SB16VoiceRight 0x33
#define SB_MIXER_SB16MidiLeft 0x34
#define SB_MIXER_SB16MidiRight 0x35


uint8_t SB_ReadMixer(uint8_t reg) {
    outp(sb_port + SB_MixerAddressPort, reg);
    return inp(sb_port + SB_MixerDataPort);
}

void SB_WriteMixer(uint8_t reg,uint8_t data) {
    outp(sb_port + SB_MixerAddressPort, reg);
    outp(sb_port + SB_MixerDataPort, data);
}

void SB_SaveVoiceVolume() {
    switch (SB_MixerType) {
		case SB_TYPE_SBPro:
		case SB_TYPE_SBPro2:
			SB_OriginalVoiceVolumeLeft  = SB_ReadMixer(SB_MIXER_SBProVoice);
			break;

		case SB_TYPE_SB16:
			SB_OriginalVoiceVolumeLeft  = SB_ReadMixer(SB_MIXER_SB16VoiceLeft);
			SB_OriginalVoiceVolumeRight = SB_ReadMixer(SB_MIXER_SB16VoiceRight);
			break;
		}
}

void SB_RestoreVoiceVolume() {
    switch (SB_MixerType) {
		case SB_TYPE_SBPro:
		case SB_TYPE_SBPro2:
			SB_WriteMixer(SB_MIXER_SBProVoice, SB_OriginalVoiceVolumeLeft);
			break;

		case SB_TYPE_SB16:
			SB_WriteMixer(SB_MIXER_SB16VoiceLeft,  SB_OriginalVoiceVolumeLeft);
			SB_WriteMixer(SB_MIXER_SB16VoiceRight, SB_OriginalVoiceVolumeRight);
			break;
    }
}

void SB_Shutdown(){
    // sfx_playing = false;

	SB_StopPlayback();

    SB_RestoreVoiceVolume();
    SB_ResetDSP();

    // Restore the original interrupt		
    if (sb_irq >= 8) {
        // IRQ_RestoreVector(sb_int);
    }
	printf("\nRestored the interrupt %i %i %lx!", IRQ_TO_INTERRUPT_MAP[sb_irq], sb_irq, SB_OldInt);

    _dos_setvect(IRQ_TO_INTERRUPT_MAP[sb_irq], SB_OldInt);

    SB_DMABuffer = NULL;
    // SB_CallBack = null;
    // SB_Installed = false;


}



void SB_SetVolume(uint8_t volume){
    if (SB_MixerType == SB_TYPE_SB16) {
        SB_WriteMixer(SB_MIXER_SB16VoiceLeft, volume & 0xf8);
        SB_WriteMixer(SB_MIXER_SB16VoiceRight, volume & 0xf8);
  
    } else if (SB_MixerType == SB_TYPE_SBPro){
        SB_WriteMixer(SB_SBProVoice, (volume & 0xF) + (volume >> 4));

    } 
}




uint16_t SB_GetDSPVersion() {

    SB_WriteDSP(0xE1);	// get version

    SB_DSP_Version.bu.bytehigh = SB_ReadDSP();
    SB_DSP_Version.bu.bytelow  = SB_ReadDSP();

    if ((SB_DSP_Version.b.bytehigh == SB_Error) ||
        (SB_DSP_Version.b.bytelow  == SB_Error)) {
        return SB_Error;
    }

	// SB_DSP_Version.hu = 0x101;
    printf("DSP Version detected:  %x\n", SB_DSP_Version.hu);

    if (SB_DSP_Version.hu >= SB_DSP_Version4xx) {
        SB_MixerType = SB_TYPE_SB16;
    } else if (SB_DSP_Version.hu >= SB_DSP_Version3xx) {
        SB_MixerType = SB_TYPE_SBPro;
    } else if (SB_DSP_Version.hu >= SB_DSP_Version2xx) {
        SB_MixerType = SB_TYPE_NONE;
    } else {
        SB_MixerType = SB_TYPE_NONE;
    }

    return SB_DSP_Version.hu;
}

int16_t SB_InitCard(){
	int8_t status;

	//todo get these from environment variables or config file.
	sb_irq      = FIXED_SB_IRQ;
	sb_dma_8    = FIXED_SB_DMA_8;
	sb_dma_16   = FIXED_SB_DMA_16;
	sb_port 	= FIXED_SB_PORT;
	SB_MixerType = SB_TYPE_SB16;



    // Save the interrupt masks
    SB_IntController1Mask = inp(0x21);
    SB_IntController2Mask = inp(0xA1);
	status = SB_ResetDSP();

    if (status == SB_OK) {
		uint8_t sb_int;
		uint8_t used_dma;
		// sfx_playing = false;
		SB_GetDSPVersion();
        SB_SaveVoiceVolume();

        SB_SetPlaybackRate(SAMPLE_RATE_11_KHZ_UINT);
        SB_SetMixMode();

        // if (SB_Config.Dma16 != UNDEFINED)
        // {
        //     status = SB_DMA_VerifyChannel(SB_Config.Dma16);
        //     if (status == DMA_Error)
        //     {
        //         return (SB_Error);
        //     }
        // }
		
		// if (SB_MixMode & SB_SIXTEEN_BIT) {
			// used_dma = sb_dma_16;
		// } else {
			used_dma = sb_dma_8;
		// }

		if (SB_DMA_VerifyChannel(used_dma) == DMA_ERROR) {
			return SB_Error;
		}
		sb_dma = used_dma;
        // Install our interrupt handler
        
        if (!VALID_IRQ(sb_irq)) {
            return (SB_Error);
        }

		// todo make IRQ_TO_INTERRUPT_MAP logic handle 8 bit (single dma controller etc) machines right
        sb_int = IRQ_TO_INTERRUPT_MAP[sb_irq];
        if (sb_int == INVALID_IRQ) {
            return SB_Error;
        }


        SB_OldInt = _dos_getvect(sb_int);
        if (sb_irq < 8) {
			// 8 bit logic?
			printf("Set the interrupt %i %i %lx!", sb_int, sb_irq, SB_OldInt);
            _dos_setvect(sb_int, SB_ServiceInterrupt);
        } else {
			// 16 bit logic?
            // status = IRQ_SetVector(Interrupt, SB_ServiceInterrupt);
        }

        return  SB_OK;
    }


	return status;

}


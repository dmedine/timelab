#ifndef __tl_core_h_


#define tl_smp float // type of variable for synthesis
                   // since portaudio is all that's currently supported
                   // this is the only type we need for now

#define tl_name const char *// string to hold names 

     //*****************************************//
     //  scheduling and housekeeping functions  //
     //*****************************************//

// compute one block of samples for the 
// given pclass register
inline void dsp_tick(int samples, tl_pclass *x);

// put the above in a while(1) on its own thread 
// to do realtime dsp!!!

int tl_get_samplerate(void); // returns the samplerate
int tl_get_block_len(void); // returns block_len

void tl_set_samplerate(int samplerate); // returns the samplerate
void tl_set_block_len(int block_len); // returns block_len

static int tl_g_samplerate = 44100;
static int tl_g_block_len = 64;

#define __tl_core_h_
#endif

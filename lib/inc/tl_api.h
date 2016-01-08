#ifndef __tl_sigs_h_

/* this file belongs to */
/* timelab-0.10 by David Medine */

// this header specifies the global
// variables, data structures, and
// functions the comprise the timelab
// API









   //*******************//
   //       tl_sig      // 
   //*******************//
// class defining a tl signal

#define tl_sig_t int //macro for categorizing signal type
#define TL_OUTLET 0 //output type
#define TL_INLET 1  //input type

typedef struct _sig{

  tl_smp *smps;  // pointer to a block of samples
  int smp_cnt;     // # of samples in the block
  tl_sig_t type;  // whether this is an input or output
  int up;        // factor by which to upsample
 
}tl_sig;

// initialize one signal -- assume its an outlet
// because inlets don't require intialization
// 
tl_sig *init_one_sig(int block_len, int up);
// initialize plural signals
tl_sig **init_sigs(int sig_cnt, tl_sig_t sig_type, int block_len, int up);

// destroy one signal
void kill_one_sig(tl_sig *x);
// destroy plural signals
void kill_sigs(tl_sig **x, int sig_cnt);

// utilities for manipulatign signals
inline void set_sig_vals(tl_sig *x, tl_smp val);
inline void scale_sig_vals(tl_sig *x, tl_smp scalar);
// should scalar be a vector perhaps? 
inline void multiply_sigs(tl_sig *x, tl_sig *y);
inline void divide_sigs(tl_sig *x, tl_sig *y);
inline void add_sigs(tl_sig *x, tl_sig *y);
inline void subtract_sigs(tl_sig *x, tl_sig *y);
inline void copy_sigs(tl_sig *x, tl_sig *y);
inline void zero_out_sig(tl_sig *x);









// a_portaudio interface
int pa_get_device_count(void);

// call this to initialize portaudio
void pa_initialize(int out_devno, int in_devno, int out_channs, int in_channs, float latency);

long int get_pa_conversion_buff_len(void);

void pa_audio_on(void);
void pa_audio_off(void);


#define __tl_sigs_h_
#endif

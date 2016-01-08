#ifndef __a_pa_h_


#include "portaudio.h"
//#include "pthread.h"
#include "g_api.h"

//the above is the modified protaudio buffering utility that comes with Pd
//see the file itself (and s_audio_paring.c) for copyright info etc.
//#include "pthread.h" //included in api_rtsched.h

#define t_pa_stream_params struct PaStreamParameters;

int stream_is_open;

static PaStreamParameters  g_outstreamparams;
static PaStreamParameters  g_instreamparams;
PaStream *stream;

t_tlsmp *pa_conversion_buff;

pthread_mutex_t pa_mutex;
pthread_cond_t pa_sem;
pthread_t sched_thread;

void pa_set_output_device(int devno);
void pa_set_input_device(int devno);
void pa_set_output_channels(int channels);
void pa_set_input_channels(int channels);

void pa_audio_on(void);
static void pa_start_stream(PaStream *a_stream);
static void pa_stop_stream(PaStream *a_stream);
static void pa_close_stream(PaStream *a_stream);
void pa_audio_off(void);

static PaError pa_params_setup(void);//here we will get the device information 
//and implement it into data structures?
static int callback(const void *input_buffer, void *output_buffer,
	     unsigned long frames_per_buffer,
	     const PaStreamCallbackTimeInfo* time_info,
	     PaStreamCallbackFlags status_flags,
	     void *user_data );
//this function will pack the audio buffers in and out of
//port audio's callback function from my own dsp routines

static int no_tick_callback(const void *input_buffer, void *output_buffer,
		     unsigned long frames_per_buffer,
		     const PaStreamCallbackTimeInfo* time_info,
		     PaStreamCallbackFlags status_flags,
		     void *user_data );
//sister function to pa_do_fifos -- again, modelled on Pd
//the idea here is to at least keeep the call to tick the dsp 
//out of the callback function -- yeeesh!

void pa_sched_block(void);


int pa_device_count(void);
char *pa_devices(int device);

int audio_sync(void);

static PaStream *pa_open_stream(PaStreamCallback *callback);

//tutorial artifact
void pa_sleep(long ms);

#define  __a_pa_h_
#endif

//port audio stuff

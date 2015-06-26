#include "a_portaudio.h"
#include "tl_core.h"
#include "stdlib.h"
#include "stdio.h"

static PaStream *pa_open_stream(void){

#ifdef DEBUG
  printf("pa_open_stream\n");
#endif // DEBUG


  PaError err;
  const  PaDeviceInfo *pdi;
  int block_len = tl_get_block_len();
  int samplerate = tl_get_samplerate();
  int out_channs = pa_g_outstreamparams.channelCount;
  int in_channs = pa_g_instreamparams.channelCount;
  int nbuffers = 1;
  int i;

  printf("samplerate: %d\n", samplerate);

  /* filep1 = fopen("./so_buff", "w"); */
  /* filep2 = fopen("./pa_buff", "w"); */

  pa_conversion_buff = malloc(sizeof(tl_smp) * block_len * out_channs);
  pa_conversion_buff_len = block_len*out_channs;
  for(i=0; i<(block_len * out_channs); i++)
    pa_conversion_buff[i] = 0.0;
    
  /* Open an audio I/O stream. */
  err = Pa_OpenStream( 
		      &pa_stream,
		      &pa_g_instreamparams,         
		      &pa_g_outstreamparams, 
		      samplerate,
		      0, // is this right?
		      paNoFlag,
		      0, // no callback, 
		      0); // we don't need user data
  
  
  if( err != paNoError ) goto error;

 error:
  printf("open_stream PortAudio error: %s\n", Pa_GetErrorText( err ) );


}

static void pa_start_stream(void){
  
  PaError err;
  pa_streaming = 1;
  err = Pa_StartStream( pa_stream );
  if( err != paNoError ) goto error;
 
 error:
  printf(  "Start Stream PortAudio error: %s\n", Pa_GetErrorText( err ) );


}

// call this to interrupt a stream
void pa_stop_stream(void){

  PaError err;
 
  err = Pa_StopStream( pa_stream );
    
  if( err != paNoError ) goto error;
  
 error:
  printf(  "Stop Stream PortAudio error: %s\n", Pa_GetErrorText( err ) );

}

// call this when we are all done
void pa_close_stream(void){
  
  PaError err;
 
  err = Pa_CloseStream( pa_stream );
  pa_streaming = 0;  
  if( err != paNoError ) goto error;

 error:
  printf(  "Close Stream PortAudio error: %s\n", Pa_GetErrorText( err ) );

}

// utility for gui interaction
int pa_get_device_cnt(void){
  
  PaError err;
  if(pa_initialized == 0)
    {
      err = Pa_Initialize();
      if( err != paNoError )
	goto error;
    }


  pa_initialized = 1;

  return Pa_GetDeviceCount();
 error:
  printf(  "Audio On PortAudio error: %s\n", Pa_GetErrorText( err ) );
  return 0;

}

const char *pa_get_device_name(int devno){
  PaDeviceInfo *info;
  if(devno<=pa_get_device_cnt())
    return Pa_GetDeviceInfo(devno)->name;
  else
    {
      printf("error: pa_get_device_name: invalid devno %d\n", devno);
      return NULL;
    }

}


// this must be called before portaudio can do anything
void pa_initialize(int out_devno, int in_devno, int out_channs, int in_channs, float latency){

  PaError err;

  pa_audio_off();

  pa_g_outstreamparams.device = out_devno;
  pa_g_outstreamparams.channelCount = out_channs;
  pa_g_outstreamparams.sampleFormat = paFloat32;
  pa_g_outstreamparams.suggestedLatency = latency;
  pa_g_outstreamparams.hostApiSpecificStreamInfo = 0;

  pa_g_instreamparams.device = in_devno;
  pa_g_instreamparams.channelCount = in_channs;
  pa_g_instreamparams.sampleFormat = paFloat32;
  pa_g_instreamparams.suggestedLatency = latency;
  pa_g_instreamparams.hostApiSpecificStreamInfo = 0;

  //err = Pa_IsFormatSupported( pa_g_instreamparams, pa_g_outstreamparams, tl_get_samplerate());

  printf(  "pa_initialize: %s\n", Pa_GetErrorText( err ) );


}

void pa_initialize_strct(tl_a_info a_info){

  pa_initialize(a_info.outdevno, a_info.indevno, a_info.outchanns, a_info.inchanns, a_info.latency); 

}

long int get_pa_conversion_buff_len(void){

  return pa_conversion_buff_len;

}

void pa_audio_on(void){
 
  PaError err;
  int i;

  if(pa_streaming==1)
    pa_close_stream();

  if(pa_initialized ==0)
    {
      err = Pa_Initialize();
      if( err != paNoError )
  	goto error;
      pa_initialized = 1;

    }

  pa_open_stream();
  pa_start_stream();
  

 error:
  printf(  "pa_audio_on: Pa_Initialize error: %s\n", Pa_GetErrorText( err ) );

 
}

void pa_kill_conversion_buff(void){
    if(pa_conversion_buff!=NULL)
	{
	  free(pa_conversion_buff);
	  pa_conversion_buff = NULL;
	}
  
}

void pa_audio_suspend(void){

  PaError err;

  if(pa_initialized = !0)
    {  
      //pa_end_graceful(); //needs to thread and block in order to work properly
      
      /* err = Pa_Terminate(); */
      /* if( err != paNoError ) */
      /*   goto error; */
      Pa_Sleep(100);
      pa_close_stream();
      
      pa_initialized = 0;  
      // this is safe, but it might be better
      // not to free this until system exit 
    }
error:
    printf(  "Audio Suspend PortAudio error: %s\n", Pa_GetErrorText( err ) );


}

void pa_audio_off(void){

  PaError err;

  //pa_end_graceful(); //needs to thread and block in order to work properly



  pa_close_stream();
  
  pa_initialized = 0;  
  // this is safe, but it might be better
  // not to free this until system exit 
  

  err = Pa_Terminate();
  if( err != paNoError )
    goto error;

error:
    printf(  "Audio Off PortAudio error: %s\n", Pa_GetErrorText( err ) );

}

// public interfaces to these static variables
int pa_is_initialized(void){return pa_initialized;}
int pa_is_streaming(void){return pa_streaming;}



inline void pa_push_out(tl_smp *output_buff){


  int out_channs = pa_g_outstreamparams.channelCount;
  int block_len = tl_get_block_len();
  tl_smp *fp1, *fp2, *fp3;
  int i, j, k;

  // this is totally copied from Pd
  // this statement converts a buffer 
  // that is arranged [chann1 block][chann2 block]...[channn block]
  // into one that is interleaved, ie:
  // [chann1 samp1, chann2 samp1 ...] ... [chann1 sampn, chann2 sampn ...] 


  for(j=0, fp1=output_buff, fp2=pa_conversion_buff;
      j<out_channs; j++, fp2++)
    for(k=0, fp3=fp2; k<block_len;
	k++, fp1++, fp3+=out_channs)
      *fp3 = *fp1;
  
  // write the stream to the output
  Pa_WriteStream(pa_stream, pa_conversion_buff, block_len);

}

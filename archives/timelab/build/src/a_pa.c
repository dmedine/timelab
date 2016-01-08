#include "a_pa.h"
#include "stdio.h"
#include "stdlib.h"
#include "math.h"
#include "g_api.h"
#include "unistd.h"

FILE *filep1, *filep2;

long pa_counter = 0;
long sched_counter = 0;
int pa_initialized = 0;
int pa_streaming = 0;

int default_params = 1;

static void pa_get_instreamparams(int devno, int channels){

  g_instreamparams.device = devno;
  g_instreamparams.channelCount = channels;//2;//inchannels;
  g_instreamparams.sampleFormat = paFloat32;
  g_instreamparams.suggestedLatency = .025;
  g_instreamparams.hostApiSpecificStreamInfo = 0;

}

static void pa_get_outstreamparams(int devno, int channels){
  //printf("pa_get_outstreamparams\n");
  //printf("g_outchannels : %d (pa_get_outstreamparams)\n");
  printf("channels : %d\n", channels);
  g_outstreamparams.device = devno;
  g_outstreamparams.channelCount = channels;//2;//outchannels;
  g_outstreamparams.sampleFormat = paFloat32;
  g_outstreamparams.suggestedLatency = .025;
  g_outstreamparams.hostApiSpecificStreamInfo = 0;

}

void pa_set_output_device(int devno){

  g_outdevno = devno;

}

void pa_set_input_device(int devno){

    g_indevno = devno;

}

void pa_set_output_channels(int channels){
  //TODO: implement safeguards -- we can't exceed maximum channels
  g_outchannels = channels;

}

void pa_set_input_channels(int channels){

    g_inchannels = channels;

}

void pa_set_devices(void){

  pa_audio_off();
  nanosleep(&g_sleep_time, &g_empty_time);
  pa_audio_on();

}

void pa_audio_on(){

  PaError err;
  int i;
  float *fp;
  err = Pa_Initialize();
  if( err != paNoError )
    goto error;

  //stream = pa_open_stream(callback);
  stream = pa_open_stream(0);
  pa_start_stream(stream);
  g_dac_done = 1;

 error:
  printf(  "Audio On PortAudio error: %s\n", Pa_GetErrorText( err ) );
 
}

void pa_end_graceful(void){
  
  int i, j;
  int its = 5;
  t_tlsmp cur = g_soundout->buff[g_dsp_block_len-1];
  t_tlsmp diff = cur - 0.0;
  t_tlsmp ramp_delta = diff/((t_tlsmp)its * g_dsp_block_len);
 
  for(i=0; i<its; i++)
    {
      for(j=0; j<g_dsp_block_len; j++)
	{
	  cur+=ramp_delta;
	  g_soundout->buff[j] = cur;
	}
      pa_sched_block();
    }
}

void pa_audio_off(){
  PaError err;

  //pa_end_graceful(); //needs to thread and block in order to work properly

  err = Pa_Terminate();
  if( err != paNoError )
    goto error;

  pa_stop_stream(stream);
  pa_close_stream(stream);
  if(pa_conversion_buff)
    {
      free(pa_conversion_buff);
      pa_conversion_buff = NULL;
    }

error:
    printf(  "Audio Off PortAudio error: %s\n", Pa_GetErrorText( err ) );


}


static void pa_start_stream(PaStream *a_stream){
  PaError err;
  pa_streaming = 1;
  err = Pa_StartStream( a_stream );
  if( err != paNoError ) goto error;
 

 error:
  printf(  "Start Stream PortAudio error: %s\n", Pa_GetErrorText( err ) );

}

static void pa_stop_stream(PaStream *a_stream){
  printf("pa_stop_stream\n");

  PaError err;
  err = Pa_AbortStream( a_stream );
  if( err != paNoError ) goto error;

  /* err = Pa_StopStream (stream); */
  /* if( err != paNoError ) goto error; */
  
 error:
  printf(  "Stop Stream PortAudio error: %s\n", Pa_GetErrorText( err ) );
}

static void pa_close_stream(PaStream *a_stream){
  PaError err;

 
  err = Pa_CloseStream( stream );
  if( err != paNoError ) goto error;



 error:
  printf(  "Close Stream PortAudio error: %s\n", Pa_GetErrorText( err ) );

}

void pa_sched_block(void){

  t_tlsmp *fp1, *fp2, *fp3;
  int i, j, k;
  //if(pa_sched_block)
  //{
      //write output -- interleaved
      if(g_outchannels > 0)
	for(j=0, fp1=g_soundout->buff, fp2=pa_conversion_buff; 
	    j<g_outchannels; j++, fp2++)
	  for(k=0, fp3=fp2; k<g_soundout->block_len;
	      k++, fp1++, fp3+=g_outchannels)
	    *fp3 = *fp1;
      /* for(i=0; i<g_soundout_buff_len; i++) */
      /*   { */
      /*     printf("%d  :  %f\t", i, g_soundout_buff[i]); */
      /*     printf("%f\n", pa_conversion_buff[i]); */
      /*   } */
      Pa_WriteStream(stream, pa_conversion_buff, g_soundout->block_len);
      //}
}






static PaStream *pa_open_stream(PaStreamCallback *cb){
  printf("pa_open_stream\n");
  PaError err;
  PaStream *new_stream;
  int devno;
  const  PaDeviceInfo *pdi;
  int frames_per_buffer = g_soundout->block_len;
  int nbuffers = 1;
  int i;

  /* filep1 = fopen("./so_buff", "w"); */
  /* filep2 = fopen("./pa_buff", "w"); */

  pa_get_instreamparams(g_indevno, g_inchannels);
  pa_get_outstreamparams(g_outdevno, g_outchannels);

  pa_conversion_buff = malloc(sizeof(t_tlsmp) * g_soundout->block_len * g_outchannels);
  for(i=0; i<(g_soundout->block_len * g_outchannels); i++)
    pa_conversion_buff[i] = 0.0;
  printf("pa_conversion_buff_len : %d\n", g_outchannels * g_soundout->block_len);
  
  /* Open an audio I/O stream. */
  err = Pa_OpenStream( 
		      &new_stream,
		      &g_instreamparams,         
		      &g_outstreamparams, //we need a re_open function in case these change?
		      g_samplerate,
		      frames_per_buffer,
		      paNoFlag,
		      0,//callback,//no callback, 
		      0); //we don't need user data
  
  
  if( err != paNoError ) goto error;
  stream_is_open = 1;
  return(new_stream);
 error:
  printf("PortAudio error: %s\n", Pa_GetErrorText( err ) );
  return(NULL);

}



//for gui interaction
int pa_device_count(void){
  
  PaError err;
  if(!pa_initialized)
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

void pa_sleep(long ms){
  Pa_Sleep(ms);
}



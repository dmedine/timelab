#ifndef __a_portaudio_h_

#ifdef __cplusplus
extern "C" {
#endif

  /* this file belongs to */
  /* timelab-0.10 by David Medine */
  
  // this file declares the 
  // timelab portaudio interface
  // most of this code is portaudio boilerplate
  
#include "tl_core.h"
#include "portaudio.h"
  
  // these pointers are global, but must be initialized
  // before anything can be done
  PaStreamParameters pa_g_outstreamparams;
  PaStreamParameters pa_g_instreamparams;
  PaStream *pa_stream;
  tl_smp *pa_conversion_buff;
  
  static int pa_streaming = 0; // flag to know if we stream or not
  static int pa_initialized = 0; // did we initialize?
  
  static long int pa_conversion_buff_len = 0; // initialize to 0
  
  static PaStream *pa_open_stream(void);
  static void pa_start_stream(void);
  static void pa_stop_stream(void);
  static void pa_close_stream(void);
  
  // a_portaudio interface
  int pa_get_device_cnt(void);
  const char *pa_get_device_name(int devno);
  
  // call this to initialize portaudio
  void pa_initialize(int out_devno, int in_devno, int out_channs, int in_channs, float latency);
  void pa_initialize_strct(tl_a_info a_info);
  
  
  long int get_pa_conversion_buff_len(void);
  
  void pa_audio_on(void);
  void pa_audio_suspend(void);
  void pa_kill_conversion_buff(void);
  void pa_audio_off(void);
  int pa_is_initialized(void);
  int pa_is_streaming(void);
  
  //**************//
  // pa_push_out  //
  //**************//
  
  // this is the function that delivers samples to 
  // the hardware DAC
  inline void pa_push_out(tl_smp *output_buff);
  
  
#ifdef __cplusplus
}
#endif

#define __a_portaudio_h_
#endif

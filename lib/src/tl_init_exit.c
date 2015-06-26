#include "tl_init_exit.h"

void tl_init(void){

  // initialize control structure
  set_g_lvl_stck(init_lvl_stck());
  set_g_ctl_head(init_ctl(TL_HEAD_CTL));

  // now register the classes
  set_g_class_head(init_class());

  // initialize global variables (there must be others)
  tl_g_a_info_set_initialized();

}

// also a re-init function
void tl_audio_init(void){

  int i;

  // make sure we are off
  tl_dsp_off();
  pa_audio_suspend();


  /* if(tl_g_a_info_get_initialized()==1) */
  /*   { */
      // initialize portaudio
      pa_initialize_strct(tl_get_a_info());
      //pa_audio_on();

      // initialize the empty signal 
      tl_kill_empty_sig(); // make sure it's dead
      tl_init_empty_sig();
      

  /*   } */
  /* else printf("error: tl_audio_init: tl_g_a_info not initialized\n"); */

}

void tl_exit(void){


  tl_audio_off();
  pa_audio_suspend();
  pa_audio_off(); 
  pa_kill_conversion_buff(); 
  // kill the control structure
  tl_process_kill_list(get_g_class_head());
  // and its level stack
  kill_g_lvl_stck(get_g_lvl_stck());

}











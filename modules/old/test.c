/* this file belongs to */
/* timelab-0.10 by David Medine */

// this file is a test to evaluate
// the functionality of the timelab api
// and the portaudio module


#include "../lib/inc/tl_api.h"
#include "./inc/a_portaudio.h"
#include "math.h"
#include "stdlib.h"

int main(void){

  long int len;
  unsigned int i, j;
  tl_smp pi = 6.28;
  tl_smp *buff;

  // first test portaudio
  //pa_initialize(0, 0, 2, 2, .25);
  len = get_pa_conversion_buff_len();
  
  buff = (tl_smp*)malloc(sizeof(tl_smp) * len);
  for(i=0; i<len; i++)
    buff[i] = sin(440*2*pi);

  /* pa_audio_on(); */
  /* for(i=0; i<10000000; i+=2) */
  /*   pa_push_out(buff); */
    

  /* pa_audio_off(); */




  return 0;
}

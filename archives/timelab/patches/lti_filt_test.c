#include "stdio.h"
#include "math.h"
#include "stdlib.h"
#include "g_api.h"
#include "m_modules.h"
#include "pthread.h"
//asdf


t_tlsmp a[3];
t_tlsmp b[3];

static FILE *fp1, *fp2;

t_lti_filt *filter;
t_tlsig *impulse;
void setup_this(void){
  
  int i;
  a[0] = 1.0; a[1] = -0.9; a[2] = 0.0;
  b[0] = .1; b[1] = 0.0; b[2] = 0.0;
  filter = lti_filt_init(3, a, b);

  install_obj(&filter->od);

  impulse = init_one_tlsig(0, 1);
  impulse->s_block[0] = 1.0;//generate filter's impulse response
  /* for(i=0; i<impulse->sample_cnt; i++) */
  /*   printf("%f  ",impulse->s_block[i]); */
  /* printf("\n"); */
  filter->i_sigs[0] = impulse;

  /* fp1 = fopen("lkup1_out", "w"); */
  /* fp2 = fopen("lkup2_out", "w"); */
  
}

void do_kill(void){
    
  kill_lti_filt(filter);
  kill_one_tlsig(impulse);

}

void dsp_chain(int samples,
	       t_tlsig **adc_sigs,
	       t_tlsig **dac_sigs){
  
  //printf("hello from the dsp chain\n");
  int s = samples;
  int j, i;

  filter->dsp_func(s, filter);

}

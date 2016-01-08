#include "stdio.h"
#include "math.h"
#include "stdlib.h"
#include "g_api.h"
#include "m_modules.h"
#include "pthread.h"


static FILE *fp1, *fp2;

t_lookup **lookup;
t_table **table;
static t_dwnsample *dwnsample;

t_lin_ctl *l_freq, *l_amp, *r_freq, *r_amp;

int h = 2;

void setup_this(void){
  
  int i;
  float freql, freqr;
  freql = 500.0;
  freqr = 400.0;
  
    
  lookup = (t_lookup **)malloc(sizeof(t_lookup *) * 2);
  table = (t_table **)malloc(sizeof(t_table *) * 2);
  
  lookup[0] = (t_lookup *)lookup_init(h);
  lookup[1] = (t_lookup *)lookup_init(h);
  
  table[0] = (t_table *)table_init(100000, h);
  table[1] = (t_table *)table_init(100000, h);

  table[0]->i_sigs[0] = lookup[0]->o_sigs[0];
  table[1]->i_sigs[0] = lookup[1]->o_sigs[0];

  dwnsample = dwnsample_init(2, h);
  dwnsample->i_sigs[0] =  table[0]->o_sigs[0];
  dwnsample->i_sigs[1] =  lookup[1]->o_sigs[0];

  install_obj(&table[0]->od);
  install_obj(&table[1]->od);
  install_obj(&lookup[0]->od);
  install_obj(&lookup[1]->od);
  install_obj(&dwnsample->od);

  l_freq = init_lin_ctl(0, CTL_T_LIN, h);
  r_freq = init_lin_ctl(1, CTL_T_LIN, h);
  l_amp = init_lin_ctl(2, CTL_T_LIN, h);  
  r_amp = init_lin_ctl(3, CTL_T_LIN, h);

  set_lin_ctl_val(l_freq, freql);
  set_lin_ctl_val(r_freq, freqr);
  set_lin_ctl_val(l_amp, .5);
  set_lin_ctl_val(r_amp, .5);
 
  lookup[0]->i_sigs[0] = l_freq->ctl_sig;
  lookup[1]->i_sigs[0] = r_freq->ctl_sig;

  /* fp1 = fopen("lkup1_out", "w"); */
  /* fp2 = fopen("lkup2_out", "w"); */
  
}

void do_kill(void){
    
  kill_lookup(lookup[0]);
  kill_lookup(lookup[1]);
  
  free(lookup);
  lookup = NULL;
  
  kill_table(table[0]);
  kill_table(table[1]);
    
  free(table);
  table = NULL;

  kill_dwnsample(dwnsample);

  /* fclose(fp1); */
  /* fclose(fp2); */
}

void dsp_chain(int samples, 
	       t_tlsig **adc_sigs,
	       t_tlsig **dac_sigs){
  
  //printf("hello from the dsp chain\n");
  int s = samples;
  int j, i;

  lookup[0]->dsp_func(s, lookup[0]);
  lookup[1]->dsp_func(s, lookup[1]);
 
  table[0]->i_sigs[0] = lookup[0]->o_sigs[0];
  table[0]->dsp_func(s, table[0]);
  multiply_sigs(table[0]->o_sigs[0], l_amp->ctl_sig); 
  
  table[1]->i_sigs[0] = lookup[1]->o_sigs[0];
  table[1]->dsp_func(s, table[1]);
  multiply_sigs(table[1]->o_sigs[0], r_amp->ctl_sig); 

  dwnsample->dsp_func(s, dwnsample);
      
  dac_sigs[0] = dwnsample->o_sigs[0];  
  dac_sigs[1] = dwnsample->o_sigs[1];  

}

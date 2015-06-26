
#include "unistd.h"
#include "pthread.h"
#include "globals.h"
#include "m_modules.h"
#include "a_portaudio.h"
#include "stdio.h";

tl_dac *dac;
tl_table *tbl_l, *tbl_r;
tl_lookup *lkup_l, *lkup_r;

typedef struct _osc{
  tl_table *tbl;
  tl_lookup *lkup;
  tl_sig *outlet;
  tl_sig *amp;
  tl_sig *freq;
  tl_dsp_func dsp_func;
  tl_kill_func kill_func;
  tl_name *name;
}tl_osc;

tl_osc *init_osc(tl_table *tbl, tl_lookup *lkup, tl_sig *freq, tl_sig *amp);
void dsp_osc(int samples, void *mod);
void kill_osc(void *mod);

tl_osc *osc_l, *osc_r;

int setup(void){

  int i, cnt = 5000;
  tl_class *class_list;

  // initialize globals ... rethink this
  tl_set_samplerate(44100);
  tl_set_block_len(64);

  set_g_lvl_stck(init_lvl_stck());
  set_g_ctl_head(init_ctl(TL_HEAD_CTL));

  // TODO: attach these to modules, I can't see
  // any reason not to do this and automate this whole process
  ctl_l_freq = init_ctl(TL_LIN_CTL);
  ctl_r_freq = init_ctl(TL_LIN_CTL);

  ctl_l_freq->is_verbose = 0;
  ctl_r_freq->is_verbose = 0;

  ctl_l_amp = init_ctl(TL_LIN_CTL);
  ctl_r_amp = init_ctl(TL_LIN_CTL);

  ctl_l_amp->is_verbose = 1;
  ctl_r_amp->is_verbose = 1;

  install_onto_ctl_list(get_g_ctl_head(), ctl_l_freq);
  install_onto_ctl_list(get_g_ctl_head(), ctl_r_freq);
  install_onto_ctl_list(get_g_ctl_head(), ctl_l_amp);
  install_onto_ctl_list(get_g_ctl_head(), ctl_r_amp);

  // initialize portaudio
  pa_initialize(0,0,2,2,.25);

  // setup the modules
  dac = (tl_dac *)tl_init_dac(2,1);
  // dac will initialize the global output buffer
  tbl_l = (tl_table *)tl_init_table(1000, 1);
  tbl_r = (tl_table *)tl_init_table(1000, 1);
  lkup_l = (tl_lookup *)tl_init_lookup(1);
  lkup_r = (tl_lookup *)tl_init_lookup(1);

  osc_l = init_osc(tbl_l, lkup_l, ctl_l_freq->outlet, ctl_l_amp->outlet);
  osc_r = init_osc(tbl_r, lkup_r, ctl_r_freq->outlet, ctl_r_amp->outlet);


  // this part needs to be done explicitly
  // first, register the ctls
  set_ctl_kr(ctl_l_freq, &l_freq_val);
  set_ctl_kr(ctl_r_freq, &r_freq_val);
  set_ctl_kr(ctl_l_amp, &l_amp_val);
  set_ctl_kr(ctl_r_amp, &r_amp_val);

  // now register the classes
  set_g_class_head(init_class());

  tl_install_class(get_g_class_head(), (void *)osc_l, osc_l->dsp_func, osc_l->kill_func);
  tl_install_class(get_g_class_head(), (void *)osc_r, osc_r->dsp_func, osc_r->kill_func);
  tl_install_class(get_g_class_head(), (void *)dac, dac->dsp_func, dac->kill_func);

  // connect to the dac
  dac->inlets[0] = osc_l->tbl->outlets[0];
  dac->inlets[1] = osc_r->tbl->outlets[0];

  return 0;

}

void dsp_osc(int samples, void *mod){

  int i;
  tl_osc *x = (tl_osc *)mod;
  tl_smp scalar = .01;
  x->lkup->dsp_func(samples, x->lkup);
  x->tbl->dsp_func(samples, x->tbl);


  /* for(i=0; i<64; i++) */
  /*   printf("%d %f %f\n", i, x->amp->smps[i], x->freq->smps[i]); */
 
  /* for(i=0; i<64; i++) */
  /*   printf("%f %f\n", x->tbl->inlets[0]->smps[i], x->tbl->outlets[0]->smps[i] ); */

  /* for(i=0; i<64; i++) */
  /*   printf("amps  before: %f\n", x->amp->smps[i]); */

  /* for(i=0; i<64; i++) */
  /*   printf("outlets  before: %f\n", x->tbl->outlets[0]->smps[i]); */
  multiply_sigs(x->tbl->outlets[0], x->amp);
  scale_sig_vals(x->tbl->outlets[0], &scalar);

  /* for(i=0; i<64; i++) */
  /*   printf("outlets after: %f\n", x->tbl->outlets[0]->smps[i]); */

}

void kill_osc(void *mod){

  tl_osc *x = (tl_osc *)mod;
  if(x!=NULL)
    {
      tl_kill_table(x->tbl);
      tl_kill_lookup(x->lkup);      
      kill_one_sig(x->freq);
      kill_one_sig(x->amp);
      free(x);
      x=NULL;
    }
  else
    printf("error: kill_osc: invalid obj\n");

}

tl_osc *init_osc(tl_table *tbl, tl_lookup *lkup, tl_sig *freq, tl_sig *amp){

  tl_osc *x = (tl_osc *)malloc(sizeof(tl_osc));
  x->tbl = tbl;
  x->lkup = lkup;
  x->outlet = init_one_sig(tl_get_block_len(), 1);
  x->freq = freq;
  x->amp = amp;
  x->tbl->inlets[0] = x->lkup->outlets[0];
  x->lkup->inlets[0] = x->freq;
  x->kill_func = &kill_osc;
  x->dsp_func = &dsp_osc;
  return x;

}


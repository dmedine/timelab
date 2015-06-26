
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


  return 0;

}



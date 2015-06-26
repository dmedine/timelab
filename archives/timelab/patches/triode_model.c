#include "stdio.h"
#include "math.h"
#include "stdlib.h"
//#include "a_pa.h"
#include "g_api.h"
//#include "ui_main.h"
#include "m_modules.h"
#include "m_ode_prim.h"
#include "limits.h"

static t_lookup *lookup;
static t_table *table;
static t_diff *diff;
static t_rk_mother *rk_circuit;

t_lin_ctl *c_sig_freq;
t_lin_ctl *c_sig_level;
t_lin_ctl *c_dac_level;
t_lin_ctl *c_fb_in;
t_lin_ctl *c_fb_out;

t_tlsmp sig_freq  =  100;
t_tlsmp sig_level = .707; 
t_tlsmp out_level = .707; 
t_tlsmp fb_in     = .001; 
t_tlsmp fb_out    = .001; 

#define FB_IN  c_fb_in->s_block
#define FB_OUT c_fb_out->s_block

static void rk_circuit_func(int samples, void *ptr, t_tlsmp *input){

  t_rk_mother *x = (t_rk_mother *)ptr;
  t_tlsmp data[2];

  int s = samples;

}

void setup_this(void){

   lookup = (t_lookup *)lookup_init(1);
   table = (t_table *)table_init(1000, 1);
   diff = (t_diff *)diff_init(1, 1);

   rk_circuit = (t_rk_circuit *)rk_mother_init(&rk_circuit_func,
					       2,
					       0,
					       1,
					       1);

   install_obj(&lookup->od);
   install_obj(&table->od);
   install_obj(&rk_circuit->od);
   install_obj(&diff->od);

   c_sig_freq  = init_lin_ctl(0, CTL_T_LIN, 1);
   c_sig_level = init_lin_ctl(1, CTL_T_LIN, 1);
   c_out_level = init_lin_ctl(2, CTL_T_LIN, 1);
   c_fb_in     = init_lin_ctl(3, CTL_T_LIN, 1);
   c_fb_out    = init_lin_ctl(4, CTL_T_LIN, 1);

   level_lin_ctl(c_sig_freq, sig_freq);
   level_lin_ctl(c_sig_level, sig_level);
   level_lin_ctl(c_out_level, out_level);
   level_lin_ctl(c_fb_in, fb_in);
   level_lin_ctl(c_fb_in, fb_out);

}

void do_kill(void){

  kill_lookup(lookup);
  kill_table(table);
  kill_diff(diff);

  kil_rk_mother(rk_circuit);
}


void dsp_chain(int samples,
	       t_tlsig **adc_sigs,
	       t_tlsig **dac_sigs){

  lookup->i_sigs[0] = c_sig_freq->ctl_sig;
  lookup->dsp_func(samples, lookup);

  table->i_sigs[0] = lookup->o_sigs[0];
  table->dsp_func(samples, table);
 multiply_sigs(table->o_sigs[0], c_sig_level->ctl_sig):

  rk_circuit->i_sigs[0] = table;
  rk_circuit->dsp_func(samples, rk_circuit, NULL);

  diff->i_sigs[0] = rk_circuit->o_sigs[0];
  diff->dsp_func(samples, diff);

  multiply_sigs(diff->o_sigs[0], c_out_level->ctl_sig);

  dac_sigs[0] = diff->o_sigs[0];

}

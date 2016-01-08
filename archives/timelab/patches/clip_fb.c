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
static t_rk_mother *rk_clipper;

t_lin_ctl *c_sig_freq;
t_lin_ctl *c_sig_level;
t_lin_ctl *c_out_level;
t_lin_ctl *c_fb_in;
t_lin_ctl *c_fb_out;
t_lin_ctl *c_clip_gain;

t_tlsmp sig_freq  =  100;
t_tlsmp sig_level = .707; 
t_tlsmp out_level = .707; 
t_tlsmp fb_in     = 0.00; 
t_tlsmp fb_out    = 0.00; 
t_tlsmp clip_gain = 1.0;

#define FB_IN  c_fb_in->ctl_sig->s_block
#define FB_OUT c_fb_out->ctl_sig->s_block
#define CLIP_GAIN c_clip_gain->ctl_sig->s_block

static t_tlsmp diff(t_tlsmp alpha, t_tlsmp *data){

  t_tlsmp out;
  out = tanh(alpha * (data[0] - data[1]));
  return out;

}

static t_tlsmp tanh_clip(t_tlsmp alpha, t_tlsmp datum){

  t_tlsmp out;
  out = tanh(alpha * datum);
  return out;

}


static void rk_clipper_func(int samples, void *ptr, t_tlsmp *input){

  t_rk_mother *x = (t_rk_mother *)ptr;
  t_tlsmp data[2];

  int i, j;
  int s = samples;


  for(i=0; i<s; i++)
    {
    
      //ladder filter
      for(j=0; j<4; j++)
	{
	  
	  data[0] = CLIP_GAIN[i] * x->i_sigs[0]->s_block[i] + FB_IN[i] * x->rk_children[0]->state;
	  data[1] = FB_OUT[i]*x->rk_children[0]->state;
	  //data[1] is the current state here

	  x->rk_children[0]->ks[j+1] = 
	    diff(1, data);

	}

      rk_child_estimate(x->rk_children[0]);
      x->o_sig0[i] = x->rk_children[0]->state;
    }
}

void setup_this(void){

   lookup = (t_lookup *)lookup_init(1);
   table = (t_table *)table_init(1000, 1);

   rk_clipper = (t_rk_mother *)rk_mother_init(&rk_clipper_func,
					       1,
					       0,
					       1,
					       1);

   install_obj(&lookup->od);
   install_obj(&table->od);
   install_obj(&rk_clipper->od);

   c_sig_freq  = init_lin_ctl(0, CTL_T_LIN, 1);
   c_sig_level = init_lin_ctl(1, CTL_T_LIN, 1);
   c_out_level = init_lin_ctl(2, CTL_T_LIN, 1);
   c_fb_in     = init_lin_ctl(3, CTL_T_LIN, 1);
   c_fb_out    = init_lin_ctl(4, CTL_T_LIN, 1);
   c_clip_gain = init_lin_ctl(5, CTL_T_LIN, 1);

   level_lin_ctl(c_sig_freq, sig_freq);
   level_lin_ctl(c_sig_level, sig_level);
   level_lin_ctl(c_out_level, out_level);
   level_lin_ctl(c_fb_in, fb_in);
   level_lin_ctl(c_fb_out, fb_out);
   level_lin_ctl(c_clip_gain, clip_gain);

}

void do_kill(void){

  kill_lookup(lookup);
  kill_table(table);

  kill_rk_mother(rk_clipper);
}


void dsp_chain(int samples,
	       t_tlsig **adc_sigs,
	       t_tlsig **dac_sigs){

  lookup->i_sigs[0] = c_sig_freq->ctl_sig;
  lookup->dsp_func(samples, lookup);

  table->i_sigs[0] = lookup->o_sigs[0];
  table->dsp_func(samples, table);
  multiply_sigs(table->o_sigs[0], c_sig_level->ctl_sig);

  rk_clipper->i_sigs[0] = table->o_sigs[0];
  rk_clipper->dsp_func(samples, rk_clipper, NULL);

  dac_sigs[0] = rk_clipper->o_sigs[0];

}

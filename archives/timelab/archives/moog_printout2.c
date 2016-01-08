#include "stdio.h"
#include "math.h"
#include "stdlib.h"
#include "g_api.h"
#include "m_modules.h"
#include "m_ode_prim.h"


t_noise *noise;
t_rk_mother *rk_moog;
t_tlsig *relay_sig;

t_tlsmp slp_dir;

t_tlsmp noise_amp, r_val, g_val, ctr;

t_lin_ctl *c_cutoff, *c_res;
t_lin_ctl *c_output_amp;

t_lin_ctl *c_swth_freq, *c_swth_thresh;
t_lin_ctl *c_swth_amp, *c_noise_amp;

#define CUTOFF      c_cutoff->ctl_sig->s_block
#define RES         c_res->ctl_sig->s_block
#define SWTH_FREQ   c_swth_freq->ctl_sig->s_block
#define SWTH_THRESH c_swth_thresh->ctl_sig->s_block
#define SWTH_AMP    c_swth_amp->ctl_sig->s_block
#define NOISE_AMP   c_noise_amp->ctl_sig->s_block

static FILE *fp1, *fp2;

static t_tlsmp swth_func(t_tlsmp freq, t_tlsmp thresh, t_tlsmp state){

  t_tlsmp out = freq/2.0;
  if(state>=thresh)
    out = -1000000.0;

  return out;

}

static t_tlsmp diff(t_tlsmp alpha, t_tlsmp *data){

  t_tlsmp out;
  out = alpha * tanh((data[0] - data[1]));
  return out;

}

static void rk_moog_func(int samples, void *ptr, t_tlsmp *input){

  t_rk_mother *x = (t_rk_mother *)ptr;
  t_tlsmp data[2];
  t_tlsmp alpha;
  t_tlsmp res;
  t_tlsmp freq, thresh, swth_amp, n_amp, tri_state;

  int s = samples;
  int i, j, k;

  for(i=0; i<s; i++)
    {
      alpha    = CUTOFF[i] * 2 * M_PI;
      res      = RES[i];
      freq     = SWTH_FREQ[i];
      thresh   = SWTH_THRESH[i];
      swth_amp = SWTH_AMP[i];
      n_amp    = NOISE_AMP[i];


      //ladder filter
      for(j=0; j<4; j++)
	{
	  
	  //first, swth wave
	  tri_state = rk_child_stage(j, x->rk_children[4]);
	  x->rk_children[4]->ks[j+1] = swth_func(freq, thresh, tri_state);

	  //now the filter
	  //stage 1 is unique
	  data[0] = swth_amp * x->rk_children[4]->state+//swth input
	    n_amp * x->i_sigs[0]->s_block[i] - //noise input
	    res * rk_child_stage(j, x->rk_children[3]);
	  //data[0] is the input to the filter minus the feedback from the other end

	  data[1] = rk_child_stage(j, x->rk_children[0]);
	  //data[1] is the current state here

	  x->rk_children[0]->ks[j+1] = 
	    diff(alpha, data);

	  //the other three stages are the same
	  //input to that stage (previous stage) and current state at that stage
	  for(k=1; k<4; k++)
	    {
	      data[0] = rk_child_stage(j, x->rk_children[k-1]);
	      data[1] = rk_child_stage(j, x->rk_children[k]);
	      x->rk_children[k]->ks[j+1] = 
		diff(alpha, data);
	    }
	}

      //estimate
      rk_child_estimate(x->rk_children[0]);
      rk_child_estimate(x->rk_children[1]);
      rk_child_estimate(x->rk_children[2]);
      rk_child_estimate(x->rk_children[3]);
  
      rk_child_estimate(x->rk_children[4]);

      x->o_sig0[i] = x->rk_children[0]->state;
      x->o_sig1[i] = x->rk_children[1]->state;
      x->o_sig2[i] = x->rk_children[2]->state;
      x->o_sig3[i] = x->rk_children[3]->state;
  
      if(x->rk_children[4]->state<-1.0)x->rk_children[4]->state=-1.0;
      x->o_sig4[i] = x->rk_children[4]->state;
    }

}


void setup_this(void){

  noise_amp = .10;
  r_val = 2.0;
  g_val = 1000.0;
  ctr = 0;
  noise = noise_init(1);
  install_obj(&noise->od);

  rk_moog = (t_rk_mother *)rk_mother_init(&rk_moog_func,
					  5,//4 filter stages plus 1 oscillator
					  0,
					  1,
					  1);

  install_obj(&rk_moog->od);

  rk_moog->i_sigs[0] = noise->o_sigs[0];

  relay_sig = init_one_tlsig(O_TYPE, 1);

  c_cutoff       = init_lin_ctl(0, CTL_T_LIN, 1);
  c_res          = init_lin_ctl(1, CTL_T_LIN, 1);   
  c_output_amp   = init_lin_ctl(2, CTL_T_LIN, 1);
  c_swth_freq    = init_lin_ctl(3, CTL_T_LIN, 1);
  c_swth_thresh  = init_lin_ctl(4, CTL_T_LIN, 1);
  c_swth_amp     = init_lin_ctl(5, CTL_T_LIN, 1);
  c_noise_amp    = init_lin_ctl(6, CTL_T_LIN, 1);

  level_lin_ctl(c_cutoff, g_val);
  level_lin_ctl(c_res, r_val);
  level_lin_ctl(c_output_amp, .707);
  level_lin_ctl(c_swth_freq, 500.0);
  level_lin_ctl(c_swth_thresh, .75);
  level_lin_ctl(c_swth_amp, 0.0);
  level_lin_ctl(c_noise_amp, noise_amp);

  fp1 = fopen("moog_printout1", "w");
  fp2 = fopen("moog_printout_ctls", "w");
}

void do_kill(void){

  kill_noise(noise);
  kill_rk_mother(rk_moog);
  kill_one_tlsig(relay_sig);
  fclose(fp1);
  fclose(fp2);
}

void dsp_chain(int samples, 
	       t_tlsig **adc_sigs,
	       t_tlsig **dac_sigs){

  int i;
  int s = samples;

  if(r_val < 4.00)
    r_val += .01;
  /* else  */
  /* if(noise_amp>=0.0) */
  /*     noise_amp -= .005; */
  /* if(g_val < 20000.0) */
  /*   g_val +=80.0; */
  ctr+=1;

  push_duple(0, g_val);
  push_duple(1, r_val);
  push_duple(6, noise_amp);

  noise->dsp_func(s, noise);

  rk_moog->dsp_func(s, rk_moog, NULL);

  cpy_sigs(relay_sig, rk_moog->o_sigs[3]);
  for(i=0; i<s; i++)
    fprintf(fp1, "%f\n", relay_sig->s_block[i]);
  fprintf(fp2, "%f %f %f %f\n", ctr, r_val, g_val, noise_amp);


  multiply_sigs(relay_sig, c_output_amp->ctl_sig);

  dac_sigs[0] = relay_sig;
  if(r_val>=4.0)g_audio_off();
}

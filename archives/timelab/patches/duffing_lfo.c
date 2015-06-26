#include "stdio.h"
#include "math.h"
#include "stdlib.h"
//#include "a_pa.h"
#include "g_api.h"
//#include "ui_main.h"
#include "m_modules.h"
#include "m_ode_prim.h"
#include "limits.h"

FILE *fp1, *fp2;

static t_rk_mother *rk_osc;

int g_h  = 1;
int g_up = 1;


t_lin_ctl *reset_ctl;
t_lin_ctl **dac_amp_ctls;


#define N_CTLS 16
t_lin_ctl **ctls;

t_tlsmp omega0 = 1.0;

t_tlsmp alpha0  = 0.0;
t_tlsmp beta0   = 0.0;
t_tlsmp delta0  = 0.0;
t_tlsmp gamma0  = 0.0;

t_tlsmp tri_freq  = 300.0;
t_tlsmp filt_freq = 300.0;
t_tlsmp res       = 1.0;

t_tlsmp tri_f_depth  = 0.0;
t_tlsmp filt_f_depth = 0.0;

t_tlsmp fba = 0.0;
t_tlsmp fbb = 0.0;

t_tlsmp sqr_flyback = 72000.0;
t_tlsmp curl        = 5.0;
t_tlsmp tri_thresh  = .9;

t_tlsmp duff_state = -.5;



/*****************/

void reset_oscs(void);

static t_tlsmp tanh_clip(t_tlsmp x, t_tlsmp gain){

  t_tlsmp out = tanh(x * gain);
  return(out);
}

static t_tlsmp sin_x_dot(t_tlsmp *data, t_tlsmp omega){
  
  t_tlsmp out = omega * data[1];
  return out;

}

static t_tlsmp sin_y_dot(t_tlsmp *data, t_tlsmp omega){

  t_tlsmp out = -1.0 * omega * data[0];
  return out;

}

static t_tlsmp duff_x_dot(t_tlsmp *data){

  t_tlsmp out = data[1];

  return out;
}


static t_tlsmp duff_y_dot(t_tlsmp *data, t_tlsmp alpha, t_tlsmp beta, t_tlsmp delta, t_tlsmp gamma, t_tlsmp in){

  t_tlsmp third_term = beta * data[0] * data[0] * data[0];

  //  printf("beta0 %f data[0] %f third term %f\n", beta0, data[0], third_term);
  t_tlsmp out = 
    -1.0 * delta * data[1] - 
    alpha * data[0] -
    third_term +
    gamma * in;

  return out;

}


static t_tlsmp tri(t_tlsmp *data, t_tlsmp slp){

  t_tlsmp out = tanh_clip(data[1], 5) * slp;
  //printf("data[1] %f slp %f out %f\n", data[1], slp, out);
  return out;

}


static t_tlsmp sqr(t_tlsmp *data, t_tlsmp thresh, t_tlsmp flyback){

  t_tlsmp out = 0.0;
  if(data[0]>thresh)
      out = -1.0*flyback;
      
  if(data[0]<(-1.0*thresh))
    out = flyback;
  
  //printf("%f %f\n", data[0], out);  
  // printf("data[0] %f data[1] %f thresh %f dir %d out %f\n", data[0], data[1], thresh, *dir, out); 
  return out;

}

static t_tlsmp diff(t_tlsmp alpha, t_tlsmp *data){

  t_tlsmp out;
  out = alpha * tanh((data[0] - data[1]));
  return out;

}

void reset_oscs(void){

  //printf("hello, you!\n");
  int i, j;
  for(i=0; i<4; i++)
    {
      rk_osc->rk_children[i]->state = 0.0;
      for(j=0; j<4; j++)
	rk_osc->rk_children[i]->ks[j] = 0.0;
    }
  
  rk_osc->rk_children[0]->state = -1.0;
  rk_osc->rk_children[1]->state = 0.0;
  rk_osc->rk_children[2]->state = duff_state;
  rk_osc->rk_children[3]->state = 0.0;
  rk_osc->rk_children[4]->state = 1.0;
  rk_osc->rk_children[5]->state = 0.0;
  rk_osc->rk_children[6]->state = 0.0;
  rk_osc->rk_children[7]->state = 0.0;
  rk_osc->rk_children[8]->state = 0.0;
  rk_osc->rk_children[9]->state = 0.0;
  
}


static void rk_osc_func(int samples, void *ptr, t_tlsmp *input){

  t_rk_mother *x = ptr;
  t_tlsmp data0[2], data1[2], data2[2], data3[2];//sinusoid, duffing, tri/sqr, moog
  t_tlsmp duff_in, moog_in;
  int s = samples * x->h * x->up;
  int i, j, k;
  
  for(i=0; i< s; i++)
    {

      omega0 = ctls[0]->c_sig[i];
      alpha0 = ctls[1]->c_sig[i];
      beta0  = ctls[2]->c_sig[i];
      delta0 = ctls[3]->c_sig[i];
      gamma0 = ctls[4]->c_sig[i];

      tri_freq  = ctls[5]->c_sig[i];
      filt_freq = ctls[6]->c_sig[i] * M_PI;
      res       = ctls[7]->c_sig[i];

      tri_f_depth  = ctls[8]->c_sig[i];
      filt_f_depth = ctls[9]->c_sig[i];

      fba     = ctls[10]->c_sig[i];
      fbb = ctls[11]->c_sig[i];

      sqr_flyback = ctls[12]->c_sig[i];
      curl        = ctls[13]->c_sig[i];
      tri_thresh  = ctls[14]->c_sig[i];

      duff_state  = ctls[15]->c_sig[i];
   
      //      printf("rk beta0 %f\n", beta0);
      //**************************************

      for(k=0; k<4; k++)
  	{
	  //sinusoidal osc
	  //printf("%f %f %f\n", data1[0], fb1, 	  
  	  /*sin x*/
  	  data0[0] = rk_child_stage(k, x->rk_children[0]);
  	  /*sin y*/
  	  data0[1] = rk_child_stage(k, x->rk_children[1]);

	  duff_in = data0[0];

	  //*********************************************
	  //duffing oscillator
  	  /*duff x*/
  	  data1[0] = rk_child_stage(k, x->rk_children[2]);
  	  /*duff y/x_dot*/
  	  data1[1] = rk_child_stage(k, x->rk_children[3]);

	  //*********************************************
	  //triangle/square oscillator
	  /* tri */
	  data2[0] = rk_child_stage(k, x->rk_children[4]);
	  /* sqr */
	  data2[1] = rk_child_stage(k, x->rk_children[5]);

	  moog_in = data2[0];
	  //*********************************************
	  /* moog */
	  //ladder rung one:
	  data3[0] = moog_in - (res + (.5 *(data1[0]+1.0))) * tri_f_depth * rk_child_stage(k, x->rk_children[9]);
	  //filter input minus resonance times output
	  data3[1] = rk_child_stage(k, x->rk_children[6]);
	  //current state here

  	  x->rk_children[0]->ks[k+1] = 
  	    sin_x_dot(data0, omega0);
	  
  	  x->rk_children[1]->ks[k+1] = 
  	    sin_y_dot(data0, omega0);

  	  x->rk_children[2]->ks[k+1] = 
  	    duff_x_dot(data1);
	  
  	  x->rk_children[3]->ks[k+1] = 
  	    duff_y_dot(data1, alpha0 + x->rk_children[9]->state*fba, beta0 + x->rk_children[9]->state*fbb, delta0, gamma0, duff_in);

  	  x->rk_children[4]->ks[k+1] = 
  	    tri(data2, 2*(tri_freq + tri_f_depth));
	  
  	  x->rk_children[5]->ks[k+1] = 
  	    sqr(data2, tri_thresh, sqr_flyback);

	  x->rk_children[6]->ks[k+1] =
	    diff(filt_freq + (data1[1]+1.0) * .5 * filt_f_depth, data3);

	  //ladder rungs 2-4:
	  for(j=1; j<4; j++)
	    {
	      data3[0] = rk_child_stage(k, x->rk_children[6+j-1]);
	      data3[1] = rk_child_stage(k, x->rk_children[6+j]);
	      x->rk_children[6+j]->ks[k+1] =
	      	diff(filt_freq + (data1[1]+1.0) * .5 * filt_f_depth, data3);
	    }



  	}


      
      rk_child_estimate(x->rk_children[0]);
      rk_child_estimate(x->rk_children[1]);
      rk_child_estimate(x->rk_children[2]);
      rk_child_estimate(x->rk_children[3]);
      rk_child_estimate(x->rk_children[4]);
      rk_child_estimate(x->rk_children[5]);
      rk_child_estimate(x->rk_children[6]);
      rk_child_estimate(x->rk_children[7]);
      rk_child_estimate(x->rk_children[8]);
      rk_child_estimate(x->rk_children[9]);

      x->o_sigs[0]->s_block[i] = x->rk_children[0]->state;
      x->o_sigs[1]->s_block[i] = x->rk_children[1]->state;

      x->o_sigs[2]->s_block[i] = x->rk_children[2]->state;
      x->o_sigs[3]->s_block[i] = x->rk_children[3]->state;

      x->o_sigs[4]->s_block[i] = x->rk_children[4]->state;
      x->o_sigs[5]->s_block[i] = x->rk_children[5]->state = 
	tanh_clip(x->rk_children[5]->state, curl);

      x->o_sigs[6]->s_block[i] = x->rk_children[6]->state;
      x->o_sigs[7]->s_block[i] = x->rk_children[7]->state;
      x->o_sigs[8]->s_block[i] = x->rk_children[8]->state;
      x->o_sigs[9]->s_block[i] = x->rk_children[9]->state;


      
    }

}


void setup_this(void){
  
  int i;
  int h = g_h;
  int up = g_up;

  rk_osc = (t_rk_mother *)rk_mother_init(&rk_osc_func,
  					 10,//
  					 0,//get rid of this arg
  					 h,//step size factor
					 up);//upsampling factor

  rk_osc->rk_children[0]->state = -1.0;
  rk_osc->rk_children[2]->state = duff_state;
  rk_osc->rk_children[4]->state = 1.0;
  
  install_obj(&rk_osc->od);

  //*******************************
  //controls

  ctls = (t_lin_ctl **)malloc(sizeof(t_lin_ctl*) * N_CTLS);
  dac_amp_ctls = (t_lin_ctl **)malloc(sizeof(t_lin_ctl*) * 2);
  

  for(i=0; i<2; i++)
    {
      dac_amp_ctls[i] = init_lin_ctl(i, CTL_T_LIN, h*up);//0,1
      level_lin_ctl(dac_amp_ctls[i], .707);
      printf("installing dac ctls %d\n", i);
    }
  level_lin_ctl(dac_amp_ctls[0], .707);

  reset_ctl = init_lin_ctl(2, CTL_T_BANG, h*up);//2
  printf("installing reset ctl %d\n", 2);
  reset_ctl->do_bang = reset_oscs;
    

  for(i=0; i<N_CTLS; i++)
    ctls[i] = init_lin_ctl(3+i, CTL_T_LIN, h*up);//3+N_MATRIX - N_CTLS

  level_lin_ctl(ctls[0], omega0);

  level_lin_ctl(ctls[1], alpha0);
  level_lin_ctl(ctls[2], beta0);
  level_lin_ctl(ctls[3], delta0);
  level_lin_ctl(ctls[4], gamma0);
  level_lin_ctl(ctls[5], tri_freq);
  level_lin_ctl(ctls[6], filt_freq);
  level_lin_ctl(ctls[7], res);
  level_lin_ctl(ctls[8], tri_f_depth);
  level_lin_ctl(ctls[9], filt_f_depth);
  level_lin_ctl(ctls[10], fba);
  level_lin_ctl(ctls[11], fbb);
  level_lin_ctl(ctls[12], sqr_flyback);
  level_lin_ctl(ctls[13], curl);
  level_lin_ctl(ctls[14], tri_thresh);
  level_lin_ctl(ctls[15], duff_state);
  
}


void do_kill(void){

  int i;

  kill_rk_mother(rk_osc);

  free(dac_amp_ctls);
  dac_amp_ctls = NULL;

  free(ctls);
  ctls = NULL;

  /* fclose(fp1); */
  /* fclose(fp2); */


}

void dsp_chain(int samples, 
	       t_tlsig **adc_sigs,
	       t_tlsig **dac_sigs){
  
  int i, j;
  int s = samples;

  rk_osc->dsp_func(s, rk_osc, NULL);

  multiply_sigs(rk_osc->o_sigs[4], dac_amp_ctls[0]->ctl_sig);
  multiply_sigs(rk_osc->o_sigs[9], dac_amp_ctls[1]->ctl_sig);

  //dac_sigs[0] = rk_osc->o_sigs[4];
  dac_sigs[1] = rk_osc->o_sigs[9];

}




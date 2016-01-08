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
static t_ie_mother *ie_osc;
static t_upsample *upsample;
static t_dwnsample *dwnsample;

int g_h  = 1;
int g_up = 1;


t_lin_ctl *reset_ctl;
t_lin_ctl **dac_amp_ctls;


#define N_CTLS 6
t_lin_ctl **ctls;

t_tlsmp omega0 = 10.0;
t_tlsmp omega1 = 10.0;

t_tlsmp alpha0  = 0.0;
t_tlsmp beta0   = 0.0;
t_tlsmp delta0  = 0.0;
t_tlsmp gamma0  = 0.0;



/*****************/

void reset_oscs(void);

static t_tlsmp sin_x_dot(t_tlsmp *data, t_tlsmp omega){
  
  t_tlsmp out = omega * data[1];
  return out;

}

static t_tlsmp sin_y_dot(t_tlsmp *data, t_tlsmp omega){

  t_tlsmp out = -1.0 * omega * data[0];
  return out;

}

static t_tlsmp duff_x_dot(t_tlsmp *data, t_tlsmp alpha){

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
  rk_osc->rk_children[2]->state = -.5;
  ie_osc->ie_children[0]->state = -.5;

}


static void rk_osc_func(int samples, void *ptr, t_tlsmp *input){

  t_rk_mother *x = ptr;
  t_tlsmp data0[2], data1[2];
  int s = samples * x->h * x->up;
  int i, j;
  
  for(i=0; i< s; i++)
    {

      omega0 = ctls[0]->c_sig[i];
      omega1 = ctls[1]->c_sig[i];
      alpha0 = ctls[2]->c_sig[i];
      beta0  = ctls[3]->c_sig[i];
      delta0 = ctls[4]->c_sig[i];
      gamma0 = ctls[5]->c_sig[i];
   
      //      printf("rk beta0 %f\n", beta0);
      //**************************************

      for(j=0; j<4; j++)
  	{
	  //printf("%f %f %f\n", data1[0], fb1, 	  
  	  /*sin x*/
  	  data0[0] = rk_child_stage(j, x->rk_children[0]);
  	  /*sin y*/
  	  data0[1] = rk_child_stage(j, x->rk_children[1]);
	  
  	  /*duff x*/
  	  data1[0] = rk_child_stage(j, x->rk_children[2]);
  	  /*duff y/x_dot*/
  	  data1[1] = rk_child_stage(j, x->rk_children[3]);

	  
  	  x->rk_children[0]->ks[j+1] = 
  	    sin_x_dot(data0, omega0);
	  
  	  x->rk_children[1]->ks[j+1] = 
  	    sin_y_dot(data0, omega0);
	  
  	  x->rk_children[2]->ks[j+1] = 
  	    duff_x_dot(data1, alpha0);
	  
  	  x->rk_children[3]->ks[j+1] = 
  	    duff_y_dot(data1, alpha0, beta0, delta0, gamma0, data0[0]);


	  
  	}


      
      rk_child_estimate(x->rk_children[0]);
      rk_child_estimate(x->rk_children[1]);
      rk_child_estimate(x->rk_children[2]);
      rk_child_estimate(x->rk_children[3]);


      x->o_sigs[0]->s_block[i] = x->rk_children[0]->state;
      x->o_sigs[1]->s_block[i] = x->rk_children[1]->state;

      x->o_sigs[2]->s_block[i] = x->rk_children[2]->state;
      x->o_sigs[3]->s_block[i] = x->rk_children[3]->state;

      
    }

}

static void ie_osc_func(int samples, void *ptr, t_tlsmp *input){

  t_ie_mother *x = ptr;
  t_tlsmp data0[2];
  t_tlsmp in;

  int s = samples * x->h * x->up;
  int i, j;
  int its = 10;
  
  for(i=0; i< s; i++)
    {

      omega0 = ctls[0]->c_sig[i];
      omega1 = ctls[1]->c_sig[i];
      alpha0 = ctls[2]->c_sig[i];
      beta0  = ctls[3]->c_sig[i];
      delta0 = ctls[4]->c_sig[i];
      gamma0 = ctls[5]->c_sig[i];

      //      printf("ie beta0 %f\n", beta0);   
      in = x->i_sigs[0]->s_block[i];// force oscillator is decoupled

      //**************************************

      ie_child_begin(x->ie_children[0]);
      ie_child_begin(x->ie_children[1]);

      while(its--)
  	{
	  //printf("%f %f %f\n", data1[0], fb1, 	  
  	  data0[0] = x->ie_children[0]->y_k01;
  	  data0[1] = x->ie_children[1]->y_k01;

	  x->ie_children[0]->y_k11 = x->ie_children[0]->y_k + x->h_time *
	    duff_x_dot(data0, alpha0);
	  
	  ie_child_iterate(x->ie_children[0]);

	  x->ie_children[1]->y_k11 = x->ie_children[1]->y_k + x->h_time *
	    duff_y_dot(data0, alpha0, beta0, delta0, gamma0, in);
	  
	  ie_child_iterate(x->ie_children[1]);
	  
	  if(x->ie_children[0]->diff<=x->ie_children[0]->tol &&
	     x->ie_children[1]->diff<=x->ie_children[1]->tol) break;

  	}

 
      x->o_sigs[0]->s_block[i] = x->ie_children[0]->state;
      x->o_sigs[1]->s_block[i] = x->ie_children[1]->state;

      
    }

}


void setup_this(void){
  
  int i;
  int h = g_h;
  int up = g_up;

  rk_osc = (t_rk_mother *)rk_mother_init(&rk_osc_func,
  					 4,//2 children for each of to oscillators
  					 0,//get rid of this arg
  					 h,//step size factor
					 up);//upsampling factor

  rk_osc->rk_children[0]->state = -1.0;
  rk_osc->rk_children[2]->state = -.5;

  ie_osc = (t_ie_mother *)ie_mother_init(&ie_osc_func,
  					 2,//2 children for each of to oscillators
  					 0,//get rid of this arg
  					 h,//step size factor
					 up);//upsampling factor

  ie_osc->ie_children[0]->state = -.5;
  ie_osc->i_sigs[0] = rk_osc->o_sigs[1];
  
  dwnsample = dwnsample_init(2, h*up);
  dwnsample->i_sigs[0] = rk_osc->o_sigs[3];
  dwnsample->i_sigs[1] = ie_osc->o_sigs[1];

  install_obj(&rk_osc->od);
  install_obj(&ie_osc->od);
  install_obj(&dwnsample->od);

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

  reset_ctl = init_lin_ctl(2, CTL_T_BANG, h*up);//2
  printf("installing reset ctl %d\n", 2);
  reset_ctl->do_bang = reset_oscs;
    

  for(i=0; i<N_CTLS; i++)
    ctls[i] = init_lin_ctl(3+i, CTL_T_LIN, h*up);//3+N_MATRIX - N_CTLS

  level_lin_ctl(ctls[0], omega0);
  level_lin_ctl(ctls[1], omega1);

  level_lin_ctl(ctls[2], alpha0);
  level_lin_ctl(ctls[3], beta0);
  level_lin_ctl(ctls[4], delta0);
  level_lin_ctl(ctls[5], gamma0);



}


void do_kill(void){

  int i;

  kill_rk_mother(rk_osc);
  kill_ie_mother(ie_osc);
  kill_dwnsample(dwnsample);

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
  //ie_osc->dsp_func(s, ie_osc, NULL);
  dwnsample->dsp_func(s, dwnsample);
    
  dac_sigs[0] = dwnsample->o_sigs[0];
  dac_sigs[1] = dwnsample->o_sigs[1];

  multiply_sigs(dac_sigs[0], dac_amp_ctls[0]->ctl_sig);
  multiply_sigs(dac_sigs[1], dac_amp_ctls[1]->ctl_sig);

}




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
static t_upsample *upsample;
static t_dwnsample *dwnsample;

t_tlsmp old_state1;
t_tlsmp old_state2;
t_tlsmp old_state3;

int g_h = 1;
int g_up = 1;

t_lin_ctl **ctls;

t_lin_ctl *reset_ctl;
t_lin_ctl **dac_amp_ctls;

#define N_CTLS 18
t_tlsmp omega0 = 10.0;
t_tlsmp omega1 = 10.0;
t_tlsmp omega2 = 10.0;

t_tlsmp alpha0 = 0.0;
t_tlsmp alpha1 = 0.0;
t_tlsmp alpha2 = 0.0;

t_tlsmp fb00 = 0.0;
t_tlsmp fb01 = 0.0;
t_tlsmp fb02 = 0.0;

t_tlsmp fb10 = 0.0;
t_tlsmp fb11 = 0.0;
t_tlsmp fb12 = 0.0;

t_tlsmp fb20 = 0.0;
t_tlsmp fb21 = 0.0;
t_tlsmp fb22 = 0.0;

t_tlsmp mu0 = 0.0;
t_tlsmp mu1 = 0.0;
t_tlsmp mu2 = 0.0;

#define N_MATRIX 9
t_lin_ctl **sync_matrix_ctls;

int sync_matrix[3][3];

/*****************/

void reset_oscs(void);

static t_tlsmp x_dot(t_tlsmp *data, t_tlsmp omega, t_tlsmp alpha, t_tlsmp r, t_tlsmp in){

  
  t_tlsmp out = ((omega + in) * data[1]) + (data[0]*(alpha * (1.0-r)));
  return out;

}

static t_tlsmp y_dot(t_tlsmp *data, t_tlsmp omega, t_tlsmp alpha, t_tlsmp r, t_tlsmp mu, t_tlsmp in){

  t_tlsmp out;
  
  // * (1.0 - (data[0]*data[0]))
  out = (mu * (1.0 - (data[0]*data[0])) * data[1]) -
    ((omega + in) * data[0]) +
    (data[1]*(alpha * (1.0-r)));
  return(out);

}


static void rk_osc_func(int samples, void *ptr, t_tlsmp *input){

  t_rk_mother *x = ptr;
  t_tlsmp data0[2], data1[2], data2[2], old_states[3], cur_states[3];
  t_tlsmp r0, r1, r2;
  int s = samples * x->h * x->up;
  int i, j;
  
  for(i=0; i< s; i++)
    {

      omega0 = ctls[0]->ctl_sig->s_block[i];
      omega1 = ctls[1]->ctl_sig->s_block[i];
      omega2 = ctls[2]->ctl_sig->s_block[i];
      //printf("omega0: %f\n", ctls[0]->ctl_sig->s_block[i]);
      alpha0 = ctls[3]->ctl_sig->s_block[i];
      alpha1 = ctls[4]->ctl_sig->s_block[i];
      alpha2 = ctls[5]->ctl_sig->s_block[i];

      fb00 = ctls[6]->ctl_sig->s_block[i];
      fb01 = ctls[7]->ctl_sig->s_block[i];
      fb02 = ctls[8]->ctl_sig->s_block[i];

      fb10 = ctls[9]->ctl_sig->s_block[i];
      fb11 = ctls[10]->ctl_sig->s_block[i];
      fb12 = ctls[11]->ctl_sig->s_block[i];

      fb20 = ctls[12]->ctl_sig->s_block[i];
      fb21 = ctls[13]->ctl_sig->s_block[i];
      fb22 = ctls[14]->ctl_sig->s_block[i];

      mu0 = ctls[15]->ctl_sig->s_block[i];
      mu1 = ctls[16]->ctl_sig->s_block[i];
      mu2 = ctls[17]->ctl_sig->s_block[i];

      for(j=0; j<4; j++)
	{
	  //printf("%f %f %f\n", data1[0], fb1,
	  /*y0*/
	  data0[0] = rk_child_stage(j, x->rk_children[0]);
	  /*y0*/
	  data0[1] = rk_child_stage(j, x->rk_children[1]);
	  
	  /*x1*/
	  data1[0] = rk_child_stage(j, x->rk_children[2]);
	  /*y1*/
	  data1[1] = rk_child_stage(j, x->rk_children[3]);
	  
	  /*x2*/
	  data2[0] = rk_child_stage(j, x->rk_children[4]);
	  /*y2*/
	  data2[1] = rk_child_stage(j, x->rk_children[5]);
	  
	  
	  r0 = sqrt(data0[0]*data0[0] + data0[1]*data0[1]);
	  r1 = sqrt(data1[0]*data1[0] + data1[1]*data1[1]);
	  r2 = sqrt(data2[0]*data2[0] + data2[1]*data2[1]);
	  
	  x->rk_children[0]->ks[j+1] =
	    x_dot(data0, omega0, alpha0, r0, data0[0]*fb00 + data1[0]*fb01 + data2[0]*fb02);
	  
	  x->rk_children[1]->ks[j+1] =
	    y_dot(data0, omega0, alpha0, r0, mu0,
		  data0[0]*fb00 + data1[0]*fb01 + data2[0]*fb02);
	  
	  x->rk_children[2]->ks[j+1] =
	    x_dot(data1, omega1, alpha1, r1, data0[0]*fb10 + data1[0]*fb11 + data2[0]*fb12);
	  
	  x->rk_children[3]->ks[j+1] =
	    y_dot(data1, omega1, alpha1, r1, mu1,
		  data0[0]*fb10 + data1[0]*fb11 + data2[0]*fb12);
	  
	  x->rk_children[4]->ks[j+1] =
	    x_dot(data2, omega2, alpha2, r2, data0[0]*fb20 + data1[0]*fb21 + data2[0]*fb22);
	  
	  x->rk_children[5]->ks[j+1] =
	    y_dot(data2, omega2, alpha2, r2, mu2,
		  data0[0]*fb20 + data1[0]*fb21 + data2[0]*fb22);
	  
	}
      

      
      rk_child_estimate(x->rk_children[0]);
      rk_child_estimate(x->rk_children[1]);
      rk_child_estimate(x->rk_children[2]);
      rk_child_estimate(x->rk_children[3]);
      rk_child_estimate(x->rk_children[4]);
      rk_child_estimate(x->rk_children[5]);

      cur_states[0] = x->rk_children[0]->state;
      cur_states[1] = x->rk_children[2]->state;
      cur_states[2] = x->rk_children[4]->state;

      
      old_states[0] = x->o_sigs[0]->s_block[i] = x->rk_children[0]->state;
      x->o_sigs[1]->s_block[i] = x->rk_children[1]->state;
      old_states[1] = x->o_sigs[2]->s_block[i] = x->rk_children[2]->state;
      x->o_sigs[3]->s_block[i] = x->rk_children[3]->state;
      old_states[2] = x->o_sigs[4]->s_block[i] = x->rk_children[4]->state;
      x->o_sigs[5]->s_block[i] = x->rk_children[5]->state;
      
    }

}

void setup_this(void){
  
  int i;
  int h = g_h;
  int up = g_up;

  rk_osc = (t_rk_mother *)rk_mother_init(&rk_osc_func,
					 6,//2 children for each of to oscillators
					 0,//get rid of this arg
					 h,//step size factor
					 up);//upsampling factor

  rk_osc->rk_children[0]->state = -1.0;
  rk_osc->rk_children[2]->state = -1.0;
  rk_osc->rk_children[4]->state = -1.0;
  
  dwnsample = dwnsample_init(2, h*up);
  dwnsample->i_sigs[0] = rk_osc->o_sigs[1];
  dwnsample->i_sigs[1] = rk_osc->o_sigs[3];

  ctls = (t_lin_ctl **)malloc(sizeof(t_lin_ctl*) * N_CTLS);
  dac_amp_ctls = (t_lin_ctl **)malloc(sizeof(t_lin_ctl*) * 2);
  sync_matrix_ctls = (t_lin_ctl **)malloc(sizeof(t_lin_ctl*) * N_MATRIX);

  for(i=0; i<2; i++)
    {
      dac_amp_ctls[i] = init_lin_ctl(i, CTL_T_LIN, h*up);//0,1
      level_lin_ctl(dac_amp_ctls[i], .707);
      printf("installing dac ctls %d\n", N_CTLS+N_MATRIX+i);
    }

  reset_ctl = init_lin_ctl(2, CTL_T_BANG, h*up);//2
  printf("installing reset ctl %d\n", N_CTLS+N_MATRIX+i);
  reset_ctl->do_bang = reset_oscs;

  for(i=0; i<N_CTLS; i++)
    ctls[i] = init_lin_ctl(3+i, CTL_T_LIN, h*up);//3-3+N_CTLS

  level_lin_ctl(ctls[0], omega0);
  level_lin_ctl(ctls[1], omega1);
  level_lin_ctl(ctls[2], omega2);
  level_lin_ctl(ctls[3], alpha0);
  level_lin_ctl(ctls[4], alpha1);
  level_lin_ctl(ctls[5], alpha2);

  level_lin_ctl(ctls[6], fb00);
  level_lin_ctl(ctls[7], fb01);
  level_lin_ctl(ctls[8], fb02);
  level_lin_ctl(ctls[9], fb10);
  level_lin_ctl(ctls[10], fb11);
  level_lin_ctl(ctls[11], fb12);
  level_lin_ctl(ctls[12], fb20);
  level_lin_ctl(ctls[13], fb21);
  level_lin_ctl(ctls[14], fb22);

  level_lin_ctl(ctls[15], mu0);
  level_lin_ctl(ctls[16], mu1);
  level_lin_ctl(ctls[17], mu2);

  for(i=0; i<N_MATRIX; i++)
    {
      printf("installing sync matrix ctl %d\n", 3+N_CTLS+i);//etc
      sync_matrix_ctls[i] = init_lin_ctl(N_CTLS+i, CTL_T_TOGGLE, h*up);
    }

  install_obj(&rk_osc->od);
  install_obj(&dwnsample->od);


}


void do_kill(void){

  int i;

  kill_rk_mother(rk_osc);
  kill_dwnsample(dwnsample);

  free(dac_amp_ctls);
  sync_matrix_ctls = NULL;

  free(sync_matrix_ctls);
  sync_matrix_ctls = NULL;

  free(ctls);
  ctls = NULL;



}

void dsp_chain(int samples,
	       t_tlsig **adc_sigs,
	       t_tlsig **dac_sigs){
  
  int i, j;
  int s = samples;
  
  rk_osc->dsp_func(s, rk_osc, NULL);
  dwnsample->dsp_func(s, dwnsample);
  
  dac_sigs[0] = dwnsample->o_sigs[0];
  dac_sigs[1] = dwnsample->o_sigs[1];
  

  multiply_sigs(dac_sigs[0], dac_amp_ctls[0]->ctl_sig);
  multiply_sigs(dac_sigs[1], dac_amp_ctls[1]->ctl_sig);

  
}



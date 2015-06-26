#include "stdio.h"
#include "math.h"
#include "stdlib.h"
//#include "a_pa.h"
#include "g_api.h"
//#include "ui_main.h"
#include "m_modules.h"
#include "m_ode_prim.h"
#include "limits.h"


static t_rk_mother *rk_osc;
static t_rk_mother *rk_moog;
static t_upsample *upsample;
static t_dwnsample *dwnsample;

t_tlsmp old_state1;
t_tlsmp old_state2;

int g_h  = 1;
int g_up = 1;

t_lin_ctl **ctls;

t_lin_ctl *reset_ctl;
t_lin_ctl **dac_amp_ctls;

#define N_CTLS 14

t_tlsmp sqr_flyback0 = 72000.0;
t_tlsmp sqr_flyback1 = 72000.0;

t_tlsmp sqr_curl0 = 500.0;
t_tlsmp sqr_curl1 = 500.0;

t_tlsmp sqr_gain0 = 500.0;;
t_tlsmp sqr_gain1 = 400.0;

t_tlsmp tri_curl0 = 5.0;
t_tlsmp tri_curl1 = 5.0;

t_tlsmp tri_thresh0 = .9;
t_tlsmp tri_thresh1 = .9;

t_tlsmp fb00 = 0.0;
t_tlsmp fb01 = 0.0;
t_tlsmp fb10 = 0.0;
t_tlsmp fb11 = 0.0;


#define N_MATRIX 4
t_lin_ctl **sync_matrix_ctls;

int sync_matrix[2][2];

/*****************/

void reset_oscs(void){

  int i, j;

  for(i=0; i<4; i++)
    {
      rk_osc->rk_children[i]->state = 0.0;
      for(j=0; j<5; j++)
	rk_osc->rk_children[i]->ks[j] = 0.0;
    }

      rk_osc->rk_children[1]->state = 1.0;
      rk_osc->rk_children[3]->state = 1.0;
  

}


//TODO: make these into a single function
void sync_by_matrix(int osc_no){
  
  int i, j;
  switch(osc_no)
    {

    case 0:
      //printf("resetting 1\n");
      for(i=0; i<2; i++)
	{
	  rk_osc->rk_children[i]->state = 0.0;
	  for(j=0; j<5; j++)
	    rk_osc->rk_children[i]->ks[j] = 0.0;
	}
      printf("sync0\n");
      
      rk_osc->rk_children[0]->state = 1.0;
      break;

    case 1:
      //printf("resetting 2\n");
      for(i=2; i<4; i++)
	{
	  rk_osc->rk_children[i]->state = 0.0;
	  for(j=0; j<5; j++)
	    rk_osc->rk_children[i]->ks[j] = 0.0;
	}
      printf("syc1\n");
      
      rk_osc->rk_children[2]->state = 1.0;
      break;

    }
}



void check_sync_matrix(t_tlsmp *old_s, t_tlsmp *cur_s){

  int i, j, k;

  for(i=0, k=0; i<2; i++)
    for(j=0; j<2; j++,  k=i*2+j)
      if(sync_matrix_ctls[k]->toggle_flag == 1)
	{
	  //printf("osc %d synced by ctl_matrix ref %d\n", i, k);
	  if(old_s[j]<0.0 & cur_s[j]>=0.0)sync_by_matrix(i);
	}

}

static t_tlsmp tanh_clip(t_tlsmp x, t_tlsmp gain){

  t_tlsmp out = tanh(x * gain);
  return(out);
}

static t_tlsmp x_dot(t_tlsmp *data, t_tlsmp gain, t_tlsmp curl, t_tlsmp fb0, t_tlsmp fb1){//x: triangle

  t_tlsmp out;
  out = (gain + gain * ((fb0*data[0]) * (fb1*data[1]))) * tanh_clip(data[1], curl);
  return out;

}

static t_tlsmp y_dot(t_tlsmp *data, t_tlsmp thresh, t_tlsmp flyback){//y: square

  t_tlsmp out = 0.0;

  if(data[0]>thresh)out = -1.0*flyback;
  if(data[0]<(-1.0*thresh))out = flyback;
  return(out);

}

static void rk_osc_func(int samples, void *ptr, t_tlsmp *input){

  t_rk_mother *x = ptr;
  int s = samples * x->h * x->up;
  int i, j;

  t_tlsmp data0[2], data1[2];
  t_tlsmp old_states[2], cur_states[2];
  
  for(i=0; i< s; i++)
    {

      sqr_flyback0 = ctls[0]->c_sig[i];
      sqr_flyback1 = ctls[1]->c_sig[i];
      sqr_curl0    = ctls[2]->c_sig[i];
      sqr_curl1    = ctls[3]->c_sig[i];
      sqr_gain0    = ctls[4]->c_sig[i];
      sqr_gain1    = ctls[5]->c_sig[i];
      tri_curl0    = ctls[6]->c_sig[i];
      tri_curl1    = ctls[7]->c_sig[i];
      tri_thresh0  = ctls[8]->c_sig[i];
      tri_thresh1  = ctls[9]->c_sig[i];

      fb00         = ctls[10]->c_sig[i];
      fb01         = ctls[11]->c_sig[i];
      fb10         = ctls[12]->c_sig[i];
      fb11         = ctls[13]->c_sig[i];

      for(j=0; j<4; j++)
  	{
	  //printf("%f %f %f\n", data1[0], fb1, 	  
  	  /*x0*/
  	  data0[0] = rk_child_stage(j, x->rk_children[0]);
  	  /*y0*/
  	  data0[1] = rk_child_stage(j, x->rk_children[1]);
	  
  	  /*x1*/
  	  data1[0] = rk_child_stage(j, x->rk_children[2]);
  	  /*y1*/
  	  data1[1] = rk_child_stage(j, x->rk_children[3]);

  	  x->rk_children[0]->ks[j+1] = 
  	    x_dot(data0, sqr_gain0, tri_curl0, fb00, fb01);
	  
  	  x->rk_children[1]->ks[j+1] = 
  	    y_dot(data0, tri_thresh0, sqr_flyback0);
	  
  	  x->rk_children[2]->ks[j+1] = 
  	    x_dot(data1, sqr_gain1, tri_curl1, fb10, fb11);
	  
  	  x->rk_children[3]->ks[j+1] = 
  	    y_dot(data1, tri_thresh1, sqr_flyback1);
	  
  	}


      
      rk_child_estimate(x->rk_children[0]);
      rk_child_estimate(x->rk_children[1]);
      rk_child_estimate(x->rk_children[2]);
      rk_child_estimate(x->rk_children[3]);
      
      cur_states[0] = x->rk_children[0]->state;
      cur_states[1] = x->rk_children[2]->state;
      
      check_sync_matrix(old_states, cur_states);
      
      old_states[0] = x->o_sigs[0]->s_block[i] = 
	x->rk_children[0]->state;
      x->o_sigs[1]->s_block[i] = x->rk_children[1]->state = 
	tanh_clip(x->rk_children[1]->state, sqr_curl0);
      old_states[1] = x->o_sigs[2]->s_block[i] = 
	x->rk_children[2]->state;
      x->o_sigs[3]->s_block[i] = x->rk_children[3]->state = 
	tanh_clip(x->rk_children[3]->state, sqr_curl1);
            
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

  rk_osc->rk_children[1]->state = 1.0;
  rk_osc->rk_children[3]->state = 1.0;
  
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
  
  level_lin_ctl(ctls[0], sqr_flyback0);
  level_lin_ctl(ctls[1], sqr_flyback1);
  level_lin_ctl(ctls[2], sqr_curl0);
  level_lin_ctl(ctls[3], sqr_curl1);
  level_lin_ctl(ctls[4], sqr_gain0);
  level_lin_ctl(ctls[5], sqr_gain1);
  level_lin_ctl(ctls[6], tri_curl0);
  level_lin_ctl(ctls[7], tri_curl1);
  level_lin_ctl(ctls[8], tri_thresh0);
  level_lin_ctl(ctls[9], tri_thresh1);
  level_lin_ctl(ctls[10], fb00);
  level_lin_ctl(ctls[11], fb01);
  level_lin_ctl(ctls[12], fb10);
  level_lin_ctl(ctls[13], fb11);


  for(i=0; i<N_MATRIX; i++)
    {
      printf("installing sync matrix ctl %d\n", 3+N_CTLS+i);//etc
      sync_matrix_ctls[i] = init_lin_ctl(N_CTLS+i, CTL_T_TOGGLE, h*up);
    }

  install_obj(&rk_osc->od);
  install_obj(&dwnsample->od);


}


void do_kill(void){

  kill_rk_mother(rk_osc);
  kill_dwnsample(dwnsample);

  free(dac_amp_ctls);
  dac_amp_ctls = NULL;

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

  multiply_sigs(dwnsample->o_sigs[0], dac_amp_ctls[0]->ctl_sig);
  multiply_sigs(dwnsample->o_sigs[1], dac_amp_ctls[1]->ctl_sig);
    
  dac_sigs[0] = dwnsample->o_sigs[0];
  dac_sigs[1] = dwnsample->o_sigs[1];


}




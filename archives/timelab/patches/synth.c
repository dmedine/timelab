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

t_sample old_state1;
t_sample old_state2;

int g_h  = 1;
int g_up = 1;

t_lin_ctl **ctls;

t_lin_ctl *reset_ctl;
t_lin_ctl **dac_amp_ctls;

#define N_CTLS 14

t_sample sqr_flyback0 = 72000.0;
t_sample sqr_flyback1 = 72000.0;

t_sample sqr_curl0 = 500.0;
t_sample sqr_curl1 = 500.0;

t_sample sqr_gain0 = 500.0;
t_sample sqr_gain1 = 400.0;


t_sample tri_curl0 = 500.0;
t_sample tri_curl1 = 500.0;

t_sample tri_thresh0 = .9;
t_sample tri_thresh1 = .9;

t_sample fb00 = 0.0;
t_sample fb01 = 0.0;
t_sample fb10 = 0.0;
t_sample fb11 = 0.0;


#define N_MATRIX 4
t_lin_ctl **sync_matrix_ctls;

int sync_matrix[2][2];

//---------------------------
t_lin_ctl *c_cutoff0, *c_res0;
t_lin_ctl *c_cutoff1, *c_res1;
t_signal  *moog_in0, *moog_in1;

#define CUTOFF0     c_cutoff0->ctl_sig->s_block
#define RES0        c_res0->ctl_sig->s_block
#define CUTOFF1     c_cutoff1->ctl_sig->s_block
#define RES1        c_res1->ctl_sig->s_block

/*****************/

void reset_moog(void){
int i, j;

  for(i=0; i<8; i++)
    {
      rk_moog->rk_children[i]->state = 0.0;
      for(j=0; j<5; j++)
	rk_moog->rk_children[i]->ks[j] = 0.0;
    }
}

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

reset_moog();
  

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
      
      rk_osc->rk_children[2]->state = 1.0;
      break;

    }
}



void check_sync_matrix(t_sample *old_s, t_sample *cur_s){

  int i, j, k;

  for(i=0, k=0; i<2; i++)
    for(j=0; j<2; j++,  k=i*2+j)
      if(sync_matrix_ctls[k]->toggle_flag == 1)
	{
	  //printf("osc %d synced by ctl_matrix ref %d\n", i, k);
	  if(old_s[j]<0.0 & cur_s[j]>=0.0)sync_by_matrix(i);
	}

}

static t_sample tanh_clip(t_sample x, t_sample gain){

  t_sample out = tanh(x * gain);
  return(out);
}

static t_sample x_dot(t_sample *data, t_sample gain, t_sample curl, t_sample fb0, t_sample fb1){//x: triangle

  t_sample out;
  out = (gain + fb0  + fb1) * tanh_clip(data[1], curl);
  return out;

}

static t_sample y_dot(t_sample *data, t_sample thresh, t_sample flyback){//y: square

  t_sample out = 0.0;

  if(data[0]>thresh)out = -1.0*flyback;
  if(data[0]<(-1.0*thresh))out = flyback;
  return(out);

}

static void rk_osc_func(int samples, void *ptr, t_sample *input){

  t_rk_mother *x = ptr;
  int s = samples * x->h * x->up;
  int i, j;

  t_sample data0[2], data1[2];
  t_sample old_states[2], cur_states[2];
  
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
  	    x_dot(data0, sqr_gain0, tri_curl0, fb00*data0[0], fb01*data1[0]);
	  
  	  x->rk_children[1]->ks[j+1] = 
  	    y_dot(data0, tri_thresh0, sqr_flyback0);
	  
  	  x->rk_children[2]->ks[j+1] = 
  	    x_dot(data1, sqr_gain1, tri_curl1, fb10*data0[0],  fb11 * data1[0]);
	  
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
      
      x->o_sigs[1]->s_block[i] = 
	tanh_clip(x->rk_children[1]->state, sqr_curl0);
      
      old_states[1] = x->o_sigs[2]->s_block[i] = 
	x->rk_children[2]->state;
      
      x->o_sigs[3]->s_block[i] = 
	tanh_clip(x->rk_children[3]->state, sqr_curl1);
            
    }

}

//---------------------------------------------

static t_sample diff(t_sample alpha, t_sample *data){

  t_sample out;
  out = alpha * tanh((data[0] - data[1]));
  return out;

}

static void rk_moog_func(int samples, void *ptr, t_sample *input){

  t_rk_mother *x = (t_rk_mother *)ptr;
  t_sample data0[2], data1[2];
  t_sample alpha0, alpha1;
  t_sample res0, res1;
  t_sample in0, in1;

  int s = samples;
  int i, j, k;
  
  for(i=0; i<s; i++)
    {
      alpha0    = CUTOFF0[i] * M_PI;
      res0      = RES0[i];
      alpha1    = CUTOFF1[i] * M_PI;
      res1      = RES1[i];
      in0       = moog_in0->s_block[i];
      in1       = moog_in1->s_block[i];
      
      //ladder filter
      for(j=0; j<4; j++)
	{
	  
	  //now the filter0
	  //stage 1 is unique
	  data0[0] = in0 -	    
	    res0 * rk_child_stage(j, x->rk_children[3]);
	  //data[0] is the input to the filter minus the feedback from the other end

	  data0[1] = rk_child_stage(j, x->rk_children[0]);
	  //data[1] is the current state here

	  x->rk_children[0]->ks[j+1] = 
	    diff(alpha0, data0);

	  //the other three stages are the same
	  //input to that stage (previous stage) and current state at that stage
	  for(k=1; k<4; k++)
	    {
	      data0[0] = rk_child_stage(j, x->rk_children[k-1]);
	      data0[1] = rk_child_stage(j, x->rk_children[k]);
	      x->rk_children[k]->ks[j+1] = 
		diff(alpha0, data0);
	    }

	  //now the filter1
	  //stage 1 is unique
	  data1[0] = in1 -	    
	    res1 * rk_child_stage(j, x->rk_children[7]);
	  //data[0] is the input to the filter minus the feedback from the other end

	  data1[1] = rk_child_stage(j, x->rk_children[4]);
	  //data[1] is the current state here
	  
	  x->rk_children[4]->ks[j+1] = 
	    diff(alpha1, data1);

	  for(k=5; k<8; k++)
	    {
	      data1[0] = rk_child_stage(j, x->rk_children[k-1]);
	      data1[1] = rk_child_stage(j, x->rk_children[k]);
	      x->rk_children[k]->ks[j+1] = 
		diff(alpha1, data1);
	    }
	}

      //estimate
      rk_child_estimate(x->rk_children[0]);
      rk_child_estimate(x->rk_children[1]);
      rk_child_estimate(x->rk_children[2]);
      rk_child_estimate(x->rk_children[3]);
  

      x->o_sig0[i] = x->rk_children[0]->state;
      x->o_sig1[i] = x->rk_children[1]->state;
      x->o_sig2[i] = x->rk_children[2]->state;
      x->o_sig3[i] = x->rk_children[3]->state;


      //estimate
      rk_child_estimate(x->rk_children[4]);
      rk_child_estimate(x->rk_children[5]);
      rk_child_estimate(x->rk_children[6]);
      rk_child_estimate(x->rk_children[7]);
  

      x->o_sig4[i] = x->rk_children[4]->state;
      x->o_sig5[i] = x->rk_children[5]->state;
      x->o_sig6[i] = x->rk_children[6]->state;
      x->o_sig7[i] = x->rk_children[7]->state;
  

    }
  //printf("%f %f %f %f\n", alpha0, res0, alpha1, res1);
}



//---------------------------------------------



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


  rk_moog = (t_rk_mother *)rk_mother_init(&rk_moog_func,
					  8,//4 filter stages 
					  0,
					  h,
					  up);
  
  moog_in0 = init_one_signal(O_TYPE, h*up);
  moog_in1 = init_one_signal(O_TYPE, h*up);

  dwnsample = dwnsample_init(2, h*up);
  dwnsample->i_sigs[0] = rk_moog->o_sigs[0];
  dwnsample->i_sigs[1] = rk_moog->o_sigs[4];

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

  c_cutoff0 = init_lin_ctl(3+N_CTLS+N_MATRIX, CTL_T_LIN, h*up);
  c_res0    = init_lin_ctl(4+N_CTLS+N_MATRIX, CTL_T_LIN, h*up);
  c_cutoff1 = init_lin_ctl(5+N_CTLS+N_MATRIX, CTL_T_LIN, h*up);
  c_res1    = init_lin_ctl(6+N_CTLS+N_MATRIX, CTL_T_LIN, h*up);

  install_obj(&rk_osc->od);
  install_obj(&rk_moog->od);
  install_obj(&dwnsample->od);


}


void do_kill(void){

  kill_rk_mother(rk_osc);
  kill_rk_mother(rk_moog);
  kill_dwnsample(dwnsample);

  kill_one_signal(moog_in0);
  kill_one_signal(moog_in1);

  free(dac_amp_ctls);
  dac_amp_ctls = NULL;

  free(sync_matrix_ctls);
  sync_matrix_ctls = NULL;

  free(ctls);
  ctls = NULL;


}

void dsp_chain(int samples, 
	       t_signal **adc_sigs,
	       t_signal **dac_sigs){
  
  int i, j;
  int s = samples;

  rk_osc->dsp_func(s, rk_osc, NULL);  

  cpy_signal_vals(moog_in0, rk_osc->o_sigs[0]);
  cpy_signal_vals(moog_in1, rk_osc->o_sigs[2]);

  rk_moog->dsp_func(s, rk_moog, NULL);
  //add mechanism for switching signal inputs with toggles

  dwnsample->dsp_func(s, dwnsample);

  scale_signal_vec(dwnsample->o_sigs[0], dac_amp_ctls[0]->ctl_sig);
  scale_signal_vec(dwnsample->o_sigs[1], dac_amp_ctls[1]->ctl_sig);
    
  dac_sigs[0] = dwnsample->o_sigs[0];
  dac_sigs[1] = dwnsample->o_sigs[1];


}




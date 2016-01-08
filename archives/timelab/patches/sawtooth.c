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
int stage0, stage1;

t_tlsmp old_state1;
t_tlsmp old_state2;

int g_h  = 1;
int g_up = 1;

t_lin_ctl **ctls;
t_lin_ctl *reset_ctl;
t_lin_ctl **dac_amp_ctls;

#define N_CTLS 8

t_tlsmp flyback0 = 72000.0;
t_tlsmp flyback1 = 72000.0;

t_tlsmp curl0 = 500.0;
t_tlsmp curl1 = 500.0;

t_tlsmp thresh0 = .9;
t_tlsmp thresh1 = .9;

t_tlsmp gain0 = 400;
t_tlsmp gain1 = 500;


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
      stage0 = 0;
      stage1 = 0;
  

}



static t_tlsmp tanh_clip(t_tlsmp x, t_tlsmp gain){

  t_tlsmp out = tanh(x * gain);
  return(out);
}

static t_tlsmp x_dot(t_tlsmp datum, t_tlsmp gain){

  t_tlsmp out;
  out = gain * datum;
  return out;
  
}

static t_tlsmp y_dot(t_tlsmp datum, t_tlsmp thresh, t_tlsmp flyback, int *stage){

  t_tlsmp out;
  switch(*stage)
    {
      
    case 0://going up
      {
	if(datum>thresh)
	  {
	    *stage = 1;//
	    out = -1.0 * flyback;
	    break;
	  }
	else
	  {
	    out = 1.0;     
	    break;    
	  }
	
      case 1://going down
	if(datum<(-1.0 * thresh))
	  {
	    *stage = 0;
	    out = 1.0;
	    break;
	  }
	else
	  {
	    out = -1.0 * flyback;
	    break;
	  }
      }
    }

  return out;

}


static void rk_osc_func(int samples, void *ptr, t_tlsmp *input){

  t_rk_mother *x = ptr;
  int s = samples * x->h * x->up;
  int i, j;

  t_tlsmp data0, data1;
  t_tlsmp old_states[2], cur_states[2];
  
  for(i=0; i< s; i++)
    {

      flyback0 = ctls[0]->c_sig[i];
      flyback1 = ctls[1]->c_sig[i];
      curl0    = ctls[2]->c_sig[i];
      curl1    = ctls[3]->c_sig[i];
      thresh0  = ctls[4]->c_sig[i];
      thresh1  = ctls[5]->c_sig[i];
      gain0    = ctls[6]->c_sig[i];
      gain1    = ctls[7]->c_sig[i];


      for(j=0; j<4; j++)
  	{
	  //printf("%f %f %f\n", data1[0], fb1, 	  
  	  /*x0*/
  	  data0 = rk_child_stage(j, x->rk_children[0]);
	  
  	  /*x1*/
  	  data1 = rk_child_stage(j, x->rk_children[2]);
 

  	  x->rk_children[0]->ks[j+1] = 
  	    x_dot(data0, gain0);
  	  x->rk_children[1]->ks[j+1] = 
  	    y_dot(data0, thresh0, flyback0, &stage0);
	  
  	  x->rk_children[2]->ks[j+1] = 
  	    x_dot(data1, gain1);	  
  	  x->rk_children[3]->ks[j+1] = 
  	    y_dot(data1, thresh1, flyback1, &stage1);

	  
  	}
      
      rk_child_estimate(x->rk_children[0]);
      rk_child_estimate(x->rk_children[1]);
      rk_child_estimate(x->rk_children[2]);
      rk_child_estimate(x->rk_children[3]);

      x->o_sigs[0]->s_block[i] = 
	x->rk_children[0]->state;
      x->o_sigs[1]->s_block[i] = x->rk_children[1]->state = 
	tanh_clip(x->rk_children[1]->state, curl0); 

     x->o_sigs[2]->s_block[i] = 
	x->rk_children[1]->state;
     x->o_sigs[3]->s_block[i] = x->rk_children[3]->state = 
	tanh_clip(x->rk_children[3]->state, curl1); 
 
            
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

  stage0 = 0;
  stage1 = 0;

  rk_osc->rk_children[1]->state = 1.0;
  rk_osc->rk_children[3]->state = 1.0;
  
  dwnsample = dwnsample_init(2, h*up);
  dwnsample->i_sigs[0] = rk_osc->o_sigs[0];
  dwnsample->i_sigs[1] = rk_osc->o_sigs[1];

  ctls = (t_lin_ctl **)malloc(sizeof(t_lin_ctl*) * N_CTLS);
  dac_amp_ctls = (t_lin_ctl **)malloc(sizeof(t_lin_ctl*) * 2);
  
  for(i=0; i<2; i++)
    {
      dac_amp_ctls[i] = init_lin_ctl(i, CTL_T_LIN, h*up);//0,1
      level_lin_ctl(dac_amp_ctls[i], .707);
    }

  reset_ctl = init_lin_ctl(2, CTL_T_BANG, h*up);//2
  reset_ctl->do_bang = reset_oscs;

  for(i=0; i<N_CTLS; i++)
    ctls[i] = init_lin_ctl(3+i, CTL_T_LIN, h*up);//3-3+N_CTLS
  
  level_lin_ctl(ctls[0], flyback0);
  level_lin_ctl(ctls[1], flyback1);
  level_lin_ctl(ctls[2], curl0);
  level_lin_ctl(ctls[3], curl1);
  level_lin_ctl(ctls[4], thresh0);
  level_lin_ctl(ctls[5], thresh1);
  level_lin_ctl(ctls[6], gain0);
  level_lin_ctl(ctls[7], gain1);

  install_obj(&rk_osc->od);
  install_obj(&dwnsample->od);


}


void do_kill(void){

  kill_rk_mother(rk_osc);
  kill_dwnsample(dwnsample);

  free(dac_amp_ctls);
  dac_amp_ctls = NULL;

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




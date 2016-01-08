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
static t_ie_mother *ie_osc;
static t_rk_mother *rk_moog;
static t_upsample *upsample;
static t_dwnsample *dwnsample;

int g_h  = 1;
int g_up = 2;

t_lin_ctl **ctls;

t_lin_ctl *reset_ctl;
t_lin_ctl **dac_amp_ctls;

#define N_CTLS 12

t_tlsmp sqr_flyback0 = 72000.0;
t_tlsmp sqr_flyback1 = 72000.0;

t_tlsmp curl0 = 5.0;
t_tlsmp curl1 = 5.0;

t_tlsmp freq0 = 500.0;
t_tlsmp freq1 = 400.0;

t_tlsmp tri_thresh0 = .9;
t_tlsmp tri_thresh1 = .9;

t_tlsmp skew0 = .5;
t_tlsmp skew1 = .5;

t_tlsmp tcurl0 = 5.0;
t_tlsmp tcurl1 = 5.0;

#define SLP_DN 0
#define SLP_UP 1

int dir0, dir1;
t_tlsmp scale = 1000.0;

//---------------------------
t_tlsmp slp_up0;
t_tlsmp slp_dn0;
t_tlsmp slp_up1;
t_tlsmp slp_dn1;


void find_slopes(t_tlsmp freq, t_tlsmp *up, t_tlsmp *dn, t_tlsmp skew){

  t_tlsmp total_slope = .004*freq;
  if(skew>=1.0)skew = .99;
  if(skew<=0.0)skew = .01;
  *up = skew * total_slope;
  *dn = total_slope - *up;

  //printf("freq %f skew %f up %f dn %f\n", freq, skew, *up, *dn);  


}

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
  dir0 = SLP_UP;  
  dir1 = SLP_UP;
  
}


static t_tlsmp tanh_clip(t_tlsmp x, t_tlsmp gain){

  t_tlsmp out = tanh(x * gain);
  return(out);
}

static t_tlsmp tri(t_tlsmp *data, int dir){//x: triangle

  t_tlsmp out = data[1] * scale;
  //printf("data[0] %f data [1] %f up %f dn %f %d\n",data[0], data[1], up, dn, dir); 
 
  /* if(dir==SLP_UP) */
  /*   out = data[1];// * up * scale;//tanh_clip(data[1], curl) * up; */

  /* else */
  /*   out = data[1];// * dn * scale;//tanh_clip(data[1], curl) * dn; */
  //printf("x out %f\n", out);
  return out;

}


static t_tlsmp sqr(t_tlsmp *data, t_tlsmp thresh, t_tlsmp flyback,  int *dir){

  t_tlsmp out = 0.0;
  if(data[0]>thresh)
    {
      out = -1.0*flyback;
      *dir = SLP_DN;
    }

  if(data[0]<(-1.0*thresh))
    {
      out = flyback;
      *dir = SLP_UP;
    }
  // printf("data[0] %f data[1] %f thresh %f dir %d out %f\n", data[0], data[1], thresh, *dir, out); 
  return out;

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
      curl0        = ctls[2]->c_sig[i];
      curl1        = ctls[3]->c_sig[i];
      freq0        = ctls[4]->c_sig[i];
      freq1        = ctls[5]->c_sig[i];
      tri_thresh0  = ctls[6]->c_sig[i];
      tri_thresh1  = ctls[7]->c_sig[i];
      skew0        = ctls[8]->c_sig[i];
      skew1        = ctls[9]->c_sig[i];
      tcurl0       = ctls[10]->c_sig[i];
      tcurl1       = ctls[11]->c_sig[i];

      find_slopes(freq0, &slp_up0, &slp_dn0, skew0);
      find_slopes(freq1, &slp_up1, &slp_dn1, skew1);


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
  	    tri(data0, dir0);
	  
  	  x->rk_children[1]->ks[j+1] = 
  	    sqr(data0, tri_thresh0, sqr_flyback0, &dir0);
	  
	  
  	}

      rk_child_estimate(x->rk_children[0]);
      rk_child_estimate(x->rk_children[1]);
      
      x->o_sigs[0]->s_block[i] = x->rk_children[0]->state;// = 
      //tanh_clip(x->rk_children[0]->state, tcurl0);
      if(dir0 ==SLP_UP)
	x->o_sigs[1]->s_block[i] = x->rk_children[1]->state = 
	   slp_up0 * tanh_clip(x->rk_children[1]->state, curl0);
      if(dir0 ==SLP_DN)
	x->o_sigs[1]->s_block[i] = x->rk_children[1]->state = 
	  slp_dn0 * tanh_clip(x->rk_children[1]->state, curl0);


      /* x->o_sigs[2]->s_block[i] =  */
      /* 	x->rk_children[2]->state; */
      /* x->o_sigs[3]->s_block[i] = x->rk_children[3]->state =  */
      /* 	tanh_clip(x->rk_children[3]->state, sqr_curl1); */
            
    }

}

static void ie_osc_func(int samples, void *ptr, t_tlsmp *input){

  t_ie_mother *x = ptr;
  int s = samples * x->h * x->up;
  int i, j;

  t_tlsmp data0[2], data1[2];
  t_tlsmp old_states[2], cur_states[2];
  int its = 1000;
  
  for(i=0; i< s; i++)
    {

      sqr_flyback0 = ctls[0]->c_sig[i];
      sqr_flyback1 = ctls[1]->c_sig[i];
      curl0        = ctls[2]->c_sig[i];
      curl1        = ctls[3]->c_sig[i];
      freq0        = ctls[4]->c_sig[i];
      freq1        = ctls[5]->c_sig[i];
      tri_thresh0  = ctls[6]->c_sig[i];
      tri_thresh1  = ctls[7]->c_sig[i];
      skew0        = ctls[8]->c_sig[i];
      skew1        = ctls[9]->c_sig[i];
      tcurl0       = ctls[10]->c_sig[i];
      tcurl1       = ctls[11]->c_sig[i];

      find_slopes(freq0, &slp_up0, &slp_dn0, skew0);
      find_slopes(freq1, &slp_up1, &slp_dn1, skew1);

      ie_child_begin(x->ie_children[0]);
      ie_child_begin(x->ie_children[1]);

      while(its--)
	{
	  data0[0] = x->ie_children[0]->y_k01;
	  data0[1] = x->ie_children[1]->y_k01;

	  x->ie_children[0]->y_k11 = x->ie_children[0]->y_k + x->h_time * 
	    tri(data0, dir0);

	  ie_child_iterate(x->ie_children[0]);

	  x->ie_children[1]->y_k11 = x->ie_children[1]->y_k + x->h_time * 
	    sqr(data0, tri_thresh0, sqr_flyback0, &dir0);

	  ie_child_iterate(x->ie_children[1]);

	  if(x->ie_children[0]->diff<=x->ie_children[0]->tol &&
	     x->ie_children[1]->diff<=x->ie_children[1]->tol) break;


	}

      
      x->o_sigs[0]->s_block[i] = x->ie_children[0]->state;// = 
      //tanh_clip(x->rk_children[0]->state, tcurl0);
      if(dir0 ==SLP_UP)
	{
	  x->ie_children[1]->state = 
	    tanh_clip(x->ie_children[1]->state, slp_up0);
	  x->o_sigs[1]->s_block[i] = x->ie_children[1]->state/slp_up0;
	}
	  
      if(dir0 ==SLP_DN)
	{
	  x->ie_children[1]->state =
	    tanh_clip(x->ie_children[1]->state, slp_dn0);
	  x->o_sigs[1]->s_block[i] = x->ie_children[1]->state/slp_dn0; 
	}
      /* x->o_sigs[2]->s_block[i] =  */
      /* 	x->rk_children[2]->state; */
      /* x->o_sigs[3]->s_block[i] = x->rk_children[3]->state =  */
      /* 	tanh_clip(x->rk_children[3]->state, sqr_curl1); */
            
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

  rk_osc->rk_children[0]->state = 1.0;
  rk_osc->rk_children[2]->state = 1.0;


  ie_osc = (t_ie_mother *)ie_mother_init(&ie_osc_func,
  					 4,//2 children for each of to oscillators
  					 0,//get rid of this arg
  					 h,//step size factor
					 up);//upsampling factor

  ie_osc->ie_children[1]->state = 1.0;
  ie_osc->ie_children[3]->state = 1.0;

  dir0 = SLP_UP;  
  dir1 = SLP_UP;  

  dwnsample = dwnsample_init(2, h*up);
  dwnsample->i_sigs[0] = rk_osc->o_sigs[1];
  dwnsample->i_sigs[1] = rk_osc->o_sigs[3];

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
  
  level_lin_ctl(ctls[0], sqr_flyback0);
  level_lin_ctl(ctls[1], sqr_flyback1);
  level_lin_ctl(ctls[2], curl0);
  level_lin_ctl(ctls[3], curl1);
  level_lin_ctl(ctls[4], freq0);
  level_lin_ctl(ctls[5], freq1);
  level_lin_ctl(ctls[6], tri_thresh0);
  level_lin_ctl(ctls[7], tri_thresh1);
  level_lin_ctl(ctls[8], skew0);
  level_lin_ctl(ctls[9], skew1);
  level_lin_ctl(ctls[10], tcurl0);
  level_lin_ctl(ctls[11], tcurl1);


  install_obj(&rk_osc->od);
  install_obj(&ie_osc->od);
  install_obj(&dwnsample->od);


}


void do_kill(void){

  kill_rk_mother(rk_osc);
  kill_ie_mother(ie_osc);
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
  //ie_osc->dsp_func(s, ie_osc, NULL);
  dwnsample->dsp_func(s, dwnsample);

  multiply_sigs(dwnsample->o_sigs[0], dac_amp_ctls[0]->ctl_sig);
  multiply_sigs(dwnsample->o_sigs[1], dac_amp_ctls[1]->ctl_sig);
    
  dac_sigs[0] = dwnsample->o_sigs[0];
  dac_sigs[1] = dwnsample->o_sigs[1];


}




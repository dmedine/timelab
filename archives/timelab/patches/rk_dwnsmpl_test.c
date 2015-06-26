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

t_lin_ctl *ctl_omega;

int g_h = 2;
int g_up = 1;

/*****************/

static t_tlsmp x_dot(t_tlsmp *data, t_tlsmp omega){

  
  t_tlsmp out = omega * data[1];
  return out;

}

static t_tlsmp y_dot(t_tlsmp *data, t_tlsmp omega){

  t_tlsmp out;
  
  out = -1.0 * omega * data[0];
  return(out);

}

static void rk_osc_func(int samples, void *ptr, t_tlsmp *input){

  t_rk_mother *x = ptr;
  t_tlsmp data0[2];
  int s = samples * x->up * x->h;
  int i, j;
  t_tlsmp omega0;
  
  for(i=0; i< s; i++)
    {

      omega0 = 2 * M_PI * 440;//ctl_omega->ctl_sig->s_block[i];

      for(j=0; j<4; j++)
	{
	  //printf("%f %f %f\n", data1[0], fb1,
	  /*y0*/
	  data0[0] = rk_child_stage(j, x->rk_children[0]);
	  /*y0*/
	  data0[1] = rk_child_stage(j, x->rk_children[1]);
		  
	  x->rk_children[0]->ks[j+1] =
	    x_dot(data0, omega0);
	  
	  x->rk_children[1]->ks[j+1] =
	    y_dot(data0, omega0);
	
	}      
      rk_child_estimate(x->rk_children[0]);
      rk_child_estimate(x->rk_children[1]);
      x->o_sigs[0]->s_block[i] = x->rk_children[0]->state;
      x->o_sigs[1]->s_block[i] = x->rk_children[1]->state;
            
    }

}

void setup_this(void){
  
  int i;
  int h = g_h;
  int up = g_up;

  rk_osc = (t_rk_mother *)rk_mother_init(&rk_osc_func,
					 2,//2 children for each of to oscillators
					 0,//get rid of this arg
					 h,//step size factor
					 up);//upsampling factor

  rk_osc->rk_children[0]->state = -1.0;

  dwnsample = dwnsample_init(1, h*up);
  dwnsample->i_sigs[0] = rk_osc->o_sigs[1];

  ctl_omega = init_lin_ctl(0, CTL_T_LIN, h*up);
  t_tlsmp f = 440.0 * 2.0 * M_PI;
  printf("f: %f\n", f);
  level_lin_ctl(ctl_omega, f);

  install_obj(&rk_osc->od);
  install_obj(&dwnsample->od);


}


void do_kill(void){

  int i;

  kill_rk_mother(rk_osc);
  kill_dwnsample(dwnsample);


}

void dsp_chain(int samples,
	       t_tlsig **adc_sigs,
	       t_tlsig **dac_sigs){
  
  int i, j;
  int s = samples;
  
  rk_osc->dsp_func(s, rk_osc, NULL);
  dwnsample->dsp_func(s, dwnsample);
  
  dac_sigs[0] = dwnsample->o_sigs[0];

  
}



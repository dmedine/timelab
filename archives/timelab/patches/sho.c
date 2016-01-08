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

int g_h = 1;
int g_up = 1;

t_lin_ctl *freq;
t_lin_ctl **dac_amp_ctls;

t_tlsmp omega = 440.0;



/*****************/

void reset_oscs(void);

static t_tlsmp x_dot(t_tlsmp *data, t_tlsmp om){

  
  t_tlsmp out = om *data[1];
  printf("%f %f\n", data[1], out);
  return out;

}

static t_tlsmp y_dot(t_tlsmp *data, t_tlsmp om){

  t_tlsmp out;
  
  // * (1.0 - (data[0]*data[0]))
  //  out = -1.0 * om * om* data[0];
  out = -1.0 * om* data[0];
  printf("%f %f\n", data[0], out);
  return(out);

}


static void rk_osc_func(int samples, void *ptr, t_tlsmp *input){

  t_rk_mother *x = ptr;
  t_tlsmp data[2];
  int s = samples * x->h * x->up;
  int i, j;
  
  for(i=0; i<s; i++)
    {

      omega = 440;//2 * M_PI * 440;//freq->ctl_sig->s_block[i];

      for(j=0; j<4; j++)
	{
	  //printf("%f %f %f\n", data1[0], fb1,
	  /*x0*/
	  data[0] = rk_child_stage(j, x->rk_children[0]);
	  /*y0*/
	  data[1] = rk_child_stage(j, x->rk_children[1]);
	  
	  x->rk_children[0]->ks[j+1] =
	    x_dot(data, omega);
	  
	  x->rk_children[1]->ks[j+1] =
	    y_dot(data, omega);
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
    

  dac_amp_ctls = (t_lin_ctl **)malloc(sizeof(t_lin_ctl*) * 2);

  for(i=0; i<2; i++)
    {
      dac_amp_ctls[i] = init_lin_ctl(i, CTL_T_LIN, h*up);//0,1
      level_lin_ctl(dac_amp_ctls[i], .7);
    }

  freq = init_lin_ctl(2, CTL_T_LIN, h*up);
  level_lin_ctl(freq, omega);
 
  install_obj(&rk_osc->od);
  
}


void do_kill(void){

  int i;

  kill_rk_mother(rk_osc);
  free(dac_amp_ctls);



}

void dsp_chain(int samples,
	       t_tlsig **adc_sigs,
	       t_tlsig **dac_sigs){
  
  int i, j;
  int s = samples;
  
  rk_osc->dsp_func(s, rk_osc, NULL);
    
  dac_sigs[0] = rk_osc->o_sigs[0];
  /* for(i=0;i<s;i++) */
  /*   printf("%d %f\n", i, rk_osc->o_sigs[0]->s_block[i]); */

  multiply_sigs(dac_sigs[0], dac_amp_ctls[0]->ctl_sig);
  multiply_sigs(dac_sigs[1], dac_amp_ctls[1]->ctl_sig);

  
}



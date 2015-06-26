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


t_lin_ctl *mass1_ctl, *mass2_ctl;
t_lin_ctl *sc1_ctl, *sc2_ctl, *sc3_ctl;
t_lin_ctl **dac_amp_ctls;

t_tlsmp mass1 = 1;
t_tlsmp mass2 = 1;
t_tlsmp sc1  = .5;
t_tlsmp sc2  = .5;
t_tlsmp sc3  = .5;



/*****************/

void reset_oscs(void);

static t_tlsmp mass_func(t_tlsmp *data, t_tlsmp k){

  //displacement
  t_tlsmp out = k*data[1];
  return out;

}

static t_tlsmp spring_func(t_tlsmp *data, t_tlsmp m){

  t_tlsmp out;
  
  //force
  out = -1.0 * m * data[0];
  return(out);

}


static void rk_osc_func(int samples, void *ptr, t_tlsmp *input){

  t_rk_mother *x = ptr;
  t_tlsmp data[2], data1[2], data2[2];
  int s = samples * x->h * x->up;
  int i, j;
  
  for(i=0; i<s; i++)
    {

      mass1 = mass1_ctl->ctl_sig->s_block[i];
      mass2 = mass2_ctl->ctl_sig->s_block[i];
      sc1  = sc1_ctl->ctl_sig->s_block[i];
      sc2  = sc2_ctl->ctl_sig->s_block[i];


      for(j=0; j<4; j++)
	{

	  data[0] = rk_child_stage(j, x->rk_children[0])+rk_child_stage(j, x->rk_children[2]);
	  data[1] = rk_child_stage(j, x->rk_children[1])+rk_child_stage(j, x->rk_children[3]);
	  	  
	  x->rk_children[0]->ks[j+1] =
	    y_dot(data, sc1);
	  
	  x->rk_children[1]->ks[j+1] =
	    x_dot(data, mass1);

	  x->rk_children[2]->ks[j+1] =
	    y_dot(data, sc2);
	  
	  x->rk_children[3]->ks[j+1] =
	    x_dot(data, mass2);

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
  rk_osc->rk_children[2]->state = -1.0;
    

  dac_amp_ctls = (t_lin_ctl **)malloc(sizeof(t_lin_ctl*) * 2);

  for(i=0; i<2; i++)
    {
      dac_amp_ctls[i] = init_lin_ctl(i, CTL_T_LIN, h*up);//0,1
      level_lin_ctl(dac_amp_ctls[i], .7);
    }

  mass1_ctl = init_lin_ctl(2, CTL_T_LIN, h*up);
  level_lin_ctl(mass1_ctl, mass1);

  mass2_ctl = init_lin_ctl(3, CTL_T_LIN, h*up);
  level_lin_ctl(mass2_ctl, mass2);

  sc1_ctl = init_lin_ctl(4, CTL_T_LIN, h*up);
  level_lin_ctl(sc1_ctl, sc1);

  sc2_ctl = init_lin_ctl(5, CTL_T_LIN, h*up);
  level_lin_ctl(sc2_ctl, sc2);

  sc3_ctl = init_lin_ctl(6, CTL_T_LIN, h*up);
  level_lin_ctl(sc3_ctl, sc3);
 
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
    
  dac_sigs[0] = rk_osc->o_sigs[1];
  dac_sigs[1] = rk_osc->o_sigs[3];
  

  multiply_sigs(dac_sigs[0], dac_amp_ctls[0]->ctl_sig);
  multiply_sigs(dac_sigs[1], dac_amp_ctls[1]->ctl_sig);

  
}



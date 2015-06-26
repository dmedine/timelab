#include "stdio.h"
#include "math.h"
#include "stdlib.h"
//#include "a_pa.h"
#include "g_api.h"
//#include "ui_main.h"
#include "m_modules.h"
#include "m_ode_prim.h"
#include "limits.h"

FILE *fp, *fp2;

static t_rk_mother *rk_osc;
static t_cd_mother *cd_osc;
static t_cd_mother *cd2_osc;

int g_h = 1;
int g_up = 1;

t_lin_ctl *ctl_freq;
t_lin_ctl *ctl_bvel;
t_lin_ctl *ctl_bforce;
t_lin_ctl *ctl_fric;
t_lin_ctl **dac_amp_ctls;

t_tlsmp freq = 100.0;//a string
t_tlsmp bvel = .2;//m/s
t_tlsmp bforce =  4000.0;//500.0;//force/mass -- m/s^2
t_tlsmp fric = 100.0;//friction parameter

t_tlsmp E_TO_HALF;

/*****************/

void reset_oscs(void);

static t_tlsmp phi(t_tlsmp alpha, t_tlsmp y){

  t_tlsmp out = sqrt(2*alpha) * y * exp(-2.0 * alpha * y * y + .5);

  //printf("%f %f %f %f\n", alpha, y, E_TO_HALF, out);
  return out;

}

static t_tlsmp x_dot(t_tlsmp *data, t_tlsmp omega, t_tlsmp alpha){

  
  t_tlsmp out = data[1] * omega;// - bforce*phi(alpha, data[1] - bvel);
  //printf("x_dot %f %f %f\n", out, omega, data[0]);
  return out;

}

static t_tlsmp y_dot(t_tlsmp *data, t_tlsmp omega, t_tlsmp alpha){

  t_tlsmp out; 
  
  out = (-1.0 * omega * data[0]) - bforce*phi(alpha, data[1] - bvel);
  //printf("y_dot %f %f %f\n", out, omega, data[0]);
  return(out);

}



static void rk_osc_func(int samples, void *ptr, t_tlsmp *input){

  t_rk_mother *x = ptr;
  t_tlsmp data[2];
  t_tlsmp ret[2];
  int s = samples * x->h * x->up;
  int i, j;
  
  for(i=0; i<s; i++)
    {

      freq   = 2 * M_PI * ctl_freq->ctl_sig->s_block[i];
      bvel   = ctl_bvel->ctl_sig->s_block[i];
      bforce = ctl_bforce->ctl_sig->s_block[i];
      fric   = ctl_fric->ctl_sig->s_block[i];

      for(j=0; j<4; j++)
	{
	  //printf("%f %f %f\n", data1[0], fb1,
	  /*x0*/
	  data[0] = rk_child_stage(j, x->rk_children[0]);
	  /*y0*/
	  data[1] = rk_child_stage(j, x->rk_children[1]);
	  
	  x->rk_children[0]->ks[j+1] =
	    x_dot(data, freq, fric);

	  
	  x->rk_children[1]->ks[j+1] =
	    y_dot(data, freq, fric);	    
	}
      

      
      rk_child_estimate(x->rk_children[0]);
      rk_child_estimate(x->rk_children[1]);
      
      x->o_sigs[0]->s_block[i] = x->rk_children[0]->state;
      x->o_sigs[1]->s_block[i] = x->rk_children[1]->state;
      x->o_sigs[2]->s_block[i] = x->rk_children[1]->state - bvel;
            
    }

}

static void cd_osc_func(int samples, void *ptr, t_tlsmp *input){

  t_cd_mother *x = ptr;
  t_tlsmp data[2];
  
  int s = samples * x->h * x->up;
  int i, j;
  
  for(i=0; i<s; i++)
    {

      freq   = 2 * M_PI * ctl_freq->ctl_sig->s_block[i];
      bvel   = ctl_bvel->ctl_sig->s_block[i];
      bforce = ctl_bforce->ctl_sig->s_block[i];
      fric   = ctl_fric->ctl_sig->s_block[i];

      //printf("%f %f %f\n", data1[0], fb1,
      /*x0*/
      data[0] = cd_child_stage(0, x->cd_children[0]);
      /*y0*/
      data[1] = cd_child_stage(1, x->cd_children[1]);
      //fprintf(fp, "data: %f %f\n", data[0], data[1]);
      
      x->cd_children[0]->sides[0] =
	x_dot(data, freq, fric);
      
      x->cd_children[1]->sides[1] =
	y_dot(data, freq, fric);
      //fprintf(fp, "sides: %f %f\n", x->cd_children[0]->sides[j], x->cd_children[1]->sides[j]);
      

      
      cd_child_estimate(x->cd_children[0]);
      cd_child_estimate(x->cd_children[1]);
      
      x->o_sigs[0]->s_block[i] = x->cd_children[0]->state;
      x->o_sigs[1]->s_block[i] = x->cd_children[1]->state;
      x->o_sigs[2]->s_block[i] = x->cd_children[1]->state - bvel;
            
    }

}


static void cd2_osc_func(int samples, void *ptr, t_tlsmp *input){

  t_cd2_mother *x = ptr;
  t_tlsmp data[2];
  t_tlsmp ret[2];
  int s = samples * x->h * x->up;
  int i, j;
  
  for(i=0; i<s; i++)
    {

      freq   = 2 * M_PI * ctl_freq->ctl_sig->s_block[i];
      bvel   = ctl_bvel->ctl_sig->s_block[i];
      bforce = ctl_bforce->ctl_sig->s_block[i];
      fric   = ctl_fric->ctl_sig->s_block[i];

      for(j=0; j<2; j++)
	{
	  //printf("%f %f %f\n", data1[0], fb1,
	  /*x0*/
	  data[0] = cd2_child_stage(j, x->cd2_children[0]);
	  /*y0*/
	  data[1] = cd2_child_stage(j, x->cd2_children[1]);
	  
	  x->cd2_children[0]->sides[j] =
	    y_dot(data, freq, fric);

	  x->cd2_children[1]->sides[j] =
	    y_dot(data, freq, fric);	    
	}
      

      
      cd2_child_estimate(x->cd2_children[0]);
      cd2_child_estimate(x->cd2_children[1]);
      
      x->o_sigs[0]->s_block[i] = x->cd2_children[0]->state;
      x->o_sigs[1]->s_block[i] = x->cd2_children[1]->state;
      x->o_sigs[2]->s_block[i] = x->cd2_children[1]->state - bvel;
            
    }

}

void setup_this(void){
  
  int i;
  int h = g_h;
  int up = g_up;

  rk_osc = (t_rk_mother *)rk_mother_init(&rk_osc_func,
					 3,//
					 0,//get rid of this arg
					 h,//step size factor
					 up);//upsampling factor

  cd_osc = (t_cd_mother *)cd_mother_init(&cd_osc_func,
					 3,//
					 0,//get rid of this arg
					 h,//step size factor
					 up);//upsampling factor


  cd2_osc = (t_cd_mother *)cd2_mother_init(&cd2_osc_func,
					   3,//
					   0,//get rid of this arg
					   h,//step size factor
					   up);//upsampling factor

  rk_osc->rk_children[1]->state = -1.0;
  cd_osc->cd_children[1]->state = -1.0;
  cd2_osc->cd_children[0]->state = -1.0;

    

  dac_amp_ctls = (t_lin_ctl **)malloc(sizeof(t_lin_ctl*) * 2);

  for(i=0; i<2; i++)
    {
      dac_amp_ctls[i] = init_lin_ctl(i, CTL_T_LIN, h*up);//0,1
      level_lin_ctl(dac_amp_ctls[i], .7);
    }

  ctl_freq = init_lin_ctl(2, CTL_T_LIN, h*up);
  level_lin_ctl(ctl_freq, freq);
  
  ctl_bvel = init_lin_ctl(3, CTL_T_LIN, h*up);
  level_lin_ctl(ctl_bvel, bvel);
  
  ctl_bforce = init_lin_ctl(4, CTL_T_LIN, h*up);
  level_lin_ctl(ctl_bforce, bforce);
  
  ctl_fric = init_lin_ctl(5, CTL_T_LIN, h*up);
  level_lin_ctl(ctl_fric, fric);
  
  install_obj(&rk_osc->od);
  install_obj(&cd2_osc->od);

  //fp = fopen("cd_dbg", "w");
  
  E_TO_HALF = exp(.5);//macro for optimization

}


void do_kill(void){

  int i;

  kill_rk_mother(rk_osc);
  kill_cd_mother(cd_osc);
  kill_cd2_mother(cd2_osc);
  free(dac_amp_ctls);



}

void dsp_chain(int samples,
	       t_tlsig **adc_sigs,
	       t_tlsig **dac_sigs){
  
  int i, j;
  int s = samples;
  
  rk_osc->dsp_func(s, rk_osc, NULL);
  //cd_osc->dsp_func(s, cd_osc, NULL);
  //cd2_osc->dsp_func(s, cd2_osc, NULL);
    
  //dac_sigs[0] = cd2_osc->o_sigs[0];
  dac_sigs[1] = rk_osc->o_sigs[1];
  

  multiply_sigs(dac_sigs[0], dac_amp_ctls[0]->ctl_sig);
  multiply_sigs(dac_sigs[1], dac_amp_ctls[1]->ctl_sig);

  
}



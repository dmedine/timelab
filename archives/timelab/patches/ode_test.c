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
static t_cd2_mother *cd2_osc;
static t_ie_mother *ie_osc;
static t_sie_mother *sie_osc;
static t_lf_mother *lf_osc;

int g_h = 1;
int g_up = 1;

t_lin_ctl *ctl_freq;

t_lin_ctl **dac_amp_ctls;

t_tlsmp freq = 2.0;


t_tlsmp E_TO_HALF;

/*****************/

void reset_oscs(void);



static t_tlsmp x_dot(t_tlsmp *data, t_tlsmp omega){

  
  t_tlsmp out = data[1];
  //t_tlsmp out = data[1] * omega;
  //printf("x_dot: %f %f\n", data[1], out);
  return out;

}

static t_tlsmp y_dot(t_tlsmp *data, t_tlsmp omega){

  t_tlsmp out; 
  
  out = (-1.0 * omega * omega * data[0]);
  //out = (-1.0 * omega * data[0]);
  //printf("omega %f y_dot: %f %f\n", omega, data[0], out);
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

      for(j=0; j<4; j++)
	{
	  //printf("%f %f %f\n", data1[0], fb1,
	  /*x0*/
	  data[0] = rk_child_stage(j, x->rk_children[0]);
	  /*y0*/
	  data[1] = rk_child_stage(j, x->rk_children[1]);
	  
	  x->rk_children[0]->ks[j+1] =
	    x_dot(data, freq);

	  
	  x->rk_children[1]->ks[j+1] =
	    y_dot(data, freq);	    
	}
      

      
      rk_child_estimate(x->rk_children[0]);
      rk_child_estimate(x->rk_children[1]);
      
      x->o_sigs[0]->s_block[i] = x->rk_children[0]->state;
      x->o_sigs[1]->s_block[i] = x->rk_children[1]->state;

            
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

      //printf("%f %f %f\n", data1[0], fb1,
      /*x0*/
      data[0] = cd_child_stage(0, x->cd_children[0]);
      /*y0*/
      data[1] = cd_child_stage(1, x->cd_children[1]);
      //fprintf(fp, "data: %f %f\n", data[0], data[1]);
      
      x->cd_children[0]->sides[0] =
	x_dot(data, freq);
      
      x->cd_children[1]->sides[1] =
	y_dot(data, freq);
      //fprintf(fp, "sides: %f %f\n", x->cd_children[0]->sides[j], x->cd_children[1]->sides[j]);
      

      
      cd_child_estimate(x->cd_children[0]);
      cd_child_estimate(x->cd_children[1]);
      
      x->o_sigs[0]->s_block[i] = x->cd_children[0]->state;
      x->o_sigs[1]->s_block[i] = x->cd_children[1]->state;
            
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


      for(j=0; j<2; j++)
	{
	  //printf("%f %f %f\n", data1[0], fb1,
	  /*x0*/
	  data[0] = cd2_child_stage(j, x->cd2_children[0]);
	  /*y0*/
	  data[1] = cd2_child_stage(j, x->cd2_children[1]);
	  
	  x->cd2_children[0]->sides[j] =
	    y_dot(data, freq);

	  x->cd2_children[1]->sides[j] =
	    y_dot(data, freq);	    
	}
      

      
      cd2_child_estimate(x->cd2_children[0]);
      cd2_child_estimate(x->cd2_children[1]);
      
      x->o_sigs[0]->s_block[i] = x->cd2_children[0]->state;
      x->o_sigs[1]->s_block[i] = x->cd2_children[1]->state;
            
    }

}

static void ie_osc_func(int samples, void *ptr, t_tlsmp *input){

  t_ie_mother *x = ptr;
  t_tlsmp data[2];
  t_tlsmp ret[2];
  int s = samples * x->h * x->up;
  int i, j;
  int its = 100;  

  for(i=0; i<s; i++)
    {

      freq   = 2 * M_PI * ctl_freq->ctl_sig->s_block[i];

      ie_child_begin(x->ie_children[0]);
      ie_child_begin(x->ie_children[1]);

      //printf("0 diff %f 0 tol %f 1 diff %f 1 tol %f\n", x->ie_children[0]->diff, x->ie_children[0]->tol, x->ie_children[0]->diff, x->ie_children[0]->tol);
      while(its--)
	{
	  data[0] = x->ie_children[0]->y_k01;
	  data[1] = x->ie_children[1]->y_k01;

	  x->ie_children[0]->y_k11 = x->ie_children[0]->y_k + x->h_time * 
	    x_dot(data, freq);

	  ie_child_iterate(x->ie_children[0]);

	  x->ie_children[1]->y_k11 = x->ie_children[1]->y_k + x->h_time * 
	    y_dot(data, freq);

	  ie_child_iterate(x->ie_children[1]);
	  if(x->ie_children[0]->diff<=x->ie_children[0]->tol &&
	     x->ie_children[1]->diff<=x->ie_children[1]->tol) break;

	}
      
      
      x->o_sigs[0]->s_block[i] = x->ie_children[0]->state;
      x->o_sigs[1]->s_block[i] = x->ie_children[1]->state;

            
    }

}

static void sie_osc_func1(int samples, void *ptr, t_tlsmp *input){

  //this sechme uses the automatic functionality of the sei solver

  t_sie_mother *x = ptr;
  t_tlsmp data[2];
  t_tlsmp ret[2];
  int s = samples * x->h * x->up;
  int i, j;
  int its = 100;  

  for(i=0; i<s; i++)
    {

      freq   = 2 * M_PI * ctl_freq->ctl_sig->s_block[i];

      data[0] = x->sie_children[0]->xn;
            
      x->sie_children[0]->vn1 = x->sie_children[0]->vn + x->h_time * 
	y_dot(data, freq);
      
      sie_child_iterate(x->sie_children[0]);
      
      x->o_sigs[0]->s_block[i] = x->sie_children[0]->state;
      
            
    }

}

static void sie_osc_func2(int samples, void *ptr, t_tlsmp *input){

  //this sechme requires the user to setup the functions

  t_sie_mother *x = ptr;
  t_tlsmp data[2];
  t_tlsmp ret[2];
  int s = samples * x->h * x->up;
  int i, j;
  int its = 100;  

  for(i=0; i<s; i++)
    {

      freq   = 2 * M_PI * ctl_freq->ctl_sig->s_block[i];

      /*x*/
      data[0] = x->sie_children[0]->state;
      /*y*/
      data[1] = x->sie_children[1]->state;
      
      x->sie_children[0]->state = x->sie_children[0]->state + x->h * x_dot(data, freq);
      x->sie_children[1]->state = x->sie_children[1]->state + x->h * y_dot(data, freq);

      x->o_sigs[0]->s_block[i] = x->sie_children[0]->state;
      x->o_sigs[1]->s_block[i] = x->sie_children[1]->state;
      
            
    }

}


static void lf_osc_func(int samples, void *ptr, t_tlsmp *input){

  t_lf_mother *x = ptr;
  t_tlsmp data[2];
  t_tlsmp ret[2];
  int s = samples * x->h * x->up;
  int i, j;
  
  for(i=0; i<s; i++)
    {

      freq   = 2 * M_PI * ctl_freq->ctl_sig->s_block[i];

      data[0] = x->lf_children[0]->xi;      
      data[1] = 0.0;
      x->lf_children[0]->ai = y_dot(data, freq);//find ai
      //printf("%f %f\n",x->lf_children[0]->xi, x->lf_children[0]->ai);
      lf_child_estimate_x(x->lf_children[0]);//find xi1
      data[0] = x->lf_children[0]->xi;      
      x->lf_children[0]->ai1 = y_dot(data, freq);//find ai1
      lf_child_estimate_v(x->lf_children[0]);//find vi1

      //printf("%f %f %f %f\n",x->lf_children[0]->xi * freq, x->lf_children[0]->vi, x->lf_children[0]->ai, x->lf_children[0]->ai1);
      x->o_sigs[0]->s_block[i] = x->lf_children[0]->xi * freq;
                 
    }

}


void setup_this(void){
  
  int i;
  int h = g_h;
  int up = g_up;

  rk_osc = (t_rk_mother *)rk_mother_init(&rk_osc_func,
					 2,//
					 0,//get rid of this arg
					 h,//step size factor
					 up);//upsampling factor

  cd_osc = (t_cd_mother *)cd_mother_init(&cd_osc_func,
					 2,//
					 0,//get rid of this arg
					 h,//step size factor
					 up);//upsampling factor


  cd2_osc = (t_cd2_mother *)cd2_mother_init(&cd2_osc_func,
					    2,//
					    0,//get rid of this arg
					    h,//step size factor
					    up);//upsampling factor
  
  ie_osc = (t_ie_mother *)ie_mother_init(&ie_osc_func,
					 2,//
					 0,//get rid of this arg
					 h,//step size factor
					 up);//upsampling factor

  sie_osc = (t_sie_mother *)sie_mother_init(&sie_osc_func2,
					 2,//
					 0,//get rid of this arg
					 h,//step size factor
					 up);//upsampling factor

  ie_osc->tol = .001;
  
  lf_osc = (t_lf_mother *)lf_mother_init(&lf_osc_func,
					 1,//
					 0,//get rid of this arg
					 h,//step size factor
					 up);//upsampling factor


  rk_osc->rk_children[1]->state = -1.0;
  cd_osc->cd_children[1]->state = -1.0;
  cd2_osc->cd2_children[1]->state = -1.0;
  ie_osc->ie_children[1]->state = -1.0;
  sie_osc->sie_children[0]->xn = -1.0;
  sie_osc->sie_children[1]->state = -1.0;
  lf_osc->lf_children[0]->vi = -1.0;

    

  dac_amp_ctls = (t_lin_ctl **)malloc(sizeof(t_lin_ctl*) * 2);

  for(i=0; i<2; i++)
    {
      dac_amp_ctls[i] = init_lin_ctl(i, CTL_T_LIN, h*up);//0,1
      level_lin_ctl(dac_amp_ctls[i], .7);
    }

  ctl_freq = init_lin_ctl(2, CTL_T_LIN, h*up);
  level_lin_ctl(ctl_freq, freq);
  
  
  install_obj(&rk_osc->od);
  install_obj(&cd_osc->od);
  install_obj(&cd2_osc->od);
  install_obj(&ie_osc->od);
  install_obj(&sie_osc->od);
  install_obj(&lf_osc->od);


  //fp = fopen("cd_dbg", "w");

}


void do_kill(void){

  int i;

  kill_rk_mother(rk_osc);
  kill_cd_mother(cd_osc);
  kill_cd2_mother(cd2_osc);
  kill_ie_mother(ie_osc);
  kill_sie_mother(sie_osc);
  kill_lf_mother(lf_osc);
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
  ie_osc->dsp_func(s, ie_osc, NULL);
  sie_osc->dsp_func(s, sie_osc, NULL);
  lf_osc->dsp_func(s, lf_osc, NULL);
    
  //dac_sigs[0] = ie_osc->o_sigs[1];
  //dac_sigs[0] = lf_osc->o_sigs[0];
  //dac_sigs[0] = rk_osc->o_sigs[1];
  //dac_sigs[1] = cd2_osc->o_sigs[1];
  

  multiply_sigs(dac_sigs[0], dac_amp_ctls[0]->ctl_sig);
  multiply_sigs(dac_sigs[1], dac_amp_ctls[1]->ctl_sig);

  
}



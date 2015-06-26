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

t_lin_ctl **ctls;
t_lin_ctl **dac_amp_ctls;
t_lin_ctl **sync_ctls;
t_lin_ctl *reset_ctl;

#define N_CTLS 6
t_tlsmp omega0 = 10.0;
t_tlsmp omega1 = 10.0;
t_tlsmp thresh0 = .7;
t_tlsmp thresh1 = .7;
t_tlsmp sync_g0 = 1.0;
t_tlsmp sync_g1 = 1.0;

int sync0, sync1;

/*****************/

void reset_oscs(void);

void reset_oscs(void){

  //printf("hello, you!\n");
  int i, j;
  for(i=0; i<4; i++)
    {
      rk_osc->rk_children[i]->state = 0.0;
      for(j=0; j<5; j++)
	rk_osc->rk_children[i]->ks[j] = 0.0;
    }
  
  rk_osc->rk_children[0]->state = -1.0;
  rk_osc->rk_children[2]->state = -1.0;
}

static t_tlsmp x_dot(t_tlsmp *data, t_tlsmp omega, int sync_on, t_tlsmp thresh, t_tlsmp g, t_tlsmp *sync_data){

  
  t_tlsmp out;
  if(sync_on == 1 && sync_data[0]>thresh && data[0]!=-1.0)
    out = g*(0.0-data[0]);

  else out = (omega * data[1]);
  return out;

}

static t_tlsmp y_dot(t_tlsmp *data, t_tlsmp omega, int sync_on, t_tlsmp thresh, t_tlsmp g, t_tlsmp *sync_data){

  t_tlsmp out;
  
  if(sync_on == 1 && sync_data[0]>thresh && data[1]!=0.0)
    out = g*(-1.0-data[1]);
  else  out = -1.0*omega * data[0];
  return(out);

}


static void rk_osc_func(int samples, void *ptr, t_tlsmp *input){

  t_rk_mother *x = ptr;
  t_tlsmp data0[2], data1[2];
  t_tlsmp r0, r1, r2;
  int s = samples * x->h * x->up;
  int i, j;
  
  for(i=0; i< s; i++)
    {

      omega0  = ctls[0]->ctl_sig->s_block[i];
      omega1  = ctls[1]->ctl_sig->s_block[i];
      thresh0 = ctls[2]->ctl_sig->s_block[i];
      thresh1 = ctls[3]->ctl_sig->s_block[i];
      sync_g0 = ctls[4]->ctl_sig->s_block[i];
      sync_g1 = ctls[5]->ctl_sig->s_block[i];
   
      sync0 = sync_ctls[0]->toggle_flag;
      sync1 = sync_ctls[1]->toggle_flag;
      

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
	  
	  
	  r0 = sqrt(data0[0]*data0[0] + data0[1]*data0[1]);
	  r1 = sqrt(data1[0]*data1[0] + data1[1]*data1[1]);
	  
	  x->rk_children[0]->ks[j+1] =
	    x_dot(data0, omega0, sync0, thresh0, sync_g0, data1);
	  
	  x->rk_children[1]->ks[j+1] =
	    y_dot(data0, omega0, sync0, thresh0, sync_g0, data1);
	  
	  x->rk_children[2]->ks[j+1] =
	    x_dot(data1, omega1, sync1, thresh1, sync_g1, data0);
	  
	  x->rk_children[3]->ks[j+1] =
	    y_dot(data1, omega1, sync1, thresh1, sync_g1, data0);
	  
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

  ctls = (t_lin_ctl **)malloc(sizeof(t_lin_ctl*) * N_CTLS);
  dac_amp_ctls = (t_lin_ctl **)malloc(sizeof(t_lin_ctl*) * 2);
  sync_ctls = (t_lin_ctl **)malloc(sizeof(t_lin_ctl*) * 2);

  for(i=0; i<2; i++)
    {
      dac_amp_ctls[i] = init_lin_ctl(i, CTL_T_LIN, h*up);//0,1
      level_lin_ctl(dac_amp_ctls[i], .707);
    }

  for(i=0; i<N_CTLS; i++)
    ctls[i] = init_lin_ctl(2+i, CTL_T_LIN, h*up);//3-3+N_CTLS

  level_lin_ctl(ctls[0], omega0);
  level_lin_ctl(ctls[1], omega1);
  level_lin_ctl(ctls[2], thresh0);
  level_lin_ctl(ctls[3], thresh1);
  level_lin_ctl(ctls[4], sync_g0);
  level_lin_ctl(ctls[5], sync_g1);

  
  sync_ctls[0] = init_lin_ctl(2+N_CTLS, CTL_T_TOGGLE, h*up);
  sync_ctls[1] = init_lin_ctl(3+N_CTLS, CTL_T_TOGGLE, h*up);
      
  reset_ctl = init_lin_ctl(4+N_CTLS, CTL_T_BANG, h*up);//2
  reset_ctl->do_bang = reset_oscs;

  install_obj(&rk_osc->od);
  fp1 = fopen("sync_out", "w");
}


void do_kill(void){

  int i;

  kill_rk_mother(rk_osc);

  free(dac_amp_ctls);
  dac_amp_ctls = NULL;

  free(sync_ctls);
  sync_ctls = NULL;

  free(ctls);
  ctls = NULL;
  fclose(fp1);


}

void dsp_chain(int samples,
	       t_tlsig **adc_sigs,
	       t_tlsig **dac_sigs){
  
  int i, j;
  int s = samples;
  
  rk_osc->dsp_func(s, rk_osc, NULL);
  
  dac_sigs[0] = rk_osc->o_sigs[1];
  dac_sigs[1] = rk_osc->o_sigs[3];

  for(i=0;i<s;i++)
    {
      fprintf(fp1,"%f %f\n", rk_osc->o_sigs[0]->s_block[i], rk_osc->o_sigs[2]->s_block[i]);
      //      printf("%f %f\n", rk_osc->o_sigs[1], rk_osc->o_sigs[3]);
    }

  multiply_sigs(dac_sigs[0], dac_amp_ctls[0]->ctl_sig);
  multiply_sigs(dac_sigs[1], dac_amp_ctls[1]->ctl_sig);

  
}



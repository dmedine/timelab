#include "stdio.h"
#include "math.h"
#include "stdlib.h"
#include "a_pa.h"
#include "g_api.h"
#include "ui_main.h"
#include "m_modules.h"
#include "m_ode_prim.h"
#include "limits.h"

FILE *fp;
static t_dac *dac;
static t_rk_mother *rk_osc;
static t_upsample *upsample;
static t_dwnsample *dwnsample;

t_sample old_state1;
t_sample old_state2;
t_sample old_state3;

int g_h  = 1;
int g_up = 1;

t_lin_ctl **ctls;

t_lin_ctl *reset_ctl;
t_lin_ctl **dac_amp_ctls;

#define N_CTLS 18
t_sample omega0 = 10.0;
t_sample omega1 = 10.0;
t_sample omega2 = 10.0;

t_sample alpha0 = 0.0;
t_sample alpha1 = 0.0;
t_sample alpha2 = 0.0;

t_sample fb00   = 0.0;
t_sample fb01   = 0.0;
t_sample fb02   = 0.0;

t_sample fb10   = 0.0;
t_sample fb11   = 0.0;
t_sample fb12   = 0.0;

t_sample fb20   = 0.0;
t_sample fb21   = 0.0;
t_sample fb22   = 0.0;

t_sample mu0    = 0.0;
t_sample mu1    = 0.0;
t_sample mu2    = 0.0;

#define N_MATRIX 9
t_lin_ctl **sync_matrix_ctls;




int sync_matrix[3][3];

/*****************/
void dsp_chain(int samples, void *class_ptr);
void do_kill(void);
void reset_oscs(void);

static t_sample x_dot(t_sample *data, t_sample omega, t_sample alpha, t_sample r, t_sample in){

  
  t_sample out = ((omega + in) *  data[1]) + (data[0]*(alpha * (1.0-r)));
  return out;

}

static t_sample y_dot(t_sample *data, t_sample omega, t_sample alpha, t_sample r, t_sample mu, t_sample in){

  t_sample out;
  
  // * (1.0 - (data[0]*data[0]))
  out = (mu  * (1.0 - (data[0]*data[0])) * data[1]) -
    ((omega + in) * data[0]) +
    (data[1]*(alpha * (1.0-r)));
  return(out);

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
      
      rk_osc->rk_children[0]->state = -1.0;
      break;

    case 1:
      //printf("resetting 2\n");
      for(i=2; i<4; i++)
	{
	  rk_osc->rk_children[i]->state = 0.0;
	  for(j=0; j<5; j++)
	    rk_osc->rk_children[i]->ks[j] = 0.0;
	}
      
      rk_osc->rk_children[2]->state = -1.0;
      break;

    case 2:
      //printf("resetting 3\n");
      for(i=4; i<6; i++)
	{
	  rk_osc->rk_children[i]->state = 0.0;
	  for(j=0; j<5; j++)
	    rk_osc->rk_children[i]->ks[j] = 0.0;
	}
      
      rk_osc->rk_children[4]->state = -1.0;
      break;
    }
}

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

void check_sync_matrix(t_sample *old_s, t_sample *cur_s){

  int i, j, k;

  for(i=0, k=0; i<3; i++)
    for(j=0; j<3; j++,  k=i*3+j)
      if(sync_matrix_ctls[k]->toggle_flag == 1)
	{
	  //printf("osc %d synced by ctl_matrix ref %d\n", i, k);
	  if(old_s[j]<0.0 & cur_s[j]>=0.0)sync_by_matrix(i);
	}

}

static void rk_osc_func(int samples, void *ptr, t_sample *input){

  t_rk_mother *x = ptr;
  t_sample data0[2], data1[2], data2[2], old_states[3], cur_states[3];
  t_sample r0, r1, r2;
  int s = samples * x->h * x->up;
  int i, j;
  
  for(i=0; i< s; i++)
    {

      omega0  = ctls[0]->ctl_sig->s_block[i];
      omega1  = ctls[1]->ctl_sig->s_block[i];
      omega2  = ctls[2]->ctl_sig->s_block[i];

      alpha0  = ctls[3]->ctl_sig->s_block[i];
      alpha1  = ctls[4]->ctl_sig->s_block[i];
      alpha2  = ctls[5]->ctl_sig->s_block[i];

      fb00     = ctls[6]->ctl_sig->s_block[i];
      fb01     = ctls[7]->ctl_sig->s_block[i];
      fb02     = ctls[8]->ctl_sig->s_block[i];

      fb10     = ctls[9]->ctl_sig->s_block[i];
      fb11     = ctls[10]->ctl_sig->s_block[i];
      fb12     = ctls[11]->ctl_sig->s_block[i];

      fb20     = ctls[12]->ctl_sig->s_block[i];
      fb21     = ctls[13]->ctl_sig->s_block[i];
      fb22     = ctls[14]->ctl_sig->s_block[i];

      mu0      = ctls[15]->ctl_sig->s_block[i];
      mu1      = ctls[16]->ctl_sig->s_block[i];
      mu2      = ctls[17]->ctl_sig->s_block[i];

      for(j=0; j<4; j++)
  	{
	  //printf("%f %f %f\n", data1[0], fb1, 	  
  	  /*y0*/
  	  data0[0] = rk_child_stage(j, x->rk_children[0]);
  	  /*y0*/
  	  data0[1] = rk_child_stage(j, x->rk_children[1]);
	  
  	  /*x1*/
  	  data1[0] = rk_child_stage(j, x->rk_children[2]);
  	  /*y1*/
  	  data1[1] = rk_child_stage(j, x->rk_children[3]);

  	  /*x2*/
  	  data2[0] = rk_child_stage(j, x->rk_children[4]);
  	  /*y2*/
  	  data2[1] = rk_child_stage(j, x->rk_children[5]);

	  
	  r0 = sqrt(data0[0]*data0[0] + data0[1]*data0[1]);
	  r1 = sqrt(data1[0]*data1[0] + data1[1]*data1[1]);
	  r2 = sqrt(data2[0]*data2[0] + data2[1]*data2[1]);

  	  x->rk_children[0]->ks[j+1] = 
  	    x_dot(data0, omega0, alpha0, r0, data0[0]*fb00 + data1[0]*fb01 + data2[0]*fb02);
	  
  	  x->rk_children[1]->ks[j+1] = 
  	    y_dot(data0, omega0, alpha0, r0, mu0,
		  data0[0]*fb00 + data1[0]*fb01 + data2[0]*fb02);
	  
  	  x->rk_children[2]->ks[j+1] = 
  	    x_dot(data1, omega1, alpha1, r1, data0[0]*fb10 + data1[0]*fb11 + data2[0]*fb12);
	  
  	  x->rk_children[3]->ks[j+1] = 
  	    y_dot(data1, omega1, alpha1, r1, mu1,
		  data0[0]*fb10 + data1[0]*fb11 + data2[0]*fb12);

  	  x->rk_children[4]->ks[j+1] = 
  	    x_dot(data2, omega2, alpha2, r2, data0[0]*fb20 + data1[0]*fb21 + data2[0]*fb22);
	  
  	  x->rk_children[5]->ks[j+1] = 
  	    y_dot(data2, omega2, alpha2, r2, mu2, 
		  data0[0]*fb20 + data1[0]*fb21 + data2[0]*fb22);
	  
  	}


      
      rk_child_estimate(x->rk_children[0]);
      rk_child_estimate(x->rk_children[1]);
      rk_child_estimate(x->rk_children[2]);
      rk_child_estimate(x->rk_children[3]);
      rk_child_estimate(x->rk_children[4]);
      rk_child_estimate(x->rk_children[5]);

      cur_states[0] = x->rk_children[0]->state;
      cur_states[1] = x->rk_children[2]->state;
      cur_states[2] = x->rk_children[4]->state;

      check_sync_matrix(old_states, cur_states);
      
      old_states[0] = x->o_sigs[0]->s_block[i] = x->rk_children[0]->state;
      x->o_sigs[1]->s_block[i] = x->rk_children[1]->state;
      old_states[1] = x->o_sigs[2]->s_block[i] = x->rk_children[2]->state;
      x->o_sigs[3]->s_block[i] = x->rk_children[3]->state;
      old_states[2] = x->o_sigs[4]->s_block[i] = x->rk_children[4]->state;
      x->o_sigs[5]->s_block[i] = x->rk_children[5]->state;
      
    }

}

static void setup_this(void){
  
  int i;
  int h = g_h;
  int up = g_up;
  init_all();

  dac = (t_dac *)dac_init(g_outchannels);

  rk_osc = (t_rk_mother *)rk_mother_init(&rk_osc_func,
  					 6,//2 children for each of to oscillators
  					 0,//get rid of this arg
  					 h,//step size factor
					 up);//upsampling factor

  rk_osc->rk_children[0]->state = -1.0;
  rk_osc->rk_children[2]->state = -1.0;
  rk_osc->rk_children[4]->state = -1.0;
  
  dwnsample = dwnsample_init(2, h*up);
  dwnsample->i_sigs[0] = rk_osc->o_sigs[1];
  dwnsample->i_sigs[1] = rk_osc->o_sigs[3];

  dac->i_sigs[0] = dwnsample->o_sigs[0];
  dac->i_sigs[1] = dwnsample->o_sigs[1];

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

  level_lin_ctl(ctls[0], omega0);
  level_lin_ctl(ctls[1], omega1);
  level_lin_ctl(ctls[2], omega2);
  level_lin_ctl(ctls[3], alpha0);
  level_lin_ctl(ctls[4], alpha1);
  level_lin_ctl(ctls[5], alpha2);

  level_lin_ctl(ctls[6], fb00);
  level_lin_ctl(ctls[7], fb01);
  level_lin_ctl(ctls[8], fb02);
  level_lin_ctl(ctls[9], fb10);
  level_lin_ctl(ctls[10], fb11);
  level_lin_ctl(ctls[11], fb12);
  level_lin_ctl(ctls[12], fb20);
  level_lin_ctl(ctls[13], fb21);
  level_lin_ctl(ctls[14], fb22);

  level_lin_ctl(ctls[15], mu0);
  level_lin_ctl(ctls[16], mu1);
  level_lin_ctl(ctls[17], mu2);

  for(i=0; i<N_MATRIX; i++)
    {
      printf("installing sync matrix ctl %d\n", 3+N_CTLS+i);//etc
      sync_matrix_ctls[i] = init_lin_ctl(N_CTLS+i, CTL_T_TOGGLE, h*up);
    }

  install_obj(&dac->od);
  install_obj(&rk_osc->od);
  install_obj(&dwnsample->od);

  g_circ_buff = init_circ_buff(10000, 1000);
  g_circ_buff->feeder = g_empty_sig;

  g_dsp_func_reg  = init_dsp_func_reg();
  set_first_dsp_reg_func(&dsp_chain, g_dsp_func_reg);

  g_kill_func_reg = init_kill_func_reg();
  set_first_kill_reg_func(&do_kill, g_kill_func_reg);

  //fp = fopen("fm1_out", "w");

}


void do_kill(void){

  int i;

  kill_dac(dac);
  kill_rk_mother(rk_osc);
  kill_dwnsample(dwnsample);

  free(dac_amp_ctls);
  sync_matrix_ctls = NULL;


  free(sync_matrix_ctls);
  sync_matrix_ctls = NULL;


  free(ctls);
  ctls = NULL;


  kill_circ_buff(g_circ_buff);


}

void dsp_chain(int samples, void *class_ptr){
  
  int i, j;
  int s = samples;

  t_sample *foo;
  rk_osc->dsp_func(s, rk_osc, foo);
  dwnsample->dsp_func(s, dwnsample);
    
 
  if(g_batch_mode==1)
    {
      for(i=0; i<s; i++)
	fprintf(fp, "%f, %f\n", dwnsample->o_sigs[0]->s_block[i], dwnsample->o_sigs[1]->s_block[i]);
    }
  for(i=0; i<2; i++)
    for(j=0; j<s; j++)
      dac->i_sigs[i]->s_block[j] *= dac_amp_ctls[i]->ctl_sig->s_block[j];
  dac->dsp_func(s, dac);
 fill_circ_buff(g_circ_buff, s);
}

int main(int argc, char **argv){

  setup_this();

  g_start_timelab();
  if(g_batch_mode==1)kill_all();

  return(0);
}


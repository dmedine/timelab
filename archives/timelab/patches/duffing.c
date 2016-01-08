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

t_tlsmp old_state1;
t_tlsmp old_state2;
t_tlsmp old_state3;

int g_h  = 1;
int g_up = 1;



t_lin_ctl *reset_ctl;
t_lin_ctl **dac_amp_ctls;

#define N_MATRIX 9
t_lin_ctl **sync_matrix_ctls;
int sync_matrix[3][3];

#define N_CTLS 33
t_lin_ctl **ctls;

t_tlsmp omega0 = 10.0;
t_tlsmp omega1 = 10.0;
t_tlsmp omega2 = 10.0;

t_tlsmp fbfm00   = 0.0;
t_tlsmp fbfm01   = 0.0;
t_tlsmp fbfm02   = 0.0;

t_tlsmp fbfm10   = 0.0;
t_tlsmp fbfm11   = 0.0;
t_tlsmp fbfm12   = 0.0;

t_tlsmp fbfm20   = 0.0;
t_tlsmp fbfm21   = 0.0;
t_tlsmp fbfm22   = 0.0;

t_tlsmp fbg00   = 0.0;
t_tlsmp fbg01   = 0.0;
t_tlsmp fbg02   = 0.0;

t_tlsmp fbg10   = 0.0;
t_tlsmp fbg11   = 0.0;
t_tlsmp fbg12   = 0.0;

t_tlsmp fbg20   = 0.0;
t_tlsmp fbg21   = 0.0;
t_tlsmp fbg22   = 0.0;

t_tlsmp alpha0  = 1.0;
t_tlsmp alpha1  = 1.0;
t_tlsmp alpha2  = 1.0;

t_tlsmp beta0   = 0.0;
t_tlsmp beta1   = 0.0;
t_tlsmp beta2   = 0.0;

t_tlsmp delta0  = 0.0;
t_tlsmp delta1  = 0.0;
t_tlsmp delta2  = 0.0;

t_tlsmp eps0    = 0.0;
t_tlsmp eps1    = 0.0;
t_tlsmp eps2    = 0.0;


/*****************/

void reset_oscs(void);

static t_tlsmp x_dot(t_tlsmp *data, t_tlsmp omega, t_tlsmp r, t_tlsmp eps, t_tlsmp in){

  
  t_tlsmp out = ((omega + in) *  data[1]) + (data[0] * eps * (1.0-r));
  return out;

}

static t_tlsmp y_dot(t_tlsmp *data, t_tlsmp omega, t_tlsmp r, 
		      t_tlsmp alpha, t_tlsmp beta, t_tlsmp delta, t_tlsmp eps,
		      t_tlsmp in1, t_tlsmp in2){

  t_tlsmp out;
  
  // * (1.0 - (data[0]*data[0]))
  out = (omega + in1) * //frequency in
    
    ((beta*data[0]*data[0]*data[0]) - (delta * data[1]) - (alpha * data[0])) + 
    //duffing expression : y_dot = Bx^3 - Dy - Ax

    (data[1] * eps * (1.0 -r)) -//circle force 
    in2;  //driver force
  
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
  for(i=0; i<6; i++)
    {
      rk_osc->rk_children[i]->state = 0.0;
      for(j=0; j<6; j++)
	rk_osc->rk_children[i]->ks[j] = 0.0;
    }
  
  rk_osc->rk_children[0]->state = -1.0;
  rk_osc->rk_children[2]->state = -1.0;
  rk_osc->rk_children[4]->state = -1.0;
}

void check_sync_matrix(t_tlsmp *old_s, t_tlsmp *cur_s){

  int i, j, k;

  for(i=0, k=0; i<3; i++)
    for(j=0; j<3; j++,  k=i*3+j)
      if(sync_matrix_ctls[k]->toggle_flag == 1)
	{
	  //printf("osc %d synced by ctl_matrix ref %d\n", i, k);
	  if(old_s[j]<0.0 & cur_s[j]>=0.0)sync_by_matrix(i);
	}

}

static void rk_osc_func(int samples, void *ptr, t_tlsmp *input){

  t_rk_mother *x = ptr;
  t_tlsmp data0[2], data1[2], data2[2], old_states[3], cur_states[3];
  t_tlsmp r0, r1, r2;
  int s = samples * x->h * x->up;
  int i, j;
  
  for(i=0; i< s; i++)
    {

      omega0  = ctls[0]->ctl_sig->s_block[i];
      omega1  = ctls[1]->ctl_sig->s_block[i];
      omega2  = ctls[2]->ctl_sig->s_block[i];

      fbfm00  = ctls[3]->ctl_sig->s_block[i];
      fbfm01  = ctls[4]->ctl_sig->s_block[i];
      fbfm02  = ctls[5]->ctl_sig->s_block[i];

      fbfm10  = ctls[6]->ctl_sig->s_block[i];
      fbfm11  = ctls[7]->ctl_sig->s_block[i];
      fbfm12  = ctls[8]->ctl_sig->s_block[i];

      fbfm20  = ctls[9]->ctl_sig->s_block[i];
      fbfm21  = ctls[10]->ctl_sig->s_block[i];
      fbfm22  = ctls[11]->ctl_sig->s_block[i];

      fbg00   = ctls[12]->ctl_sig->s_block[i];
      fbg01   = ctls[13]->ctl_sig->s_block[i];
      fbg02   = ctls[14]->ctl_sig->s_block[i];

      fbg10   = ctls[15]->ctl_sig->s_block[i];
      fbg11   = ctls[16]->ctl_sig->s_block[i];
      fbg12   = ctls[17]->ctl_sig->s_block[i];

      fbg20   = ctls[18]->ctl_sig->s_block[i];
      fbg21   = ctls[19]->ctl_sig->s_block[i];
      fbg22   = ctls[20]->ctl_sig->s_block[i];

      alpha0  = ctls[21]->ctl_sig->s_block[i];
      alpha1  = ctls[22]->ctl_sig->s_block[i];
      alpha2  = ctls[23]->ctl_sig->s_block[i];

      beta0  = ctls[24]->ctl_sig->s_block[i];
      beta1  = ctls[25]->ctl_sig->s_block[i];
      beta2  = ctls[26]->ctl_sig->s_block[i];

      delta0  = ctls[27]->ctl_sig->s_block[i];
      delta1  = ctls[28]->ctl_sig->s_block[i];
      delta2  = ctls[29]->ctl_sig->s_block[i];

      eps0    = ctls[30]->ctl_sig->s_block[i];
      eps1    = ctls[31]->ctl_sig->s_block[i];
      eps2    = ctls[32]->ctl_sig->s_block[i];

      //**************************************

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

  	  /*x2*/
  	  data2[0] = rk_child_stage(j, x->rk_children[4]);
  	  /*y2*/
  	  data2[1] = rk_child_stage(j, x->rk_children[5]);

	  
	  r0 = sqrt(data0[0]*data0[0] + data0[1]*data0[1]);
	  r1 = sqrt(data1[0]*data1[0] + data1[1]*data1[1]);
	  r2 = sqrt(data2[0]*data2[0] + data2[1]*data2[1]);

  	  x->rk_children[0]->ks[j+1] = 
  	    x_dot(data0, omega0, r0, eps0, 
		  data0[0]*fbfm00 + data1[0]*fbfm01 + data2[0]*fbfm02);
	  
  	  x->rk_children[1]->ks[j+1] = 
  	    y_dot(data0, omega0, r0,
		  alpha0, beta0, delta0, eps0, 
		  data0[0]*fbfm00 + data1[0]*fbfm01 + data2[0]*fbfm02,//in1
		  data0[0]*fbg00 + data1[0]*fbg01 + data2[0]*fbg02);//in2
	  
  	  x->rk_children[2]->ks[j+1] = 
  	    x_dot(data1, omega1, r1, eps1,
		  data0[0]*fbfm10 + data1[0]*fbfm11 + data2[0]*fbfm12);
	  
  	  x->rk_children[3]->ks[j+1] = 
  	    y_dot(data1, omega1, r1,
		  alpha1, beta1, delta1, eps1,
		  data0[0]*fbfm10 + data1[0]*fbfm11 + data2[0]*fbfm12,//in1
		  data0[0]*fbg10 + data1[0]*fbg11 + data2[0]*fbg12);//in2

  	  x->rk_children[4]->ks[j+1] = 
  	    x_dot(data2, omega2, r2, eps2, 
		  data0[0]*fbfm20 + data1[0]*fbfm21 + data2[0]*fbfm22);
	  
  	  x->rk_children[5]->ks[j+1] = 
  	    y_dot(data2, omega2, r2,
		  alpha2, beta2, delta2, eps2, 
		  data0[0]*fbfm20 + data1[0]*fbfm21 + data2[0]*fbfm22,//in1
		  data0[0]*fbg20 + data1[0]*fbg21 + data2[0]*fbg22);//in2
	  
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

void setup_this(void){
  
  int i;
  int h = g_h;
  int up = g_up;

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

  install_obj(&rk_osc->od);
  install_obj(&dwnsample->od);

  //*******************************
  //controls

  ctls = (t_lin_ctl **)malloc(sizeof(t_lin_ctl*) * N_CTLS);
  dac_amp_ctls = (t_lin_ctl **)malloc(sizeof(t_lin_ctl*) * 2);
  sync_matrix_ctls = (t_lin_ctl **)malloc(sizeof(t_lin_ctl*) * N_MATRIX);

  for(i=0; i<2; i++)
    {
      dac_amp_ctls[i] = init_lin_ctl(i, CTL_T_LIN, h*up);//0,1
      level_lin_ctl(dac_amp_ctls[i], .707);
      printf("installing dac ctls %d\n", i);
    }

  reset_ctl = init_lin_ctl(2, CTL_T_BANG, h*up);//2
  printf("installing reset ctl %d\n", 2);
  reset_ctl->do_bang = reset_oscs;

  for(i=0; i<N_MATRIX; i++)
    sync_matrix_ctls[i] = init_lin_ctl(3+i, CTL_T_TOGGLE, h*up);
    

  for(i=0; i<N_CTLS; i++)
    ctls[i] = init_lin_ctl(3+N_MATRIX+i, CTL_T_LIN, h*up);//3+N_MATRIX - N_CTLS

  level_lin_ctl(ctls[0], omega0);
  level_lin_ctl(ctls[1], omega1);
  level_lin_ctl(ctls[2], omega2);

  level_lin_ctl(ctls[3], fbfm00);
  level_lin_ctl(ctls[4], fbfm01);
  level_lin_ctl(ctls[5], fbfm02);

  level_lin_ctl(ctls[6], fbfm10);
  level_lin_ctl(ctls[7], fbfm11);
  level_lin_ctl(ctls[8], fbfm12);

  level_lin_ctl(ctls[9], fbfm20);
  level_lin_ctl(ctls[10], fbfm21);
  level_lin_ctl(ctls[11], fbfm22);

  level_lin_ctl(ctls[12], fbg00);
  level_lin_ctl(ctls[13], fbg01);
  level_lin_ctl(ctls[14], fbg02);

  level_lin_ctl(ctls[15], fbg10);
  level_lin_ctl(ctls[16], fbg11);
  level_lin_ctl(ctls[17], fbg12);

  level_lin_ctl(ctls[18], fbg20);
  level_lin_ctl(ctls[19], fbg21);
  level_lin_ctl(ctls[20], fbg22);

  level_lin_ctl(ctls[21], alpha0);
  level_lin_ctl(ctls[22], alpha1);
  level_lin_ctl(ctls[23], alpha2);

  level_lin_ctl(ctls[24], beta0);
  level_lin_ctl(ctls[25], beta1);
  level_lin_ctl(ctls[26], beta2);

  level_lin_ctl(ctls[27], delta0);
  level_lin_ctl(ctls[28], delta1);
  level_lin_ctl(ctls[29], delta2);

  level_lin_ctl(ctls[30], eps0);
  level_lin_ctl(ctls[31], eps1);
  level_lin_ctl(ctls[32], eps2);


}


void do_kill(void){

  int i;

  kill_rk_mother(rk_osc);
  kill_dwnsample(dwnsample);

  free(dac_amp_ctls);
  sync_matrix_ctls = NULL;


  free(sync_matrix_ctls);
  sync_matrix_ctls = NULL;

  free(ctls);
  ctls = NULL;

  /* fclose(fp1); */
  /* fclose(fp2); */


}

void dsp_chain(int samples, 
	       t_tlsig **adc_sigs,
	       t_tlsig **dac_sigs){
  
  int i, j;
  int s = samples;

  rk_osc->dsp_func(s, rk_osc, NULL);
  dwnsample->dsp_func(s, dwnsample);
    
  dac_sigs[0] = dwnsample->o_sigs[0];
  dac_sigs[1] = dwnsample->o_sigs[1];


  multiply_sigs(dac_sigs[0], dac_amp_ctls[0]->ctl_sig);
  multiply_sigs(dac_sigs[1], dac_amp_ctls[1]->ctl_sig);



}




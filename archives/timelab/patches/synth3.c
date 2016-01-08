#include "stdio.h"
#include "math.h"
#include "stdlib.h"
//#include "a_pa.h"
#include "g_api.h"
//#include "ui_main.h"
#include "m_modules.h"
#include "m_ode_prim.h"
#include "limits.h"


static t_rk_mother *rk_synth;

static t_upsample *upsample;
static t_dwnsample *dwnsample;

t_tlsmp old_state1;
t_tlsmp old_state2;

int g_h  = 1;
int g_up = 1;

t_lin_ctl **ctls;

t_lin_ctl *reset_ctl;
t_lin_ctl **dac_amp_ctls;
t_lin_ctl **lfo_ctls;

//--lfos
#define N_LFO_CTLS 24
t_tlsmp omega0 = 0.0;
t_tlsmp omega1 = 0.0;
t_tlsmp omega2 = 0.0;
t_tlsmp omega3 = 0.0;

t_tlsmp fb00 = 0.0;
t_tlsmp fb01 = 0.0;
t_tlsmp fb02 = 0.0;
t_tlsmp fb03 = 0.0;

t_tlsmp fb10 = 0.0;
t_tlsmp fb11 = 0.0;
t_tlsmp fb12 = 0.0;
t_tlsmp fb13 = 0.0;

t_tlsmp fb20 = 0.0;
t_tlsmp fb21 = 0.0;
t_tlsmp fb22 = 0.0;
t_tlsmp fb23 = 0.0;

t_tlsmp fb30 = 0.0;
t_tlsmp fb31 = 0.0;
t_tlsmp fb32 = 0.0;
t_tlsmp fb33 = 0.0;

t_tlsmp g0 = 0.0;
t_tlsmp g1 = 0.0;
t_tlsmp g2 = 0.0;
t_tlsmp g3 = 0.0;


//--oscillators
#define N_CTLS 12

t_tlsmp sqr_flyback0 = 72000.0;
t_tlsmp sqr_flyback1 = 72000.0;

t_tlsmp sqr_curl0 = 500.0;
t_tlsmp sqr_curl1 = 500.0;

t_tlsmp tri_curl0 = 500.0;
t_tlsmp tri_curl1 = 500.0;

t_tlsmp tri_thresh0 = .9;
t_tlsmp tri_thresh1 = .9;

t_tlsmp skew0 = .5;
t_tlsmp skew1 = .5;

t_tlsmp freq0 = 400;
t_tlsmp freq1 = 500;

#define N_MATRIX 16
t_lin_ctl **sync_matrix_ctls;

int sync_matrix[4][4];

//---------------------------
t_lin_ctl *c_cutoff0, *c_res0;
t_lin_ctl *c_cutoff1, *c_res1;
t_lin_ctl *c_sqr_in0, *c_sqr_in1, *c_tri_in0, *c_tri_in1;

#define CUTOFF0     c_cutoff0->ctl_sig->s_block
#define RES0        c_res0->ctl_sig->s_block
#define CUTOFF1     c_cutoff1->ctl_sig->s_block
#define RES1        c_res1->ctl_sig->s_block
#define SQR_IN0     c_sqr_in0->ctl_sig->s_block
#define SQR_IN1     c_sqr_in1->ctl_sig->s_block
#define TRI_IN0     c_tri_in0->ctl_sig->s_block
#define TRI_IN1     c_tri_in1->ctl_sig->s_block


/*****************/


#define N_PATCH_SIGS 14
t_tlsig **patch_sigs;


//--defines for rk_synth inlets
#define OSC0_FREQ_IN patch_sigs[0]->s_block[i] *.5 + .5
#define OSC1_FREQ_IN patch_sigs[1]->s_block[i] *.5 + .5

#define LFO0_FREQ_IN patch_sigs[2]->s_block[i] *.5 + .5
#define LFO1_FREQ_IN patch_sigs[3]->s_block[i] *.5 + .5
#define LFO2_FREQ_IN patch_sigs[4]->s_block[i] *.5 + .5
#define LFO3_FREQ_IN patch_sigs[5]->s_block[i] *.5 + .5

#define LFO0_DEPTH_IN patch_sigs[6]->s_block[i] *.5 + .5
#define LFO1_DEPTH_IN patch_sigs[7]->s_block[i] *.5 + .5
#define LFO2_DEPTH_IN patch_sigs[8]->s_block[i] *.5 + .5
#define LFO3_DEPTH_IN patch_sigs[9]->s_block[i] *.5 + .5

#define MOOG0_FREQ_IN patch_sigs[10]->s_block[i] *.5 + .5
#define MOOG1_FREQ_IN patch_sigs[11]->s_block[i] *.5 + .5

#define MOOG0_RES_IN patch_sigs[12]->s_block[i] *.5 + .5
#define MOOG1_RES_IN patch_sigs[13]->s_block[i] *.5 + .5

#define N_GAINS 8
t_lin_ctl **gains_matrix_ctls;
#define N_GAINS_MATRIX N_PATCH_SIGS * N_GAINS

t_tlsig *tmp_sig;


void reset_lfos(void){
  int i, j;
  for(i=12; i<20; i++)
    {
      rk_synth->rk_children[i]->state = 0.0;
      for(j=0; j<5; j++)
	rk_synth->rk_children[i]->ks[j] = 0.0;
    }
  rk_synth->rk_children[13]->state = 1.0;
  rk_synth->rk_children[15]->state = 1.0;
  rk_synth->rk_children[17]->state = 1.0;
  rk_synth->rk_children[19]->state = 1.0;



}

void reset_moog(void){
int i, j;

  for(i=4; i<12; i++)
    {
      rk_synth->rk_children[i]->state = 0.0;
      for(j=0; j<5; j++)
	rk_synth->rk_children[i]->ks[j] = 0.0;
    }
}

void reset(void){

  int i, j;

  for(i=0; i<4; i++)
    {
      rk_synth->rk_children[i]->state = 0.0;
      for(j=0; j<5; j++)
	rk_synth->rk_children[i]->ks[j] = 0.0;
    }

  rk_synth->rk_children[1]->state = 1.0;
  rk_synth->rk_children[3]->state = 1.0;

  reset_moog();
  reset_lfos();

}


//TODO: make these into a single function
void sync_by_matrix(int osc_no){
  
  int i, j;
  switch(osc_no)
    {

    case 0:
      //printf("resetting 1\n");
      for(i=12; i<14; i++)
	{
	  rk_synth->rk_children[i]->state = 0.0;
	  for(j=0; j<5; j++)
	    rk_synth->rk_children[i]->ks[j] = 0.0;
	}
            
      rk_synth->rk_children[13]->state = 1.0;
      break;

    case 1:
      //printf("resetting 2\n");
      for(i=14; i<16; i++)
	{
	  rk_synth->rk_children[i]->state = 0.0;
	  for(j=0; j<5; j++)
	    rk_synth->rk_children[i]->ks[j] = 0.0;
	}
      
      rk_synth->rk_children[15]->state = 1.0;
      break;

   case 2:
      //printf("resetting 2\n");
      for(i=16; i<18; i++)
	{
	  rk_synth->rk_children[i]->state = 0.0;
	  for(j=0; j<5; j++)
	    rk_synth->rk_children[i]->ks[j] = 0.0;
	}
      
      rk_synth->rk_children[17]->state = 1.0;
      break;

   case 3:
      //printf("resetting 2\n");
      for(i=18; i<20; i++)
	{
	  rk_synth->rk_children[i]->state = 0.0;
	  for(j=0; j<5; j++)
	    rk_synth->rk_children[i]->ks[j] = 0.0;
	}
      
      rk_synth->rk_children[19]->state = 1.0;
      break;



    }
}



void check_sync_matrix(t_tlsmp *old_s, t_tlsmp *cur_s){

  int i, j, k;

  for(i=0, k=0; i<4; i++)
    for(j=0; j<4; j++,  k=i*4+j)
      if(sync_matrix_ctls[k]->toggle_flag == 1)
	{
	  //printf("osc %d synced by ctl_matrix ref %d\n", i, k);
	  if(old_s[j]<0.0 & cur_s[j]>=0.0)sync_by_matrix(i);
	}

}

static t_tlsmp lfo_x_dot(t_tlsmp *data, t_tlsmp omega, t_tlsmp r, t_tlsmp in){

  
  t_tlsmp out = ((omega + in) * data[1]) + (data[0]*(90000.0 * (1.0-r)));
  return out;

}

static t_tlsmp lfo_y_dot(t_tlsmp *data, t_tlsmp omega,t_tlsmp r, t_tlsmp in){

  t_tlsmp out;
  
  // * (1.0 - (data[0]*data[0]))
  out = -1.0 * ((omega + in) * data[0]) + (data[1]*(20000.0 * (1.0-r)));
  return(out);

}



//--oscillators

static void find_gains(t_tlsmp skew, t_tlsmp freq, t_tlsmp *gainup, t_tlsmp *gaindn){
  
  t_tlsmp per;
  per = 1.0/freq;
  *gainup = 2.0/(skew*per);
  *gaindn = -2.0/((1.0-skew) *per);
  //printf("%f %f %f %f\n", per, skew, *gainup, *gaindn);
}

static t_tlsmp tanh_clip(t_tlsmp x, t_tlsmp gain){

  t_tlsmp out = tanh(x * gain);
  return(out);
}

static t_tlsmp osc_x_dot(t_tlsmp *data, t_tlsmp gainup, t_tlsmp gaindn, t_tlsmp curl){//x: triangle

  t_tlsmp out;
  if(data[1]>=0)
    out = gainup * tanh_clip(data[1], curl);
  else
    out = gaindn * -1.0 * tanh_clip(data[1], curl);
  return out;

}

static t_tlsmp osc_y_dot(t_tlsmp *data, t_tlsmp thresh, t_tlsmp flyback){//y: square

  t_tlsmp out = 0.0;

  if(data[0]>thresh)out = -1.0*flyback;
  if(data[0]<(-1.0*thresh))out = flyback;
  return(out);

}

//--moog filter

static t_tlsmp diff(t_tlsmp alpha, t_tlsmp *data){

  t_tlsmp out;
  out = alpha * tanh((data[0] - data[1]));
  return out;

}

static void rk_synth_func(int samples, void *ptr, t_tlsmp *input){

  t_rk_mother *x = ptr;
  int s = samples * x->h * x->up;
  int i, j, k;

  t_tlsmp lfo_data0[2], lfo_data1[2], lfo_data2[2], lfo_data3[2];
  t_tlsmp r0, r1, r2, r3;
  t_tlsmp lfo_old_states[4], lfo_cur_states[4];

  t_tlsmp osc_data0[2], osc_data1[2], m_data0[2], m_data1[2];
  t_tlsmp gainup0, gainup1, gaindn0, gaindn1;

  t_tlsmp alpha0, alpha1, res0, res1;
  t_tlsmp tri_in0, tri_in1, sqr_in0, sqr_in1;
  t_tlsmp foo;
  
  for(i=0; i< s; i++)
    {
      /* printf("ofreq0 %f\t", OSC0_FREQ_IN); */
      /* printf("ofreq1 %f\n", OSC1_FREQ_IN); */
      //--lfos

      omega0 = lfo_ctls[0]->c_sig[i] + LFO0_FREQ_IN;
      omega1 = lfo_ctls[1]->c_sig[i] + LFO1_FREQ_IN;
      omega2 = lfo_ctls[2]->c_sig[i] + LFO2_FREQ_IN;
      omega3 = lfo_ctls[3]->c_sig[i] + LFO3_FREQ_IN;
      
      fb00 = lfo_ctls[4]->c_sig[i];
      fb01 = lfo_ctls[5]->c_sig[i];
      fb02 = lfo_ctls[6]->c_sig[i];
      fb03 = lfo_ctls[7]->c_sig[i];

      fb10 = lfo_ctls[8]->c_sig[i];
      fb11 = lfo_ctls[9]->c_sig[i];
      fb12 = lfo_ctls[10]->c_sig[i];
      fb13 = lfo_ctls[11]->c_sig[i];

      fb20 = lfo_ctls[12]->c_sig[i];
      fb21 = lfo_ctls[13]->c_sig[i];
      fb22 = lfo_ctls[14]->c_sig[i];
      fb23 = lfo_ctls[15]->c_sig[i];

      fb30 = lfo_ctls[16]->c_sig[i];
      fb31 = lfo_ctls[17]->c_sig[i];
      fb32 = lfo_ctls[18]->c_sig[i];
      fb33 = lfo_ctls[19]->c_sig[i];

      g0 = lfo_ctls[20]->c_sig[i] + LFO0_DEPTH_IN;
      g1 = lfo_ctls[21]->c_sig[i] + LFO1_DEPTH_IN;
      g2 = lfo_ctls[22]->c_sig[i] + LFO2_DEPTH_IN;
      g3 = lfo_ctls[23]->c_sig[i] + LFO3_DEPTH_IN;


      //--tri-square
      sqr_flyback0 = ctls[0]->c_sig[i];
      sqr_flyback1 = ctls[1]->c_sig[i];
      sqr_curl0    = ctls[2]->c_sig[i];
      sqr_curl1    = ctls[3]->c_sig[i];
      tri_curl0    = ctls[4]->c_sig[i];
      tri_curl1    = ctls[5]->c_sig[i];
      tri_thresh0  = ctls[6]->c_sig[i];
      tri_thresh1  = ctls[7]->c_sig[i];
      skew0        = ctls[8]->c_sig[i];
      skew1        = ctls[9]->c_sig[i];
      freq0        = ctls[10]->c_sig[i] + OSC0_FREQ_IN;
      freq1        = ctls[11]->c_sig[i] + OSC1_FREQ_IN;


      //--moog
	  
      alpha0    = CUTOFF0[i] * M_PI + MOOG0_FREQ_IN;
      res0      = RES0[i] + MOOG0_RES_IN;
      alpha1    = CUTOFF1[i] * M_PI + MOOG1_FREQ_IN;
      res1      = RES1[i] + MOOG1_RES_IN;
      sqr_in0   = SQR_IN0[i];
      sqr_in1   = SQR_IN1[i];
      tri_in0   = TRI_IN0[i];
      tri_in1   = TRI_IN1[i];


      for(j=0; j<4; j++)
  	{
	  //--lfos

	  /*y0*/
	  lfo_data0[0] = rk_child_stage(j, x->rk_children[12]);
	  /*y0*/
	  lfo_data0[1] = rk_child_stage(j, x->rk_children[13]);
	  
	  /*x1*/
	  lfo_data1[0] = rk_child_stage(j, x->rk_children[14]);
	  /*y1*/
	  lfo_data1[1] = rk_child_stage(j, x->rk_children[15]);
	  
	  /*x2*/
	  lfo_data2[0] = rk_child_stage(j, x->rk_children[16]);
	  /*y2*/
	  lfo_data2[1] = rk_child_stage(j, x->rk_children[17]);

	  /*x2*/
	  lfo_data3[0] = rk_child_stage(j, x->rk_children[18]);
	  /*y2*/
	  lfo_data3[1] = rk_child_stage(j, x->rk_children[19]);
	  
	  
	  r0 = sqrt(lfo_data0[0]*lfo_data0[0] + lfo_data0[1]*lfo_data0[1]);
	  r1 = sqrt(lfo_data1[0]*lfo_data1[0] + lfo_data1[1]*lfo_data1[1]);
	  r2 = sqrt(lfo_data2[0]*lfo_data2[0] + lfo_data2[1]*lfo_data2[1]);
	  r3 = sqrt(lfo_data2[0]*lfo_data2[0] + lfo_data2[1]*lfo_data2[1]);
	  
	  x->rk_children[12]->ks[j+1] =
	    lfo_x_dot(lfo_data0, omega0, r0, 
		  lfo_data0[0]*fb00 + lfo_data1[0]*fb01 + lfo_data2[0]*fb02 + lfo_data3[0]*fb03);
	  
	  x->rk_children[13]->ks[j+1] =
	    lfo_y_dot(lfo_data0, omega0, r0, 
		  lfo_data0[0]*fb00 + lfo_data1[0]*fb01 + lfo_data2[0]*fb02 + lfo_data3[0]*fb03);
	  
	  x->rk_children[14]->ks[j+1] =
	    lfo_x_dot(lfo_data1, omega1, r1, 
		  lfo_data0[0]*fb10 + lfo_data1[0]*fb11 + lfo_data2[0]*fb12 + lfo_data3[0]*fb13);
	  
	  x->rk_children[15]->ks[j+1] =
	    lfo_y_dot(lfo_data1, omega1, r1,
		  lfo_data0[0]*fb10 + lfo_data1[0]*fb11 + lfo_data2[0]*fb12 + lfo_data3[0]*fb13);
	  
	  x->rk_children[16]->ks[j+1] =
	    lfo_x_dot(lfo_data2, omega2, r2, 
		  lfo_data0[0]*fb20 + lfo_data1[0]*fb21 + lfo_data2[0]*fb22 + lfo_data3[0]*fb23);
	  
	  x->rk_children[17]->ks[j+1] =
	    lfo_y_dot(lfo_data2, omega2,  r2,
		  lfo_data0[0]*fb20 + lfo_data1[0]*fb21 + lfo_data2[0]*fb22 + lfo_data3[0]*fb23);

	  x->rk_children[18]->ks[j+1] =
	    lfo_x_dot(lfo_data3, omega3, r3, 
		  lfo_data0[0]*fb30 + lfo_data1[0]*fb31 + lfo_data2[0]*fb32 + lfo_data3[0]*fb33);
	  
	  x->rk_children[19]->ks[j+1] =
	    lfo_y_dot(lfo_data3, omega3, r3,
		  lfo_data0[0]*fb30 + lfo_data1[0]*fb31 + lfo_data2[0]*fb32 + lfo_data3[0]*fb33);
	  
	

  	  
	  //--oscillators

	  find_gains(skew0, freq0, &gainup0, &gaindn0);
	  find_gains(skew1, freq1, &gainup1, &gaindn1);
	  /* find_gains(skew0, freq0 +  */
	  /* 	     (x->rk_children[12]->state + 1.0)*g0*.5,  */
	  /* 	     &gainup0,  */
	  /* 	     &gaindn0); */
	  /* find_gains(skew1, freq1 +  */
	  /* 	     (x->rk_children[14]->state + 1.0)*g1*.5,  */
	  /* 	     &gainup1,  */
	  /* 	     &gaindn1); */
	  //x0
  	  osc_data0[0] = rk_child_stage(j, x->rk_children[0]);
	  //y0
  	  osc_data0[1] = rk_child_stage(j, x->rk_children[1]);
	  
	  // x1
  	  osc_data1[0] = rk_child_stage(j, x->rk_children[2]);
	  //y1
  	  osc_data1[1] = rk_child_stage(j, x->rk_children[3]);

  	  x->rk_children[0]->ks[j+1] =
  	    osc_x_dot(osc_data0, gainup0, gaindn0, tri_curl0);
	  
  	  x->rk_children[1]->ks[j+1] =
  	    osc_y_dot(osc_data0, tri_thresh0, sqr_flyback0);
	  
  	  x->rk_children[2]->ks[j+1] =
  	    osc_x_dot(osc_data1, gainup1, gaindn1, tri_curl1);
	  
  	  x->rk_children[3]->ks[j+1] =
  	    osc_y_dot(osc_data1, tri_thresh1, sqr_flyback1);
	  

	  //--moog
	  
	  //now the filter0
	  //stage 1 is unique
	  m_data0[0] = sqr_in0 *rk_child_stage(j+1, x->rk_children[1]) + 
	    tri_in0 * rk_child_stage(j+1, x->rk_children[0]) -
	    //res0*
	    (res0 + (x->rk_children[16]->state+1.0) * g2 * .5) * 
	    rk_child_stage(j, x->rk_children[7]);
      	  
	  m_data0[1] = rk_child_stage(j, x->rk_children[4]);
      
	  x->rk_children[4]->ks[j+1] =
	    diff(alpha0, m_data0);
	  
	  //the other three stages are the same
	  //input to that stage (previous stage) and current state at that stage
	  for(k=5; k<8; k++)
	    {
	      m_data0[0] = rk_child_stage(j, x->rk_children[k-1]);
	      m_data0[1] = rk_child_stage(j, x->rk_children[k]);
	      x->rk_children[k]->ks[j+1] =
		diff(alpha0, m_data0);
	    }
	  
	  //now the filter1
	  //stage 1 is unique
	  m_data1[0] = sqr_in1 * rk_child_stage(j+1, x->rk_children[3]) + 
	    tri_in1 * rk_child_stage(j+1, x->rk_children[2])-
	    (res1 + (x->rk_children[18]->state+1.0) * .5 * g3) * 
	    //res1*
	    rk_child_stage(j, x->rk_children[11]);
	  
	  m_data1[1] = rk_child_stage(j, x->rk_children[8]);
      
      
	  x->rk_children[8]->ks[j+1] = 
	  diff(alpha1, m_data1);
	  
      
	  for(k=9; k<12; k++)
	    {
	      m_data1[0] = rk_child_stage(j, x->rk_children[k-1]);
	      m_data1[1] = rk_child_stage(j, x->rk_children[k]);
	      x->rk_children[k]->ks[j+1] =
		diff(alpha1, m_data1);
	    }
	  
	}
      

      //--lfos
      rk_child_estimate(x->rk_children[12]);
      rk_child_estimate(x->rk_children[13]);
      rk_child_estimate(x->rk_children[14]);
      rk_child_estimate(x->rk_children[15]);
      rk_child_estimate(x->rk_children[16]);
      rk_child_estimate(x->rk_children[17]);
      rk_child_estimate(x->rk_children[18]);
      rk_child_estimate(x->rk_children[19]);

      lfo_cur_states[0] = x->rk_children[12]->state;
      lfo_cur_states[1] = x->rk_children[14]->state;
      lfo_cur_states[2] = x->rk_children[16]->state;
      lfo_cur_states[3] = x->rk_children[18]->state;

      check_sync_matrix(lfo_old_states, lfo_cur_states);
      
      lfo_old_states[0] = x->o_sigs[12]->s_block[i] = x->rk_children[12]->state;
      x->o_sigs[13]->s_block[i] = tanh_clip(x->rk_children[1]->state, 1.0);
      lfo_old_states[1] = x->o_sigs[14]->s_block[i] = tanh_clip(x->rk_children[14]->state, 1.0);
      x->o_sigs[15]->s_block[i] = tanh_clip(x->rk_children[3]->state, 1.0);
      lfo_old_states[2] = x->o_sigs[16]->s_block[i] = tanh_clip(x->rk_children[16]->state, 1.0);
      x->o_sigs[17]->s_block[i] = tanh_clip(x->rk_children[17]->state, 1.0);
      lfo_old_states[2] = x->o_sigs[18]->s_block[i] = tanh_clip(x->rk_children[18]->state, 1.0);
      x->o_sigs[19]->s_block[i] = tanh_clip(x->rk_children[19]->state, 1.0);


      //--oscillators
      rk_child_estimate(x->rk_children[0]);
      rk_child_estimate(x->rk_children[1]);
      rk_child_estimate(x->rk_children[2]);
      rk_child_estimate(x->rk_children[3]);
      
      x->o_sigs[0]->s_block[i] =
  	tanh_clip(x->rk_children[0]->state, 1.0);
      x->o_sigs[1]->s_block[i] =
  	tanh_clip(x->rk_children[1]->state, sqr_curl0);
      
      x->o_sigs[2]->s_block[i] =
  	tanh_clip(x->rk_children[2]->state, 1.0);
      x->o_sigs[3]->s_block[i] =
  	tanh_clip(x->rk_children[3]->state, sqr_curl1);

      //--moog
      //estimate
      rk_child_estimate(x->rk_children[4]);
      rk_child_estimate(x->rk_children[5]);
      rk_child_estimate(x->rk_children[6]);
      rk_child_estimate(x->rk_children[7]);
      
      x->o_sig4[i] = tanh_clip(x->rk_children[4]->state, 1.0);
      x->o_sig5[i] = tanh_clip(x->rk_children[5]->state, 1.0);
      x->o_sig6[i] = tanh_clip(x->rk_children[6]->state, 1.0);
      x->o_sig7[i] = tanh_clip(x->rk_children[7]->state, 1.0);
       
      rk_child_estimate(x->rk_children[8]);
      rk_child_estimate(x->rk_children[9]);
      rk_child_estimate(x->rk_children[10]);
      rk_child_estimate(x->rk_children[11]);

      x->o_sig8[i] = tanh_clip(x->rk_children[8]->state, 1.0);
      x->o_sig9[i] = tanh_clip(x->rk_children[9]->state, 1.0);
      x->o_sig10[i] = tanh_clip(x->rk_children[10]->state, 1.0);
      x->o_sig11[i] = tanh_clip(x->rk_children[11]->state, 1.0);


    }

}

//---------------------------------------------



void setup_this(void){
  
  int i;
  int h = g_h;
  int up = g_up;

  rk_synth = (t_rk_mother *)rk_mother_init(&rk_synth_func,
					   20,//2 children for each of to oscillators
					   0,//get rid of this arg
					   h,//step size factor
					   up);//upsampling factor


  rk_synth->rk_children[1]->state = 1.0;
  rk_synth->rk_children[3]->state = 1.0;

  rk_synth->rk_children[13]->state = 1.0;
  rk_synth->rk_children[15]->state = 1.0;
  rk_synth->rk_children[17]->state = 1.0;
  rk_synth->rk_children[19]->state = 1.0;


  dwnsample = dwnsample_init(2, h*up);
  dwnsample->i_sigs[0] = rk_synth->o_sigs[4];
  dwnsample->i_sigs[1] = rk_synth->o_sigs[8];

  patch_sigs = init_tlsigs(N_PATCH_SIGS, O_TYPE, h*up);
  tmp_sig = init_one_tlsig(O_TYPE, h*up);

  ctls = (t_lin_ctl **)malloc(sizeof(t_lin_ctl*) * N_CTLS);
  lfo_ctls = (t_lin_ctl **)malloc(sizeof(t_lin_ctl*) * N_LFO_CTLS);
  dac_amp_ctls = (t_lin_ctl **)malloc(sizeof(t_lin_ctl*) * 2);
  sync_matrix_ctls = (t_lin_ctl **)malloc(sizeof(t_lin_ctl*) * N_MATRIX);
  gains_matrix_ctls = (t_lin_ctl **)malloc(sizeof(t_lin_ctl*) * N_GAINS_MATRIX);

  for(i=0; i<2; i++)
    {
      dac_amp_ctls[i] = init_lin_ctl(i, CTL_T_LIN, h*up);//0,1
      level_lin_ctl(dac_amp_ctls[i], .707);
      printf("installing dac ctls %d\n", N_CTLS+N_MATRIX+i);
    }

  reset_ctl = init_lin_ctl(2, CTL_T_BANG, h*up);//2
  printf("installing reset ctl %d\n", N_CTLS+N_MATRIX+i);
  reset_ctl->do_bang = reset;

  for(i=0; i<N_MATRIX; i++)
    {
      printf("installing sync matrix ctl %d\n", 3+i);//etc
      sync_matrix_ctls[i] = init_lin_ctl(3+i, CTL_T_TOGGLE, h*up);
    }

  for(i=0; i<N_CTLS; i++)
    ctls[i] = init_lin_ctl(3+N_MATRIX+i, CTL_T_LIN, h*up);//3-3+N_CTLS+N_MATRIX
  
  level_lin_ctl(ctls[0], sqr_flyback0);
  level_lin_ctl(ctls[1], sqr_flyback1);
  level_lin_ctl(ctls[2], sqr_curl0);
  level_lin_ctl(ctls[3], sqr_curl1);
  level_lin_ctl(ctls[4], tri_curl0);
  level_lin_ctl(ctls[5], tri_curl1);
  level_lin_ctl(ctls[6], tri_thresh0);
  level_lin_ctl(ctls[7], tri_thresh1);

  level_lin_ctl(ctls[8], skew0);
  level_lin_ctl(ctls[9], skew1);
  level_lin_ctl(ctls[10], freq0);
  level_lin_ctl(ctls[11], freq1);

  c_cutoff0 = init_lin_ctl(3+N_CTLS+N_MATRIX, CTL_T_LIN, h*up);
  c_res0    = init_lin_ctl(4+N_CTLS+N_MATRIX, CTL_T_LIN, h*up);
  c_cutoff1 = init_lin_ctl(5+N_CTLS+N_MATRIX, CTL_T_LIN, h*up);
  c_res1    = init_lin_ctl(6+N_CTLS+N_MATRIX, CTL_T_LIN, h*up);
  c_sqr_in0 = init_lin_ctl(7+N_CTLS+N_MATRIX, CTL_T_LIN, h*up);
  c_sqr_in1 = init_lin_ctl(8+N_CTLS+N_MATRIX, CTL_T_LIN, h*up);
  c_tri_in0 = init_lin_ctl(9+N_CTLS+N_MATRIX, CTL_T_LIN, h*up);
  c_tri_in1 = init_lin_ctl(10+N_CTLS+N_MATRIX, CTL_T_LIN, h*up);

  for(i=0; i<N_LFO_CTLS; i++)
    lfo_ctls[i] = init_lin_ctl(10+N_CTLS+N_MATRIX+i, CTL_T_LIN, h*up);//10+N_CTLS_M_MATIRX-all that + N_LFO_CTLS

  level_lin_ctl(lfo_ctls[0], omega0);
  level_lin_ctl(lfo_ctls[1], omega1);
  level_lin_ctl(lfo_ctls[2], omega2);
  level_lin_ctl(lfo_ctls[3], omega3);

  level_lin_ctl(lfo_ctls[4], fb00);
  level_lin_ctl(lfo_ctls[5], fb01);
  level_lin_ctl(lfo_ctls[6], fb02);
  level_lin_ctl(lfo_ctls[7], fb03);

  level_lin_ctl(lfo_ctls[8], fb10);
  level_lin_ctl(lfo_ctls[9], fb11);
  level_lin_ctl(lfo_ctls[10], fb12);
  level_lin_ctl(lfo_ctls[11], fb13);

  level_lin_ctl(lfo_ctls[12], fb20);
  level_lin_ctl(lfo_ctls[13], fb21);
  level_lin_ctl(lfo_ctls[14], fb22);
  level_lin_ctl(lfo_ctls[15], fb23);

  level_lin_ctl(lfo_ctls[16], fb30);
  level_lin_ctl(lfo_ctls[17], fb31);
  level_lin_ctl(lfo_ctls[18], fb32);
  level_lin_ctl(lfo_ctls[19], fb33);

  level_lin_ctl(lfo_ctls[20], g0);
  level_lin_ctl(lfo_ctls[21], g1);
  level_lin_ctl(lfo_ctls[22], g2);
  level_lin_ctl(lfo_ctls[23], g3);

  for(i=0; i<N_GAINS_MATRIX; i++)
    gains_matrix_ctls[i] = init_lin_ctl(10+N_CTLS+N_MATRIX+N_LFO_CTLS+i, CTL_T_LIN, h*up);


  install_obj(&rk_synth->od);
  install_obj(&dwnsample->od);


}


void do_kill(void){

  kill_rk_mother(rk_synth);
  kill_dwnsample(dwnsample);
  kill_tlsigs(patch_sigs, N_PATCH_SIGS);
  kill_one_tlsig(tmp_sig);

  free(dac_amp_ctls);
  dac_amp_ctls = NULL;

  free(sync_matrix_ctls);
  sync_matrix_ctls = NULL;

  free(ctls);
  ctls = NULL;

  free(lfo_ctls);
  lfo_ctls = NULL;


}

void dsp_chain(int samples, 
	       t_tlsig **adc_sigs,
	       t_tlsig **dac_sigs){
  
  int i, j;
  int s = samples;

  rk_synth->dsp_func(s, rk_synth, NULL);  
  //printf("ctl_test %f\n",  gains_matrix_ctls[0]->c_sig[0]);


  for(i=0; i<N_PATCH_SIGS; i++)
    {
      zero_out_tlsig(patch_sigs[i]);
      for(j=0; j<N_GAINS-4; j++)
	{
	  cpy_tlsigs(tmp_sig, rk_synth->o_sigs[j*2 + 12]);//every other lfo out (12, 14, 16, 18)
	  multiply_tlsigs(tmp_sig, gains_matrix_ctls[i*N_GAINS+j]->ctl_sig);
	  add_tlsigs(patch_sigs[i], tmp_sig);
	}

      for(j=N_GAINS-4; j<N_GAINS; j++)
	{
	  cpy_tlsigs(tmp_sig, rk_synth->o_sigs[j-4]);//offset to osc outs (0-11)
	  multiply_tlsigs(tmp_sig, gains_matrix_ctls[i*N_GAINS+j]->ctl_sig);
	  add_tlsigs(patch_sigs[i], tmp_sig);
	}
    }
					
  dwnsample->dsp_func(s, dwnsample);

  multiply_sigs(dwnsample->o_sigs[0], dac_amp_ctls[0]->ctl_sig);
  multiply_sigs(dwnsample->o_sigs[1], dac_amp_ctls[1]->ctl_sig);
    
  dac_sigs[0] = dwnsample->o_sigs[0];
  dac_sigs[1] = dwnsample->o_sigs[1];


}




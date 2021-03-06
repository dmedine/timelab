#include "tl_core.h"
#include "m_modules.h"
#include "math.h"
#include "stdio.h"
#include "stdlib.h"

FILE *fp;

// globals
#define osc_cnt 2
static tl_ctl *b_reset; // bang to reset
static tl_ctl *b_sync_toggle; // first of osc_cnt^2 bangs to toggle sync
static tl_ctl *b_sync_check;
static tl_UDS_solver *solver;
static tl_UDS_node *sync; // node to check and flag for sync timing
static tl_ctl *osc_head;
static tl_dac *dac;

// matrix of who syncs who
int sync_matrix[osc_cnt][osc_cnt];

// fm oscillator structure
typedef struct _UDS_osc{
  
  // all node and ctl ptrs are freed automatically
  // but one must be careful not to free this struct first!
  tl_UDS_node *sin_node; //
  tl_UDS_node *cos_node; // oscillator building blocks
  tl_ctl *k_osc_freq;
  tl_ctl *k_mod_depth[osc_cnt]; // osc_cnt of these for each oscillator
  tl_smp *alphas; // accumulated modulation coefficients
  tl_ctl *k_sync_thresh; // where in the cycle to synchronize
  tl_ctl *k_delta; // push back to unit circle
  tl_ctl *k_sync_g; // sync gain
  tl_ctl *k_mu; // 
  tl_ctl *k_gamma[osc_cnt];
  //tl_ctl *b_sync_who; // controls who syns who
  
  int which; // which oscillator is it  
  int sync_from[osc_cnt];
  int is_sync;

}tl_UDS_osc;

// our oscillators
tl_UDS_osc **oscs;

// bang data for sync matrix
int toggle_data[osc_cnt*osc_cnt][2];
// kill and init functions for oscillators
static tl_UDS_osc *init_UDS_osc(int which);

static void kill_UDS_osc(tl_UDS_osc *x);

const int in_cnt = 0;
const int out_cnt = osc_cnt*2;

// bang function to reset the oscillators
void reset_oscs(void){

  tl_UDS_node *x;
  int i;
  printf("hello from reset_oscs\n");

  for(i=0;i<osc_cnt;i++)
    {
      tl_reset_UDS_node(oscs[i]->sin_node, 0.0);
      tl_reset_UDS_node(oscs[i]->cos_node, -1.0);

    }
}


// ctl interface to set sync matrix
// do we even need to call this?
void set_sync_matrix(void *data){

  int i,j;
  int cnt = 0;
  int *i_data = (int *)data;
  for(i=0;i<osc_cnt;i++)
    for(j=0;j<osc_cnt;j++)
      {
	if(i_data[0]==i && i_data[1]==j)
	  {
	    sync_matrix[i][j]+=1;
	    sync_matrix[i][j]%=2;
	  }	
	printf("matrix[%d][%d] %d\n",i,j,sync_matrix[i][j]);
	cnt++;
      }
}

// solves for sine part
static tl_smp osc_sin(tl_UDS_node *x, int iter){


  int i,j;
  tl_smp out; // output
  tl_smp omega; // frequency
  tl_smp delta; // push back factor
  tl_smp sync_thresh; // sync threshold
  tl_smp g; // sync gain
  tl_smp r = 0; // radius -- not a ctl
  tl_smp mu; // van der pol variable
  tl_smp gamma[osc_cnt]; // forcing amplitude eqaution varaible
  tl_smp lambda_n = 0; // frequency addition here
  int sync = 0;

  int block_len = tl_get_block_len();
    
  tl_UDS_osc *y = (tl_UDS_osc*)x->extra_data;
  //printf("sin : %f %f\n", *x->data_in[0], *x->data_in[1]);
  int which = y->which;
  int who[osc_cnt];
  // first ctl
  omega = y->k_osc_freq->outlet->smps[iter];//x->ctls->outlet->smps[iter];
  // second        1     2     
  sync_thresh = y->k_sync_thresh->outlet->smps[iter];//x->ctls->next->outlet->smps[iter];
  // third   1     2     3 
  delta = y->k_delta->outlet->smps[iter];//x->ctls->next->next->outlet->smps[iter];
  // fourth 1     2     3     4     
  g = y->k_sync_g->outlet->smps[iter];//x->ctls->next->next->next->outlet->smps[iter];
  mu = y->k_mu->outlet->smps[iter];
    
  for(i=0;i<osc_cnt; i++)
    {
      gamma[i] = y->k_gamma[i]->outlet->smps[iter] * *x->data_in[i*2];
      // find the lambdas  
      lambda_n += *x->data_in[i*2] * y->k_mod_depth[i]->outlet->smps[iter];
      
      // check if we are straying from the unit circle
      if(i==which)
  	r = sqrt(*x->data_in[i*2] * *x->data_in[i*2] + *x->data_in[i*2+1] * *x->data_in[i*2+1]);


      if(sync_matrix[i][which]!=0 && *x->data_in[i*2]>sync_thresh)
	sync = 1;

    }

  // sync regime
  if(sync==1)
      //      printf("%d s\n", which);
      out = g*(-1.0-*x->data_in[which*2]);

  // normal regime
  else
    out = mu * (1 - *x->data_in[which*2+1] * *x->data_in[which*2+1])
      * *x->data_in[which*2]
      - (omega + lambda_n) 
      * *x->data_in[which*2+1];


  for(i=0;i<osc_cnt;i++)
    out-=gamma[i];
  
  return out;

}

// solves for cosine part
static tl_smp osc_cos(tl_UDS_node *x, int iter){
  int i,j;
  tl_smp out; // output
  tl_smp omega; // frequency
  tl_smp delta; // push back factor
  tl_smp sync_thresh; // sync threshold
  tl_smp g; // sync gain
  tl_smp r = 0; // radius -- not a ctl
  tl_smp mu;  // van der pol variable
  tl_smp gamma[osc_cnt]; // forcing amplitude
  tl_smp lambda_n = 0; // frequency addition here
  int sync = 0;

  int block_len = tl_get_block_len();

  tl_UDS_osc *y = (tl_UDS_osc*)x->extra_data;
  //printf("cos : %f %f\n", *x->data_in[0], *x->data_in[1]);
  int which = y->which;
  int who;
  // first ctl
  omega = y->k_osc_freq->outlet->smps[iter];//x->ctls->outlet->smps[iter];
  // second        1     2     
  sync_thresh = y->k_sync_thresh->outlet->smps[iter];//x->ctls->next->outlet->smps[iter];
  // third   1     2     3 
  delta = y->k_delta->outlet->smps[iter];//x->ctls->next->next->outlet->smps[iter];
  // fourth 1     2     3     4     
  g = y->k_sync_g->outlet->smps[iter];//x->ctls->next->next->next->outlet->smps[iter];
  mu = y->k_mu->outlet->smps[iter];
   

  // find the lambdas
  for(i=0;i<osc_cnt; i++)
    {
      gamma[i] = y->k_gamma[i]->outlet->smps[iter] * *x->data_in[i*2];
      lambda_n += *x->data_in[i*2] *y->k_mod_depth[i]->outlet->smps[iter];
      
      // check if we are straying from the unit circle
      if(i==which)
  	r = sqrt(*x->data_in[i*2] * *x->data_in[i*2] + *x->data_in[i*2+1] * *x->data_in[i*2+1]);



      if(sync_matrix[i][which]!=0 && *x->data_in[i*2]>sync_thresh)
	{
	  sync = 1;        
	  who = i;    
	}
    }

  if(sync==1)
      // do the sync routine
      out = g*(0.0-*x->data_in[which*2+1]);

  else
    out = (omega + lambda_n)
      * *x->data_in[which*2];

  /* for(i=0;i<osc_cnt;i++) */
  /*   out-=gamma[i]; */

  return out;


}

// init func
void tl_init_fm_vdp(tl_arglist *args){

  int i, j;
  tl_procession *procession; // needed for DAC
  // check for a procession in the args
  if(args->argv[0]->type!=TL_PROCESSION)
    {
      printf("error: tl_init_fm : first init argument needs to be a valid procession pointer\n");
      return;
    }
  else procession = args->argv[0]->procession;

  // initialize the solver
  solver = tl_init_UDS_solver(0, osc_cnt*2, 1);

  // we need a dac, so make one
  dac = tl_init_dac(procession, osc_cnt*2, 1);

  // initialize the local ctl list head
  osc_head = init_ctl(TL_HEAD_CTL);

  // initialize the oscillator parts
  oscs = malloc(sizeof(tl_UDS_osc *) * osc_cnt);
  for(i=0;i<osc_cnt; i++)
    {
      // initialize
      oscs[i] = init_UDS_osc(i);

      // install into the solver
      oscs[i]->sin_node->extra_data = (void *)oscs[i];
      oscs[i]->cos_node->extra_data = (void *)oscs[i];
      tl_push_UDS_node(solver->UDS_net, oscs[i]->sin_node);
      tl_push_UDS_node(solver->UDS_net, oscs[i]->cos_node);


    }
  
  for(i=0;i<osc_cnt*2; i++)
    dac->inlets[i] = solver->outlets[i];
  reset_oscs(); // initialize the oscillators' states

  // connect the outlet of each oscillator to a each inlet on each one
  // including its own
  int k;
  for(i=0;i<osc_cnt;i++)
    {
      k = 0;
      for(j=0;j<osc_cnt*2;j+=2)
	{
	  // arrange the inputs in the same order for each oscillator
	  oscs[i]->sin_node->data_in[j] = oscs[k]->sin_node->data_out;
	  oscs[i]->cos_node->data_in[j] = oscs[k]->sin_node->data_out;

	  oscs[i]->sin_node->data_in[j+1] = oscs[k]->cos_node->data_out;
	  oscs[i]->cos_node->data_in[j+1] = oscs[k++]->cos_node->data_out;

	}

    }



  // define the reset function bang ctl
  b_reset = init_ctl(TL_BANG_CTL);
  b_reset->name = name_new("reset_fm_oscs");
  b_reset->bang_func = reset_oscs;
  install_onto_ctl_list(osc_head, b_reset);

  tl_ctl *next_toggle;
  char buf[50];
  int cnt = 0;
  next_toggle = b_sync_toggle;
  for(i=0;i<osc_cnt;i++)
    for(j=0;j<osc_cnt;j++)
      {
	// initialize the control with appropriate name
	sprintf(buf, "b_sync_toggle_%d_%d",i+1, j+1);
	next_toggle = init_ctl(TL_BANG_CTL);
	next_toggle->name = name_new(buf);

	// give it a numeric tag
	toggle_data[cnt][0] = i;
	toggle_data[cnt][1] = j;
	
	// set the bang function
	next_toggle->bang_func = set_sync_matrix;

	// assign the tag
	set_ctl_bang_data(next_toggle, (void *)toggle_data[cnt++]);
	//printf("tl_init_fm: %p %p\n",toggle_data, next_toggle->bang_data);
	// install the current ctl
	install_onto_ctl_list(osc_head, next_toggle);
      }




  for(i=0;i<osc_cnt;i++)
    for(j=0;j<osc_cnt;j++)
      sync_matrix[i][j] = 0;


}

static tl_UDS_osc *init_UDS_osc(int which){

  tl_UDS_osc *x = malloc(sizeof(tl_UDS_osc));
  char buf[50];
  int i;

  // initialize the nodes
  x->sin_node = tl_init_UDS_node(osc_sin, osc_cnt*2, 1);
  x->cos_node = tl_init_UDS_node(osc_cos, osc_cnt*2, 1);

  // initialize the controls
  x->k_osc_freq = init_ctl(TL_LIN_CTL);
  sprintf(buf, "k_osc_freq_%d", which);
  x->k_osc_freq->name = name_new(buf);
  set_ctl_val(x->k_osc_freq, 440);
  install_onto_ctl_list(osc_head, x->k_osc_freq);
	
  x->k_sync_thresh = init_ctl(TL_LIN_CTL);
  sprintf(buf, "k_sync_thresh_%d", which);
  x->k_sync_thresh->name = name_new(buf);
  set_ctl_val(x->k_sync_thresh, .9);
  install_onto_ctl_list(osc_head, x->k_sync_thresh);

  x->k_delta = init_ctl(TL_LIN_CTL);
  sprintf(buf, "k_delta_%d", which);
  x->k_delta->name = name_new(buf);
  set_ctl_val(x->k_delta, 0);
  install_onto_ctl_list(osc_head, x->k_delta);

  x->k_sync_g = init_ctl(TL_LIN_CTL);
  sprintf(buf, "k_sync_g_%d", which);
  x->k_sync_g->name = name_new(buf);
  set_ctl_val(x->k_sync_g, 1);
  install_onto_ctl_list(osc_head, x->k_sync_g);

  x->k_mu = init_ctl(TL_LIN_CTL);
  sprintf(buf, "k_mu_%d", which);
  x->k_mu->name = name_new(buf);
  set_ctl_val(x->k_mu, 0);
  install_onto_ctl_list(osc_head, x->k_mu);


  for(i=0;i<osc_cnt;i++)
    {
      x->k_mod_depth[i] = init_ctl(TL_LIN_CTL);
      sprintf(buf, "k_mod_depth_%d_%d", which,i);
      x->k_mod_depth[i]->name = name_new(buf);
      set_ctl_val(x->k_mod_depth[i], 0);
      install_onto_ctl_list(osc_head, x->k_mod_depth[i]);
    }

  for(i=0;i<osc_cnt;i++)
    {
      x->k_gamma[i] = init_ctl(TL_LIN_CTL);
      sprintf(buf, "k_gamma_%d_%d", which,i);
      x->k_gamma[i]->name = name_new(buf);
      set_ctl_val(x->k_gamma[i], 0);
      install_onto_ctl_list(osc_head, x->k_gamma[i]);
    }


  // the other variables
  x->is_sync = 0;
  x->which = which;

  x->alphas = malloc(sizeof(tl_smp)*osc_cnt*tl_get_block_len());
  for(i=0;i<osc_cnt*tl_get_block_len();i++)
    x->alphas[i] = 0;

  // copy the top ctl ptr into the node stuctures
  x->sin_node->ctls = x->k_osc_freq;
  x->cos_node->ctls = x->k_osc_freq;


  return x;

}


// control interface
tl_ctl *tl_reveal_ctls_fm_vdp(void){

  return osc_head;

}


// kill func
void tl_kill_fm_vdp(tl_class *class_ptr){

  // the kill class functions get called after kill ctl functions

  // do this before killing the oscillators
  // this should take care of all the nodes
  tl_kill_UDS_solver(solver);

  // kill the dac
  tl_kill_dac(dac);
  
  // now free our oscillators
  int i;
  for(i=0; i<osc_cnt; i++)
    {
      free(oscs[i]->alphas);
      free(oscs[i]);
    }

  free(oscs);

}

// dsp func
void tl_dsp_fm_vdp(int samples, void *mod_ptr){

  int samps = samples;
  int i,j,k;

  tl_dsp_UDS_solver(samps, solver);
  tl_dsp_dac(samps, dac);

}

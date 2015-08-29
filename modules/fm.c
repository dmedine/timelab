#include "tl_core.h"
#include "m_modules.h"
#include "math.h"
#include "stdio.h"
#include "stdlib.h"

FILE *fp;

// globals
#define osc_cnt 1
static tl_ctl *b_reset; // bang to reset
static tl_UDS_solver *solver;
static tl_UDS_node *sync; // node to check and flag for sync timing
static tl_ctl *osc_head;
static tl_dac *dac;

// matrix of who syncs who
// use double to make sure it is big
// enough to handle pd sample type
double sync_matrix[osc_cnt][osc_cnt];

// fm oscillator structure
typedef struct _UDS_osc{
  
  // all node and ctl ptrs are freed automatically
  // but one must be careful not to free this struct first!
  tl_UDS_node *sin_node; //
  tl_UDS_node *cos_node; // oscillator building blocks
  tl_ctl *k_osc_freq, *k_mod_depth; // fm ctrls
  tl_ctl *k_sync_thresh; // where in the cycle to synchronize
  tl_ctl *k_delta; // push back to unit circle
  tl_ctl *k_sync_g; // sync gain
  tl_ctl *b_sync_who; // controls who syns who
  
  int which; // which oscillator is it  
  int sync_from[osc_cnt];
  int is_sync;

}tl_UDS_osc;

// our oscillators
tl_UDS_osc **oscs;

static tl_smp *alphas; // accumulated modulation coefficients

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
  x=solver->UDS_net;
  for(i=0;i<osc_cnt;i++)
    {
      tl_reset_UDS_node(x, 0.0);
      x=x->next;
      tl_reset_UDS_node(x, -1.0);
      x=x->next;
    }
}

// determine sync routine
int check_sync_matrix(int which, int *armed){

  int i, j;
  int is_armed = 0;
  for(i=0;i<osc_cnt;i++)
    if(sync_matrix[which][i] == 1)
      {
	is_armed = 1;
	armed[i] = 1;
      }
    else armed[i] = 0;
  return is_armed;
}

// ctl interface to set sync matrix
void set_sync_matrix(void *data){



}

// solves for sine part
static tl_smp osc_sin(tl_UDS_node *x, int iter){


  int i;
  tl_smp out; // output
  tl_smp omega; // frequency
  tl_smp delta; // push back factor
  tl_smp sync_thresh; // sync threshold
  tl_smp g; // sync gain
  tl_smp r = 0; // radius -- not a ctl
  tl_smp lambda_n = 0; // frequency addition here
    
  tl_UDS_osc *y = (tl_UDS_osc*)x->extra_data;
  //printf("sin : %f %f\n", *x->data_in[0], *x->data_in[1]);
  int which = y->which;
  int who[osc_cnt];
  // first ctl
  omega = x->ctls->outlet->smps[iter];
  //printf("omega %f\n", omega);
  // skip second (get it in dsp loop for lambdas)
  // third      1     2     3
  sync_thresh = x->ctls->next->next->outlet->smps[iter];
  // fourth   1     2     3     4
  delta = x->ctls->next->next->next->outlet->smps[iter];
  // fifth 1     2     3     4     5
  g = x->ctls->next->next->next->next->outlet->smps[iter];
 

  // find the lambdas
  for(i=0;i<osc_cnt; i++)
    {
     // move this to the osc struct so we don't have to calculated twice
      lambda_n += *x->data_in[i*2] * alphas[i*tl_get_block_len()+iter];
      //printf("lambda_n: %f\n", lambda_n);            
      // check if we are straying from the unit circle
      if(i==which)
  	r = sqrt(*x->data_in[i*2] * *x->data_in[i*2] + *x->data_in[i*2+1] * *x->data_in[i*2+1]);


      // check the sync matrix
      check_sync_matrix(which, who);

      // check each master to see if it is above the threshold
      for(i=0;i<osc_cnt;i++)
  	if(*x->data_in[i*2+1]>sync_thresh && who[i] == 1)
  	  // if so, do the sync routine
  	  y->is_sync = 1;
      // else, not
      else y->is_sync = 0;

    }

  if(y->is_sync==1)
    out = g*(0.0-*x->data_in[which*2+1]);

  else
    out = (omega + lambda_n) * *x->data_in[which*2+1]+delta*(1.0-r);
    // out = omega  * *x->data_in[which*2+1];
    
  //printf("sin in : %f\n", *x->data_in[which*2+1]);
  //printf("sin out : %f\n", out);  
  //out = 0.0;
  return out;

}

// solves for cosine part
static tl_smp osc_cos(tl_UDS_node *x, int iter){
  int i;
  tl_smp out; // output
  tl_smp omega; // frequency
  tl_smp delta; // push back factor
  tl_smp sync_thresh; // sync threshold
  tl_smp g; // sync gain
  tl_smp r = 0; // radius -- not a ctl
  tl_smp lambda_n = 0; // frequency addition here
    
  tl_UDS_osc *y = (tl_UDS_osc*)x->extra_data;
  //printf("cos : %f %f\n", *x->data_in[0], *x->data_in[1]);
  int which = y->which;
  int who[osc_cnt];
  // first ctl
  omega = x->ctls->outlet->smps[iter];
  // skip second (get it in dsp loop for lambdas)
  // third      1     2     3
  sync_thresh = x->ctls->next->next->outlet->smps[iter];
  // fourth   1     2     3     4
  delta = x->ctls->next->next->next->outlet->smps[iter];
  // fifth 1     2     3     4     5
  g = x->ctls->next->next->next->next->outlet->smps[iter];
 

  // find the lambdas
  for(i=0;i<osc_cnt; i++)
    {
      // move this to the osc struct so we don't have to calculated twice
      lambda_n += *x->data_in[i*2] * alphas[i*tl_get_block_len()+iter];
      
      // check if we are straying from the unit circle
      if(i==which)
  	r = sqrt(*x->data_in[i*2] * *x->data_in[i*2] + *x->data_in[i*2+1] * *x->data_in[i*2+1]);

      // check for sync in the sine routine only
    }
  // this is the same variable accross each pair of odes
  if(y->is_sync==1)
    // do the sync routine
    out = g*(-1.0-*x->data_in[which*2]);
  // if it is also back to target, end sync
  //if(out == 0.0)y->is_sync = 0;
  else
    out = -1* (omega + lambda_n) * *x->data_in[which*2] + delta*(1.0-r);
    //out = -1* omega * *x->data_in[which*2];

  //printf("cos in : %f\n", *x->data_in[which*2]);
  //printf("cos out : %f\n", out);  
  //out = 0.0;
  return out;


}

tl_smp sync_func(tl_UDS_node *x, int iter){

}

// init func
void tl_init_fm(tl_arglist *args){

  int i, j;

  // initialize the solver
  solver = tl_init_UDS_solver(0, osc_cnt*2, 1);

  // we need a dac, so make one
  dac = tl_init_dac(osc_cnt*2, 1);

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

  alphas = malloc(sizeof(tl_smp)*osc_cnt*tl_get_block_len());

  for(i=0;i<osc_cnt;i++)
    for(j=0;j<osc_cnt;j++)
      sync_matrix[i][j] = 0;

}

static tl_UDS_osc *init_UDS_osc(int which){

  tl_UDS_osc *x = malloc(sizeof(tl_UDS_osc));
  char buf[50];

  // initialize the nodes
  x->sin_node = tl_init_UDS_node(osc_sin, osc_cnt*2, 1);
  x->cos_node = tl_init_UDS_node(osc_cos, osc_cnt*2, 1);

  // initialize the controls
  x->k_osc_freq = init_ctl(TL_LIN_CTL);
  sprintf(buf, "k_osc_freq_%d", which);
  x->k_osc_freq->name = name_new(buf);
  set_ctl_val(x->k_osc_freq, 440);

  x->k_mod_depth = init_ctl(TL_LIN_CTL);
  sprintf(buf, "k_mod_depth_%d", which);
  x->k_mod_depth->name = name_new(buf);
  set_ctl_val(x->k_mod_depth, 0);

  x->k_sync_thresh = init_ctl(TL_LIN_CTL);
  sprintf(buf, "k_sync_thresh_%d", which);
  x->k_sync_thresh->name = name_new(buf);
  set_ctl_val(x->k_sync_thresh, 0);

  x->k_delta = init_ctl(TL_LIN_CTL);
  sprintf(buf, "k_delta_%d", which);
  x->k_delta->name = name_new(buf);
  set_ctl_val(x->k_delta, 0);

  x->k_sync_g = init_ctl(TL_LIN_CTL);
  sprintf(buf, "k_sync_g_%d", which);
  x->k_sync_g->name = name_new(buf);
  set_ctl_val(x->k_sync_g, 100);

  x->b_sync_who = init_ctl(TL_BANG_CTL);
  sprintf(buf, "b_sync_who_%d", which);
  x->b_sync_bang->name = name_new(buf);
  set_ctl_bang_data(x->k_sync_bang, (void *)sync_matrix[which]);
  x->b_sync_who->bang_func = set_sync_matrix;

  // the other variables
  x->is_sync = 0;
  x->which = which;

  // stack the ctls together
  x->k_osc_freq->next = x->k_mod_depth;
  x->k_mod_depth->next = x->k_sync_thresh;
  x->k_sync_thresh->next = x->k_delta;
  x->k_delta->next = x->k_sync_g;
  x->k_sync_g->next = x->b_sync_who;

  // copy the top ctl ptr into the node stuctures
  x->sin_node->ctls = x->k_osc_freq;
  x->cos_node->ctls = x->k_osc_freq;

  // copy them all into the global ctl processor
  install_onto_ctl_list(osc_head, x->k_osc_freq);

  return x;

}


// control interface
tl_ctl *tl_reveal_ctls_fm(void){

  return osc_head;

}


// kill func
void tl_kill_fm(tl_class *class_ptr){

  // the kill class functions get called after kill ctl functions

  // do this before killing the oscillators
  // this should take care of all the nodes
  tl_kill_UDS_solver(solver);

  // kill the dac
  tl_kill_dac(dac);
  
  // now free our oscillators
  int i;
  for(i=0; i<osc_cnt; i++)
    free(oscs[i]);
  
  free(oscs);

  free(alphas);

}

// dsp func
void tl_dsp_fm(int samples, void *mod_ptr){

  int samps = samples;
  int i,j;
  for(i=0;i<osc_cnt;i++)
    for(j=0;j<samples;j++)
      alphas[i*samples+j]=oscs[i]->k_mod_depth->outlet->smps[j];

  // this is the easy part...
  tl_dsp_UDS_solver(samps, solver);
  tl_dsp_dac(samps, dac);

}

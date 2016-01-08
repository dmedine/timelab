// array of nonlinear oscillators
// the function to solve is
// y_dot = omega * x - alpha1 * x^2 - alpha2 * y^2 - beta1 *x^3 - beta2 * y^3
// x_dot = -omega * y - alpha1 * x^2 - alpha2 * y^2 - beta1 *x^3 - beta2 * y^3

// control input is an array of N values giving an intial
// 'pluck' displacement of the string

#include "tl_core.h"
#include "m_modules.h"
#include <stdlib.h>
#include <stdio.h>
#include <math.h>


static tl_UDS_node *node_x;
static tl_UDS_node *node_y;


static tl_UDS_solver *solver;
static tl_dac *dac;
static tl_ctl *ctl_head;
tl_smp wall_const; // used to terminate the ends of the string

tl_ctl *b_reset; // bang this to set the oscillators to initial cond.


// number of modes to the string
#define OSC_CNT 1

// needed for external loaders
const int in_cnt = 0;
const int out_cnt = OSC_CNT*2;


typedef struct _nl_osc{ // mass spring emelment on an ideal string


  tl_UDS_node *pos_node; // pos node
  tl_UDS_node *vel_node; // left vel

  // control pointers for the parameters
  tl_ctl *k_omega; // frequency
  tl_ctl *k_alpha1; // 
  tl_ctl *k_alpha2; // 
  tl_ctl *k_beta1; // 
  tl_ctl *k_beta2; // 
  
  tl_smp disp;
  tl_smp vel;
  
  int which;

}tl_nl_osc;

tl_nl_osc **nl_oscs;


static tl_nl_osc *init_UDS_nl_osc(int which);

// bang pluck function
void reset(void){

  
  int i;
  tl_UDS_node *x=solver->UDS_net->next;
  tl_smp omega;
  for(i=0;i<OSC_CNT;i++) {

    tl_reset_UDS_node(x, -1);
    x = x->next;

    tl_reset_UDS_node(x, 0);//string_pts[i]->vel);
    x = x->next;

  }
}

static tl_smp pos_dot(tl_UDS_node *x, int iter){ 

  tl_smp out;
  tl_nl_osc *y = (tl_nl_osc *)x->extra_data;
  out = y->k_omega->outlet->smps[iter] * *x->data_in[1];// /* + */
    /* y->k_alpha1->outlet->smps[iter] * *x->data_in[0] + */
    /* y->k_alpha2->outlet->smps[iter] * *x->data_in[1] +    */
    /* y->k_beta1->outlet->smps[iter] * *x->data_in[0] + */
    /* y->k_beta2->outlet->smps[iter] * *x->data_in[1];   */

  return out;
}

tl_smp vel_dot(tl_UDS_node *x, int iter){

  tl_smp out = 0;
  tl_nl_osc *y = (tl_nl_osc *)x->extra_data;

  out = -1.0 * y->k_omega->outlet->smps[iter] * *x->data_in[0] -
    y->k_alpha1->outlet->smps[iter] * *x->data_in[0] -
    y->k_alpha2->outlet->smps[iter] * *x->data_in[1] -   
    y->k_beta1->outlet->smps[iter] * *x->data_in[0] -
    y->k_beta2->outlet->smps[iter] * *x->data_in[1];
  return out;
}

void tl_init_nl_osc(tl_arglist *args){

  int i;
  tl_procession *procession; // needed for DAC  
  // check for a procession in the args
  // this should never actually happen:
  if(args->argv[0]->type!=TL_PROCESSION) 
    {
      printf("error: tl_init_dyfunc_test : first init argument needs to be a valid procession pointer\n");
      return;
    }
  else procession = args->argv[0]->procession;

  ctl_head = init_ctl(TL_HEAD_CTL);
  
  solver = tl_init_UDS_solver(0, 
			      OSC_CNT*2, // two for each 
			      1);


  nl_oscs = malloc(sizeof(tl_nl_osc *) * (OSC_CNT));
  
  for(i=0;i<OSC_CNT; i++) {

    // initialize each point
    nl_oscs[i] = init_UDS_nl_osc(i);
    
    // use the extra data field for ease of access to stuff in the node
    nl_oscs[i]->pos_node->extra_data = (void *)nl_oscs[i];
    nl_oscs[i]->vel_node->extra_data = (void *)nl_oscs[i];

    // connect up the nodes
    nl_oscs[i]->pos_node->data_in[0] = nl_oscs[i]->pos_node->data_out;
    nl_oscs[i]->pos_node->data_in[1] = nl_oscs[i]->vel_node->data_out;

    nl_oscs[i]->vel_node->data_in[0] = nl_oscs[i]->pos_node->data_out;
    nl_oscs[i]->vel_node->data_in[1] = nl_oscs[i]->vel_node->data_out;
    
    // create the processing stack
    tl_push_UDS_node(solver->UDS_net, nl_oscs[i]->vel_node);
    tl_push_UDS_node(solver->UDS_net, nl_oscs[i]->pos_node);

  }
  


  // we need a dac, so make one
  dac = tl_init_dac(procession, OSC_CNT*2, 1);

  for(i=0;i<OSC_CNT*2;i++)
    dac->inlets[i] = solver->outlets[i];

  b_reset = init_ctl(TL_BANG_CTL);
  b_reset->name = name_new("reset");
  b_reset->bang_func = reset;
  install_onto_ctl_list(ctl_head, b_reset);


  
}

static tl_nl_osc *init_UDS_nl_osc(int which){

  tl_nl_osc *x = malloc(sizeof(tl_nl_osc));
  char buf[50];
  int i;
  
  // we need to keep track of this
  x->which = which;

  // initialize the velocity and position nodes
  x->pos_node = tl_init_UDS_node(pos_dot, 2, 1);
  x->vel_node = tl_init_UDS_node(vel_dot, 2, 1);


  // initialize the controls
  x->k_omega = init_ctl(TL_LIN_CTL);
  sprintf(buf, "k_omega_%d", which);
  x->k_omega->name = name_new(buf);
  set_ctl_val(x->k_omega, 0);

    // initialize the controls
  x->k_omega = init_ctl(TL_LIN_CTL);
  sprintf(buf, "k_omega_%d", which);
  x->k_omega->name = name_new(buf);
  set_ctl_val(x->k_omega, 0);

  // initialize the controls
  x->k_alpha1 = init_ctl(TL_LIN_CTL);
  sprintf(buf, "k_alpha1_%d", which);
  x->k_alpha1->name = name_new(buf);
  set_ctl_val(x->k_alpha1, 0);

  // initialize the controls
  x->k_alpha2 = init_ctl(TL_LIN_CTL);
  sprintf(buf, "k_alpha2_%d", which);
  x->k_alpha2->name = name_new(buf);
  set_ctl_val(x->k_alpha2, 0);

  // initialize the controls
  x->k_beta1 = init_ctl(TL_LIN_CTL);
  sprintf(buf, "k_beta1_%d", which);
  x->k_beta1->name = name_new(buf);
  set_ctl_val(x->k_beta1, 0);

  // initialize the controls
  x->k_beta2 = init_ctl(TL_LIN_CTL);
  sprintf(buf, "k_beta2_%d", which);
  x->k_beta2->name = name_new(buf);
  set_ctl_val(x->k_beta2, 0);

  // install the controls correctly
  x->k_omega->next = x->k_alpha1;
  x->k_alpha1->next = x->k_alpha2;
  x->k_alpha2->next = x->k_beta1;
  x->k_beta1->next = x->k_beta2;
  
  install_onto_ctl_list(ctl_head, x->k_omega);

  x->pos_node->ctls = x->k_omega;
  x->vel_node->ctls = x->k_omega;
  
  return x;

}

tl_ctl *tl_reveal_ctls_nl_osc(void){
  return ctl_head;
}

void tl_kill_nl_osc(tl_class *class_ptr){

  int i;

  tl_kill_UDS_solver(solver); // kills the nodes automatically
  tl_kill_dac(dac);
  
  for(i=0;i<OSC_CNT;i++) {

    free(nl_oscs[i]);

  }
  free(nl_oscs);

}

void tl_dsp_nl_osc(int samples, void *mod_ptr){

  tl_dsp_UDS_solver(samples, solver);
  tl_dsp_dac(samples, dac);

}

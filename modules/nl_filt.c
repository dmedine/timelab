#include "tl_core.h"
#include "m_modules.h"
#include "math.h"
#include "stdio.h"
#include "stdlib.h"

FILE *fp;

// globals
// timelab modules
static tl_UDS_solver *solver;
static tl_dac *dac;
static tl_adc *adc;


// ctls
static tl_ctl *b_set_nl_filt; // bang to reset
static tl_ctl *nl_filt_head; // ctl head

#define N_NODES 3

// chua circuit things
tl_UDS_node *pos_node; 
tl_UDS_node *vel_node; 
tl_UDS_node *z_node;

tl_ctl *k_g;
tl_ctl *k_beta;
tl_ctl *k_damp;
tl_ctl *k_m0;
tl_ctl *k_m1;
tl_ctl *k_states[3];


const int in_cnt = 1;
const int out_cnt = 3;

// function to set the state information
void set_nl_filt(void){
  
  int i;
  tl_UDS_node *x=solver->UDS_net->next;
  tl_smp omega;
  for(i=0;i<N_NODES;i++) {
    // set the displacement according to orders
    tl_reset_UDS_node(x, k_states[i]->outlet->smps[0]);
    x = x->next;
  }

}

static tl_smp phi_piecewise(tl_smp x) {

  
  tl_smp m0;
  tl_smp m1;
  tl_smp sgn = -1.0;
  
  m0 = k_m0->outlet->smps[0];
  m1 = k_m1->outlet->smps[0];

  if(x>=0) 
    sgn = 1.0;
    
  return m1*x + .5*(m0-m1) * (sgn * (x+1.0) - sgn * (x-1.0));
  
}

static tl_smp phi_cubic(tl_smp x) {

  tl_smp m0;
  tl_smp m1;
  
  m0 = k_m0->outlet->smps[0];
  m1 = k_m1->outlet->smps[0];

  return m0*x*x*x + m1*x;
  
}

// solves for sine part
static tl_smp pos_dot(tl_UDS_node *x, int iter){

  tl_smp out; // output
  tl_smp g    = k_g->outlet->smps[iter];
  tl_smp damp = k_damp->outlet->smps[iter]; 
  tl_smp m0   = k_m0->outlet->smps[iter];
  tl_smp m1   = k_m1->outlet->smps[iter];

  out = g * 2 * M_PI * phi_piecewise(*x->data_in[0])
    - solver->inlets[0]->smps[iter];
  
  return out;

}

// solves for cosine part
static tl_smp vel_dot(tl_UDS_node *x, int iter){

  tl_smp out;
  tl_smp g    = k_g->outlet->smps[iter];
  tl_smp damp = k_damp->outlet->smps[iter]; 
  tl_smp m0   = k_m0->outlet->smps[iter];
  tl_smp m1   = k_m1->outlet->smps[iter];

  out = -1.0 * g * 2 * M_PI * *x->data_in[0]
    - damp * *x->data_in[1]
    + *x->data_in[2];
  
  return out;

}

static tl_smp z_dot(tl_UDS_node *x, int iter) {

  tl_smp out;
  tl_smp beta = k_beta->outlet->smps[iter];

  out = -1.0 * beta * *x->data_in[1];
  
  return out;

}

// init func
void tl_init_nl_filt(tl_arglist *args){

  
  tl_procession *procession; // needed for DAC
  // check for a procession in the args
  if(args->argv[0]->type!=TL_PROCESSION)
    {
      printf("error: tl_init_fm : first init argument needs to be a valid procession pointer\n");
      return;
    }
  else procession = args->argv[0]->procession;




  
  // initialize the solver
  solver = tl_init_UDS_solver(2, N_NODES, 1);

  // adc:
  adc = tl_init_adc(procession, 2, 1);
  
  // we need a dac, so make one
  dac = tl_init_dac(procession, N_NODES, 1);

  // initialize the local ctl list head
  nl_filt_head = init_ctl(TL_HEAD_CTL);

  
  // initialize nl_filt solver nodes
  pos_node = tl_init_UDS_node(pos_dot, N_NODES, 1);
  tl_reset_UDS_node(pos_node, 0.0);
  tl_push_UDS_node(solver->UDS_net, pos_node);

  vel_node = tl_init_UDS_node(vel_dot, N_NODES, 1);
  tl_reset_UDS_node(vel_node, 0.0);
  tl_push_UDS_node(solver->UDS_net, vel_node);

  z_node = tl_init_UDS_node(z_dot, N_NODES, 1);
  tl_reset_UDS_node(z_node, 0.0);
  tl_push_UDS_node(solver->UDS_net, z_node);
  
  // hook up the inlets and outlets
  pos_node->data_in[0] = vel_node->data_out;
  pos_node->data_in[1] = pos_node->data_out;
  pos_node->data_in[2] = z_node->data_out;
   
  vel_node->data_in[0] = pos_node->data_out;
  vel_node->data_in[1] = vel_node->data_out;
  vel_node->data_in[2] = z_node->data_out;

  z_node->data_in[0] = pos_node->data_out;
  z_node->data_in[1] = vel_node->data_out;
  z_node->data_in[2] = z_node->data_out;
  
  // ctl setup
  int i;
  char buf[50];
  
  k_g = init_ctl(TL_LIN_CTL);
  sprintf(buf, "k_g");
  k_g->name = name_new(buf);
  set_ctl_val(k_g, 0);
  install_onto_ctl_list(nl_filt_head, k_g);

  k_beta = init_ctl(TL_LIN_CTL);
  sprintf(buf, "k_beta");
  k_beta->name = name_new(buf);
  set_ctl_val(k_beta, 0);
  install_onto_ctl_list(nl_filt_head, k_beta);
  
  k_damp = init_ctl(TL_LIN_CTL);
  sprintf(buf, "k_damp");
  k_damp->name = name_new(buf);
  set_ctl_val(k_damp, 0);
  install_onto_ctl_list(nl_filt_head, k_damp);

  k_m0 = init_ctl(TL_LIN_CTL);
  sprintf(buf, "k_m0");
  k_m0->name = name_new(buf);
  set_ctl_val(k_m0, 0);
  install_onto_ctl_list(nl_filt_head, k_m0);

  k_m1 = init_ctl(TL_LIN_CTL);
  sprintf(buf, "k_m1");
  k_m1->name = name_new(buf);
  set_ctl_val(k_m1, 0);
  install_onto_ctl_list(nl_filt_head, k_m1);

  for(i=0;i<N_NODES;i++) {

    k_states[i] = init_ctl(TL_LIN_CTL);
    sprintf(buf, "k_states_%d", i);
    k_states[i]->name = name_new(buf);
    set_ctl_val(k_states[i], 0);
    install_onto_ctl_list(nl_filt_head, k_states[i]);

  }

  // set up the set function
  b_set_nl_filt = init_ctl(TL_BANG_CTL);
  b_set_nl_filt->name = name_new("set_nl_filt_states");
  b_set_nl_filt->bang_func = set_nl_filt;
  install_onto_ctl_list(nl_filt_head, b_set_nl_filt);
  

  // hookup inlets to adc
  for(i=0;i<1;i++)
    solver->inlets[i] = adc->outlets[i];
  
  // hookup oulets to dac
  for(i=0;i<N_NODES;i++)
    dac->inlets[i] = solver->outlets[i];
    
  
}


// control interface
tl_ctl *tl_reveal_ctls_nl_filt(void){return nl_filt_head;}

// kill func
void tl_kill_nl_filt(tl_class *class_ptr){

  // the kill class functions get called after kill ctl functions
  tl_kill_UDS_solver(solver);
  tl_kill_dac(dac);
  tl_kill_adc(adc);

}

// dsp func
void tl_dsp_nl_filt(int samples, void *mod_ptr){

  int samps = samples;
  tl_dsp_adc(samps, adc);
  tl_dsp_UDS_solver(samps, solver);
  tl_dsp_dac(samps, dac);

}

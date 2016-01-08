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
static tl_ctl *b_set_lotka_volt; // bang to reset
static tl_ctl *lotka_volt_head; // ctl head

#define N_NODES 2

// chua circuit things
tl_UDS_node *pred_node; 
tl_UDS_node *prey_node; 

tl_ctl *k_alpha;
tl_ctl *k_beta;
tl_ctl *k_delta;
tl_ctl *k_gamma;
tl_ctl *k_m0;
tl_ctl *k_m1;
tl_ctl *k_states[2];


const int in_cnt = 0;
const int out_cnt = 2;

// function to set the state information
void set_lotka_volt(void){
  
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


static tl_smp pred_dot(tl_UDS_node *x, int iter){

  tl_smp out; // output
  tl_smp g    = k_g->outlet->smps[iter];
  tl_smp damp = k_damp->outlet->smps[iter]; 
  tl_smp m0   = k_m0->outlet->smps[iter];
  tl_smp m1   = k_m1->outlet->smps[iter];

  out = g * 2 * M_PI * phi_piecewise(*x->data_in[0])
    - solver->inlets[0]->smps[iter];
  
  return out;

}


static tl_smp prey_dot(tl_UDS_node *x, int iter){

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


// init func
void tl_init_lotka_volt(tl_arglist *args){

  
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

  // we need a dac, so make one
  dac = tl_init_dac(procession, N_NODES, 1);

  // initialize the local ctl list head
  lotka_volt_head = init_ctl(TL_HEAD_CTL);

  
  // initialize lotka_volt solver nodes
  pred_node = tl_init_UDS_node(pred_dot, N_NODES, 1);
  tl_reset_UDS_node(pred_node, 0.0);
  tl_push_UDS_node(solver->UDS_net, pred_node);

  prey_node = tl_init_UDS_node(prey_dot, N_NODES, 1);
  tl_reset_UDS_node(prey_node, 0.0);
  tl_push_UDS_node(solver->UDS_net, prey_node);

  // hook up the inlets and outlets
  pred_node->data_in[0] = pred_node->data_out;
  pred_node->data_in[1] = prey_node->data_out;
     
  prey_node->data_in[0] = pred_node->data_out;
  prey_node->data_in[1] = prey_node->data_out;
    
  // ctl setup
  int i;
  char buf[50];
  
  k_alpha = init_ctl(TL_LIN_CTL);
  sprintf(buf, "k_alpha");
  k_alpha->name = name_new(buf);
  set_ctl_val(k_alpha, 0);
  install_onto_ctl_list(lotka_volt_head, k_alpha);

  k_beta = init_ctl(TL_LIN_CTL);
  sprintf(buf, "k_beta");
  k_beta->name = name_new(buf);
  set_ctl_val(k_beta, 0);
  install_onto_ctl_list(lotka_volt_head, k_beta);
  
  k_delta = init_ctl(TL_LIN_CTL);
  sprintf(buf, "k_delta");
  k_delta->name = name_new(buf);
  set_ctl_val(k_delta, 0);
  install_onto_ctl_list(lotka_volt_head, k_delta);

  k_gamma = init_ctl(TL_LIN_CTL);
  sprintf(buf, "k_gamma");
  k_gamma->name = name_new(buf);
  set_ctl_val(k_gamma, 0);
  install_onto_ctl_list(lotka_volt_head, k_gamma);

  k_m0 = init_ctl(TL_LIN_CTL);
  sprintf(buf, "k_m0");
  k_m0->name = name_new(buf);
  set_ctl_val(k_m0, 0);
  install_onto_ctl_list(lotka_volt_head, k_m0);

  k_m1 = init_ctl(TL_LIN_CTL);
  sprintf(buf, "k_m1");
  k_m1->name = name_new(buf);
  set_ctl_val(k_m1, 0);
  install_onto_ctl_list(lotka_volt_head, k_m1);

  for(i=0;i<N_NODES;i++) {

    k_states[i] = init_ctl(TL_LIN_CTL);
    sprintf(buf, "k_states_%d", i);
    k_states[i]->name = name_new(buf);
    set_ctl_val(k_states[i], 0);
    install_onto_ctl_list(lotka_volt_head, k_states[i]);

  }

  // set up the set function
  b_set_lotka_volt = init_ctl(TL_BANG_CTL);
  b_set_lotka_volt->name = name_new("set_lotka_volt_states");
  b_set_lotka_volt->bang_func = set_lotka_volt;
  install_onto_ctl_list(lotka_volt_head, b_set_lotka_volt);
  

  
  // hookup oulets to dac
  for(i=0;i<N_NODES;i++)
    dac->inlets[i] = solver->outlets[i];
    
  
}


// control interface
tl_ctl *tl_reveal_ctls_lotka_volt(void){return lotka_volt_head;}

// kill func
void tl_kill_lotka_volt(tl_class *class_ptr){

  // the kill class functions get called after kill ctl functions
  tl_kill_UDS_solver(solver);
  tl_kill_dac(dac);

}

// dsp func
void tl_dsp_lotka_volt(int samples, void *mod_ptr){

  int samps = samples;
  tl_dsp_UDS_solver(samps, solver);
  tl_dsp_dac(samps, dac);

}

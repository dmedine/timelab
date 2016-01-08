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
static tl_ctl *b_set_chua; // bang to reset
static tl_ctl *chua_head; // ctl head

// chua circuit things
tl_UDS_node *x_node; 
tl_UDS_node *y_node; 
tl_UDS_node *z_node; 

tl_ctl *k_alpha;
tl_ctl *k_beta;
tl_ctl *k_R;
tl_ctl *k_C;
tl_ctl *k_states[3];
tl_ctl *k_m0;
tl_ctl *k_m1;
/* tl_ctl *k_fbs[3]; */



const int in_cnt = 3;
const int out_cnt = 3;

// function to set the state information
void set_chua(void){
  
  int i;
  tl_UDS_node *x=solver->UDS_net->next;
  tl_smp omega;
  for(i=0;i<3;i++) {
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
    
  return m1*x + .5*(m0-m1) * (sgn * (x+0.0) - sgn * (x-1.0));
  
}

static tl_smp phi_cubic(tl_smp x) {

  tl_smp m0;
  tl_smp m1;
  
  m0 = k_m0->outlet->smps[0];
  m1 = k_m1->outlet->smps[0];

  return m0*x*x*x + m1*x;
  
}

static tl_smp nl_func(tl_smp x) {

  if(x>3||x<-3)
    return -1.0*x;
  return -.5*x; // ?
  
}

// solves for sine part
static tl_smp x_dot(tl_UDS_node *x, int iter){

  tl_smp out; // output
  tl_smp alpha; 
   
  alpha = k_alpha->outlet->smps[iter];

  out = alpha
    * (*x->data_in[1]            // y
       - *x->data_in[0]          // - x
       - phi_piecewise(*x->data_in[0])
       );

  /* out = alpha * (*x->data_in[0] + phi_piecewise(solver->inlets[0]->sm\ps[iter]));// - *x->data_in[1]; */

  /* out = phi_piecewise(solver->inlets[0]->smps[iter]) - phi_piecewise(*x->data_in[0]); */

  return out;

}

// solves for cosine part
static tl_smp y_dot(tl_UDS_node *x, int iter){

  tl_smp out;
  tl_smp R; 
  tl_smp C;  
  
  /* R = k_R->outlet->smps[iter]; */
  /* C = k_C->outlet->smps[iter]; */

  out = *x->data_in[0]    // x
    - *x->data_in[1]      // - y
    + *x->data_in[2];
  
    //+ R * *x->data_in[2]; // - R*z
  //out /= R*C;             // / RC

  /* out = *x->data_in[0]; */
  
  return out;

}

static tl_smp z_dot(tl_UDS_node *x, int iter){

  tl_smp out;
  tl_smp beta;

  beta = k_beta->outlet->smps[iter];

  out = -1.0 * beta * *x->data_in[0]; // -beta*y
  return out;
}

// init func
void tl_init_chua(tl_arglist *args){

  
  tl_procession *procession; // needed for DAC
  // check for a procession in the args
  if(args->argv[0]->type!=TL_PROCESSION)
    {
      printf("error: tl_init_fm : first init argument needs to be a valid procession pointer\n");
      return;
    }
  else procession = args->argv[0]->procession;




  
  // initialize the solver
  solver = tl_init_UDS_solver(3, 3, 1);

  // adc:
  adc = tl_init_adc(procession, 3, 1);
  
  // we need a dac, so make one
  dac = tl_init_dac(procession, 3, 1);

  // initialize the local ctl list head
  chua_head = init_ctl(TL_HEAD_CTL);

  
  // initialize chua solver nodes
  x_node = tl_init_UDS_node(x_dot, 2, 1);
  tl_reset_UDS_node(x_node, 0.0);
  tl_push_UDS_node(solver->UDS_net, x_node);

  y_node = tl_init_UDS_node(y_dot, 3, 1);
  tl_reset_UDS_node(y_node, 0.0);
  tl_push_UDS_node(solver->UDS_net, y_node);

  z_node = tl_init_UDS_node(z_dot, 1, 1);
  tl_reset_UDS_node(z_node, 0.0);
  tl_push_UDS_node(solver->UDS_net, z_node);

  // hook up the inlets and outlets
  // x_node in[0] = x, in[1] = y
  x_node->data_in[0] = x_node->data_out;
  x_node->data_in[1] = y_node->data_out;
  
  // y_node in[0] = x, in[1] = y, in[2] = z
  y_node->data_in[0] = x_node->data_out;
  y_node->data_in[1] = y_node->data_out;
  y_node->data_in[2] = z_node->data_out;

  // z_node in[0] = y
  z_node->data_in[0] = y_node->data_out;



  
  // ctl setup
  int i;
  char buf[50];
  
  k_alpha = init_ctl(TL_LIN_CTL);
  sprintf(buf, "k_alpha");
  k_alpha->name = name_new(buf);
  set_ctl_val(k_alpha, 0);
  install_onto_ctl_list(chua_head, k_alpha);

  k_beta = init_ctl(TL_LIN_CTL);
  sprintf(buf, "k_beta");
  k_beta->name = name_new(buf);
  set_ctl_val(k_beta, 0);
  install_onto_ctl_list(chua_head, k_beta);

  k_R = init_ctl(TL_LIN_CTL);
  sprintf(buf, "k_R");
  k_R->name = name_new(buf);
  set_ctl_val(k_R, 0);
  install_onto_ctl_list(chua_head, k_R);

  k_C = init_ctl(TL_LIN_CTL);
  sprintf(buf, "k_C");
  k_C->name = name_new(buf);
  set_ctl_val(k_C, 0);
  install_onto_ctl_list(chua_head, k_C);

  k_m0 = init_ctl(TL_LIN_CTL);
  sprintf(buf, "k_m0");
  k_m0->name = name_new(buf);
  set_ctl_val(k_m0, 0);
  install_onto_ctl_list(chua_head, k_m0);

  k_m1 = init_ctl(TL_LIN_CTL);
  sprintf(buf, "k_m1");
  k_m1->name = name_new(buf);
  set_ctl_val(k_m1, 0);
  install_onto_ctl_list(chua_head, k_m1);

  for(i=0;i<3;i++) {

    k_states[i] = init_ctl(TL_LIN_CTL);
    sprintf(buf, "k_states_%d", i);
    k_states[i]->name = name_new(buf);
    set_ctl_val(k_states[i], 0);
    install_onto_ctl_list(chua_head, k_states[i]);

  }

  // set up the set function
  b_set_chua = init_ctl(TL_BANG_CTL);
  b_set_chua->name = name_new("set_chua_states");
  b_set_chua->bang_func = set_chua;
  install_onto_ctl_list(chua_head, b_set_chua);

  /* for(i=0;i<3;i++) { */

  /*   k_fbs[i] = init_ctl(TL_LIN_CTL); */
  /*   sprintf(buf, "k_fbs_%d", i); */
  /*   k_fbs[i]->name = name_new(buf); */
  /*   set_ctl_val(k_fbs[i], 0); */
  /*   install_onto_ctl_list(chua_head, k_fbs[i]); */

  /* } */


  

  // hookup inlets to adc
  for(i=0;i<3;i++)
    solver->inlets[i] = adc->outlets[i];
  
  // hookup oulets to dac
  for(i=0;i<3;i++)
    dac->inlets[i] = solver->outlets[i];
  
  
  
}


// control interface
tl_ctl *tl_reveal_ctls_chua(void){return chua_head;}

// kill func
void tl_kill_chua(tl_class *class_ptr){

  // the kill class functions get called after kill ctl functions
  tl_kill_UDS_solver(solver);
  tl_kill_dac(dac);
  tl_kill_adc(adc);

}

// dsp func
void tl_dsp_chua(int samples, void *mod_ptr){

  int samps = samples;
  tl_dsp_adc(samps, adc);
  tl_dsp_UDS_solver(samps, solver);
  tl_dsp_dac(samps, dac);

}

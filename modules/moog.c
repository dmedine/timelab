#include "tl_core.h"
#include "m_modules.h"
#include "math.h"
#include "stdio.h"
#include "stdlib.h"

// module parts
static tl_UDS_solver *solver;
static tl_UDS_node *node[4];
static tl_dac *dac;
static tl_adc *adc;

int in_cnt = 1;
int out_cnt = 1; 

// controls
tl_ctl *ctls;
tl_ctl *b_reset;
tl_ctl *k_cutoff;
tl_ctl *k_res;
tl_ctl *k_sat;


// reset function to bang
void reset(void){

  tl_UDS_node *x;
int i;
  x = solver->UDS_net->next;
  for(i=0;i<4;i++);
    {
      tl_reset_UDS_node(x, 0.0);
      x=x->next;
    }

}


// define any clipping function...
static tl_smp clip(tl_smp data, tl_smp sat, tl_smp satinv){


  return sat * tanh(data*satinv);
}

// the first stage is unique.
// it is the filter input minus the 
// feedback from the last stage (4th node)
static tl_smp first_stage(tl_UDS_node *x, int iter){

  tl_smp out;
  tl_smp alpha = k_cutoff->outlet->smps[iter];
  tl_smp res = k_res->outlet->smps[iter];
  tl_smp sat = k_sat->outlet->smps[iter];
  tl_smp satinv = 1.0/sat;  
  // printf("%f\n", solver->inlets[0]->smps[iter]); 
  out = alpha * (
		 // solver inlet
		 clip(solver->inlets[0]->smps[iter] -
		      // last stage * resonance
		      res * *x->data_in[0], sat, satinv) -
		 // current state
		 clip(*x->data_in[1], sat, satinv)
		 );

  //out = 0;
  return out;

}

// other three stages (no input or resonance
static tl_smp stage(tl_UDS_node *x, int iter){

  tl_smp out;
  tl_smp alpha = k_cutoff->outlet->smps[iter];
  tl_smp sat = k_sat->outlet->smps[iter];
  tl_smp satinv = 1.0/sat;  


  out = alpha * (clip(*x->data_in[0],sat, satinv) - clip(*x->data_in[1],sat, satinv));
  /* out = 0; */
  return out;

}

void tl_init_moog(tl_arglist *args){

  tl_procession *procession; // for dac
  // check for a procession in the args
  if(args->argv[0]->type!=TL_PROCESSION)
    {
      printf("error: tl_init_moog: first init argument needs to be a valid procession pointer\n");
      return;
    }
  else procession = args->argv[0]->procession;

  // initialize solver:
  solver = tl_init_UDS_solver(1, // ins 
			      4, // outs, at least number of nodes
			      1); // upsampling (not yet implemented)

  // dac:
  dac = tl_init_dac(procession, 1, 1);

  // adc:
  adc = tl_init_adc(procession, 1, 1);

  solver->inlets[0] = adc->outlets[0];

  // control interface:
  ctls = init_ctl(TL_HEAD_CTL);

  int i;
  // init and install nodes
  node[0] = tl_init_UDS_node(first_stage, 2, 1);
  tl_reset_UDS_node(node[0],0.0);
  tl_push_UDS_node(solver->UDS_net, node[0]);
  
  for(i=1;i<4;i++)
    {
      // init and install
      node[i] = tl_init_UDS_node(stage, 2, 1);
      tl_reset_UDS_node(node[i],0.0);
      tl_push_UDS_node(solver->UDS_net, node[i]);
 
      // connect inputs
      node[i]->data_in[0] = node[i-1]->data_out;
      node[i]->data_in[1] = node[i]->data_out;

   }

  // finish connections to first stage
  node[0]->data_in[0] = node[3]->data_out;
  node[0]->data_in[1] = node[0]->data_out;

  // init controls
  b_reset = init_ctl(TL_BANG_CTL);
  b_reset->name = name_new("reset");
  b_reset->bang_func = reset;
  install_onto_ctl_list(ctls, b_reset);

  k_cutoff = init_ctl(TL_LIN_CTL);
  k_cutoff->name = name_new("cutoff");
  install_onto_ctl_list(ctls, k_cutoff);

  k_res = init_ctl(TL_LIN_CTL);
  k_res->name = name_new("resonance");
  install_onto_ctl_list(ctls, k_res);

  k_sat = init_ctl(TL_LIN_CTL);
  k_sat->name = name_new("saturation");
  set_ctl_val(k_sat, 1.0);
  install_onto_ctl_list(ctls, k_sat);

  // hook up to dac
  dac->inlets[0] = solver->outlets[3];


}

tl_ctl *tl_reveal_ctls_moog(void){return b_reset;}

void tl_kill_moog(tl_class *class_ptr){

  tl_kill_UDS_solver(solver); // destroys the nodes automatically
  tl_kill_dac(dac);
  tl_kill_adc(adc);
}

void tl_dsp_moog(int samples, void *mod_ptr){

  int s = samples;
  tl_dsp_adc(s, adc);
  tl_dsp_UDS_solver(s, solver);
  tl_dsp_dac(s, dac);

}


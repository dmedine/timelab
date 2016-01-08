#include "tl_core.h"
#include "m_modules.h"
#include "math.h"
#include "stdio.h"
#include "stdlib.h"

// module parts

static tl_UDS_solver *solver;
static tl_UDS_node *node1;
static tl_dac *dac;
static tl_adc *adc;

int in_cnt = 1;
int out_cnt = 1; 

// controls
tl_ctl *ctls;
tl_ctl *b_reset;


tl_triode_stage *triode_stages[stage_cnt];

// define any clipping function...
static tl_smp clip(tl_smp data, tl_smp sat, tl_smp satinv){


  return sat * tanh(data*satinv);
}


static tl_triode_stage *init_triode_stage(int which);

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


static tl_smp exper(tl_UDS_node *x, int iter){

  tl_smp out;
  return out;

}
void tl_init_triode(tl_arglist *args){

  tl_procession *procession; // for dac
  // check for a procession in the args
  if(args->argv[0]->type!=TL_PROCESSION)
    {
      printf("error: tl_init_moog_abuse: first init argument needs to be a valid procession pointer\n");
      return;
    }
  else procession = args->argv[0]->procession;

  // initialize solver:
  solver = tl_init_UDS_solver(0, // ins 
			      1, //
			      1); // upsampling (not yet implemented)

  // dac:
  dac = tl_init_dac(procession, 2, 1);

  // adc:
  adc = tl_init_adc(procession, 1, 1);

  solver->inlets[0] = adc->outlets[0];

  // control interface:
  ctls = init_ctl(TL_HEAD_CTL);

  // init ctls here

  // init controls
  b_reset = init_ctl(TL_BANG_CTL);
  b_reset->name = name_new("reset");
  b_reset->bang_func = reset;
  install_onto_ctl_list(ctls, b_reset);


  // hook up to dac
  dac->inlets[0] = solver->outlets[0];
  dac->inlets[1] = adc->outlets[0];


}


tl_ctl *tl_reveal_ctls_triode(void){return ctls;}

void tl_kill_triode(tl_class *class_ptr){


  tl_kill_UDS_solver(solver); // destroys the nodes automatically
  tl_kill_dac(dac);
  tl_kill_adc(adc);


}

void tl_dsp_triode(int samples, void *mod_ptr){

  int s = samples;
  tl_dsp_adc(s, adc);
  tl_dsp_UDS_solver(s, solver);
  tl_dsp_dac(s, dac);

}


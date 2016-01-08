#include "tl_core.h"
#include "m_modules.h"
#include "math.h"
#include "stdio.h"
#include "stdlib.h"

// module parts
#define stage_cnt 6 
static tl_UDS_solver *solver;
static tl_UDS_node *node[stage_cnt];
static tl_dac *dac;
static tl_adc *adc;

int in_cnt = 1;
int out_cnt = 1; 

// controls
tl_ctl *ctls;
tl_ctl *b_reset;

typedef struct _moog_stage{

  tl_ctl *k_cutoff; // one per stage?
  tl_ctl *k_sat; // one per stage?
  tl_ctl *k_res[stage_cnt]; // stage_cnt of these
  tl_UDS_node *node;

  int which;

}tl_moog_stage;

tl_moog_stage *moog_stages[stage_cnt];
static tl_moog_stage *init_moog_stage(int which);

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
static tl_smp general_stage(tl_UDS_node *x, int iter){

  tl_smp out;
  tl_moog_stage *y = (tl_moog_stage *)x->extra_data;
  int i;
  tl_smp f1, f2, sat, satinv, cutoff;
  cutoff = y->k_cutoff->outlet->smps[iter];
  sat = y->k_sat->outlet->smps[iter];
  satinv = 1.0/sat;
  /* if(iter==0) */
  /*   printf("y->whcih %d, sat %f, cutoff %f k_res3 %f\n", y->which, sat, cutoff, y->k_res[3]->outlet->smps[iter]); */

  if(y->which==0)
    f1 = solver->inlets[0]->smps[iter]; 
      
  else 
    f1 = *x->data_in[y->which-1];

  for(i=1;i<stage_cnt;i++)
    f1-=y->k_res[i]->outlet->smps[iter] * *x->data_in[i];

  f2 = *x->data_in[y->which];

  out = cutoff *(clip(f1,sat,satinv) - clip(f2,sat,satinv));



  //out = 0;
  return out;

}

void tl_init_moog_abuse(tl_arglist *args){

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
			      stage_cnt, //
			      1); // upsampling (not yet implemented)

  // dac:
  dac = tl_init_dac(procession, 1, 1);

  // adc:
  adc = tl_init_adc(procession, 1, 1);

  solver->inlets[0] = adc->outlets[0];

  // control interface:
  ctls = init_ctl(TL_HEAD_CTL);

  int i;
  // init the stages  
  for(i=0;i<stage_cnt;i++)
    {
      moog_stages[i] = init_moog_stage(i);
      tl_push_UDS_node(solver->UDS_net, moog_stages[i]->node);
    }    

  // hook 'em up
  int j;
  for(i=0;i<stage_cnt;i++)
    for(j=0;j<stage_cnt;j++)
      {
	moog_stages[i]->node->data_in[j] = moog_stages[j]->node->data_out; 
	printf("connecting node %d out to node %d slot %d\n",j,i,j);
      }

  // init controls
  b_reset = init_ctl(TL_BANG_CTL);
  b_reset->name = name_new("reset");
  b_reset->bang_func = reset;
  install_onto_ctl_list(ctls, b_reset);


  // hook up to dac
  dac->inlets[0] = solver->outlets[stage_cnt-1];


}

static tl_moog_stage *init_moog_stage(int which){

  tl_moog_stage *x = malloc(sizeof(tl_moog_stage));
  char buf[50];

  // init the node
  x->node = tl_init_UDS_node(general_stage, stage_cnt, 1);
  x->node->extra_data = (void *)x;
  x->which = which;


  // init the ctls
  x->k_sat = init_ctl(TL_LIN_CTL);
  sprintf(buf, "k_sat_%d",which);
  x->k_sat->name = name_new(buf);
  set_ctl_val(x->k_sat, 1.0);
  install_onto_ctl_list(ctls, x->k_sat);

  x->k_cutoff = init_ctl(TL_LIN_CTL);
  sprintf(buf, "k_cutoff_%d",which);
  x->k_cutoff->name = name_new(buf);
  set_ctl_val(x->k_cutoff, 440.0);
  install_onto_ctl_list(ctls, x->k_cutoff);

  int i;
  for(i=0;i<stage_cnt;i++)
    {
      x->k_res[i] = init_ctl(TL_LIN_CTL);
      sprintf(buf, "k_res_%d_to_%d",i,which);
      x->k_res[i]->name = name_new(buf);
      set_ctl_val(x->k_res[i], 0.0);
      install_onto_ctl_list(ctls, x->k_res[i]);
    }

  return x;
}

tl_ctl *tl_reveal_ctls_moog_abuse(void){return ctls;}

void tl_kill_moog_abuse(tl_class *class_ptr){


  int i;
  for(i=0;i<stage_cnt;i++)
    free(moog_stages[i]);
  
  tl_kill_UDS_solver(solver); // destroys the nodes automatically
  tl_kill_dac(dac);
  tl_kill_adc(adc);


}

void tl_dsp_moog_abuse(int samples, void *mod_ptr){

  int s = samples;
  tl_dsp_adc(s, adc);
  tl_dsp_UDS_solver(s, solver);
  tl_dsp_dac(s, dac);

}


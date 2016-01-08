#include "tl_core.h"
#include "m_modules.h"
#include "math.h"
#include "stdio.h"
#include "stdlib.h"

// module parts
#define stage_cnt 1
static tl_UDS_solver *solver;
static tl_UDS_node *node[stage_cnt];
static tl_dac *dac;
static tl_adc *adc;

int in_cnt = 1;
int out_cnt = 1; 

// controls
tl_ctl *ctls;
tl_ctl *b_reset;

typedef struct _triode_stage{

  tl_ctl *k_Up; // plate voltage
  tl_ctl *k_kp; // fittine param
  tl_ctl *k_kvb; // fitting param
  tl_ctl *k_kG1; // fitting param
  tl_ctl *k_X; // fitting param
  tl_ctl *k_mu; // ?
  tl_ctl *k_cap_inv; // derivative gain
  tl_UDS_node *node;

  int which;

}tl_triode_stage;

tl_triode_stage *triode_stages[stage_cnt];
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

// function to find E1 in triode model
static tl_smp calc_E1(tl_smp Eg, tl_smp* params){

  tl_smp out;

  tl_smp Up = params[0];
  tl_smp kp = params[1]; 
  tl_smp kvb = params[2];
  tl_smp kG1 = params[3];
  tl_smp X = params[4];
  tl_smp mu = params[5]; 

  tl_smp exp_term = kp * (1.0/mu + (Eg/sqrt(kvb+Up*Up)) ); 
  out = (Up/kp) * log(1 + exp(exp_term));

  return out;
}

static tl_smp nl_triode_func(tl_smp in, tl_smp *params){

  tl_smp out;

  tl_smp Up = params[0];
  tl_smp kp = params[1]; 
  tl_smp kvb = params[2];
  tl_smp kG1 = params[3];
  tl_smp X = params[4];
  tl_smp mu = params[5]; 


  tl_smp E1 = calc_E1(in, params);
  
  tl_smp sgn = 1;
  if(E1<0)sgn = -1;

  out = (pow(E1,X) / kG1) * (1+sgn);
  printf("%f %f\n",E1, out);
  /* out *=cap_inv; */
  /* printf("%f\n",out); */
  return out;

}

tl_smp Ip = 0.0;//
static tl_smp triode(tl_UDS_node *x, int iter){

  tl_smp out;
  tl_triode_stage *y = (tl_triode_stage *)x->extra_data;
  // tl_smp Ip; 

  tl_smp cap_inv = 1.0/y->k_cap_inv->outlet->smps[iter];
  
  tl_smp params[6];
  params[0] = y->k_Up->outlet->smps[iter];
  params[1] = y->k_kp->outlet->smps[iter];
  params[2] = y->k_kvb->outlet->smps[iter];
  params[3] = y->k_kG1->outlet->smps[iter];
  params[4] = y->k_X->outlet->smps[iter];
  params[5] = y->k_mu->outlet->smps[iter];

  Ip = nl_triode_func(solver->inlets[0]->smps[iter], params);
  
  out = cap_inv*(Ip);
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
      triode_stages[i] = init_triode_stage(i);
      tl_push_UDS_node(solver->UDS_net, triode_stages[i]->node);
    }    

  // hook 'em up
  int j;
  for(i=0;i<stage_cnt;i++)
    for(j=0;j<stage_cnt;j++)
      {
	triode_stages[i]->node->data_in[j] = triode_stages[j]->node->data_out; 
	//	printf("connecting node %d out to node %d slot %d\n",j,i,j);
      }

  // init controls
  b_reset = init_ctl(TL_BANG_CTL);
  b_reset->name = name_new("reset");
  b_reset->bang_func = reset;
  install_onto_ctl_list(ctls, b_reset);


  // hook up to dac
  dac->inlets[0] = solver->outlets[0];


}

static tl_triode_stage *init_triode_stage(int which){

  tl_triode_stage *x = malloc(sizeof(tl_triode_stage));
  char buf[50];

  // init the node
  x->node = tl_init_UDS_node(triode, stage_cnt, 1);
  x->node->extra_data = (void *)x;
  x->which = which;


  // init the ctls
  x->k_Up = init_ctl(TL_LIN_CTL);
  sprintf(buf, "k_Up_%d",which);
  x->k_Up->name = name_new(buf);
  set_ctl_val(x->k_Up, 0.0);
  install_onto_ctl_list(ctls, x->k_Up);

  x->k_kp = init_ctl(TL_LIN_CTL);
  sprintf(buf, "k_kp_%d",which);
  x->k_kp->name = name_new(buf);
  set_ctl_val(x->k_kp, 0.0);
  install_onto_ctl_list(ctls, x->k_kp);

  x->k_kvb = init_ctl(TL_LIN_CTL);
  sprintf(buf, "k_kvb_%d",which);
  x->k_kvb->name = name_new(buf);
  set_ctl_val(x->k_kvb, 0.0);
  install_onto_ctl_list(ctls, x->k_kvb);

  x->k_kG1 = init_ctl(TL_LIN_CTL);
  sprintf(buf, "k_kG1_%d",which);
  x->k_kG1->name = name_new(buf);
  set_ctl_val(x->k_kG1, 0.0);
  install_onto_ctl_list(ctls, x->k_kG1);

  x->k_X = init_ctl(TL_LIN_CTL);
  sprintf(buf, "k_X_%d",which);
  x->k_X->name = name_new(buf);
  set_ctl_val(x->k_X, 0.0);
  install_onto_ctl_list(ctls, x->k_X);

  x->k_mu = init_ctl(TL_LIN_CTL);
  sprintf(buf, "k_mu_%d",which);
  x->k_mu->name = name_new(buf);
  set_ctl_val(x->k_mu, 0.0);
  install_onto_ctl_list(ctls, x->k_mu);

  x->k_cap_inv = init_ctl(TL_LIN_CTL);
  sprintf(buf, "k_cap_inv_%d",which);
  x->k_cap_inv->name = name_new(buf);
  set_ctl_val(x->k_cap_inv, 0.0);
  install_onto_ctl_list(ctls, x->k_cap_inv);




  return x;
}

tl_ctl *tl_reveal_ctls_triode(void){return ctls;}

void tl_kill_triode(tl_class *class_ptr){


  int i;
  for(i=0;i<stage_cnt;i++)
    free(triode_stages[i]);
  
  tl_kill_UDS_solver(solver); // destroys the nodes automatically
  tl_kill_dac(dac);
  tl_kill_adc(adc);


}

void tl_dsp_triode(int samples, void *mod_ptr){

  int s = samples;
  tl_dsp_adc(s, adc);
  tl_dsp_UDS_solver(s, solver);
  /* int i; */
  /* for(i=0;i<s;i++) */
  /*   printf("%f\n", solver->outlets[0]->smps[i]); */
  tl_dsp_dac(s, dac);

}


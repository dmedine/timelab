#include "tl_core.h"
#include "m_modules.h"
#include "math.h"
#include "stdio.h"

FILE *fp;
#define UP 1
#define DN 0

static tl_UDS_solver *solver;
static tl_dac *dac;
static tl_UDS_node *node_sqr;
static tl_UDS_node *node_tri;
static tl_ctl *k_freq, *k_skew, *k_tanh_g, *k_thresh, *k_reset;
static tl_smp inv_cap = -100000.0;
static tl_smp tri_thresh = .99;
static tl_smp tanh_gain = 50.0;
static tl_smp g_skew = .5;
static int slp_dir = UP;
static tl_smp g_freq = 480;

// globals to pass to mother class
int in_cnt = 0;
int out_cnt = 2;
void *ctls;

void reset_oscs(void){


  //printf("hello from reset_oscs\n");
  tl_reset_UDS_node(solver->UDS_net, 0.0);
  tl_reset_UDS_node(solver->UDS_net->next, 1.0);

}

tl_smp find_slope(tl_smp freq, tl_smp skew, int dir){

  tl_smp slp;
  if(skew>=1.0)skew = .99999;
  if(skew<=0.0)skew = .00001;
  if(dir==DN)slp = 1.0/(1.0-skew) * freq;
  if(dir==UP)slp = 1.0/skew * freq;
  return slp;

}

tl_smp tanh_clip(tl_smp x, tl_smp g){

  tl_smp out = tanh(x*g);
  //if(g==1.0)printf("%f\n", out);
  return out;

}

tl_smp tanh_clip_dash(tl_smp x, tl_smp g){

  return 1.0-tan(tan(x*g));

}

tl_smp tri(tl_UDS_node *x, int iter){

  tl_smp out;
  tl_smp freq, skew, tanh_g;

  // grab the appropriate control value for this iteration
  freq = k_freq->outlet->smps[iter];
  skew = k_skew->outlet->smps[iter];
  tanh_gain = k_tanh_g->outlet->smps[iter];
  tri_thresh = k_thresh->outlet->smps[iter];


  out = tanh_clip(*x->data_in[0], tanh_gain)*4*find_slope(freq, skew, slp_dir);
  return out;
}

tl_smp sqr(tl_UDS_node *x, int iter){

  tl_smp out = 0.0;

  //  current triangle state > threhold, time to go low
  if(*x->data_in[0]>*x->data_in[1])
    {
      slp_dir = DN;
      out = *x->data_in[0];
    }

  //  current triangle state < threhold, time to go high
  if(*x->data_in[0]<(-1**x->data_in[1]))
    { 
      out =  *x->data_in[0];
      slp_dir = UP;
    }
  x->state = tanh_clip(x->state, tanh_gain);

  return *x->data_in[2] * out;
}

void tl_init_tri_sqr(tl_arglist *args){

  tl_procession *procession; // needed for DAC
  // check for a procession in the args
  // this should never actually happen:
  if(args->argv[0]->type!=TL_PROCESSION) 
    {
      printf("error: tl_init_dyfunc_test : first init argument needs to be a valid procession pointer\n");
      return;
    }
  else procession = args->argv[0]->procession;

  solver = tl_init_UDS_solver(0, 
			      2, 
			      1);

  node_tri = tl_init_UDS_node(tri, 
			      2, 
			      //0,
			      1);

  tl_reset_UDS_node(node_tri, 0.0);

  node_sqr = tl_init_UDS_node(sqr, 
			      4, //    
			      //0,
			      1);
  
  tl_reset_UDS_node(node_sqr, 1.0);
  
  tl_push_UDS_node(solver->UDS_net, node_tri);
  tl_push_UDS_node(solver->UDS_net, node_sqr);  
  
  dac=tl_init_dac(procession, 2, 1); // this needs some more thought

  node_tri->data_in[0] = node_sqr->data_out;
  
  node_sqr->data_in[0] = node_tri->data_out;
  node_sqr->data_in[1] = &tri_thresh;
  node_sqr->data_in[2] = &inv_cap;
  node_sqr->data_in[3] = &tanh_gain;

  dac->inlets[0] = solver->outlets[0];
  dac->inlets[1] = solver->outlets[1];
  //fp = fopen("tri_sqr_out", "w");

  k_freq = init_ctl(TL_LIN_CTL);
  k_freq->name = name_new("freq");
  set_ctl_val(k_freq, 400);

  k_skew = init_ctl(TL_LIN_CTL);
  k_skew->name = name_new("skew");
  set_ctl_val(k_skew, .5);

  k_tanh_g = init_ctl(TL_LIN_CTL);
  k_tanh_g->name = name_new("tanh_g");
  set_ctl_val(k_tanh_g, 1);

  k_thresh = init_ctl(TL_LIN_CTL);
  k_thresh->name = name_new("thresh");
  set_ctl_val(k_thresh, tri_thresh);

  k_freq->next = k_skew; 
  k_skew->next = k_tanh_g;
  k_tanh_g->next = k_thresh;

  k_reset = init_ctl(TL_BANG_CTL);
  k_reset->name = name_new("reset_oscs");
  k_reset->bang_func = reset_oscs;
  k_thresh->next = k_reset;

}

tl_ctl *tl_reveal_ctls_tri_sqr(void){
  return k_freq;
}

void tl_kill_tri_sqr(tl_class *class_ptr){


  printf("killing tri_sqr...\n");
  tl_kill_UDS_solver(solver); // kills the nodes automatically
  tl_kill_dac(dac);

  //  fclose(fp);
}

void tl_dsp_tri_sqr(int samples, void *mod_ptr){

  tl_smp atten = 0.0;
  int samps = samples;
  //printf("in dsp_tri_sqr %d\n",samples);
  //  printf("%p\n",solver);
  tl_dsp_UDS_solver(samps, solver);
  int i;
  /* for(i=0;i<samps;i++) */
  /*   { */

  /*     fprintf(fp, "%f %f\n", solver->outlets[0]->smps[i], solver->outlets[1]->smps[i]); */
  /*   } */

  tl_dsp_dac(samps, dac);

}


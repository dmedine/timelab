#include "tl_core.h"
#include "m_modules.h"
#include "math.h"
#include "stdio.h"

FILE *fp;
#define UP 1
#define DN 0

static tl_UDS_solver *solver;
static tl_UDS_node *node_sqr;
static tl_UDS_node *node_tri;
static tl_ctl *value;
static tl_smp inv_cap = -100000.0;
static tl_smp tri_thresh = .99;
static tl_smp tanh_gain = 50.0;
static tl_smp g_skew = .5;
static int slp_dir = UP;
static tl_smp g_freq = 480;

// globals to pass to mother class
int in_cnt = 0;
int out_cnt = 4;
tl_ctl **ctls;

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
  out = tanh_clip(*x->data_in[0], tanh_gain)*4*find_slope(g_freq, g_skew, slp_dir);
  //out = tanh_clip(*x->data_in[0], 1.0)*4*440;
  //     printf("tri in %f out %f\n", *x->data_in[0], out);
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

  solver = tl_init_UDS_solver(0, 
			      2, 
			      1);

  node_tri = tl_init_UDS_node(tri, 
			      2, 
			      0,
			      1);

  tl_reset_UDS_node(node_tri, 0.0);

  node_sqr = tl_init_UDS_node(sqr, 
			      4, //    
			      0,
			      1);
  
  tl_reset_UDS_node(node_sqr, 1.0);
  
  tl_push_UDS_node(solver->UDS_net, node_tri);
  tl_push_UDS_node(solver->UDS_net, node_sqr);  
  
  tl_init_dac(2, 1); // this needs some more thought

  node_tri->data_in[0] = node_sqr->data_out;
  
  node_sqr->data_in[0] = node_tri->data_out;
  node_sqr->data_in[1] = &tri_thresh;
  node_sqr->data_in[2] = &inv_cap;
  node_sqr->data_in[3] = &tanh_gain;

  set_g_dac_in(0, solver->outlets[0]);
  set_g_dac_in(1, solver->outlets[1]);
  //  fp = fopen("tri_sqr_out", "w");


  value = init_ctl(TL_LIN_CTL);
  value->name = name_new("value");
  *ctls = value;


}

void tl_kill_tri_sqr(tl_class *class_ptr){

  tl_kill_UDS_solver(solver); // kills the nodes automatically
  tl_kill_dac();

  // fclose(fp);
}

void tl_dsp_tri_sqr(int samples, void *mod_ptr){

  tl_smp atten = 0.0;
  int samps = tl_get_block_len(), i;

  tl_dsp_UDS_solver(samps, solver);

  /* for(i=0;i<samps;i++) */
  /*   { */

  /*   printf("%f %f\n", solver->outlets[0]->smps[i], solver->outlets[1]->smps[i]); */
  /*   //    fprintf(fp, "%f %f\n", solver->outlets[0]->smps[i], solver->outlets[1]->smps[i]); */
  /*   } */
  /* scale_sig_vals(solver->outlets[0], &atten); */
  scale_sig_vals(solver->outlets[1], &atten);
  tl_dsp_dac(samps);

}

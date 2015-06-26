#include "tl_core.h"
#include "m_modules.h"
#include "math.h"
#include "stdio.h"

FILE *fp;

static tl_UDS_solver *solver;
static tl_UDS_node *node_sqr;
static tl_UDS_node *node_tri;
static tl_smp inv_cap = -10000000.0;
static tl_smp tri_thresh = .99;
static tl_smp tanh_gain = 1.0;

tl_smp tanh_clip(tl_smp x, tl_smp g){

  return tanh(x*g);

}

tl_smp tri(tl_UDS_node *x, int iter){

  tl_smp out;
  out = tanh_clip(*x->data_in[0], 1.0)*4*440;
  //     printf("tri in %f out %f\n", *x->data_in[0], out);
  return out;
}

tl_smp sqr(tl_UDS_node *x, int iter){

  tl_smp out = 0.0;

  //  current triangle state > threhold, time to go low
  if(*x->data_in[0]>*x->data_in[1])
    // tanh clip the (negative) reciprocal of the capacitance
    out = *x->data_in[0];

  //  current triangle state < threhold, time to go high
  if(*x->data_in[0]<(-1**x->data_in[1]))
        // tanh clip the reciprocal of the capacitance
    out =  *x->data_in[0];
  //printf("sqr in0 %f out %f", *x->data_in[0], out);
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
  
  tl_init_dac(1, 1); // this needs some more thought

  node_tri->data_in[0] = node_sqr->data_out;
  
  node_sqr->data_in[0] = node_tri->data_out;
  node_sqr->data_in[1] = &tri_thresh;
  node_sqr->data_in[2] = &inv_cap;
  node_sqr->data_in[3] = &tanh_gain;

  set_g_dac_in(0, solver->outlets[0]);
  //set_g_dac_in(1, solver->outlets[1]);
  fp = fopen("tri_sqr_out", "w");
}

void tl_kill_tri_sqr(tl_class *class_ptr){

  tl_kill_UDS_solver(solver); // kills the nodes automatically
  tl_kill_dac();

  fclose(fp);
}

void tl_dsp_tri_sqr(int samples, void *mod_ptr){

  tl_smp atten = .7;
  int samps = tl_get_block_len(), i;

  tl_dsp_UDS_solver(samps, solver);

  /* for(i=0;i<samps;i++) */
  /*   printf("%d %f\n", i, solver->outlets[0]->smps[i]); */
  /* for(i=0;i<samps;i++) */
  /*   printf("%d %f\n", i, solver->outlets[0]->smps[i]); */
  for(i=0;i<samps;i++) 
    //    fprintf(fp, "%f\n", solver->outlets[0]->smps[i]); 
    fprintf(fp, "%f %f\n", solver->outlets[0]->smps[i], solver->outlets[1]->smps[i]); 
  
  /* scale_sig_vals(solver->outlets[0], &atten); */
  /* scale_sig_vals(solver->outlets[1], &atten); */
  /* for(i=0;i<samps;i++) */
  /*   printf("%d %f\n", i, solver->outlets[0]->smps[i]); */
  tl_dsp_dac(samps);

}

#include "tl_core.h"
#include "m_modules.h"

static tl_UDS_solver *solver;
static tl_UDS_node *node_x;
static tl_UDS_node *node_y;

tl_smp x_dot(tl_UDS_node *x, int iter){

  tl_smp out;
  tl_smp freq = 440 * 2 * 3.14;// = x->ctls[0]->smps[iter] * 2pi;
  
  out = freq * *x->data_in[0];
  //printf("x_dot %f %f\n", *x->data_in[0], out);
  return out;
}

tl_smp y_dot(tl_UDS_node *x, int iter){

  tl_smp out;
  tl_smp freq = 440 * 2 * 3.14;// = x->ctls[0]->smps[iter] * 2pi;
  out = -1 * freq * *x->data_in[0];
  //printf("y_dot %f %f\n", *x->data_in[0], out);
  return out;
}

void tl_init_dyfunc_test(tl_arglist *args){

  solver = tl_init_UDS_solver(0, 
			      2, 
			      1);

  node_x = tl_init_UDS_node(x_dot, 
			    1, 
			    0,
			    1);

  tl_reset_UDS_node(node_x, 0.0);

  node_y = tl_init_UDS_node(y_dot, 
			    1, 
			    0,
			    1);

  tl_reset_UDS_node(node_y, -1.0);
  
  tl_push_UDS_node(solver->UDS_net, node_x);
  tl_push_UDS_node(solver->UDS_net, node_y);  
  
  tl_init_dac(2, 1); // this needs some more thought

  node_x->data_in[0] = node_y->data_out;
  node_y->data_in[0] = node_x->data_out;

  set_g_dac_in(0, solver->outlets[0]);
  set_g_dac_in(1, solver->outlets[1]);

}

void tl_kill_dyfunc_test(tl_class *class_ptr){

  tl_kill_UDS_solver(solver); // kills the nodes automatically
  tl_kill_dac();

}

void tl_dsp_dyfunc_test(int samples, void *mod_ptr){

  tl_smp atten = .7;
  int samps = tl_get_block_len(), i;

  tl_dsp_UDS_solver(samps, solver);

  /* for(i=0;i<samps;i++) */
  /*   printf("%d %f\n", i, solver->outlets[0]->smps[i]); */
  /* for(i=0;i<samps;i++) */
  /*   printf("%d %f\n", i, solver->outlets[0]->smps[i]); */
  
  scale_sig_vals(solver->outlets[0], &atten);
  scale_sig_vals(solver->outlets[1], &atten);
  /* for(i=0;i<samps;i++) */
  /*   printf("%d %f\n", i, solver->outlets[0]->smps[i]); */
  tl_dsp_dac(samps);

}

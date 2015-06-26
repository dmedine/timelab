#include "g_api.h"
#include "m_modules.h"

typedef struct _mlf_class{

  

  struct _item **o_sigs;
  struct _item **i_sigs;
  struct _item **empty_sigs;

  struct _item *resonance_sig;
  struct _item *cutoff_sig;

  int i_sig_count;
  int o_sig_count;
  double state[4];
  double k[4][4];
  double resonance;
  double cutoff;
  double gain;
  int h;
  double h_time;
  double half_h_time;
  int block_len;
  double scalar;

  //  t_runge_class *solver;

  //  t_runge_func_ptr diff;

}t_mlf_class;

t_mlf_class *mlf_init(int h);

void mlf_dsp_loop(int samples, void *class_ptr);
void kill_mlf_class(t_mlf_class *x);

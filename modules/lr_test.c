#include "tl_core.h"
#include "m_modules.h"

//tl_dac *dac;
tl_table *table_l;
tl_table *table_r;
tl_lookup *lookup_l;
tl_lookup *lookup_r;


void tl_init_lr_test(tl_arglist *args){

  tl_init_dac(2, 1); // this needs some more thought
  table_l = tl_init_table(10000,1);
  table_r = tl_init_table(10000,1);
  lookup_l = tl_init_lookup(1);
  lookup_r = tl_init_lookup(1);

  lookup_l->freq = 400;
  lookup_r->freq = 500;

  set_g_dac_in(0, table_l->outlets[0]);
  set_g_dac_in(1, table_r->outlets[0]);

  table_l->inlets[0] = lookup_l->outlets[0];
  table_r->inlets[0] = lookup_r->outlets[0];


}

void tl_kill_lr_test(tl_class *class_ptr){

  tl_kill_table(table_l);
  tl_kill_table(table_r);
  tl_kill_lookup(lookup_l);
  tl_kill_lookup(lookup_r);
  tl_kill_dac();

}

void tl_dsp_lr_test(int samples, void *mod_ptr){

  tl_smp atten = .6;
  int samps = tl_get_block_len();
  tl_dsp_lookup(samps, lookup_l);
  tl_dsp_lookup(samps, lookup_r);

  tl_dsp_table(samps, table_l);
  tl_dsp_table(samps, table_r);


  scale_sig_vals(table_l->outlets[0], &atten);
  scale_sig_vals(table_r->outlets[0], &atten);

  tl_dsp_dac(samps);

}

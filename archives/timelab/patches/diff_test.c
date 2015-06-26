#include "stdio.h"
#include "math.h"
#include "stdlib.h"
//#include "a_pa.h"
#include "g_api.h"
//#include "ui_main.h"
#include "m_modules.h"
#include "m_ode_prim.h"
#include "limits.h"

static t_lookup *lookup;
static t_table *table;
static t_diff *diff1, *diff2;

void setup_this(void){

   lookup = (t_lookup *)lookup_init(1);
   diff1 = (t_diff *)diff_init(1, 1);
   diff2 = (t_diff *)diff_init(1, 1);
   table = (t_table *)table_init(1000, 1);

   install_obj(&lookup->od);
   install_obj(&diff1->od);
   install_obj(&diff2->od);
   install_obj(&table->od);

}

void do_kill(void){

  kill_lookup(lookup);
  kill_diff(diff1);
  kill_diff(diff2);
  kill_table(table);

}


void dsp_chain(int samples,
	       t_tlsig **adc_sigs,
	       t_tlsig **dac_sigs){

  lookup->dsp_func(samples, lookup);

  table->i_sigs[0] = lookup->o_sigs[0];
  table->dsp_func(samples, table);

  diff1->i_sigs[0] = lookup->o_sigs[0];
  diff1->dsp_func(samples, diff1);
  diff2->i_sigs[0] = table->o_sigs[0];
  diff2->dsp_func(samples, diff2);


}

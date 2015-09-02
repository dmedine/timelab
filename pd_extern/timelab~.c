#include "m_pd.h"
#include "tl_core.h"
#include "m_modules.h"
#include "dlfcn.h"
#include "string.h"
#include "stdio.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#define MAX_TL_CTL 9999

static t_class *timelab_tilde_class;
t_sample foo;

static void variable_dsp_add(int sig_cnt){
  // add the dsp adds here...
}

typedef struct _timelab_tilde{

  t_object x_obj;
  t_sample f;

  tl_class *class;
  tl_procession *procession;

  // thanks, IOhannes, for how to do this
  // https://svn.code.sf.net/p/pure-data/svn/trunk/externals/iem/iemmatrix/src/mtx_mul~.c
  t_float **io;

  int in_cnt;
  int out_cnt;
  int ctl_cnt;
  int n_sigs;
  int block_len;
  
}t_timelab_tilde;

static t_int *timelab_tilde_perform(t_int *w){

  t_timelab_tilde *x = (t_timelab_tilde *)(w[1]);
  tl_audio_buff *ab;
  int i,j;
  int advance = 0;
  int n=(int)(w[2]); // get number of samples
  t_sample *pd_buff_ptr;
  t_float **io = x->io;


  process_ctl_list(x->procession->ctl_head, x->procession->lvl_stck);
  
  // write to global in bus for timelab (read by its adc)
  if(x->in_cnt>=1)
    {
      ab=x->procession->ab_in;
      for(i=0;i<x->in_cnt; i++)
  	{
  	  pd_buff_ptr = (t_sample *)io[i];
  	  for(j=0; j<n; j++)
  	    ab->buff[i*n+j] = pd_buff_ptr[j];
  	}
    }

  tl_process_dsp_list(n, x->procession->class_head);
  
  // write out to timelab's global out bus (read from its dac)
  if(x->out_cnt>0)
    {
      ab=x->procession->ab_out;
      for(i=0;i<x->out_cnt; i++)
  	{
  	  pd_buff_ptr = (t_sample *)io[i+x->in_cnt];
	  for(j=0;j<n;j++)
	    pd_buff_ptr[j] = (t_sample)ab->buff[i*n+j];
 	}
    }

  return w+3;
}

static void timelab_tilde_ctl(t_timelab_tilde *x, 
			      t_floatarg who,
			      t_floatarg what){


  int who_int = (int)who;
  tl_ctl *y = x->procession->ctl_head;
  int i;
  if(who_int<=x->ctl_cnt && who_int >0)  
    {
      for(i=0; i<who_int; i++)
	y=y->next;
      

      if(y->type == TL_LIN_CTL)
	y->val_is = what;
      if(y->type == TL_BANG_CTL);
      {
	//	printf("hello from timelab_tilde_ctl\n"); 
	y->bang_go = 1;
      }
    }


}
 
static void timelab_tilde_list_ctl(t_timelab_tilde *x){

  post("hello!");
  int i = 0;
  tl_ctl *y = x->procession->ctl_head;
  while(y!=NULL)
    {
      if(y->type==TL_HEAD_CTL)
	post("ctl: %d, name: %s, type: head", i++, y->name);
      if(y->type==TL_BANG_CTL)
	post("ctl: %d name: %s, type: bang", i++, y->name);
      if(y->type==TL_LIN_CTL)
	post("ctl: %d, name: %s, type: linear ctl", i++, y->name);
      y=y->next;
    }


}

static void timelab_tilde_dsp(t_timelab_tilde *x, t_signal **sp){


  int i;
  int advance = (x->in_cnt!=0)?0:1;

  for(i=0;i<x->n_sigs;i++)
    x->io[i] = sp[i+advance]->s_vec;

  dsp_add(timelab_tilde_perform, 
	  2,
	  x,
	  sp[0]->s_n);

}

static void *timelab_tilde_new(t_symbol *s, int argc, t_atom *argv){

  t_timelab_tilde *x = (t_timelab_tilde *)pd_new(timelab_tilde_class);

  int i, j;

  // todo: make this parametric
  int block_len = 64;

  // get the sample rate correct
  tl_set_samplerate((int)sys_getsr());

  // set the block length
  tl_set_block_len(block_len);
  x->block_len = block_len;  

  // instantiate a gateway to timelab's scheduler
  x->procession = init_procession();
 
  // initialize the empty signal 
  tl_kill_empty_sig(); // make sure it's dead
  tl_init_empty_sig();

  // load the requested module
  t_symbol *mod_arg = atom_getsymbolarg(0,argc,argv);
  // todo: generalize so that init arguments can be passed
  post("modname: %s\n", mod_arg->s_name);
  if(!(x->class = tl_load_module(x->procession, mod_arg->s_name)))
    {
      post("tl_load_module failed: invalid tl file:\n%s", mod_arg->s_name);
      goto end;
    }

  // determine the number of controls
  tl_ctl *y = x->procession->ctl_head;
  i = 0;
  while(y!=NULL)
    {
      y = y->next;
      i++;
    }
  x->ctl_cnt = i-1;

  
  x->in_cnt = x->class->in_cnt;
  x->out_cnt = x->class->out_cnt;
  x->n_sigs = x->in_cnt + x->out_cnt;

  x->io = (t_float **)getbytes(x->n_sigs * sizeof(t_float *));
  
  post("in_cnt %d\n", x->in_cnt);
  // start count from 1 because we get one signal in no matter what
  for(i=1;i<x->in_cnt;i++)
    inlet_new(&x->x_obj, &x->x_obj.ob_pd, &s_signal, &s_signal);

  post("out_cnt %d\n", x->out_cnt);
  for(i=0;i<x->out_cnt;i++)
    outlet_new(&x->x_obj, &s_signal);
  
  post("n_sigs %d\n", x->n_sigs);
  
  goto end;

 end:
  return(void *)x;

}

static void timelab_tilde_free(t_timelab_tilde *x){


  kill_procession(x->procession);

  freebytes(x->io, (x->n_sigs * sizeof(t_float *))); 

  
}

void timelab_tilde_setup(void){

  timelab_tilde_class = class_new(gensym("timelab~"),
				  (t_newmethod)timelab_tilde_new,
				  (t_method)timelab_tilde_free,
				  sizeof(t_timelab_tilde),
				  0,
				  A_GIMME,
				  0);

  CLASS_MAINSIGNALIN(timelab_tilde_class, t_timelab_tilde, f);

  class_addmethod(timelab_tilde_class,
  		  (t_method)timelab_tilde_dsp,
  		  gensym("dsp"),
  		  0);

  // method for reading in control data
  class_addmethod(timelab_tilde_class,
		  (t_method)timelab_tilde_ctl,
		  gensym("tl"),
		  A_DEFFLOAT,
		  A_DEFFLOAT,
		  0);


  class_addmethod(timelab_tilde_class,
		  (t_method)timelab_tilde_list_ctl,
		  gensym("tl_list_ctl"),
		  0);

}

/* timelab_pd~ is a pd port of the timelab synthesis engine */
/* written by David Medine */
/* released under the GPL */

#include "m_pd.h"
#include "g_api.h"
#include "m_modules.h"
#include "dlfcn.h"
#include "string.h"
#include "stdio.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

static t_class *timelab_pd_tilde_class;

typedef struct _timelab_pd_tilde{

  t_object x_obj;
  t_sample f;

  int loaded;

  t_tl_so *tl_so;

  t_ki_func init;
  t_ki_func kill;
  t_dsp_chain dsp;
  t_tlsig **in_sigs;
  t_tlsig **out_sigs;

  char *tl_name;

}t_timelab_pd_tilde;

static void timelab_pd_tilde_load_tl(t_timelab_pd_tilde *x, char *full_path){

  if(x->tl_so)kill_tl_so(x->tl_so);
  
  x->tl_so = init_tl_so(full_path);
  cpy_file_name_no_path(x->tl_name, full_path);

  x->tl_so->handle = dlopen(full_path, RTLD_LAZY | RTLD_GLOBAL);
  if(!x->tl_so->handle)
    printf("tl_so load ERROR:\n\t%s\n",dlerror());
  
  else
    {
      x->tl_so->kill = (t_ki_func)dlsym(x->tl_so->handle, "do_kill");
      if(!x->tl_so->kill)printf("not adding kill\n");
      //error
      
      else
  	printf("added %s kill\n", x->tl_name);
	  
      x->tl_so->init = (t_ki_func)dlsym(x->tl_so->handle, "setup_this");
      if(!x->tl_so->init)("not adding setup\n");
      //error
      
      else
  	{
  	  printf("initializing %s setup\n", x->tl_name);
  	  x->tl_so->init();
  	  printf("...done\n");
  	}
      
      x->dsp = (t_dsp_chain)dlsym(x->tl_so->handle, "dsp_chain");
      if(!x->dsp)("not adding dsp\n");
      //error
      
      else
  	printf("added %s dsp chain\n", x->tl_name);
	
    }

}

static void timelab_pd_tilde_ctl(t_timelab_pd_tilde *x, 
			  t_floatarg who, 
			  t_floatarg what){

  //post("tl : %f %f\n", who, what);

  push_duple(who, what);

}

static t_int *timelab_pd_tilde_perform(t_int *w){

  t_timelab_pd_tilde *x = (t_timelab_pd_tilde *)(w[1]);
  t_sample *in0 = (t_sample *)(w[2]);
  t_sample *in1 = (t_sample *)(w[3]);
  t_sample *out0 = (t_sample *)(w[4]);
  t_sample *out1 = (t_sample *)(w[5]);
  int n = (int)(w[6]);

  int i;

  //cpy the in samples
  for(i=0; i<n; i++)
    {
      x->in_sigs[0]->s_block[i] = in0[i];
      x->in_sigs[1]->s_block[i] = in1[i];
    }

  //run the dsp_chain
  x->dsp(n, x->in_sigs, x->out_sigs);
  //cpy the out signals
  for(i=0; i<n; i++)
    {
      out0[i] = (t_sample)x->out_sigs[0]->s_block[i];
      out1[i] = (t_sample)x->out_sigs[1]->s_block[i];
    }

  //parse ctl input
  process_ctl_input();
  return(w+7);

}

static void timelab_pd_tilde_list_ctls(t_timelab_pd_tilde *x){
  
  t_lin_ctl *y;
  int i;
  char *type_str = (char *)malloc(50);

  //post("%d", g_lin_ctl_reg->ref_cnt);
  for(i=0; i<MAX_LEN; i++)
    {
      if(g_lin_ctl_reg->ctl_ptr_arr[i]!=NULL)
	{
	  y=g_lin_ctl_reg->ctl_ptr_arr[i];
	    
	  if(y->type==CTL_T_BANG)strcpy(type_str, "CTL_T_BANG");
	  else if(y->type==CTL_T_TOGGLE)strcpy(type_str, "CTL_T_TOGGLE");
	  else if(y->type==CTL_T_LIN)strcpy(type_str, "CTL_T_LIN");
	  else type_str = "unknown type -- not good!";
	  
	  if(y->name)
	    post("ctl %s, registered %d of type %s", y->name, y->ref_num, type_str);
	  else
	    post("%d ctl of type %s", y->ref_num, type_str);
	}
    }
  free(type_str);

}

static void timelab_pd_tilde_dsp(t_timelab_pd_tilde *x, t_signal **sp){

  g_samplerate = sp[0]->s_sr;
  dsp_add(timelab_pd_tilde_perform, 6, 
	  x, 
	  sp[0]->s_vec, 
	  sp[1]->s_vec, 
	  sp[2]->s_vec, 
	  sp[3]->s_vec,
	  sp[0]->s_n);
	  
}

static void *timelab_pd_tilde_new(t_symbol *s, int argc, t_atom *argv){

  t_timelab_pd_tilde *x = (t_timelab_pd_tilde *)pd_new(timelab_pd_tilde_class);

  inlet_new(&x->x_obj, &x->x_obj.ob_pd, &s_signal, &s_signal);
  outlet_new(&x->x_obj, &s_signal);
  outlet_new(&x->x_obj, &s_signal);

  g_samplerate = sys_getsr();

  init_all_for_pd();

  x->loaded = 0;

  x->in_sigs = init_tlsigs(2, O_TYPE, 1);
  x->out_sigs = init_tlsigs(2, I_TYPE, 1);

  x->tl_name = (char *)malloc(sizeof(char) * 256);
  x->tl_so = NULL;
  timelab_pd_tilde_load_tl(x, "../patches/moog_filt.tl");

  
  return(void *)x;
}

static void timelab_pd_tilde_free(t_timelab_pd_tilde *x){



  kill_tlsigs(x->in_sigs, 2);
  kill_tlsigs(x->out_sigs, 2);


  x->tl_so->kill();
  if(x->tl_so)kill_tl_so(x->tl_so);

  if(x->tl_name)
    {
      free(x->tl_name);
      x->tl_name = NULL;
    }

  if(x->tl_name)
    {
      free(x->tl_name);
      x->tl_name = NULL;
    }

  //kill tl's ctl layer
  kill_all_for_pd();

}

void timelab_pd_tilde_setup(void){

  timelab_pd_tilde_class = class_new(gensym("timelab_pd~"),
				     (t_newmethod)timelab_pd_tilde_new,
				     (t_method)timelab_pd_tilde_free,
				     sizeof(t_timelab_pd_tilde),
				     0,
				     A_GIMME,
				     0);

  CLASS_MAINSIGNALIN(timelab_pd_tilde_class, t_timelab_pd_tilde, f);

  class_addmethod(timelab_pd_tilde_class, 
		  (t_method)timelab_pd_tilde_dsp, 
		  gensym("dsp"), 
		  0);

  class_addmethod(timelab_pd_tilde_class,
		  (t_method)timelab_pd_tilde_load_tl,
		  gensym("load_tl"),
		  A_GIMME,
		  0);

  class_addmethod(timelab_pd_tilde_class,
		  (t_method)timelab_pd_tilde_ctl,
		  gensym("tl"),
		  A_DEFFLOAT,
		  A_DEFFLOAT,
		  0);

  class_addmethod(timelab_pd_tilde_class,
		  (t_method)timelab_pd_tilde_list_ctls,
		  gensym("tl_list_ctls"),
		  0);


}

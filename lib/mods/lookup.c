#include "stdlib.h"
#include "math.h"
#include "tl_core.h"

    //****************//
    //     lookup     //
    //****************//

typedef struct _lookup{

  tl_kill_func kill_func;
  tl_dsp_func dsp_func;
  
  tl_sig **inlets;
  int in_cnt;
  tl_sig **outlets;
  int out_cnt;
  
  int up;
  int sr;
  int denom;
  
  tl_smp phase;
  tl_smp freq;
  tl_smp phase_inc;
  
}tl_lookup;

inline void tl_dsp_lookup(int samples, void *mod);
void *tl_init_lookup(tl_arglist *args);
void tl_kill_lookup(tl_class *mod);
  

inline void tl_dsp_lookup(int samples, void *mod){

  tl_lookup *x = (tl_lookup *)mod;
  int s = samples * x->up;
  int i;
  
  if(x->inlets[0]!=NULL)
    {
      for(i=0; i<s; i++)
	{
	  x->phase_inc = x->inlets[0]->smps[i]/(tl_smp)x->denom;
	  x->outlets[0]->smps[i] = x->phase;
	  x->phase += x->phase_inc;
	  if(x->phase >= 1.0)x->phase -= 1.0;
	}
    }
  else
    {
      x->phase_inc = x->freq/(tl_smp)x->denom;
      for(i=0; i<s; i++)
	{
	  x->outlets[0]->smps[i] = x->phase;
	  x->phase += x->phase_inc;
	  if(x->phase >= 1.0)x->phase -= 1.0;
	}
    }
}

void *tl_init_lookup(tl_arglist *args){

  if(args->argv[0]->type!=A_INT)
    {
      printf("sorry, invalid arglist for tl_lookup\n");
      return NULL;
    }

  int up = args->argv[0]->i_val;

  tl_lookup *x = (tl_lookup *)malloc(sizeof(tl_lookup));

  x->dsp_func = &tl_dsp_lookup;
  x->kill_func =  &tl_kill_lookup;

  x->outlets = init_sigs(1, TL_OUTLET, up);
  x->out_cnt = 1;

  x->inlets = init_sigs(1, TL_INLET, up);
  x->in_cnt = 1;

  x->phase = 0.0;
  x->freq = 0.0;
  x->phase_inc = 0.0;

  x->up = up;
  x->sr = tl_get_samplerate();
  x->denom = x->up*x->sr;

  //tl_lookup_cnt++;

  return (void*)x;

}

void tl_kill_lookup(tl_class *mod){

  tl_lookup *x = (tl_lookup *)mod;
  if(x!=NULL)
    {
      free(x);
      x=NULL;
      //tl_lookup_cnt--;

    }
  else printf("error: tl_kill_lookup: invalid class ptr\n");

}

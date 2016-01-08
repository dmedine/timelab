/* this file belongs to */
/* timelab-0.10 by David Medine */

// this source code defines the 
// signals in timelab

#include "tl_core.h"
#include "stdlib.h"

//***********************************
//***********************************

   //*******************//
   //       tl_sig      // 
   //*******************//

tl_sig *init_one_sig(int block_len, int up){

  int i;
  tl_sig *x;
 
  x = (tl_sig *) malloc(sizeof(tl_sig));

  x->smps = (tl_smp *)malloc(sizeof(tl_smp) * block_len* up);
  for(i=0; i<block_len*up; i++)
    x->smps[i] = 0.0;

  x->smp_cnt = block_len * up;
  x->type = TL_OUTLET; // we never need to initialize a single inlet
  x->up = up;

  return x;

}

tl_sig **init_sigs(int sig_cnt, tl_sig_t sig_type, int up){

  int i;
  tl_sig **x;

  if(up<1)
    {
      printf("ERROR: in init_sigs : argument up < 1\n");
      return;
    }

  x = (tl_sig **) malloc(sizeof(tl_sig *) * sig_cnt);

  if(sig_type == TL_OUTLET)
    for(i=0; i<sig_cnt; i++)
      x[i] = init_one_sig(tl_get_block_len(), up);
  else
    for(i=0; i<sig_cnt; i++)
      x[i]=NULL;
      
  return x;

}

void kill_one_sig(tl_sig *x){

  if(x->smps)
    {
      free(x->smps);
      x->smps = NULL;
    }

  if(x)
    {
      free(x);
      x = NULL;
    }
}


void kill_outlets(tl_sig **x, int sig_cnt){

  int i;
  if(x)
    {
      for(i=0; i<sig_cnt; i++)
	if(x[i]->type == TL_OUTLET)
	  kill_one_sig(x[i]);
      
      free(x);
      x=NULL;
    }
}

void kill_inlets(tl_sig **x){

  if(x!=NULL)
    {
      free(x);
      x = NULL;
    }

}

inline extern void set_sig_vals(tl_sig *x, tl_smp val){

  int i;
  for(i=0; i<x->smp_cnt; i++)
    x->smps[i] = val;

}

inline extern void scale_sig_vals(tl_sig *x, tl_smp *scalar){

  int i;

  //printf("scale...\n");
  for(i=0; i<x->smp_cnt; i++)
    {
      //  printf("before %d: %f %f\n", i, x->smps[i], *scalar);
      x->smps[i] *= *scalar;
      // printf("after %d: %f %f\n", i, x->smps[i], *scalar);
    }
}

inline extern void multiply_sigs(tl_sig *x, tl_sig *y){

  int i;
  if(x->smp_cnt != y->smp_cnt)
    {
      printf("ERROR: in multiply_sigs : count mismatch\n");
 
      return;
    }
  //printf("multiply...\n");
  for(i=0; i<x->smp_cnt; i++)
    {
      //printf("before %d: %f %f\n", i, x->smps[i], y->smps[i]);
      x->smps[i]*=y->smps[i];
      //printf("after %d: %f %f\n", i, x->smps[i], y->smps[i]);
    }
}

inline extern void divide_sigs(tl_sig *x, tl_sig *y){

  int i;
  if(x->smp_cnt != y->smp_cnt)
    {
      printf("ERROR: in divide_sigs : count mismatch\n");
 
      return;
    }
  for(i=0; i<x->smp_cnt; i++)
    x->smps[i]/=y->smps[i];
}

inline extern void add_sigs(tl_sig *x, tl_sig *y){

  int i;
  if(x->smp_cnt != y->smp_cnt)
    {
      printf("ERROR: in add_sigs : count mismatch\n");
 
      return;
    }
  for(i=0; i<x->smp_cnt; i++)
    x->smps[i]+=y->smps[i];
}

inline extern void subtract_sigs(tl_sig *x, tl_sig *y){

  int i;
  if(x->smp_cnt != y->smp_cnt)
    {
      printf("ERROR: in subtract_sigs : count mismatch\n");
 
      return;
    }
  for(i=0; i<x->smp_cnt; i++)
    x->smps[i]-=y->smps[i];
}

inline extern void zero_out_sig(tl_sig *x){

  int i;
  for(i=0; i<x->smp_cnt; i++)
    x->smps[i] = 0.0;

}

void tl_init_empty_sig(void){

  tl_g_empty_sig = init_one_sig(tl_get_block_len(),1);

}

tl_sig *tl_get_empty_sig(void){


  if(tl_g_empty_sig !=NULL)
    return tl_g_empty_sig;
  else 
    {
      printf("error: tl_get_empty_sig: tl_g_empty_sig not initialized\n");
      return NULL;
    }

}

void tl_kill_empty_sig(void){

  if(tl_g_empty_sig!=NULL)
    {
      free(tl_g_empty_sig);
      tl_g_empty_sig = NULL;
    }
  else printf("warning: tl_kill_empty_sig: tl_g_empty_sig does not exist\n");

}

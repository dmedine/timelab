#include "m_modules.h"
#include "stdlib.h"
#include "math.h"

    //*************//
    //     DAC     //
    //*************//


inline void tl_dsp_dac(int samples, void *mod){

  int i,s, j;
  tl_dac *x = tl_get_dac();
  s=samples;
  for(i=0; i<x->in_cnt; i++)
    {
      for(j=0; j<s; j++)
	{
	  x->ab->buff[x->ab->buff_pos_w++] = 
	    x->inlets[i]->smps[j];

	  if(x->ab->buff_pos_w >= x->ab->buff_len)
	    x->ab->buff_pos_w -= x->ab->buff_len;
	}

    }

}


void tl_init_dac(int in_cnt, int up){

  printf("creating dac...\n");

  // think more about this policy
  // later ...
  // one idea is to have a module called dac_bus
  // that simply looks like a [dac~] in Pd and directs
  // its output to 'the' tl_dac
  /* if(tl_dac_cnt > 0) */
  /*   { */
  /*     printf("error: tl_dac_init: only one dac allowed\n"); */
  /*     return(NULL); */
  /*   } */


  tl_g_dac = (tl_dac *)malloc(sizeof(tl_dac));

  tl_g_dac->dsp_func = &tl_dsp_dac;
  tl_g_dac->kill_func = &tl_kill_dac;

  tl_g_dac->inlets = init_sigs(in_cnt, TL_INLET, 1);
  tl_g_dac->in_cnt = in_cnt;

  tl_g_dac->sr = tl_get_samplerate();

  //tl_dac_cnt++;
  
  // again, we are presuming only one dac at a time ...
  set_g_out_chann_cnt(in_cnt);
  tl_g_dac->ab = init_audio_buff(in_cnt);
  set_g_audio_buff_out(tl_g_dac->ab);


}

void tl_kill_dac(void){
  
  
  if(tl_g_dac!=NULL)
    {
      if(tl_g_dac->ab!=NULL)
	kill_audio_buff(tl_g_dac->ab);
      free(tl_g_dac);
      tl_g_dac = NULL;
      //tl_dac_cnt--;
    }
  else printf("warning: tl_kill_dac: tl_g_dac does not (yet) exist\n");
}

void set_g_dac_in(int in, tl_sig *x){

  if(in< tl_get_dac()->in_cnt && in >=0)
    if(x!=NULL)
      tl_get_dac()->inlets[in]=x;
    else printf("error: set_g_dac_in: invalid sig ptr\n");
  else printf("error: set_g_dac_in: invlaid in ref\n");

}

tl_dac *tl_get_dac(void){

  if(tl_g_dac != NULL)
    return tl_g_dac;
  else
    {
      printf("error: tl_get_dac: tl_g_dac not initialized\n");
      return NULL;
    }
}

/*     //\***************\// */
/*     //     table     // */
/*     //\***************\// */

/* inline void tl_dsp_table(int samples, void *mod){ */
  
/*   int s, i; */
/*   tl_table *x = (tl_table *)mod; */

/*   s = x->up * samples; */
/*   for(i=0; i<s; i++) */
/*     { */
/*       x->outlets[0]->smps[i] = (tl_smp) */
/* 	x->table_array[(int)(x->inlets[0]->smps[i] * x->grain)]; */
/*     } */

/* } */

/* void *tl_init_table(tl_class *y){ */

/*   tl_table *x = (tl_table *)malloc(sizeof(tl_table)); */

/*   x->kill_func = &tl_kill_table; */
/*   x->dsp_func = &tl_dsp_table; */

/*   x->in_cnt = 1; */
/*   x->out_cnt = 1; */

/*   x->inlets = init_sigs(x->in_cnt, TL_INLET, 1); */
/*   x->outlets = init_sigs(x->out_cnt, TL_OUTLET, 1); */

/*   x->up = up; */
/*   x->grain = table_len; */
/*   create_table_array(x); */
  
/*   x->sr = tl_get_samplerate(); */

/*   //tl_table_cnt++; */

/*   return (void *)x; */
/* } */

/* void create_table_array(tl_table *x){ */

/*   int i; */
/*   tl_smp inc, val; */
/*   inc = 1.0/x->grain; */
/*   val = 0.0; */

/*   x->table_array = (tl_smp *)malloc(sizeof(tl_smp)*x->grain); */
/*   for(i=0; i<x->grain; i++) */
/*     { */
/*       x->table_array[i] = sin(2*PI * val); */
/*       val +=inc; */
/*       //printf("table array %d: %f\n", i, x->table_array[i]); */
/*     } */

/* } */

/* void tl_kill_table(void *mod){ */

/*   tl_table *x = (tl_table *)mod; */

/*   if(x!=NULL) */
/*     { */
/*       if(x->table_array != NULL) */
/* 	{ */
/* 	  free(x->table_array); */
/* 	  x->table_array = NULL; */
/* 	} */
/*       free(x); */
/*       x=NULL; */
/*       //tl_lookup_cnt--; */
/*     } */
/*   else printf("error: tl_kill_table: invalid class ptr\n"); */
/* } */




/*     //\**********************\// */
/*     //     housekeeping     // */
/*     //\**********************\// */

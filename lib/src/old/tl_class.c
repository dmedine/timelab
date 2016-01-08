/* this file belongs to */
/* timelab-0.10 by David Medine */

// this source code defines the 
// class structure in timelab

#include "tl_core.h"
#include "stdlib.h"


tl_dsp_list *init_dsp_list(void){

  tl_dsp_list *x = (tl_dsp_list *)malloc(sizeof(tl_dsp_list));
  return x;

}

tl_init_list *init_init_list(void){

  tl_init_list *x = (tl_init_list *)malloc(sizeof(tl_init_list));
  return x;

}

tl_kill_list *init_kill_list(void){

  tl_kill_list *x = (tl_kill_list *)malloc(sizeof(tl_kill_list));
  return x;

}

tl_dsp_list *get_g_dsp_list(void){

  if(tl_g_dsp_list!=NULL)
    return tl_g_dsp_list;
  else
    {
      printf("error: get_g_dsp_list: tl_g_dsp_list not initialized\n");
      return NULL;
    }
}

tl_init_list *get_g_init_list(void){

  if(tl_g_init_list!=NULL)
    return tl_g_init_list;
  else
    {
      printf("error: get_g_init_list: tl_g_init_list not initialized\n");
      return NULL;
    }
}

tl_kill_list *get_g_kill_list(void){

  if(tl_g_kill_list!=NULL)
    return tl_g_kill_list;
  else
    {
      printf("error: get_g_kill_list: tl_g_kill_list not initialized\n");
      return NULL;
    }
}

void set_g_dsp_list(tl_dsp_list *x){

  if(x!=NULL)
    tl_g_dsp_list = x;
  else
    printf("error: set_g_dsp_list: invalid list ptr\n");

}

void set_g_init_list(tl_init_list *x){

  if(x!=NULL)
    tl_g_init_list = x;
  else
    printf("error: set_g_init_list: invalid list ptr\n");

}

void set_g_kill_list(tl_kill_list *x){

  if(x!=NULL)
    tl_g_kill_list = x;
  else
    printf("error: set_g_kill_list: invalid list ptr\n");

}

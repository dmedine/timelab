/* this file belongs to */
/* timelab-0.10 by David Medine */

// this source code defines the 
// kill structure in timelab

#include "tl_core.h"
#include "stdlib.h"

tl_kill_array *get_g_kill_array(void){

  if(tl_g_kill_array!=NULL)
    return tl_g_kill_array;x
  else
    {
      printf("error: get_g_kill_array: tl_g_kill_array not initialized\n");
      return 0;
    }
}

void set_g_kill_array(tl_kill_array *x){

  if(x!=NULL)
    tl_g_kill_array = x;
  else
      printf("error: set_g_kill_array: invalid array\n");

}

tl_kill_array *init_kill_array(void){

  int i;

  tl_kill_array *x = (tl_kill_array *)malloc(sizeof(tl_kill_array));

  for(i=0; i<MAX_KILL; i++)
    {
      x->funcs[i] = NULL;
      x->classes[i] = NULL;
    }
  x->cnt = 0;

  return x;

}

void do_kill_array(tl_kill_array *x){

  int i;
  if(x!=NULL)
    for(i=0; i<x->cnt; i++)
      if(x->funcs[i]!=NULL)
	if(x->class[i]!=NULL)
	  x->funcs[i](x->classes[i]);
	else
	  printf("error: do_kill_array: invalid class ptr %d\n", i);
      else
	printf("error: do_kill_array: invalid tl_kill_func ptr %d\n", i);


  else
    printf("error: do_kill_array: invalid tl_kill_array\n");

}

void install_kill_func(tl_kill_array *x, void *y){

  tl_pclass *z;

  if(x!=NULL);
  if(y!=NULL);
   

}

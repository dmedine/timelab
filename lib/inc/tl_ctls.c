/* this file belongs to */
/* timelab-0.10 by David Medine */

// this source code defines the 
// controlss in timelab

#include "tl_ctls.h"
#include "tl_ctls.h"
#include "stdlib.h"

//***********************************
//***********************************

   //*******************//
   //      tl_ctl       // 
   //*******************//

tl_ctl *init_ctl(tl_ctl_t type){

  tl_ctl *x;
  x = (tl_ctl *) malloc(sizeof(tl_ctl));

  x->type = type;
  // all other fields must be handled by the 
  // client -- see tl_api.h for more details

  x->name = NULL;
  x->bang_func = NULL;
  x->was = 0.0;
  x->will_be = 0.0;
  x->outlet = NULL;

  x->next = NULL;
  x->top = 0;


}

void kill_ctl(tl_ctl *x){
  
  if(x)
    {
      free(x);
      x = NULL;
    }

}

void set_ctl_val(tl_ctl *x, tl_smp val){

  int i;
  if(x->type!=TL_LIN_CTL)
    {
      printf("ERROR: in set_ctl_val : type is not TL_LIN_CTL\n");
      return;
    }

  if(!x->outlet)
    {
      printf("ERROR: in set_ctl_val : no outlet available\n");
      return;
    }

  for(i=0; i<x->outlet->smp_cnt; i++)
    x->outlet->smps[i] = val;
  x->was = val;

}

void interpolate_ctl_val(tl_ctl *lvl_stck, tl_ctl *x, tl_smp new_val){

  tl_smp diff;
  tl_smp inc;
  tl_smp val;
  int i;

  if(!x->outlet)
    {
      printf("ERROR: in interpolate_ctl_val : no outlet available\n");
      return;
    }

  diff = new_val - x->was;
  val = x->was;
  inc = diff/x->outlet->smp_cnt;

  for(i=0; i<x->outlet->smp_cnt; i++)
    {
      x->outlet->smps[i] = val;
      val += inc;
    }

  x->will_be = new_val; // we must set all the values
  // in the outlet buffer to this value on the next tick

  // so, we push it onto this stack
  // and make sure to deal with it in the dsp loop
  // the client must provide this stack for us
  push_ctl_stck(lvl_stck, x);

}

// TODO:
void process_ctl_stck(tl_ctl *stck){

}

void push_ctl_stck(tl_ctl *stck, tl_ctl *y){

  tl_ctl *x = stck;
  
  while(x->next!=NULL) x=x->next;
  x->next = y;
  stck->top++;
    
}

tl_ctl *pop_ctl_stck(tl_ctl *stck){

  tl_ctl *x = stck;
  tl_ctl *y;
  if(stck->top<=0)
    {
      printf("ERROR: pop_ctl_stck : stack underflow\n");
      return NULL;
    }

  while(x->next->next!=NULL) x=x->next;
  y=x->next;
  
  x->next = NULL;
  stck->top--;
  
  return y;  

}

void flush_ctl_stck(tl_ctl *stck){

  stck->top = 0;

} 

void kill_ctl_stck(tl_ctl *stck){

  tl_ctl *x = stck;
  while(x->next!=NULL)
    kill_ctl(pop_ctl_stck(x));
  if(stck)
    {
      free(stck);
      stck=NULL;
    }

}

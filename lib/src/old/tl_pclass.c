/* this file belongs to */
/* timelab-0.10 by David Medine */

// this source code defines the 
// parent classes in timelab

#include "tl_core.h"
#include "stdlib.h"

//***********************************
//***********************************

   //*******************//
   //     tl_pclass     // 
   //*******************//

tl_pclass *init_pclass(void){

  tl_pclass *x;
  x = (tl_pclass *)malloc(sizeof(tl_pclass));

  x->dsp_func = NULL;
  //x->init_func = NULL;
  x->kill_func = NULL;
  x->inlets = NULL;
  x->inlet_cnt = 0;
  x->outlets = NULL;
  x->outlet_cnt = 0;
  x->cclass_ptr = NULL;
  x->name = NULL;
  x->next = NULL;

}

void kill_pclass(tl_pclass *x){
  
  if(x->outlets!=NULL)
    kill_sigs(x->outlets, x->outlet_cnt);

  // inlets don't need to be freed

 if(x->kill_func)   
   x->kill_func(x->cclass_ptr);
 if(x->cclass_ptr)
   {
     if(x->cclass_ptr->kill_func)
       x->cclass_ptr->kill_func(x->cclass_ptr);
     else
       {
	 free(x->cclass_ptr);
	 x->cclass_ptr = NULL;
       }
   }

  if(x->name)
    {
      free(x->name);
      x->name = NULL;
    }

}

void install_class(void *head, tl_pclass *x){

  tl_pclass *y = head;
  while(y->next!=NULL)y=y->next;
  y->next = x;

}

void clear_class_list(tl_pclass *head){

  tl_pclass *dummy;
  tl_pclass *x = head;
  while(x->next!=NULL)
    {
      dummy = x;
      if(dummy->next!=NULL)
	x=dummy->next;
      dummy = NULL;
    }
}

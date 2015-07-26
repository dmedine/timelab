/* this file belongs to */
/* timelab-0.10 by David Medine */
// this source code defines the
// class structure in timelab

#include "tl_core.h"
#include "stdlib.h"


tl_class *init_class(void){

tl_class *x = (tl_class *)malloc(sizeof(tl_class));
 x->dsp_func = NULL;
 x->kill_func = NULL;
 x->init_func = NULL;
 x->args = NULL;
 x->mod = NULL;
 x->mod_ctls = NULL;
 x->name = NULL;
 x->in_cnt = 0;
 x->out_cnt = 0;
 x->next=NULL;
 return x;
}

//void kill_class_list(tl_class_list *x){
void kill_class(tl_class *x){
  if(x!=NULL)
    {
      //printf("killing class %s\n", (char *)x->name);
      free(x);
      x = NULL;
    }
}

//tl_class_list *get_g_class_list(void){
tl_class *get_g_class_head(void){
  if(tl_g_class_head!=NULL)
    return tl_g_class_head;
  else
    {
      printf("error: get_g_class_head: tl_g_class_head not initialized\n");
      return NULL;
    }
}

//void set_g_class_head(tl_class_head *x){
void set_g_class_head(tl_class *x){
  if(x!=NULL)
    tl_g_class_head = x;
  else
    printf("error: set_g_class_head: invalid list ptr\n");
}

void tl_install_class(tl_class *x, tl_class *y){

  while(x->next!=NULL)
    x=x->next;
  x->next = y;
  y->init_func(y->args);

  //printf("installing class %s\n", y->name);
}

inline void tl_process_dsp_list(int samples, tl_class *x){

  //printf("processing dsp list\n");
  if(x!=NULL)
    {
      x=x->next;
      while(x!=NULL)
	{
	  //if(x->dsp_func!=NULL)
	  x->dsp_func(samples,x->mod);
	  x=x->next;
	}
      //else printf("error: process_dsp_list: null pr to dsp_func\n");
    }
  else printf("error: process_dsp_list: dsp list is null\n");
}

//void tl_process_kill_list(tl_class_list *x){
void tl_process_kill_list(tl_class *x){
  tl_class *dummy = x;
  tl_class *y = x;
  if(x!=NULL)
    {
      x = x->next; // the class list head is empty
      while(x!=NULL) // proceed as long as we have classes to kill
	{

	  // kill the module with its special kill function
	  x->kill_func(x->mod);
	  // save the class
	  dummy = x;
	  // progress to the next class
	  x=x->next;
	  // kill the dead class
	  kill_class (dummy);

	}
      // kill the class head
      kill_class(y);
    }
  else printf("error: process_kill_list: kill list is null\n");

}


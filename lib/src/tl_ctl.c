/* this file belongs to */
/* timelab-0.10 by David Medine */

// this source code defines the 
// control sturctures in timelab

#include "tl_core.h"
#include "stdlib.h"
#include "pthread.h"



//***********//
//  tl_ctl   // 
//**********//

pthread_mutex_t ctl_lock = PTHREAD_MUTEX_INITIALIZER;



tl_ctl *init_ctl(int type){

  tl_ctl *x;
  x = (tl_ctl *) malloc(sizeof(tl_ctl));

  x->type = type;
  // all other fields must be handled by the
  // client -- see tl_api.h for more details

  x->name = NULL;
  x->bang_func = NULL;
  x->ctl_kr = NULL;
  x->ctl_inlet = NULL;
  x->val_was = 0.0;
  x->val_is = 0.0;
  x->outlet = NULL;
  x->is_verbose = 0;

  if(x->type == TL_LIN_CTL)
    x->outlet = init_one_sig(tl_get_block_len(), 1);

  x->next = NULL;

  if(x->type == TL_HEAD_CTL)
    x->top = 0;
  else x->top = -1;


  return x;
}

void kill_ctl(tl_ctl *x){
  
  if(x)
    {
      if(x->outlet)
	kill_one_sig(x->outlet);
      free(x);
      x = NULL;
    }

}

// set the ctl input at control rate
void set_ctl_kr(tl_ctl *x, tl_smp *val){

  if(x->type == TL_LIN_CTL)
    x->ctl_kr = val;
  else
    printf("could not set_ctl_kr: wrong ctl type\n");

}

void set_ctl_bang_go(tl_ctl *x, int *bang_go){

  if(x->type == TL_BANG_CTL)
    x->bang_go = bang_go;
  else
    printf("could not set_ctl_bang_go: wron ctl type\n");

}

void set_ctl_bang_data(tl_ctl *x, void *data){

  if(x->type == TL_BANG_CTL)
    x->bang_data = data;
  else
    printf("could not set_ctl_bang_data: wron ctl type\n");


}

// utility -- not a necessary function
// but included for the sake of constancy
// NOTE bad nomeclature here
void set_ctl_val(tl_ctl *x, tl_smp val){

  int i;
  if(x->type!=TL_LIN_CTL)
    {
      printf("ERROR: in set_ctl_val : type is not TL_LIN_CTL\n");
      return;
    }

  if(x->outlet == NULL)
    {
      printf("ERROR: in set_ctl_val : no outlet available\n");
      return;
    }

  set_sig_vals(x->outlet, val);
  x->val_was = val;

}

inline void interpolate_ctl_val(tl_ctl *x){

  tl_smp diff;
  tl_smp inc;
  tl_smp val;
  int i;

/* #ifdef TESTING */
  printf("testing: enter interpolate_ctl_val\n");
/* #endif // TESTING */

  if(x->type==TL_LIN_CTL)
    {
      if(!x->outlet)
	{
	  printf("ERROR: in interpolate_ctl_val : no outlet available\n");
	  return;
	}
      x->val_is = *x->ctl_kr;
      diff = x->val_is - x->val_was;
      val = x->val_was;
      inc = diff/x->outlet->smp_cnt;
      
      if(x->is_verbose==0)
	for(i=0; i<x->outlet->smp_cnt; i++)
	  {
	    x->outlet->smps[i] = val;
	    val += inc;
	    
	  }
      else
	for(i=0; i<x->outlet->smp_cnt; i++)
	  {
	    printf("ctl %s[%d} %f\n", x->name, i, val);
	    x->outlet->smps[i] = val;
	    val += inc;
	    
	  }
      
      x->val_was = x->val_is; // we must set all the values
      // in the outlet buffer to this value on the next tick
      // so, we push it onto this 'level off' stack
      push_lvl_stck(get_g_lvl_stck(), x);
    }
}

inline void process_ctl_list(tl_ctl *head){

  tl_ctl *x = (tl_ctl *)head;
  //printf("processing control\n");
  pthread_mutex_lock(&ctl_lock);
  
  process_lvl_stck(get_g_lvl_stck());
  while(x->next!=NULL) 
    {
      x=x->next;
      if(x->type == TL_LIN_CTL)
	{
	  if(*x->ctl_kr != x->val_was)
	    interpolate_ctl_val(x);
	}	 
      else if(x->type == TL_BANG_CTL)
	if(*x->bang_go == 1)
	  {
	    x->bang_func(x->bang_data);
	    *x->bang_go = 0;
	  }
    }

  pthread_mutex_unlock(&ctl_lock);

}

void install_onto_ctl_list(tl_ctl *head, tl_ctl *x){
  
  tl_ctl *y = head;
  while(y->next!=NULL)y=y->next;
  y->next = x;

}

void set_g_ctl_head(tl_ctl *x){

  if(x!=NULL)
    tl_g_ctl_head = x;
  else
    printf("error: set_g_ctl_head: argument is null\n");

}

tl_ctl *get_g_ctl_head(void){

  if(tl_g_ctl_head != NULL)
    return tl_g_ctl_head;
  else
    { 
      printf("error get_g_ctl_head: head list is null\n");
      return(NULL);
    }
}

void tl_kill_ctl_list(tl_ctl *head){

  tl_ctl *x,*dummy;
  if(head!=NULL)
    {
      x = head;
      while(x->next!=NULL)
	{
	  dummy = x;
	  x=dummy->next;
	  if(dummy!=NULL)
	    {
	      free(dummy);
	      dummy = NULL;
	    }
	}
    }
  else printf("warning: tl_kil_ctl_list: head is invalid\n");
}

tl_lvl_stck *init_lvl_stck(void){

  tl_lvl_stck *x = (tl_lvl_stck *) malloc(sizeof(tl_lvl_stck));
  x->top = -1;

  return x;
}

void kill_g_lvl_stck(tl_lvl_stck *x){

  if(x!=NULL)
    {
      free(x);
      x = NULL;
    }

}

inline void push_lvl_stck(tl_lvl_stck *x, tl_ctl *y){

  if(x!=NULL)
    {
      if(++x->top<0)
	{
	  printf("error in push_g_lvl_stck: stack underflow\n");
	  return;
	}
      

      if(x->top>=MAX_CTL)
	{
	  printf("error in push_g_lvl_stck: stack overflow\n");
	  return;
	}
 
      x->ctls[x->top] = y;
    }
  else
    {
      printf("error in pus_g_lvl_stk: tl_g_lvl_stck not initialized\n");
      return;
    }

}

inline tl_ctl *pop_lvl_stck(tl_lvl_stck *x){

  if(x->top<0)
    {
      printf("error in pop_g_lvl_stck: stack underflow\n");
      return NULL;
    }
  else
    return(x->ctls[x->top--]);

}

void flush_lvl_stck(tl_lvl_stck *x){

  x->top = -1;

}

inline void process_lvl_stck(tl_lvl_stck *x){

  tl_ctl *y;
  if(tl_g_lvl_stck!=NULL)
    {
      while(tl_g_lvl_stck->top>=0)
	{
	  y = pop_lvl_stck(x);
	  set_sig_vals(y->outlet, y->val_is);
	}
    }
  else
    printf("error in g_lvl_stck: tl_g_lvl_stck not initialized\n");

}

void set_g_lvl_stck(tl_lvl_stck *x){

  tl_g_lvl_stck = x;

}


inline tl_lvl_stck *get_g_lvl_stck(void){

  if(tl_g_lvl_stck != NULL)
    return(tl_g_lvl_stck);
  else
    {
      printf("error get_g_lvl_stck: tl_g_lvl_stck not set");
      return (NULL);
    }
}

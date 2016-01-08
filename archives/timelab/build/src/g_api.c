#include "g_api.h"
#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "dlfcn.h"
#include "a_pa.h"
#include "m_modules.h"

FILE *fp;



//this is lifted from Miller -- his comments on this value from 'm_sched.c' 
//(pd-0.43-4) are quoted:
/* LATER consider making this variable.  It's now the LCM of all sample
    rates we expect to see: 32000, 44100, 48000, 88200, 96000. */
#define TIMEUNIT (32.*441000.)
long counter = 0;
int block_counter = 0;
int stream_on = 0;

t_tlsmp *a_conversion_buff;

pthread_mutex_t g_duple_stack_lock = PTHREAD_MUTEX_INITIALIZER; 
pthread_mutex_t dac_lock = PTHREAD_MUTEX_INITIALIZER; 
pthread_mutex_t g_sys_quit_lock = PTHREAD_MUTEX_INITIALIZER; 
pthread_mutex_t g_circ_buff_lock = PTHREAD_MUTEX_INITIALIZER; 

t_tlsmp a_time_per_dsp_tick;


/*********signals**********/
t_tlsig **init_tlsigs(int sig_cnt, int type, int up){
  
  int i, j;
  t_tlsig **x = (t_tlsig **)malloc(sig_cnt * sizeof(t_tlsig *));

  if(type==O_TYPE)
    {
      for(i=0; i<sig_cnt; i++)
	{
	  x[i] = (t_tlsig *)malloc(sizeof(t_tlsig));
	  x[i]->type = type;
	  x[i]->s_block = (t_tlsmp *)malloc(sizeof(t_tlsmp) * 
					     g_dsp_block_len * 
					     up);
	  for(j=0; j<(g_dsp_block_len * up); j++)
	    x[i]->s_block[j] = (t_tlsmp)0.0;
	  
	  x[i]->up = up;
	  x[i]->sample_cnt = g_dsp_block_len * up;

	}
    }
  else
    for(i=0; i<sig_cnt; i++)
      x[i]=NULL;

  return (x);

}

t_tlsig *init_one_tlsig(int type, int up){

  int j;
  t_tlsig *x;
  if(type==O_TYPE)
    {
      x = (t_tlsig *)malloc(sizeof(t_tlsig));
      
      x->s_block = (t_tlsmp *)malloc(sizeof(t_tlsmp) * 
				      g_dsp_block_len * 
				      up);
      
      for(j=0; j<(g_dsp_block_len * up); j++)
	x->s_block[j] = (t_tlsmp)0.0;
      
      x->up = up;
      x->sample_cnt = g_dsp_block_len * up;

    }
  else
    x=NULL;    

  return(x);

}

inline void set_sig_val(t_tlsig *x, t_tlsmp val){

  int i;
  int lim = x->up * g_dsp_block_len;
  for(i=0; i<lim; i++)
    x->s_block[i] = val;

}

inline void scale_sig_vals(t_tlsig *x, t_tlsmp val){

  int i;
  int lim = x->up * g_dsp_block_len;
  for(i=0; i<lim; i++)
    x->s_block[i] *= val;


}

inline void multiply_sigs(t_tlsig *x, t_tlsig *y){

  int i, lim;

  if(x->up <= y->up)
    {
      lim = x->up * g_dsp_block_len;
      for(i=0; i<lim; i++)
	x->s_block[i] *= y->s_block[i];
    } 

}


inline void divide_sigs(t_tlsig *x, t_tlsig *y){

  int i, lim;

  if(x->up <= y->up)
    {
      lim = x->up * g_dsp_block_len;
      for(i=0; i<lim; i++)
	x->s_block[i] /= y->s_block[i];
    } 

}


inline void add_sigs(t_tlsig *x, t_tlsig *y){

  int i, lim;

  if(x->up <= y->up)
    {
      lim = x->up * g_dsp_block_len;
      for(i=0; i<lim; i++)
	x->s_block[i] += y->s_block[i];
    } 

}


inline void subtract_sigs(t_tlsig *x, t_tlsig *y){

  int i, lim;

  if(x->up <= y->up)
    {
      lim = x->up * g_dsp_block_len;
      for(i=0; i<lim; i++)
	x->s_block[i] -= y->s_block[i];
    } 

}

inline void cpy_sigs(t_tlsig *x, t_tlsig *y){

  int i, lim;

  if(x->up <= y->up)
    {
      lim = x->up * g_dsp_block_len;
      for(i=0; i<lim; i++)
	x->s_block[i] = y->s_block[i];
    } 
}

inline void cpy_vecs(t_tlsig *x, t_tlsmp *vec, int s_cnt){

  int i;

  if(s_cnt <= g_dsp_block_len)
      for(i=0; i<s_cnt; i++)
	x->s_block[i] = vec[i];
  else printf("vector is too long to copy\n");

}

inline void zero_out_tlsig(t_tlsig *x){
  int i, lim;

  lim = x->up * g_dsp_block_len;
  for(i=0; i<lim; i++)
    x->s_block[i] = 0.0;
  
}

void signal_setup(t_tlsig *x, int type, int up){
 
  int i;
  x->type = type;
  x->up = up;

  x->s_block = malloc(sizeof(t_tlsmp) * g_dsp_block_len * up);
  for(i=0; i<(g_dsp_block_len * up); i++)
    x->s_block[i] = 0.0f;

}


void kill_tlsigs(t_tlsig **x, int sig_cnt){
  int i;

  if(x)
    {
      for(i=0; i<sig_cnt; i++)
	{
	  if(x[i]->s_block)
	    {
	      free(x[i]->s_block);
	      x[i]->s_block = 0;
	    }
	  free(x[i]);
	  x[i] = NULL;
	}
      free(x);
      x = NULL;
    }

}

void kill_one_tlsig(t_tlsig *x){

  if(x)
    {
      if(x->s_block)
	{
	  free(x->s_block);
	  x->s_block = NULL;
	}
      free(x);
      x = NULL;
    }

}

//--------------------------------------------
//--filer class

void init_filer_list(void){

  g_filer_head  =  (t_filer *)malloc(sizeof(t_filer));
  g_filer_head->next = NULL;
  g_filer_head->fp = NULL;
  g_filer_head->full_path = NULL;
  g_filer_head->name = (char *)malloc(sizeof(char) * 512);
  strcpy(g_filer_head->name, "list_head");
  g_filer_head->frames = 0;
  g_filer_head->sig = NULL;
  g_filer_head->pos = 0;
}

void do_filer_list(void){
  printf("doing filer list\n");
  t_filer *x = g_filer_head->next;
  while(x!=NULL)
    {
      write_filer(x);
      x=x->next;
    }

}

void kill_filer_list(void){

  t_filer *x = g_filer_head;
  t_filer *tmp = x;
  
  while(x->next != NULL)
    {
      x = x->next;
      close_filer(tmp);
      tmp=x;
    }

  close_filer(x);

}

t_filer *init_filer(char *full_path){



  t_filer *x = (t_filer *)malloc(sizeof(t_filer));
  x->name = (char *)malloc(sizeof(char) * 512);
  x->full_path = (char *)malloc(sizeof(char) * 512);
  strcpy(x->full_path, full_path);
  printf("x->fp : %p\n", x->fp);

  x->fp = fopen(x->full_path, "w");
  /* if(x->fp = fopen(x->full_path, "w") == NULL) */
  /*   { */
  /*     printf("could not open file: %s\n", x->full_path); */
  /*     free(x->full_path); */
  /*     free(x->name); */
  /*     free(x); */
  /*     x=NULL; */
  /*     return x; */
  /*   } */
  printf("x->fp : %p\n", x->fp);
  cpy_file_name_no_path(x->name, x->full_path);
  x->sig = NULL;
  x->next = NULL;
  x->pos = -1;
  x->frames = 0;

  return x;

}

void install_filer(t_filer *x){

  t_filer *y = g_filer_head;
  int pos = 1;
  while(y->next!=NULL)
    {
      y=y->next;
      pos++;
    }
  x->pos = pos;
  y->next = x;

}

void write_filer(t_filer *x){
  
  int i, j;
  printf("writing to frame %d\n", x->frames); 
  if(x->sig !=NULL)
    {
      for(i=0; i<x->sig->sample_cnt; i++)
	fprintf(x->fp, "%f\n", x->sig->s_block[i]);
     
    }
  
  else
    printf("sorry, no file ptr or no signal ptr in %s\n", x->name);
  x->frames++;   
}

void close_filer(t_filer *x){
  

  printf("closing file %s\n", x->name);
  if(x)
    {
      if(x->fp)
	fclose(x->fp);
      x->sig = NULL;
      if(x->name)
	{
	  free(x->name);
	  x->name = NULL;
	}
      if(x->full_path)
	{
	  free(x->full_path);
	  x->full_path = NULL;
	}
      free(x);
      x=NULL;
    }
}

void remove_filer(t_filer *x){

  if(x==g_filer_head)return;

  int i = 0;
  t_filer *tmp;
  tmp = g_filer_head;

  while(tmp->next != x)
    tmp=tmp->next;
  if(x->next!=NULL)
    tmp->next=x->next;
  else tmp->next = NULL;
  if(x)
    close_filer(x);

}

//-----------------------------------------
//--'obj' (module and signal data) registry

g_container_cnt = 0;
t_obj_data *init_sig_container(t_tlsig **sigs, int sig_cnt){


  t_obj_data *x = (t_obj_data *)malloc(sizeof(t_obj_data));
  g_container_cnt++;
  x->type = 3;//container type, not a module's od
  x->sigs =sigs;
  x->sig_cnt = sig_cnt;
  x->type_str = name_new("container");
  x->name_str = NULL;
  x->this_type_cnt = g_container_cnt;
  return(x);

}

void kill_sig_container(t_obj_data *x){

  if(x->type_str)
    {
      free(x->type_str);
      x->type_str = NULL;
    }

  if(x->name_str)
    {
      free(x->name_str);
      x->name_str = NULL;
    }


  if(x)
    free(x);

}


void init_obj_reg(void){

  int i;
  g_obj_reg = (t_obj_reg *)malloc(sizeof(t_obj_reg));
  g_obj_reg->obj_arr = 
    (t_obj_data **)malloc(sizeof(t_obj_data) * MAX_LEN);
  for(i=0; i<MAX_LEN; i++)
    g_obj_reg->obj_arr[i] = NULL;
  g_obj_reg->ref_cnt = -1;

}

void install_obj(t_obj_data *x){
  
  int i;  
  g_obj_reg->ref_cnt++; 
  g_obj_reg->obj_arr[g_obj_reg->ref_cnt] = x;
  x->reg_place = g_obj_reg->ref_cnt;

  printf("installing %s %d with %d signals\n", x->type_str, x->this_type_cnt, x->sig_cnt);
  printf("ref_cnt = %d\n", g_obj_reg->ref_cnt);

  for(i=0; i<x->sig_cnt; i++)
    {
      g_sig_reg[g_sig_reg_cntr]->sig_ptr = x->sigs[i];
      g_sig_reg[g_sig_reg_cntr]->obj_ptr = x;
      g_sig_reg[g_sig_reg_cntr++]->sig_no = i;
    }


}

void empty_obj_reg(t_obj_reg *x){

  int i;
  for(i=0; i<x->ref_cnt; i++)
    x->obj_arr[i] = NULL;//don't have to free these objects -- 
  //they belong to other data structures


  g_obj_reg->ref_cnt = -1;
  install_obj(&g_empty_obj);

}

void kill_obj_reg(void){

  int i;

  for(i=0; i<g_obj_reg->ref_cnt; i++)
    if(g_obj_reg->obj_arr[i])
      {
	if(g_obj_reg->obj_arr[i]->type == 3)
	  kill_sig_container(g_obj_reg->obj_arr[i]);

	g_obj_reg->obj_arr[i] = NULL;
      }

  free(g_obj_reg);
  g_obj_reg = NULL;

}

void init_sig_reg(void){
  
  int i;

  g_sig_reg = (t_sig_reg_data **)malloc(sizeof(t_sig_reg_data *) * MAX_LEN);
  for(i=0; i<MAX_LEN; i++)
    g_sig_reg[i] = (t_sig_reg_data *)malloc(sizeof(t_sig_reg_data));
  g_sig_reg_cntr = 0;

}


void empty_sig_reg(void){

  int i;
  for(i+0; i<g_sig_reg_cntr; i++)
    {  
      g_sig_reg[i]->sig_ptr = NULL;
      g_sig_reg[i]->obj_ptr =  NULL;
      g_sig_reg[i]->sig_no = 0;
    }
  g_sig_reg_cntr = 0;
  

}

void kill_sig_reg(void){

  int i;
  for(i=0; i<MAX_LEN; i++)
    if(g_sig_reg[i])
      {
	free(g_sig_reg[i]);
	g_sig_reg[i] = NULL;
      }

  if(g_sig_reg)
    free(g_sig_reg);

}

/*********circular buffer object************/
t_circ_buff *init_circ_buff(int len, int blck_len){

  t_circ_buff *x = (t_circ_buff *)malloc(sizeof(t_circ_buff));

  x->len = len;
  x->buff = (t_tlsmp *)malloc(sizeof(t_tlsmp) * x->len);
  x->w_pos = x->r_pos = 0;

  x->blck_len = blck_len;
  x->blck = (t_tlsmp *)malloc(sizeof(t_tlsmp) * x->blck_len);

  zero_out(x);
  x->feeder = g_empty_sig;
  x->counter = 0;
  x->go_flag = 0;

  return (x);

}

void kill_circ_buff(t_circ_buff *x){

  if(x->blck)
    {
      free(x->blck);
      x->blck = NULL;
    }

  if(x->buff)
    {
      free(x->buff);
      x->buff = NULL;
    }

  if(x)
    {
      free(x);
      x = NULL;
    }

}

void zero_out(t_circ_buff *x){
  
  int i;
  if(x->buff)
    {
      for(i=0; i<x->len; i++)
	{
	  x->buff[i] = 0.0;
  //if debug
	  //printf("%d %f\t",i, x->buff[i]);
	}
      //if_debug
      //printf("\n");    
    }
  if(x->blck)
    {
      for(i=0; i<x->blck_len; i++)
	x->blck[i] = 0.0;
    }
  x->w_pos = x->r_pos = 0;
}


void resize_blck(t_circ_buff *x, int blck_len){
  
  int i;
  if(x->blck)
    free(x->blck);
    
  x->blck_len = blck_len;
  x->blck = (t_tlsmp *)malloc(sizeof(t_tlsmp) * x->blck_len);     
  
  for(i=0; i<x->blck_len; i++)
    x->blck[i] = 0.0;
  
}

void set_blck(t_circ_buff *x){

  int i;
  
  if(x->blck)
    {
      pthread_mutex_lock(&g_circ_buff_lock);
      for(i=0; i<x->blck_len; i++)
	{
	  x->blck[i] = x->buff[x->r_pos++];
	  if(x->r_pos >= x->len)x->r_pos -= x->len;
	}
      pthread_mutex_unlock(&g_circ_buff_lock);
      //printf("set_blck is over. i = %d\n", i);
      //x->go_flag = 0;
    }

  else{};
}

void fill_circ_buff(t_circ_buff *x, int s){
  //printf("filling circ buff...\n");
  int i;
 
  for(i=0; i<s; i++)
    {
      x->buff[x->w_pos++] = x->feeder->s_block[i];
      if(x->w_pos>=x->len) x->w_pos-=x->len;
      if(x->counter++>=x->blck_len)
  	{
  	  //printf("%d %d\n", x->counter, x->go_flag);
  	  //x->go_flag = 1;
  	  set_blck(x);
  	  x->counter = 0;
	  
  	}
    }

}

/*********ctl stack etc.************/

void init_duple_stack(void){

  g_duple_stack = (t_duple_stack *)malloc(sizeof(t_duple_stack));
  g_duple_stack->top = -1;

}

void push_duple(t_tlsmp who, t_tlsmp what){

  if(g_duple_stack->top++>g_max_stack)
      printf("error: g_duple_stack stack overflow\n");
  else
    {
      g_duple_stack->duples[g_duple_stack->top].who = who;
      g_duple_stack->duples[g_duple_stack->top].who_int = (int)who;
      g_duple_stack->duples[g_duple_stack->top].what = what;
      //if debug
      //printf("pushing: %d %f\n", (int)who, what);
    }

}

t_duple *pop_duple(void){

  if(g_duple_stack->top--<0)
    {
      printf("error: g_duple_stack stack underflow\n");
      return NULL;
    }
  else
      return(&g_duple_stack->duples[g_duple_stack->top+1]);

}

void empty_duple_stack(void){
  g_duple_stack->top = -1;
}


void kill_duple_stack(void){
  free(g_duple_stack);
}

void *read_ctl_input(void *ptr){
    
  t_tlsmp a, b;
  int i;
  while(1)
    {
      
      scanf("%f %f;", &a, &b);//read input
      
      //lock and push
      pthread_mutex_lock(&g_duple_stack_lock);
        push_duple(a, b);
      pthread_mutex_unlock(&g_duple_stack_lock);
      //printf("pushing %f %f\n", a, b);
    }
  pthread_exit(NULL);

}




void process_ctl_input(void){
 
  int top = g_duple_stack->top;
  int i;
  t_duple *duple;

  //flatline those that need it
  for(i=0; i<g_lin_ctl_reg->ref_cnt; i++)
    if(g_lin_ctl_reg->ctl_ptr_arr[i]->flatline_flag==1)
      flatline_lin_ctl(g_lin_ctl_reg->ctl_ptr_arr[i]);

  pthread_mutex_lock(&g_duple_stack_lock);
  while(g_duple_stack->top>-1)
    {
      duple = pop_duple();
      if(duple->who_int == MAX_LEN)
	g_audio_off();
      if(duple->who_int == 9998)
	g_audio_on();
      if(duple->who_int == 9997)
	g_sys_quit = 1;

      execute_lin_ctl(duple);
      //printf("popping %f %f\n", duple->who, duple->what);
    }
  pthread_mutex_unlock(&g_duple_stack_lock);
}


/********control objects**********/
t_lin_ctl *init_lin_ctl(int ref_num, int type, int up){



  t_lin_ctl *x = (t_lin_ctl *)malloc(sizeof(t_lin_ctl));
  x->type = type;

  if(x->type == CTL_T_LIN)
    x->ctl_sig = init_one_tlsig(0, up);
  else x->ctl_sig = NULL;

  x->do_bang = NULL;
  x->toggle_flag = 0;
  x->ref_num = ref_num;
  x->val_was = 0.0;
  x->val_is = 0.0;
  x->up = up;
  x->flatline_flag=-1;
  x->name = NULL;
  install_lin_ctl(x);
  return(x);

}

void set_toggle(t_lin_ctl *x, t_tlsmp val){
  
  x->toggle_flag = 0;
  if(val!=0.0)
    x->toggle_flag = 1;

  //printf("toggle %d is at %d\n", x->ref_num, x->toggle_flag);
}

void lin_ctl_interp(t_lin_ctl *x, t_tlsmp val){

  //printf("lin_ctl_interp\n");
  t_tlsmp diff, inc, start;
  int i;
  x->val_was = x->ctl_sig->s_block[x->ctl_sig->sample_cnt-1];//last sample
  x->val_is = val;
  diff = x->val_is - x->val_was;
  inc = diff/g_dsp_block_len;
  start = x->val_was;

  //if debug
  //printf("lin ctl interp: %d %f %f\n", x->ref_num, x->val_was, x->val_is);

  for(i=0; i<x->ctl_sig->sample_cnt; i++)
    {
      start+=inc;
      x->ctl_sig->s_block[i] = start;
      //if debug
      /* printf("ctl_sig %d[%d]: %f\nval_was : %f val_is %f\n", */
      /* 	     x->ref_num, */
      /* 	     i, */
      /* 	     x->ctl_sig->s_block[i], */
      /* 	     x->val_was, */
      /* 	     x->val_is); */
    }
  //flag this ctl for flatline on next clock tick
  x->val_was = x->val_is;
  x->flatline_flag = 1;

}


void execute_lin_ctl(t_duple *x){

  t_lin_ctl *y = g_lin_ctl_reg->ctl_ptr_arr[x->who_int];
  //printf("execute_lin_ctl %d, %f\n", x->who_int, x->what);
  
  if(y->type == CTL_T_BANG)
    {
      if(y->do_bang)
	y->do_bang();
      else printf("please assign a method to this bang control\n");
    }
  else if(y->type == CTL_T_LIN)
    lin_ctl_interp(y, x->what);

  else if(y->type == CTL_T_TOGGLE)
    set_toggle(y, x->what);
  
  else printf("no such control structure with ref num : %d\n", x->who_int);

}

void flatline_lin_ctl(t_lin_ctl *x){

  int i;
  t_tlsmp val;

  //printf("flatlining:\n");
  val = x->val_was;
  //printf("%d\n", x->ctl_sig->sample_cnt);

  for(i=0; i<x->ctl_sig->sample_cnt; i++)
      x->ctl_sig->s_block[i] = val;
  x->flatline_flag = -1;
     
}

void kill_lin_ctl(t_lin_ctl *x){

  if(x->ctl_sig)
    kill_one_tlsig(x->ctl_sig);
  if(x->name)
    {
      free(x->name);
      x->name = NULL;
    }
  if(x)
    {
      free(x);
      x = NULL;
    }
}

extern void set_lin_ctl_val(t_lin_ctl *x, t_tlsmp val){

  int i;
  int lim = x->ctl_sig->up * g_dsp_block_len;
  for(i=0; i<lim; i++)
    x->ctl_sig->s_block[i] = val;
  

}

void zero_lin_ctl(t_lin_ctl *x){

  int i;
  for(i=0; i<x->ctl_sig->sample_cnt; i++)
    x->ctl_sig->s_block[i] = 0.0;

}

void unity_lin_ctl(t_lin_ctl *x){

  int i;
  for(i=0; i<x->ctl_sig->sample_cnt; i++)
    x->ctl_sig->s_block[i] = 1.0;

}

void level_lin_ctl(t_lin_ctl *x, t_tlsmp val){

  int i;
  for(i=0; i<x->ctl_sig->sample_cnt; i++)
    x->ctl_sig->s_block[i] = val;

}



void init_lin_ctl_reg(){
  
  int i;
  g_lin_ctl_reg = (t_lin_ctl_reg *)malloc(sizeof(t_lin_ctl_reg));
  g_lin_ctl_reg->ctl_ptr_arr = (t_lin_ctl **)malloc(MAX_LEN *
						    sizeof(t_lin_ctl *));
  for(i=0; i<MAX_LEN; i++)
    {
      
      g_lin_ctl_reg->flatline_arr[i] = -1; 
      g_lin_ctl_reg->ctl_ptr_arr[i] = NULL;
   }

  g_lin_ctl_reg->ref_cnt = 0; 

}

void install_lin_ctl(t_lin_ctl *x){

  x->ref_num = g_lin_ctl_reg->ref_cnt++;
  g_lin_ctl_reg->ctl_ptr_arr[x->ref_num] = x;
  printf("installed ctl of type %d at pt %d\n", x->type, x->ref_num);

}

void empty_lin_ctl_reg(void){

  int i;

  for(i=0; i<MAX_LEN; i++)
    {
      if(g_lin_ctl_reg->ctl_ptr_arr[i]!=NULL)
	kill_lin_ctl(g_lin_ctl_reg->ctl_ptr_arr[i]);
      
      g_lin_ctl_reg->ctl_ptr_arr[i] = NULL;
    }

  g_lin_ctl_reg->ref_cnt = 0;


}

void kill_lin_ctl_reg(void){

  int i;
  for(i=0; i<MAX_LEN; i++)
    {
      if(g_lin_ctl_reg->ctl_ptr_arr[i]!=NULL)
	kill_lin_ctl(g_lin_ctl_reg->ctl_ptr_arr[i]);

      g_lin_ctl_reg->ctl_ptr_arr[i] = NULL;
    }

  free(g_lin_ctl_reg->ctl_ptr_arr);
  g_lin_ctl_reg->ctl_ptr_arr = NULL;

  free(g_lin_ctl_reg);
  g_lin_ctl_reg=NULL;

}

/********dsp chain registers**********/

t_dsp_chain_list *new_dsp_chain_list(void){
  
  
  int i;
  t_dsp_chain_list *x;
  x = (t_dsp_chain_list*)malloc(sizeof(t_dsp_chain_list));
  x->func = NULL;
  x->next=NULL;
  return(x);

}

void install_dsp_chain(t_dsp_chain x, t_dsp_chain_list *y){
  
  int cnt = 0;
  t_dsp_chain_list *cond = y;
  t_dsp_chain_list *z;
  //goto end of chain
  while(cond->next!=NULL)
    {
      cnt++;  
      cond=cond->next;
    }
  if(cnt<1)//just the one chain
    cond->func = x;
  else
    {
      z= new_dsp_chain_list();
      cond->next=z;
      z->func = x;
    }
}

void run_dsp_chain(t_dsp_chain x, int samples){

  int s = samples;
  int c = 0, count;

  g_adc->dsp_func(s, g_adc);
  if(x)
      x(s, g_adc->o_sigs, g_dac->i_sigs);

  pthread_mutex_lock(&dac_lock);//I don't think we need this
  g_dac->dsp_func(s, g_dac);
  pthread_mutex_unlock(&dac_lock);

  fill_circ_buff(g_circ_buff_scope_y, s);
  fill_circ_buff(g_circ_buff_scope_z, s);

  /* for(i=0; i<s; i++) */
  /*   fprintf(fp,"%f\n", g_dac->i_sigs[1]->s_block[i]); */
}

void empty_dsp_chain_list(t_dsp_chain_list *x){

  t_dsp_chain_list *y=x;
  
  if(x->next)
    kill_dsp_chain_list(x->next);
  x->func = NULL;
  x->next = NULL;

}

void kill_dsp_chain_list(t_dsp_chain_list *x){

  //printf("killing dsp chain %d\n",x->count);  
  t_dsp_chain_list *y = x;
  t_dsp_chain_list *tmp;
  if(y)
    {

      while(y->next!=NULL)
	{
	  tmp=y;
	  y=y->next;
	  free(tmp);
	  tmp=NULL;
	
	}

    }
}

//******************************
//kill/init function stacks

t_ki_stack *init_ki_stack(void){

  int i;

  t_ki_stack *x = (t_ki_stack *)malloc(sizeof(t_ki_stack));
  x->stack = (t_ki_func **)malloc(sizeof(t_ki_func *) * MAX_LEN);
  for(i=0; i<MAX_LEN; i++)
    x->stack[i] = NULL;
  x->pos = -1;
  return(x);
}

void push_ki(t_ki_stack *x, t_ki_func *y){
  
  x->pos++;
  if(x->pos>=MAX_LEN)
    printf("ki stack overflow\n");
  else
    x->stack[x->pos] = y;

}

t_ki_func *pop_ki(t_ki_stack *x){
  //printf("welcome to the kill stack %d\n", x->pos);
  x->pos--;
  if(x->pos>=-1)
    return(x->stack[x->pos+1]);
  else
    {
      printf("ki stack underflow\n");
      return(NULL);
    }
      
  
}

void kill_ki_stack(t_ki_stack *x){
  
  int i;
  if(x)
    {
      if(x->pos!=-1)
	for(i=0; i<x->pos; i++)
	  x->stack[i] = NULL;
      free(x);
      x= NULL;
    }
}

//************************************
//tl_so stuff
t_tl_so *init_tl_so(char *name){

  t_tl_so *x = (t_tl_so *)malloc(sizeof(t_tl_so));
  if(name)
    x->name = name_new(name);
  x->kill = NULL;
  x->init = NULL;
  x->dsp = NULL;

  return(x);

}

void kill_tl_so(t_tl_so *x){

  if(x->name)
    {
      free(x->name);
      x->name = NULL;
    }
  if(x)
    {
      free(x);
      x= NULL;
    }
}

void tl_so_do_kill(void){

  t_ki_func kill;
  while(g_kill_stack->pos>=0)
    {
      printf("killing %s in kill stack %d\n", g_tl_name, g_kill_stack->pos);
      kill = pop_ki(g_kill_stack);
      kill();
    }

  empty_dsp_chain_list(g_dsp_chain_head);
  //empty dsp_stack

}

void reinit_sig_reg(void){
  empty_sig_reg();
}

void reinit_circ_buffs(void){
  printf("zero out scope y\n");
  zero_out(g_circ_buff_scope_y);
  printf("zero out scope z\n");
  zero_out(g_circ_buff_scope_z);
  g_circ_buff_scope_y->feeder = g_empty_sig;
  g_circ_buff_scope_z->feeder = g_empty_sig;
}

void reinit_soundout(void){
  int i;
  g_soundout->buff_len = g_soundout->block_len * g_outchannels;
  g_soundout->buff_pos_w = 0;
  g_soundout->buff_pos_r = 0;
  for(i=0; i<g_soundout->buff_len; i++)
    g_soundout->buff[i] = 0.0;
}

void reinit_for_new_so(void){

  int i;
  tl_so_do_kill();
  reinit_soundout();
  reinit_sig_reg();
  empty_obj_reg(g_obj_reg);
  empty_duple_stack();
  empty_lin_ctl_reg();
  reinit_circ_buffs();
  for(i=0; i<g_outchannels; i++)
    g_dac->i_sigs[i] = g_empty_sig;
 
}


void load_tl_so(char *name){

  int i;//implement later -- search through possible paths
  reinit_for_new_so();
  t_tl_so *x = init_tl_so(name);

  cpy_file_name_no_path(g_tl_name, name);
  
  x->handle = dlopen(name, RTLD_LAZY | RTLD_GLOBAL);
  if(!x->handle)
    printf("tl_so load ERROR:\n\t%s\n",dlerror());
  
  else
    {
      x->kill = (t_ki_func)dlsym(x->handle, "do_kill");
      if(!x->kill)printf("not adding kill\n");
      //error
      
      else
	{
	  printf("pushing %s kill onto kill stack\n", g_tl_name);
	  push_ki(g_kill_stack, x->kill);
	  printf("...done\n");
	}
      x->init = (t_ki_func)dlsym(x->handle, "setup_this");
      if(!x->init)("not adding setup\n");
      //error
      
      else
	{
	  printf("initializing %s setup\n", g_tl_name);
	  x->init();
	  printf("...done\n");
	}
      
      x->dsp = (t_dsp_chain)dlsym(x->handle, "dsp_chain");
      if(!x->dsp)("not adding dsp\n");
      //error
      
      else
	{
	  printf("about to install %s dsp chain\n", g_tl_name);
	  install_dsp_chain(x->dsp, g_dsp_chain_head);
	  printf("...done\n");
	}
    }
  if(x->name)
    {
      free(x->name);
      x->name = NULL;
    }
  free(x);
  x = NULL;
  
}

void cpy_file_name_no_path(char *name, char *full_path){

  int i;
  i = 0;

  while(full_path[i] != '\0')i++;
  while(full_path[i] != '/')i--;
  strcpy(name, full_path+i+1);
  //printf("my name is: %s\n", g_tl_name);


}



/************functions***************/

void init_all(void){

  int i;
  
  //global variables (adjustable anytime)
  g_samplerate = 48000;

  //this is not right yet: if these two aren't 
  //equivalent, the buffering is not correct
  g_dsp_block_len = DSP_BLOCK_LEN;


  g_loop_cnt = LOOPS;
  //g_batch_mode_flag = BATCH_MODE;
  
  g_count_flag = 0;
  if(g_loop_cnt!=1)
    g_count_flag = 1;

  g_inchannels = 2;
  g_outchannels = 2;

  g_indevno = 0;
  g_outdevno = 0;

  g_soundout = (t_soundout *)malloc(sizeof(t_soundout));
  g_soundout->block_len = SOUNDOUT_BLOCK_LEN;
  g_dsp_soundout_fact = g_soundout->block_len/g_dsp_block_len * g_outchannels;
 
  //////////////////////////////////////////////
  //setup the output buffer:
  g_soundout->buff_len = g_soundout->block_len * g_outchannels;
  g_soundout->buff_pos_w = 0;
  g_soundout->buff_pos_r = 0;
  g_soundout->buff = (t_tlsmp *)malloc(sizeof(t_tlsmp) * g_soundout->buff_len * g_outchannels);  
  for(i=0; i<g_soundout->buff_len; i++)
    g_soundout->buff[i] = 0.0;

#ifndef M_PI
  g_pi = 3.14159;
#else
  g_pi = M_PI;
#endif

  g_two_pi = g_pi * 2.0;
  g_one_sixth = .16666666667;

  g_max_stack = MAX_LEN;

  g_sample_time = 1.0/(t_tlsmp)g_samplerate;
  g_block_time = g_sample_time * (t_tlsmp)g_dsp_block_len; 
  g_audio_quit = 0;
  g_sys_quit = 0;

  g_loops_per_step = DEFAULT_LOOPS;
  g_flow_mode = RT_MODE;

  g_empty_sig = init_one_tlsig(0, 1);  
  g_empty_sigs = init_tlsigs(1, O_TYPE, 1);//what if we upsample?
  g_circ_buff_scope_y = init_circ_buff(10000, 1000);
  g_circ_buff_scope_y->feeder = g_empty_sig;
  g_circ_buff_scope_z = init_circ_buff(10000, 1000);
  g_circ_buff_scope_z->feeder = g_empty_sig;

  g_dsp_chain_head=new_dsp_chain_list();

  init_duple_stack();
  init_lin_ctl_reg();
  init_obj_reg();
  init_sig_reg();

  init_filer_list();

  g_empty_obj.obj_cnt = 1;
  g_empty_obj.type = 99;
  g_empty_obj.reg_place = -1;
  g_empty_obj.type_str = name_new("empty");
  g_empty_obj.name_str = name_new("empty");
  g_empty_obj.sigs = g_empty_sigs;
  g_empty_obj.sig_cnt  = 1;
  g_empty_obj.this_type_cnt = 1;
  g_empty_obj.sig_reg_ptr = -1;
  install_obj(&g_empty_obj);

  /* g_sig_reg = (t_sig_reg_data **)malloc(sizeof(t_sig_reg_data *) * MAX_LEN); */
  /* for(i=0; i<MAX_LEN; i++) */
  /*   g_sig_reg[i] = (t_sig_reg_data *)malloc(sizeof(t_sig_reg_data)); */
  /* g_sig_reg_cntr = 0; */

  g_kill_stack = init_ki_stack();


  g_dac = (t_dac *)dac_init(g_outchannels);
  for(i=0; i<g_outchannels; i++)
    g_dac->i_sigs[i] = g_empty_sig;
  
  //install_obj(&g_dac->od);

  g_adc = (t_adc *)adc_init(g_inchannels);
      
  g_batch_mode = BATCH;

  g_sleep_time.tv_sec = 1;
  g_sleep_time.tv_nsec = 1000;

  g_step_mode = 0;
  g_tl_name = (char *)malloc(sizeof(char) * MAX_LEN);

  load_tl_so(DEFAULT_TL_PATH);
  //fp=fopen("dac_out", "w");
}

//kills everything
void kill_all(void){

  int i;


  tl_so_do_kill();
  if(g_dsp_chain_head)
    {
      kill_dsp_chain_list(g_dsp_chain_head);
      /* for(i=0; i<g_dsp_chain_reg->count; i++) */
      /* 	{ */
      /* 	  printf("%d %d\n",g_dsp_func_reg->count, i); */
  	  
      /* 	  g_dsp_chain_reg->reg[i]=NULL; */
      /* 	} */
      /* free(g_dsp_chain_reg); */
      /* g_dsp_chain_reg=NULL; */
    }

  if(a_conversion_buff)
    {
      free(a_conversion_buff);
      a_conversion_buff = NULL;
    }

  if(g_empty_obj.type_str)
    {
      free(g_empty_obj.type_str);
      g_empty_obj.type_str = NULL;
    }

  if(g_empty_obj.name_str)
    {
      free(g_empty_obj.name_str);
      g_empty_obj.name_str = NULL;
    }

  
  kill_duple_stack();
  kill_lin_ctl_reg();
  kill_one_tlsig(g_empty_sig);
  kill_tlsigs(g_empty_sigs, 1);
  kill_obj_reg();
  
  kill_ki_stack(g_kill_stack);
  kill_sig_reg();
  kill_filer_list();

  if(pa_conversion_buff)
    {
      free(pa_conversion_buff);
      pa_conversion_buff = NULL;
    }

  if(g_soundout)
    {
      if(g_soundout->buff)
	{
	  free(g_soundout->buff);
	  g_soundout->buff = NULL;
	}
      free(g_soundout);
      g_soundout = NULL;
    }

  kill_dac(g_dac);
  kill_adc(g_adc);

  kill_circ_buff(g_circ_buff_scope_y);
  kill_circ_buff(g_circ_buff_scope_z);

  if(g_tl_name)
    {
      free(g_tl_name);
      g_tl_name = NULL;
    }

  //fclose(fp);
  printf("Goodbye!\n");
  exit(0);

    
}


//unused:
void throw_error(char *string){
  printf("ERROR: \n\t%s\n", string);
}

char *name_new(char *a_name){
  char *str;
  str = malloc(sizeof(char) * (strlen(a_name)+1));
  strcpy(str, a_name);
  return(str);
}

t_tlsmp g_getrealtime(void){
  //this code is literaly copied and pasted right out of
  //the Pd function 'sys_getrealtime' in the file s_inter.c
  //from pd-0.43-4
  //the only change is that we measure in usecs rather than secs
  //note: this will only work on Unix/BSD operating systems --
  //when we move to imbedded devices, we will need to do something else
  
  static struct timeval then;
  struct timeval now;
  gettimeofday(&now, 0);
  if (then.tv_sec == 0 && then.tv_usec == 0) then = now;
  return ((now.tv_sec - then.tv_sec) +
	   (now.tv_usec - then.tv_usec));
}

/********scheduler:***********/
void sched_block(void){

  //todo implement other audio apis:
  if(pa_sched_block)
    pa_sched_block();

  
}


void dsp_tick(int samples){
  int s = samples;
  int c = 0, count = 0;
  t_dsp_chain_list *x = g_dsp_chain_head;
  //printf("in the dsp_tick\n");
  run_dsp_chain(x->func, s);
  while(x->next!=NULL)
    {
      x=x->next;
      run_dsp_chain(x->func, s);
    }

}

void whole_chain(void){

  int i;
  pthread_attr_t thread_attr;

  /* pthread_attr_init(&thread_attr); */
  /* pthread_attr_setdetachstate(&thread_attr, PTHREAD_CREATE_DETACHED); */
  if(g_batch_mode!=1)
    g_read_ctl_id = pthread_create( &g_read_ctl_thread,
  				    NULL,
  				    read_ctl_input,
  				    NULL);

  if(g_step_mode == 0)
    {
      //printf("loops: %d\n", LOOPS);
      while(counter < g_loops_per_step)
	{
	  //printf("counter: %d\n", counter);

	  if(g_audio_quit)
	    {
	      
	      goto quit;
	    }
	  
	  //1: read/write samples
	  if(g_flow_mode == RT_MODE)	  
	    sched_block();


	  //2: do the dsp tick
	  for(i=0; i<g_soundout->block_len; i+=g_dsp_block_len)
	    dsp_tick(g_dsp_block_len);
	  
	  //3: do the control level processing
	  process_ctl_input();
	  if(g_filer_head->next!=NULL)
	    do_filer_list();
	  g_main_context_iteration(NULL, 0);//

	  	
	  if(g_flow_mode == STEP_MODE)
	    //printf("step_mode\n");
	    counter++;

	}

    
      printf("done :%d\n", counter);
      g_audio_off();
      //pthread_exit(NULL);
      return;
    }

  else //stepping -- don't write audio, just do one tick at a time
    {
      printf("stepping . . .\n");
      //2: do the dsp tick
      for(i=0; i<g_soundout->block_len; i+=g_dsp_block_len)
	dsp_tick(g_dsp_block_len);
      
      //3: do the control level processing
      process_ctl_input();
	  if(g_filer_head->next!=NULL)
	    do_filer_list();
	  g_main_context_iteration(NULL, 0);//
	  
    }
 quit:
  //pthread_exit(NULL);
  return;

}

void whole_step(void){
  whole_chain();
}

void g_start_timelab(void){

  if(g_batch_mode!=1)
    {

      gtk_init(NULL, NULL); // yes gui
      do_gui();
    }
  else g_audio_on();

}



void g_audio_on(void){
 
  g_audio_quit = 0;
  pa_audio_on();
  printf("audio on\n"); 
  whole_chain();//call the 'main loop'

}

void g_audio_off(void){

  printf("audio off\n");
  pa_audio_off();

  g_audio_quit = 1;//eliminate the 'main loop'
    
}

void *wait_for_sys_quit(void *ptr){

  while(1)
    {
      pthread_mutex_lock(&g_sys_quit_lock);


      if(g_sys_quit != 0)
	return;
      pthread_mutex_unlock(&g_sys_quit_lock);
    }
  return;
}

 void kill_from_gui(void){
	  
   g_audio_off();
   nanosleep(&g_sleep_time, &g_empty_time);//wait for the buffers to clear??
   kill_all();
}


//-----------------------------
//--interface for pd
void init_all_for_pd(void){

  int i;

  //g_samplerate set in timelab_pd~.c
   g_dsp_block_len = DSP_BLOCK_LEN;

   
  g_inchannels = 2;
  g_outchannels = 2;//make this variable in the future

  /* g_soundout = (t_soundout *)malloc(sizeof(t_soundout)); */
  /* g_soundout->block_len = SOUNDOUT_BLOCK_LEN; */
  /* g_dsp_soundout_fact = g_soundout->block_len/g_dsp_block_len * g_outchannels; */
  /* g_soundout->buff_len = g_soundout->block_len * g_outchannels; */
  /* g_soundout->buff_pos_w = 0; */
  /* g_soundout->buff_pos_r = 0; */
  /* g_soundout->buff = (t_tlsmp *)malloc(sizeof(t_tlsmp) * g_soundout->buff_len * g_outchannels);   */
  /* for(i=0; i<g_soundout->buff_len; i++) */
  /*   g_soundout->buff[i] = 0.0; */

#ifndef M_PI
  g_pi = 3.14159;
#else
  g_pi = M_PI;
#endif

  g_two_pi = g_pi * 2.0;
  g_one_sixth = .16666666667;

  g_max_stack = MAX_LEN;

  g_sample_time = 1.0/(t_tlsmp)g_samplerate;
  g_block_time = g_sample_time * (t_tlsmp)g_dsp_block_len; 

  g_empty_sig = init_one_tlsig(0, 1);  
  g_empty_sigs = init_tlsigs(1, O_TYPE, 1);//what if we upsample?


  init_duple_stack();
  init_lin_ctl_reg();
  init_obj_reg();
  init_sig_reg();   

}

void kill_all_for_pd(void){

  kill_duple_stack();
  kill_lin_ctl_reg();
  kill_one_tlsig(g_empty_sig);
  kill_tlsigs(g_empty_sigs, 1);
  kill_obj_reg();
  kill_sig_reg();

  /* if(g_soundout) */
  /*   { */
  /*     if(g_soundout->buff) */
  /* 	{ */
  /* 	  free(g_soundout->buff); */
  /* 	  g_soundout->buff = NULL; */
  /* 	} */
  /*     free(g_soundout); */
  /*     g_soundout = NULL; */
  /*   } */
 
}

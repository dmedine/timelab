/* this file belongs to */
/* timelab-0.10 by David Medine */
// this source code defines the
// scheduling and other core fuctionality in timelab
#include "tl_core.h"
#include "stdlib.h"
#include "string.h"
#include "unistd.h"
#include "pthread.h"
#include "dlfcn.h"

pthread_t dsp_thread;

//***********************************
//***********************************

//*************************//
// global audio //
//*************************//

inline void wrap_buff_w(tl_audio_buff *x){
  if(x->buff_pos_w >= x->buff_len)
    x->buff_pos_w -= x->buff_len;
}

inline void wrap_buff_r(tl_audio_buff *x){
  if(x->buff_pos_r >= x->buff_len)
    x->buff_pos_r -= x->buff_len;
}

tl_audio_buff *init_audio_buff(int channs){

  tl_audio_buff *x;
  int i;
  x = (tl_audio_buff *)malloc(sizeof(tl_audio_buff));
  x->buff = (tl_smp *)malloc(sizeof(tl_smp) * channs * tl_get_block_len());
  x->channs = channs;
  x->block_len = tl_get_block_len();
  x->buff_len = channs * tl_get_block_len();
  x->buff_pos_w = 0;
  x->buff_pos_r = 0;
  for(i=0; i<x->buff_len; i++)
    x->buff[i] = 0.0;
  return x;
}

void reset_audio_buff(tl_audio_buff *x){
  int i;
  for(i=0; i<x->buff_len; i++)
    x->buff[i] = 0.0;
  x->buff_pos_w = 0;
  x->buff_pos_r = 0;
}

void resize_audio_buff_channs(tl_audio_buff *x, int channs){
  // will this work?
  kill_audio_buff(x);
  x = init_audio_buff(channs);
}

void resize_audio_buff_block_len(tl_audio_buff *x){
  int channs = x->channs;
  kill_audio_buff(x);
  x = init_audio_buff(channs);
}

void kill_audio_buff(tl_audio_buff *x){
  if(x!=NULL)
    {
      if(x->buff!=NULL)
	{
	  free(x->buff);
	  x->buff = NULL;
	}
      free(x);
      x=NULL;
    }
}

void set_g_audio_buff_out(tl_audio_buff *x){
  tl_g_audio_buff_out = x;
}

void set_g_audio_buff_in(tl_audio_buff *x){
  tl_g_audio_buff_in = x;
}

tl_audio_buff *get_g_audio_buff_out(void){
  if(tl_g_audio_buff_out!=NULL)
    return(tl_g_audio_buff_out);
  else
    {
      printf("error: get_g_audio_buff_out: no out buff established\n");
      return (NULL);
    }
}

tl_audio_buff *get_g_audio_buff_in(void){
  if(tl_g_audio_buff_in!=NULL)
    return(tl_g_audio_buff_in);
  else
    {
      printf("error: get_g_audio_buff_in: no in buff established\n");
      return (NULL);
    }
}

void set_g_out_chann_cnt(int cnt){
  tl_g_out_chann_cnt = cnt;
}

void set_g_in_chann_cnt(int cnt){
  tl_g_in_chann_cnt = cnt;
}

int get_g_out_chann_cnt(void){
  return tl_g_out_chann_cnt;
}

int get_g_in_chann_cnt(void){
  return tl_g_in_chann_cnt;
}


//*****************************************************
//*****************************************************


//*****************************************//
// scheduling and housekeeping functions //
//*****************************************//

// TODO: implement error codes for this and other critical functions
static void tl_dsp_tick(void){
  
  tl_ctl *ctl_head;
  int thread_id;
  pthread_t thread;
  tl_audio_buff *buff_out;
  tl_class *class_head;
  int samples;
  
  printf("in the dsp tick\n");
  ctl_head = get_g_ctl_head();
  
  if(ctl_head == NULL)
    {
      printf("error: tl_dsp_tick: tl_g_ctl_head does not exist\n");
      goto over;
      tl_dsp_off();
    }

/* if(get_g_dsp_list == NULL) */
/* { */
/* printf("cannot begin dsp loop: tl_g_dsp_list does not exist\n"); */
/* goto over; */
/* } */
  
  buff_out = get_g_audio_buff_out();
  if(buff_out==NULL)
    {
      printf("error: tl_dsp_tick: tl_g_audio_buff_out not defined\n");
      goto over;
      tl_dsp_off;
    }

  class_head = get_g_class_head();

  if(class_head==NULL)
    {
      printf("error: tl_dsp_tick: tl_g_class_head not initialized\n");
      goto over;
      tl_dsp_off;
    }
  samples = tl_get_block_len();

  while(tl_g_dsp_status == DSP_ON)
    {
      // 1. read/write a block of audio
      pa_push_out(buff_out->buff);
      // 2. process samples
      tl_process_dsp_list(samples, class_head);
      // 3. process control
      process_ctl_list(ctl_head);
      //usleep(15000);// simulate audio block time
    }
  goto over;
 over:
  printf("exiting dsp tick\n");
  return;
}

extern tl_audio_on(void){
  int thread_id;
  printf("starting audio...\n");
  pa_audio_on(); // TODO: implement other audio APIs
  tl_dsp_on(); // turn the 'on' flag on
  // launch the tight loop until tl_dsp_off
  
  thread_id = pthread_create(&dsp_thread, NULL, &tl_dsp_tick, NULL);
}

extern tl_audio_off(void){
  printf("stopping audio...\n");
  tl_dsp_off();
  
}


// does this need thread protection?
// I'm inclined to say 'no'
inline int is_dsp_on(void){
  return tl_g_dsp_status;
}

void tl_dsp_on(void){
  tl_g_dsp_status = DSP_ON;
}

void tl_dsp_off(void){
  tl_g_dsp_status = DSP_OFF;
}

int tl_get_samplerate(void){
  return tl_g_samplerate;
}

int tl_get_block_len(void){
  return tl_g_block_len;
}

void tl_set_samplerate(int samplerate){
  tl_g_samplerate = samplerate;
}

void tl_set_block_len(int block_len){
  tl_g_block_len = block_len;
}

tl_name name_new(char *a_name){

  tl_name str;
  str = malloc(sizeof(char) * (strlen(a_name)+1));
  strcpy(str, a_name);
  return(str);
}


// this is copied directly from pd's sourcecode
void *resizebytes(void *old, size_t oldsize, size_t newsize){
  
  void *ret;
  if (newsize < 1) newsize = 1;
  if (oldsize < 1) oldsize = 1;
  ret = (void *)realloc((char *)old, newsize);
  if (newsize > oldsize && ret)
memset(((char *)ret) + oldsize, 0, newsize - oldsize);
  /* #ifdef LOUD */
  /* fprintf(stderr, "resize %lx %d --> %lx %d\n", (int)old, oldsize, (int)ret, newsize); */
  /* #endif /\* LOUD *\/ */
  /* #ifdef DEBUGMEM */
  /* totalmem += (newsize - oldsize); */
  /* #endif */
  /* if (!ret) */
  /* post("pd: resizebytes() failed -- out of memory"); */
  return (ret);
}


void tl_set_a_info(int sr, int blck, int indevno, int outdevno, int inchanns, int outchanns, float ltncy){
  tl_g_a_info.sr = sr;
  tl_g_a_info.block_len = blck;
  tl_g_a_info.indevno = indevno;
  tl_g_a_info.outdevno = outdevno;
  tl_g_a_info.inchanns = outchanns;
  tl_g_a_info.outchanns = outchanns;
  tl_g_a_info.latency = ltncy;
  tl_g_a_info_set_initialized();
}

tl_a_info tl_get_a_info(void){return tl_g_a_info;}
int tl_g_a_info_get_intitialized(void){return tl_g_a_info_initialized;}

void tl_g_a_info_set_initialized(void){tl_g_a_info_initialized = 1;}
void tl_g_a_info_unset_initialized(void){tl_g_a_info_initialized = 0;}

  //******************//
  //  module loading  //
  //******************//

tl_name cpy_file_name_no_path(char *full_path){
  
  char *file_name, *pch, *name;
  int i;

  i = strlen(full_path);
  // go to final / if any
  while(full_path[i]!='/'&&i>=0)i--;


  // copy the string into a buffer
  file_name = strdup(full_path+i+1);

  printf("%s\n",file_name);
  // kill the file extension
  if(pch = strchr(file_name, '.'))
     *pch = '\0';

  name = name_new(file_name);

  free(file_name);
  return name;
}

int tl_load_module(const char *arg_str){

  //printf("%s\n", arg_str);

  if (get_g_class_head==NULL)
    {
      printf("error: tl_load_module: tl_g_class_head does not exist, cannot load module\n");
      return -1;
    }

  // TODO: make error checks at every step

  tl_arglist arglist;
  tl_name mod_name;
  tl_class *x;
  void *handle;
  char *init_name;
  char *kill_name;
  char *dsp_name;
  const char *init_affix = "tl_init_";
  const char *kill_affix = "tl_kill_";
  const char *dsp_affix  = "tl_dsp_";
  int mod_name_len;
  int i;

  arglist.argc = 0; // perhaps we should init/kill these
  // for now, this initialization is necessary...

  // parse the creation string into arguments
  tl_parse_args(&arglist, arg_str);

  // first arg is the full path to the module
  mod_name = cpy_file_name_no_path(arglist.argv[0]->str_val); 

  // concoct the init function name
  mod_name_len = strlen(mod_name);

  init_name = malloc(sizeof(char) * (strlen(init_affix) + mod_name_len +1));
  strcpy(init_name, init_affix);
  init_name[strlen(init_affix)]='\0'; 
  strcat(init_name, mod_name);// correct?

  kill_name = malloc( sizeof(char) *(strlen(kill_affix) + mod_name_len));
  strcpy(kill_name, kill_affix);
  strcat(kill_name, mod_name);// correct?
  
  dsp_name = malloc(sizeof(char)*(strlen(dsp_affix) + mod_name_len));
  strcpy(dsp_name, dsp_affix);
  strcat(dsp_name, mod_name);// correct?


  // dlopen the module
  handle = dlopen(arglist.argv[0]->str_val, RTLD_LAZY | RTLD_GLOBAL);
  if(!handle);// TODO: check this
  
  x = init_class();

  // get the functions we want
  x->init_func = (tl_init_func)dlsym(handle, init_name);
  x->kill_func = (tl_init_func)dlsym(handle, kill_name);
  x->dsp_func = (tl_init_func)dlsym(handle, dsp_name);

  // intialize the module
  x->init_func(&arglist); // TODO: check for errors

  // get the dsp and kill functions and push them onto the stack
  tl_install_class(get_g_class_head(), x);  


  // free memory
  free(init_name);
  free(kill_name);
  free(dsp_name);

}



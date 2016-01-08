/* this file belongs to */
/* timelab-0.10 by David Medine */
// this source code defines the
// scheduling and other core fuctionality in timelab
#include "tl_core.h"
//#include "a_portaudio.h"
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

// NOTE: these may be redundant now! fixme
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
static void tl_dsp_tick(tl_procession *x){
  
  int thread_id;
  pthread_t thread;
  tl_audio_buff *buff_out;
  int samples;
  
  printf("in the dsp tick\n");
  
  if(x->ctl_head == NULL)
    {
      printf("error: tl_dsp_tick: invalid class head in provided procession\n");
      goto over;
      tl_dsp_off();
    }

  buff_out = get_g_audio_buff_out();
  if(buff_out==NULL)
    {
      printf("error: tl_dsp_tick: tl_g_audio_buff_out not defined\n");
      goto over;
      tl_dsp_off;
    }

  if(x->class_head==NULL)
    {
      printf("error: tl_dsp_tick: invalid class head in provided procession\n");
      goto over;
      tl_dsp_off;
    }
  samples = tl_get_block_len();

  while(tl_g_dsp_status == DSP_ON)
    {

      // 1. read/write a block of audio
      // this will block until it is time to read
      // write audio samples
      pa_push_out(buff_out->buff);

      // 2. process samples
      tl_process_dsp_list(samples, x->class_head);

      // this can maybe go on a separate thread:
      // 3. process control
      process_ctl_list(x->ctl_head, x->lvl_stck);

    }
  goto over;
 over:
  printf("exiting dsp tick\n");
  return;
}

extern tl_audio_on(tl_procession *x){
  int thread_id;
  printf("starting audio...\n");
  pa_audio_on(); // TODO: implement other audio APIs
  tl_dsp_on(); // turn the 'on' flag on
  // launch the tight loop until tl_dsp_off
  
  thread_id = pthread_create(&dsp_thread, NULL, &tl_dsp_tick, (void *)x);
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

}

tl_a_info tl_get_a_info(void){return tl_g_a_info;}

void kill_procession(tl_procession *x){

  // kill the controls first to avoid double free
  tl_kill_ctl_list(x->ctl_head);
  kill_lvl_stck(x->lvl_stck);

  // then kill the module list
  tl_process_kill_list(x->class_head);

  kill_audio_buff(x->ab_in);
  kill_audio_buff(x->ab_out);

  free(x);
  x=NULL;

}

tl_procession *init_procession(void){

  tl_procession *x = malloc(sizeof(tl_procession));
  x->class_head = init_class();
  x->ctl_head = init_ctl(TL_HEAD_CTL);
  x->lvl_stck = init_lvl_stck();
  x->ab_in = init_audio_buff(TL_MAXCHANNS);
  x->ab_out = init_audio_buff(TL_MAXCHANNS);
  return x;

}

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

tl_class *tl_load_module(tl_procession *procession, const char *arg_str){

  tl_arglist arglist;
  tl_name mod_name;
  tl_class *x;
  void *handle;
  char *init_name;
  char *kill_name;
  char *dsp_name;
  char *ctls_name;
  void* (*ctls_func)(void);
  const char *init_affix = "tl_init_";
  const char *kill_affix = "tl_kill_";
  const char *dsp_affix  = "tl_dsp_";
  const char *ctls_affix  = "tl_reveal_ctls_";
  int mod_name_len;
  int i;

  // we *ALWAYS* pass the procession as the first argument
  // so plug it into the arglist
  arglist.argc = 1;
  arglist.argv[0] = (tl_arg *)malloc(sizeof(tl_arg));
  arglist.argv[0]->procession = procession;
  arglist.argv[0]->type = TL_PROCESSION;
 
 // parse the creation string into arguments
  tl_parse_args(&arglist, arg_str);
  
  // first arg is the full path to the module
  mod_name = cpy_file_name_no_path(arglist.argv[1]->str_val);
  printf("module name copied: %s\n%s\n",arglist.argv[1]->str_val, mod_name);

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

  ctls_name = malloc(sizeof(char)*(strlen(ctls_affix) + mod_name_len));
  strcpy(ctls_name, ctls_affix);
  strcat(ctls_name, mod_name);// correct?


  // dlopen the module
  handle = dlopen(arglist.argv[1]->str_val, RTLD_LAZY | RTLD_GLOBAL);
  if(!handle)
    {
      printf("error: tl_load_module: dlopen did return valid file handle\n");
      goto fail;// TODO: check this
    }
  x = init_class();

  // get the functions we want
  if(!(x->init_func = (tl_init_func)dlsym(handle, init_name)))
    {
      printf("error: tl_load_module: %s has invalid init function\n",mod_name);
goto fail;
    }
  if(!(x->kill_func = (tl_init_func)dlsym(handle, kill_name)))
    {
      printf("error: tl_load_module: %s has invalid kill function\n",mod_name);
      goto fail;
    }
  if(!(x->dsp_func = (tl_init_func)dlsym(handle, dsp_name)))
    {
      printf("error: tl_load_module: %s has invalid dsp function\n",mod_name);
      goto fail;
    }
  int *ptr;


  // get the ctls, if any
  if(!(ctls_func = (tl_init_func)dlsym(handle, ctls_name)))
    {
      printf("warning: tl_load_module: %s has no ctls revelation\n",mod_name);
      ctls_func = NULL;
    }


  // TODO: othis needs a more elegant solution:

  if(!(ptr = dlsym(handle, "in_cnt")))
    printf("warning tl_load_module: %s has no symbol 'in_cnt'\n",mod_name);
  else
    x->in_cnt = *ptr;

  if(!(ptr=dlsym(handle, "out_cnt")))
    printf("warning tl_load_module: %s has no symbol 'out_cnt'\n",mod_name);
  else
    x->out_cnt = *ptr;
 printf("ins %d outs %d\n",x->in_cnt, x->out_cnt);
    
  // printf("ins %d outs %d\n",x->in_cnt, x->out_cnt);
     
  if(!(x->mod = (void *)dlsym(handle, "this")))
      printf("warning: tl_load_module: %s has no self-reference\n",mod_name);

  /* if(!(tmp = (void *)dlsym(handle, "name"))) */
  /*     printf("warning: tl_load_module: %s has no name\n",mod_name); */
  /* else x->name = tmp(x); */

  // intialize the module
  x->args = &arglist;
  // TODO: check to see that args are properly destroyed

  // this installs the class onto the procession class stack
  // it also initializes the module itself
  tl_install_class(procession->class_head, x);

  // install ctls if any
  if(ctls_func != NULL)
    install_onto_ctl_list(procession->ctl_head, (tl_ctl *)ctls_func());

  // free memory
  free(init_name);
  free(kill_name);
  free(dsp_name);
  free(ctls_name);

  // free the args we created (this doesn't destroy the procession that we use in the first arg, just the container that points to it)
  for(i=0;i<arglist.argc;i++)
    free(arglist.argv[i]);

  return x;
 fail:
  printf("error: tl_load_module: module was invalid\n");
  return NULL;

}



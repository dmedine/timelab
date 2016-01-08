#ifndef __g_api_h_
#include "pthread.h"
#include "float.h"
#include "time.h"
#include <stdio.h>
#include <stddef.h>


//--------------------------

#define t_tlsmp float
//typedef float t_tlsmp;

#define DSP_BLOCK_LEN 64
#define SOUNDOUT_BLOCK_LEN 64
#define MAX_LEN 9999
#define M_MODULE_TYPE 1
#define M_ODE_TYPE 2
#define CONTAINER_TYPE 3

struct timespec g_sleep_time, g_empty_time;
int g_outdevno, g_indevno;

int g_flow_mode;
int g_loops_per_step;
#define DEFAULT_LOOPS 1
#define RT_MODE 0
#define STEP_MODE 1

typedef void (*t_dsp_func)(int samples, void *class_ptr);
typedef void (*t_kill_func)(void);

typedef void (*t_bang_func)(void);

typedef struct _soundout{

  t_tlsmp *buff;
  int block_len;
  int buff_len;
  int buff_pos_w;
  int buff_pos_r;

}t_soundout;
t_soundout *g_soundout;

typedef struct _soundin{

  t_tlsmp *buff;
  int block_len;
  int buff_len;
  int buff_pos_w;
  int buff_pos_r;

}t_soundin;
t_soundin *g_soundin;
/********constants**********/
t_tlsmp g_pi;
t_tlsmp g_two_pi;
t_tlsmp g_one_sixth;

int g_batch_mode_flag;
int g_loop_cnt;
int g_count_flag;

int g_samplerate;
int g_dsp_block_len;//dsp calc block size
int g_inchannels, g_outchannels;
int g_max_inchannels, g_max_outchannels;//safeguards, not implemented yet
t_tlsmp g_sample_time;//the time of 1 sample in seconds
t_tlsmp g_block_time;//time of one block in seconds

int g_dsp_soundout_fact;
t_tlsmp g_audio_quit;

int g_batch_mode;

int g_dac_done;
int g_dac_wait;
int g_use_gui;

int g_ontime;

t_tlsmp g_tick_time;
int g_sys_quit;

int g_step_mode;

/************signals**************/
typedef struct _tlsig{

  t_tlsmp *s_block;
  int sample_cnt;
  int up;
  int type;

}t_tlsig;

#define O_TYPE 0
#define I_TYPE 1

//void signals_new(t_item *x, int signal_count, int type);
t_tlsig **init_tlsigs(int sig_cnt, int type, int up);
t_tlsig *init_one_tlsig(int type, int up);

inline void set_sig_val(t_tlsig *x, t_tlsmp val);//set all signal values to val
inline void scale_sig_vals(t_tlsig *x, t_tlsmp val);//scale signal x values by val
inline void multiply_sigs(t_tlsig *x, t_tlsig *y);//multiply signal x by a vector given by signal y
inline void divide_sigs(t_tlsig *x, t_tlsig *y);
inline void add_sigs(t_tlsig *x, t_tlsig *y);
inline void subtract_sigs(t_tlsig *x, t_tlsig *y);
inline void cpy_sigs(t_tlsig *x, t_tlsig *y);//copy signal y's values to signal x
inline void cpy_vecs(t_tlsig *x, t_tlsmp *vec1, int s_cnt);//copy at the sample vector level
inline void zero_out_tlsig(t_tlsig *x);

void signal_setup(t_tlsig *x, int type, int up);
void kill_tlsigs(t_tlsig **x, int sig_cnt);
void kill_one_tlsig(t_tlsig  *x);

t_tlsig *g_empty_sig;
t_tlsig **g_empty_sigs;
//this is bad!!!

//--------------------------
//--filer class

typedef struct _filer{

  FILE *fp;
  t_tlsig *sig;
  char *name;
  char *full_path;
  int frames;
  int pos;
  struct _filer *next;//sl list

}t_filer;
void init_filer_list(void);
void do_filer_list(void);
void kill_filer_list(void);

t_filer *init_filer(char *full_path);//put a new filer on the end of the list
void install_filer(t_filer *x);
void write_filer(t_filer *x);
void close_filer(t_filer *x);
void remove_filer(t_filer *x);


t_filer *g_filer_head;//just pointer to the start, unused;

/********object stuff********/
/* object data is a kludge to enable a smidge of  */
/* inheritance in this pure c api                 */

/* object data includes names and references to   */
/* signals -- every module should have one        */

/* 'free' signals (ones not part of a module)     */
/* are collected (if desired) in containers       */
/* that are of type t_obj_data -- this is how     */
/* those signals may be passed to the object      */
/* register for printing/scoping purposes         */

/* contianers are freed automatically but must be */
/* created by the user                            */
typedef struct _obj_data{

  int obj_cnt;//how many of this type of object there are
  int reg_place;
  char *type_str;
  char *name_str;
  int type;
  t_tlsig **sigs;
  int sig_cnt;
  int this_type_cnt;
  int sig_reg_ptr;
 
}t_obj_data;

t_obj_data *init_sig_container(t_tlsig **sigs, int sig_cnt);

t_obj_data g_empty_obj;

typedef struct _obj_reg{

  t_obj_data **obj_arr;
  int ref_cnt;

}t_obj_reg;

t_obj_reg *g_obj_reg;
int g_container_cnt;

void init_obj_reg(void);//this should be abstracted? (ie return or take a ptr to any obj_reg)
void install_obj(t_obj_data *x);
void empty_obj_reg(t_obj_reg *x);
void kill_obj_reg(void);

typedef struct _sig_reg_data{
  t_tlsig *sig_ptr;
  t_obj_data *obj_ptr;
  int sig_no;
}t_sig_reg_data;

//array implementation for ease of access in other files
void init_sig_reg(void);
void empty_sig_reg(void);
void kill_sig_reg(void);
t_sig_reg_data **g_sig_reg;
int g_sig_reg_cntr;

/*********circular buffer object************/
typedef struct _circ_buff{

  t_tlsmp *buff;
  t_tlsmp *blck;//ptr to output
  int len; //max size of the buffer
  int w_pos; //points to current write position
  int r_pos; //points to current read postion
  int blck_len;  //how many samples the buffer 
                 //is expected to output at a time;
  t_tlsig *feeder;
  int counter;
  int go_flag;

}t_circ_buff;

t_circ_buff *init_circ_buff(int len, int blck_len);
void kill_circ_buff_(t_circ_buff *x);
void zero_out(t_circ_buff *x);
void resize_blck(t_circ_buff *x, int blck_len);
void set_blck(t_circ_buff *x);
void fill_circ_buff(t_circ_buff *x, int s);

t_circ_buff *g_circ_buff_scope_y;
t_circ_buff *g_circ_buff_scope_z;

/*********ctl stack etc.************/
typedef struct _duple{
  t_tlsmp who;
  int who_int;
  t_tlsmp what;
}t_duple;

int g_max_stack;

typedef struct _duple_stack{
  t_duple duples[MAX_LEN];  
  int top;
}t_duple_stack;

t_duple_stack *g_duple_stack;

pthread_t g_gui_thread;
pthread_t g_read_ctl_thread;
pthread_t g_process_ctl_thread;
pthread_t g_sys_quit_thread;
int g_gui_id;
int g_read_ctl_id;
int g_process_ctl_id;
int g_read_ctl_interrupt;
int gl_scope_on_flag;

void init_duple_stack(void);
void push_duple(t_tlsmp who, t_tlsmp what);
t_duple *pop_duple(void);
void empty_duple_stack(void);
void kill_duple_stack(void);

//threaded function for reading and processing ctl input
void *read_ctl_input(void *ptr);
void process_ctl_input(void);

#define CTL_T_BANG 0
#define CTL_T_TOGGLE 1
#define CTL_T_LIN 2

/***********ctl object***********/
typedef struct _lin_ctl{

  t_tlsig *ctl_sig;
  int type;
  char *name;

  t_bang_func do_bang;
  
  int toggle_flag;

  t_tlsmp val_was;
  t_tlsmp val_is;

  int ref_num;
  int flatline_flag;
  int up;


}t_lin_ctl;


t_lin_ctl *init_lin_ctl(int ref_num, int type, int up);
void set_toggle(t_lin_ctl *x, t_tlsmp val);
void lin_ctl_interp(t_lin_ctl *x, t_tlsmp val);
void execute_lin_ctl(t_duple *x);
void flatline_lin_ctl(t_lin_ctl *x);
void kill_lin_ctl(t_lin_ctl *x);

void set_lin_ctl_val(t_lin_ctl *x, t_tlsmp val);
void zero_lin_ctl(t_lin_ctl *x);
void unity_lin_ctl(t_lin_ctl *x);
void level_lin_ctl(t_lin_ctl *x, t_tlsmp val);


typedef struct _lin_ctl_reg{

  t_lin_ctl **ctl_ptr_arr;
  int flatline_arr[MAX_LEN];//flags to see if we need to flatten out ctls
  int ref_cnt;

}t_lin_ctl_reg;

t_lin_ctl_reg *g_lin_ctl_reg;

void init_lin_ctl_reg(void);
void install_lin_ctl(t_lin_ctl *x);
void empty_lin_ctl_reg(void);
void kill_lin_ctl_reg(void);


/************dsp functions************/

//************************************
//prototype for a tl patch's dsp chain
typedef void (*t_dsp_chain)(int samples, 
			    t_tlsig **adc_sigs,
			    t_tlsig **dac_sigs);

typedef struct _dsp_chain_list{
  t_dsp_chain func;
  struct _dsp_chain_list *next;

}t_dsp_chain_list;



t_dsp_chain_list *new_dsp_chain_list(void);
void install_dsp_chain(t_dsp_chain x, t_dsp_chain_list *y);
void run_dsp_chain(t_dsp_chain x, 
		       int samples);
void empty_dsp_chain_list(t_dsp_chain_list *x);
void kill_dsp_chain_list(t_dsp_chain_list *x);	

t_dsp_chain_list *g_dsp_chain_head;

//*******************************
//kill/init function stacks
//ki for kill init

typedef void(*t_ki_func)(void);
typedef struct _ki_stack{
  t_ki_func **stack;
  int pos;
}t_ki_stack;

t_ki_stack *g_kill_stack;

t_ki_stack *init_ki_stack(void);
void push_ki(t_ki_stack *x, t_ki_func *y);
t_ki_func *pop_ki(t_ki_stack *x);
void kill_ki_stack(t_ki_stack *x);


//*******************************
//tl_so data types

typedef struct _tl_so{
  
  char *name;
  void *handle;
  t_ki_func kill;
  t_ki_func init;
  t_dsp_func dsp;

}t_tl_so;

t_tl_so *g_tl_so;

t_tl_so *init_tl_so(char *name);
void kill_tl_so(t_tl_so *x);
void tl_so_do_kill(void);//cleanup currently loaded so
void reinit_sig_reg(void);
void reinit_circ_buffs(void);
void reinit_soundout(void);
void reinit_for_new_so(void);
void load_tl_so(char *tl_name);

typedef struct _tl_so_list{//not implemented yet, ever?

  t_tl_so *x;
  struct _tl_so_list *next;

}t_tl_so_list;

t_tl_so_list *g_tl_so_list_head;

t_tl_so_list *new_tl_so_list(void);
void empty_tl_so_list(t_tl_so_list *x);
void install_tl_so_list(t_tl_so *x, t_tl_so_list *y);
void kill_tl_so_list(t_tl_so_list *x); 

//create a tl_so object and install it on the register
//push the kill and init functions
//put the dsp fucntion on the dsp_register

char *tl_path_list;
char *tl_full_path;
int tl_path_cnt;
#define TL_PATH_LEN 512
#define DEFAULT_TL_PATH "./patches/lr_test.tl"
#define TL_PATH_PD "../patches/lr_test.tl"
char *g_tl_name;//this will have to be a sl_list when multiple tl files are implemented
void cpy_file_name_no_path(char *name, char *full_path);



//////////////////////////////////////////////
void init_all(void);//this is the wrong way to do it!
void kill_structures(void);

void throw_error(char *string);//still hasn't been implemented

char *name_new(char *a_name);

t_tlsmp g_getrealtime(void);


//setup and audio processing routines:
void sched_block(void);
void dsp_tick(int samples);
void whole_chain(void);

void g_start_timelab(void);
void g_audio_on(void);
void g_audio_off(void);

//old:
void test_gui_connect(t_tlsmp val);
void kill_from_gui(void);

void *wait_for_sys_quit(void *ptr);

extern void do_gui(void);

//******************************************
//defines for convenience
#define o_sig0 o_sigs[0]->s_block
#define o_sig1 o_sigs[1]->s_block
#define o_sig2 o_sigs[2]->s_block
#define o_sig3 o_sigs[3]->s_block
#define o_sig4 o_sigs[4]->s_block
#define o_sig5 o_sigs[5]->s_block
#define o_sig6 o_sigs[6]->s_block
#define o_sig7 o_sigs[7]->s_block
#define o_sig8 o_sigs[8]->s_block
#define o_sig9 o_sigs[9]->s_block
#define o_sig10 o_sigs[10]->s_block
#define o_sig11 o_sigs[11]->s_block
#define o_sig12 o_sigs[12]->s_block
#define o_sig13 o_sigs[13]->s_block
#define o_sig14 o_sigs[14]->s_block
#define o_sig15 o_sigs[15]->s_block
#define o_sig16 o_sigs[16]->s_block
#define o_sig17 o_sigs[17]->s_block
#define o_sig18 o_sigs[18]->s_block
#define o_sig19 o_sigs[19]->s_block
#define o_sig20 o_sigs[20]->s_block
#define o_sig21 o_sigs[21]->s_block



#define i_sig0 i_sigs[0]->s_block
#define i_sig1 i_sigs[1]->s_block
#define i_sig2 i_sigs[2]->s_block
#define i_sig3 i_sigs[3]->s_block
#define i_sig4 i_sigs[4]->s_block
#define i_sig5 i_sigs[5]->s_block
#define i_sig6 i_sigs[6]->s_block
#define i_sig7 i_sigs[7]->s_block
#define i_sig8 i_sigs[8]->s_block
#define i_sig9 i_sigs[9]->s_block

#define c_sig ctl_sig->s_block

//-------------------------------
//--pd interface

void init_all_for_pd(void);
void kill_all_for_pd(void);

#ifdef __cplusplus
extern "C"
#endif

#define  __g_api_h_
#endif






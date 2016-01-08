

#ifndef tl_core_h_

#ifdef __cplusplus
extern "C" {
#endif

#include "stdio.h"
#include "math.h"

//copy/pasted from m_pd.h:
#if !defined(_SIZE_T) && !defined(_SIZE_T_)
#include <stddef.h>     /* just for size_t -- how lame! */
#endif


#define PI M_PI

#define tl_smp float // type of variable for synthesis
                   // since portaudio is all that's currently supported
                   // this is the only type we need for now

#define tl_name const char *// string to hold names

  // these are just place holders for construction
  // purposes  
  static tl_smp foo_smp = 0.0;
  static int foo_int = 0;
   //********************//
   //       tl_sig       //
   //********************//


#define tl_sig_t int //macro for categorizing signal type
#define TL_OUTLET 1 //output type
#define TL_INLET 0  //input type

  typedef struct _sig{
    
    tl_smp *smps;  // pointer to a block of samples
    int smp_cnt;     // # of samples in the block
    tl_sig_t type;  // whether this is an input or output
    int up;        // factor by which to upsample
    
  }tl_sig;

  
  // initialize one signal -- assume its an outlet
  // because inlets don't require intialization
  // 
  tl_sig *init_one_sig(int block_len, int up);
  // initialize plural signals
  tl_sig **init_sigs(int sig_cnt, tl_sig_t sig_type, int up);
  
  // destroy one signal
  void kill_one_sig(tl_sig *x);
  // destroy plural signals
  void kill_outlets(tl_sig **x, int sig_cnt);
  void kill_inlets(tl_sig **x);

  // utilities for manipulatign signals
  inline extern void set_sig_vals(tl_sig *x, tl_smp val);
  inline extern void scale_sig_vals(tl_sig *x, tl_smp *scalar);
  // should scalar be a vector perhaps?
  inline extern void multiply_sigs(tl_sig *x, tl_sig *y);
  inline extern void divide_sigs(tl_sig *x, tl_sig *y);
  inline extern void add_sigs(tl_sig *x, tl_sig *y);
  inline extern void subtract_sigs(tl_sig *x, tl_sig *y);
  inline extern void copy_sigs(tl_sig *x, tl_sig *y);
  inline extern void zero_out_sig(tl_sig *x);
  
  static tl_sig *tl_g_empty_sig;
  void tl_init_empty_sig(void);
  tl_sig *tl_get_empty_sig(void);
  void tl_kill_empty_sig(void);


  //*******************//
  //      tl_args      // 
  //*******************//

  // we actually define this later, but we need it now
  struct _processsion;
#define tl_procession struct _procession
  //typedef tl_stuff tl_procession;
  // timelab argument parser
  typedef enum{
    TL_INT,
    TL_FLOAT,
    TL_STR,
    TL_FAIL,
    TL_PROCESSION
  }tl_arg_t;
  
#define TLMAXARG 50
  typedef struct _arg {
    
    tl_arg_t type; // one of the types given above
    char str_val[256]; // this needs to be appropriately cast
    int i_val;
    float f_val;
    tl_procession *procession;
  }tl_arg;
  
  typedef struct _arglist{
    tl_arg *argv[99]; // who needs more arguments than this?
    const char *mod_name;
    int argc;
  }tl_arglist;

  //functions for parsing arguments -- this need not be in the api  
  static float do_mantissa(char *float_chars, int len);
  static int get_number(tl_arg *arg, char *num_chars, int len);
  static void got_one(tl_arglist *args, char *buff, int lent);
  void tl_parse_args(tl_arglist *x, const char *arg_str);

  //**************************//
  //      tl_class_lists      // 
  //**************************//
   
  
  // there is no tl_class prototype, it should be variable
  // this is very 'Pd style'
  
  struct _class;
#define tl_class struct _class
  //typedef tl_class *tl;

  
  // class function prototypes:
  typedef void (*tl_dsp_func)(int samples, void *mod_ptr);
  typedef void (*tl_init_func)(tl_arglist *args);
  typedef void (*tl_kill_func)(tl_class *class_ptr);

  // In timelab, classes are external containers for 
  // modules. It is a fixed structure that holds pointers
  // the functions of a module and a reference to the
  // module itself.
  // Module structures are variable, but the class structure
  // is fixed. 
  
  // class lists and helper functions
  typedef struct _class{
    
    // ptrs to the three pertinent functions 
    tl_dsp_func dsp_func;
    tl_kill_func kill_func;
    tl_init_func init_func;
    tl_arglist *args; 
    void *mod; // this is a ptr to the module for this class
    void *mod_ctls; // ptr to controls in the module
    tl_name name;
    int in_cnt;
    int out_cnt;
    struct _class *next;
    
  };
  
#define TL_GET_IN_CNT(x, type, field)\
  tl_get_in_cnt(x, (char *)(&((type *0)0)->field) - (char *)0);

  static tl_class *tl_g_class_head;
  // construct a class
  tl_class *init_class(void);
  // destroy a class
  void kill_class(tl_class *x);
  //tl_class *tl_class_setup(const char *arg_str);

  // return the class head (empty class starting the list)
  tl_class *get_g_class_head(void);
  // set an instantiated (empty) class as the head
  void set_g_class_head(tl_class *x);
  
  // push a class onto the class list
  void tl_install_class(tl_class *x, tl_class *y);
  // TODO:  
  // move class x immediately after class y
  void  tl_move_class(tl_class *x, tl_class *y);
  // move through the class list and process the dsp functions
  inline extern void tl_process_dsp_list(int samples, tl_class *x);
  // move through the list and destory all classes
  void tl_process_kill_list(tl_class *x); // kills the classes as well
  
  int tl_get_class_in_cnt(tl_class *x);
  int tl_get_class_out_cnt(tl_class *x);

  
  //*******************//
  //      tl_ctl       // 
  //*******************//
  
  // control type in timelab
#define tl_ctl_t int
#define TL_BANG_CTL 0 // bang type
#define TL_LIN_CTL 1 // linearly interpolated ctl
#define TL_HEAD_CTL 2 // list head
  // bang function is for control objects that bang
  typedef void *(*tl_bang_func)(void *arg);
  
  typedef struct _ctl{
    
    tl_ctl_t type;
    tl_name *name; // optional
    tl_bang_func bang_func; // if it is a bang,
    // define what this bang will do
    void *bang_data; // we can pass it data if we wish
    

    // this variable controls the bang function action
    int bang_go;

    // if it is linearly interpolated
    // we need to know previous value,
    // new value, and have a send outlet    
    tl_smp val_was;
    tl_smp val_is;
    tl_sig *ctl_inlet;
    tl_sig *outlet;
    
    struct _ctl *next;
    int top;
    // listable -- we need this to
    // process control on its own thread
    // and to create a special level off
    // list when we need to set the value
    // in for the next tick
    
    int is_verbose;//set this to non zero to print control
    
  }tl_ctl;
  
  tl_ctl *init_ctl(tl_ctl_t type);
  void kill_ctl(tl_ctl *x);
  
  void set_ctl_bang_data(tl_ctl *x, void *data);
  
  // utility function
  // NOTE bad nomenclature
  void set_ctl_val(tl_ctl *x, tl_smp val);
  void install_onto_ctl_list(tl_ctl *head, tl_ctl *x);
  /* // combine the above */
  /* void init_ctl_full(tl_ctl *head, */
  /* 		     tl_ctl *x, */
  /* 		     tl_smp val, */
  /* 		     int TYPE, */
  /* 		     char *buff); */

  
  static tl_ctl *tl_g_ctl_head;
  void set_g_ctl_head(tl_ctl *x);
  tl_ctl *get_g_ctl_head(void);
  void tl_kill_ctl_list(tl_ctl *head);
  
  // stack for leveling off when control has
  // been interpolated
#define MAX_CTL 9999
  typedef struct _lvl_stck{
    
    tl_ctl **ctls;
    int top;
    
  }tl_lvl_stck;
  
  inline extern void interpolate_ctl_val(tl_ctl *x, tl_lvl_stck *lvl_stck);

  // needs to be intialized
  static tl_lvl_stck *tl_g_lvl_stck;
  tl_lvl_stck *init_lvl_stck(void);
  void kill_lvl_stck(tl_lvl_stck *x);
  inline extern void push_lvl_stck(tl_lvl_stck *x, tl_ctl *y);
  inline extern tl_ctl *pop_lvl_stck(tl_lvl_stck *x);
  void flush_lvl_stck(tl_lvl_stck *x);
  inline extern void process_lvl_stck(tl_lvl_stck *x);
  
  void set_g_lvl_stck(tl_lvl_stck *x);
  inline extern tl_lvl_stck * get_g_lvl_stck(void);

  // this belongs above, but we hadn't defined tl_lvl_stck yet
  inline extern void process_ctl_list(tl_ctl *head, tl_lvl_stck *lvl_stck);

  //*************************//
  //      global audio       //
  //*************************//
  // these structures are provided for
  // global input and output
  
  // buffer for reading from adc or writing to dac
  // this is a bus to and from timelab

#define TL_MAXCHANNS 32
  typedef struct _audio_buff{
    
    tl_smp *buff;
    int block_len;
    int channs;
    int buff_len;
    int buff_pos_w;
    int buff_pos_r;
    
  }tl_audio_buff;
  
  // one universal static instance of each
  static tl_audio_buff *tl_g_audio_buff_out;
  static tl_audio_buff *tl_g_audio_buff_in;
  static int tl_g_out_chann_cnt;
  static int tl_g_in_chann_cnt;
  
  tl_audio_buff *init_audio_buff(int channs);
  void reset_audio_buff(tl_audio_buff *x);
  void resize_audio_buff_channs(tl_audio_buff *x, int channs);
  void resize_audio_buff_block_len(tl_audio_buff *x); 
  // call the above if g_block_len has changed!
  
  void kill_audio_buff(tl_audio_buff *x);
  
  void set_g_audio_buff_out(tl_audio_buff *x);
  void set_g_audio_buff_in(tl_audio_buff *x);
  tl_audio_buff *get_g_audio_buff_out(void);
  tl_audio_buff *get_g_audio_buff_in(void);
  
  // NOTE: these may be redundant now! fixme
  void set_g_out_chann_cnt(int cnt);
  void set_g_in_chann_cnt(int cnt);
  int get_g_out_chann_cnt(void);
  int get_g_in_chann_cnt(void);
  
  //*****************************************//
  //  scheduling and housekeeping functions  //
  //*****************************************//

  // conglomerate structure for class head, ctl head, and lvl_stck

  typedef struct _procession{
    tl_class *class_head;
    tl_ctl *ctl_head;
    tl_lvl_stck *lvl_stck;
    // todo: implement this, requires fundamental
    // restructuring
    tl_audio_buff *ab_in;
    tl_audio_buff *ab_out;
  };

  void kill_procession(tl_procession *x);
  tl_procession *init_procession(void);  

  
  static void tl_dsp_tick(tl_procession *x);
  extern void tl_audio_on(tl_procession *x);
  extern void tl_audio_off(void);
  extern void tl_audio_suspend(void);
#define DSP_OFF 0
#define DSP_ON 1
#define TLTRUE 1
#define TLFALSE 0
  static int tl_audio_initialized;
  static int tl_g_dsp_status = DSP_OFF;
  int is_dsp_on(void);
  void tl_dsp_on(void);
  void tl_dsp_off(void);
  
  int tl_get_samplerate(void); // returns the samplerate
  int tl_get_block_len(void); // returns block_len
  
  void tl_set_samplerate(int samplerate); // sets the samplerate
  void tl_set_block_len(int block_len); // sets block_len

  // TODO: change name to tl_name_new
  tl_name name_new(char *a_name);

  // from pd:
  void *resizebytes(void *old, size_t oldsize, size_t newsize);
  
  // TODO: utilize these:
  typedef struct _a_info{
    
    int sr;
    int block_len;
    int indevno;
    int outdevno;
    int inchanns;
    int outchanns;
    float latency;
    
  }tl_a_info;
  
  static tl_a_info tl_g_a_info;
  void tl_set_a_info(int sr, int blck, int indevno, int outdevno, int inchanns, int outchanns, float ltncy);
  tl_a_info tl_get_a_info(void);

  static int tl_g_samplerate;// = 44100;
  static int tl_g_block_len;// = 64;


  //******************//
  //  module loading  //
  //******************//
  tl_name cpy_file_name_no_path(char *full_path);
  tl_class *tl_load_module(tl_procession *procession, const char *arg_str);  

#ifdef __cplusplus
}
#endif

#define tl_core_h_
#endif



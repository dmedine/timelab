
#ifndef __m_modules_h_


#ifdef __cplusplus
extern "C" {
#endif

#include "tl_core.h"

    //*************//
    //     ADC     //
    //*************//
  typedef struct _adc{
    
    /* tl_dsp_func dsp_func; */
    /* tl_kill_func kill_func; */
    
    tl_sig **outlets;
    int out_cnt;
    int in_cnt;

    int sr;
    tl_audio_buff *ab;
    
    tl_name *name;
    
  }tl_adc;
  inline void tl_dsp_adc(int samples);
  // this is special because there is only one dac
  void tl_init_adc(int in_cnt, int up);
  void tl_kill_adc(void);
  void set_g_adc_in(int in, tl_sig *x);
  tl_adc *tl_get_adc(void);
  static tl_adc *tl_g_adc;
    

    //*************//
    //     DAC     //
    //*************//

  //int tl_dac_cnt = 0;
  typedef struct _dac{
    
    /* tl_dsp_func dsp_func; */
    /* tl_kill_func kill_func; */
    
    tl_sig **inlets;
    int in_cnt;
    int out_cnt;    
    int sr;
    tl_audio_buff *ab;
    
    tl_name *name;
    
  }tl_dac;
  
  inline void tl_dsp_dac(int samples);
  // this is special because there is only one dac
  void tl_init_dac(int in_cnt, int up);
  void tl_kill_dac(void);
  void set_g_dac_in(int in, tl_sig *x);
  tl_dac *tl_get_dac(void);
  static tl_dac *tl_g_dac;
  
  
  //***************//
  //     table     //
  //***************//
  
  //int tl_table_cnt = 0;
  typedef struct _table{
    
    /* tl_dsp_func dsp_func; */
    /* tl_kill_func kill_func; */
    
    tl_sig **inlets;
    int in_cnt;
    tl_sig **outlets;
    int out_cnt;
    
    int sr;
    int up;
    
    tl_smp *table_array;
    int grain;
    
  }tl_table;
  
  inline void tl_dsp_table(int samples, void *mod);
  void create_table_array(tl_table *x);
  void tl_kill_table(void *mod);
  void *tl_init_table(int table_len, int up);
  //tl_class *tl_init_setup(void);
  
  //****************//
  //     lookup     //
  //****************//
  
  //int tl_lookup_cnt = 0;
  typedef struct _lookup{
    
    /* tl_kill_func kill_func; */
    /* tl_dsp_func dsp_func; */
    
    tl_sig **inlets;
    int in_cnt;
    tl_sig **outlets;
    int out_cnt;
    
    int up;
    int sr;
    int denom;
    
    tl_smp phase;
    tl_smp freq;
    tl_smp phase_inc;
    
  }tl_lookup;
  
  inline void tl_dsp_lookup(int samples, void *mod);
  void *tl_init_lookup(int up);
  void tl_kill_lookup(void *mod);

  //********************//
  //     UDS solver     //
  //********************//

#define MAX_UDS_DATA 50
  // function prototype for calculating one node of a UDS network

  struct _UDS_node;
#define tl_UDS_node struct _UDS_node
  typedef tl_smp (*tl_dyfunc)(tl_UDS_node *x, int iter);
  // listified
  typedef struct _UDS_node{
    tl_dyfunc func;
    tl_smp *ks;
  
    tl_smp **data_in;
    int in_cnt;
    tl_smp data_out[1]; // only one output
    /* tl_sigs **ins; */
    /* tl_sigs **outs; */
    /* int in_cnt; */
    /* int out_cnt; */
    tl_ctl **ctls;
    int ctl_cnt;
    tl_smp dx;
    tl_smp state;
    tl_smp out;
    tl_smp reset_state;

    int stage; // which stage in the lopp

    struct _UDS_node *next;
    tl_name name;
  };


  // 'constructor' and 'destructor'
  tl_UDS_node *tl_init_UDS_node(tl_dyfunc func, int in_cnt, int ctl_cnt, int up);
  void tl_reset_UDS_node(tl_UDS_node *x, tl_smp state);
  void tl_push_UDS_node(tl_UDS_node *x, tl_UDS_node *y);
  void tl_kill_UDS_node(tl_UDS_node *x);
  void tl_kill_UDS_net(tl_UDS_node *x);

  // dysolver module
  typedef struct _UDS_solver {

    tl_UDS_node *UDS_net; // head to the list
    tl_class *mother;    
    tl_sig **inlets; // input to the whole segment
    tl_sig **outlets; // final output
    int in_cnt;
    int out_cnt; 

    int up;
    int h;
    tl_smp h_time;
    tl_smp half_h_time;
    tl_smp mult[4];
    tl_smp one_sixth;
    tl_name name;

  }tl_UDS_solver;

  inline void tl_dsp_UDS_solver(int samples, void *mod);
  void *tl_init_UDS_solver(int ins, int outs, int up);
  void tl_kill_UDS_solver(void *mod);

  
  //**********************//
  //     housekeeping     //
  //**********************//
  
  static int dac_cnt;
  static int table_cnt;
  static int lookup_cnt;
  
  void initialize_modules(void);


  
#ifdef __cplusplus
}
#endif

#define __m_modules_h_
#endif



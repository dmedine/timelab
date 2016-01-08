
#ifndef __m_modules_h_


#ifdef __cplusplus
extern "C" {
#endif

#include "tl_core.h"

    //*************//
    //     DAC     //
    //*************//

  //int tl_dac_cnt = 0;
  typedef struct _dac{
    
    tl_dsp_func dsp_func;
    tl_kill_func kill_func;
    
    tl_sig **inlets;
    int in_cnt;
    
    int sr;
    tl_audio_buff *ab;
    
    tl_name *name;
    
  }tl_dac;
  
  inline void tl_dsp_dac(int samples, void *mod);
  // this is special because there is only one dac
  void tl_init_dac(int in_cnt, int up);
  void tl_kill_dac(void);
  void set_g_dac_in(int in, tl_sig *x);
  tl_dac *tl_get_dac(void);
  static tl_dac *tl_g_dac;
  
  
  /* //\***************\// */
  /* //     table     // */
  /* //\***************\// */
  
  /* //int tl_table_cnt = 0; */
  /* typedef struct _table{ */
    
  /*   tl_dsp_func dsp_func; */
  /*   tl_kill_func kill_func; */
    
  /*   tl_sig **inlets; */
  /*   int in_cnt; */
  /*   tl_sig **outlets; */
  /*   int out_cnt; */
    
  /*   int sr; */
  /*   int up; */
    
  /*   tl_smp *table_array; */
  /*   int grain; */
    
  /* }tl_table; */
  
  /* inline void tl_dsp_table(int samples, void *mod); */
  /* void create_table_array(tl_table *x); */
  /* void tl_kill_table(void *mod); */
  /* void *tl_init_table(int table_len, int up); */
  /* tl_class *tl_init_setup(void); */
  
  /* //\****************\// */
  /* //     lookup     // */
  /* //\****************\// */
  
  /* //int tl_lookup_cnt = 0; */

  
  /* //\**********************\// */
  /* //     housekeeping     // */
  /* //\**********************\// */
  
  /* static int dac_cnt; */
  /* static int table_cnt; */
  /* static int lookup_cnt; */
  
  /* void initialize_modules(void); */
  
#ifdef __cplusplus
}
#endif

#define __m_modules_h_
#endif



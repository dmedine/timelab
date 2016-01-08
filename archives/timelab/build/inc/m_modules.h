#ifndef __m_modules_h_

#include "g_api.h"


///////////////////////
/*******dac***********/

int g_dac_cnt;
typedef struct _dac{

  t_dsp_func dsp_func;

  struct _tlsig **i_sigs;//no output
  int i_sig_cnt;

  int up;

  t_obj_data od;

}t_dac;

void dac_dsp_loop(int samples, t_dac *x);
void *dac_init(int channels);
void kill_dac(t_dac *x);

///////////////////////
/*******adc***********/

int g_adc_cnt;
typedef struct _adc{

  t_dsp_func dsp_func;
  int up;
  struct _tlsig **o_sigs;//no output
  int o_sig_cnt;

  t_obj_data od;

}t_adc;

void adc_dsp_loop(int samples, t_adc *x);
void *adc_init(int channels);
void kill_adc(t_adc *x);

t_dac *g_dac;
t_adc *g_adc;


//////////////////////////
/*********table**********/
int g_table_cnt;
typedef struct _table{

  t_dsp_func dsp_func;

  t_tlsig **o_sigs;
  int o_sig_cnt;  
  t_tlsig **i_sigs;
  int i_sig_cnt;
  
  int up;
  
  t_tlsmp *vals;
  int grain;

  t_obj_data od;

}t_table;


void *table_init(int grain, int up);
void kill_table(t_table *x);
void create_table(t_table *x);
void table_dsp_loop(int samples, t_table *x);


/////////////////////////////
/********lookup************/
int g_lookup_cnt;
typedef struct _lookup{

  t_dsp_func dsp_func;

  struct _tlsig **o_sigs;
  int o_sig_cnt; 

  struct _tlsig **i_sigs;
  int i_sig_cnt;

  int up;

  t_tlsmp phase;
  t_tlsmp phase_inc;
  t_tlsmp sr;
  t_tlsmp freq;
  t_tlsmp freq_was;
  t_tlsig *freq_vec;

  t_obj_data od;
  
}t_lookup;

void *lookup_init(int up);
void kill_lookup(t_lookup *x);
void lookup_dsp_loop(int samples, t_lookup *x);

//global variables
t_tlsmp global_phase;

//////////////////////////
/********noise***********/

int g_noise_cnt;
typedef struct _noise{


  t_dsp_func dsp_func;
  struct _tlsig **o_sigs;
  int o_sig_cnt;

  t_obj_data od;

  int up;
   

}t_noise;

void *noise_init(int up);
void kill_noise(t_noise *x);
void noise_dsp_loop(int samples, t_noise *x);


/////////////////////////////
/********dwnsample************/
/* downsampling algorithm -- takes every hth element of the  */
/* input and puts it in the output */

int g_dwnsample_cnt;
typedef  struct _dwnsample{
 
  t_dsp_func dsp_func;

  struct _tlsig **o_sigs;
  int o_sig_cnt;

  struct _tlsig **i_sigs;
  int i_sig_cnt; 

  struct _tlsig **dummy_sigs;
  
  t_tlsmp *av_buff;

  int up;//h*SR = input rate
  int block_len;
  
  int avg_fact;//number of samples over which to average the new sample
  t_tlsmp inv_avg_fact;//floating point inverse of above

  t_obj_data od;

}t_dwnsample;

void *dwnsample_init(int sig_cnt, int up);
void  kill_dwnsample(t_dwnsample *x);
void dwnsample_dsp_loop(int samples, t_dwnsample *x);


//////////////////////////////
/********upsample************/

int g_upsample_cnt;
typedef struct _upsample{

  t_dsp_func dsp_func;

  struct _tlsig **i_sigs;
  int i_sig_cnt;

  struct _tlsig **o_sigs;
  int o_sig_cnt;
 
  t_tlsmp *last_tlsmp;
  t_tlsmp diff;
  t_tlsmp step;
  int up; 

  t_obj_data od;
  
}t_upsample;

void *upsample_init(int sig_cnt, int up);
void kill_upsample(t_upsample *x);
void upsample_dsp_loop(int samples, t_upsample *x);


///////////////////////////////////////
/**********lti_filt*******************/
/* this filter is linear and time invariant                 */
/* once you set it up, that's it                            */
/* it is meant to be helpful in implementing                */
/* matlab/octave examples (of which there are far too many) */
/* of dsp algorithms                                        */
/* thus it resembles the octave/matlab function filt        */
/*                                                          */
/* computation is done in the time domain directly          */

int g_lti_filt_cnt;
typedef struct _lti_filt{

  t_dsp_func dsp_func;

  int up;
  
  struct _tlsig **i_sigs;
  int i_sig_cnt;

  struct _tlsig **o_sigs;
  int o_sig_cnt;

  int len;
  
  t_tlsmp *b;
  t_tlsmp *a;
  t_tlsmp *w;

  t_obj_data od;

}t_lti_filt;

void *lti_filt_init(int len, t_tlsmp *a, t_tlsmp *b);
void kill_lti_filt(t_lti_filt *x);
inline t_tlsmp lti_filt(t_lti_filt *x, t_tlsmp input);//non dsp loop version for rolling into other modules
void lti_filt_dsp_loop(int samples, t_lti_filt *x);
void lti_filt_clear(t_lti_filt *x);

/////////////////////////////////////////////////////
/***********digital wave guide module***************/
#define DWG_LIM 1000
int g_dwg_cnt;
typedef struct _dwg{

  t_dsp_func dsp_func;

  int up;
  
  struct _tlsig **i_sigs;
  int i_sig_cnt;

  struct _tlsig **o_sigs;
  int o_sig_cnt;

  t_tlsmp *l_wave;
  t_tlsmp *r_wave;//buffers for left and right going waves

  int lg_0;
  int lg_L;
  int lg_rd;

  int rg_0;
  int rg_L;
  int rg_rd;//ptrs to the appropriate places in the dwg buffers

  int L;//length of waveguide in samples

  t_obj_data od;

}t_dwg;

void *dwg_init(void);
void kill_dwg(t_dwg *x);
void dwg_dsp_loop(int samples, t_dwg *x); 


typedef struct _mix_bus{

  t_dsp_func dsp_func;

  struct _tlsig **i_sigs;
  int i_sig_cnt;

  struct _tlsig **o_sigs;
  int o_sig_cnt;

  int up;

}t_mix_bus;

void *mix_bus_init(int ins, int up);
void kill_mix_bus(t_mix_bus *x);
void mix_bus_dsp_loop(int samples, t_mix_bus *x);


typedef struct _diff{

  t_dsp_func dsp_func;

  int channs;

  struct _tlsig **i_sigs;
  struct _tlsig **o_sigs;
  
  t_tlsmp *danglers;

  int up;
  t_obj_data od;
  
}t_diff;

void *diff_init(int channs, int up);
void kill_diff(t_diff *x);
void diff_dsp_loop(int samples, t_diff *x);




#define  __m_modules_h_
#endif

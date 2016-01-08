#ifndef __m_ode_prim_h_

#include "g_api.h"
int g_rk_cnt;
typedef t_tlsmp(*t_rk_child_func_ptr)(t_tlsmp *data);
typedef void(*t_rk_mother_func_ptr)(int samples, void *ptr, void *ctl_data);
/*--------------------------
  child*/
typedef struct _rk_child{

  t_tlsmp input;

  int up;//upsample factor
  int h;//step size -- a factor of sample size
  t_tlsmp h_time;//same value in seconds
  t_tlsmp half_h_time;

  t_tlsmp gain;

  t_tlsmp *ks;
  t_tlsmp state;
  t_tlsmp data; 


  t_tlsmp mult[4];

  t_tlsmp scalar;
  t_tlsmp offset;

  int block_len;

}t_rk_child;




/*-------------------------
  mother*/
typedef struct _rk_mother{
 
  t_rk_mother_func_ptr dsp_func;
  
  struct _tlsig **o_sigs;
  struct _tlsig **i_sigs;
  struct _tlsig **c_sigs;

  struct _tlsig **empty_sigs;

  t_rk_child **rk_children;
  int i_sig_cnt;
  int o_sig_cnt;
  int c_sig_cnt;
  int child_cnt;

  int up;
  int h;
  t_tlsmp half_h_time;
  t_tlsmp h_time;

  t_obj_data od;

}t_rk_mother;



t_rk_mother *rk_mother_init(void *dsp_func,
			    int child_cnt,
			    int c_sig_cnt,
			    int h,
			    int up);

void kill_rk_mother(t_rk_mother *x);

t_rk_child *rk_child_init(t_rk_mother *y, int h);

void kill_rk_child(t_rk_child *x);

inline void rk_child_estimate(t_rk_child *x);
inline t_tlsmp rk_child_stage(int stage_num,//which stage in the rg algorithm 
			   t_rk_child *x);

/***************implicit Euler method***************/

int g_ie_cnt;
typedef t_tlsmp(*t_ie_child_func_ptr)(t_tlsmp *data);
typedef void(*t_ie_mother_func_ptr)(int samples, void *ptr, void *ctl_data);
/*--------------------------
  child*/
typedef struct _ie_child{

  t_tlsmp input;

  int up;//upsample factor
  int h;//step size -- a factor of sample size
  t_tlsmp h_time;//same value in seconds
  t_tlsmp half_h_time;

  t_tlsmp gain;

  t_tlsmp y_k;//current value
  t_tlsmp y_k01;//first guess
  t_tlsmp y_k11;//iterated guess
  t_tlsmp diff;
  t_tlsmp tol;

  /*****************************/
  /* the scheme is:            */
  /* initially: y_k01 = y_k    */
  /* then iterate:             */
  /* y_k11 = y_k + h(f(y_k01), */
  /* diff = y_k11 - y_k01,     */ 
  /* y_k01 = y_k11,            */
  /* keep going until:         */
  /* diff>= tol                */
  /*****************************/

  t_tlsmp state;
  t_tlsmp data; 

  t_tlsmp scalar;
  t_tlsmp offset;

  int block_len;
  int it_cnt;

}t_ie_child;




/*-------------------------
  mother*/
typedef struct _ie_mother{
 
  t_ie_mother_func_ptr dsp_func;
  
  struct _tlsig **o_sigs;
  struct _tlsig **i_sigs;
  struct _tlsig **c_sigs;

  struct _tlsig **empty_sigs;

  t_ie_child **ie_children;
  int i_sig_cnt;
  int o_sig_cnt;
  int c_sig_cnt;
  int child_cnt;

  int up;
  int h;
  t_tlsmp half_h_time;
  t_tlsmp h_time;
  t_tlsmp tol;

  t_obj_data od;

}t_ie_mother;



t_ie_mother *ie_mother_init(void *dsp_func,
			    int child_cnt,
			    int c_sig_cnt,
			    int h,
			    int up);

void kill_ie_mother(t_ie_mother *x);

t_ie_child *ie_child_init(t_ie_mother *y, int h);

void kill_ie_child(t_ie_child *x);

void ie_child_iterate(t_ie_child *x);
void ie_child_begin(t_ie_child *x);


/***************semi-implicit Euler method***************/

int g_sie_cnt;
typedef t_tlsmp(*t_sie_child_func_ptr)(t_tlsmp *data);
typedef void(*t_sie_mother_func_ptr)(int samples, void *ptr, void *ctl_data);
/*--------------------------
  child*/
typedef struct _sie_child{

  t_tlsmp input;

  int up;//upsample factor
  int h;//step size -- a factor of sample size
  t_tlsmp h_time;//same value in seconds
  t_tlsmp half_h_time;

  t_tlsmp gain;

  t_tlsmp vn;
  t_tlsmp vn1;
  t_tlsmp xn;
  t_tlsmp xn1;
  t_tlsmp diff;
  t_tlsmp tol;
  t_tlsmp k;

  /*****************************/
  /* the scheme is:            */
  /* dx/dt = v(t)              */
  /* dv/dt = f(t)              */
  /* vn1 = vn + h*f(x)         */
  /* xn1 = xn + h*vn1          */ 
  /*****************************/

  t_tlsmp state;
  t_tlsmp data; 

  t_tlsmp scalar;
  t_tlsmp offset;

  int block_len;
  int it_cnt;

}t_sie_child;




/*-------------------------
  mother*/
typedef struct _sie_mother{
 
  t_sie_mother_func_ptr dsp_func;
  
  struct _tlsig **o_sigs;
  struct _tlsig **i_sigs;
  struct _tlsig **c_sigs;

  struct _tlsig **empty_sigs;

  t_sie_child **sie_children;
  int i_sig_cnt;
  int o_sig_cnt;
  int c_sig_cnt;
  int child_cnt;

  int up;
  int h;
  t_tlsmp half_h_time;
  t_tlsmp h_time;
  t_tlsmp tol;

  t_obj_data od;

}t_sie_mother;



t_sie_mother *sie_mother_init(void *dsp_func,
			    int child_cnt,
			    int c_sig_cnt,
			    int h,
			    int up);

void kill_sie_mother(t_sie_mother *x);

t_sie_child *sie_child_init(t_sie_mother *y, int h);

void kill_sie_child(t_sie_child *x);
void sie_child_iterate(t_sie_child *x);
void sie_child_advance(t_sie_child *x);




/***************central difference method***************/
typedef t_tlsmp(*t_cd_child_func_ptr)(t_tlsmp *data);
typedef void(*t_cd_mother_func_ptr)(int samples, void *ptr, void *ctl_data);

int g_cd_cnt;
/*--------------------------
  child*/
typedef struct _cd_child{

  t_tlsmp input;

  int up;//
  int h;//step size -- a factor of sample size
  t_tlsmp h_time;//same value in seconds
  t_tlsmp half_h_time;
  t_cd_child_func_ptr diff_func;
  t_tlsmp gain;


  int coeff_cnt;
  t_tlsmp *sides;
  t_tlsmp *coeffs;
  t_tlsmp state;
  t_tlsmp data; 

  t_tlsmp scalar;
  t_tlsmp offset;

  int block_len;
}t_cd_child;


/*-------------mother*/
typedef struct _cd_mother{
 
  t_cd_mother_func_ptr dsp_func;
  
  struct _tlsig **o_sigs;
  struct _tlsig **i_sigs;
  struct _tlsig **c_sigs;

  struct _tlsig **empty_sigs;

  t_cd_child **cd_children;
  int i_sig_cnt;
  int o_sig_cnt;
  int c_sig_cnt;
  int child_cnt;
  
  int up;
  int h;
  t_tlsmp half_h_time;
  t_tlsmp h_time;

  t_obj_data od;

}t_cd_mother;



t_cd_mother *cd_mother_init(void *dsp_func,
			    int child_cnt,
			    int c_sig_cnt,
			    int h,
			    int up);

void kill_cd_mother(t_cd_mother *x);

t_cd_child *cd_child_init(t_cd_mother *y, int h);

void kill_cd_child(t_cd_child *x);

t_tlsmp cd_child_stage(int stage_num, t_cd_child *x);
void cd_child_estimate(t_cd_child *x);

/***************2nd order central difference method***************/
typedef t_tlsmp(*t_cd2_child_func_ptr)(t_tlsmp *data);
typedef void(*t_cd2_mother_func_ptr)(int samples, void *ptr, void *ctl_data);

int g_cd2_cnt;
/*--------------------------
  child*/
typedef struct _cd2_child{

  t_tlsmp input;

  int up;//
  int h;//step size -- a factor of sample size
  t_tlsmp h_time;//same value in seconds
  t_tlsmp half_h_time;
  t_cd2_child_func_ptr diff_func;
  t_tlsmp gain;


  int coeff_cnt;
  t_tlsmp *sides;
  t_tlsmp *coeffs;
  t_tlsmp state;
  t_tlsmp data; 

  t_tlsmp scalar;
  t_tlsmp offset;

  int block_len;
}t_cd2_child;


/*-------------mother*/
typedef struct _cd2_mother{
 
  t_cd2_mother_func_ptr dsp_func;
  
  struct _tlsig **o_sigs;
  struct _tlsig **i_sigs;
  struct _tlsig **c_sigs;

  struct _tlsig **empty_sigs;

  t_cd2_child **cd2_children;
  int i_sig_cnt;
  int o_sig_cnt;
  int c_sig_cnt;
  int child_cnt;
  
  int up;
  int h;
  t_tlsmp half_h_time;
  t_tlsmp h_time;

  t_obj_data od;

}t_cd2_mother;



t_cd2_mother *cd2_mother_init(void *dsp_func,
			      int child_cnt,
			      int c_sig_cnt,
			      int h,
			      int up);

void kill_cd2_mother(t_cd2_mother *x);

t_cd2_child *cd2_child_init(t_cd2_mother *y, int h);

void kill_cd2_child(t_cd2_child *x);

t_tlsmp cd2_child_stage(int stage_num, t_cd2_child *x);
void cd2_child_estimate(t_cd2_child *x);


/***************nsv (semi-implicit Euler) method***************/

typedef t_tlsmp(*t_nsv_child_func_ptr)(t_tlsmp *data);
typedef void(*t_nsv_mother_func_ptr)(int samples, void *ptr, void *ctl_data);

int g_nsv_cnt;
/*--------------------------
  child*/
typedef struct _nsv_child{

  t_tlsmp input;

  int up;
  int h;//step size -- a factor of sample size
  t_tlsmp h_time;//same value in seconds
  t_tlsmp half_h_time;
  t_nsv_child_func_ptr diff_func;
  t_tlsmp gain;

  t_tlsmp dx;
  t_tlsmp state;

  t_tlsmp scalar;
  t_tlsmp offset;

  int block_len;

}t_nsv_child;


/*-------------mother*/
typedef struct _nsv_mother{
 
  t_nsv_mother_func_ptr dsp_func;
  
  struct _tlsig **o_sigs;
  struct _tlsig **i_sigs;
  struct _tlsig **c_sigs;

  struct _tlsig **empty_sigs;

  t_nsv_child **nsv_children;
  int i_sig_cnt;
  int o_sig_cnt;
  int c_sig_cnt;
  int child_cnt;

  int up;
  int h;
  t_tlsmp half_h_time;
  t_tlsmp h_time;

  t_obj_data od;

}t_nsv_mother;



t_nsv_mother *nsv_mother_init(void *dsp_func,
			      int child_cnt,
			      int c_sig_cnt,
			      int h,
			      int up);

void kill_nsv_mother(t_nsv_mother *x);

t_nsv_child *nsv_child_init(t_nsv_mother *y, int h);

void kill_nsv_child(t_nsv_child *x);

void nsv_child_estimate(t_nsv_child *x);


/***************(naive) b-series attempt method***************/

typedef t_tlsmp(*t_bser_child_func_ptr)(t_tlsmp *data);
typedef void(*t_bser_mother_func_ptr)(int samples, void *ptr, void *ctl_data);

int g_bser_cnt;
/*--------------------------
  child*/
typedef struct _bser_child{

  int up;
  int h;//step size -- a factor of sample size
  t_tlsmp h_time;//same value in seconds
  t_tlsmp half_h_time;
 
  t_tlsmp *fs;
  t_tlsmp *mult;
  t_tlsmp state;

}t_bser_child;


/*-------------mother*/
typedef struct _bser_mother{
 
  t_bser_mother_func_ptr dsp_func;
  
  struct _tlsig **o_sigs;
  struct _tlsig **i_sigs;
  struct _tlsig **c_sigs;

  struct _tlsig **empty_sigs;

  t_bser_child **bser_children;
  int i_sig_cnt;
  int o_sig_cnt;
  int c_sig_cnt;
  int child_cnt;

  int up;
  int h;
  t_tlsmp half_h_time;
  t_tlsmp h_time;

  t_obj_data od;

}t_bser_mother;



t_bser_mother *bser_mother_init(void *dsp_func,
				int child_cnt,
				int c_sig_cnt,
				int h,
				int up);

void kill_bser_mother(t_bser_mother *x);

t_bser_child *bser_child_init(t_bser_mother *y, int h);

void bser_child_stage(int stage_num, t_bser_child *x, t_tlsmp *out);
t_tlsmp bser_child_stage1(int stage_num, t_bser_child *x);
t_tlsmp bser_child_stage2(int stage_num, t_bser_child *x);
void kill_bser_child(t_bser_child *x);

void bser_child_estimate(t_bser_child *x);



/***************leafrog method***************/

typedef t_tlsmp(*t_lf_child_func_ptr)(t_tlsmp *data);
typedef void(*t_lf_mother_func_ptr)(int samples, void *ptr, void *ctl_data);

int g_lf_cnt;
/*--------------------------
  child*/
typedef struct _lf_child{

  int up;
  int h;//step size -- a factor of sample size
  t_tlsmp h_time;//same value in seconds
  t_tlsmp half_h_time;
  t_tlsmp h_time_sqrd;
 
  t_tlsmp xi, vi;
  t_tlsmp ai, ai1;

}t_lf_child;


/*-------------mother*/
typedef struct _lf_mother{
 
  t_lf_mother_func_ptr dsp_func;
  
  struct _tlsig **o_sigs;
  struct _tlsig **i_sigs;
  struct _tlsig **c_sigs;

  struct _tlsig **empty_sigs;

  t_lf_child **lf_children;
  int i_sig_cnt;
  int o_sig_cnt;
  int c_sig_cnt;
  int child_cnt;

  int up;
  int h;
  t_tlsmp half_h_time;
  t_tlsmp h_time;

  t_obj_data od;

}t_lf_mother;



t_lf_mother *lf_mother_init(void *dsp_func,
			    int child_cnt,
			    int c_sig_cnt,
			    int h,
			    int up);

void kill_lf_mother(t_lf_mother *x);

t_lf_child *lf_child_init(t_lf_mother *y, int h);

t_tlsmp lf_child_stage(int stage_num, t_lf_child *x);
void kill_lf_child(t_lf_child *x);

void lf_child_estimate_x(t_lf_child *x);
void lf_child_estimate_v(t_lf_child *x);

#define __m_ode_prim_h_
#endif

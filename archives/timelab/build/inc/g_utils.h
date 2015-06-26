#ifndef __g_utils_h_

#include "g_api.h"

//math utils
t_tlsmp clip_scale(t_tlsmp val, t_tlsmp hi, t_tlsmp lo);
t_tlsmp limiter(t_tlsmp val, t_tlsmp hi, t_tlsmp lo);

//t_tlsmp tanh_clip_scale(t_tlsmp val, t_tlsmp gain,
//			 t_tlsmp hi, t_tlsmp lo);
t_tlsmp tanh_clip(t_tlsmp val, t_tlsmp gain);

//signal utils
void sig_multiply(t_tlsig *x, t_tlsig *y, int s);
//multiply the values of signal x by y, store products in x
void sig_divide(t_tlsig *x, t_tlsig *y, int s);
void sig_add(t_tlsig *x, t_tlsig *y, int s);
void sig_subtract(t_tlsig *x, t_tlsig *y, int s);
void sig_replace(t_tlsig *x, t_tlsig *y, int s);


#define __g_utils_h_
#endif

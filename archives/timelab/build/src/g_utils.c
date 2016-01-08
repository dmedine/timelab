#include "g_utils.h"
#include "math.h"

t_tlsmp clip_scale(t_tlsmp val, t_tlsmp lo, t_tlsmp hi){

  t_tlsmp out;

  if(val > 1) val = 1;
  else if(val < -1) val = -1;
  else;

  out = 0.5 + .66667 * (val - (1./3.) * val * val *val);
  return(lo + (hi - lo) * out);

}

t_tlsmp limiter(t_tlsmp val, t_tlsmp lo, t_tlsmp hi){

  if(val>hi)
    return( - (val-hi) * (val-hi));
  else if (val < lo)
    return((val + lo) * (val +lo));
  else return(0);

}


t_tlsmp tanh_clip(t_tlsmp val, t_tlsmp gain){

  return(tanh(gain * val));

}

void sig_multiply(t_tlsig *x, t_tlsig *y, int s){

  int i;
  for(i=0; i<s; i++)
    x->s_block[i] *= y->s_block[i];

}

void sig_divide(t_tlsig *x, t_tlsig *y, int s){

  int i;
  for(i=0; i<s; i++)
    x->s_block[i] /= y->s_block[i];

}

void sig_add(t_tlsig *x, t_tlsig *y, int s){

  int i;
  for(i=0; i<s; i++)
    x->s_block[i] += y->s_block[i];

}

void sig_subtract(t_tlsig *x, t_tlsig *y, int s){

  int i;
  for(i=0; i<s; i++)
    x->s_block[i] -= y->s_block[i];

}	

void sig_replace(t_tlsig *x, t_tlsig *y, int s){

  int i;
  for(i=0; i<s; i++)
    x->s_block[i] = y->s_block[i];

}	

#include "stdio.h"
#include "math.h"
#include "stdlib.h"
#include "g_api.h"
#include "m_modules.h"
#include "pthread.h"
//patch to emulate an analog string
//each spatial step is equal to the 
//speed of sound * sample time
//or, v*s_t
//where v = sqrt(T/mu)
//were T is tension and mu is density
//this reduces to the wave equation


//mass spring structure
//dll? head and tail point to
//reflecting terminations

//----------------------------------------------------
//NOTE:
//modelling the string this way ensures (perhaps)
//against aliasing as the limited number of available
//masses in shorter strings limits the 
//number of available modes that the 
//string can oscillate at
//----------------------------------------------------

typedef struct _ms{

  struct _ms *next;//next item
  struct _ms *last;//last item
  t_tlsmp u;//current displacement
  t_tlsmp alpha;//reflecting factor
  t_tlsmp beta;//bowing factor
  t_tlsmp dx;//spatial width of the oscillator

}t_ms;

t_ms *strng;
t_tlsmp freq;//
int L;//


//----------------------------------------------------

static FILE *fp1, *fp2;

static t_tlsmp phi(t_tlsmp alpha, t_tlsmp y){

  t_tlsmp out = sqrt(2*alpha) * y * exp(-2.0 * alpha * y * y + .5);

  //printf("%f %f %f %f\n", alpha, y, E_TO_HALF, out);
  return out;

}

static t_tlsmp x_dot(t_tlsmp *data, t_tlsmp omega, t_tlsmp alpha){

  
  t_tlsmp out = data[1] * omega;// - bforce*phi(alpha, data[1] - bvel);
  //printf("x_dot %f %f %f\n", out, omega, data[0]);
  return out;

}

static t_tlsmp y_dot(t_tlsmp *data, t_tlsmp omega, t_tlsmp alpha){

  t_tlsmp out; 
  
  out = (-1.0 * omega * data[0]);// - bforce*phi(alpha, data[1] - bvel);
  //printf("y_dot %f %f %f\n", out, omega, data[0]);
  return(out);

}

static void rk_osc_func(int samples, void *ptr, t_tlsmp *input){

  t_rk_mother *x = ptr;
  t_tlsmp data[2];
  t_tlsmp ret[2];
  int s = samples * x->h * x->up;
  int i, j;
  
  for(i=0; i<s; i++)
    {
    }
}

void setup_this(void){
  
  
}

void do_kill(void){
    

}

void dsp_chain(int samples,
	       t_tlsig **adc_sigs,
	       t_tlsig **dac_sigs){
  
  //printf("hello from the dsp chain\n");
  int s = samples;
  int j, i;


}

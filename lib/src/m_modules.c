#include "m_modules.h"
#include "stdlib.h"
#include "math.h"

    //*************//
    //     ADC     //
    //*************//
inline void tl_dsp_adc(int samples){

  int i,s, j, buff_pos=0;
  tl_adc *x = tl_get_adc();
  s=samples;
  for(i=0; i<x->out_cnt; i++)
    {
      for(j=0; j<s; j++)
	{
	  x->outlets[i]->smps[j] = 
	    x->ab->buff[buff_pos++]; 
	    

	  /* if(x->ab->buff_pos_w >= x->ab->buff_len) */
	  /*   x->ab->buff_pos_w -= x->ab->buff_len; */
	}

    }

}

void tl_init_adc(int out_cnt, int up){

  printf("creating adc...\n");

  tl_g_adc = (tl_adc *)malloc(sizeof(tl_adc));

  tl_g_adc->outlets = init_sigs(out_cnt, TL_OUTLET, 1);
  tl_g_adc->out_cnt = out_cnt;
  tl_g_adc->in_cnt = 0;

  tl_g_adc->sr = tl_get_samplerate();

  tl_g_adc->ab = get_g_audio_buff_in();

}

void tl_kill_adc(void){
  
  
  if(tl_g_adc!=NULL)
    {
      kill_outlets(tl_g_adc->outlets, tl_g_adc->out_cnt);

      free(tl_g_adc);
      tl_g_adc = NULL;

    }
  else printf("warning: tl_kill_adc: tl_g_adc does not (yet) exist\n");
}

tl_adc *tl_get_adc(void){

  if(tl_g_adc != NULL)
    return tl_g_adc;
  else
    {
      printf("error: tl_get_dac: tl_g_dac not initialized\n");
      return NULL;
    }
}

    //*************//
    //     DAC     //
    //*************//


inline void tl_dsp_dac(int samples){

  //  printf("in the dac... %d\n", samples);
  int i,s, j, buff_pos=0;
  tl_dac *x = tl_get_dac();
  s=samples;
  for(i=0; i<x->in_cnt; i++)
    {

      for(j=0; j<s; j++)
	{

	  x->ab->buff[buff_pos++] = 
	    x->inlets[i]->smps[j];

	  /* if(x->ab->buff_pos_w >= x->ab->buff_len) */
	  /*   x->ab->buff_pos_w -= x->ab->buff_len; */
	}

    }

}

void tl_init_dac(int in_cnt, int up){

  printf("creating dac...\n");

  // think more about this policy
  // later ...
  // one idea is to have a module called dac_bus
  // that simply looks like a [dac~] in Pd and directs
  // its output to 'the' tl_dac
  /* if(tl_dac_cnt > 0) */
  /*   { */
  /*     printf("error: tl_dac_init: only one dac allowed\n"); */
  /*     return(NULL); */
  /*   } */

  /* if(tl_arg_check((tl_arg_t *)args, dac_args)==0, 2) */
  /*   goto sunk; */

  tl_g_dac = (tl_dac *)malloc(sizeof(tl_dac));

  /* tl_g_dac->dsp_func = &tl_dsp_dac; */
  /* tl_g_dac->kill_func = &tl_kill_dac; */

  /* tl_g_dac->dac_args = args; */
  /* tl_g_dac->arg_cnt = 2; */

  tl_g_dac->inlets = init_sigs(in_cnt, TL_INLET, 1);
  tl_g_dac->in_cnt = in_cnt;

  tl_g_dac->sr = tl_get_samplerate();

  //tl_dac_cnt++;

  
  /* // again, we are presuming only one dac at a time ... */
  /* set_g_out_chann_cnt(in_cnt); */
  tl_g_dac->ab = get_g_audio_buff_out();
  /* tl_g_dac->ab = init_audio_buff(in_cnt); */
  /* set_g_audio_buff_out(tl_g_dac->ab); */

 /* sunk: */
 /*  printf("error: tl_dac_init: invalid arg list, could not create\n"); */

}

void tl_kill_dac(void){
  
  
  if(tl_g_dac!=NULL)
    {
      kill_inlets(tl_g_dac->inlets);

      /* if(tl_g_dac->ab!=NULL) */
      /* 	kill_audio_buff(tl_g_dac->ab); */
      free(tl_g_dac);
      tl_g_dac = NULL;
      //tl_dac_cnt--;
    }
  else printf("warning: tl_kill_dac: tl_g_dac does not (yet) exist\n");
}

void set_g_dac_in(int in, tl_sig *x){

  if(in< tl_get_dac()->in_cnt && in >=0)
    if(x!=NULL)
      tl_get_dac()->inlets[in]=x;
    else printf("error: set_g_dac_in: invalid sig ptr\n");
  else printf("error: set_g_dac_in: invlaid in ref\n");

}

tl_dac *tl_get_dac(void){

  if(tl_g_dac != NULL)
    return tl_g_dac;
  else
    {
      printf("error: tl_get_dac: tl_g_dac not initialized\n");
      return NULL;
    }
}

    //***************//
    //     table     //
    //***************//

inline void tl_dsp_table(int samples, void *mod){
  
  int s, i;
  tl_table *x = (tl_table *)mod;

  s = x->up * samples;
  for(i=0; i<s; i++)
    {
      x->outlets[0]->smps[i] = (tl_smp)
	x->table_array[(int)(x->inlets[0]->smps[i] * x->grain)];
    }

}

void *tl_init_table(int table_len, int up){


  tl_table *x = (tl_table *)malloc(sizeof(tl_table));


  x->in_cnt = 1;
  x->out_cnt = 1;

  x->inlets = init_sigs(x->in_cnt, TL_INLET, 1);
  x->outlets = init_sigs(x->out_cnt, TL_OUTLET, 1);

  x->up = up;
  x->grain = table_len;
  create_table_array(x);
  
  x->sr = tl_get_samplerate();

  //tl_table_cnt++;
  return x;
}


void create_table_array(tl_table *x){

  int i;
  tl_smp inc, val;
  inc = 1.0/x->grain;
  val = 0.0;

  x->table_array = (tl_smp *)malloc(sizeof(tl_smp)*x->grain);
  for(i=0; i<x->grain; i++)
    {
      x->table_array[i] = sin(2*PI * val);
      val +=inc;
      //printf("table array %d: %f\n", i, x->table_array[i]);
    }

}

void tl_kill_table(void *mod){

  tl_table *x = (tl_table *)mod;

  if(x!=NULL)
    {
      kill_outlets(x->outlets, 1);
      kill_inlets(x->inlets);

      if(x->table_array != NULL)
	{
	  free(x->table_array);
	  x->table_array = NULL;
	}
      free(x);
      x=NULL;
      //tl_lookup_cnt--;
    }
  else printf("error: tl_kill_table: invalid class ptr\n");
}


    //****************//
    //     lookup     //
    //****************//

inline void tl_dsp_lookup(int samples, void *mod){

  tl_lookup *x = (tl_lookup *)mod;
  int s = samples * x->up;
  int i;

  if(x->inlets[0]!=NULL)
    {
      for(i=0; i<s; i++)
	{
	  x->phase_inc = x->inlets[0]->smps[i]/(tl_smp)x->denom;
	  x->outlets[0]->smps[i] = x->phase;
	  x->phase += x->phase_inc;
	  if(x->phase >= 1.0)x->phase -= 1.0;
	}
    }
  else
    {
      x->phase_inc = x->freq/(tl_smp)x->denom;
      for(i=0; i<s; i++)
	{
	  x->outlets[0]->smps[i] = x->phase;
	  x->phase += x->phase_inc;
	  if(x->phase >= 1.0)x->phase -= 1.0;
	}
    }
}

void *tl_init_lookup(int up){

  tl_lookup *x = (tl_lookup *)malloc(sizeof(tl_lookup));

  /* x->dsp_func = &tl_dsp_lookup; */
  /* x->kill_func =  &tl_kill_lookup; */

  x->outlets = init_sigs(1, TL_OUTLET, up);
  x->out_cnt = 1;

  x->inlets = init_sigs(1, TL_INLET, up);
  x->in_cnt = 1;

  x->phase = 0.0;
  x->freq = 0.0;
  x->phase_inc = 0.0;

  x->up = up;
  x->sr = tl_get_samplerate();
  x->denom = x->up*x->sr;

  //tl_lookup_cnt++;

  return x;

}

void tl_kill_lookup(void *mod){

  tl_lookup *x = (tl_lookup *)mod;
  if(x!=NULL)
    {

      kill_inlets(x->inlets);
      kill_outlets(x->outlets, 1);

      free(x);
      x=NULL;
      //tl_lookup_cnt--;

    }
  else printf("error: tl_kill_lookup: invalid class ptr\n");

}


  //********************//
  //     UDS solver     //
  //********************//

// all of this is hardcoded for Runge-Kutta
// in future, add more solver options

tl_UDS_node *tl_init_UDS_node(tl_dyfunc func, int in_cnt, int ctl_cnt, int up){

  int i;

  tl_UDS_node *x = malloc(sizeof(tl_UDS_node));
  x->func = func;
  
  x->ks = malloc(5*sizeof(tl_smp));


  // like sigs, these special inlets point to other pointers that will
  // get allocated
  x->data_in = malloc(sizeof(tl_smp*) * in_cnt);
  for(i=0; i<in_cnt; i++)
      x->data_in[i] = NULL;


  x->in_cnt = in_cnt; 
  
  *x->data_out = 0.0;
  /* x->ins = init_sigs(in_cnt, TL_INLET, 4*up); */
  /* x->in_cnt = in_cnt; */
  /* x->outs = init_sigs(1, TL_OUTLET, 4*up);  */
  /* x->out_cnt = 1; */
  
  x->ctls = malloc(sizeof(tl_ctl *) * ctl_cnt);
  x->ctl_cnt = ctl_cnt;
  
  x->state = 0;
  x->next = NULL;
  return x;

}

void tl_reset_UDS_node(tl_UDS_node *x, tl_smp state){

  int i;
  x->state = state;
  for(i=0; i<5; i++)
    x->ks[i] = 0;
  /* for(i=0;i<x->in_cnt;i++) */
  /*   x->data_in[i] = NULL; */
  *x->data_out = 0.0;
}

// theoretically, the order doesn't matter 
// so we can just push and pop at will
void tl_push_UDS_node(tl_UDS_node *x, tl_UDS_node *y){


  while(x->next!=NULL)x=x->next;
  x->next = y;
    
}

void tl_kill_UDS_node(tl_UDS_node *x){
  
  int i;

  if(x!=NULL)
    {

      if(x->ks != NULL)
	{
	  free(x->ks);
	  x->ks = NULL;
	}
      if(x->data_in != NULL)
	{
	  free(x->data_in);
	  x->data_in = NULL;
	}


      for(i=0;i<x->ctl_cnt;i++)
	kill_ctl(x->ctls[i]);      

      free(x);
      x = NULL;

    }

}

void tl_kill_UDS_net(tl_UDS_node *x){

  tl_UDS_node *y = x->next;
  if(x!=NULL)tl_kill_UDS_node(x);
  while(y!=NULL)
    {
      x=y;
      y=x->next;
      tl_kill_UDS_node(x);
    }
}


// for now, hardcode 4 stage Runge-Kutta
// in future add more solver options


// again, hardcoded for Runge-Kutta
inline void tl_dsp_UDS_solver(int samples, void *mod){

  int i, j, k;
  tl_UDS_solver *x = (tl_UDS_solver *)mod;
  printf("%p\n", x);
  tl_UDS_node *y;
  for(i=0; i<samples; i++)
    {
      for(j=0;j<4;j++)
  	{
  	  // go through the whole network
  	  // each time
  	  y = x->UDS_net->next;
  	  k=0; // tracks which node we are solving for

  	  // think about how to do this in one loop!
  	  // first gather the current partial states
  	  while(y!=NULL)
  	    {
  	    // push the current RK stage datum
  	      *y->data_out = y->state + (x->mult[j] * y->ks[j]);
  	      // printf("y %p data_out %f  state %f :: ", y, *y->data_out, y->state);
  	      y=y->next;
  	    }

  	  // now call the differential functions at each node
  	  y = x->UDS_net->next;
  	  while(y!=NULL)
  	  	{
  	      // evaluate the ODE at this stage
  	      // use i for ctl referencing
  	      y->ks[j+1] = y->func(y,i);

  	      // if stage 4, solve it ...
  	      if(j==3) // calculate the dx
  	      	{
  	      	  y->dx =  x->one_sixth * x->h_time *
  	      	    (y->ks[1] +
  	      	     (2*y->ks[2]) +
  	      	     (2*y->ks[3]) +
  	      	     y->ks[4]);
		  
  	      	  // output new state and update
  	      	  y->state = y->state + y->dx;
  	      	  x->outlets[k++]->smps[i] = y->state;
  	      	}
  	      y=y->next;
  	    }

	  	  
  	}

    }

}


void *tl_init_UDS_solver(int ins, int outs, int up){

  tl_UDS_solver *x = malloc(sizeof(tl_UDS_solver));
  x->UDS_net = tl_init_UDS_node(NULL, 0, 0, 1);
  
  x->in_cnt = ins;
  x->out_cnt = outs;

  x->inlets = init_sigs(ins, TL_INLET, up);
  x->outlets = init_sigs(outs, TL_OUTLET, up);

  x->up = up;
  x->h = 1;
  x->h_time = (1.0/(tl_smp)x->h) * (1.0/(tl_smp)tl_get_samplerate());
  x->half_h_time = .5 *x->h_time;

  x->mult[0] = 0.0;
  x->mult[1] = x->mult[2] = x->half_h_time;
  x->mult[3] = x->h_time;

  x->one_sixth = 1.0/6.0;
  return x;
}

void tl_kill_UDS_solver(void *mod){

  tl_UDS_solver *x = (tl_UDS_solver *)mod;
  tl_kill_UDS_net(x->UDS_net);
  kill_outlets(x->outlets, x->out_cnt);
  kill_inlets(x->inlets);


}

    //**********************//
    //     housekeeping     //
    //**********************//

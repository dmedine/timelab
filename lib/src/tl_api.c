/* this file belongs to */
/* timelab-0.10 by David Medine */

// this source code defines the 
// timelab C api

#include "tl_api.h"
#include "stdlib.h"

//***********************************
//***********************************

   //*******************//
   //       tl_sig      // 
   //*******************//

tl_sig *init_one_sig(int block_len, int up){

  int i;
  tl_sig *x;
 
  x = (tl_sig *) malloc(sizeof(tl_sig));

  x->smps = (tl_smp *)malloc(sizeof(tl_smp) * block_len* up);
  for(i=0; i<block_len*up; i++)
    x->smps[i] = 0.0;

  x->smp_cnt = block_len * up;
  x->type = TL_OUTLET; // we never need to initialize a single inlet
  x->up = up;

  return x;

}

tl_sig **init_sigs(int sig_cnt, tl_sig_t sig_type, int block_len, int up){

  int i;
  tl_sig **x;

  if(up<1)
    {
      printf("ERROR: in init_sigs : argument up < 1\n");
      return;
    }

  x = (tl_sig **) malloc(sizeof(tl_sig *) * sig_cnt);

  if(sig_type == TL_OUTLET)
    for(i=0; i<sig_cnt; i++)
      x[i] = init_one_sig(block_len, up);
  else
    for(i=0; i<sig_cnt; i++)
      x[i]=NULL;
      
  return x;

}

void kill_one_sig(tl_sig *x){

  if(x->smps)
    {
      free(x->smps);
      x->smps = NULL;
    }

  if(x)
    {
      free(x);
      x = NULL;
    }
}

void kill_sigs(tl_sig **x, int sig_cnt){

  int i;


  if(x)
    {
      for(i=0; i<sig_cnt; i++)
	if(x[i]->type == TL_OUTLET)
	  kill_one_sig(x[i]);
      
      free(x);
      x=NULL;
    }
}

inline void set_sig_vals(tl_sig *x, tl_smp val){

  int i;
  for(i=0; i<x->smp_cnt; i++)
    x->smps[i] = val;

}

inline void scale_sig_vals(tl_sig *x, tl_smp scalar){

  int i;
  for(i=0; i<x->smp_cnt; i++)
    x->smps[i] *= scalar;

}

inline void multiply_sigs(tl_sig *x, tl_sig *y){

  int i;
  if(x->smp_cnt != y->smp_cnt)
    {
      printf("ERROR: in multiply_sigs : count mismatch\n");
 
      return;
    }
  for(i=0; i<x->smp_cnt; i++)
    x->smps[i]*=y->smps[i];
}

inline void divide_sigs(tl_sig *x, tl_sig *y){

  int i;
  if(x->smp_cnt != y->smp_cnt)
    {
      printf("ERROR: in divide_sigs : count mismatch\n");
 
      return;
    }
  for(i=0; i<x->smp_cnt; i++)
    x->smps[i]/=y->smps[i];
}

inline void add_sigs(tl_sig *x, tl_sig *y){

  int i;
  if(x->smp_cnt != y->smp_cnt)
    {
      printf("ERROR: in add_sigs : count mismatch\n");
 
      return;
    }
  for(i=0; i<x->smp_cnt; i++)
    x->smps[i]+=y->smps[i];
}

inline void subtract_sigs(tl_sig *x, tl_sig *y){

  int i;
  if(x->smp_cnt != y->smp_cnt)
    {
      printf("ERROR: in subtract_sigs : count mismatch\n");
 
      return;
    }
  for(i=0; i<x->smp_cnt; i++)
    x->smps[i]-=y->smps[i];
}

inline void zero_out_sig(tl_sig *x){

  int i;
  for(i=0; i<x->smp_cnt; i++)
    x->smps[i] = 0.0;

}

//***********************************
//***********************************

   //***********************//
   //      gloabl i/o       // 
   //***********************//

void init_gloabal_io(int out_channs, int in_channs){

  int i;

  if(tl_g_audio_out || tl_g_audio_in)
    close_global_io();

  tl_g_audio_out = (tl_audio_io *)malloc(sizeof(tl_audio_io));
  tl_g_audio_in = (tl_audio_io *)malloc(sizeof(tl_audio_io));

  tl_g_audio_out->block_len=tl_get_block_len();
  tl_g_audio_out->channs = out_channs;
  tl_g_audio_out->buff_len = tl_g_audio_out->block_len*tl_g_audio_out->channs;
  tl_g_audio_out->buff_pos_w = 0;
  tl_g_audio_out->buff_pos_r = 0;
  tl_g_audio_out->buff = (tl_smp *)malloc(sizeof(tl_smp) *tl_g_audio_out->buff_len);

  for(i=0; i<tl_g_audio_out->buff_len; i++)
    tl_g_audio_out->buff[i] = 0.0;

  tl_g_audio_in->block_len=tl_get_block_len();
  tl_g_audio_in->channs = in_channs;
  tl_g_audio_in->buff_len = tl_g_audio_in->block_len*tl_g_audio_in->channs;
  tl_g_audio_in->buff_pos_w = 0;
  tl_g_audio_in->buff_pos_r = 0;
  tl_g_audio_in->buff = (tl_smp *)malloc(sizeof(tl_smp) *tl_g_audio_in->buff_len);

  for(i=0; i<tl_g_audio_in->buff_len; i++)
    tl_g_audio_in->buff[i] = 0.0;


}

void close_global_io(void){

  if(tl_g_audio_out)
    {
      if(tl_g_audio_out->buff)
	{
	  free(tl_g_audio_out->buff);
	  tl_g_audio_out->buff = NULL;
	}
      free(tl_g_audio_out);
      tl_g_audio_out = NULL;
    }

  if(tl_g_audio_in)
    {
      if(tl_g_audio_in->buff)
	{
	  free(tl_g_audio_in->buff);
	  tl_g_audio_in->buff = NULL;
	}
      free(tl_g_audio_in);
      tl_g_audio_in = NULL;
    }

}

//***********************************
//***********************************

   //*******************//
   //      tl_ctl       // 
   //*******************//

tl_ctl *init_ctl(tl_ctl_t type){

  tl_ctl *x;
  x = (tl_ctl *) malloc(sizeof(tl_ctl));

  x->type = type;
  // all other fields must be handled by the 
  // client -- see tl_api.h for more details

  x->name = NULL;
  x->bang_func = NULL;
  x->was = 0.0;
  x->will_be = 0.0;
  x->outlet = NULL;

  x->next = NULL;
  x->top = 0;


}

void kill_ctl(tl_ctl *x){
  
  if(x)
    {
      free(x);
      x = NULL;
    }

}

void set_ctl_val(tl_ctl *x, tl_smp val){

  int i;
  if(x->type!=TL_LIN_CTL)
    {
      printf("ERROR: in set_ctl_val : type is not TL_LIN_CTL\n");
      return;
    }

  if(!x->outlet)
    {
      printf("ERROR: in set_ctl_val : no outlet available\n");
      return;
    }

  for(i=0; i<x->outlet->smp_cnt; i++)
    x->outlet->smps[i] = val;
  x->was = val;

}

void interpolate_ctl_val(tl_ctl *lvl_stck, tl_ctl *x, tl_smp new_val){

  tl_smp diff;
  tl_smp inc;
  tl_smp val;
  int i;

  if(!x->outlet)
    {
      printf("ERROR: in interpolate_ctl_val : no outlet available\n");
      return;
    }

  diff = new_val - x->was;
  val = x->was;
  inc = diff/x->outlet->smp_cnt;

  for(i=0; i<x->outlet->smp_cnt; i++)
    {
      x->outlet->smps[i] = val;
      val += inc;
    }

  x->will_be = new_val; // we must set all the values
  // in the outlet buffer to this value on the next tick

  // so, we push it onto this stack
  // and make sure to deal with it in the dsp loop
  // the client must provide this stack for us
  push_ctl_stck(lvl_stck, x);

}

// TODO:
void process_ctl_stck(tl_ctl *stck){

}

void push_ctl_stck(tl_ctl *stck, tl_ctl *y){

  tl_ctl *x = stck;
  
  while(x->next!=NULL) x=x->next;
  x->next = y;
  stck->top++;
    
}

tl_ctl *pop_ctl_stck(tl_ctl *stck){

  tl_ctl *x = stck;
  tl_ctl *y;
  if(stck->top<=0)
    {
      printf("ERROR: pop_ctl_stck : stack underflow\n");
      return NULL;
    }

  while(x->next->next!=NULL) x=x->next;
  y=x->next;
  
  x->next = NULL;
  stck->top--;
  
  return y;  

}

void flush_ctl_stck(tl_ctl *stck){

  stck->top = 0;

} 

void kill_ctl_stck(tl_ctl *stck){

  tl_ctl *x = stck;
  while(x->next!=NULL)
    kill_ctl(pop_ctl_stck(x));
  if(stck)
    {
      free(stck);
      stck=NULL;
    }

}


//***********************************
//***********************************

   //*******************//
   //     tl_pclass     // 
   //*******************//

tl_pclass *init_pclass(void){

  tl_pclass *x;
  x = (tl_pclass *)malloc(sizeof(tl_pclass));

  x->dsp_func = NULL;
  x->inlets = NULL;
  x->inlet_cnt = 0;
  x->outlets = NULL;
  x->outlet_cnt = 0;
  x->cclass_ptr = NULL;
  x->name = NULL;

}

void kill_pclass(tl_pclass *x){
  
  if(x->outlets)
    kill_sigs(x->outlets, x->outlet_cnt);

  // inlets don't need to be freed

 if(x->kill_func)   
   x->kill_func();
 if(x->cclass_ptr)
    {
     
      free(x->cclass_ptr);
      x->cclass_ptr = NULL;
    }

  if(x->name)
    {
      free(x->name);
      x->name = NULL;
    }

}

//*****************************************************
//*****************************************************

     //*****************************************//
     //  scheduling and housekeeping functions  //
     //*****************************************//

inline void dsp_tick(int samples, tl_pclass *head){

  tl_pclass *y = head;
  if(y->next==NULL)
    y->dsp_func(samples, y);
  else
    while(y->next!=NULL)
      {
	y->dsp_func(samples, y);
	y=y->next;
      }
}

int tl_get_samplerate(void){

  return tl_g_samplerate;

}

int tl_get_block_len(void){

  return tl_g_block_len;

}

void tl_set_samplerate(int samplerate){

  tl_g_samplerate = samplerate;

}

void tl_set_block_len(int block_len){

  tl_g_block_len = block_len;

}

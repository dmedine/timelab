#include "m_modules.h"
#include "stdio.h"
#include "stdlib.h"
#include "math.h"

 
FILE *fp;

//these first two modules are special
//they belong to the tl engine, not to the user
//we need methods for dealing with number of channels, etc.

//////////////////////////
/*******dac*************/
g_dac_cnt = 0;
void dac_dsp_loop(int samples, t_dac *x){

  int i, s, j;

  for(i=0; i<x->i_sig_cnt; i++)
    {
      s = samples;

      for(j=0; j<s; j++)
      	{
  	  g_soundout->buff[g_soundout->buff_pos_w++] =
  	    x->i_sigs[i]->s_block[j];
	  
	  /* printf("%d:%d %f\t%d %f\n", i, j, x->i_sigs[i]->s_block[j], */
	  /* 	 g_soundout_buff_pos_w-1, g_soundout_buff[g_soundout_buff_pos_w]); */

  	  if(g_soundout->buff_pos_w >= g_soundout->buff_len)
  	      g_soundout->buff_pos_w -= g_soundout->buff_len;
	  
	}
    }
}

void *dac_init(int channels){

  int i;
  t_dac *x = (t_dac *)malloc(sizeof(t_dac));
  x->dsp_func = (t_dsp_func)&dac_dsp_loop;

  /* fp = fopen("./output.txt", "w"); */
  
  x->up = 1;
  x->i_sig_cnt = channels;

   x->i_sigs = (t_tlsig **)malloc(x->i_sig_cnt * sizeof(t_tlsig *));
  for(i = 0; i<x->i_sig_cnt; i++)
       x->i_sigs[i] = NULL;


  //fp = fopen("dac_out", "w");
  x->od.type_str = name_new("dac");
  x->od.name_str = NULL;

  x->od.obj_cnt = g_dac_cnt;
  x->od.reg_place = -1;
  g_dac_cnt++;

  x->od.type = 2;
  x->od.sigs = x->i_sigs;
  x->od.sig_cnt = x->i_sig_cnt;
  x->od.this_type_cnt = g_dac_cnt;

  return(x);
}

void kill_dac(t_dac *x){

  int i;

  if(x->i_sigs)
    {
      free(x->i_sigs);
      x->i_sigs = NULL;
    }


  free(x->od.type_str);
  x->od.type_str = NULL;

  if(x->od.name_str)
    {
      free(x->od.name_str);
      x->od.name_str = NULL;
    }

  g_dac_cnt--;

  if(x)
    {
      free(x); 
      x = NULL ;
    }
}

//////////////////////////
/*******adc*************/
g_adc_cnt = 0;
void adc_dsp_loop(int samples, t_adc *x){

}

void *adc_init(int channels){

  int i;
  t_adc *x = (t_adc *)malloc(sizeof(t_adc));
  x->dsp_func = (t_dsp_func)&adc_dsp_loop;

  /* fp = fopen("./output.txt", "w"); */
  
  x->up = 1;
  x->o_sig_cnt = channels;
  x->o_sigs = init_tlsigs(x->o_sig_cnt, 0, x->up);


  //fp = fopen("dac_out", "w");
  x->od.type_str = name_new("adc");
  x->od.name_str = NULL;
  
  x->od.obj_cnt = g_dac_cnt;
  x->od.reg_place = -1;
  g_adc_cnt++;

  x->od.type = 2;
  x->od.sigs = x->o_sigs;
  x->od.sig_cnt = x->o_sig_cnt;
  x->od.this_type_cnt = g_adc_cnt;

  return(x);
}

void kill_adc(t_adc *x){

  int i;

  if(x->o_sigs)
    kill_tlsigs(x->o_sigs, x->o_sig_cnt);

  free(x->od.type_str);
  x->od.type_str = NULL;

  if(x->od.name_str)
    {
      free(x->od.name_str);
      x->od.name_str = NULL;
    }

  g_adc_cnt--;

  if(x)
    {
      free(x); 
      x = NULL ;
    }
}


////////////////////////
/*******table*********/
g_table_cnt = 0;

void table_dsp_loop(int samples, t_table *x){

  int s = samples * x->up;
  int i;
  for(i=0; i<s; i++)
    {
      x->o_sigs[0]->s_block[i] = 
	x->vals[(int)(x->i_sigs[0]->s_block[i] * x->grain)];
      //printf("%f %f\n", x->i_sigs[0]->s_block[i], x->o_sigs[0]->s_block[i]);
      //fprintf(fp, "%f %f\n", x->i_sigs[0]->s_block[i], x->o_sigs[0]->s_block[i]);
    }


}


void *table_init(int grain, int up){

  int i;
  t_table *x = (t_table *)malloc(sizeof(t_table));
  x->dsp_func = (t_dsp_func)&table_dsp_loop;

  x->o_sig_cnt = 1;
  x->o_sigs = init_tlsigs(x->o_sig_cnt, 0, up);

  x->i_sig_cnt = 1;
  x->i_sigs = init_tlsigs(x->i_sig_cnt, 1, up);

  x->up = up;

  x->grain = grain;
  
  create_table(x);

  x->od.type_str = name_new("table");
  x->od.name_str = NULL;

  x->od.obj_cnt = g_table_cnt;
  x->od.reg_place = -1;
  
  x->od.type = 2;
  x->od.sigs = x->o_sigs;
  x->od.sig_cnt = x->o_sig_cnt;
  x->od.this_type_cnt = ++g_table_cnt;

  return (x);
}

/* //internal function */
void create_table(t_table *x){

  int i;
  t_tlsmp inc, val;

  inc = 1.0/x->grain;
  val = 0.0;

  x->vals = malloc(sizeof(t_tlsmp) * x->grain);

  for(i=0; i<x->grain; i++)
    {
      x->vals[i] = sin( g_two_pi*val );
      //printf("vals[%d]: %f\n", i, x->vals[i]);
      val+=inc;
    }


}

void kill_table(t_table *x){

  int i;


  if(x->o_sigs)
    kill_tlsigs(x->o_sigs, x->o_sig_cnt);

  if(x->i_sigs)
    {
      free(x->i_sigs);  
      x->i_sigs=NULL;
    }

  if(x->vals)
    {
      free(x->vals);
      x->vals = NULL;
    }

  free(x->od.type_str);
  x->od.type_str = NULL;
  
  if(x->od.name_str)
    {
      free(x->od.name_str);
      x->od.name_str = NULL;
    }

  g_table_cnt--;

  if(x)
    {
      free(x);
      x = NULL;
    }
}

/////////////////////////////////
/***********lookup**************/

g_lookup_cnt = 0;
void lookup_dsp_loop(int samples, t_lookup *x){

  int s = samples * x->up;
  int i;

  
  if(x->i_sigs[0]!=NULL)
    x->phase_inc = (x->i_sigs[0]->s_block[0]/(x->sr*x->up));
  else x->phase_inc = x->freq/(x->sr*x->up);
  
  for(i=0; i<s; i++)
    {
      // printf("s %d, phase %f, phase_inc %f\n", i, x->phase, x->phase_inc);
     x->o_sigs[0]->s_block[i] =
	     x->phase;

      x->phase += x->phase_inc;
      if(x->phase >= 1.0) x->phase-=1.0;

    }

  x->freq_was = x->freq;

}


void *lookup_init(int up){

  int i;

  t_lookup *x = (t_lookup *)malloc(sizeof(t_lookup));
  x->dsp_func = (t_dsp_func)&lookup_dsp_loop;

  x->o_sig_cnt = 1;
  x->o_sigs = init_tlsigs(x->o_sig_cnt, 0, up);

  x->i_sig_cnt = 1;
  x->i_sigs = init_tlsigs(x->i_sig_cnt, 1, up);

  x->up = up;

  x->phase = 0.0;
  x->sr = (t_tlsmp)g_samplerate;
  
  x->freq = x->freq_was = 440.0;
  x->phase_inc = (x->freq / x->sr);

  x->od.type_str = name_new("lookup");
  x->od.name_str = NULL;

  x->od.obj_cnt = g_lookup_cnt;
  x->od.reg_place = -1;
  g_lookup_cnt++;

  x->od.type = 2;
  x->od.sigs = x->o_sigs;
  x->od.sig_cnt = x->o_sig_cnt;
  x->od.this_type_cnt = g_lookup_cnt;

  return(x);

}



void kill_lookup(t_lookup *x){
  
  int i;

  if(x->o_sigs)
    kill_tlsigs(x->o_sigs, x->o_sig_cnt);

  if(x->i_sigs)
    {
      free(x->i_sigs);
      x->i_sigs = NULL;
    }

  free(x->od.type_str);
  x->od.type_str = NULL;

  if(x->od.name_str)
    {
      free(x->od.name_str);
      x->od.name_str = NULL;
    }

  g_lookup_cnt--;
  if(x)
    {
      free(x);
      x = NULL;
    }
}

//////////////////////////
/********noise***********/

void noise_dsp_loop(int samples, t_noise *x){

  int i, r;
  int s = samples * x->up;
  for(i=0; i<s; i++)
    {
      r = rand();
      x->o_sigs[0]->s_block[i] = ((t_tlsmp)r/RAND_MAX) * 2 - 1;
    }

}

void *noise_init(int up){

  t_noise *x = (t_noise *)malloc(sizeof(t_noise));
  x->dsp_func = (t_dsp_func)&noise_dsp_loop;
 
  x->o_sigs = init_tlsigs(1, 0, up);
  x->o_sig_cnt = 1;
  x->up = up;

  x->od.type_str = name_new("noise");
  x->od.name_str = NULL;

  x->od.obj_cnt = g_noise_cnt;
  x->od.reg_place = -1;
  g_noise_cnt++;

  x->od.type = 2;
  x->od.sigs = x->o_sigs;
  x->od.sig_cnt = x->o_sig_cnt;
  x->od.this_type_cnt = g_noise_cnt;

  return(x);

}


void kill_noise(t_noise *x){
  if(x->o_sigs)
    kill_tlsigs(x->o_sigs, x->o_sig_cnt);
 
  free(x->od.type_str);
  x->od.type_str = NULL;

  if(x->od.name_str)
    {
      free(x->od.name_str);
      x->od.name_str = NULL;
    }

  g_noise_cnt--;

  free(x);
  x = NULL;
}


/////////////////////////////
/********dwnsample************/

g_dwnsample_cnt = 0;
void dwnsample_dsp_loop(int samples, t_dwnsample *x){

  int s, i, j, k;

  for(k=0; k<x->i_sig_cnt; k++)
    {
      s = samples * x->up;
      
      for(i=0; i<s; i++)
	{
	  x->dummy_sigs[k]->s_block[i] =
	    x->i_sigs[k]->s_block[i];
	  // printf("%d %f %f\n", i, x->i_sigs[k]->s_block[i],x->dummy_sigs[k]->s_block[i]);
	}
      //printf("***********\n");
      s = samples;
      for(i=0, j=0; i<s; i++, j+=x->up)
	{
	  x->o_sigs[k]->s_block[i] = x->dummy_sigs[k]->s_block[j];
	  //printf("%d %f\n", i, x->o_sigs[k]->s_block[i]);
	}
    }
}

/* void dwnsample_dsp_loop(int samples, t_dwnsample *x){ */

/*   int s, i, j, k; */

/*   for(k=0; k<x->i_sig_cnt; k++) */
/*     { */
/*       s = samples * x->h; */
      
/*       for(i=0; i<s; i++) */
/* 	x->dummy_sigs[k]->s_block[i] = */
/* 	  x->i_sigs[k]->s_block[i]; */
      
/*       s = samples; */
/*       for(i=0, j=0; i<s; i++, j+=x->h) */
/* 	x->o_sigs[k]->s_block[i] = x->dummy_sigs[k]->s_block[j]; */
/*     } */
/* } */

void *dwnsample_init(int sig_cnt, int up){

  int i;
  t_dwnsample *x = (t_dwnsample*)malloc(sizeof(t_dwnsample));

  x->dsp_func = (t_dsp_func)&dwnsample_dsp_loop;

  x->i_sig_cnt = sig_cnt;
  x->o_sig_cnt = sig_cnt;

  x->i_sigs = (t_tlsig **)malloc(x->i_sig_cnt * sizeof(t_tlsig *));
  for(i=0; i<x->i_sig_cnt; i++)
    x->i_sigs[i] = NULL;

  x->o_sigs = init_tlsigs(x->o_sig_cnt, 0, 1);

  x->dummy_sigs = init_tlsigs(sig_cnt, 0, up);

  x->up = up;
  x->av_buff = (t_tlsmp *)malloc(sizeof(t_tlsmp) * x->up);
  for(i=0; i<x->up; i++)x->av_buff[i] = 0.0;
  x->avg_fact = up;
  x->inv_avg_fact = 1.0/(t_tlsmp)x->avg_fact;

  x->od.type_str = name_new("dwnsample");
  x->od.name_str = NULL;

  x->od.obj_cnt = g_dwnsample_cnt;
  x->od.reg_place = -1;
  g_dwnsample_cnt++;
 
  x->od.type = 2;
  x->od.sigs = x->o_sigs;
  x->od.sig_cnt = x->o_sig_cnt;
  x->od.this_type_cnt = g_dwnsample_cnt;

  return(x);

}

void kill_dwnsample(t_dwnsample *x){

  if(x->o_sigs)
    kill_tlsigs(x->o_sigs, x->o_sig_cnt);

  if(x->i_sigs)
    {
      free(x->i_sigs);
      x->i_sigs = NULL;
    }

  if(x->dummy_sigs)
    kill_tlsigs(x->dummy_sigs, x->o_sig_cnt);

  if(x->av_buff)
    {
      free(x->av_buff);
      x->av_buff = NULL;
    }

  free(x->od.type_str);
  x->od.type_str = NULL;

  if(x->od.name_str)
    {
      free(x->od.name_str);
      x->od.name_str = NULL;
    }

  g_dwnsample_cnt--;

  if(x)
    {
      free(x);
      x = NULL;
    }
}

//////////////////////////////
/********upsample************/

g_upsample_cnt = 0;
void upsample_dsp_loop(int samples, t_upsample *x){

  int s, i, j, k, idx;
  
  s = samples;

  for(k=0;k<x->i_sig_cnt; k++)
    {
      idx = 0;
      for(i=0; i<s; i++)
	{
	  for(j=0; j<x->up; j++)
	    {
		  x->diff = x->i_sigs[k]->s_block[i] - 
		    x->last_tlsmp[k];
		  x->step = x->diff/x->up;
		  x->o_sigs[k]->s_block[idx+j] = 
		    x->last_tlsmp[k] + x->step*(j+1);
	    }
	  
	  idx+=x->up;
	  x->last_tlsmp[k] = x->i_sigs[k]->s_block[i];
    	}
    }
}

void *upsample_init(int sig_cnt, int up){


  int i;
  t_upsample *x = (t_upsample*)malloc(sizeof(t_upsample));
  x->dsp_func = (t_dsp_func)&upsample_dsp_loop;

  x->o_sig_cnt = sig_cnt;
  x->o_sigs = init_tlsigs(x->o_sig_cnt, 0, up);

  x->i_sig_cnt = sig_cnt;
  x->i_sigs = (t_tlsig **)malloc(sizeof(t_tlsig *) * x->i_sig_cnt);
  for(i = 0; i<x->i_sig_cnt; i++)
    x->i_sigs[i] = NULL;

  x->last_tlsmp = (t_tlsmp *)malloc(sizeof(t_tlsmp) * sig_cnt);
  for(i=0; i<sig_cnt; i++)
    x->last_tlsmp[i] = 0.0;

  x->diff = 0.0;
  x->step = 0.0;
  x->up = up;

  x->od.type_str = name_new("upsample");
  x->od.name_str = NULL;

  x->od.obj_cnt = g_upsample_cnt;
  x->od.reg_place = -1;
  g_upsample_cnt++;

  x->od.type = 2;
  x->od.sigs = x->o_sigs;
  x->od.sig_cnt = x->o_sig_cnt;
  x->od.this_type_cnt = g_upsample_cnt;

  return (x);

}

void kill_upsample(t_upsample *x){

  if(x->o_sigs)
      kill_tlsigs(x->o_sigs, x->o_sig_cnt);

  if(x->i_sigs)
    {
      free(x->i_sigs);
      x->i_sigs = NULL;
    }  

  if(x->last_tlsmp)
    {
      free(x->last_tlsmp);
      x->last_tlsmp = NULL;
    }

  free(x->od.type_str);
  x->od.type_str = NULL;

  if(x->od.name_str)
    {
      free(x->od.name_str);
      x->od.name_str = NULL;
    }

  g_upsample_cnt--;

  if(x)
    {  
      free(x);
      x=NULL;
    }
}

///////////////////////////////////////
/**********lti_filt*******************/
g_lti_filt_cnt = 0;

void lti_filt_dsp_loop(int samples, t_lti_filt *x){

  int i, j, k;
  int s = samples;

  t_tlsmp yn;//output sample
  t_tlsmp wn;//middle of Direct Form II

  for(i=0; i<s; i++)
    {
      
      //feedback component
      //start with the current input
      wn = x->i_sigs[0]->s_block[i];
      //then subtract the a coefficients
      //times the stored history of w
      for(j=1; j<x->len; j++)
	{

	  wn-=x->a[j] * x->w[j-1];
	  /* printf("\twn : %f, x->a[%d] : %f, x->w[%d] : %f\n", */
	  /* 	 wn, j, x->a[j], j-1, x->w[j-1]); */

	}

      //feed forward
      //right side of Direct Form II
      yn=x->b[0]*wn;
      //      printf("yn : %f\n", yn);
      for(j=1; j<x->len; j++)
	{
	  yn+=x->b[j] *x->w[j-1];
	  /* printf("\tyn : %f, x->b[%d] : %f, x->w[%d] : %f\n", */
	  /* 	 yn, j, x->b[j], j-1, x->w[j-1]); */
	}

      yn *= x->a[0];
      //      printf("yn : %f, x->a[0] : %f\n",yn, x->a[0]);
     
      //shift w down and store wn
      for(j=1;j<x->len;j++)
	x->w[j] = x->w[j-1];
      x->w[0] = wn;
      x->o_sigs[0]->s_block[i] = yn;
    }

}

inline t_tlsmp lti_filt(t_lti_filt *x, t_tlsmp input){

  int i, j, k;

  t_tlsmp ma = 0.0;//FIR component
  t_tlsmp ar = 0.0;//IIR component
  t_tlsmp yn = 0.0;
  //sum them and multiply by a0 at the end
  
  return yn;
}

void *lti_filt_init(int len, t_tlsmp *a, t_tlsmp *b){

  int i;

  t_lti_filt *x = (t_lti_filt *)malloc(sizeof(t_lti_filt));
  x->dsp_func = (t_dsp_func)&lti_filt_dsp_loop;
  x->up = 1;

  x->o_sig_cnt = 1;
  x->o_sigs = init_tlsigs(x->o_sig_cnt, 0, x->up);

  x->i_sig_cnt = 1;
  x->i_sigs = init_tlsigs(x->i_sig_cnt, 1, x->up);

  x->len = len;
  x->b = (t_tlsmp *)malloc(sizeof(t_tlsmp)*x->len);
  x->a = (t_tlsmp *)malloc(sizeof(t_tlsmp)*x->len);
  x->w = (t_tlsmp *)malloc(sizeof(t_tlsmp)*x->len-1);

  for(i=0;i<x->len-1; i++)
    x->w[i] = 0.0;

  for(i=0;i<x->len; i++)
    {
      x->a[i] = a[i];
      x->b[i] = b[i];
    }

  x->od.type_str = name_new("lti_filt");
  x->od.name_str = NULL;

  x->od.obj_cnt = g_lti_filt_cnt;
  x->od.reg_place = -1;
  g_lti_filt_cnt++;

  x->od.type = 2;
  x->od.sigs = x->o_sigs;
  x->od.sig_cnt = x->o_sig_cnt;
  x->od.this_type_cnt = g_lti_filt_cnt;

  return x;

}

void kill_lti_filt(t_lti_filt *x){

  if(x->a)
    {
      free(x->a);
      x->a = NULL;
    }

  if(x->b)
    {
      free(x->b);
      x->b = NULL;
    }

  if(x->b)
    {
      free(x->b);
      x->b = NULL;
    }


  if(x->w)
    {
      free(x->w);
      x->w = NULL;
    }

  if(x->o_sigs)
    kill_tlsigs(x->o_sigs, x->o_sig_cnt);

  if(x->i_sigs)
    {
      free(x->i_sigs);  
      x->i_sigs=NULL;
    }


 free(x->od.type_str);
  x->od.type_str = NULL;

  if(x->od.name_str)
    {
      free(x->od.name_str);
      x->od.name_str = NULL;
    }

  g_lti_filt_cnt--;

  if(x)
    {  
      free(x);
      x=NULL;
    }
}



/////////////////////////////////////////////////////
/***********digital wiage guide module*************/
g_dwg_cnt = 0;
void dwg_dsp_loop(int samples, t_dwg *x){
 
  int i;
  int s = samples;
  
  for(i=0; i<s; i++)
    {

    }


}

void *dwg_init(void){

  t_dwg *x = (t_dwg *)malloc(sizeof(t_dwg));
  x->dsp_func = (t_dsp_func)&dwg_dsp_loop;

  x->up =1;
  
  x->o_sig_cnt = 1;//??meh??
  x->o_sigs = init_tlsigs(x->o_sig_cnt, 0, x->up);

  x->i_sigs = NULL;

  //fp = fopen("dac_out", "w");
  x->od.type_str = name_new("dwg");
  x->od.name_str = NULL;
  
  x->od.obj_cnt = g_dwg_cnt;
  x->od.reg_place = -1;
  g_dwg_cnt++;

  x->od.type = 2;
  x->od.sigs = x->o_sigs;
  x->od.sig_cnt = x->o_sig_cnt;
  x->od.this_type_cnt = g_dwg_cnt;

  return(x);

}

void kill_dwg(t_dwg *x){
  
  if(x->o_sigs)
      kill_tlsigs(x->o_sigs, x->o_sig_cnt);

  if(x->i_sigs)
    {
      free(x->i_sigs);
      x->i_sigs = NULL;
    }  


  free(x->od.type_str);
  x->od.type_str = NULL;

  if(x->od.name_str)
    {
      free(x->od.name_str);
      x->od.name_str = NULL;
    }

  g_dwg_cnt--;

  if(x)
    {  
      free(x);
      x = NULL;
    }
}


/*****mix_bus*******/
/*******************/
void mix_bus_dsp_loop(int samples, t_mix_bus *x){

  int i, j, s;
  for(j=0; j<samples; j++)
    {
      x->o_sigs[0]->s_block[j] = 0.0;//clear the old data
      //printf("%d %d : ",j, x->i_sig_cnt);  
      for(i=0; i<x->i_sig_cnt; i++)
	{
	  x->o_sigs[0]->s_block[j] += x->i_sigs[i]->s_block[j];

	  //write in the sum of the inputs 
	}
      /* printf("%f %f %f\n", x->i_sigs[0]->s_block[i], */
      /* 	     x->i_sigs[1]->s_block[i], */
      /* 	     x->o_sigs[0]->s_block[i]); */
    }
}

void *mix_bus_init(int ins, int up){

  int i;
  t_mix_bus *x = (t_mix_bus *)malloc(sizeof(t_mix_bus));

  x->dsp_func = (t_dsp_func)&mix_bus_dsp_loop;
  x->o_sig_cnt = 1;
  x->i_sig_cnt = ins;
  
  x->i_sigs = (t_tlsig **)malloc(sizeof(t_tlsig *) * x->i_sig_cnt);
  for(i=0; i<x->i_sig_cnt; i++)
    x->i_sigs[i] = NULL;

  x->o_sigs = init_tlsigs(x->o_sig_cnt, 0, up);

  x->up = up;

  return(x);

}

void kill_mix_bus(t_mix_bus *x){


  if(x->o_sigs)
    kill_tlsigs(x->o_sigs, x->o_sig_cnt);

  if(x->i_sigs)
    {
      free(x->i_sigs);
      x->i_sigs = NULL;
    }

  if(x)
    {
      free(x);
      x = NULL;
    }

}


/********diff**********/
/**********************/
g_diff_cnt = 0;

void diff_dsp_loop(int samples, t_diff *x){
  int i, j;
  
  for(j=0; j<x->channs; j++)
    {
      //first sample minus last sample from last time
      x->o_sigs[j]->s_block[0] = 
	x->i_sigs[j]->s_block[0] - x->danglers[j];
 
      //diffs inside     
      for(i=1; i<samples; i++)
	x->o_sigs[j]->s_block[i] = 
	  x->i_sigs[j]->s_block[i] - x->i_sigs[j]->s_block[i-1]; 	
     //store the last sample for next time
      x->danglers[j] = x->i_sigs[j]->s_block[samples-1];
    }

}


void *diff_init(int channs, int up){

  t_diff *x = (t_diff *)malloc(sizeof(t_diff));
  x->dsp_func = (t_dsp_func)&diff_dsp_loop;

  x->channs = channs;
  
  x->danglers = (t_tlsmp *)malloc(sizeof(t_tlsmp) * channs);
  int i;
  for(i=0; i<channs; i++)
    x->danglers[i] = 0.0;

  x->o_sigs = init_tlsigs(x->channs, 0, up);
  x->i_sigs = init_tlsigs(x->channs, 1, up);

  x->od.type_str = name_new("diff");
  x->od.name_str = NULL;

  x->od.obj_cnt = g_diff_cnt;
  x->od.reg_place = -1;
  g_diff_cnt++;
  
  x->od.type = 2;
  x->od.sigs = x->o_sigs;
  x->od.sig_cnt = channs;
  x->od.this_type_cnt = g_diff_cnt;

  return(x);


}

void kill_diff(t_diff *x){

  if(x->i_sigs)
    {
      free(x->i_sigs);
      x->i_sigs = NULL;
    }

  if(x->o_sigs)
    kill_tlsigs(x->o_sigs, x->channs);

  free(x->od.type_str);
  x->od.type_str = NULL;

  if(x->od.name_str)
    {
      free(x->od.name_str);
      x->od.name_str = NULL;
    }

  g_diff_cnt--;
  if(x)
    {
      free(x);
      x= NULL;
    }


}


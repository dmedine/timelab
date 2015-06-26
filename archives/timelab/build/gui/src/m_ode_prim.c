/*objects for numerical solutions to ODEs*/


#include "m_ode_prim.h"
#include "stdio.h"
#include "stdlib.h"

/********runge kutta method***********/
g_rk_cnt = 0;
t_tlsmp rk_child_stage(int stage_num,//which stage in the rg algorithm
			t_rk_child *x){

    t_tlsmp datum = x->state + (x->mult[stage_num] * x->ks[stage_num]);
   return datum;   
 }


 void rk_child_estimate(t_rk_child *x){

   t_tlsmp g;
   g = (g_one_sixth *
	x->h_time *

	(x->ks[1] +
	 (2*x->ks[2]) +
	 (2*x->ks[3]) +
	 x->ks[4]) *

	x->scalar);


   x->state = x->state + g;


 }


t_rk_child *rk_child_init(t_rk_mother *y, int h){

   int i;
   t_rk_child *x = (t_rk_child *)malloc(sizeof(t_rk_child));
   x->input = 0.0;

   x->ks = (t_tlsmp *)malloc(5 * sizeof(t_tlsmp));
   //we maintain an extra 'k' in the algorithm that is set to zero,
   //just for ease of coding
   x->ks[0] = 0.0;


   x->mult[0] = 0.0;
   x->mult[1] = x->mult[2] = y->half_h_time;
   x->mult[3] = y->h_time;

   x->state = 0.0;//user must adjust these if wished
   x->scalar = 1.0;
   x->offset = 0.0;

   x->up = y->up;
   x->h = h;
   x->h_time = (1.0/(t_tlsmp)x->h) * g_sample_time; 
   x->half_h_time = .5 * x->h_time;

   x->gain = 1.0;

   x->input = 0.0;
   x->state = 0.0;

   return(x);

 }

 void kill_rk_child(t_rk_child *x){

   if(x->ks)
     free(x->ks);
   x->ks = NULL;
   free(x);
   x = NULL;

 }

 t_rk_mother *rk_mother_init(void *rk_dsp_func,
			     int child_cnt,
			     int c_sig_cnt,
			     int h,
			     int up){

   int i;

   t_rk_mother *x = (t_rk_mother *)malloc(sizeof(t_rk_mother));

   x->child_cnt = child_cnt;
   x->i_sig_cnt = x->child_cnt;
   x->o_sig_cnt = x->child_cnt;
   x->c_sig_cnt = c_sig_cnt;

   x->dsp_func = (t_rk_mother_func_ptr)rk_dsp_func;//defined outside the object
   x->o_sigs = init_tlsigs(x->o_sig_cnt, 0, h*up);

   x->i_sigs = (t_tlsig **)malloc(x->i_sig_cnt * sizeof(t_tlsig *));

   for(i=0; i<x->i_sig_cnt; i++)
     x->i_sigs[i] = g_empty_sig;   

   x->c_sigs = init_tlsigs(x->c_sig_cnt, 
			    1,//control signals are inputs 
			    h*up);//and flow at the (upsampled) audio rate

   x->up = up;
   x->h = h;
   x->h_time = ((t_tlsmp)x->h)/g_samplerate;
   x->half_h_time = .5 * x->h_time;
   
   x->rk_children = 
     (t_rk_child **)malloc(sizeof(t_rk_child*) * x->child_cnt);
   for(i=0; i<x->child_cnt; i++)
     x->rk_children[i] = rk_child_init(x, h);


   x->od.type_str = name_new("rk_solver");
   x->od.name_str = NULL;
   
   x->od.reg_place = -1;
   
   x->od.type = M_ODE_TYPE;
   x->od.sigs = x->o_sigs;
   x->od.sig_cnt = x->o_sig_cnt;
   x->od.this_type_cnt = ++g_rk_cnt;
   
   


   return(x);
}


void kill_rk_mother(t_rk_mother *x){


  int i;

  if(x->o_sigs)
    kill_tlsigs(x->o_sigs, x->o_sig_cnt);

  free(x->i_sigs);
  x->i_sigs = NULL; 

  for(i=0; i<x->child_cnt; i++)
    {
      if(x->rk_children[i])
  	kill_rk_child(x->rk_children[i]);
    }
  free(x->rk_children);
  x->rk_children = NULL;


  free(x->od.type_str);
  x->od.type_str = NULL;
  
  if(x->od.name_str)
    {
      free(x->od.name_str);
      x->od.name_str = NULL;
    }

  g_rk_cnt--;



  free(x);
  x = NULL;
}

/**********central difference method************/
/* Central difference approximation primitive object */
g_cd_cnt = 0;

t_tlsmp cd_child_stage(int stage_num, t_cd_child *x){

  t_tlsmp datum = x->state + (x->coeffs[stage_num] * x->h_time);
  return (datum);
 
}

void cd_child_estimate(t_cd_child *x){

  t_tlsmp dx;
  dx = (x->sides[0] - 2*x->state + x->sides[1])/(x->h_time * x->h_time);
  
  x->state = x->state + dx;


 }


t_cd_child *cd_child_init(t_cd_mother *y, int h){

   int i;

   t_cd_child *x = (t_cd_child *)malloc(sizeof(t_cd_child));
   x->input = 0.0;

   //only first order so far
   x->coeff_cnt = 2;
   x->sides = (t_tlsmp *)malloc(x->coeff_cnt * sizeof(t_tlsmp));
   x->sides[0] = 0.0;
   x->sides[1] = 0.0;
   x->coeffs = (t_tlsmp *)malloc(x->coeff_cnt * sizeof(t_tlsmp));
   x->coeffs[0] = 1.0;
   x->coeffs[1] = -1.0;


   x->state = 0.0;//user must adjust these if wished
   x->scalar = 1.0;
   x->offset = 0.0;

   x->up = y->up;
   x->h = h;
   x->h_time = (1.0/(t_tlsmp)x->h) * g_sample_time; 
   x->half_h_time = .5 * x->h_time;

   x->gain = 1.0;

   x->input = 0.0;
   x->state = 0.0;

   return(x);

 }

 void kill_cd_child(t_cd_child *x){

   if(x->sides)
     free(x->sides);
   x->sides = NULL;
   if(x->coeffs)
     free(x->coeffs);
   x->coeffs = NULL;
   free(x);
   x = NULL;


 }

 t_cd_mother *cd_mother_init(void *cd_dsp_func,
			     int child_cnt,
			     int c_sig_cnt,
			     int h,
			     int up){

   int i;

   t_cd_mother *x = (t_cd_mother *)malloc(sizeof(t_cd_mother));

   x->child_cnt = child_cnt;
   x->i_sig_cnt = x->child_cnt;
   x->o_sig_cnt = x->child_cnt;
   x->c_sig_cnt = c_sig_cnt;

   x->dsp_func = (t_cd_mother_func_ptr)cd_dsp_func;//defined outside the object
   x->o_sigs = init_tlsigs(x->o_sig_cnt, 0, h*up);

   x->i_sigs = (t_tlsig **)malloc(x->i_sig_cnt * sizeof(t_tlsig *));

   for(i=0; i<x->i_sig_cnt; i++)
     x->i_sigs[i] = g_empty_sig;   

   x->c_sigs = init_tlsigs(x->c_sig_cnt, 
			    1,//control signals are inputs 
			    h*up);//and flow at the (upsampled) audio rate
   x->up = up;
   x->h = h;
   x->h_time = ((t_tlsmp)x->h)/g_samplerate;
   x->half_h_time = .5 * x->h_time;
   
   x->cd_children = 
     (t_cd_child **)malloc(sizeof(t_cd_child*) * x->child_cnt);
   for(i=0; i<x->child_cnt; i++)
     x->cd_children[i] = cd_child_init(x, h);

   x->od.type_str = name_new("cd_solver");
   x->od.name_str = NULL;
   
   x->od.reg_place = -1;
   
   x->od.type = M_ODE_TYPE;
   x->od.sigs = x->o_sigs;
   x->od.sig_cnt = x->o_sig_cnt;
   x->od.this_type_cnt = ++g_cd_cnt;



   return(x);
}


void kill_cd_mother(t_cd_mother *x){


  int i;

  if(x->o_sigs)
    kill_tlsigs(x->o_sigs, x->o_sig_cnt);

  free(x->i_sigs);
  x->i_sigs = NULL; 

  for(i=0; i<x->child_cnt; i++)
    {
      if(x->cd_children[i])
  	kill_cd_child(x->cd_children[i]);
    }
  free(x->cd_children);
  x->cd_children = NULL;

  free(x->od.type_str);
  x->od.type_str = NULL;
  
  if(x->od.name_str)
    {
      free(x->od.name_str);
      x->od.name_str = NULL;
    }

  g_cd_cnt--;

  free(x);
  x = NULL;
}

/**********NSV (semi-implicit Euler) method************/
/* Central difference approximation primitive object */

g_nsv_cnt = 0;
void nsv_child_estimate(t_nsv_child *x){

  x->state = x->state + x->dx;

}


t_nsv_child *nsv_child_init(t_nsv_mother *y, int h){

   int i;

   t_nsv_child *x = (t_nsv_child *)malloc(sizeof(t_nsv_child));
   x->input = 0.0;

   x->dx = 0.0;

   x->state = 0.0;//user must adjust these if wished
   x->scalar = 1.0;
   x->offset = 0.0;

   x->up = y->up;
   x->h = h;
   x->h_time = (1.0/(t_tlsmp)x->h) * g_sample_time; 
   x->half_h_time = .5 * x->h_time;

   x->gain = 1.0;

   x->input = 0.0;
   x->state = 0.0;

   return(x);

 }

 void kill_nsv_child(t_nsv_child *x){

   free(x);
   x = NULL;


 }

 t_nsv_mother *nsv_mother_init(void *nsv_dsp_func,
			       int child_cnt,
			       int c_sig_cnt,
			       int h,
			       int up){

   int i;

   t_nsv_mother *x = (t_nsv_mother *)malloc(sizeof(t_nsv_mother));

   x->child_cnt = child_cnt;
   x->i_sig_cnt = x->child_cnt;
   x->o_sig_cnt = x->child_cnt;
   x->c_sig_cnt = c_sig_cnt;

   x->dsp_func = (t_nsv_mother_func_ptr)nsv_dsp_func;//defined outside the object
   x->o_sigs = init_tlsigs(x->o_sig_cnt, 0, h*up);

   x->i_sigs = (t_tlsig **)malloc(x->i_sig_cnt * sizeof(t_tlsig *));

   for(i=0; i<x->i_sig_cnt; i++)
     x->i_sigs[i] = g_empty_sig;   

   x->c_sigs = init_tlsigs(x->c_sig_cnt, 
			    1,//control signals are inputs 
			    h*up);//and flow at the (upsampled) audio rate
   x->up = up;
   x->h = h;
   x->h_time = ((t_tlsmp)x->h)/g_samplerate;
   x->half_h_time = .5 * x->h_time;
   
   x->nsv_children = 
     (t_nsv_child **)malloc(sizeof(t_nsv_child*) * x->child_cnt);
   for(i=0; i<x->child_cnt; i++)
     x->nsv_children[i] = nsv_child_init(x, h);



   x->od.type_str = name_new("nsv_solver");
   x->od.name_str = NULL;
   
   x->od.reg_place = -1;
   
   x->od.type = M_ODE_TYPE;
   x->od.sigs = x->o_sigs;
   x->od.sig_cnt = x->o_sig_cnt;
   x->od.this_type_cnt = ++g_nsv_cnt;


   return(x);
}


void kill_nsv_mother(t_nsv_mother *x){


  int i;

  if(x->o_sigs)
    kill_tlsigs(x->o_sigs, x->o_sig_cnt);

  free(x->i_sigs);
  x->i_sigs = NULL; 

  for(i=0; i<x->child_cnt; i++)
    {
      if(x->nsv_children[i])
  	kill_nsv_child(x->nsv_children[i]);
    }
  free(x->nsv_children);
  x->nsv_children = NULL;

  free(x->od.type_str);
  x->od.type_str = NULL;
  
  if(x->od.name_str)
    {
      free(x->od.name_str);
      x->od.name_str = NULL;
    }

  g_nsv_cnt--;


  free(x);
  x = NULL;
}

/**********(naive b-series attempt method************/
/*I belive that setting h to 1 makes this method identical to
  forward Euler, but I have to think a little bit more about that claim*/

g_bser_cnt = 0;

t_tlsmp bser_child_stage1(int stage_num, t_bser_child *x){

  t_tlsmp weight = (1.0 - x->mult[stage_num]);
  t_tlsmp out = x->state * weight;
  //printf("stage1 %f %f %f\t", weight, x->state, out);
  //fprintf(fp1, "%f ", out);
  return(out);

}

t_tlsmp bser_child_stage2(int stage_num, t_bser_child *x){

  t_tlsmp weight = x->mult[stage_num];
  t_tlsmp out = (x->state+x->h_time) * weight;
  //printf("stage2 %f %f %f\n", weight, x->state+x->h_time, out);
  //fprintf(fp1, "%f \n", out);
  return(out);
}

void bser_child_stage(int stage_num, t_bser_child *x, t_tlsmp *out){
  
  t_tlsmp weight1, weight2;
  weight1 = 1.0-x->mult[stage_num];
  weight2 = x->mult[stage_num];

  *out = x->state*weight1 + (x->state+x->h_time) * weight2;
  
}

void bser_child_estimate(t_bser_child *x){
  
  t_tlsmp dx = 0.0;
  int i;

  for(i=0; i<x->h+1; i++)
    dx+=x->fs[i];
  dx/=(x->h+1);
  dx*=x->h_time;
  x->state = x->state + dx;
  //printf("dx:%f\n",dx);
}


t_bser_child *bser_child_init(t_bser_mother *y, int h){

   int i;

   t_bser_child *x = (t_bser_child *)malloc(sizeof(t_bser_child));
  
   x->fs = (t_tlsmp *)malloc(sizeof(t_tlsmp) * (h+1));
   x->mult = (t_tlsmp *)malloc(sizeof(t_tlsmp) * (h+1));
   for(i=0; i<(h+1); i++)
     {
       x->mult[i] = (t_tlsmp)i/(t_tlsmp)h;
       printf("%f\n", x->mult[i]);
       x->fs[i] = 0.0;
     }   

   x->state = 0.0;//user must adjust these if wished

   x->up = y->up;  
   x->h = h;
   x->h_time = (1.0/(t_tlsmp)x->h) * g_sample_time; 
   x->half_h_time = .5 * x->h_time;

   return(x);

 }

 void kill_bser_child(t_bser_child *x){

   if(x->fs)
     {
       free(x->fs);
       x->fs = NULL;
     }
   if(x->mult)
     {
       free(x->mult);
       x->mult= NULL;
     }
   free(x);
   x = NULL;


 }

 t_bser_mother *bser_mother_init(void *bser_dsp_func,
				 int child_cnt,
				 int c_sig_cnt,
				 int h,
				 int up){

   int i;
   //fp1 = fopen("bser_children_out", "w");
   t_bser_mother *x = (t_bser_mother *)malloc(sizeof(t_bser_mother));

   x->child_cnt = child_cnt;
   x->i_sig_cnt = x->child_cnt;
   x->o_sig_cnt = x->child_cnt;
   x->c_sig_cnt = c_sig_cnt;

   x->dsp_func = (t_bser_mother_func_ptr)bser_dsp_func;//defined outside the object
   x->o_sigs = init_tlsigs(x->o_sig_cnt, 0, h*up);

   x->i_sigs = (t_tlsig **)malloc(x->i_sig_cnt * sizeof(t_tlsig *));

   for(i=0; i<x->i_sig_cnt; i++)
     x->i_sigs[i] = g_empty_sig;   

   x->c_sigs = init_tlsigs(x->c_sig_cnt, 
			    1,//control signals are inputs 
			    h*up);//and flow at the (upsampled) audio rate
   x->up = up;
   x->h = h;
   x->h_time = ((t_tlsmp)x->h)/g_samplerate;
   x->half_h_time = .5 * x->h_time;
   
   x->bser_children = 
     (t_bser_child **)malloc(sizeof(t_bser_child*) * x->child_cnt);
   for(i=0; i<x->child_cnt; i++)
     x->bser_children[i] = bser_child_init(x, h);


   x->od.type_str = name_new("bser_solver");
   x->od.name_str = NULL;
   
   x->od.reg_place = -1;
   
   x->od.type = M_ODE_TYPE;
   x->od.sigs = x->o_sigs;
   x->od.sig_cnt = x->o_sig_cnt;
   x->od.this_type_cnt = ++g_bser_cnt;
   


   return(x);
}


void kill_bser_mother(t_bser_mother *x){


  int i;

  if(x->o_sigs)
    kill_tlsigs(x->o_sigs, x->o_sig_cnt);

  free(x->i_sigs);
  x->i_sigs = NULL; 

  for(i=0; i<x->child_cnt; i++)
    {
      if(x->bser_children[i])
  	kill_bser_child(x->bser_children[i]);
    }
  free(x->bser_children);
  x->bser_children = NULL;


  free(x->od.type_str);
  x->od.type_str = NULL;
  
  if(x->od.name_str)
    {
      free(x->od.name_str);
      x->od.name_str = NULL;
    }
  g_bser_cnt--;

  free(x);
  x = NULL;
}


/**********(leapfrog method************/
/*I belive that setting h to 1 makes this method identical to
  forward Euler, but I have to think a little bit more about that claim*/

  /* equation for leapfrog  */
  /* x(i+1) = x(i) + v(i)del_t + 1/2a(i)del_t^2 */
  /* v(i+1) = v(i) + 1/2(a(i) + a(i+1))del_t */  

g_lf_cnt = 0;
void lf_child_estimate_v(t_lf_child *x, t_tlsmp ai, t_tlsmp ai1){
  
  t_tlsmp dv = 0.0;
  dv = .5 * (ai + ai1) * x->h_time;
  x->state = x->state + dv;
  //printf("dx:%f\n",dv);
}

void lf_child_estimate_x(t_lf_child *x, t_tlsmp ai, t_tlsmp vi){
  
  t_tlsmp dx = 0.0;
  dx = (vi*x->h_time) + (.5 * ai * x->h_time_sqrd);
  x->state = x->state + dx;
  //printf("dx:%f\n",dx);
}


t_lf_child *lf_child_init(t_lf_mother *y, int h){

   int i;

   t_lf_child *x = (t_lf_child *)malloc(sizeof(t_lf_child));
  
   x->state = 0.0;//user must adjust these if wished

   x->up = y->up;  
   x->h = h;
   x->h_time = (1.0/(t_tlsmp)x->h) * g_sample_time; 
   x->h_time_sqrd = x->h_time * x->h_time;
   x->half_h_time = .5 * x->h_time;

   return(x);

 }

 void kill_lf_child(t_lf_child *x){

   if(x)
     {
       free(x);
       x = NULL;
     }

 }

 t_lf_mother *lf_mother_init(void *lf_dsp_func,
			     int child_cnt,
			     int c_sig_cnt,
			     int h,
			     int up){

   int i;
   
   t_lf_mother *x = (t_lf_mother *)malloc(sizeof(t_lf_mother));

   x->child_cnt = child_cnt;
   x->i_sig_cnt = x->child_cnt;
   x->o_sig_cnt = x->child_cnt;
   x->c_sig_cnt = c_sig_cnt;

   x->dsp_func = (t_lf_mother_func_ptr)lf_dsp_func;//defined outside the object
   x->o_sigs = init_tlsigs(x->o_sig_cnt, 0, h*up);//this needs to be addressed!!!

   x->i_sigs = (t_tlsig **)malloc(x->i_sig_cnt * sizeof(t_tlsig *));

   for(i=0; i<x->i_sig_cnt; i++)
     x->i_sigs[i] = g_empty_sig;   

   x->c_sigs = init_tlsigs(x->c_sig_cnt, 
			    1,//control signals are inputs 
			    h*up);//and flow at the (upsampled) audio rate
   x->up = up;
   x->h = h;
   x->h_time = ((t_tlsmp)x->h)/g_samplerate;
   x->half_h_time = .5 * x->h_time;
   
   x->lf_children = 
     (t_lf_child **)malloc(sizeof(t_lf_child*) * x->child_cnt);
   for(i=0; i<x->child_cnt; i++)
     x->lf_children[i] = lf_child_init(x, h);

   x->od.type_str = name_new("lf_solver");
   x->od.name_str = NULL;
   
   x->od.reg_place = -1;
   
   x->od.type = M_ODE_TYPE;
   x->od.sigs = x->o_sigs;
   x->od.sig_cnt = x->o_sig_cnt;
   x->od.this_type_cnt = ++g_lf_cnt;


   return(x);
}


void kill_lf_mother(t_lf_mother *x){


  int i;

  if(x->o_sigs)
    kill_tlsigs(x->o_sigs, x->o_sig_cnt);

  free(x->i_sigs);
  x->i_sigs = NULL; 

  for(i=0; i<x->child_cnt; i++)
    {
      if(x->lf_children[i])
  	kill_lf_child(x->lf_children[i]);
    }
  free(x->lf_children);
  x->lf_children = NULL;

  free(x->od.type_str);
  x->od.type_str = NULL;
  
  if(x->od.name_str)
    {
      free(x->od.name_str);
      x->od.name_str = NULL;
    }

  g_lf_cnt--;


  free(x);
  x = NULL;
}

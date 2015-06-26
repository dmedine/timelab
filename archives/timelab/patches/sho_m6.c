#include "stdio.h"
#include "math.h"
#include "stdlib.h"
//#include "a_pa.h"
#include "g_api.h"
//#include "ui_main.h"
#include "m_modules.h"
#include "m_ode_prim.h"
#include "limits.h"

FILE *fp1, *fp2;

static t_rk_mother *rk_osc;

int g_h = 1;
int g_up = 1;

int ppos = 1;

int no_mass = 3;
int no_spring;

t_lin_ctl **mass_ctls;
t_lin_ctl **sc_ctls;
t_lin_ctl **slen_ctls;
t_lin_ctl **dac_amp_ctls;
t_lin_ctl **dac_input_ctls;

t_tlsmp *ks;//arrays to hold the coefficients
t_tlsmp *ms;
t_tlsmp *lens;

t_tlsmp mass = .0001;//initial constants
t_tlsmp sc   = 100;
t_tlsmp slen = 0;
t_tlsmp h_squared;
t_tlsmp youngs = 1;
t_tlsmp rho= 1;
t_tlsmp m0 = 0; // mass of left wall
t_tlsmp mn = 0; // mass of right wall

/*****************/

void reset_oscs(void);

void init_func(t_tlsmp *func_vec, int pts, int pos, int type){

  int i;
 
  t_tlsmp upslope, downslope;
  t_tlsmp val = 0.0;
  if(type == 0)//pluck function
    {
      upslope = 1.0/(pos+1);//account for 0 value boundary
      downslope = -1.0/(pts-pos);// "
      for(i=0;i<pos+1;i++)
	{
	  val+=upslope;
	  func_vec[i] = val;
	  printf("%d %f\n", i, val);
	}
      for(i=pos+1;i<pts;i++)
	{

	  val+=downslope;
	  func_vec[i] = val;
	  printf("%d %f\n", i, val);
	
	}
    }
}

static t_tlsmp y_dot(t_tlsmp xn, t_tlsmp xl, t_tlsmp xr, t_tlsmp kl, t_tlsmp kr, t_tlsmp l_len, t_tlsmp r_len, t_tlsmp m){

  t_tlsmp out;

  out = -1.0*kl * (xn - xl - l_len) - kr*(xn- xr - r_len);
  out /= m;

  return out;

}

static t_tlsmp x_dot(t_tlsmp y){

  return(y);

}


static void rk_osc_func(int samples, void *ptr, t_tlsmp *input){

  t_rk_mother *x = ptr;

  int s = samples * x->h * x->up;
  int i, j, k;
  t_tlsmp vel[no_mass], pos[no_mass];

  
  for(i=0; i<s; i++)
    {

      for(j=0; j<4; j++)
	{
	  for(k=0; k<no_mass; k++)
	    {

	      vel[k] = rk_child_stage(j, x->rk_children[k]);
	      pos[k] = rk_child_stage(j, x->rk_children[k+no_mass]);
	    }	      

	  //spring constants
	  for(k=0; k<no_mass+1; k++)
	    {
	      ks[k] = sc_ctls[k]->ctl_sig->s_block[i];
	      lens[k] = slen_ctls[k]->ctl_sig->s_block[i];
	    }

	  //masses
	  for(k=0; k<no_mass; k++)
	    ms[k] = mass_ctls[k]->ctl_sig->s_block[i];
	  
	  // left wall
	  x->rk_children[0]->ks[j+1] = 
	    y_dot(pos[0], 0, pos[1], ks[0], ks[1], lens[0], lens[1], ms[0]);
	  
	  // inner displacements
	  for(k=1; k<no_mass-1; k++)
	    x->rk_children[k]->ks[j+1] = 
	      y_dot(pos[k], pos[k-1], pos[k+1], ks[k], ks[k+1], lens[k], lens[k+1], ms[k]);
	  
	  // right wall
	  x->rk_children[no_mass-1]->ks[j+1] = 
	    y_dot(pos[no_mass-1], pos[no_mass-2], 0, ks[no_mass-1], ks[no_mass], lens[no_mass-1], lens[no_mass], ms[no_mass-1]);
	  
	  for(k=no_mass; k<no_mass*2; k++)
	    {
	      x->rk_children[k]->ks[j+1] = 
		x_dot(vel[k-no_mass]);
	      
	    }
	  

	}
    
      //printf("%d\n", i);
      for(k=0; k<no_mass*2; k++)
	{
	  rk_child_estimate(x->rk_children[k]);      
	  x->o_sigs[k]->s_block[i] = x->rk_children[k]->state;
	  /* if(k>=no_mass) */
	  /*   printf("state : %d val : %1.5f  ", k, x->rk_children[k]->state); */
	  
	}
      //      printf("\n");
    }

}

void setup_this(void){
  
  int i;
  int h = g_h;
  int up = g_up;
  t_tlsmp *func_vec;

  func_vec = (t_tlsmp *)malloc(sizeof(t_tlsmp) * no_mass);

  init_func(func_vec, no_mass, ppos, 0);
  no_spring = no_mass+1;

  rk_osc = (t_rk_mother *)rk_mother_init(&rk_osc_func,
					 no_mass *2,// 2 for each mass
					 0,//get rid of this arg
					 h,//step size factor
					 up);//upsampling factor


  for(i=0;i<no_mass; i++)  
    rk_osc->rk_children[no_mass+i]->state = func_vec[i];

  dac_amp_ctls = (t_lin_ctl **)malloc(sizeof(t_lin_ctl*) * 2);
  mass_ctls    = (t_lin_ctl **)malloc(sizeof(t_lin_ctl*) * no_mass);
  sc_ctls      = (t_lin_ctl **)malloc(sizeof(t_lin_ctl*) * no_spring);
  slen_ctls      = (t_lin_ctl **)malloc(sizeof(t_lin_ctl*) * no_spring);

  ms           = (t_tlsmp *)malloc(sizeof(t_tlsmp) * no_mass);
  ks           = (t_tlsmp *)malloc(sizeof(t_tlsmp) * no_spring);
  lens         = (t_tlsmp *)malloc(sizeof(t_tlsmp) * no_spring);


  for(i=0; i<2; i++)
    {
      dac_amp_ctls[i] = init_lin_ctl(i, CTL_T_LIN, h*up);//0,1
      level_lin_ctl(dac_amp_ctls[i], .7);
    }

  for(i=0; i<no_mass; i++)
    {
      mass_ctls[i] = init_lin_ctl(i+2, CTL_T_LIN, h*up);//0,1
      level_lin_ctl(mass_ctls[i], mass);
    }


  for(i=0; i<no_spring; i++)
    {
      sc_ctls[i] = init_lin_ctl(i+2+no_mass, CTL_T_LIN, h*up);//0,1
      level_lin_ctl(sc_ctls[i], sc);
    }

  for(i=0; i<no_spring; i++)
    {
      slen_ctls[i] = init_lin_ctl(i+2+no_mass+no_spring, CTL_T_LIN, h*up);//0,1
      level_lin_ctl(slen_ctls[i], slen);
    }


 
  install_obj(&rk_osc->od);

  fp1 = fopen("sho_m6_out_sprng", "w");
  fp2 = fopen("sho_m6_out_mass", "w");

  free(func_vec);  
}


void do_kill(void){

  int i;

  kill_rk_mother(rk_osc);
  
  free(dac_amp_ctls);
  free(mass_ctls);
  free(sc_ctls);
  free(slen_ctls);
  
  free(ms);
  free(ks);
  free(lens);

  fclose(fp1);
  fclose(fp2);
}

void dsp_chain(int samples,
	       t_tlsig **adc_sigs,
	       t_tlsig **dac_sigs){
  
  int i, j;
  int s = samples;
  
  rk_osc->dsp_func(s, rk_osc, NULL);
  
  dac_sigs[0] = rk_osc->o_sigs[no_mass];
  dac_sigs[1] = rk_osc->o_sigs[no_mass+1];
  

  multiply_sigs(dac_sigs[0], dac_amp_ctls[0]->ctl_sig);
  multiply_sigs(dac_sigs[1], dac_amp_ctls[1]->ctl_sig);

  /* for(i=0;i<s;i++) */
  /*   { */
  /*     for(j=0; j<no_mass; j++) */
  /* 	fprintf(fp1, "%1.6f, ",rk_osc->o_sigs[j]->s_block[i]); */
  /*     fprintf(fp1, "\n"); */

  /*     for(j=no_mass; j<no_mass*2; j++) */
  /* 	fprintf(fp2, "%1.6f, ",rk_osc->o_sigs[j]->s_block[i]); */
  /*     fprintf(fp2, "\n"); */

  /*   } */


  
}



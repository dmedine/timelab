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

int ppos = 10;

int no_mass = 21;

t_lin_ctl **mass_ctls;
t_lin_ctl **sc_ctls;
t_lin_ctl **dac_amp_ctls;
t_lin_ctl **dac_input_ctls;

//solve the wave equation for a bunch of masses and springs in the form
//   d^2u/dt^2 = (km1 * um1 - 2*k*u + k*up1)/m
//where u is displacement of a mass
//k is spring constant and m is mass
//m1 and p1 correspond to -1 and +1
//implemented as :
//   y_dot = (km1 * um1 - 2*k*u + k*up1)/m
//   x_dot = y


t_tlsmp *ks;//arrays to hold the coefficients
t_tlsmp *ms;

t_tlsmp mass = .0001;//initial constants
t_tlsmp sc   = 100;
t_tlsmp h_squared;
t_tlsmp youngs = 1;
t_tlsmp rho= 1;


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

static t_tlsmp y_dot(t_tlsmp ul, t_tlsmp u, t_tlsmp ur, t_tlsmp kl1, t_tlsmp kl2, t_tlsmp kr1, t_tlsmp kr2, t_tlsmp m){

  t_tlsmp out;

  //out = (kl1/m) * (ur - 2*u + ul);//(kr1+kr2) * ur - 2 * (kl1+kr1) * u + (kl1+kl2) * ul ;
  out = (kl1 + kl2) * ul- 2*(kr2 + kl1)*u + (kr1+kr2) * ur ;
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
  t_tlsmp d2us[no_mass], ys[no_mass];

  
  for(i=0; i<s; i++)
    {

      for(j=0; j<4; j++)
	{

	  //d2us
	  for(k=0; k<no_mass; k++)
	    {
	      //printf("k : %d\n", k);
	      ys[k] = rk_child_stage(j, x->rk_children[k]);
	    }
	  //ys
	  for(k=no_mass; k<no_mass*2; k++)
	    {
	      //printf("k : %d, k-no_mass : %d\n", k, k-no_mass);
	      d2us[k-no_mass] = rk_child_stage(j, x->rk_children[k]);
	    }
	  //spring constants
	  for(k=0; k<no_mass+1; k++)
	    ks[k] = sc_ctls[k]->ctl_sig->s_block[i];

	  //masses
	  for(k=0; k<no_mass; k++)
	    ms[k] = mass_ctls[k]->ctl_sig->s_block[i];

	  //d2us first
	  //boundaries
	  x->rk_children[0]->ks[j+1] = 
	    y_dot(0, d2us[0], d2us[1], 1000, ks[0]*.5, ks[1]*.5, ks[2] *.5, ms[0]);	 

	  x->rk_children[no_mass-1]->ks[j+1] = 
	    y_dot(d2us[no_mass-2], d2us[no_mass-1], 0, ks[no_mass - 2]*.5, ks[no_mass-1]*.5, ks[no_mass],1000, ms[no_mass-1]);//-1 to count from 0	 
	  //printf();

	  //inner d2us
	  for(k=1; k<no_mass-1; k++)
	    {
	      // printf("\tj : %d, k : %d\n", j, k);
	    x->rk_children[k]->ks[j+1] = 
	      y_dot(d2us[k-1], d2us[k], d2us[k+1], ks[k-1]*.5, ks[k]*.5, ks[k+1]*.5, ks[k+2] *.5, ms[k]);	 
	    }

	  //ys
	  for(k=no_mass; k<no_mass*2; k++)
	    {
	      //printf("k(ys):%d\n", k);
	      x->rk_children[k]->ks[j+1] = 
		x_dot(ys[k-no_mass]);

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


  rk_osc = (t_rk_mother *)rk_mother_init(&rk_osc_func,
					 no_mass * 2,//3,//
					 0,//get rid of this arg
					 h,//step size factor
					 up);//upsampling factor


  for(i=0;i<no_mass; i++)  
    rk_osc->rk_children[no_mass+i]->state = func_vec[i];

  dac_amp_ctls = (t_lin_ctl **)malloc(sizeof(t_lin_ctl*) * 2);
  mass_ctls    = (t_lin_ctl **)malloc(sizeof(t_lin_ctl*) * no_mass);
  sc_ctls      = (t_lin_ctl **)malloc(sizeof(t_lin_ctl*) * no_mass+1);

  ms           = (t_tlsmp *)malloc(sizeof(t_tlsmp) * no_mass);
  ks           = (t_tlsmp *)malloc(sizeof(t_tlsmp) * no_mass+1);


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


  for(i=0; i<no_mass+1; i++)
    {
      sc_ctls[i] = init_lin_ctl(i+2+no_mass, CTL_T_LIN, h*up);//0,1
      level_lin_ctl(sc_ctls[i], sc);
    }


 
  install_obj(&rk_osc->od);

  fp1 = fopen("sho_m5_out_sprng", "w");
  fp2 = fopen("sho_m5_out_mass", "w");

  free(func_vec);  
}


void do_kill(void){

  int i;

  kill_rk_mother(rk_osc);
  
  free(dac_amp_ctls);
  free(mass_ctls);
  free(sc_ctls);
  
  free(ms);
  free(ks);

  fclose(fp1);
  fclose(fp2);
}

void dsp_chain(int samples,
	       t_tlsig **adc_sigs,
	       t_tlsig **dac_sigs){
  
  int i, j;
  int s = samples;
  
  rk_osc->dsp_func(s, rk_osc, NULL);
    

  dac_sigs[0] = rk_osc->o_sigs[22];
  dac_sigs[1] = rk_osc->o_sigs[23];
  

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



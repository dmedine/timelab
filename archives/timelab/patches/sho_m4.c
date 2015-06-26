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

int no_mass = 10;

t_lin_ctl **mass_ctls;
t_lin_ctl **sc_ctls;
t_lin_ctl **dac_amp_ctls;

t_tlsmp mass = .001;//initial constants
t_tlsmp sc   = 1;

t_tlsmp *ms;//arrays to hold the control values
t_tlsmp *scs;

t_tlsmp *fs;//arrays to hold the states
t_tlsmp *ds;



/*****************/

void reset_oscs(void);

static t_tlsmp spring_func(t_tlsmp dl, t_tlsmp dr, t_tlsmp ml, t_tlsmp mr){

  //force
  t_tlsmp out = ml*dl + mr*dr;
  return out;

}

static t_tlsmp mass_func(t_tlsmp fl, t_tlsmp fr, t_tlsmp kl, t_tlsmp kr){

  t_tlsmp out;
  
  //displacement 
  out = -1.0 * kl * fl - kr * fr;
  return(out);

}


static void rk_osc_func(int samples, void *ptr, t_tlsmp *input){

  t_rk_mother *x = ptr;

  int s = samples * x->h * x->up;
  int i, j, k;
  
  for(i=0; i<s; i++)
    {

      for(j=0; j<4; j++)
	{

	  //forces
	  for(k=0; k<no_mass+1; k++)
	    {
	      //printf("k : %d\n", k);
	      fs[k] = rk_child_stage(j, x->rk_children[k]);
	    }
	  for(k=0; k<no_mass+1; k++)
	    scs[k] = sc_ctls[k]->ctl_sig->s_block[i];

	  //displacements
	  for(k=no_mass+1; k<no_mass*2+1; k++)
	    {
	      //printf("k : %d, k-no_mass-1 : %d\n", k, k-no_mass-1);
	      ds[k-no_mass-1] = rk_child_stage(j, x->rk_children[k]);
	    }

	  for(k=0; k<no_mass; k++)
	    ms[k] = mass_ctls[k]->ctl_sig->s_block[i];

	  //springs first
	  //boundaries
	  x->rk_children[0]->ks[j+1] = 
	    spring_func(0, ds[0], 0, ms[0]);	 

	  x->rk_children[no_mass]->ks[j+1] = //x->rk_children[no_mass+1-1]->ks[j+1]
	    spring_func(ds[no_mass-1], 0, ms[no_mass-1], 0);//-1 to count from 0	 
	  //printf("no_mass-1 : %d\n", no_mass-1);

	  //inner springs
	  for(k=1; k<no_mass; k++)
	    x->rk_children[k]->ks[j+1] = 
	      spring_func(ds[k-1], ds[k], ms[k-1], ms[k]);	 


	  //masses
	  for(k=no_mass+1; k<no_mass*2+1; k++)
	    {
	      //printf("no_mass : %d, no_mass*2+1 : %d,k : %d, k-no_mass-1 : %d\n", no_mass, no_mass*2+1, k, k-no_mass-1);
	      x->rk_children[k]->ks[j+1] = 
		mass_func(fs[k-no_mass-1], fs[k-no_mass], scs[k-no_mass-1], scs[k-no_mass]);	 

	    }

	  /* f1 = rk_child_stage(j, x->rk_children[0]); */
	  /* f2 = rk_child_stage(j, x->rk_children[2]); */
	  /* f3 = rk_child_stage(j, x->rk_children[4]); */
	  /* f4 = rk_child_stage(j, x->rk_children[6]); */

	  /* //displacements */
	  /* d1 = rk_child_stage(j, x->rk_children[1]); */
	  /* d2 = rk_child_stage(j, x->rk_children[3]); */
	  /* d3 = rk_child_stage(j, x->rk_children[5]); */

	  /* //spring1 */
	  /* x->rk_children[0]->ks[j+1] = */
	  /*   spring_func(0, d1, 0, mass1); */
	  
	  /* //mass1 */
	  /* x->rk_children[1]->ks[j+1] = */
	  /*   mass_func(f1, f2, sc1, sc2); */

	  /* //spring2 */
	  /* x->rk_children[2]->ks[j+1] = */
	  /*   spring_func(d1, d2, mass1, mass2); */
	  
	  /* //mass2 */
	  /* x->rk_children[3]->ks[j+1] = */
	  /*   mass_func(f2, f3, sc2, sc3);	  */

	  /* //spring3 */
	  /* x->rk_children[4]->ks[j+1] = */
	  /*   spring_func(d2, d3, mass2, mass3); */
	  
	  /* //mass3 */
	  /* x->rk_children[5]->ks[j+1] = */
	  /*   mass_func(f3, f4, sc3, sc4); */

	  /* //spring4 */
	  /* x->rk_children[6]->ks[j+1] = */
	  /*   spring_func(d3, 0, mass3, 0); */




	}

      for(k=0; k<no_mass*2+1; k++)
	{
	  rk_child_estimate(x->rk_children[k]);      
	  x->o_sigs[k]->s_block[i] = x->rk_children[k]->state;
	}

      /* rk_child_estimate(x->rk_children[0]); */
      /* rk_child_estimate(x->rk_children[1]); */
      /* rk_child_estimate(x->rk_children[2]); */
      /* rk_child_estimate(x->rk_children[3]); */
      /* rk_child_estimate(x->rk_children[4]); */
      /* rk_child_estimate(x->rk_children[5]); */
      /* rk_child_estimate(x->rk_children[6]); */
      
      /* x->o_sigs[0]->s_block[i] = x->rk_children[0]->state; */
      /* x->o_sigs[1]->s_block[i] = x->rk_children[1]->state; */
      /* x->o_sigs[2]->s_block[i] = x->rk_children[2]->state; */
      /* x->o_sigs[3]->s_block[i] = x->rk_children[3]->state; */
      /* x->o_sigs[4]->s_block[i] = x->rk_children[4]->state; */
      /* x->o_sigs[5]->s_block[i] = x->rk_children[5]->state; */
      /* x->o_sigs[6]->s_block[i] = x->rk_children[6]->state; */
            
    }

}

void setup_this(void){
  
  int i;
  int h = g_h;
  int up = g_up;

  rk_osc = (t_rk_mother *)rk_mother_init(&rk_osc_func,
					 no_mass*2+1,//7,//
					 0,//get rid of this arg
					 h,//step size factor
					 up);//upsampling factor

  rk_osc->rk_children[0]->state = -1.0;

  dac_amp_ctls = (t_lin_ctl **)malloc(sizeof(t_lin_ctl*) * 2);
  mass_ctls    = (t_lin_ctl **)malloc(sizeof(t_lin_ctl*) * no_mass);
  sc_ctls      = (t_lin_ctl **)malloc(sizeof(t_lin_ctl*) * no_mass+1);

  ms           = (t_tlsmp *)malloc(sizeof(t_tlsmp) * no_mass);
  scs          = (t_tlsmp *)malloc(sizeof(t_tlsmp) * no_mass+1);

  ds           = (t_tlsmp *)malloc(sizeof(t_tlsmp) * no_mass);
  fs           = (t_tlsmp *)malloc(sizeof(t_tlsmp) * no_mass+1);


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

  fp1 = fopen("sho_m4_out_sprng", "w");
  fp2 = fopen("sho_m4_out_mass", "w");
  
}


void do_kill(void){

  int i;

  kill_rk_mother(rk_osc);
  
  free(dac_amp_ctls);
  free(mass_ctls);
  free(sc_ctls);
  
  free(ms);
  free(scs);

  free(fs);
  free(ds);

  fclose(fp1);
  fclose(fp2);
}

void dsp_chain(int samples,
	       t_tlsig **adc_sigs,
	       t_tlsig **dac_sigs){
  
  int i, j;
  int s = samples;
  
  rk_osc->dsp_func(s, rk_osc, NULL);
    

  for(i=0;i<s;i++)
    {
      for(j=0; j<no_mass+1; j++)
	fprintf(fp1, "%1f, ",rk_osc->o_sigs[j]->s_block[i]);
      fprintf(fp1, "\n");

      for(j=no_mass+1; j<no_mass*2+1; j++)
	fprintf(fp2, "%1f, ",rk_osc->o_sigs[j]->s_block[i]);
      fprintf(fp2, "\n");

    }


  
}



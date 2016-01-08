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


t_lin_ctl *mass1_ctl, *mass2_ctl, *mass3_ctl;
t_lin_ctl *sc1_ctl, *sc2_ctl, *sc3_ctl, *sc4_ctl;
t_lin_ctl **dac_amp_ctls;

t_tlsmp mass1 = 3300;
t_tlsmp mass2 = 200;
t_tlsmp mass3 = 10;
t_tlsmp sc1  = 101;
t_tlsmp sc2  = 104;
t_tlsmp sc3  = 300;
t_tlsmp sc4  = 9;



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
  t_tlsmp d1, d2, d3, f1, f2, f3, f4;
  int s = samples * x->h * x->up;
  int i, j, k;
  
  for(i=0; i<s; i++)
    {

      mass1 = mass1_ctl->ctl_sig->s_block[i];
      mass2 = mass2_ctl->ctl_sig->s_block[i];
      mass3 = mass3_ctl->ctl_sig->s_block[i];
      sc1  = sc1_ctl->ctl_sig->s_block[i];
      sc2  = sc2_ctl->ctl_sig->s_block[i];
      sc3  = sc3_ctl->ctl_sig->s_block[i];
      sc4  = sc4_ctl->ctl_sig->s_block[i];


      for(j=0; j<4; j++)
	{
	  //forces
	  f1 = rk_child_stage(j, x->rk_children[0]);
	  f2 = rk_child_stage(j, x->rk_children[2]);
	  f3 = rk_child_stage(j, x->rk_children[4]);
	  f4 = rk_child_stage(j, x->rk_children[6]);

	  //displacements
	  d1 = rk_child_stage(j, x->rk_children[1]);
	  d2 = rk_child_stage(j, x->rk_children[3]);
	  d3 = rk_child_stage(j, x->rk_children[5]);
	  
	  //spring1
	  x->rk_children[0]->ks[j+1] =
	    spring_func(0, d1, 0, mass1);
	  
	  //mass1
	  x->rk_children[1]->ks[j+1] =
	    mass_func(f1, f2, sc1, sc2);

	  //spring2
	  x->rk_children[2]->ks[j+1] =
	    spring_func(d1, d2, mass1, mass2);
	  
	  //mass2
	  x->rk_children[3]->ks[j+1] =
	    mass_func(f2, f3, sc2, sc3);	 

	  //spring3
	  x->rk_children[4]->ks[j+1] =
	    spring_func(d2, d3, mass2, mass3);
	  
	  //mass3
	  x->rk_children[5]->ks[j+1] =
	    mass_func(f3, f4, sc3, sc4);

	  //spring4
	  x->rk_children[6]->ks[j+1] =
	    spring_func(d3, 0, mass3, 0);




	}

      
      rk_child_estimate(x->rk_children[0]);
      rk_child_estimate(x->rk_children[1]);
      rk_child_estimate(x->rk_children[2]);
      rk_child_estimate(x->rk_children[3]);
      rk_child_estimate(x->rk_children[4]);
      rk_child_estimate(x->rk_children[5]);
      rk_child_estimate(x->rk_children[6]);
      
      x->o_sigs[0]->s_block[i] = x->rk_children[0]->state;
      x->o_sigs[1]->s_block[i] = x->rk_children[1]->state;
      x->o_sigs[2]->s_block[i] = x->rk_children[2]->state;
      x->o_sigs[3]->s_block[i] = x->rk_children[3]->state;
      x->o_sigs[4]->s_block[i] = x->rk_children[4]->state;
      x->o_sigs[5]->s_block[i] = x->rk_children[5]->state;
      x->o_sigs[6]->s_block[i] = x->rk_children[6]->state;
            
    }

}

void setup_this(void){
  
  int i;
  int h = g_h;
  int up = g_up;

  rk_osc = (t_rk_mother *)rk_mother_init(&rk_osc_func,
					 7,//
					 0,//get rid of this arg
					 h,//step size factor
					 up);//upsampling factor

  rk_osc->rk_children[0]->state = -1.0;

  dac_amp_ctls = (t_lin_ctl **)malloc(sizeof(t_lin_ctl*) * 2);

  for(i=0; i<2; i++)
    {
      dac_amp_ctls[i] = init_lin_ctl(i, CTL_T_LIN, h*up);//0,1
      level_lin_ctl(dac_amp_ctls[i], .7);
    }

  mass1_ctl = init_lin_ctl(2, CTL_T_LIN, h*up);
  level_lin_ctl(mass1_ctl, mass1);

  mass2_ctl = init_lin_ctl(3, CTL_T_LIN, h*up);
  level_lin_ctl(mass2_ctl, mass2);

  mass3_ctl = init_lin_ctl(4, CTL_T_LIN, h*up);
  level_lin_ctl(mass3_ctl, mass3);

  sc1_ctl = init_lin_ctl(5, CTL_T_LIN, h*up);
  level_lin_ctl(sc1_ctl, sc1);

  sc2_ctl = init_lin_ctl(6, CTL_T_LIN, h*up);
  level_lin_ctl(sc2_ctl, sc2);

  sc3_ctl = init_lin_ctl(7, CTL_T_LIN, h*up);
  level_lin_ctl(sc3_ctl, sc3);

  sc4_ctl = init_lin_ctl(8, CTL_T_LIN, h*up);
  level_lin_ctl(sc4_ctl, sc4);
 
  install_obj(&rk_osc->od);

  fp1 = fopen("sho_m3_out_sprng", "w");
  fp2 = fopen("sho_m3_out_mass", "w");
  
}


void do_kill(void){

  int i;

  kill_rk_mother(rk_osc);
  free(dac_amp_ctls);
  fclose(fp1);
  fclose(fp2);
}

void dsp_chain(int samples,
	       t_tlsig **adc_sigs,
	       t_tlsig **dac_sigs){
  
  int i, j;
  int s = samples;
  
  rk_osc->dsp_func(s, rk_osc, NULL);
    
  cpy_sigs(dac_sigs[0], rk_osc->o_sigs[0]);
  add_sigs(dac_sigs[0], rk_osc->o_sigs[2]);
  add_sigs(dac_sigs[0], rk_osc->o_sigs[4]);
  add_sigs(dac_sigs[0], rk_osc->o_sigs[6]);

  cpy_sigs(dac_sigs[1], rk_osc->o_sigs[1]);
  add_sigs(dac_sigs[1], rk_osc->o_sigs[3]);
  add_sigs(dac_sigs[1], rk_osc->o_sigs[5]);

  for(i=0;i<s;i++)
    {
      fprintf(fp1, "%1f %1f %1f %1f 1%f\n", dac_sigs[0]->s_block[i], rk_osc->o_sigs[0]->s_block[i], rk_osc->o_sigs[2]->s_block[i], rk_osc->o_sigs[4]->s_block[i], rk_osc->o_sigs[6]->s_block[i]);

      fprintf(fp2, "%1f %1f %1f %1f\n", dac_sigs[1]->s_block[i], rk_osc->o_sigs[1]->s_block[i], rk_osc->o_sigs[3]->s_block[i], rk_osc->o_sigs[5]->s_block[i]);

    }

  multiply_sigs(dac_sigs[0], dac_amp_ctls[0]->ctl_sig);
  multiply_sigs(dac_sigs[1], dac_amp_ctls[1]->ctl_sig);

  
}



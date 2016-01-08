// solves a string consistint of N
// harmonic oscillators as a pair of differential equations
//   d^2u/dt^2 = (kn-1 * yn-1 - kn-1*yn - kn+1yn - kn+1yn+1)mn
// where u is displacement of a mass
// k is spring constant and m is mass
// m1 and p1 correspond to -1 and +1 neighbors to
// the current mass
// implemented as :
//   y_dot = (km1 * um1 - 2*k*u + kp1*up1)/m
//   x_dot = -y

// control input is an array of N values giving an intial
// 'pluck' displacement of the string

#include "tl_core.h"
#include "m_modules.h"
#include <stdlib.h>
#include <stdio.h>
#include <math.h>


static tl_UDS_node *node_x;
static tl_UDS_node *node_y;


static tl_UDS_solver *solver;
static tl_dac *dac;
static tl_ctl *ctl_head;
tl_smp wall_const; // used to terminate the ends of the string

tl_ctl **k_disp; // this is used to set an intial displacement/deformation of the string pts
tl_ctl *b_pluck; // bang this to set the displacements to the above

// bowing params
tl_ctl *k_bvel;
tl_ctl *k_bforce;
tl_ctl *k_bfric;

// number of modes to the string
#define MASS_CNT 4

// needed for external loaders
const int in_cnt = 0;
const int out_cnt = MASS_CNT*2;


typedef struct _string_pt{ // mass spring emelment on an ideal string


  tl_UDS_node *pos_node; // pos node
  tl_UDS_node *vel_node; // left vel

  // control pointers for the parameters
  tl_ctl *k_m; // mass 
  tl_ctl *k_Kl; // vel constants left
  tl_ctl *k_Kr; // spring constants right
  // in order to be realistic, the right constant
  // should be equal to the left constant of the next point
  tl_ctl *k_cl; // damping constant left
  tl_ctl *k_cr; // damping constant left

  tl_ctl *k_lenl; // equilibrium length of left spring
  tl_ctl *k_lenr; // equilibrium length of right spring
  // the same not applies to the damping constants
  tl_ctl *k_barea; // this is the percentage of the point
  // on the string that comes into contact with the bow
  
  tl_smp disp;
  tl_smp vel;
  
  int which;

}tl_string_pt;

tl_string_pt **string_pts;

// continuous bow velocity model
static inline tl_smp phi(tl_smp fric, tl_smp vel){

  tl_smp out = sqrt(2*fric) * vel * exp(-2.0 * fric * vel * vel + .5);
  // printf("fric : %f\nvel : %f\nfirst term : %f\n second term %f\nout : %f", fric, vel,  sqrt(2*fric) * vel, exp(-2.0 * fric * vel * vel + .5), out);
  return out;
  
}

static tl_string_pt *init_UDS_string_pt(int which);

// bang pluck function
void pluck(void){

  
  int i;
  tl_UDS_node *x=solver->UDS_net->next;
  tl_smp omega;
  for(i=0;i<MASS_CNT;i++) {

    // set the displacement according to orders
    string_pts[i]->disp = k_disp[i]->outlet->smps[0];
    tl_reset_UDS_node(x, string_pts[i]->disp);
    x = x->next;

    // determine the instantaneous velocity at this displacement
    // omega = k/m^1/2
    /* omega = sqrt( (.5 *(string_pts[i]->k_Kl->outlet->smps[0]+string_pts[i]->k_Kr->outlet->smps[0]))/string_pts[i]->k_m->outlet->smps[0]); */
    /* string_pts[i]->vel = (string_pts[i]->disp - (string_pts[i]->disp*sin(omega)))/cos(omega); */
    tl_reset_UDS_node(x, 0);//string_pts[i]->vel);
    x = x->next;

  }
}

static tl_smp pos_dot(tl_UDS_node *x, int iter){ 

  tl_smp out;
  tl_string_pt *y = (tl_string_pt *)x->extra_data;

    // the walls don't 'move' unless externally displaced
  if(y->which==0 | y->which == MASS_CNT-1)
    return y->disp;

  
  out = *x->data_in[0];///y->k_m->outlet->smps[iter];
  //y->k_ml->outlet->smps[iter] * *x->data_in[0] + y->k_mr->outlet->smps[iter] * *x->data_in[1];
  
  return out;
}

tl_smp vel_dot(tl_UDS_node *x, int iter){

  tl_smp out = 0;
  tl_string_pt *y = (tl_string_pt *)x->extra_data;

  tl_smp bow_contrib = 0;




  if(y->which == 0 | y->which == MASS_CNT-1)
    return 0;


    // bow model
  bow_contrib = y->k_barea->outlet->smps[iter] *
    k_bforce->outlet->smps[iter] *
    phi(k_bfric->outlet->smps[iter],
	(*x->data_out - k_bvel->outlet->smps[iter]));

  /* if(iter==0) */
    //    printf("%f\n", bow_contrib);
  /* if(iter==0) */
  /*   printf("a: %f\nfric:, %f\nfrc: %f\nv: %f\nc: %f\n", */
  /* 	   y->k_barea->outlet->smps[iter], */
  /* 	   k_bfric->outlet->smps[iter], */
  /* 	   k_bforce->outlet->smps[iter], */
  /* 	   k_bvel->outlet->smps[iter], */
  /* 	   bow_contrib); */
  // the walls always have a velocity of 0


  out =  // positional difference between this and the left mass
    -1.0 * y->k_Kl->outlet->smps[iter] * (*x->data_in[0] - *x->data_in[1] - y->k_lenl->outlet->smps[iter]) - // positional difference between this and the right mass
    y->k_Kr->outlet->smps[iter] *(*x->data_in[0] - *x->data_in[2] - y->k_lenr->outlet->smps[iter]) -
    // damping and left velocity
    y->k_cl->outlet->smps[iter] *(*x->data_out - *x->data_in[3]) -
    // damping and right velocity
    y->k_cr->outlet->smps[iter] *(*x->data_out - *x->data_in[4]) -
    // bow model;
    bow_contrib;

  out/=y->k_m->outlet->smps[iter];
  return out;
}

void tl_init_string_trans(tl_arglist *args){

  int i;
  tl_procession *procession; // needed for DAC  
  // check for a procession in the args
  // this should never actually happen:
  if(args->argv[0]->type!=TL_PROCESSION) 
    {
      printf("error: tl_init_dyfunc_test : first init argument needs to be a valid procession pointer\n");
      return;
    }
  else procession = args->argv[0]->procession;

  ctl_head = init_ctl(TL_HEAD_CTL);
  
  solver = tl_init_UDS_solver(0, 
			      MASS_CNT*2, // two for each 
			      1);


  string_pts = malloc(sizeof(tl_string_pt *) * (MASS_CNT));
  
  for(i=0;i<MASS_CNT; i++) {

    // initialize each point
    string_pts[i] = init_UDS_string_pt(i);

    // use the extra data field for ease of access to stuff in the node
    string_pts[i]->pos_node->extra_data = (void *)string_pts[i];
    string_pts[i]->vel_node->extra_data = (void *)string_pts[i];

    // create the processing stack
    tl_push_UDS_node(solver->UDS_net, string_pts[i]->vel_node);
    tl_push_UDS_node(solver->UDS_net, string_pts[i]->pos_node);

  }

  wall_const = 0; // end is always  0 change

  // left of left wall
  string_pts[0]->pos_node->data_in[0] = &wall_const; // 0

  // the left most vel dangles -- does nothing
  string_pts[0]->vel_node->data_in[0] =
    string_pts[0]->pos_node->data_out; // can move the bridges from equilibrium if wanted, but should be 0 and unchanging otherwise
  string_pts[0]->vel_node->data_in[1] = &wall_const; // 0
  string_pts[0]->vel_node->data_in[2] =
    string_pts[1]->pos_node->data_out;
  // velocity of next pt for damping
  string_pts[0]->vel_node->data_in[3] = &wall_const;
  string_pts[0]->vel_node->data_in[4] =
    string_pts[1]->vel_node->data_out;

  
  
  // right wall
  string_pts[MASS_CNT-1]->pos_node->data_in[0] =
    string_pts[MASS_CNT-1]->vel_node->data_out;
  
  string_pts[MASS_CNT-1]->vel_node->data_in[0] =
    string_pts[MASS_CNT-1]->pos_node->data_out;
  string_pts[MASS_CNT-1]->vel_node->data_in[1] =
    string_pts[MASS_CNT-2]->pos_node->data_out;
  string_pts[MASS_CNT-1]->vel_node->data_in[2] =
    &wall_const;
  string_pts[MASS_CNT-1]->vel_node->data_in[3] =
    string_pts[MASS_CNT-2]->vel_node->data_out;  
  string_pts[MASS_CNT-1]->vel_node->data_in[4] =
  &wall_const;  

  
  for(i=1;i<MASS_CNT-1; i++) {
  
    string_pts[i]->pos_node->data_in[0] =
      string_pts[i]->vel_node->data_out; // this vel

     string_pts[i]->vel_node->data_in[0] =
       string_pts[i]->pos_node->data_out; // this pos

     string_pts[i]->vel_node->data_in[1] =
       string_pts[i-1]->pos_node->data_out; // pos left
     
     string_pts[i]->vel_node->data_in[2] =
       string_pts[i+1]->pos_node->data_out; //  pos right

     string_pts[i]->vel_node->data_in[3] =
       string_pts[i-1]->vel_node->data_out; //  vel left

     string_pts[i]->vel_node->data_in[4] =
       string_pts[i+1]->vel_node->data_out; //  vel right
   
  }

  // we need a dac, so make one
  dac = tl_init_dac(procession, MASS_CNT*2, 1);

  for(i=0;i<MASS_CNT*2;i++)
    dac->inlets[i] = solver->outlets[i];

  // create and initialize the dispacment controls
  k_disp = malloc(sizeof(tl_ctl *) * MASS_CNT);
  char buf[50];
  for(i=0;i<MASS_CNT; i++) {
    k_disp[i] = init_ctl(TL_LIN_CTL);
    sprintf(buf, "k_disp_%d", i);
    k_disp[i]->name = name_new(buf);
    set_ctl_val(k_disp[i], 0);
    install_onto_ctl_list(ctl_head, k_disp[i]);
  }

  b_pluck = init_ctl(TL_BANG_CTL);
  b_pluck->name = name_new("pluck");
  b_pluck->bang_func = pluck;
  install_onto_ctl_list(ctl_head, b_pluck);

  k_bvel = init_ctl(TL_LIN_CTL);
  k_bvel->name = name_new("k_bvel");
  set_ctl_val(k_bvel, 0);
  install_onto_ctl_list(ctl_head, k_bvel);

  k_bforce = init_ctl(TL_LIN_CTL);
  k_bforce->name = name_new("k_bforce");
  set_ctl_val(k_bforce, 0);
  install_onto_ctl_list(ctl_head, k_bforce);

  k_bfric = init_ctl(TL_LIN_CTL);
  k_bfric->name = name_new("k_bfric");
  set_ctl_val(k_bfric, 0);
  install_onto_ctl_list(ctl_head, k_bfric);

  
}

static tl_string_pt *init_UDS_string_pt(int which){

  tl_string_pt *x = malloc(sizeof(tl_string_pt));
  char buf[50];
  int i;
  
  // we need to keep track of this
  x->which = which;

  // initialize the velocity and position nodes
  x->pos_node = tl_init_UDS_node(pos_dot, 1, 1);
  x->vel_node = tl_init_UDS_node(vel_dot, 4, 1);

  // initially at rest at equilibrium
  tl_reset_UDS_node(x->pos_node, 0.0);
  tl_reset_UDS_node(x->vel_node, 0.0);
  x->disp = 0;
  
  // initialize the controls
  x->k_m = init_ctl(TL_LIN_CTL);
  sprintf(buf, "k_m_%d", which);
  x->k_m->name = name_new(buf);
  set_ctl_val(x->k_m, 1);

  // initialize the controls
  x->k_Kl = init_ctl(TL_LIN_CTL);
  sprintf(buf, "k_Kl_%d", which);
  x->k_Kl->name = name_new(buf);
  set_ctl_val(x->k_Kl, 1);

  // initialize the controls
  x->k_Kr = init_ctl(TL_LIN_CTL);
  sprintf(buf, "k_Kr_%d", which);
  x->k_Kr->name = name_new(buf);
  set_ctl_val(x->k_Kr, 1);

  // initialize the controls
  x->k_cl = init_ctl(TL_LIN_CTL);
  sprintf(buf, "k_cl_%d", which);
  x->k_cl->name = name_new(buf);
  set_ctl_val(x->k_cl, 0);

  // initialize the controls
  x->k_cr = init_ctl(TL_LIN_CTL);
  sprintf(buf, "k_cr_%d", which);
  x->k_cr->name = name_new(buf);
  set_ctl_val(x->k_cr, 0);

  // initialize the controls
  x->k_lenl = init_ctl(TL_LIN_CTL);
  sprintf(buf, "k_lenl_%d", which);
  x->k_lenl->name = name_new(buf);
  set_ctl_val(x->k_lenl, 0);

  // initialize the controls
  x->k_lenr = init_ctl(TL_LIN_CTL);
  sprintf(buf, "k_lenr_%d", which);
  x->k_lenr->name = name_new(buf);
  set_ctl_val(x->k_lenr, 0);

  // initialize the controls
  x->k_barea = init_ctl(TL_LIN_CTL);
  sprintf(buf, "k_barea_%d", which);
  x->k_barea->name = name_new(buf);
  set_ctl_val(x->k_barea, 0);

  // install the controls correctly
  x->k_m->next = x->k_Kl;
  x->k_Kl->next = x->k_Kr;
  x->k_Kr->next = x->k_cl;
  x->k_cl->next = x->k_cr;
  x->k_cr->next = x->k_lenl;
  x->k_lenl->next = x->k_lenr;
  x->k_lenr->next = x->k_barea; 
  
  install_onto_ctl_list(ctl_head, x->k_m);

  x->pos_node->ctls = x->k_m;
  x->vel_node->ctls = x->k_m;
  
  return x;

}

tl_ctl *tl_reveal_ctls_string_trans(void){
  return ctl_head;
}

void tl_kill_string_trans(tl_class *class_ptr){

  int i;

  tl_kill_UDS_solver(solver); // kills the nodes automatically
  tl_kill_dac(dac);
  
  for(i=0;i<MASS_CNT;i++) {

    free(string_pts[i]);

  }
  free(string_pts);
  free(k_disp);  


}

void tl_dsp_string_trans(int samples, void *mod_ptr){

  tl_dsp_UDS_solver(samples, solver);
  tl_dsp_dac(samples, dac);

}

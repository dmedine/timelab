#include "m_mlf.h"
#include "stdio.h"
#include "stdlib.h"
#include "g_api.h"

t_mlf_class *mlf;

t_tl_sample diff(t_tl_sample alpha, t_tl_sample *data){

  t_tl_sample out;
  //printf("data[1] : %f\t data[2] %f\n", data[1], data[2]);
  out = alpha * tanh((data[1] - data[2]));
  return out;

}

void mlf_dsp_loop(int samples, void *class_ptr){
  
  int s;
  int i, j, k, step;
  int h_fact;
  t_tl_sample data[3];

  t_tl_sample res_h_fact, cut_h_fact;
  t_tl_sample res_step, cut_step;

  mlf = (t_mlf_class *)class_ptr;
  // printf("mlf->h_time : %f\t mlf->half_h_time : %f\n", mlf->h_time, mlf->half_h_time);
  s = samples * mlf->i_sigs[1]->h;
  h_fact = mlf->h/mlf->i_sigs[1]->h;

/*   res_h_fact = (t_tl_sample)mlf->resonance_sig->h/(t_tl_sample)mlf->h; */
/*   cut_h_fact = (t_tl_sample)mlf->cutoff_sig->h/(t_tl_sample)mlf->h; */

/*   for(i=0, res_step = 0.0, cut_step = 0.0; i<s; i++, res_step += res_h_fact, cut_step += cut_h_fact) */
  for(i=0; i<s; i++)
    {
      step = i*mlf->h;
      for(j=0; j<h_fact; j++)
	{
/* 	  mlf->resonance = mlf->resonance_sig->s_block[(int)res_step]; */
/* 	  mlf->cutoff = mlf->cutoff_sig->s_block[(int)cut_step] * PI;//optimize */

	  mlf->resonance = mlf->resonance_sig->s_block[i];
	  mlf->cutoff = mlf->cutoff_sig->s_block[i] * PI;//optimize
	 	  
	  data[1] = mlf->i_sigs[1]->s_block[i] - 
	    (mlf->resonance * mlf->state[3]);/*input minus resonance 
					       times last state*/
	
	  data[2] = mlf->state[0];//current state here
	  mlf->k[0][0] = diff(mlf->cutoff, data);
	  
	  data[1] = mlf->state[0];//state here
	  data[2] = mlf->state[1];//state of last stage
	  mlf->k[1][0] = diff(mlf->cutoff, data);
	  
	  data[1] = mlf->state[1];//state here
	  data[2] = mlf->state[2];//state of last stage
	  mlf->k[2][0] = diff(mlf->cutoff, data);
	  
	  data[1] = mlf->state[2];//state here
	  data[2] = mlf->state[3];//state of last stage
	  mlf->k[3][0] = diff(mlf->cutoff, data);
	  //for(i=0; i<4; i++)
	  //if(mlf->k[i][0]>1.0)mlf->k[i][0] = 10.0;
	  
      
	  //2.
	  //data[0] = mlf->signals[0]->u_item.s_blocmlf->k[i];
	  data[1] = mlf->i_sigs[1]->s_block[i] - 
	    (mlf->resonance * (mlf->state[3] + (mlf->half_h_time * mlf->k[3][0])));
	  //input - resonance * final state with runge-kutta estimation
	  data[2] = mlf->state[0] + (mlf->half_h_time * mlf->k[0][0]);
	  //this state with rk estimation
	  mlf->k[0][1] = diff(mlf->cutoff, data);
	  
	  data[1] = mlf->state[0] + (mlf->half_h_time * mlf->k[0][0]);
	  data[2] = mlf->state[1] + (mlf->half_h_time * mlf->k[1][0]);
	  mlf->k[1][1] = diff(mlf->cutoff, data);
	  
	  data[1] = mlf->state[1] + (mlf->half_h_time * mlf->k[1][0]);
	  data[2] = mlf->state[2] + (mlf->half_h_time * mlf->k[2][0]);
	  mlf->k[2][1] = diff(mlf->cutoff, data);
	  
	  data[1] = mlf->state[2] + (mlf->half_h_time * mlf->k[2][0]);
	  data[2] = mlf->state[3] + (mlf->half_h_time * mlf->k[3][0]);
	  mlf->k[3][1] = diff(mlf->cutoff, data);
	  //for(i=0; i<4; i++)
	  //if(mlf->k[i][1]>1.0)mlf->k[i][1] = 10.0;
	  
	  
	  //3.
	  //data[0] = mlf->signals[0]->s_block[i];
	  data[1] = mlf->i_sigs[1]->s_block[i] - 
	    (mlf->resonance * (mlf->state[3] + (mlf->half_h_time * mlf->k[3][1])));
	  //input - resonance * final state with runge-kutta estimation
	  data[2] = mlf->state[0] + (mlf->half_h_time * mlf->k[0][1]);
	  //this state with rk estimation
	  mlf->k[0][2] = diff(mlf->cutoff, data);
	  
	  data[1] = mlf->state[0] + (mlf->half_h_time * mlf->k[0][1]);
	  data[2] = mlf->state[1] + (mlf->half_h_time * mlf->k[1][1]);
	  mlf->k[1][2] = diff(mlf->cutoff, data);
	  
	  data[1] = mlf->state[1] + (mlf->half_h_time * mlf->k[1][1]);
	  data[2] = mlf->state[2] + (mlf->half_h_time * mlf->k[2][1]);
	  mlf->k[2][2] = diff(mlf->cutoff, data);
	  
	  data[1] = mlf->state[2] + (mlf->half_h_time * mlf->k[2][1]);
	  data[2] = mlf->state[3] + (mlf->half_h_time * mlf->k[3][1]);
	  mlf->k[3][2] = diff(mlf->cutoff, data);
	  //for(i=0; i<4; i++)
	  //if(mlf->k[i][2]>1.0)mlf->k[i][2] = 10.0;

	  //4.
	  //data[0] = mlf->signals[0]->s_block[i];
	  data[1] = mlf->i_sigs[1]->s_block[i] - 
	    (mlf->resonance * (mlf->state[3] + (mlf->h_time * mlf->k[3][2])));
	  //input - resonance * final state with runge-kutta estimation
	  data[2] = mlf->state[0] + (mlf->h_time * mlf->k[0][2]);
	  //this state with rk estimation
	  mlf->k[0][3] = diff(mlf->cutoff, data);
	  
	  data[1] = mlf->state[0] + (mlf->h_time * mlf->k[0][2]);
	  data[2] = mlf->state[1] + (mlf->h_time * mlf->k[1][2]);
	  mlf->k[1][3] = diff(mlf->cutoff, data);
	  
	  data[1] = mlf->state[1] + (mlf->h_time * mlf->k[1][2]);
	  data[2] = mlf->state[2] + (mlf->h_time * mlf->k[2][2]);
	  mlf->k[2][3] = diff(mlf->cutoff, data);
	  
	  data[1] = mlf->state[2] + (mlf->h_time * mlf->k[2][2]);
	  data[2] = mlf->state[3] + (mlf->h_time * mlf->k[3][2]);
x	  mlf->k[3][3] = diff(mlf->cutoff, data);
	  //for(i=0; i<4; i++)
	  //if(mlf->k[i][3]>1.0)mlf->k[i][3] = 10.0;
	  
	  
	  for(k=0; k<4; k++)
	    {
	      mlf->state[k] = mlf->state[k] + 
		(ONE_SIXTH *
		 mlf->h_time *
		 (mlf->k[k][0] + (2*mlf->k[k][1]) + (2*mlf->k[k][2]) + mlf->k[k][3]) *
		 mlf->scalar);
	      
	  // printf("%f ", mlf->state[i]);

	    }
      //      printf("\n");
      //the above printfs are for testing -- plug data into octave and graph it
      //we should see the characteristic decay//oscillation patterns

	  mlf->o_sigs[0]->s_block[step+j] = mlf->state[3];//sound out
	}
    }

}

t_mlf_class *mlf_init(int h){

  int i, j;
  
  t_mlf_class *x;

  x = malloc(sizeof(t_mlf_class));
  x->i_sig_count = 2;
  x->o_sig_count = 1;
  x->i_sigs = signals_new(x->i_sig_count, 1);
  x->o_sigs = signals_new(x->o_sig_count, 0);
  x->empty_sigs = signals_new(x->i_sig_count, 1);

  for(i=0; i<x->i_sig_count+2; i++)
    {
      x->empty_sigs[i] = 
	signal_setup(i, h, IT_SAMPLES);
      x->i_sigs[i] =x->empty_sigs[i]; 
    }



  x->o_sigs[0] = 
    signal_setup(0, h, IT_SAMPLES);
 
  x->resonance = 0.0;
  x->cutoff = 0.0;


  //controls as signals:
  // x->resonance_sig = signal_setup(1, h, IT_SAMPLES);//these might want to be at normal signal rate, not h*SR
  // x->cutoff_sig = signal_setup(1, h, IT_SAMPLES);
  x->resonance_sig = x->empty_sigs[2];
  x->cutoff_sig = x->empty_sigs[3];

  x->h = h;
  x->h_time = (1.0/(t_tl_sample)x->h) * (t_tl_sample)g_sample_time;
  x->half_h_time = .5*x->h_time;
  x->block_len = h * g_dsp_block_len;

  x->scalar = 1.0;
  x->gain = 1.0;


 /*  x->k = malloc(16 * sizeof(t_tl_sample)); */
/*   x->state = malloc(4 * sizeof(t_tl_sample)); */

  //TODO: add capability of initializing non-zero state conditions
  x->state[0] = x->state[1] = x->state[2] = x->state[3] = 0.0;
  for(i=0; i<4; i++)  
    {
      x->state[i];
      for(j=0; j<4; j++)
	x->k[i][j] = 0.0;
    }

  return x;


}

void kill_mlf_class(t_mlf_class *x){



  if(x->empty_sigs)
    kill_signals(x->empty_sigs, x->i_sig_count+2);
  if(x->o_sigs)
    kill_signals(x->o_sigs, x->o_sig_count);
/*   if(x->resonance_sig) */
/*     kill_one_signal(x->resonance_sig); */
/*   if(x->cutoff_sig) */
/*     kill_one_signal(x->cutoff_sig); */
  if(x->i_sigs)
    free(x->i_sigs);

  if(x)
    free(x);
}

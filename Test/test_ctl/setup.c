
#include "unistd.h"
#include "pthread.h"
#include "globals.h"


void *do_ctl(void *arg){
  printf("in do_ctl\n");

  while(done!=0)
    process_ctl_list(head);

}

void *this_bang(void *arg){

  printf("this bang got banged\n");
  printf("...and the ctl value is currently : %f\n", value);

}

int setup(void){

  int i;
  int thread_id;
  pthread_t thread;
  

  tl_lvl_stck *x = init_lvl_stck();
  set_g_lvl_stck(x);

  done = 1;

  head = init_ctl(TL_HEAD_CTL);
  counter = init_ctl(TL_LIN_CTL);
  banger = init_ctl(TL_BANG_CTL);

  install_onto_ctl_list(head, counter);
  install_onto_ctl_list(head, banger);

  // the above should be equivalent to:
  //head->next = counter;
  //counter->next = banger;
  //but install_onto_ctl_list produces segfault

  set_g_lvl_stck(init_lvl_stck());

  banger->bang_func = (tl_bang_func)&this_bang;
  set_ctl_kr(counter, &value);
  set_ctl_bang_go(banger, &do_bang);

  thread_id = pthread_create(&thread, NULL, &do_ctl, NULL);
  
  value = 0.0;
  do_bang = 0;
  /* for(i=0; i<5; i++) */
  /*   { */

  /*     value+=10.0; */
  /*     do_bang=1; */
  /*     printf("do_bang : %d\n", do_bang); */
  /*     usleep(65000); */
  /*   } */
  
  //done = 0;
  //pthread_join(thread, NULL);
  return 0;

}

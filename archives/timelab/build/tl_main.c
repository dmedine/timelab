#include <stdio.h>
#include <stdlib.h>
#include "tl_main.h"
#include "g_api.h"

int main(int argc, char **argv){

  init_all();
  g_start_timelab();
  if(g_batch_mode_flag !=1);
  else kill_all();

  return(0);

}

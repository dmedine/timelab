#ifndef __m_fft_h_

#include "g_api.h"

/*******windows*********/
typedef struct _win_func{

  int len;
  int type;
  t_sample *win;

}t_win_func;

t_win_func *gen_win_func(int len, int type);
void resize_win(int new_len, t_win_func *x);
void kill_win_func(t_win_func *x);




#define __m_fft_h_
#endif

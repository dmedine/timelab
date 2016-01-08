
#ifdef __cplusplus
extern "C" {
#endif
#ifndef globals_h_



#include "tl_core.h"


tl_ctl *ctl_l_freq, *ctl_r_freq, *ctl_l_amp, *ctl_r_amp;
tl_smp l_freq_val, r_freq_val, l_amp_val, r_amp_val;
int done;
int do_bang;




int setup(void);

#define globals_h_
#endif

#ifdef __cplusplus
}
#endif

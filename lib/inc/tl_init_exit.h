#ifndef tl_init_ext_h_

#ifdef __cplusplus
extern "C" {
#endif

#include "tl_core.h"
#include "a_portaudio.h"
#include "m_modules.h"

  // must be called in this order
  void tl_init(void);
  void tl_audio_init(void);
  void tl_exit(void);


#ifdef __cplusplus
}
#endif

#define tl_init_exit_h_
#endif

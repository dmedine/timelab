#ifndef __ui_main_h_

#include "gtk/gtk.h"
#include "pthread.h"
#include "g_api.h"

//main
void create_gui(void);


//scope
void scope_gui(void);
char *sig_menu_names;
char *sig_str_y, *sig_str_z;
t_tlsig **sig_ptr_list;
int sig_counter;

int g_no_scope;
int gl_scope_go(void);
void gl_scope_redraw(void);
int gl_thread_id;
pthread_t gl_thread;

//sl list for output filer
typedef struct _gui_filer_data{
  int cnt;
  GtkWidget *label;
  GtkWidget *menu_button;
  GtkWidget *close_button;
  GtkWidget *hbox;
  GtkWidget *menu;
  GtkWidget *menu_items;
  GtkWidget *sig_label;
  GtkWidget *apply_button;
  char *sig_str;
  struct _gui_filer_data *next;
  t_filer *filer;
}t_gui_filer_data;

t_gui_filer_data *tgfd_root;

t_gui_filer_data *init_gui_filer_data(char *filename);
void remove_gui_filer_data(t_gui_filer_data *x);
void kill_gui_filer_data(t_gui_filer_data *x);
void destroy_tgfd(void);




#define __ui_main_h_
#endif

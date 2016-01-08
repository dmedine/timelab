//hi, david

#include "ui_main.h"
#include "g_api.h"
#include "a_pa.h"
#include "stdlib.h"
#include "string.h"

int nin = 0;
int nout= 0;
int indevno, outdevno;
int outchans, inchans;
char *in_names, *out_names;
char *in_dev_str, *out_dev_str;
char in_num_string[10];
char out_num_string[10];
char out_chan_label_str[50];
char in_chan_label_str[50];

GtkWidget *tl_file_label;

GtkWidget *in_button, *out_button;
GtkWidget *in_dev_label, *out_dev_label;
GtkWidget *in_chan_label, *out_chan_label;
GtkWidget *inchan_entry, *outchan_entry;
int first_time = 1;

int sig_counter;
GtkWidget *sig_label_y, *sig_label_z;
GtkWidget *sig_button_y, *sig_button_z;
GtkWidget *sig_menu_y, *sig_menu_z;
GtkWidget *sig_menu_items_y, *sig_menu_items_z;
GtkWidget *global_box1, *global_box2, *global_box3;

GtkWidget *audio_check;
int sig_no_y, sig_no_z;
int sig_cnt;

int scope_on = 0;
t_filer *filer_list[50];
int fl_ptr = 0;



static void set_dev_strs(int in_no, int out_no);

static void inchannel_retrieve(void);
static void outchannel_retrieve(void);

static void flush_sig_menus(void);
static void cb_audio_on(GtkWidget *button);
static void cb_audio_off(GtkWidget *button);
static void do_sig_menu(void);


//--------------------------------
//filer data functions
t_gui_filer_data *init_gui_filer_data(char *filename){

  t_gui_filer_data *x = (t_gui_filer_data *)malloc(sizeof(t_gui_filer_data));
  x->cnt = -1;
  x->label = NULL;
  x->menu_button = NULL;
  x->close_button = NULL;
  x->hbox = NULL;
  x->menu = NULL;
  x->menu_items = NULL;
  x->next = NULL;
  x->sig_str = (char *)malloc(sizeof(char) * 256);
  if(filename!=NULL)
    {
      x->filer = init_filer(filename);
      x->filer->sig = g_empty_sig;
      install_filer(x->filer);
      filer_list[fl_ptr++] = x->filer;
    }

  return x;
}

void remove_gui_filer_data(t_gui_filer_data *x){

  t_gui_filer_data *y = tgfd_root;
  t_gui_filer_data *tmp;
  
  while(y->next!=x)
    y=y->next;

  tmp = x->next;
  y->next = tmp;
  kill_gui_filer_data(x);


}

void kill_gui_filer_data(t_gui_filer_data *x){

  if(x->label)gtk_widget_destroy(x->label);
  if(x->menu_button)gtk_widget_destroy(x->menu_button);
  if(x->close_button)gtk_widget_destroy(x->close_button);
  if(x->hbox)gtk_widget_destroy(x->hbox);
  if(x->menu)gtk_widget_destroy(x->menu);
  if(x->menu_items)gtk_widget_destroy(x->menu_items);
  if(x->sig_label)gtk_widget_destroy(x->sig_label);
  if(x->filer)remove_filer(x->filer);
  if(x->sig_str)
    {
      free(x->sig_str);
      x->sig_str = NULL;
    }
  if(x)
    {
      free(x);
      x=NULL;
    }

}

void destroy_tgfd(void){

  t_gui_filer_data *y = tgfd_root;
  t_gui_filer_data *tmp;
  while(y->next!=NULL)
    {
      tmp=y;
      y=y->next;
      kill_gui_filer_data(tmp);
    }

}


//--------------------------------
//main ctls gui stuff

static void tl_file_dialog(GtkWidget *button, GtkWidget *window){

     GtkWidget *dialog;

     //g_audio_off();
     
     dialog = gtk_file_chooser_dialog_new ("Choose tl File",
					   window,
					   GTK_FILE_CHOOSER_ACTION_OPEN,
					   GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
					   GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT,
					   NULL);
     
     if (gtk_dialog_run (GTK_DIALOG (dialog)) == GTK_RESPONSE_ACCEPT)
       {
         char *filename;
     
         filename = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (dialog));
	 load_tl_so(filename);
	 gtk_label_set_text((GtkLabel *)tl_file_label, g_tl_name);//filename w/out path
	 g_free (filename);
	 do_sig_menu();
       }
     
     gtk_widget_destroy (dialog);

 }

 static void apply_devs(GtkWidget *button, GtkWidget *window){


   pa_set_output_device(outdevno);
   pa_set_input_device(indevno);
   pa_set_output_channels(outchans);
   pa_set_input_channels(inchans);


   inchannel_retrieve();
   sprintf(in_num_string, "%d", inchans);
   sprintf(in_chan_label_str, "input channels : %s", in_num_string);
   gtk_label_set_text((GtkLabel *)in_chan_label, in_chan_label_str);
   //printf("gtk: inchans: %s\n", in_num_string);

   outchannel_retrieve();
   sprintf(out_num_string, "%d", outchans);
   sprintf(out_chan_label_str, "output channels : %s", out_num_string);
   gtk_label_set_text((GtkLabel *)out_chan_label, out_chan_label_str);

   gtk_label_set_text((GtkLabel *)in_dev_label, in_dev_str);
   gtk_button_set_label((GtkButton *)in_button, "-----Select Input Device-----");
   gtk_label_set_text((GtkLabel *)out_dev_label, out_dev_str);
   gtk_button_set_label((GtkButton *)out_button, "-----Select Output Device-----");

 }

 static void set_out_dev(GtkWidget *widget, int i){

   const PaDeviceInfo *pdi;
   pdi = Pa_GetDeviceInfo(i);
   outdevno = i;
   //printf("output device: %s\n", pdi->name);

   gtk_button_set_label((GtkButton *)out_button, pdi->name);
   set_dev_strs(indevno, outdevno);


 }

 static void set_in_dev(GtkWidget *widget, int i){

   const PaDeviceInfo *pdi;
   pdi = Pa_GetDeviceInfo(i);
   indevno = i;
   //printf("input device: %s\n", pdi->name);
   gtk_button_set_label((GtkButton *)in_button, pdi->name);
   set_dev_strs(indevno, outdevno);

 }

 static void outchannel_retrieve(void)
 {

   int i;
   const gchar *entry_text;
   entry_text = gtk_entry_get_text (GTK_ENTRY (outchan_entry));
   i = (atoi(entry_text));

   const PaDeviceInfo *pdi = Pa_GetDeviceInfo(outdevno);

   if(i<0)
     outchans = 0;
   else if(i>pdi->maxOutputChannels)
     outchans = pdi->maxOutputChannels;
   else
     outchans = i;

 }

 static void inchannel_retrieve(void)
 {

   int i;
   const gchar *entry_text;
   entry_text = gtk_entry_get_text (GTK_ENTRY (inchan_entry));

   i = (atoi(entry_text));

   const PaDeviceInfo *pdi = Pa_GetDeviceInfo(indevno);

   if(i<0)
     inchans = 0;
   else if(i>pdi->maxInputChannels)
     outchans = pdi->maxInputChannels;
   else
     inchans = i;

 }

 static void set_dev_strs(int in_no, int out_no){

  PaDeviceInfo *pdi = Pa_GetDeviceInfo(in_no);
  if(pdi!=NULL)
    {
      printf("%s\n", pdi->name);
      strcpy(in_dev_str, pdi->name);
      printf("in dv str %s\n", in_dev_str);
      pdi = Pa_GetDeviceInfo(out_no);
      strcpy(out_dev_str, pdi->name);
    }
  else printf("portaudio failure: devices not found!\n");
 }


 static void get_audio_devs(int count){

   int i = count;
   const PaDeviceInfo *pdi = Pa_GetDeviceInfo(i);

   if(pdi->maxInputChannels > 0)
     {
       sprintf(in_names + (nin*256), "(%d)%s", nin, pdi->name);
       printf("nin : %d, %s\n", nin, in_names + (nin *256));
       nin++;
     }

   if(pdi->maxOutputChannels > 0)
     {
       sprintf(out_names + (nout*256), "(%d)%s", nout, pdi->name);
       printf("nout : %d, %s\n", nout, out_names + (nout *256));
       nout++;
     }

   first_time = 0;
 }

 static gboolean device_menu_pressed( GtkWidget *widget,
				      GdkEvent *event ){
   
   if (event->type == GDK_BUTTON_PRESS) {
     GdkEventButton *bevent = (GdkEventButton *) event;
     gtk_menu_popup (GTK_MENU (widget), NULL, NULL, NULL, NULL,
		     bevent->button, bevent->time);
     
     return TRUE;
   }
   
   return FALSE;
 }

 //------------------------------------
 //------------------------------------
 //scope gui stuff

 static gboolean scope_menu_pressed( GtkWidget *widget,
				     GdkEvent *event ){
   
   if (event->type == GDK_BUTTON_PRESS) {
     GdkEventButton *bevent = (GdkEventButton *) event;
     gtk_menu_popup (GTK_MENU (widget), NULL, NULL, NULL, NULL,
		     bevent->button, bevent->time);
     
     return TRUE;
   }
   
   return FALSE;
 }

 static void get_sig_name(char *sig_names, t_obj_data *x){

   int i;
   //if debug
   //printf("menuizing %s %d with %d sigs\n", x->type_str, x->this_type_cnt, x->sig_cnt);

   for(i=0; i<x->sig_cnt; i++)
     {

       sprintf(sig_names + (sig_cnt * 256), "%s %d signal %d",
	       x->type_str, x->this_type_cnt-1, i);
       //printf("%d get_sig_names %s %d\n", i, sig_names, sig_cnt);
       sig_cnt++;
     }
 }


 static void scope_set_sig_y(GtkWidget *widget, int i){

   sig_no_y = i;
   sprintf(sig_str_y, "%s %d signal %d",
	   g_sig_reg[sig_no_y]->obj_ptr->type_str,
	   g_sig_reg[sig_no_y]->obj_ptr->this_type_cnt-1,
	   g_sig_reg[sig_no_y]->sig_no);
   
   gtk_button_set_label((GtkButton *)sig_button_y, sig_str_y);

 }

 static void scope_set_sig_z(GtkWidget *widget, int i){

   sig_no_z = i;
   sprintf(sig_str_z, "%s %d signal %d",
	   g_sig_reg[sig_no_z]->obj_ptr->type_str,
	   g_sig_reg[sig_no_z]->obj_ptr->this_type_cnt-1,
	   g_sig_reg[sig_no_z]->sig_no);
   
   gtk_button_set_label((GtkButton *)sig_button_z, sig_str_z);

 }

 static void flush_sig_names(char *sig_names){//do not call this if the string isn't 256*MAX_LEN long!!
   int i;
   for(i=0; i<MAX_LEN; i++)
     sig_names[i*256] = 0;
 }

 static void do_sig_menu(void){
   int i;
   //get the names of all the signals
   flush_sig_names(sig_menu_names);
   sig_cnt = 0;
   sig_menu_y = gtk_menu_new();
   sig_menu_z = gtk_menu_new();
   
   for(i=0; i<=g_obj_reg->ref_cnt; i++)
     get_sig_name(sig_menu_names, g_obj_reg->obj_arr[i]);
   
   
   //printf("%\n");
   /* for(i=0; i<g_sig_reg_cntr; i++) */
   /* printf("%s\n", sig_menu_names + (i*256)); */
   /* printf("\n\n------------\n\n"); */
   //pack the names into menus
   for(i=0; i<g_sig_reg_cntr; i++)
     {
       //printf("%d %d\t%s\n", i, g_sig_reg_cntr, (sig_menu_names + (i * 256)));
       sig_menu_items_y = gtk_menu_item_new_with_label(sig_menu_names + (i * 256));
       gtk_menu_shell_append(GTK_MENU_SHELL (sig_menu_y), sig_menu_items_y);
       g_signal_connect(sig_menu_items_y, "activate",
			G_CALLBACK(scope_set_sig_y),
			i);
       gtk_widget_show(sig_menu_items_y);
       
       sig_menu_items_z = gtk_menu_item_new_with_label(sig_menu_names + (i * 256));
       gtk_menu_shell_append(GTK_MENU_SHELL (sig_menu_z), sig_menu_items_z);
       g_signal_connect(sig_menu_items_z, "activate",
			G_CALLBACK(scope_set_sig_z),
			i);
       gtk_widget_show(sig_menu_items_z);
     }
   
   gtk_widget_hide(sig_button_y);
   sig_button_y = gtk_button_new_with_label("-----Select Signal to Scope on Y-----");
   g_signal_connect_swapped(sig_button_y, "event", G_CALLBACK(scope_menu_pressed), sig_menu_y);
   gtk_box_pack_start(GTK_BOX(global_box1), sig_button_y, FALSE, FALSE, 0);
   gtk_widget_show(sig_button_y);
   
   gtk_widget_hide(sig_button_z);
   sig_button_z = gtk_button_new_with_label("-----Select Signal to Scope on Z-----");
   g_signal_connect_swapped(sig_button_z, "event", G_CALLBACK(scope_menu_pressed), sig_menu_z);
   gtk_box_pack_start(GTK_BOX(global_box2), sig_button_z, FALSE, FALSE, 0);
   gtk_widget_show(sig_button_z);
   
 }


static void scope_apply_pressed(GtkWidget *button, GtkWidget *label){

  if(g_sig_reg[sig_no_y]!=NULL && g_sig_reg[sig_no_z]!=NULL)
    {
      gtk_label_set_text((GtkLabel *)sig_label_y, sig_str_y);
      gtk_label_set_text((GtkLabel *)sig_label_z, sig_str_z);
      g_circ_buff_scope_y->feeder = g_sig_reg[sig_no_y]->sig_ptr;
      g_circ_buff_scope_z->feeder = g_sig_reg[sig_no_z]->sig_ptr;
      gtk_button_set_label((GtkButton *)sig_button_y, "-----Select Signal to Scope on Y-----");
      gtk_button_set_label((GtkButton *)sig_button_z, "-----Select Signal to Scope on Z-----");
    }
  else
    {
      printf("Scope Error: can not connect signals to scope",sig_str_y);
    }

}

static void scope_launch_pressed(GtkWidget *button, GtkWidget *label){
 
  //needs to be on a new thread, I guess
  if(gl_scope_on_flag == 0)
    {
      gl_thread = pthread_create(&gl_thread, NULL,
				 gl_scope_go, NULL);
      gl_scope_on_flag == 1;
    }

}

//------------------------------------
//------------------------------------
//main gui stuff

static gboolean filer_menu_pressed( GtkWidget *widget,
				     GdkEvent *event ){
   
   if (event->type == GDK_BUTTON_PRESS) {
     GdkEventButton *bevent = (GdkEventButton *) event;
     gtk_menu_popup (GTK_MENU (widget), NULL, NULL, NULL, NULL,
		     bevent->button, bevent->time);
     
     return TRUE;
   }
   
   return FALSE;
 }

static void filer_close(GtkWidget *button, int ptr){

  t_gui_filer_data *y = tgfd_root;
  while(y->cnt!=ptr)
    y=y->next;
  //t_filer *x = filer_list[ptr];
  //printf("%d\n", x->ptr);
  //gtk_widget_hide(button);
  //printf("%s\n\n", x->name);
  remove_gui_filer_data(y);

}


static void set_filer_sig(GtkWidget *widget, t_gui_filer_data *x){

  int i;

  for(i=0; i<MAX_LEN; i++)
    if(strcmp(gtk_menu_item_get_label(widget), sig_menu_names + (i *256))==0)
      break;    
  
  printf("%d\n", i);
  x->filer->sig = g_sig_reg[i]->sig_ptr;
  printf("fp: %p, sig_ptr: %p\n", x->filer->fp, x->filer->sig);

  gtk_label_set_text(x->sig_label, sig_menu_names + (i * 256));
  /* sprintf(x->sig_str, "%s %d signal %d", */
  /* 	   g_sig_reg[i]->obj_ptr->type_str, */
  /* 	   g_sig_reg[i]->obj_ptr->this_type_cnt-1, */
  /* 	   g_sig_reg[i]->sig_no);  */

  /* gtk_button_set_label((GtkButton *)x->menu_button, x->sig_str); */
  //


}

static void do_file_sig_menu(t_gui_filer_data *x){

  if(x->menu)
    gtk_widget_destroy(x->menu);

  x->menu = gtk_menu_new();

  int i;
  for(i=0; i<g_sig_reg_cntr; i++)
    {
      //printf("%d %d\t%s\n", i, g_sig_reg_cntr, (sig_menu_names + (i * 256)));
      x->menu_items = gtk_menu_item_new_with_label(sig_menu_names + (i * 256));
      gtk_menu_shell_append(GTK_MENU_SHELL (x->menu), x->menu_items);
      g_signal_connect(x->menu_items, "activate",
      		       G_CALLBACK(set_filer_sig),
      		       x);
      gtk_widget_show(x->menu_items);
    }

}

static void new_output_file(GtkWidget *button, GtkWindow *window){

  GtkWidget *dialog;
  GtkWidget *menu_button, *close_button;
  GtkWidget *hbox;
  GtkWidget *label;

  t_filer *x;
  char *filename;
  int tmp_ptr = fl_ptr;

  //printf("%d %d\n", gfd.ptr, fl_ptr);
  
  t_gui_filer_data *y = tgfd_root;
  while(y->next!=NULL)
    y=y->next;//goto end of sl list

  dialog = gtk_file_chooser_dialog_new ("Select file to write to",
					window,
					GTK_FILE_CHOOSER_ACTION_SAVE,
					GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
					GTK_STOCK_SAVE, GTK_RESPONSE_ACCEPT,
					NULL);
  
  /* gtk_box_pack_start(box_top, dialog, FALSE, FALSE, 0); */
  /* gtk_widget_show(dialog); */

  if (gtk_dialog_run (GTK_DIALOG (dialog)) == GTK_RESPONSE_ACCEPT)
    {   
      filename = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (dialog));
      y->next = init_gui_filer_data(filename);
      /* x = init_filer(filename); */
      /* x->sig = g_empty_sig; */
      /* install_filer(x); */
      /* filer_list[fl_ptr++] = x; */

      
      y->next->cnt = tmp_ptr;
      
      y->next->label = gtk_label_new(y->next->filer->name);
      gtk_box_pack_start(GTK_BOX(global_box3), y->next->label, TRUE, FALSE, 0);
      gtk_widget_show(y->next->label);
      
      y->next->hbox = gtk_hbox_new(FALSE, 10);
      gtk_box_pack_start(GTK_BOX(global_box3), y->next->hbox, FALSE, FALSE, 0);
      gtk_widget_show(y->next->hbox);
      
      do_file_sig_menu(y->next);
      
      y->next->menu_button = gtk_button_new_with_label("select signal");
      g_signal_connect_swapped(y->next->menu_button, "event", G_CALLBACK(filer_menu_pressed), y->next->menu);
      gtk_box_pack_start(GTK_BOX(y->next->hbox), y->next->menu_button, FALSE, FALSE, 0);
      gtk_widget_show(y->next->menu_button);
      
      y->next->close_button = gtk_button_new_with_label("close");
      g_signal_connect(y->next->close_button, "clicked", G_CALLBACK(filer_close), tmp_ptr);
      gtk_box_pack_start(GTK_BOX(y->next->hbox), y->next->close_button, FALSE, FALSE, 0);
      gtk_widget_show(y->next->close_button);
      
      y->next->sig_label = gtk_label_new("---none selected---");
      gtk_box_pack_start(GTK_BOX(global_box3), y->next->sig_label, TRUE, FALSE, 0);
      gtk_widget_show(y->next->sig_label);

      g_free (filename);
    }

  gtk_widget_destroy (dialog);

}

static void cb_quit(GtkWidget *button){

  free(in_names);
  free(out_names);
  in_names = NULL;
  out_names = NULL;
  free(in_dev_str);
  free(out_dev_str);
  in_dev_str = NULL;
  out_dev_str = NULL;

  free(sig_menu_names);
  sig_menu_names = NULL;
  
  free(sig_str_y);
  sig_str_y = NULL;
  free(sig_str_z);
  sig_str_z = NULL;
  

  gtk_main_quit();
  kill_from_gui();


}

static void toggle_audio(GtkWidget *widget, gpointer window){

  if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget)))
    {
      g_audio_on();
    }
  else
    {
      g_audio_off();
    }
}

static void cb_audio_on(GtkWidget *button){
  g_audio_on();

}

static void cb_audio_off(GtkWidget *button){
  g_audio_off();

}

static void cb_tl_dialog(GtkWidget *button, GtkWindow *window){
 GtkWidget *dialog;
  
 dialog = gtk_file_chooser_dialog_new ("Select timelab file to run",
				       window,
				       GTK_FILE_CHOOSER_ACTION_OPEN,
				       GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
				       GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT,
				       NULL);
  
  /* gtk_box_pack_start(box_top, dialog, FALSE, FALSE, 0); */
  /* gtk_widget_show(dialog); */

  if (gtk_dialog_run (GTK_DIALOG (dialog)) == GTK_RESPONSE_ACCEPT)
    {
      char *filename;
      
      filename = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (dialog));
      //open_file (filename);
      g_free (filename);
    }
  
  gtk_widget_destroy (dialog);

}

static void toggle_step(GtkWidget *widget, GtkWindow *window){

  if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget)))
    {
      g_audio_off();
      gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(audio_check), FALSE);
      g_step_mode = 1;
    }
  else
    {
     g_step_mode = 0;
    }

}


static void do_step(GtkWidget *button){

  if(g_step_mode == 1)
    whole_step();

}


void create_gui(void){

  GtkWidget *window;
  GtkWidget *label;
  GtkWidget *box_top, *hbox_top1, *hbox_top2;
  GtkWidget *box1, *box2;
  GtkWidget *box3, *box4, *box5, *box6, *box7, *box8;
  GtkWidget *vbox1, *vbox2, *vbox3;
  GtkWidget *scale;
  GtkWidget *button;
  GtkWidget *separator;
  GtkWidget *label1, *label2;
  GtkWidget *frame;
  
  GtkWidget *step_check;

  GtkWidget *in_dev_menu, *out_dev_menu;
  GtkWidget *in_dev_menu_items, *out_dev_menu_items;
  GtkWidget *in_chan_menu_items, *out_chan_menu_items;


  GtkWidget *signal_entry;

  int i;
  int device_count;
  const PaDeviceInfo *pdi;

  char label_str[100];

  tgfd_root = init_gui_filer_data(NULL);

  //------------------------------------------------
  //for scope


  g_use_gui = 1;//??


  //------------------------------------------------
  //make the gui

  window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
  gtk_container_set_border_width (GTK_CONTAINER (window), 10);
  g_signal_connect(window, "destroy", G_CALLBACK(cb_quit), NULL);
  gtk_window_set_title(GTK_WINDOW(window), "timelab");

  box_top = gtk_vbox_new(FALSE, 10);
  gtk_container_add(GTK_CONTAINER(window), box_top);
  gtk_widget_show(box_top);

  hbox_top1 = gtk_hbox_new(FALSE, 10);
  gtk_box_pack_start(GTK_BOX(box_top), hbox_top1, TRUE, FALSE, 0);
  gtk_widget_show(hbox_top1);

  //------------------------------------------------
  //on/off/quit

  vbox1 = gtk_vbox_new(FALSE, 10);
  gtk_box_pack_start(GTK_BOX(hbox_top1), vbox1, TRUE, FALSE, 0);
  //gtk_container_add(GTK_CONTAINER(window), vbox1);
  gtk_widget_show(vbox1);

  label1 = gtk_label_new("Global Ctls");
  gtk_box_pack_start(GTK_BOX(vbox1), label1, FALSE, FALSE, 0);
  gtk_widget_show(label1);
  
  box1 = gtk_hbox_new(FALSE, 10);
  gtk_box_pack_start(GTK_BOX(vbox1), box1, FALSE, FALSE, 0);
  gtk_widget_show(box1);

  audio_check = gtk_check_button_new_with_label("Audio On");
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(audio_check), FALSE);
  gtk_box_pack_start(GTK_BOX(box1), audio_check, FALSE, FALSE, 0);
  g_signal_connect(audio_check, "clicked",
          G_CALLBACK(toggle_audio), (gpointer) window);
  gtk_widget_show(audio_check);

  button = gtk_button_new_with_label("Quit");
  g_signal_connect(button, "clicked", G_CALLBACK(cb_quit), NULL);
  gtk_box_pack_start(GTK_BOX(box1), button, FALSE, FALSE, 0);
  gtk_widget_show(button);

  box2 = gtk_hbox_new(FALSE, 10);
  gtk_box_pack_start(GTK_BOX(vbox1), box2, FALSE, FALSE, 0);
  gtk_widget_show(box2);

  tl_file_label = gtk_label_new(g_tl_name);
  gtk_box_pack_start(GTK_BOX(box2), tl_file_label, TRUE, FALSE, 0);
  gtk_widget_show(tl_file_label);

  button = gtk_button_new_with_label("Load tl file");
  g_signal_connect(button, "clicked", G_CALLBACK(tl_file_dialog), NULL);
  gtk_box_pack_start(GTK_BOX(box2), button, FALSE, FALSE, 0);
  gtk_widget_show(button);

  box3 = gtk_hbox_new(FALSE, 10);
  gtk_box_pack_start(GTK_BOX(vbox1), box3, FALSE, FALSE, 0);
  gtk_widget_show(box3);

  global_box3 = gtk_vbox_new(FALSE, 10);
  gtk_box_pack_start(GTK_BOX(box3), global_box3, FALSE, FALSE, 0);
  gtk_widget_show(global_box3);

  button = gtk_button_new_with_label("New Output File");
  g_signal_connect(button, "clicked", G_CALLBACK(new_output_file), (gpointer)window);
  gtk_box_pack_start(GTK_BOX(global_box3), button, FALSE, FALSE, 0);
  gtk_widget_show(button);

  box4 = gtk_hbox_new(FALSE, 10);
  gtk_box_pack_start(GTK_BOX(vbox1), box4, FALSE, FALSE, 0);
  gtk_widget_show(box4);


  step_check = gtk_check_button_new_with_label("Step Mode");
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(step_check), FALSE);
  gtk_box_pack_start(GTK_BOX(box4), step_check, FALSE, FALSE, 0);
  g_signal_connect(step_check, "clicked",
          G_CALLBACK(toggle_step), (gpointer) window);
  gtk_widget_show(step_check);

  button = gtk_button_new_with_label("Step");
  g_signal_connect(button, "clicked", G_CALLBACK(do_step), NULL);
  gtk_box_pack_start(GTK_BOX(box4), button, FALSE, FALSE, 0);
  gtk_widget_show(button);

  separator = gtk_vseparator_new();
  gtk_box_pack_start(GTK_BOX(hbox_top1), separator, FALSE, TRUE, 5);
  gtk_widget_show(separator);


  //-----------------------------------------------
  //audio devices

  //sort the names of the devices


  //------------------------------------------------
  //audio devices

  indevno = 0;
  outdevno = 0;//TODO: rc file this so we can save settings
  pdi = Pa_GetDeviceInfo(indevno);

  in_names = (char*)malloc(sizeof(char) * 256 * 100);
  out_names = (char*)malloc(sizeof(char) * 256 * 100);
 
  in_dev_str = (char*)malloc(sizeof(char) * 1000);
  out_dev_str = (char*)malloc(sizeof(char) * 1000);

  device_count = pa_device_count();
  set_dev_strs(indevno, outdevno);

  
  for(i=0;i<device_count;i++)
    get_audio_devs(i);

  
  //make the menu
  in_dev_menu = gtk_menu_new();
  out_dev_menu = gtk_menu_new();

  //pack them into the menus
  for(i=0;i<nout;i++)
    {
      out_dev_menu_items = gtk_menu_item_new_with_label(out_names + (i * 256));
      gtk_menu_shell_append(GTK_MENU_SHELL (out_dev_menu), out_dev_menu_items);
      g_signal_connect(out_dev_menu_items, "activate",
		       G_CALLBACK(set_out_dev),
		       i);
      gtk_widget_show(out_dev_menu_items);
      
    }
  
  for(i=0;i<nin;i++)
    {
      in_dev_menu_items = gtk_menu_item_new_with_label (in_names + (i * 256));
      gtk_menu_shell_append(GTK_MENU_SHELL (in_dev_menu), in_dev_menu_items);
      g_signal_connect(in_dev_menu_items, "activate",
		       G_CALLBACK(set_in_dev),
		       i);
      gtk_widget_show(in_dev_menu_items);
    }
  
  vbox2 = gtk_vbox_new(FALSE, 10);
  gtk_box_pack_start(GTK_BOX(hbox_top1), vbox2, TRUE, TRUE, 0);
  gtk_widget_show(vbox2);

  label2 = gtk_label_new("Audio Devices");
  gtk_box_pack_start(GTK_BOX(vbox2), label2, TRUE, TRUE, 0);
  gtk_widget_show(label2);

  pdi = Pa_GetDeviceInfo(indevno);

  //-----------------------------
  //input device
  //show device
  box4 = gtk_hbox_new(FALSE, 10);
  gtk_box_pack_start(GTK_BOX(vbox2), box4, TRUE, TRUE, 0);
  gtk_widget_show(box4);


  in_dev_label = gtk_label_new(in_dev_str);
  gtk_box_pack_start(GTK_BOX(box4), in_dev_label, TRUE, FALSE, 0);
  gtk_widget_show(in_dev_label);
  
  //button to select input device
  in_button = gtk_button_new_with_label("-----Select Input Device-----");
  g_signal_connect_swapped(in_button, "event", G_CALLBACK(device_menu_pressed), in_dev_menu);
  gtk_box_pack_start(GTK_BOX(box4), in_button, FALSE, FALSE, 0);
  gtk_widget_show(in_button);

  //--------------------------------
  //input channels
  //num_string = &((char)pdi->maxInputChannels);

  box5 = gtk_hbox_new(FALSE, 10);
  gtk_box_pack_start(GTK_BOX(vbox2), box5, TRUE, TRUE, 0);
  gtk_widget_show(box5);

  sprintf(in_num_string, "%d", g_inchannels);
  sprintf(in_chan_label_str, "input channels : %s", in_num_string);
  in_chan_label = gtk_label_new(in_chan_label_str);
  gtk_box_pack_start(GTK_BOX(box5), in_chan_label, TRUE, FALSE, 0);
  gtk_widget_show(in_chan_label);

  label = gtk_label_new("set channels:");
  gtk_box_pack_start(GTK_BOX(box5), label, FALSE, FALSE, 0);
  gtk_widget_show(label);

  inchan_entry = gtk_entry_new();
  gtk_entry_set_text((GtkEntry *)inchan_entry, in_num_string);
  gtk_entry_set_max_length (GTK_ENTRY (inchan_entry), 10);
  //g_signal_connect(inchan_entry, "activate", G_CALLBACK(inchannel_cb), inchan_entry);
  gtk_box_pack_start(GTK_BOX(box5), inchan_entry, FALSE, FALSE, 0);
  gtk_widget_show(inchan_entry);

  separator = gtk_hseparator_new();
  gtk_box_pack_start(GTK_BOX(vbox2), separator, FALSE, FALSE, 0);

  /****************************************/
  //output device
  
  pdi = Pa_GetDeviceInfo(outdevno);
  /* g_outchannels = pdi->maxOutputChannels; */
  /* printf("g_outchannels : %d (l_gui.c)\n"); */

  box6 = gtk_hbox_new(FALSE, 10);
  gtk_box_pack_start(GTK_BOX(vbox2), box6, TRUE, TRUE, 0);
  gtk_widget_show(box6);

  out_dev_label = gtk_label_new(out_dev_str);
  gtk_box_pack_start(GTK_BOX(box6), out_dev_label, TRUE, FALSE, 0);
  gtk_widget_show(out_dev_label);

  //out_button
  out_button = gtk_button_new_with_label("-----Select Output Device-----");
  g_signal_connect_swapped(out_button, "event", G_CALLBACK(device_menu_pressed), out_dev_menu);
  gtk_box_pack_start(GTK_BOX(box6), out_button, FALSE, FALSE, 0);
  gtk_widget_show(out_button);

  
  /*******************************************/
  //output channels

  box7 = gtk_hbox_new(FALSE, 0);
  gtk_box_pack_start(GTK_BOX(vbox2), box7, TRUE, TRUE, 0);
  gtk_widget_show(box7);

  sprintf(out_num_string, "%d", g_outchannels);
  sprintf(out_chan_label_str, "output channels : %s", out_num_string);
  out_chan_label = gtk_label_new(out_chan_label_str);
  gtk_box_pack_start(GTK_BOX(box7), out_chan_label, TRUE, FALSE, 0);
  gtk_widget_show(out_chan_label);

  label = gtk_label_new("set channels:");
  gtk_box_pack_start(GTK_BOX(box7), label, FALSE, FALSE, 0);
  gtk_widget_show(label);

  outchan_entry = gtk_entry_new();
  gtk_entry_set_text((GtkEntry *)outchan_entry, out_num_string);
  gtk_entry_set_max_length (GTK_ENTRY (outchan_entry), 1);
  //g_signal_connect(outchan_entry, "activate", G_CALLBACK(outchannel_cb), outchan_entry);
  gtk_box_pack_start(GTK_BOX(box7), outchan_entry, FALSE, FALSE, 0);
  gtk_widget_show(outchan_entry);
  

  box8 = gtk_hbox_new(FALSE, 10);
  gtk_box_pack_start(GTK_BOX(vbox2), box8, TRUE, TRUE, 0);
  gtk_widget_show(box8);

  button = gtk_button_new_with_label("Apply");
  g_signal_connect(button, "clicked", G_CALLBACK(apply_devs), window);
  gtk_box_pack_start(GTK_BOX(box8), button, TRUE, FALSE, 0);
  gtk_widget_show(button);



  //-------------------------------
  //scope
 
  sig_menu_names = (char *)malloc(sizeof(char) * 256 * MAX_LEN);
  sig_str_y = (char *)malloc(sizeof(char) * 256 * MAX_LEN);
  sig_str_z = (char *)malloc(sizeof(char) * 256 * MAX_LEN);
  
  // printf("ref_count in gui_land %d\n", g_obj_reg->ref_cnt);
  gl_scope_on_flag = 0;
  sig_counter=0;
  
  hbox_top2 = gtk_hbox_new(FALSE, 10);
  gtk_box_pack_start(GTK_BOX(box_top), hbox_top2, TRUE, FALSE, 0);
  gtk_widget_show(hbox_top2);

  vbox3 = gtk_vbox_new(FALSE, 10);
  gtk_box_pack_start(GTK_BOX(hbox_top2), vbox3, TRUE, FALSE, 0);
  gtk_widget_show(vbox3);


  separator = gtk_hseparator_new();
  gtk_box_pack_start(GTK_BOX(vbox3), separator, TRUE, TRUE, 5);
  gtk_widget_show(separator);

  label = gtk_label_new("Scope");
  gtk_box_pack_start(GTK_BOX(vbox3), label, FALSE, FALSE, 0);
  gtk_widget_show(label);


  sig_menu_y = gtk_menu_new();
  sig_menu_z = gtk_menu_new();
  do_sig_menu();

  global_box1 = gtk_hbox_new(FALSE, 10);
  gtk_box_pack_start(GTK_BOX(vbox3), global_box1, TRUE, TRUE, 0);
  gtk_widget_show(global_box1);

  sprintf(sig_str_y, "%s", "---scope signal y---");
  sig_label_y = gtk_label_new(sig_str_y);
  gtk_box_pack_start(GTK_BOX(global_box1), sig_label_y, TRUE, FALSE, 0);
  gtk_widget_show(sig_label_y);

  sig_button_y = gtk_button_new_with_label("-----Select Signal to Scope on Y-----");
  g_signal_connect_swapped(sig_button_y, "event", G_CALLBACK(scope_menu_pressed), sig_menu_y);
  gtk_box_pack_start(GTK_BOX(global_box1), sig_button_y, FALSE, FALSE, 0);
  gtk_widget_show(sig_button_y);

  //----------------------------

  global_box2 = gtk_hbox_new(FALSE, 10);
  gtk_box_pack_start(GTK_BOX(vbox3), global_box2, TRUE, TRUE, 0);
  gtk_widget_show(global_box2);

  sprintf(sig_str_z, "%s", "---scope signal z---");
  sig_label_z = gtk_label_new(sig_str_z);
  gtk_box_pack_start(GTK_BOX(global_box2), sig_label_z, TRUE, FALSE, 0);
  gtk_widget_show(sig_label_z);

  sig_button_z = gtk_button_new_with_label("-----Select Signal to Scope on Z-----");
  g_signal_connect_swapped(sig_button_z, "event", G_CALLBACK(scope_menu_pressed), sig_menu_z);
  gtk_box_pack_start(GTK_BOX(global_box2), sig_button_z, FALSE, FALSE, 0);
  gtk_widget_show(sig_button_z);

  //-----------------------------

  box3 = gtk_hbox_new(FALSE, 10);
  gtk_box_pack_start(GTK_BOX(vbox3), box3, TRUE, TRUE, 0);
  gtk_widget_show(box3);


  button = gtk_button_new_with_label("Apply");
  g_signal_connect(button, "clicked", G_CALLBACK(scope_apply_pressed), window);
  gtk_box_pack_start(GTK_BOX(box3), button, FALSE, FALSE, 0);
  gtk_widget_show(button);

  button = gtk_button_new_with_label("Launch Scope");
  g_signal_connect(button, "clicked", G_CALLBACK(scope_launch_pressed), window);
  gtk_box_pack_start(GTK_BOX(box3), button, FALSE, FALSE, 0);
  gtk_widget_show(button);

  separator = gtk_hseparator_new();
  gtk_box_pack_start(GTK_BOX(box3), separator, FALSE, TRUE, 5);
  gtk_widget_show(separator);

  gtk_widget_show(window);


}

void do_gui(void){
  

  create_gui();
  //g_main_context_iteration(NULL, FALSE);
  //gtk_widget_show(window);
  gtk_main();
}

void redo_gui(void){
  create_gui();
  gtk_main();


}

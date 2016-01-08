#include "ui_main.h"
#include "g_api.h"
#include "a_pa.h"
#include "stdlib.h"
#include "string.h"

int sig_counter;
GtkWidget *sig_label;
GtkWidget *sig_button;
int sig_no;

int scope_on = 0;

static gboolean menu_pressed( GtkWidget *widget,
				GdkEvent *event ){

  if (event->type == GDK_BUTTON_PRESS) {
    GdkEventButton *bevent = (GdkEventButton *) event; 
    gtk_menu_popup (GTK_MENU (widget), NULL, NULL, NULL, NULL,
		    bevent->button, bevent->time);
    /* Tell calling code that we have handled this event; the buck
     * stops here. */
    return TRUE;
  }
  
  /* Tell calling code that we have not handled this event; pass it on. */
  return FALSE;
}

static void scope_set_sig(GtkWidget *widget, int i){
  
  sig_no = i; 
  sprintf(sig_str, "%s %d signal %d", 
	  g_sig_reg[sig_no]->obj_ptr->type_str, 
	  g_sig_reg[sig_no]->obj_ptr->this_type_cnt-1, 
	  g_sig_reg[sig_no]->sig_no);
  
  gtk_button_set_label(sig_button, sig_str);

}

static void scope_apply_pressed(GtkWidget *button, GtkWidget *label){

  if(g_sig_reg[sig_no]!=NULL) 
    {
      gtk_label_set_text(sig_label, sig_str);
      g_circ_buff->feeder = g_sig_reg[sig_no]->sig_ptr;
      gtk_button_set_label(sig_button, "-----Select Signal to Scope-----");
    }
  else printf("Scope Error: can not connect scope to signal %s\n",sig_str);

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




void scope_gui(void){

  GtkWidget *window;
  GtkWidget *label;
  GtkWidget *frame;
  GtkWidget *box1, *box2, *box3, *box4, *box5, *box6;
  GtkWidget *button;
  GtkWidget *signals_menu;
  GtkWidget *signals_menu_items;
  GtkWidget *signal_entry;
  GtkWidget *separator;
  
  int sig_cnt = 0;
  int i, j;
  sig_no = 0;

  sprintf(sig_str, "%s %d signal %d", 
	  g_obj_reg->obj_arr[sig_no]->type_str,  g_obj_reg->obj_arr[sig_no]->this_type_cnt-1, sig_no);

  window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
  gtk_container_set_border_width (GTK_CONTAINER (window), 10);
  g_signal_connect(window, "destroy", (GtkSignalFunc) gtk_widget_destroy, GTK_OBJECT(window));
  gtk_window_set_title(GTK_WINDOW(window), "Scope");

  signals_menu = gtk_menu_new();

  for(i=0; i<sig_counter; i++)
    {
      //printf("in the ui_scope for loop %d of %d times\n", i, g_obj_reg->ref_cnt);
      signals_menu_items = gtk_menu_item_new_with_label(sig_menu_names + (i * 256));
      gtk_menu_shell_append(GTK_MENU_SHELL (signals_menu), signals_menu_items);
  	  g_signal_connect(signals_menu_items, "activate",
  			   G_CALLBACK(scope_set_sig),
  			   i);
      gtk_widget_show(signals_menu_items);
    }

  box1 = gtk_vbox_new(FALSE, 10);
  gtk_container_add(GTK_CONTAINER(window), box1);
  gtk_widget_show(box1);


  frame = gtk_frame_new("Scope signal:");
  sig_label = gtk_label_new("");
  gtk_container_add(GTK_CONTAINER (frame), sig_label);
  gtk_box_pack_start(GTK_BOX(box1), frame, TRUE, FALSE, 0);
  gtk_widget_show(frame);

  sig_button = gtk_button_new_with_label("-----Select Signal to Scope-----");
  g_signal_connect_swapped(sig_button, "event", G_CALLBACK(menu_pressed), signals_menu);
  gtk_box_pack_start(box1, sig_button, TRUE, FALSE, 0);
  gtk_widget_show(sig_button);


  box2 = gtk_hbox_new(FALSE, 10);
  gtk_container_add(GTK_CONTAINER(box1), box2);
  gtk_widget_show(box2);

  button = gtk_button_new_with_label("Apply");
  g_signal_connect(button, "clicked", G_CALLBACK(apply_pressed), window);
  gtk_box_pack_start(GTK_BOX(box2), button, TRUE, FALSE, 0);

  button = gtk_button_new_with_label("Launch Scope");
  g_signal_connect(button, "clicked", G_CALLBACK(launch_pressed), window);
  gtk_box_pack_start(GTK_BOX(box2), button, TRUE, FALSE, 0);
  
  separator = gtk_hseparator_new();
  gtk_box_pack_start(GTK_BOX(box1), separator, FALSE, FALSE, 0);
  gtk_widget_show(separator);
  

  gtk_widget_show_all(window);



}

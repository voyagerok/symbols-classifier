#ifndef GUI_H
#define GUI_H

#include <gtk/gtk.h>

extern const gchar *ui_path;

void on_activate(GtkApplication *app,
                 gpointer data);
void on_startup(GtkApplication *app,
                gpointer data);
void on_shutdown(GtkApplication *app,
                 gpointer data);

#endif // GUI_H

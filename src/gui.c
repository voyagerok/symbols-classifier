#include "gui.h"
#include <math.h>
#include "imgproc.h"
#include "constants.h"
#include "classes.h"
#include <string.h>
#include <stdlib.h>
#include "kozinets_learning.h"
#include "neuron.h"

#define MOUSE_DOWN 4
#define MOUSE_UP 8


static int mouse_flag = MOUSE_UP;
struct point
{
  int x,y;
} start_pos = {0,0};

struct rect
{
  int x,y;
  int width, height;
};

#define INIT_RECT(rec, imx, imy, imwidth, imheight)\
{\
  rec.x = imx;\
  rec.y = imy;\
  rec.width = imwidth;\
  rec.height = imheight;\
}

#define ROI_SET (roi_is_set = 1)
#define ROI_UNSET (roi_is_set = 0)
#define CLEAR_WHITE(pixbuf) (gdk_pixbuf_fill(pixbuf, 0xffffffff))


static GtkBuilder *builder;
static float angle = 0.0f;
static classes_container *classes;
//static neuron_model *neuron;
static neuron_model **neurons;

static void
tree_view_clear_records (GtkTreeView *t_view)
{
  GtkTreeStore *store;
  GtkTreeIter root;

  store = GTK_TREE_STORE(gtk_tree_view_get_model(t_view));
  gtk_tree_store_clear(store);
  gtk_tree_store_append(store, &root, NULL);
  gtk_tree_store_set(store, &root, 0, "Сгенерированные классы", -1);
}

static void
tree_view_append_record (GtkTreeView *t_view, char *name)
{
  GtkTreeStore *store;
  GtkTreeIter root;
  GtkTreeIter new_elem_iter;

  store = GTK_TREE_STORE(gtk_tree_view_get_model(t_view));
  gtk_tree_model_get_iter_from_string(GTK_TREE_MODEL(store), &root, "0");
  gtk_tree_store_append(store, &new_elem_iter, &root);
  gtk_tree_store_set(store, &new_elem_iter, 0, name, -1);
}

static void
show_message_box(GtkBuilder *builder,
                 const gchar *msg,
                 GtkMessageType type)
{
  GtkWidget *parent, *dialog;

  parent = GTK_WIDGET(gtk_builder_get_object(builder,
                                             "mainwindow"));
  dialog = gtk_message_dialog_new(GTK_WINDOW(parent),
                                  GTK_DIALOG_DESTROY_WITH_PARENT,
                                  type,
                                  GTK_BUTTONS_CLOSE,
                                  msg, NULL);
  gtk_dialog_run(GTK_DIALOG(dialog));
  gtk_widget_destroy(dialog);
}

static char*
get_class_name_from_user (GtkBuilder *builder)
{
  GtkWidget *window, *dialog, *label, *entry;
  GtkWidget *hbox, *content_area;
  GtkDialogFlags flags;
  int response, input_length;
  char *class_name;
  const char *input_text;

  window = GTK_WIDGET(gtk_builder_get_object(builder, "mainwindow"));
  dialog = gtk_dialog_new_with_buttons("Введите имя класса",
                                       GTK_WINDOW(window),
                                       GTK_DIALOG_MODAL | GTK_DIALOG_USE_HEADER_BAR,
                                       "Отмена", GTK_RESPONSE_CANCEL,
                                       "ОК", GTK_RESPONSE_ACCEPT,
                                       NULL);
  content_area = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
  entry = gtk_entry_new();
  label = gtk_label_new("Имя класса:");
  hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 15);
  gtk_box_pack_start(GTK_BOX(hbox), label, TRUE, TRUE, 15);
  gtk_box_pack_end(GTK_BOX(hbox), entry, TRUE, TRUE, 15);
  gtk_container_set_border_width(GTK_CONTAINER(hbox), 15);
  gtk_container_add(GTK_CONTAINER(content_area), hbox);

  gtk_widget_show_all(content_area);
  response = gtk_dialog_run(GTK_DIALOG(dialog));
  class_name = (char*)malloc(MAX_CLASS_NAME_LENGTH);
  if (response == GTK_RESPONSE_ACCEPT)
    {
      input_text = gtk_entry_get_text(GTK_ENTRY(entry));
      if ((input_length = strlen(input_text)) == 0)
        sprintf(class_name, "Класс%i", classes->n_of_classes);
      else
        memcpy(class_name, input_text, input_length + 1);
    }
  gtk_widget_destroy(dialog);

  return class_name;
}

static GtkWidget*
create_dialog_with_progress_bar (GtkBuilder *builder)
{
  GtkWidget *window, *dialog, *p_bar;
  GtkWidget *hbox, *content_area;
  GtkDialogFlags flags;
  int response, input_length;
  char *class_name;
  const char *input_text;

  window = GTK_WIDGET(gtk_builder_get_object(builder, "mainwindow"));
  dialog = gtk_dialog_new ();
  gtk_window_set_title (GTK_WINDOW(dialog), "Операция выполняется...");
  content_area = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
  p_bar = gtk_progress_bar_new();
  gtk_container_add (GTK_CONTAINER(content_area), p_bar);
  gtk_container_set_border_width (GTK_CONTAINER(content_area), 15);

  return dialog;
}

static GdkPixbuf *draw_line (GdkPixbuf *image,
                             struct point *start,
                             struct point *end)
{
  cairo_surface_t *surface;
  int s_width, s_height;
  GdkPixbuf *res;
  cairo_t *context;

  surface = gdk_cairo_surface_create_from_pixbuf(image, 0, NULL);
  context = cairo_create(surface);
  cairo_set_line_width (context, 1.0);
  cairo_set_line_cap(context, CAIRO_LINE_CAP_ROUND);
  cairo_set_source_rgb (context, 0, 0, 0);
  cairo_move_to(context, start->x, start->y);
  cairo_line_to(context, end->x, end->y);
  cairo_stroke(context);
  cairo_destroy(context);

  s_width = cairo_image_surface_get_width(surface);
  s_height = cairo_image_surface_get_height(surface);
  res = gdk_pixbuf_get_from_surface(surface,
                                    0, 0,
                                    s_width,
                                    s_height);
  cairo_surface_destroy(surface);
  return res;
}

static void redraw(GtkImage *image,
                 struct point *start,
                   struct point *end)
{
  GdkPixbuf *src_img, *res_img, *resized, *orig;
  struct point scaled_start, scaled_end;

  scaled_start.x = (start->x / WIDTH_STEP);
  scaled_start.y = (start->y / HEIGHT_STEP);
  scaled_end.x = (end->x / WIDTH_STEP);
  scaled_end.y = (end->y / HEIGHT_STEP);
  src_img = gtk_image_get_pixbuf(image);
  resized = resize (src_img, WIDTH_STEP, HEIGHT_STEP, LESSER, FALSE);
  res_img = draw_line(resized, &scaled_start, &scaled_end);
  orig = resize (res_img, WIDTH_STEP, HEIGHT_STEP, GREATER, TRUE);
  gtk_image_set_from_pixbuf(image, orig);

  g_object_unref(res_img);
  g_object_unref(resized);
  g_object_unref(orig);
}

static void
on_mouse_down(GtkWidget *widget,
              GdkEvent *event,
              gpointer data)
{
  GdkEventButton *btn_event;
  GtkAllocation alloc;
  GtkWidget  *image;
  GtkBuilder *builder;
  struct point center, img_top_left;
  GdkPixbuf *pbuf;
  int img_width, img_height;
  gboolean image_empty;

  btn_event = (GdkEventButton*)event;
  builder = GTK_BUILDER(data);
  image = GTK_WIDGET(gtk_builder_get_object(builder, "image"));
  image_empty = gtk_image_get_storage_type(GTK_IMAGE(image)) == GTK_IMAGE_EMPTY;
  if(btn_event->button == GDK_BUTTON_PRIMARY && !image_empty)
    {
      pbuf = gtk_image_get_pixbuf(GTK_IMAGE(image));

      mouse_flag = MOUSE_DOWN;
      gtk_widget_get_allocation(image, &alloc);
      center.x = alloc.width / 2;
      center.y = alloc.height / 2;
      img_width = gdk_pixbuf_get_width(pbuf);
      img_height = gdk_pixbuf_get_height(pbuf);
      img_top_left.x = center.x - img_width / 2;
      img_top_left.y = center.y - img_height / 2;

      start_pos.x = btn_event->x - img_top_left.x;
      start_pos.y = btn_event->y - img_top_left.y;
    }
}

static void
on_mouse_move(GtkWidget *widget,
              GdkEvent *event,
              gpointer data)
{
  GdkEventMotion *m_event;
  struct point current_pos;
  GtkBuilder *builder;
  GtkWidget *image;
  GdkPixbuf *pbuf;
  int img_width, img_height;
  struct point center, img_top_left;
  GtkAllocation alloc;

  m_event = (GdkEventMotion*)event;
  builder = GTK_BUILDER(data);
  image = GTK_WIDGET(gtk_builder_get_object(builder, "image"));
  if(mouse_flag == MOUSE_DOWN)
    {
      pbuf = gtk_image_get_pixbuf(GTK_IMAGE(image));

      gtk_widget_get_allocation(image, &alloc);
      center.x = alloc.width / 2 + alloc.x;
      center.y = alloc.height / 2 + alloc.y;
      img_width = gdk_pixbuf_get_width(pbuf);
      img_height = gdk_pixbuf_get_height(pbuf);
      img_top_left.x = center.x - img_width / 2;
      img_top_left.y = center.y - img_height / 2;

      current_pos.x = m_event->x - img_top_left.x;
      current_pos.y = m_event->y - img_top_left.y;

      current_pos.x  = current_pos.x < 0 ? 0 : current_pos.x;
      current_pos.y  = current_pos.y < 0 ? 0 : current_pos.y;

      redraw(GTK_IMAGE(image), &start_pos, &current_pos);

      start_pos = current_pos;
    }
}

static void
on_mouse_up(GtkWidget *widget,
            GdkEvent *event,
            gpointer data)
{
  GdkEventButton *b_event;

  b_event = (GdkEventButton*)event;
  if(b_event->button == GDK_BUTTON_PRIMARY)
      mouse_flag = MOUSE_UP;
}

static void
on_class_activated (GtkTreeView *t_view,
                    GtkTreePath *path,
                    GtkTreeViewColumn *column,
                    gpointer data)
{
  int path_depth, class_index;
  int *indeces;
  GtkBuilder *builder;
  GtkImage *image;
  GdkPixbuf *class_init_img;

  indeces = gtk_tree_path_get_indices_with_depth (path, &path_depth);
  if (path_depth == 2)
    {
      class_index = indeces[path_depth - 1];
      builder = GTK_BUILDER(data);
      image = GTK_IMAGE(gtk_builder_get_object(builder, "image"));
      class_init_img = classes->classes[class_index].init_image;
      gtk_image_set_from_pixbuf (image, class_init_img);
    }

}

static void
on_create_class (GtkButton *button, gpointer data)
{
  GtkBuilder *builder;
  GtkImage *image;
  GdkPixbuf *p_image;
  GtkTreeView *t_view;
  char *class_name;

  builder = GTK_BUILDER(data);
  image = GTK_IMAGE(gtk_builder_get_object(builder, "image"));
  t_view = GTK_TREE_VIEW(gtk_builder_get_object(builder, "treeview"));

  class_name = get_class_name_from_user (builder);

  p_image = gtk_image_get_pixbuf (image);
  add_class_to_container (classes, p_image, class_name);
  tree_view_append_record (t_view, class_name);
  gtk_tree_view_expand_all (t_view);

  free (class_name);
}

static void
on_angle_changed (GtkSpinButton *button,
                  gpointer user_data)
{
  GtkBuilder *builder;
  GtkImage *image;
  GdkPixbuf *pbuf, *rotated;
  float current_angle;

  builder = GTK_BUILDER(user_data);
  image = GTK_IMAGE(gtk_builder_get_object(builder, "image"));
  pbuf = gtk_image_get_pixbuf(image);
  current_angle = gtk_spin_button_get_value(button);
  rotated = rotate (pbuf, current_angle - angle);
  gtk_image_set_from_pixbuf(image, rotated);
  angle = current_angle;

  g_object_unref(rotated);
}

static void
put_noise (GtkButton *button,
                           gpointer data)
{
  GtkBuilder *builder;
  GtkImage *image;
  GdkPixbuf *pbuf, *modified_pbuf, *resized, *orig;

  builder = GTK_BUILDER(data);
  image = GTK_IMAGE(gtk_builder_get_object(builder, "image"));
  pbuf = gtk_image_get_pixbuf(image);
  resized = resize (pbuf, WIDTH_STEP, HEIGHT_STEP, LESSER, FALSE);
  modified_pbuf = get_modified_image(resized, FALSE);
  orig = resize (modified_pbuf, WIDTH_STEP, HEIGHT_STEP, GREATER, TRUE);
  gtk_image_set_from_pixbuf(image, orig);

  g_object_unref(modified_pbuf);
  g_object_unref(orig);
  g_object_unref(resized);
}

static void
on_clear (GtkButton *button,
                           gpointer data)
{
  GtkBuilder *builder;
  GtkImage *image;
  GdkPixbuf *pbuf, *clear_img;

  builder = GTK_BUILDER(data);
  image = GTK_IMAGE(gtk_builder_get_object(builder, "image"));
  pbuf = gtk_image_get_pixbuf(image);
  clear_img = gdk_pixbuf_copy(pbuf);
  CLEAR_WHITE(clear_img);
  gtk_image_set_from_pixbuf(image, clear_img);
  g_object_unref(clear_img);
}

static void
on_recognize (GtkButton *button,
                           gpointer data)
{
  GtkBuilder *builder;
  GtkImage *image;
  GdkPixbuf *p_image;
  int class_id, n_of_classes;
  char *info_string;

  builder = GTK_BUILDER(data);
  image = GTK_IMAGE(gtk_builder_get_object(builder, "image"));
  p_image = gtk_image_get_pixbuf (image);
  n_of_classes = classes->n_of_classes;

//  if (neurons[0]->vector == NULL)
//    show_message_box (builder, "Ошибка: система не обучена", GTK_MESSAGE_ERROR);
//  else
//    {
//      class_id = predict_result_for_2_classes (p_image, neurons[0]);
//      if (class_id == -1)
//        show_message_box (builder, "Ошибка: не удалось определить класс", GTK_MESSAGE_ERROR);
//      else
//        {
//          info_string = (char*)malloc(MAX_STRING);
//          sprintf (info_string, "Распознан класс %s", classes->classes[class_id].name);
//          show_message_box (builder, info_string, GTK_MESSAGE_INFO);
//          free (info_string);
//        }
//    }
  if (neurons[0]->vector == NULL)
    {
      show_message_box (builder, "Ошибка: система не обучена", GTK_MESSAGE_ERROR);
      return;
    }
  else if (n_of_classes < 2)
    {
      show_message_box (builder, "Ошибка: не стоило очищать классы", GTK_MESSAGE_ERROR);
      return;
    }
  else if (n_of_classes == 2)
    class_id = predict_result_for_2_classes (p_image, neurons[0]);
  else
    class_id = predict_result_for_multiple_classes (p_image, neurons, n_of_classes);

  if (class_id == -1)
    show_message_box (builder, "Ошибка: не удалось определить класс", GTK_MESSAGE_ERROR);
  else
    {
      info_string = (char*)malloc(MAX_STRING);
      sprintf (info_string, "Распознан класс %s", classes->classes[class_id].name);
      show_message_box (builder, info_string, GTK_MESSAGE_INFO);
      free (info_string);
    }
}

static void
on_clear_classes (GSimpleAction *action,
                           GVariant *variant,
                           gpointer data)
{
  GtkBuilder *builder;
  GtkTreeView *t_view;

  clear_container(classes);
  for (int i = 0; i < CLASSES_MAX_COUNT; ++i)
    clear_neuron (neurons[i]);
  //clear_neuron (neuron);
  builder = GTK_BUILDER(data);
  t_view = GTK_TREE_VIEW(gtk_builder_get_object(builder, "treeview"));
  tree_view_clear_records(t_view);
}

static void
on_check_linear_separability (GSimpleAction *action,
                           GVariant *variant,
                           gpointer data)
{
  gboolean res;
  GtkBuilder *builder;
  int n_of_classes;

  builder = GTK_BUILDER(data);
  n_of_classes = classes->n_of_classes;
  if (n_of_classes < 2)
    {
      show_message_box (builder, "Ошибка: должно быть как минимум 2 класса", GTK_MESSAGE_ERROR);
      return;
    }
  else if (n_of_classes == 2)
    res = check_linear_separability_for_2_classes (classes);
  else
    res = check_linear_separability_for_multiple_classes (classes);

  if (res == TRUE)
    show_message_box (builder, "Классы линейно разделимы", GTK_MESSAGE_INFO);
  else
    show_message_box (builder, "Линейную разделимость установить не удалось", GTK_MESSAGE_INFO);
}

static void
on_learning (GSimpleAction *action,
                           GVariant *variant,
                           gpointer data)
{
  GtkBuilder *builder;
  int n_of_classes;

  builder = GTK_BUILDER(data);
  n_of_classes = classes->n_of_classes;
  if (n_of_classes < 2)
    show_message_box (builder, "Ошибка: нужно как минимум 2 класса", GTK_MESSAGE_ERROR);
  else if (n_of_classes == 2)
    learning_for_2_classes (classes, neurons[0]);
  else
      learning_for_multiple_classes (classes, neurons);
}

static GActionEntry builder_entries[] =
{
  {"clear_classes", on_clear_classes, NULL, NULL, NULL},
  {"check_lin_sep", on_check_linear_separability, NULL, NULL, NULL},
  {"learning", on_learning, NULL, NULL, NULL}
};

static void
setup_button_signals(GtkBuilder *builder)
{
  GtkWidget *new_class_button, *clear_button;
  GtkWidget *noise_button, *recog_button;

  new_class_button = GTK_WIDGET(gtk_builder_get_object(builder, "create_class"));
  g_signal_connect (GTK_BUTTON(new_class_button), "clicked", G_CALLBACK(on_create_class), builder);

  clear_button = GTK_WIDGET(gtk_builder_get_object(builder, "clear_image"));
  g_signal_connect (GTK_BUTTON(clear_button), "clicked", G_CALLBACK(on_clear), builder);

  noise_button = GTK_WIDGET(gtk_builder_get_object(builder, "noise"));
  g_signal_connect (GTK_BUTTON(noise_button), "clicked", G_CALLBACK(put_noise), builder);

  recog_button = GTK_WIDGET(gtk_builder_get_object(builder, "recognize"));
  g_signal_connect (GTK_BUTTON(recog_button), "clicked", G_CALLBACK(on_recognize), builder);
}

static void
add_action_entries (GtkBuilder *builder)
{
  GtkApplicationWindow *window;
  GtkImage *image;

  image = GTK_IMAGE(gtk_builder_get_object(builder, "image"));
  window = GTK_APPLICATION_WINDOW(gtk_builder_get_object(builder,
                                                         "mainwindow"));
  g_action_map_add_action_entries(G_ACTION_MAP(window),
                                  builder_entries, G_N_ELEMENTS(builder_entries),
                                  builder);
}

static void
setup_image_widget(GtkBuilder *builder)
{
  GtkWidget *image, *eventbox;
  gint events, new_events;
  GdkPixbuf *blank_image;

  image = GTK_WIDGET(gtk_builder_get_object(builder,
                                            "image"));
  eventbox = GTK_WIDGET(gtk_builder_get_object(builder,
                                               "eventbox"));

  new_events = 0;
  events= gtk_widget_get_events(eventbox);
  if((events & GDK_BUTTON_PRESS_MASK) == 0)
    new_events |= GDK_BUTTON_PRESS_MASK;
  if((events & GDK_BUTTON_RELEASE_MASK) == 0)
    new_events |= GDK_BUTTON_RELEASE_MASK;
  if((events & GDK_POINTER_MOTION_MASK) == 0)
    new_events |= GDK_POINTER_MOTION_MASK;
  gtk_widget_add_events(eventbox, new_events);

  g_signal_connect(eventbox, "button-press-event",
                   G_CALLBACK(on_mouse_down), builder);
  g_signal_connect(eventbox, "button-release-event",
                   G_CALLBACK(on_mouse_up), NULL);
  g_signal_connect(eventbox, "motion-notify-event",
                   G_CALLBACK(on_mouse_move), builder);

  blank_image = gdk_pixbuf_new(GDK_COLORSPACE_RGB,
                               FALSE,
                               CHANNEL_DEPTH,
                               BLANK_IMG_WIDTH,
                               BLANK_IMG_HEIGHT);
  CLEAR_WHITE(blank_image);
  gtk_image_set_from_pixbuf(GTK_IMAGE(image), blank_image);
  g_object_unref(blank_image);
}

static void
setup_menu (GtkBuilder *builder)
{
  GtkWidget *menu_button;
  GMenu *menu;

  menu_button = GTK_WIDGET(gtk_builder_get_object(builder,
                                                  "menubutton"));
  menu = g_menu_new();
  g_menu_append(menu, "Обучить модель", "win.learning");
  g_menu_append(menu, "Линейная разделимость", "win.check_lin_sep");
  g_menu_append(menu, "Очистить классы", "win.clear_classes");
  gtk_menu_button_set_menu_model(GTK_MENU_BUTTON(menu_button),
                                 G_MENU_MODEL(menu));
}

static void
setup_angle_button (GtkBuilder *builder)
{
  GtkHeaderBar *hbar;
  GtkWidget *angle_button;

  hbar = GTK_HEADER_BAR(gtk_builder_get_object(builder, "headerbar"));
  angle_button = gtk_spin_button_new_with_range(MIN_ANGLE, MAX_ANGLE, ANGLE_STEP);
  gtk_widget_set_can_focus(angle_button, FALSE);
  gtk_spin_button_set_value(GTK_SPIN_BUTTON(angle_button), 0);
  g_signal_connect(GTK_SPIN_BUTTON(angle_button), "value-changed", G_CALLBACK(on_angle_changed), builder);

  gtk_header_bar_pack_end(hbar, angle_button);
}

static void
setup_tree_view (GtkBuilder *builder)
{
  GtkTreeIter iter;
  GtkTreeStore *treestore;
  GtkWidget *treeview;
  GtkCellRenderer *renderer;
  GtkTreeViewColumn *column;

  treeview = GTK_WIDGET(gtk_builder_get_object(builder, "treeview"));
  renderer = gtk_cell_renderer_text_new();
  column = gtk_tree_view_column_new_with_attributes(NULL, renderer, "text", 0, NULL);
  gtk_tree_view_append_column(GTK_TREE_VIEW(treeview), column);

  treestore = gtk_tree_store_new(1, G_TYPE_STRING);
  gtk_tree_store_append(treestore, &iter, NULL);
  gtk_tree_store_set(treestore, &iter, 0, "Сгенерированные классы", -1);
  gtk_tree_view_set_model(GTK_TREE_VIEW(treeview), GTK_TREE_MODEL(treestore));
  gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(treeview), FALSE);

  g_signal_connect (treeview, "row-activated", G_CALLBACK(on_class_activated), builder);
}

static void
init_neurons_array (neuron_model ***neurons)
{
  (*neurons) = (neuron_model**)malloc(sizeof(neuron_model*) * CLASSES_MAX_COUNT);
  for (int i = 0; i < CLASSES_MAX_COUNT; ++i)
    init_neuron(&(*neurons)[i]);
}

static void
free_neurons_array (neuron_model ***neurons)
{
  //(*neurons) = (neuron_model**)malloc(sizeof(neuron_model*) * CLASSES_MAX_COUNT);
  for (int i = 0; i < CLASSES_MAX_COUNT; ++i)
    free_neuron (&(*neurons)[i]);
  free ((*neurons));
}

void
on_startup(GtkApplication *app,
           gpointer data)
{
  builder = gtk_builder_new_from_file(ui_path);
  init_container(&classes);
  init_neurons_array (&neurons);


  add_action_entries(builder);
  setup_menu (builder);
//  setup_angle_button(builder);
  setup_button_signals(builder);
  setup_image_widget(builder);
  setup_tree_view(builder);
}

void
on_shutdown(GtkApplication *app,
            gpointer data)
{
  g_object_unref(builder);
  free_container(&classes);
  free_neurons_array (&neurons);
}

void
on_activate(GtkApplication *app,
            gpointer data)
{
  GtkWidget *window;

  window = GTK_WIDGET(gtk_builder_get_object(builder,
                                             "mainwindow"));
  gtk_application_add_window(app, GTK_WINDOW(window));
  gtk_widget_show_all(window);
}

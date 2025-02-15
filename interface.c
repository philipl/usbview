// SPDX-License-Identifier: GPL-2.0-only
/*
 * interface.c for USBView - a USB device viewer
 * Copyright (c) 1999, 2000, 2009 by Greg Kroah-Hartman, <greg@kroah.com>
 */
#ifdef HAVE_CONFIG_H
	#include <config.h>
#endif

#include <sys/stat.h>
#include <unistd.h>
#include <string.h>
#include <gtk/gtk.h>

#include "usbtree.h"
#include "hicolor/64x64/apps/usbview_icon.xpm"

GtkWidget *treeUSB;
GtkTreeStore *treeStore;
GtkTextBuffer *textDescriptionBuffer;
GtkWidget *textDescriptionView;
GtkWidget *windowMain;
static GtkTreeViewColumn *treeColumn;

int timer;

static gboolean
query_tooltip_cb(GtkWidget *widget, gint x, gint y,
			gboolean keyboard_tip, GtkTooltip *tooltip, void *unused)
{
	GtkTreeModel *model = gtk_tree_view_get_model(GTK_TREE_VIEW(treeUSB));
	GtkTreePath *path = NULL;
	GtkTreeIter iter;
	gint cellx = 0, celly = 0;
	gchar *color;
	gboolean return_val = FALSE;

	if (!model)
		return FALSE;

	/* This -30 is a kludge. It seems that if you don't do this, gtk_tree_view_get_path_at_pos()
	 * gives you the wrong path, the path for the cell that is 30 or so pixels down below the
	 * mouse. I don't know why it does that. Either we're doing something wrong, or it's a gtk-3
	 * bug.
	 */
	y = y - 30;

	if (!gtk_tree_view_get_path_at_pos(GTK_TREE_VIEW(widget), x, y, &path, &treeColumn, &cellx, &celly))
		return FALSE;
	if (!path)
		return FALSE;
	/* Get the color, if it's red, set tooltip to indicate device has no driver. */
	gtk_tree_model_get_iter(model, &iter, path);
	gtk_tree_model_get(model, &iter, COLOR_COLUMN, &color, -1);
	if (color && strncmp(color, "red", 4) == 0) {
		gtk_tooltip_set_text(tooltip, "This device has no attached driver");
		return_val = TRUE;
	}
	gtk_tree_path_free(path);
	return return_val;
}

GtkWidget*
create_windowMain ()
{
	GtkWidget *vbox1;
	GtkWidget *hpaned1;
	GtkWidget *scrolledwindow1;
	GtkWidget *hbuttonbox1;
	GtkWidget *buttonRefresh;
	GtkWidget *buttonClose;
	GtkWidget *buttonAbout;
	GdkPixbuf *icon;
	GtkCellRenderer *treeRenderer;

	windowMain = gtk_window_new (GTK_WINDOW_TOPLEVEL);
	gtk_widget_set_name (windowMain, "windowMain");
	gtk_window_set_title (GTK_WINDOW (windowMain), "USB Viewer");
	gtk_window_set_default_size (GTK_WINDOW (windowMain), 600, 300);

	icon = gdk_pixbuf_new_from_xpm_data((const char **)usbview_icon);
	gtk_window_set_icon(GTK_WINDOW(windowMain), icon);

	vbox1 = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
	gtk_widget_set_name (vbox1, "vbox1");
	gtk_widget_show (vbox1);
	gtk_container_add (GTK_CONTAINER (windowMain), vbox1);

	hpaned1 = gtk_paned_new (GTK_ORIENTATION_HORIZONTAL);
	gtk_widget_set_name (hpaned1, "hpaned1");
	gtk_widget_show (hpaned1);
	gtk_box_pack_start (GTK_BOX (vbox1), hpaned1, TRUE, TRUE, 0);

	treeStore = gtk_tree_store_new (N_COLUMNS,
				G_TYPE_STRING,	/* NAME_COLUMN */
				G_TYPE_INT,	/* DEVICE_ADDR_COLUMN */
				G_TYPE_STRING	/* COLOR_COLUMN */);
	treeUSB = gtk_tree_view_new_with_model (GTK_TREE_MODEL (treeStore));
	treeRenderer = gtk_cell_renderer_text_new ();
	treeColumn = gtk_tree_view_column_new_with_attributes (
					"USB devices",
					treeRenderer,
					"text", NAME_COLUMN,
					"foreground", COLOR_COLUMN,
					NULL);
	gtk_tree_view_append_column (GTK_TREE_VIEW (treeUSB), treeColumn);
	gtk_widget_set_name (treeUSB, "treeUSB");
	gtk_widget_show (treeUSB);
	gtk_paned_pack1 (GTK_PANED (hpaned1), treeUSB, FALSE, FALSE);

	scrolledwindow1 = gtk_scrolled_window_new (NULL, NULL);
	gtk_widget_set_name (scrolledwindow1, "scrolledwindow1");
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolledwindow1), GTK_POLICY_NEVER, GTK_POLICY_ALWAYS);
	gtk_widget_show (scrolledwindow1);
	gtk_paned_pack2 (GTK_PANED (hpaned1), scrolledwindow1, TRUE, FALSE);

	textDescriptionBuffer = gtk_text_buffer_new(NULL);
	//textDescription = gtk_text_new (NULL, NULL);
	textDescriptionView = gtk_text_view_new_with_buffer(textDescriptionBuffer);
	gtk_widget_set_name (textDescriptionView, "textDescription");
	gtk_text_view_set_editable(GTK_TEXT_VIEW(textDescriptionView), FALSE);
	gtk_text_view_set_cursor_visible(GTK_TEXT_VIEW(textDescriptionView), FALSE);
	gtk_widget_show (textDescriptionView);
	gtk_container_add (GTK_CONTAINER (scrolledwindow1), textDescriptionView);

	hbuttonbox1 = gtk_button_box_new (GTK_ORIENTATION_HORIZONTAL);
	gtk_widget_set_name (hbuttonbox1, "hbuttonbox1");
	gtk_widget_show (hbuttonbox1);
	gtk_box_pack_start (GTK_BOX (vbox1), hbuttonbox1, FALSE, FALSE, 5);
	//gtk_button_box_set_spacing (GTK_BUTTON_BOX (hbuttonbox1), 10);
	//gtk_button_box_set_child_size (GTK_BUTTON_BOX (hbuttonbox1), 50, 25);
	//gtk_button_box_set_child_ipadding (GTK_BUTTON_BOX (hbuttonbox1), 25, 10);

	buttonRefresh = gtk_button_new_with_label("Refresh");
	gtk_widget_set_name (buttonRefresh, "buttonRefresh");
	gtk_widget_show (buttonRefresh);
	gtk_container_add (GTK_CONTAINER (hbuttonbox1), buttonRefresh);
	gtk_container_set_border_width (GTK_CONTAINER (buttonRefresh), 4);
	gtk_widget_set_can_default (buttonRefresh, TRUE);

	buttonAbout = gtk_button_new_with_label("About");
	gtk_widget_set_name (buttonAbout, "buttonAbout");
	gtk_widget_show (buttonAbout);
	gtk_container_add (GTK_CONTAINER (hbuttonbox1), buttonAbout);
	gtk_container_set_border_width (GTK_CONTAINER (buttonAbout), 4);
	gtk_widget_set_can_default (buttonAbout, TRUE);

	buttonClose = gtk_button_new_with_label("Quit");
	gtk_widget_set_name (buttonClose, "buttonClose");
	gtk_widget_show (buttonClose);
	gtk_container_add (GTK_CONTAINER (hbuttonbox1), buttonClose);
	gtk_container_set_border_width (GTK_CONTAINER (buttonClose), 4);
	gtk_widget_set_can_default (buttonClose, TRUE);

	g_signal_connect (G_OBJECT (windowMain), "delete_event",
			    G_CALLBACK (on_window1_delete_event),
			    NULL);
	g_signal_connect (G_OBJECT (buttonRefresh), "clicked",
			    G_CALLBACK (on_buttonRefresh_clicked),
			    NULL);
	g_signal_connect (G_OBJECT (buttonAbout), "clicked",
			    G_CALLBACK (on_buttonAbout_clicked),
			    NULL);
	g_signal_connect (G_OBJECT (buttonClose), "clicked",
			    G_CALLBACK (on_buttonClose_clicked),
			    NULL);
	g_object_set(G_OBJECT (treeUSB), "has-tooltip", TRUE, NULL);
	g_signal_connect(G_OBJECT (treeUSB), "query-tooltip", G_CALLBACK (query_tooltip_cb), NULL);

	/* create our timer */
	//timer = gtk_timeout_add (2000, on_timer_timeout, 0);
	
	return windowMain;
}


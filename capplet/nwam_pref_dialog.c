/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/* vim:set expandtab ts=4 shiftwidth=4: */
/*
 * Copyright 2007 Sun Microsystems, Inc.  All rights reserved.
 * Use is subject to license terms.
 *
 * CDDL HEADER START
 * 
 * The contents of this file are subject to the terms of the
 * Common Development and Distribution License (the "License").
 * You may not use this file except in compliance with the License.
 * 
 * You can obtain a copy of the license at usr/src/OPENSOLARIS.LICENSE
 * or http://www.opensolaris.org/os/licensing.
 * See the License for the specific language governing permissions
 * and limitations under the License.
 * 
 * When distributing Covered Code, include this CDDL HEADER in each
 * file and include the License file at usr/src/OPENSOLARIS.LICENSE.
 * If applicable, add the following below this CDDL HEADER, with the
 * fields enclosed by brackets "[]" replaced with your own identifying
 * information: Portions Copyright [yyyy] [name of copyright owner]
 * 
 * CDDL HEADER END
 *
 * File:   nwam_wireless_dialog.c
 *
 * Created on May 9, 2007, 11:01 AM
 *
 */

#include <gtk/gtk.h>
#include <glade/glade.h>
#include <glib/gi18n.h>

#include <libnwamui.h>

#include "nwam_pref_iface.h"
#include "nwam_pref_dialog.h"
#include "nwam_conn_conf_ip_panel.h"
#include "nwam_conn_stat_panel.h"
#include "nwam_net_conf_panel.h"

/* Names of Widgets in Glade file */
#define CAPPLET_DIALOG_NAME		"nwam_capplet"
#define CAPPLET_DIALOG_SHOW_COMBO	"show_combo"
#define CAPPLET_DIALOG_MAIN_NOTEBOOK	"mainview_notebook"
#define CAPPLET_DIALOG_REFRESH_BUTTON	"refresh_button"

enum {
	PANEL_CONN_STATUS = 0,
	PANEL_NET_PREF,
	PANEL_CONF_IP,
	N_PANELS
};

struct _NwamCappletDialogPrivate {
	/* Widget Pointers */
	GtkDialog*                  capplet_dialog;
	GtkComboBox*                show_combo;
	GtkNotebook*                main_nb;
	
        /* Panel Objects */
	NwamPrefIFace* panel[N_PANELS];
	//NwamConnConfIPPanel*        ip_panel;
        //NwamConnstatusPanel*        status_panel;
                
        /* Other Data */
        NwamuiNcp*                  ncp; /* currently active NCP */
};

static void nwam_capplet_dialog_finalize(NwamCappletDialog *self);
static void nwam_pref_init (gpointer g_iface, gpointer iface_data);

/* Callbacks */
static void show_combo_cell_cb (GtkCellLayout *cell_layout,
				GtkCellRenderer   *renderer,
				GtkTreeModel      *model,
				GtkTreeIter       *iter,
				gpointer           data);
static gboolean show_combo_separator_cb (GtkTreeModel *model,
                                         GtkTreeIter *iter,
                                         gpointer data);
static void object_notify_cb( GObject *gobject, GParamSpec *arg1, gpointer data);
static void response_cb( GtkWidget* widget, gint repsonseid, gpointer data );
static void show_changed_cb( GtkWidget* widget, gpointer data );
static void refresh_clicked_cb( GtkButton *button, gpointer data );
static gboolean refresh (NwamPrefIFace *self, gpointer data);
static gboolean apply (NwamPrefIFace *self, gpointer data);

/* Utility Functions */
static void     update_show_combo_from_ncp( GtkComboBox* combo, NwamuiNcp*  ncp );
static void     change_show_combo_model(NwamCappletDialog *self);


G_DEFINE_TYPE_EXTENDED (NwamCappletDialog,
                        nwam_capplet_dialog,
                        G_TYPE_OBJECT,
                        0,
                        G_IMPLEMENT_INTERFACE (NWAM_TYPE_PREF_IFACE, nwam_pref_init))

static void
nwam_pref_init (gpointer g_iface, gpointer iface_data)
{
	NwamPrefInterface *iface = (NwamPrefInterface *)g_iface;
	iface->refresh = refresh;
	iface->apply = apply;
}

static void
nwam_capplet_dialog_class_init(NwamCappletDialogClass *klass)
{
	/* Pointer to GObject Part of Class */
	GObjectClass *gobject_class = (GObjectClass*) klass;
		
	/* Override Some Function Pointers */
	gobject_class->finalize = (void (*)(GObject*)) nwam_capplet_dialog_finalize;
}

static void
nwam_capplet_dialog_init(NwamCappletDialog *self)
{
    	GtkButton      *btn = NULL;
        NwamuiDaemon   *daemon = NULL;

	self->prv = g_new0(NwamCappletDialogPrivate, 1);
	
	/* Iniialise pointers to important widgets */
	self->prv->capplet_dialog = GTK_DIALOG(nwamui_util_glade_get_widget(CAPPLET_DIALOG_NAME));
	self->prv->show_combo = GTK_COMBO_BOX(nwamui_util_glade_get_widget(CAPPLET_DIALOG_SHOW_COMBO));
	self->prv->main_nb = GTK_NOTEBOOK(nwamui_util_glade_get_widget(CAPPLET_DIALOG_MAIN_NOTEBOOK));
	gtk_combo_box_set_row_separator_func (self->prv->show_combo,
					show_combo_separator_cb,
					NULL,
					NULL);
        
        /* Get Current NCP */
        daemon = nwamui_daemon_get_instance();
        self->prv->ncp = nwamui_daemon_get_active_ncp( daemon );
        g_object_unref( daemon );
        daemon = NULL;
        
        /* Construct the Notebook Panels Handling objects. */
        self->prv->panel[PANEL_CONN_STATUS] = NWAM_PREF_IFACE(nwam_conn_status_panel_new( self, self->prv->ncp ));
        self->prv->panel[PANEL_NET_PREF] = NWAM_PREF_IFACE(nwam_net_conf_panel_new( self, self->prv->ncp ));
        self->prv->panel[PANEL_CONF_IP] = NWAM_PREF_IFACE(nwam_conf_ip_panel_new());

        change_show_combo_model( self ); /* Change Model */
	update_show_combo_from_ncp( self->prv->show_combo, self->prv->ncp );
                
	gtk_combo_box_set_active (GTK_COMBO_BOX(self->prv->show_combo), 0);

        g_signal_connect(G_OBJECT(self), "notify", (GCallback)object_notify_cb, NULL);
	g_signal_connect(GTK_DIALOG(self->prv->capplet_dialog), "response", (GCallback)response_cb, (gpointer)self);
	g_signal_connect(GTK_COMBO_BOX(self->prv->show_combo), "changed", (GCallback)show_changed_cb, (gpointer)self);
	
	btn = GTK_BUTTON(nwamui_util_glade_get_widget(CAPPLET_DIALOG_REFRESH_BUTTON));
	g_signal_connect(btn, "clicked", (GCallback)refresh_clicked_cb, (gpointer)self);

        /* default select "Connection Status" */
}


/**
 * nwam_capplet_dialog_new:
 * @returns: a new #NwamCappletDialog.
 *
 * Creates a new #NwamCappletDialog with an empty ncu
 **/
NwamCappletDialog*
nwam_capplet_dialog_new(void)
{
	/* capplet dialog should be single */
	static NwamCappletDialog *capplet_dialog = NULL;
	if (capplet_dialog) {
		return NWAM_CAPPLET_DIALOG (g_object_ref (G_OBJECT (capplet_dialog)));
	}
	return capplet_dialog = NWAM_CAPPLET_DIALOG(g_object_new(NWAM_TYPE_CAPPLET_DIALOG, NULL));
}

/**
 * nwam_capplet_dialog_run:
 * @nwam_capplet_dialog: a #NwamCappletDialog.
 * @returns: a GTK_DIALOG Response ID
 *
 *
 * Blocks in a recursive main loop until the dialog either emits the response signal, or is destroyed.
 **/
gint
nwam_capplet_dialog_run(NwamCappletDialog  *self)
{
    gint response = GTK_RESPONSE_NONE;
    
    g_assert(NWAM_IS_CAPPLET_DIALOG(self));

    if ( self->prv->capplet_dialog != NULL ) {
            response = gtk_dialog_run(GTK_DIALOG(self->prv->capplet_dialog));
            
            gtk_widget_hide(GTK_WIDGET(self->prv->capplet_dialog));
    }
    return response;
}

static void
nwam_capplet_dialog_finalize(NwamCappletDialog *self)
{
	if ( self->prv->capplet_dialog != NULL )
		g_object_unref(G_OBJECT(self->prv->capplet_dialog));
	if ( self->prv->show_combo != NULL )
		g_object_unref(G_OBJECT(self->prv->show_combo));
	if ( self->prv->main_nb != NULL )
		g_object_unref(G_OBJECT(self->prv->main_nb));
	
	for (int i = 0; i < N_PANELS; i ++) {
		if (self->prv->panel[i]) {
			g_object_unref(G_OBJECT(self->prv->panel[i]));
		}
	}
	if ( self->prv->ncp != NULL )
		g_object_unref(G_OBJECT(self->prv->ncp));
	
	g_free(self->prv);
	self->prv = NULL;
	
	(*G_OBJECT_CLASS(nwam_capplet_dialog_parent_class)->finalize) (G_OBJECT(self));
}

/* Callbacks */

/**
 * refresh:
 *
 * Refresh #NwamCappletDialog with the new connections.
 * Including two static enties "Connection Status" and "Network Configuration"
 * And dynamic adding connection enties.
 * FIXED ME.
 **/
static gboolean
refresh (NwamPrefIFace *self, gpointer data)
{
	/*
	 * here we should refresh a specific panel, so we need a g_interface
	 * or a function pointer to invork
	 */
}

static gboolean
apply (NwamPrefIFace *self, gpointer data)
{
	
}

/*
 * According to nwam_pref_iface defined apply, we should do
 * foreach instance apply it, if error, poping up error dialog and keep living
 * otherwise hiding myself.
 */
static void
response_cb( GtkWidget* widget, gint responseid, gpointer data )
{
	NwamCappletDialog* self = NWAM_CAPPLET_DIALOG(data);
	
	switch (responseid) {
		case GTK_RESPONSE_NONE:
			g_debug("GTK_RESPONSE_NONE");
			break;
		case GTK_RESPONSE_DELETE_EVENT:
			g_debug("GTK_RESPONSE_DELETE_EVENT");
			break;
		case GTK_RESPONSE_OK:
			g_debug("GTK_RESPONSE_OK");
			/* FIXME, ok need call into separated panel/instance
			 * apply all changes, if no errors, hide all
			 */
			gtk_widget_hide_all (GTK_WIDGET(self->prv->capplet_dialog));
			break;
		case GTK_RESPONSE_CANCEL:
			g_debug("GTK_RESPONSE_CANCEL");
			gtk_widget_hide_all (GTK_WIDGET(self->prv->capplet_dialog));
			break;
		case GTK_RESPONSE_HELP:
			g_debug("GTK_RESPONSE_HELP");
			break;
	}
	g_signal_stop_emission_by_name(widget, "response" );
}

static void
show_changed_cb( GtkWidget* widget, gpointer data )
{
	gpointer user_data = NULL;
	NwamCappletDialog* self = NWAM_CAPPLET_DIALOG(data);
	gint idx = gtk_combo_box_get_active(GTK_COMBO_BOX(widget));
	
	/* update the notetab according to the selected entry */
	if (idx == PANEL_CONN_STATUS) {
		/* Connection Status */
		gtk_notebook_set_current_page(self->prv->main_nb, PANEL_CONN_STATUS);
	} else if (idx == PANEL_NET_PREF) {
		/* Network Configuration */
		gtk_notebook_set_current_page(self->prv->main_nb, PANEL_NET_PREF);
	} else {
        /* Wired/Wireless */

        NwamuiNcu*      current_ncu = NULL;
        GtkTreeModel   *model = NULL;
        GtkTreeIter     iter;

        idx = PANEL_CONF_IP;
        
        model = gtk_combo_box_get_model(GTK_COMBO_BOX(self->prv->show_combo));

        /* Get selected NCU and update IP Panel with this information */
        if ( gtk_combo_box_get_active_iter(GTK_COMBO_BOX(self->prv->show_combo), &iter ) ) {
            gtk_tree_model_get(GTK_TREE_MODEL(model), &iter, 0, &current_ncu, -1);

            nwam_conf_ip_panel_set_ncu(NWAM_CONN_CONF_IP_PANEL(self->prv->panel[PANEL_CONF_IP]), NWAMUI_NCU(current_ncu));
            g_object_unref( G_OBJECT(current_ncu) );

            gtk_notebook_set_current_page(self->prv->main_nb, 2);
        }
        else {
            g_debug("Couldn't get currently selected NCU!");
        }
    }
    
    nwam_pref_refresh (self->prv->panel[idx], user_data);
}

static gboolean
show_combo_separator_cb (GtkTreeModel *model,
                         GtkTreeIter *iter,
                         gpointer data)
{
	gpointer item = NULL;
	gtk_tree_model_get(model, iter, 0, &item, -1);
	return item == NULL;
}

static void
show_combo_cell_cb (GtkCellLayout *cell_layout,
			GtkCellRenderer   *renderer,
			GtkTreeModel      *model,
			GtkTreeIter       *iter,
			gpointer           data)
{
	gpointer row_data = NULL;
	gchar *text = NULL;
	
	gtk_tree_model_get(model, iter, 0, &row_data, -1);
	
	if (row_data == NULL) {
		return;
	}
        
	g_assert (G_IS_OBJECT (row_data));
	
	if (NWAMUI_IS_NCU (row_data)) {
		text = nwamui_ncu_get_display_name(NWAMUI_NCU(row_data));
		g_object_set (G_OBJECT(renderer), "text", text, NULL);
		g_free (text);
		return;
	}
	
	if (NWAM_IS_CONN_STATUS_PANEL (row_data)) {
		text = _("Connection Status");
	} 
	else if (NWAM_IS_NET_CONF_PANEL( row_data )) {
		text = _("Network Configuration");
	}
	else {
		g_assert_not_reached();
	}
	g_object_set (G_OBJECT(renderer), "text", text, NULL);
}

static void
object_notify_cb( GObject *gobject, GParamSpec *arg1, gpointer data)
{
	g_debug("NwamCappletDialog: notify %s changed\n", arg1->name);
}

/*
 * According to nwam_pref_iface defined refresh, we should do
 * foreach instance refresh it.
 */
static void
refresh_clicked_cb( GtkButton *button, gpointer data )
{
	/* FIXME, refresh need call into separated panel/instance */
	NwamCappletDialog* self = NWAM_CAPPLET_DIALOG(data);
	refresh (NWAM_PREF_IFACE(self), NULL);
}

/*
 * Utility functions.
 */
void
show_comob_add (GtkComboBox* combo, GObject*  obj)
{
	GtkTreeIter iter;
	GtkTreeModel *model = gtk_combo_box_get_model(combo);
	g_assert (model);
	
	gtk_tree_store_append (GTK_TREE_STORE(model), &iter, NULL);
	gtk_tree_store_set (GTK_TREE_STORE(model), &iter, 0, obj, -1);
}

/*
 * Change the combo box model to allow for the inclusion of a reference to the NCP to be stored.
 */
static void
change_show_combo_model(NwamCappletDialog *self)
{
	GtkCellRenderer *renderer;
	GtkTreeModel      *model;
	GtkCellRenderer   *pix_renderer;
        
        g_assert(NWAM_IS_CAPPLET_DIALOG(self));
        
	model = GTK_TREE_MODEL(gtk_tree_store_new(1, G_TYPE_OBJECT));    /* Object Pointer */
	
	gtk_combo_box_set_model(GTK_COMBO_BOX(self->prv->show_combo), model);
	gtk_cell_layout_clear(GTK_CELL_LAYOUT(self->prv->show_combo));
	renderer = gtk_cell_renderer_text_new();
	gtk_cell_layout_pack_start(GTK_CELL_LAYOUT(self->prv->show_combo), renderer, TRUE);
	gtk_cell_layout_set_cell_data_func(GTK_CELL_LAYOUT(self->prv->show_combo),
					renderer,
					show_combo_cell_cb,
					NULL,
					NULL);
	g_object_unref(model);
	
	show_comob_add (self->prv->show_combo, G_OBJECT(self->prv->panel[PANEL_CONN_STATUS]));
	show_comob_add (self->prv->show_combo, G_OBJECT(self->prv->panel[PANEL_NET_PREF])); 
	show_comob_add (self->prv->show_combo, NULL); /* Separator */
}


static void     
update_show_combo_from_ncp( GtkComboBox* combo, NwamuiNcp*  ncp )
{
	GtkTreeIter     iter;
	GtkTreeModel   *model = NULL;
	gboolean        has_next;
	GList          *ncu_list;
	
	g_return_if_fail( combo != NULL && ncp != NULL );
	
	/* Update list of connections in "show_combo" */
	model = gtk_combo_box_get_model(GTK_COMBO_BOX(combo));
	
	/*
	 TODO - Check if there is a better way to handle updating combo box than remove/add all items.
	 */
	/* Remove items 3 and later ( 0 = Connect Status; 1 = Network configuration; 2 = Separator ) */
	if (gtk_tree_model_get_iter_from_string(model, &iter, "3")) {
		do {
			has_next = gtk_list_store_remove(GTK_LIST_STORE(model), &iter);
		} while (has_next);
	}
	/* Now Add Entries for NCP Enabled NCUs */
	ncu_list = nwamui_ncp_get_ncu_list( NWAMUI_NCP(ncp) );
	
	for(GList* elem = g_list_first(ncu_list); elem != NULL; elem = g_list_next(elem)) {
		g_assert(G_IS_OBJECT(elem->data));
		gtk_tree_store_append(GTK_TREE_STORE(model), &iter, NULL);
		gtk_tree_store_set(GTK_TREE_STORE(model), &iter, 0, elem->data, -1);
	}
	nwamui_util_free_obj_list( ncu_list );
	
	g_object_unref(model);
}

/*
 * Select the given NCU in the show_combo box.
 */
extern void
nwam_capplet_dialog_select_ncu(NwamCappletDialog  *self, NwamuiNcu*  ncu )
{
    GtkTreeIter     iter;
    GtkTreeModel   *model = NULL;
    gboolean        has_next;

    model = gtk_combo_box_get_model(GTK_COMBO_BOX(self->prv->show_combo));
    
    for( has_next = gtk_tree_model_get_iter_first( GTK_TREE_MODEL( model ), &iter );
         has_next;
         has_next = gtk_tree_model_iter_next( GTK_TREE_MODEL( model ), &iter ) ) {
        NwamuiNcu*   current_ncu = NULL;

        gtk_tree_model_get(GTK_TREE_MODEL(model), &iter, 0, &current_ncu, -1);
        if ( current_ncu != NULL && current_ncu == ncu ) {
            gtk_combo_box_set_active_iter(GTK_COMBO_BOX(self->prv->show_combo), &iter );
            g_object_unref( current_ncu );
            break;
        }
        if ( current_ncu != NULL )
            g_object_unref( current_ncu );
    }
}

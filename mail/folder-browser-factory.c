/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * folder-browser-factory.c: A Bonobo Control factory for Folder Browsers
 *
 * Author:
 *   Miguel de Icaza (miguel@helixcode.com)
 *
 * (C) 2000 Helix Code, Inc.
 */
#include <config.h>
#include <gnome.h>
#include <bonobo/bonobo-main.h>
#include <bonobo/bonobo-object.h>
#include <bonobo/bonobo-generic-factory.h>
#include <bonobo/bonobo-control.h> 
#include "e-util/e-util.h"
#include "e-util/e-gui-utils.h"
#include "folder-browser.h"
#include "mail.h"
#include "shell/Evolution.h"

#ifdef USING_OAF
#define CONTROL_FACTORY_ID "OAFIID:control-factory:evolution-mail:25902062-543b-4f44-8702-d90145fcdbf2"
#else
#define CONTROL_FACTORY_ID "control-factory:evolution-mail"
#endif
			
static void
random_cb (GtkWidget *button, gpointer user_data)
{
	printf ("Yow! I am called back!\n");
}

static GnomeUIInfo gnome_toolbar [] = {
	GNOMEUIINFO_ITEM_STOCK (N_("Get mail"), N_("Check for new mail"), fetch_mail, GNOME_STOCK_PIXMAP_MAIL_RCV),
	GNOMEUIINFO_ITEM_STOCK (N_("Send"), N_("Send a new message"), send_msg, GNOME_STOCK_PIXMAP_MAIL_SND),
	GNOMEUIINFO_ITEM_STOCK (N_("Find"), N_("Find messages"), random_cb, GNOME_STOCK_PIXMAP_SEARCH),

	GNOMEUIINFO_SEPARATOR,

	GNOMEUIINFO_ITEM_STOCK (N_("Reply"), N_("Reply to the sender of this message"), reply_to_sender, GNOME_STOCK_PIXMAP_MAIL_RPL),
	GNOMEUIINFO_ITEM_STOCK (N_("Reply to All"), N_("Reply to all recipients of this message"), reply_to_all, GNOME_STOCK_PIXMAP_MAIL_RPL),

	GNOMEUIINFO_ITEM_STOCK (N_("Forward"), N_("Forward this message"), forward_msg, GNOME_STOCK_PIXMAP_MAIL_FWD),

	GNOMEUIINFO_SEPARATOR,

	GNOMEUIINFO_ITEM_STOCK (N_("Print"), N_("Print the selected message"), random_cb, GNOME_STOCK_PIXMAP_PRINT),

	GNOMEUIINFO_ITEM_STOCK (N_("Delete"), N_("Delete this message"), delete_msg, GNOME_STOCK_PIXMAP_TRASH),

	GNOMEUIINFO_END
};

static void
control_activate (BonoboControl *control, BonoboUIHandler *uih)
{
	Bonobo_UIHandler  remote_uih;
	BonoboControl *toolbar_control;
	GtkWidget *toolbar, *folder_browser;

	remote_uih = bonobo_control_get_remote_ui_handler (control);
	bonobo_ui_handler_set_container (uih, remote_uih);		

	folder_browser = bonobo_control_get_widget (control);

	bonobo_ui_handler_menu_new_item (uih, "/Tools/Expunge", N_("_Expunge"),
					 NULL, -1,
					 BONOBO_UI_HANDLER_PIXMAP_STOCK,
					 GNOME_STOCK_PIXMAP_TRASH,
					 0, 0, expunge_folder, folder_browser);

	bonobo_ui_handler_menu_new_item (uih, "/Tools/Filter Druid ...", N_("_Filter Druid"),
					 NULL, -1,
					 BONOBO_UI_HANDLER_PIXMAP_NONE,
					 0,
					 0, 0, filter_edit, folder_browser);

	toolbar = gtk_toolbar_new (GTK_ORIENTATION_HORIZONTAL,
				   GTK_TOOLBAR_BOTH);

	gnome_app_fill_toolbar_with_data (GTK_TOOLBAR (toolbar),
					  gnome_toolbar,
					  NULL, folder_browser);

	gtk_widget_show_all (toolbar);

	toolbar_control = bonobo_control_new (toolbar);
	bonobo_ui_handler_dock_add (uih, "/Toolbar",
				    bonobo_object_corba_objref (BONOBO_OBJECT (toolbar_control)),
				    GNOME_DOCK_ITEM_BEH_EXCLUSIVE,
				    GNOME_DOCK_TOP,
				    1, 1, 0);
}

static void
control_deactivate (BonoboControl *control, BonoboUIHandler *uih)
{
	bonobo_ui_handler_menu_remove (uih, "/File/Mail");
	bonobo_ui_handler_menu_remove (uih, "/Tools/Expunge");
	bonobo_ui_handler_menu_remove (uih, "/Tools/Filter Druid ...");
	bonobo_ui_handler_dock_remove (uih, "/Toolbar");
}

static void
control_activate_cb (BonoboControl *control, 
		     gboolean activate, 
		     gpointer user_data)
{
	BonoboUIHandler  *uih;

	uih = bonobo_control_get_ui_handler (control);
	g_assert (uih);
	
	if (activate)
		control_activate (control, uih);
	else
		control_deactivate (control, uih);
}

static void
control_destroy_cb (BonoboControl *control,
		    gpointer       user_data)
{
	GtkWidget *folder_browser = user_data;

	gtk_object_destroy (GTK_OBJECT (folder_browser));
}

/*
 * Creates the Folder Browser, wraps it in a Bonobo Control, and
 * sets the Bonobo Control properties to point to the Folder Browser
 * Properties
 */
static BonoboObject *
folder_browser_factory (BonoboGenericFactory *factory, void *closure)
{
	BonoboControl *control;
	GtkWidget *folder_browser;

	folder_browser = folder_browser_new ();
	if (folder_browser == NULL)
		return NULL;

	gtk_widget_show(folder_browser);
	
	control = bonobo_control_new (folder_browser);
	
	if (control == NULL){
		gtk_object_destroy (GTK_OBJECT (folder_browser));
		return NULL;
	}
	
	gtk_signal_connect (GTK_OBJECT (control), "activate",
			    control_activate_cb, NULL);

	gtk_signal_connect (GTK_OBJECT (control), "destroy",
			    control_destroy_cb, folder_browser);	
	
	bonobo_control_set_property_bag (control,
					 FOLDER_BROWSER (folder_browser)->properties);

	return BONOBO_OBJECT (control);
}

void
folder_browser_factory_init (void)
{
	static BonoboGenericFactory *bonobo_folder_browser_factory = NULL;
	
	if (bonobo_folder_browser_factory != NULL)
		return;

	bonobo_folder_browser_factory = bonobo_generic_factory_new (CONTROL_FACTORY_ID,
								    folder_browser_factory,
								    NULL);

	if (bonobo_folder_browser_factory == NULL){
		e_notice (NULL, GNOME_MESSAGE_BOX_ERROR,
			  _("We are sorry, Evolution's Folder Browser can not be initialized.")); 
		exit (1);
	}
}

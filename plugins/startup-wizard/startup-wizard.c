/*
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) version 3.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with the program; if not, see <http://www.gnu.org/licenses/>
 *
 *
 * Authors:
 *		JP Rosevear <jpr@novell.com>
 *
 * Copyright (C) 1999-2008 Novell, Inc. (www.novell.com)
 *
 */

#include <unistd.h>
#include <gconf/gconf-client.h>
#include <glib/gi18n.h>
#include <gtk/gtk.h>
#include "e-util/e-alert-dialog.h"
#include "e-util/e-import.h"
#include "shell/e-shell.h"
#include "shell/es-event.h"
#include "mail/em-config.h"
#include "mail/em-account-editor.h"
#include "calendar/gui/calendar-config.h"

#include "capplet/settings/mail-capplet-shell.h"

void startup_wizard (EPlugin *ep, ESEventTargetUpgrade *target);
GtkWidget *startup_wizard_importer_page (EPlugin *ep, EConfigHookItemFactoryData *hook_data);
gboolean startup_wizard_check (EPlugin *ep, EConfigHookPageCheckData *check_data);
void startup_wizard_commit (EPlugin *ep, EMConfigTargetAccount *target);
void startup_wizard_abort (EPlugin *ep, EMConfigTargetAccount *target) G_GNUC_NORETURN;

static EImport *import;
static EImportTargetHome *import_target;
static EImportImporter *import_importer;
static GtkWidget *import_dialog, *import_progress, *import_label;
static GSList *import_iterator, *import_importers;

G_GNUC_NORETURN static void
startup_wizard_terminate (void) {
	gtk_main_quit ();
	_exit (0);
}

static void
startup_wizard_close (void) {
	gtk_main_quit ();
}

void
startup_wizard (EPlugin *ep, ESEventTargetUpgrade *target)
{
	GtkWidget *start_page;
	GtkLabel  *start_page_label;
	GConfClient *client;
	GSList *accounts;
	EConfig *config;
	EMAccountEditor *emae;

	client = gconf_client_get_default ();
	accounts = gconf_client_get_list (client, "/apps/evolution/mail/accounts", GCONF_VALUE_STRING, NULL);
	g_object_unref (client);

	if (accounts != NULL) {
		g_slist_foreach (accounts, (GFunc) g_free, NULL);
		g_slist_free (accounts);

		return;
	}

	if (e_shell_get_express_mode (e_shell_get_default ())) {
		start_page = (GtkWidget *)mail_capplet_shell_new (0, TRUE, TRUE);
		gtk_widget_show (start_page);

		g_signal_connect (
			start_page, "delete-event",
			G_CALLBACK (startup_wizard_terminate), NULL);
		g_signal_connect (
			start_page, "destroy",
			G_CALLBACK (startup_wizard_close), NULL);

		gtk_main ();

		return;
	}

	/** @HookPoint-EMConfig: New Mail Account Wizard
	 * @Id: org.gnome.evolution.mail.config.accountWizard
	 * @Type: E_CONFIG_ASSISTANT
	 * @Class: org.gnome.evolution.mail.config:1.0
	 * @Target: EMConfigTargetAccount
	 *
	 * The new mail account assistant.
	 */
	emae = em_account_editor_new (
		NULL, EMAE_ASSISTANT,
		"org.gnome.evolution.mail.config.accountWizard");

	gtk_window_set_title (
		GTK_WINDOW (emae->editor), _("Evolution Setup Assistant"));

	config = (EConfig *) emae->config;
	start_page = e_config_page_get (config, "0.start");

	gtk_assistant_set_page_title (GTK_ASSISTANT (config->widget), start_page, _("Welcome"));
	start_page_label = GTK_LABEL (em_account_editor_get_widget (emae, "start_page_label"));
	if (start_page_label) {
		gtk_label_set_text (start_page_label, _(""
				    "Welcome to Evolution. The next few screens will allow Evolution to connect "
				    "to your email accounts, and to import files from other applications. \n"
				    "\n"
				    "Please click the \"Forward\" button to continue. "));
	}

	g_signal_connect (
		emae->editor, "delete-event",
		G_CALLBACK (startup_wizard_terminate), NULL);

	gtk_widget_show (emae->editor);

	gtk_main ();
}

GtkWidget *
startup_wizard_importer_page (EPlugin *ep, EConfigHookItemFactoryData *hook_data)
{
	GtkWidget *page, *label, *sep, *table;
	GSList *l;
	gint row=0;

	if (import == NULL) {
		import = e_import_new("org.gnome.evolution.shell.importer");
		import_target = e_import_target_new_home(import);
		import_importers = e_import_get_importers(import, (EImportTarget *)import_target);
	}

	if (import_importers == NULL)
		return NULL;

	page = gtk_vbox_new (FALSE, 0);
	gtk_container_set_border_width (GTK_CONTAINER (page), 12);

	label = gtk_label_new (_("Please select the information that you would like to import:"));
	gtk_box_pack_start (GTK_BOX (page), label, FALSE, FALSE, 3);

	sep = gtk_hseparator_new ();
	gtk_box_pack_start (GTK_BOX (page), sep, FALSE, FALSE, 3);

	table = gtk_table_new(g_slist_length(import_importers), 2, FALSE);
	for (l = import_importers; l; l = l->next) {
		EImportImporter *eii = l->data;
		gchar *str;
		/* *label already declared */
		GtkWidget *w, *label;

		w = e_import_get_widget(import, (EImportTarget *)import_target, eii);

		str = g_strdup_printf(_("From %s:"), eii->name);
		label = gtk_label_new(str);
		gtk_widget_show(label);
		g_free(str);

		gtk_misc_set_alignment((GtkMisc *)label, 0, .5);

		gtk_table_attach((GtkTable *)table, label, 0, 1, row, row+1, GTK_FILL, 0, 0, 0);
		if (w)
			gtk_table_attach((GtkTable *)table, w, 1, 2, row, row+1, GTK_FILL, 0, 3, 0);
		row++;
	}

	gtk_box_pack_start (GTK_BOX (page), table, FALSE, FALSE, 3);

	gtk_widget_show_all (page);
	gtk_assistant_append_page (GTK_ASSISTANT (hook_data->parent), page);
	gtk_assistant_set_page_title (GTK_ASSISTANT (hook_data->parent), page, _("Importing files"));

	return page;
}

static void
import_status(EImport *import, const gchar *what, gint pc, gpointer d)
{
	gtk_progress_bar_set_fraction((GtkProgressBar *)import_progress, (gfloat)(pc/100.0));
	gtk_progress_bar_set_text((GtkProgressBar *)import_progress, what);
}

static void
import_dialog_response(GtkDialog *d, guint button, gpointer data)
{
	if (button == GTK_RESPONSE_CANCEL)
		e_import_cancel(import, (EImportTarget *)import_target, import_importer);
}

static void
import_done(EImport *ei, gpointer d)
{
	if (import_iterator && (import_iterator = import_iterator->next)) {
		import_status(ei, "", 0, NULL);
		import_importer = import_iterator->data;
		e_import_import(import, (EImportTarget *)import_target, import_importer, import_status, import_done, NULL);
	} else {
		gtk_widget_destroy(import_dialog);

		g_slist_free(import_importers);
		import_importers = NULL;
		import_importer = NULL;
		e_import_target_free(import, (EImportTarget *)import_target);
		import_target = NULL;
		g_object_unref(import);
		import = NULL;

		gtk_main_quit();
	}
}

void
startup_wizard_commit (EPlugin *ep, EMConfigTargetAccount *target)
{
	EShell *shell;
	EShellSettings *shell_settings;
	gchar *location;

	shell = e_shell_get_default ();
	shell_settings = e_shell_get_shell_settings (shell);

	/* Use System Timezone by default */
	e_shell_settings_set_boolean (
		shell_settings, "cal-use-system-timezone", TRUE);
	location = e_cal_util_get_system_timezone_location ();
	e_shell_settings_set_string (
		shell_settings, "cal-timezone-string", location);
	g_free (location);

	if (import_importers) {
		import_iterator = import_importers;
		import_importer = import_iterator->data;

		import_dialog = e_alert_dialog_new_for_args (e_shell_get_active_window (shell), "shell:importing", _("Importing data."), NULL);
		g_signal_connect(import_dialog, "response", G_CALLBACK(import_dialog_response), NULL);
		import_label = gtk_label_new(_("Please wait"));
		import_progress = gtk_progress_bar_new();
		gtk_box_pack_start(GTK_BOX(((GtkDialog *)import_dialog)->vbox), import_label, FALSE, FALSE, 0);
		gtk_box_pack_start(GTK_BOX(((GtkDialog *)import_dialog)->vbox), import_progress, FALSE, FALSE, 0);
		gtk_widget_show_all(import_dialog);

		e_import_import(import, (EImportTarget *)import_target, import_importer, import_status, import_done, NULL);
	} else {
		gtk_main_quit();
	}
}

void
startup_wizard_abort (EPlugin *ep, EMConfigTargetAccount *target)
{
	startup_wizard_terminate ();
}
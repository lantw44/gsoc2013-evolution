/*
 * e-mail-config-defaults-page.c
 *
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
 */

#include "e-mail-config-defaults-page.h"

#include <config.h>
#include <glib/gi18n-lib.h>

#include <libebackend/libebackend.h>

#include <libemail-engine/e-mail-folder-utils.h>

#include <mail/e-mail-config-page.h>
#include <mail/em-folder-selection-button.h>

#define E_MAIL_CONFIG_DEFAULTS_PAGE_GET_PRIVATE(obj) \
	(G_TYPE_INSTANCE_GET_PRIVATE \
	((obj), E_TYPE_MAIL_CONFIG_DEFAULTS_PAGE, EMailConfigDefaultsPagePrivate))

struct _EMailConfigDefaultsPagePrivate {
	EMailSession *session;
	ESource *account_source;
	ESource *identity_source;

	GtkWidget *drafts_button;  /* not referenced */
	GtkWidget *sent_button;    /* not referenced */
	GtkWidget *replies_toggle; /* not referenced */
	GtkWidget *trash_toggle;   /* not referenced */
	GtkWidget *junk_toggle;    /* not referenced */
};

enum {
	PROP_0,
	PROP_ACCOUNT_SOURCE,
	PROP_IDENTITY_SOURCE,
	PROP_SESSION
};

/* Forward Declarations */
static void	e_mail_config_defaults_page_interface_init
					(EMailConfigPageInterface *interface);

G_DEFINE_TYPE_WITH_CODE (
	EMailConfigDefaultsPage,
	e_mail_config_defaults_page,
	GTK_TYPE_BOX,
	G_IMPLEMENT_INTERFACE (
		E_TYPE_EXTENSIBLE, NULL)
	G_IMPLEMENT_INTERFACE (
		E_TYPE_MAIL_CONFIG_PAGE,
		e_mail_config_defaults_page_interface_init))

static CamelSettings *
mail_config_defaults_page_maybe_get_settings (EMailConfigDefaultsPage *page)
{
	ESource *source;
	ESourceCamel *camel_ext;
	ESourceBackend *backend_ext;
	const gchar *backend_name;
	const gchar *extension_name;

	source = e_mail_config_defaults_page_get_account_source (page);

	extension_name = E_SOURCE_EXTENSION_MAIL_ACCOUNT;
	backend_ext = e_source_get_extension (source, extension_name);
	backend_name = e_source_backend_get_backend_name (backend_ext);
	extension_name = e_source_camel_get_extension_name (backend_name);

	/* Avoid accidentally creating a backend-specific extension
	 * in the mail account source if the mail account source is
	 * part of a collection, in which case the backend-specific
	 * extension is kept in the top-level collection source. */
	if (!e_source_has_extension (source, extension_name))
		return NULL;

	camel_ext = e_source_get_extension (source, extension_name);

	return e_source_camel_get_settings (camel_ext);
}

static CamelStore *
mail_config_defaults_page_ref_store (EMailConfigDefaultsPage *page)
{
	ESource *source;
	EMailSession *session;
	CamelService *service;
	const gchar *uid;

	session = e_mail_config_defaults_page_get_session (page);
	source = e_mail_config_defaults_page_get_account_source (page);

	uid = e_source_get_uid (source);
	service = camel_session_ref_service (CAMEL_SESSION (session), uid);

	if (service == NULL)
		return NULL;

	if (!CAMEL_IS_STORE (service)) {
		g_object_unref (service);
		return NULL;
	}

	return CAMEL_STORE (service);
}

static gboolean
mail_config_defaults_page_addrs_to_string (GBinding *binding,
                                           const GValue *source_value,
                                           GValue *target_value,
                                           gpointer unused)
{
	gchar **strv;

	strv = g_value_dup_boxed (source_value);

	if (strv != NULL) {
		gchar *string = g_strjoinv ("; ", strv);
		g_value_set_string (target_value, string);
		g_free (string);
	} else {
		g_value_set_string (target_value, "");
	}

	g_strfreev (strv);

	return TRUE;
}

static gboolean
mail_config_defaults_page_string_to_addrs (GBinding *binding,
                                           const GValue *source_value,
                                           GValue *target_value,
                                           gpointer unused)
{
	CamelInternetAddress *address;
	const gchar *string;
	gchar **strv;
	gint n_addresses, ii;

	string = g_value_get_string (source_value);

	address = camel_internet_address_new ();
	n_addresses = camel_address_decode (CAMEL_ADDRESS (address), string);

	if (n_addresses < 0) {
		g_object_unref (address);
		return FALSE;

	} else if (n_addresses == 0) {
		g_value_set_boxed (target_value, NULL);
		g_object_unref (address);
		return TRUE;
	}

	strv = g_new0 (gchar *, n_addresses + 1);

	for (ii = 0; ii < n_addresses; ii++) {
		const gchar *name = NULL;
		const gchar *addr = NULL;

		camel_internet_address_get (address, ii, &name, &addr);
		strv[ii] = camel_internet_address_format_address (name, addr);
	}

	g_value_take_boxed (target_value, strv);

	return TRUE;
}

static gboolean
mail_config_defaults_page_folder_name_to_uri (GBinding *binding,
                                              const GValue *source_value,
                                              GValue *target_value,
                                              gpointer data)
{
	EMailConfigDefaultsPage *page;
	CamelStore *store;
	const gchar *folder_name;
	gchar *folder_uri = NULL;

	page = E_MAIL_CONFIG_DEFAULTS_PAGE (data);
	store = mail_config_defaults_page_ref_store (page);
	g_return_val_if_fail (store != NULL, FALSE);

	folder_name = g_value_get_string (source_value);

	if (folder_name != NULL)
		folder_uri = e_mail_folder_uri_build (store, folder_name);

	g_value_set_string (target_value, folder_uri);

	g_free (folder_uri);

	g_object_unref (store);

	return TRUE;
}

static gboolean
mail_config_defaults_page_folder_uri_to_name (GBinding *binding,
                                              const GValue *source_value,
                                              GValue *target_value,
                                              gpointer data)
{
	EMailConfigDefaultsPage *page;
	EMailSession *session;
	const gchar *folder_uri;
	gchar *folder_name = NULL;
	GError *error = NULL;

	page = E_MAIL_CONFIG_DEFAULTS_PAGE (data);
	session = e_mail_config_defaults_page_get_session (page);

	folder_uri = g_value_get_string (source_value);

	if (folder_uri == NULL) {
		g_value_set_string (target_value, NULL);
		return TRUE;
	}

	e_mail_folder_uri_parse (
		CAMEL_SESSION (session), folder_uri,
		NULL, &folder_name, &error);

	if (error != NULL) {
		g_warn_if_fail (folder_name == NULL);
		g_warning ("%s: %s", G_STRFUNC, error->message);
		g_error_free (error);
		return FALSE;
	}

	g_return_val_if_fail (folder_name != NULL, FALSE);

	g_value_set_string (target_value, folder_name);

	g_free (folder_name);

	return TRUE;
}

static void
mail_config_defaults_page_restore_folders (EMailConfigDefaultsPage *page)
{
	EMFolderSelectionButton *button;
	EMailSession *session;
	EMailLocalFolder type;
	const gchar *folder_uri;

	session = e_mail_config_defaults_page_get_session (page);

	type = E_MAIL_LOCAL_FOLDER_DRAFTS;
	button = EM_FOLDER_SELECTION_BUTTON (page->priv->drafts_button);
	folder_uri = e_mail_session_get_local_folder_uri (session, type);
	em_folder_selection_button_set_folder_uri (button, folder_uri);

	if (gtk_widget_is_sensitive (page->priv->sent_button)) {
		type = E_MAIL_LOCAL_FOLDER_SENT;
		button = EM_FOLDER_SELECTION_BUTTON (page->priv->sent_button);
		folder_uri = e_mail_session_get_local_folder_uri (session, type);
		em_folder_selection_button_set_folder_uri (button, folder_uri);

		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (page->priv->replies_toggle), FALSE);
	}
}

static void
mail_config_defaults_page_restore_real_folder (GtkToggleButton *toggle_button)
{
	gtk_toggle_button_set_active (toggle_button, FALSE);
}

static GtkWidget *
mail_config_defaults_page_add_real_folder (EMailConfigDefaultsPage *page,
                                           GtkSizeGroup *size_group,
                                           GtkButton *revert_button,
                                           const gchar *toggle_label,
                                           const gchar *dialog_caption,
                                           const gchar *property_name,
                                           const gchar *use_property_name)
{
	GtkWidget *box;
	GtkWidget *check_button;
	GtkWidget *folder_button;
	EMailSession *session;
	CamelSettings *settings;
	CamelStore *store;
	GObjectClass *class;

	session = e_mail_config_defaults_page_get_session (page);
	settings = mail_config_defaults_page_maybe_get_settings (page);

	if (settings == NULL)
		return NULL;

	/* These folder settings are backend-specific, so check if
	 * the CamelSettings class has the property names we need. */

	class = G_OBJECT_GET_CLASS (settings);

	if (g_object_class_find_property (class, property_name) == NULL)
		return NULL;

	if (g_object_class_find_property (class, use_property_name) == NULL)
		return NULL;

	store = mail_config_defaults_page_ref_store (page);
	g_return_val_if_fail (store != NULL, NULL);

	/* We're good to go. */

	box = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 6);

	check_button = gtk_check_button_new_with_mnemonic (toggle_label);
	g_object_set (check_button, "xalign", 1.0, NULL);
	gtk_size_group_add_widget (size_group, check_button);
	gtk_box_pack_start (GTK_BOX (box), check_button, FALSE, FALSE, 0);
	gtk_widget_show (check_button);

	g_object_bind_property (
		settings, use_property_name,
		check_button, "active",
		G_BINDING_BIDIRECTIONAL |
		G_BINDING_SYNC_CREATE);

	folder_button = em_folder_selection_button_new (
		session, "", dialog_caption);
	em_folder_selection_button_set_store (
		EM_FOLDER_SELECTION_BUTTON (folder_button), store);
	gtk_box_pack_start (GTK_BOX (box), folder_button, TRUE, TRUE, 0);
	gtk_widget_show (folder_button);

	/* XXX CamelSettings only stores the folder's path name, but the
	 *     EMFolderSelectionButton requires a full folder URI, so we
	 *     have to do some fancy transforms for the binding to work. */
	g_object_bind_property_full (
		settings, property_name,
		folder_button, "folder-uri",
		G_BINDING_BIDIRECTIONAL |
		G_BINDING_SYNC_CREATE,
		mail_config_defaults_page_folder_name_to_uri,
		mail_config_defaults_page_folder_uri_to_name,
		g_object_ref (page),
		(GDestroyNotify) g_object_unref);

	g_object_bind_property (
		check_button, "active",
		folder_button, "sensitive",
		G_BINDING_SYNC_CREATE);

	g_signal_connect_swapped (
		revert_button, "clicked",
		G_CALLBACK (mail_config_defaults_page_restore_real_folder),
		check_button);

	g_object_unref (store);

	return box;
}

static void
mail_config_defaults_page_set_account_source (EMailConfigDefaultsPage *page,
                                              ESource *account_source)
{
	g_return_if_fail (E_IS_SOURCE (account_source));
	g_return_if_fail (page->priv->account_source == NULL);

	page->priv->account_source = g_object_ref (account_source);
}

static void
mail_config_defaults_page_set_identity_source (EMailConfigDefaultsPage *page,
                                               ESource *identity_source)
{
	g_return_if_fail (E_IS_SOURCE (identity_source));
	g_return_if_fail (page->priv->identity_source == NULL);

	page->priv->identity_source = g_object_ref (identity_source);
}

static void
mail_config_defaults_page_set_session (EMailConfigDefaultsPage *page,
                                       EMailSession *session)
{
	g_return_if_fail (E_IS_MAIL_SESSION (session));
	g_return_if_fail (page->priv->session == NULL);

	page->priv->session = g_object_ref (session);
}

static void
mail_config_defaults_page_set_property (GObject *object,
                                        guint property_id,
                                        const GValue *value,
                                        GParamSpec *pspec)
{
	switch (property_id) {
		case PROP_ACCOUNT_SOURCE:
			mail_config_defaults_page_set_account_source (
				E_MAIL_CONFIG_DEFAULTS_PAGE (object),
				g_value_get_object (value));
			return;

		case PROP_IDENTITY_SOURCE:
			mail_config_defaults_page_set_identity_source (
				E_MAIL_CONFIG_DEFAULTS_PAGE (object),
				g_value_get_object (value));
			return;

		case PROP_SESSION:
			mail_config_defaults_page_set_session (
				E_MAIL_CONFIG_DEFAULTS_PAGE (object),
				g_value_get_object (value));
			return;
	}

	G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
}

static void
mail_config_defaults_page_get_property (GObject *object,
                                        guint property_id,
                                        GValue *value,
                                        GParamSpec *pspec)
{
	switch (property_id) {
		case PROP_ACCOUNT_SOURCE:
			g_value_set_object (
				value,
				e_mail_config_defaults_page_get_account_source (
				E_MAIL_CONFIG_DEFAULTS_PAGE (object)));
			return;

		case PROP_IDENTITY_SOURCE:
			g_value_set_object (
				value,
				e_mail_config_defaults_page_get_identity_source (
				E_MAIL_CONFIG_DEFAULTS_PAGE (object)));
			return;

		case PROP_SESSION:
			g_value_set_object (
				value,
				e_mail_config_defaults_page_get_session (
				E_MAIL_CONFIG_DEFAULTS_PAGE (object)));
			return;
	}

	G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
}

static void
mail_config_defaults_page_dispose (GObject *object)
{
	EMailConfigDefaultsPagePrivate *priv;

	priv = E_MAIL_CONFIG_DEFAULTS_PAGE_GET_PRIVATE (object);

	if (priv->identity_source != NULL) {
		g_object_unref (priv->identity_source);
		priv->identity_source = NULL;
	}

	if (priv->session != NULL) {
		g_object_unref (priv->session);
		priv->session = NULL;
	}

	/* Chain up to parent's dispose() method. */
	G_OBJECT_CLASS (e_mail_config_defaults_page_parent_class)->
		dispose (object);
}

static void
mail_config_defaults_page_constructed (GObject *object)
{
	EMailConfigDefaultsPage *page;
	EMailSession *session;
	ESource *source;
	ESourceBackend *account_ext;
	ESourceMailComposition *composition_ext;
	ESourceMailSubmission *submission_ext;
	ESourceMDN *mdn_ext;
	GtkLabel *label;
	GtkButton *button;
	GtkWidget *widget;
	GtkWidget *container;
	GtkSizeGroup *size_group;
	GEnumClass *enum_class;
	GEnumValue *enum_value;
	EMdnResponsePolicy policy;
	CamelProvider *provider = NULL;
	const gchar *extension_name;
	const gchar *text;
	gchar *markup;

	page = E_MAIL_CONFIG_DEFAULTS_PAGE (object);

	/* Chain up to parent's constructed() method. */
	G_OBJECT_CLASS (e_mail_config_defaults_page_parent_class)->
		constructed (object);

	source = e_mail_config_defaults_page_get_account_source (page);
	extension_name = E_SOURCE_EXTENSION_MAIL_ACCOUNT;
	account_ext = e_source_get_extension (source, extension_name);
	if (e_source_backend_get_backend_name (account_ext))
		provider = camel_provider_get (e_source_backend_get_backend_name (account_ext), NULL);

	session = e_mail_config_defaults_page_get_session (page);
	source = e_mail_config_defaults_page_get_identity_source (page);

	extension_name = E_SOURCE_EXTENSION_MAIL_COMPOSITION;
	composition_ext = e_source_get_extension (source, extension_name);

	extension_name = E_SOURCE_EXTENSION_MAIL_SUBMISSION;
	submission_ext = e_source_get_extension (source, extension_name);

	extension_name = E_SOURCE_EXTENSION_MDN;
	mdn_ext = e_source_get_extension (source, extension_name);

	gtk_orientable_set_orientation (
		GTK_ORIENTABLE (page), GTK_ORIENTATION_VERTICAL);

	gtk_box_set_spacing (GTK_BOX (page), 12);

	size_group = gtk_size_group_new (GTK_SIZE_GROUP_HORIZONTAL);

	/*** Special Folders ***/

	widget = gtk_grid_new ();
	gtk_grid_set_row_spacing (GTK_GRID (widget), 6);
	gtk_grid_set_column_spacing (GTK_GRID (widget), 6);
	gtk_box_pack_start (GTK_BOX (page), widget, FALSE, FALSE, 0);
	gtk_widget_show (widget);

	container = widget;

	text = _("Special Folders");
	markup = g_markup_printf_escaped ("<b>%s</b>", text);
	widget = gtk_label_new (markup);
	gtk_label_set_use_markup (GTK_LABEL (widget), TRUE);
	gtk_misc_set_alignment (GTK_MISC (widget), 0.0, 0.5);
	gtk_grid_attach (GTK_GRID (container), widget, 0, 0, 2, 1);
	gtk_widget_show (widget);
	g_free (markup);

	text = _("Draft Messages _Folder:");
	widget = gtk_label_new_with_mnemonic (text);
	gtk_widget_set_margin_left (widget, 12);
	gtk_size_group_add_widget (size_group, widget);
	gtk_misc_set_alignment (GTK_MISC (widget), 1.0, 0.5);
	gtk_grid_attach (GTK_GRID (container), widget, 0, 1, 1, 1);
	gtk_widget_show (widget);

	label = GTK_LABEL (widget);

	text = _("Choose a folder for saving draft messages.");
	widget = em_folder_selection_button_new (session, "", text);
	gtk_widget_set_hexpand (widget, TRUE);
	gtk_label_set_mnemonic_widget (label, widget);
	gtk_grid_attach (GTK_GRID (container), widget, 1, 1, 1, 1);
	page->priv->drafts_button = widget;  /* not referenced */
	gtk_widget_show (widget);

	g_object_bind_property (
		composition_ext, "drafts-folder",
		widget, "folder-uri",
		G_BINDING_BIDIRECTIONAL |
		G_BINDING_SYNC_CREATE);

	text = _("Sent _Messages Folder:");
	widget = gtk_label_new_with_mnemonic (text);
	gtk_widget_set_margin_left (widget, 12);
	gtk_size_group_add_widget (size_group, widget);
	gtk_misc_set_alignment (GTK_MISC (widget), 1.0, 0.5);
	gtk_grid_attach (GTK_GRID (container), widget, 0, 2, 1, 1);
	gtk_widget_show (widget);

	label = GTK_LABEL (widget);

	text = _("Choose a folder for saving sent messages.");
	widget = em_folder_selection_button_new (session, "", text);
	gtk_widget_set_hexpand (widget, TRUE);
	gtk_label_set_mnemonic_widget (label, widget);
	gtk_grid_attach (GTK_GRID (container), widget, 1, 2, 1, 1);
	page->priv->sent_button = widget;  /* not referenced */
	gtk_widget_show (widget);

	if (provider && (provider->flags & CAMEL_PROVIDER_DISABLE_SENT_FOLDER) != 0) {
		gtk_widget_set_sensitive (GTK_WIDGET (label), FALSE);
		gtk_widget_set_sensitive (widget, FALSE);
	}

	g_object_bind_property (
		submission_ext, "sent-folder",
		widget, "folder-uri",
		G_BINDING_BIDIRECTIONAL |
		G_BINDING_SYNC_CREATE);

	widget = gtk_check_button_new_with_mnemonic (_("S_ave replies in the folder of the message being replied to"));
	g_object_set (widget, "xalign", 0.0, NULL);
	gtk_widget_set_halign (widget, GTK_ALIGN_START);
	gtk_grid_attach (GTK_GRID (container), widget, 0, 3, 2, 1);
	page->priv->replies_toggle = widget; /* not referenced */
	gtk_widget_show (widget);

	if (provider && (provider->flags & CAMEL_PROVIDER_DISABLE_SENT_FOLDER) != 0) {
		gtk_widget_set_sensitive (widget, FALSE);
	}

	g_object_bind_property (
		submission_ext, "replies-to-origin-folder",
		widget, "active",
		G_BINDING_BIDIRECTIONAL |
		G_BINDING_SYNC_CREATE);

	widget = gtk_button_new_with_mnemonic (_("_Restore Defaults"));
	gtk_widget_set_halign (widget, GTK_ALIGN_START);
	gtk_grid_attach (GTK_GRID (container), widget, 1, 6, 1, 1);
	gtk_widget_show (widget);

	g_signal_connect_swapped (
		widget, "clicked",
		G_CALLBACK (mail_config_defaults_page_restore_folders),
		page);

	button = GTK_BUTTON (widget);

	widget = mail_config_defaults_page_add_real_folder (
		page, size_group, button,
		_("Use a Real Folder for _Trash:"),
		_("Choose a folder for deleted messages."),
		"real-trash-path", "use-real-trash-path");
	if (widget != NULL) {
		gtk_grid_attach (GTK_GRID (container), widget, 0, 4, 2, 1);
		gtk_widget_show (widget);
	}

	widget = mail_config_defaults_page_add_real_folder (
		page, size_group, button,
		_("Use a Real Folder for _Junk:"),
		_("Choose a folder for junk messages."),
		"real-junk-path", "use-real-junk-path");
	if (widget != NULL) {
		gtk_grid_attach (GTK_GRID (container), widget, 0, 5, 2, 1);
		gtk_widget_show (widget);
	}

	/*** Composing Messages ***/

	widget = gtk_grid_new ();
	gtk_grid_set_row_spacing (GTK_GRID (widget), 6);
	gtk_grid_set_column_spacing (GTK_GRID (widget), 6);
	gtk_box_pack_start (GTK_BOX (page), widget, FALSE, FALSE, 0);
	gtk_widget_show (widget);

	container = widget;

	text = _("Composing Messages");
	markup = g_markup_printf_escaped ("<b>%s</b>", text);
	widget = gtk_label_new (markup);
	gtk_label_set_use_markup (GTK_LABEL (widget), TRUE);
	gtk_misc_set_alignment (GTK_MISC (widget), 0.0, 0.5);
	gtk_grid_attach (GTK_GRID (container), widget, 0, 0, 1, 1);
	gtk_widget_show (widget);
	g_free (markup);

	text = _("Alway_s carbon-copy (cc) to:");
	widget = gtk_label_new_with_mnemonic (text);
	gtk_widget_set_margin_left (widget, 12);
	gtk_misc_set_alignment (GTK_MISC (widget), 0.0, 0.5);
	gtk_grid_attach (GTK_GRID (container), widget, 0, 1, 1, 1);
	gtk_widget_show (widget);

	label = GTK_LABEL (widget);

	widget = gtk_entry_new ();
	gtk_widget_set_hexpand (widget, TRUE);
	gtk_widget_set_margin_left (widget, 12);
	gtk_label_set_mnemonic_widget (label, widget);
	gtk_grid_attach (GTK_GRID (container), widget, 0, 2, 1, 1);
	gtk_widget_show (widget);

	g_object_bind_property_full (
		composition_ext, "cc",
		widget, "text",
		G_BINDING_BIDIRECTIONAL |
		G_BINDING_SYNC_CREATE,
		mail_config_defaults_page_addrs_to_string,
		mail_config_defaults_page_string_to_addrs,
		NULL, (GDestroyNotify) NULL);

	text = _("Always _blind carbon-copy (bcc) to:");
	widget = gtk_label_new_with_mnemonic (text);
	gtk_widget_set_margin_left (widget, 12);
	gtk_misc_set_alignment (GTK_MISC (widget), 0.0, 0.5);
	gtk_grid_attach (GTK_GRID (container), widget, 0, 3, 1, 1);
	gtk_widget_show (widget);

	label = GTK_LABEL (widget);

	widget = gtk_entry_new ();
	gtk_widget_set_hexpand (widget, TRUE);
	gtk_widget_set_margin_left (widget, 12);
	gtk_label_set_mnemonic_widget (label, widget);
	gtk_grid_attach (GTK_GRID (container), widget, 0, 4, 1, 1);
	gtk_widget_show (widget);

	g_object_bind_property_full (
		composition_ext, "bcc",
		widget, "text",
		G_BINDING_BIDIRECTIONAL |
		G_BINDING_SYNC_CREATE,
		mail_config_defaults_page_addrs_to_string,
		mail_config_defaults_page_string_to_addrs,
		NULL, (GDestroyNotify) NULL);

	/*** Message Receipts ***/

	widget = gtk_grid_new ();
	gtk_grid_set_row_spacing (GTK_GRID (widget), 6);
	gtk_grid_set_column_spacing (GTK_GRID (widget), 6);
	gtk_box_pack_start (GTK_BOX (page), widget, FALSE, FALSE, 0);
	gtk_widget_show (widget);

	container = widget;

	text = _("Message Receipts");
	markup = g_markup_printf_escaped ("<b>%s</b>", text);
	widget = gtk_label_new (markup);
	gtk_label_set_use_markup (GTK_LABEL (widget), TRUE);
	gtk_misc_set_alignment (GTK_MISC (widget), 0.0, 0.5);
	gtk_grid_attach (GTK_GRID (container), widget, 0, 0, 2, 1);
	gtk_widget_show (widget);
	g_free (markup);

	text = _("S_end message receipts:");
	widget = gtk_label_new_with_mnemonic (text);
	gtk_widget_set_margin_left (widget, 12);
	gtk_size_group_add_widget (size_group, widget);
	gtk_misc_set_alignment (GTK_MISC (widget), 1.0, 0.5);
	gtk_grid_attach (GTK_GRID (container), widget, 0, 1, 1, 1);
	gtk_widget_show (widget);

	label = GTK_LABEL (widget);

	widget = gtk_combo_box_text_new ();
	gtk_widget_set_hexpand (widget, TRUE);
	gtk_label_set_mnemonic_widget (label, widget);
	gtk_grid_attach (GTK_GRID (container), widget, 1, 1, 1, 1);
	gtk_widget_show (widget);

	/* XXX This is a pain in the butt, but we want to avoid hard-coding
	 *     string values from the EMdnResponsePolicy enum class in case
	 *     they change in the future. */
	enum_class = g_type_class_ref (E_TYPE_MDN_RESPONSE_POLICY);
	policy = E_MDN_RESPONSE_POLICY_NEVER;
	enum_value = g_enum_get_value (enum_class, policy);
	g_return_if_fail (enum_value != NULL);
	gtk_combo_box_text_append (
		GTK_COMBO_BOX_TEXT (widget),
		enum_value->value_nick, _("Never"));
	policy = E_MDN_RESPONSE_POLICY_ALWAYS;
	enum_value = g_enum_get_value (enum_class, policy);
	g_return_if_fail (enum_value != NULL);
	gtk_combo_box_text_append (
		GTK_COMBO_BOX_TEXT (widget),
		enum_value->value_nick, _("Always"));
	policy = E_MDN_RESPONSE_POLICY_ASK;
	enum_value = g_enum_get_value (enum_class, policy);
	g_return_if_fail (enum_value != NULL);
	gtk_combo_box_text_append (
		GTK_COMBO_BOX_TEXT (widget),
		enum_value->value_nick, _("Ask for each message"));
	g_type_class_unref (enum_class);

	g_object_bind_property_full (
		mdn_ext, "response-policy",
		widget, "active-id",
		G_BINDING_BIDIRECTIONAL |
		G_BINDING_SYNC_CREATE,
		e_binding_transform_enum_value_to_nick,
		e_binding_transform_enum_nick_to_value,
		NULL, (GDestroyNotify) NULL);

	g_object_unref (size_group);

	e_extensible_load_extensions (E_EXTENSIBLE (page));
}

static void
e_mail_config_defaults_page_class_init (EMailConfigDefaultsPageClass *class)
{
	GObjectClass *object_class;

	g_type_class_add_private (
		class, sizeof (EMailConfigDefaultsPagePrivate));

	object_class = G_OBJECT_CLASS (class);
	object_class->set_property = mail_config_defaults_page_set_property;
	object_class->get_property = mail_config_defaults_page_get_property;
	object_class->dispose = mail_config_defaults_page_dispose;
	object_class->constructed = mail_config_defaults_page_constructed;

	g_object_class_install_property (
		object_class,
		PROP_ACCOUNT_SOURCE,
		g_param_spec_object (
			"account-source",
			"Account Source",
			"Mail account source being edited",
			E_TYPE_SOURCE,
			G_PARAM_READWRITE |
			G_PARAM_CONSTRUCT_ONLY |
			G_PARAM_STATIC_STRINGS));

	g_object_class_install_property (
		object_class,
		PROP_IDENTITY_SOURCE,
		g_param_spec_object (
			"identity-source",
			"Identity Source",
			"Mail identity source being edited",
			E_TYPE_SOURCE,
			G_PARAM_READWRITE |
			G_PARAM_CONSTRUCT_ONLY |
			G_PARAM_STATIC_STRINGS));

	g_object_class_install_property (
		object_class,
		PROP_SESSION,
		g_param_spec_object (
			"session",
			"Session",
			"Mail session",
			E_TYPE_MAIL_SESSION,
			G_PARAM_READWRITE |
			G_PARAM_CONSTRUCT_ONLY |
			G_PARAM_STATIC_STRINGS));
}

static void
e_mail_config_defaults_page_interface_init (EMailConfigPageInterface *interface)
{
	interface->title = _("Defaults");
	interface->sort_order = E_MAIL_CONFIG_DEFAULTS_PAGE_SORT_ORDER;
}

static void
e_mail_config_defaults_page_init (EMailConfigDefaultsPage *page)
{
	page->priv = E_MAIL_CONFIG_DEFAULTS_PAGE_GET_PRIVATE (page);
}

EMailConfigPage *
e_mail_config_defaults_page_new (EMailSession *session,
                                 ESource *account_source,
                                 ESource *identity_source)
{
	g_return_val_if_fail (E_IS_MAIL_SESSION (session), NULL);
	g_return_val_if_fail (E_IS_SOURCE (account_source), NULL);
	g_return_val_if_fail (E_IS_SOURCE (identity_source), NULL);

	return g_object_new (
		E_TYPE_MAIL_CONFIG_DEFAULTS_PAGE,
		"account-source", account_source,
		"identity-source", identity_source,
		"session", session, NULL);
}

EMailSession *
e_mail_config_defaults_page_get_session (EMailConfigDefaultsPage *page)
{
	g_return_val_if_fail (E_IS_MAIL_CONFIG_DEFAULTS_PAGE (page), NULL);

	return page->priv->session;
}

ESource *
e_mail_config_defaults_page_get_account_source (EMailConfigDefaultsPage *page)
{
	g_return_val_if_fail (E_IS_MAIL_CONFIG_DEFAULTS_PAGE (page), NULL);

	return page->priv->account_source;
}

ESource *
e_mail_config_defaults_page_get_identity_source (EMailConfigDefaultsPage *page)
{
	g_return_val_if_fail (E_IS_MAIL_CONFIG_DEFAULTS_PAGE (page), NULL);

	return page->priv->identity_source;
}


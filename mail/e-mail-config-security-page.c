/*
 * e-mail-config-security-page.c
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

#include "e-mail-config-security-page.h"

#include <config.h>
#include <glib/gi18n-lib.h>

#include <libebackend/libebackend.h>

#if defined (HAVE_NSS)
#include <smime/gui/e-cert-selector.h>
#endif /* HAVE_NSS */

#define E_MAIL_CONFIG_SECURITY_PAGE_GET_PRIVATE(obj) \
	(G_TYPE_INSTANCE_GET_PRIVATE \
	((obj), E_TYPE_MAIL_CONFIG_SECURITY_PAGE, EMailConfigSecurityPagePrivate))

struct _EMailConfigSecurityPagePrivate {
	ESource *identity_source;
};

enum {
	PROP_0,
	PROP_IDENTITY_SOURCE
};

/* Forward Declarations */
static void	e_mail_config_security_page_interface_init
					(EMailConfigPageInterface *interface);

G_DEFINE_TYPE_WITH_CODE (
	EMailConfigSecurityPage,
	e_mail_config_security_page,
	GTK_TYPE_BOX,
	G_IMPLEMENT_INTERFACE (
		E_TYPE_EXTENSIBLE, NULL)
	G_IMPLEMENT_INTERFACE (
		E_TYPE_MAIL_CONFIG_PAGE,
		e_mail_config_security_page_interface_init))

static gboolean
mail_config_security_page_string_has_text (GBinding *binding,
                                           const GValue *source_value,
                                           GValue *target_value,
                                           gpointer unused)
{
	const gchar *string;
	gchar *stripped;

	string = g_value_get_string (source_value);

	if (string == NULL)
		string = "";

	stripped = g_strstrip (g_strdup (string));
	g_value_set_boolean (target_value, *stripped != '\0');
	g_free (stripped);

	return TRUE;
}

static void
mail_config_security_page_cert_selected (ECertSelector *selector,
                                         const gchar *key,
                                         GtkEntry *entry)
{
	if (key != NULL)
		gtk_entry_set_text (entry, key);

	gtk_widget_destroy (GTK_WIDGET (selector));
}

static void
mail_config_security_page_select_encrypt_cert (GtkButton *button,
                                               GtkEntry *entry)
{
	GtkWidget *selector;
	gpointer parent;

	parent = gtk_widget_get_toplevel (GTK_WIDGET (button));
	parent = GTK_IS_WIDGET (parent) ? parent : NULL;

	selector = e_cert_selector_new (
		E_CERT_SELECTOR_RECIPIENT,
		gtk_entry_get_text (entry));
	gtk_window_set_transient_for (
		GTK_WINDOW (selector), parent);
	gtk_widget_show (selector);

	g_signal_connect (
		selector, "selected",
		G_CALLBACK (mail_config_security_page_cert_selected),
		entry);
}

static void
mail_config_security_page_select_sign_cert (GtkButton *button,
                                            GtkEntry *entry)
{
	GtkWidget *selector;
	gpointer parent;

	parent = gtk_widget_get_toplevel (GTK_WIDGET (button));
	parent = GTK_IS_WIDGET (parent) ? parent : NULL;

	selector = e_cert_selector_new (
		E_CERT_SELECTOR_SIGNER,
		gtk_entry_get_text (entry));
	gtk_window_set_transient_for (
		GTK_WINDOW (selector), parent);
	gtk_widget_show (selector);

	g_signal_connect (
		selector, "selected",
		G_CALLBACK (mail_config_security_page_cert_selected),
		entry);
}

static void
mail_config_security_page_clear_cert (GtkButton *button,
                                      GtkEntry *entry)
{
	gtk_entry_set_text (entry, "");
}

static void
mail_config_security_page_set_identity_source (EMailConfigSecurityPage *page,
                                               ESource *identity_source)
{
	g_return_if_fail (E_IS_SOURCE (identity_source));
	g_return_if_fail (page->priv->identity_source == NULL);

	page->priv->identity_source = g_object_ref (identity_source);
}

static void
mail_config_security_page_set_property (GObject *object,
                                        guint property_id,
                                        const GValue *value,
                                        GParamSpec *pspec)
{
	switch (property_id) {
		case PROP_IDENTITY_SOURCE:
			mail_config_security_page_set_identity_source (
				E_MAIL_CONFIG_SECURITY_PAGE (object),
				g_value_get_object (value));
			return;
	}

	G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
}

static void
mail_config_security_page_get_property (GObject *object,
                                        guint property_id,
                                        GValue *value,
                                        GParamSpec *pspec)
{
	switch (property_id) {
		case PROP_IDENTITY_SOURCE:
			g_value_set_object (
				value,
				e_mail_config_security_page_get_identity_source (
				E_MAIL_CONFIG_SECURITY_PAGE (object)));
			return;
	}

	G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
}

static void
mail_config_security_page_dispose (GObject *object)
{
	EMailConfigSecurityPagePrivate *priv;

	priv = E_MAIL_CONFIG_SECURITY_PAGE_GET_PRIVATE (object);

	if (priv->identity_source != NULL) {
		g_object_unref (priv->identity_source);
		priv->identity_source = NULL;
	}

	/* Chain up to parent's dispose() method. */
	G_OBJECT_CLASS (e_mail_config_security_page_parent_class)->
		dispose (object);
}

static void
mail_config_security_page_constructed (GObject *object)
{
	EMailConfigSecurityPage *page;
	ESource *source;
	ESourceMailComposition *composition_ext;
	ESourceOpenPGP *openpgp_ext;
	GtkEntry *entry;
	GtkLabel *label;
	GtkWidget *widget;
	GtkWidget *container;
	GtkSizeGroup *size_group;
	const gchar *extension_name;
	const gchar *text;
	gchar *markup;

#if defined (HAVE_NSS)
	ESourceSMIME *smime_ext;
#endif /* HAVE_NSS */

	page = E_MAIL_CONFIG_SECURITY_PAGE (object);

	/* Chain up to parent's constructed() method. */
	G_OBJECT_CLASS (e_mail_config_security_page_parent_class)->
		constructed (object);

	source = e_mail_config_security_page_get_identity_source (page);

	extension_name = E_SOURCE_EXTENSION_MAIL_COMPOSITION;
	composition_ext = e_source_get_extension (source, extension_name);

	extension_name = E_SOURCE_EXTENSION_OPENPGP;
	openpgp_ext = e_source_get_extension (source, extension_name);

#if defined (HAVE_NSS)
	extension_name = E_SOURCE_EXTENSION_SMIME;
	smime_ext = e_source_get_extension (source, extension_name);
#endif /* HAVE_NSS */

	gtk_orientable_set_orientation (
		GTK_ORIENTABLE (page), GTK_ORIENTATION_VERTICAL);

	gtk_box_set_spacing (GTK_BOX (page), 12);

	size_group = gtk_size_group_new (GTK_SIZE_GROUP_HORIZONTAL);

	/*** General ***/

	widget = gtk_grid_new ();
	gtk_grid_set_row_spacing (GTK_GRID (widget), 6);
	gtk_grid_set_column_spacing (GTK_GRID (widget), 6);
	gtk_box_pack_start (GTK_BOX (page), widget, FALSE, FALSE, 0);
	gtk_widget_show (widget);

	container = widget;

	text = _("General");
	markup = g_markup_printf_escaped ("<b>%s</b>", text);
	widget = gtk_label_new (markup);
	gtk_label_set_use_markup (GTK_LABEL (widget), TRUE);
	gtk_misc_set_alignment (GTK_MISC (widget), 0.0, 0.5);
	gtk_grid_attach (GTK_GRID (container), widget, 0, 0, 1, 1);
	gtk_widget_show (widget);

	text = _("_Do not sign meeting requests (for Outlook compatibility)");
	widget = gtk_check_button_new_with_mnemonic (text);
	gtk_widget_set_margin_left (widget, 12);
	gtk_grid_attach (GTK_GRID (container), widget, 0, 1, 1, 1);
	gtk_widget_show (widget);

	g_object_bind_property (
		composition_ext, "sign-imip",
		widget, "active",
		G_BINDING_BIDIRECTIONAL |
		G_BINDING_SYNC_CREATE);

	/*** Pretty Good Privacy (OpenPGP) ***/

	widget = gtk_grid_new ();
	gtk_grid_set_row_spacing (GTK_GRID (widget), 6);
	gtk_grid_set_column_spacing (GTK_GRID (widget), 6);
	gtk_box_pack_start (GTK_BOX (page), widget, FALSE, FALSE, 0);
	gtk_widget_show (widget);

	container = widget;

	text = _("Pretty Good Privacy (OpenPGP)");
	markup = g_markup_printf_escaped ("<b>%s</b>", text);
	widget = gtk_label_new (markup);
	gtk_label_set_use_markup (GTK_LABEL (widget), TRUE);
	gtk_misc_set_alignment (GTK_MISC (widget), 0.0, 0.5);
	gtk_grid_attach (GTK_GRID (container), widget, 0, 0, 2, 1);
	gtk_widget_show (widget);

	text = _("OpenPGP _Key ID:");
	widget = gtk_label_new_with_mnemonic (text);
	gtk_widget_set_margin_left (widget, 12);
	gtk_size_group_add_widget (size_group, widget);
	gtk_misc_set_alignment (GTK_MISC (widget), 1.0, 0.5);
	gtk_grid_attach (GTK_GRID (container), widget, 0, 1, 1, 1);
	gtk_widget_show (widget);

	label = GTK_LABEL (widget);

	widget = gtk_entry_new ();
	gtk_widget_set_hexpand (widget, TRUE);
	gtk_label_set_mnemonic_widget (label, widget);
	gtk_grid_attach (GTK_GRID (container), widget, 1, 1, 1, 1);
	gtk_widget_show (widget);

	g_object_bind_property (
		openpgp_ext, "key-id",
		widget, "text",
		G_BINDING_SYNC_CREATE |
		G_BINDING_BIDIRECTIONAL);

	text = _("Si_gning algorithm:");
	widget = gtk_label_new_with_mnemonic (text);
	gtk_widget_set_margin_left (widget, 12);
	gtk_size_group_add_widget (size_group, widget);
	gtk_misc_set_alignment (GTK_MISC (widget), 1.0, 0.5);
	gtk_grid_attach (GTK_GRID (container), widget, 0, 2, 1, 1);
	gtk_widget_show (widget);

	label = GTK_LABEL (widget);

	widget = gtk_combo_box_text_new ();
	gtk_combo_box_text_append (
		GTK_COMBO_BOX_TEXT (widget),
		"", _("Default"));
	gtk_combo_box_text_append (
		GTK_COMBO_BOX_TEXT (widget),
		"sha1", _("SHA1"));
	gtk_combo_box_text_append (
		GTK_COMBO_BOX_TEXT (widget),
		"sha256", _("SHA256"));
	gtk_combo_box_text_append (
		GTK_COMBO_BOX_TEXT (widget),
		"sha384", _("SHA384"));
	gtk_combo_box_text_append (
		GTK_COMBO_BOX_TEXT (widget),
		"sha512", _("SHA512"));
	gtk_widget_set_halign (widget, GTK_ALIGN_START);
	gtk_label_set_mnemonic_widget (label, widget);
	gtk_grid_attach (GTK_GRID (container), widget, 1, 2, 1, 1);
	gtk_widget_show (widget);

	g_object_bind_property (
		openpgp_ext, "signing-algorithm",
		widget, "active-id",
		G_BINDING_SYNC_CREATE |
		G_BINDING_BIDIRECTIONAL);

	/* Make sure the combo box has an active item. */
	if (gtk_combo_box_get_active_id (GTK_COMBO_BOX (widget)) == NULL)
		gtk_combo_box_set_active (GTK_COMBO_BOX (widget), 0);

	text = _("Al_ways sign outgoing messages when using this account");
	widget = gtk_check_button_new_with_mnemonic (text);
	gtk_widget_set_margin_left (widget, 12);
	gtk_grid_attach (GTK_GRID (container), widget, 0, 3, 2, 1);
	gtk_widget_show (widget);

	g_object_bind_property (
		openpgp_ext, "sign-by-default",
		widget, "active",
		G_BINDING_SYNC_CREATE |
		G_BINDING_BIDIRECTIONAL);

	text = _("Always encrypt to _myself when sending encrypted messages");
	widget = gtk_check_button_new_with_mnemonic (text);
	gtk_widget_set_margin_left (widget, 12);
	gtk_grid_attach (GTK_GRID (container), widget, 0, 4, 2, 1);
	gtk_widget_show (widget);

	g_object_bind_property (
		openpgp_ext, "encrypt-to-self",
		widget, "active",
		G_BINDING_SYNC_CREATE |
		G_BINDING_BIDIRECTIONAL);

	text = _("Always _trust keys in my keyring when encrypting");
	widget = gtk_check_button_new_with_mnemonic (text);
	gtk_widget_set_margin_left (widget, 12);
	gtk_grid_attach (GTK_GRID (container), widget, 0, 5, 2, 1);
	gtk_widget_show (widget);

	g_object_bind_property (
		openpgp_ext, "always-trust",
		widget, "active",
		G_BINDING_SYNC_CREATE |
		G_BINDING_BIDIRECTIONAL);

#if defined (HAVE_NSS)

	/*** Security MIME (S/MIME) ***/

	widget = gtk_grid_new ();
	gtk_grid_set_row_spacing (GTK_GRID (widget), 6);
	gtk_grid_set_column_spacing (GTK_GRID (widget), 6);
	gtk_box_pack_start (GTK_BOX (page), widget, FALSE, FALSE, 0);
	gtk_widget_show (widget);

	container = widget;

	text = _("Secure MIME (S/MIME)");
	markup = g_markup_printf_escaped ("<b>%s</b>", text);
	widget = gtk_label_new (markup);
	gtk_label_set_use_markup (GTK_LABEL (widget), TRUE);
	gtk_misc_set_alignment (GTK_MISC (widget), 0.0, 0.5);
	gtk_grid_attach (GTK_GRID (container), widget, 0, 0, 4, 1);
	gtk_widget_show (widget);

	text = _("Sig_ning certificate:");
	widget = gtk_label_new_with_mnemonic (text);
	gtk_widget_set_margin_left (widget, 12);
	gtk_size_group_add_widget (size_group, widget);
	gtk_misc_set_alignment (GTK_MISC (widget), 1.0, 0.5);
	gtk_grid_attach (GTK_GRID (container), widget, 0, 1, 1, 1);
	gtk_widget_show (widget);

	label = GTK_LABEL (widget);

	widget = gtk_entry_new ();
	gtk_widget_set_hexpand (widget, TRUE);
	gtk_label_set_mnemonic_widget (label, widget);
	gtk_grid_attach (GTK_GRID (container), widget, 1, 1, 1, 1);
	gtk_widget_show (widget);

	g_object_bind_property (
		smime_ext, "signing-certificate",
		widget, "text",
		G_BINDING_BIDIRECTIONAL |
		G_BINDING_SYNC_CREATE);

	entry = GTK_ENTRY (widget);

	widget = gtk_button_new_with_label (_("Select"));
	gtk_grid_attach (GTK_GRID (container), widget, 2, 1, 1, 1);
	gtk_widget_show (widget);

	g_signal_connect (
		widget, "clicked",
		G_CALLBACK (mail_config_security_page_select_sign_cert),
		entry);

	widget = gtk_button_new_from_stock (GTK_STOCK_CLEAR);
	gtk_grid_attach (GTK_GRID (container), widget, 3, 1, 1, 1);
	gtk_widget_show (widget);

	g_signal_connect (
		widget, "clicked",
		G_CALLBACK (mail_config_security_page_clear_cert),
		entry);

	text = _("Signing _algorithm:");
	widget = gtk_label_new_with_mnemonic (text);
	gtk_widget_set_margin_left (widget, 12);
	gtk_size_group_add_widget (size_group, widget);
	gtk_misc_set_alignment (GTK_MISC (widget), 1.0, 0.5);
	gtk_grid_attach (GTK_GRID (container), widget, 0, 2, 1, 1);
	gtk_widget_show (widget);

	label = GTK_LABEL (widget);

	widget = gtk_combo_box_text_new ();
	gtk_combo_box_text_append (
		GTK_COMBO_BOX_TEXT (widget),
		"", _("Default"));
	gtk_combo_box_text_append (
		GTK_COMBO_BOX_TEXT (widget),
		"sha1", _("SHA1"));
	gtk_combo_box_text_append (
		GTK_COMBO_BOX_TEXT (widget),
		"sha256", _("SHA256"));
	gtk_combo_box_text_append (
		GTK_COMBO_BOX_TEXT (widget),
		"sha384", _("SHA384"));
	gtk_combo_box_text_append (
		GTK_COMBO_BOX_TEXT (widget),
		"sha512", _("SHA512"));
	gtk_widget_set_halign (widget, GTK_ALIGN_START);
	gtk_label_set_mnemonic_widget (label, widget);
	gtk_grid_attach (GTK_GRID (container), widget, 1, 2, 1, 1);
	gtk_widget_show (widget);

	g_object_bind_property (
		smime_ext, "signing-algorithm",
		widget, "active-id",
		G_BINDING_SYNC_CREATE |
		G_BINDING_BIDIRECTIONAL);

	/* Make sure the combo box has an active item. */
	if (gtk_combo_box_get_active_id (GTK_COMBO_BOX (widget)) == NULL)
		gtk_combo_box_set_active (GTK_COMBO_BOX (widget), 0);

	text = _("Always sign outgoing messages when using this account");
	widget = gtk_check_button_new_with_mnemonic (text);
	gtk_widget_set_margin_left (widget, 12);
	gtk_grid_attach (GTK_GRID (container), widget, 0, 3, 4, 1);
	gtk_widget_show (widget);

	g_object_bind_property (
		smime_ext, "sign-by-default",
		widget, "active",
		G_BINDING_SYNC_CREATE |
		G_BINDING_BIDIRECTIONAL);

	g_object_bind_property_full (
		smime_ext, "signing-certificate",
		widget, "sensitive",
		G_BINDING_SYNC_CREATE,
		mail_config_security_page_string_has_text,
		NULL,
		NULL, (GDestroyNotify) NULL);

	/* Add extra padding between signing stuff and encryption stuff. */
	gtk_widget_set_margin_bottom (widget, 6);

	text = _("Encryption certificate:");
	widget = gtk_label_new_with_mnemonic (text);
	gtk_widget_set_margin_left (widget, 12);
	gtk_size_group_add_widget (size_group, widget);
	gtk_misc_set_alignment (GTK_MISC (widget), 1.0, 0.5);
	gtk_grid_attach (GTK_GRID (container), widget, 0, 4, 1, 1);
	gtk_widget_show (widget);

	label = GTK_LABEL (widget);

	widget = gtk_entry_new ();
	gtk_widget_set_hexpand (widget, TRUE);
	gtk_label_set_mnemonic_widget (label, widget);
	gtk_grid_attach (GTK_GRID (container), widget, 1, 4, 1, 1);
	gtk_widget_show (widget);

	g_object_bind_property (
		smime_ext, "encryption-certificate",
		widget, "text",
		G_BINDING_BIDIRECTIONAL |
		G_BINDING_SYNC_CREATE);

	entry = GTK_ENTRY (widget);

	widget = gtk_button_new_with_label (_("Select"));
	gtk_grid_attach (GTK_GRID (container), widget, 2, 4, 1, 1);
	gtk_widget_show (widget);

	g_signal_connect (
		widget, "clicked",
		G_CALLBACK (mail_config_security_page_select_encrypt_cert),
		entry);

	widget = gtk_button_new_from_stock (GTK_STOCK_CLEAR);
	gtk_grid_attach (GTK_GRID (container), widget, 3, 4, 1, 1);
	gtk_widget_show (widget);

	g_signal_connect (
		widget, "clicked",
		G_CALLBACK (mail_config_security_page_clear_cert),
		entry);

	text = _("Always encrypt outgoing messages when using this account");
	widget = gtk_check_button_new_with_mnemonic (text);
	gtk_widget_set_margin_left (widget, 12);
	gtk_grid_attach (GTK_GRID (container), widget, 0, 5, 4, 1);
	gtk_widget_show (widget);

	g_object_bind_property (
		smime_ext, "encrypt-by-default",
		widget, "active",
		G_BINDING_SYNC_CREATE |
		G_BINDING_BIDIRECTIONAL);

	g_object_bind_property_full (
		smime_ext, "encryption-certificate",
		widget, "sensitive",
		G_BINDING_SYNC_CREATE,
		mail_config_security_page_string_has_text,
		NULL,
		NULL, (GDestroyNotify) NULL);

	text = _("Always encrypt to myself when sending encrypted messages");
	widget = gtk_check_button_new_with_mnemonic (text);
	gtk_widget_set_margin_left (widget, 12);
	gtk_grid_attach (GTK_GRID (container), widget, 0, 6, 4, 1);
	gtk_widget_show (widget);

	g_object_bind_property (
		smime_ext, "encrypt-to-self",
		widget, "active",
		G_BINDING_SYNC_CREATE |
		G_BINDING_BIDIRECTIONAL);

	g_object_bind_property_full (
		smime_ext, "encryption-certificate",
		widget, "sensitive",
		G_BINDING_SYNC_CREATE,
		mail_config_security_page_string_has_text,
		NULL,
		NULL, (GDestroyNotify) NULL);

#endif /* HAVE_NSS */

	g_object_unref (size_group);

	e_extensible_load_extensions (E_EXTENSIBLE (page));
}

static void
e_mail_config_security_page_class_init (EMailConfigSecurityPageClass *class)
{
	GObjectClass *object_class;

	g_type_class_add_private (
		class, sizeof (EMailConfigSecurityPagePrivate));

	object_class = G_OBJECT_CLASS (class);
	object_class->set_property = mail_config_security_page_set_property;
	object_class->get_property = mail_config_security_page_get_property;
	object_class->dispose = mail_config_security_page_dispose;
	object_class->constructed = mail_config_security_page_constructed;

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
}

static void
e_mail_config_security_page_interface_init (EMailConfigPageInterface *interface)
{
	interface->title = _("Security");
	interface->sort_order = E_MAIL_CONFIG_SECURITY_PAGE_SORT_ORDER;
}

static void
e_mail_config_security_page_init (EMailConfigSecurityPage *page)
{
	page->priv = E_MAIL_CONFIG_SECURITY_PAGE_GET_PRIVATE (page);
}

EMailConfigPage *
e_mail_config_security_page_new (ESource *identity_source)
{
	g_return_val_if_fail (E_IS_SOURCE (identity_source), NULL);

	return g_object_new (
		E_TYPE_MAIL_CONFIG_SECURITY_PAGE,
		"identity-source", identity_source, NULL);
}

ESource *
e_mail_config_security_page_get_identity_source (EMailConfigSecurityPage *page)
{
	g_return_val_if_fail (E_IS_MAIL_CONFIG_SECURITY_PAGE (page), NULL);

	return page->priv->identity_source;
}


/*
 * e-google-chooser-button.h
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
 * License along with the program; if not, see <webcal://www.gnu.org/licenses/>
 *
 */

#ifndef E_GOOGLE_CHOOSER_BUTTON_H
#define E_GOOGLE_CHOOSER_BUTTON_H

#include "e-google-chooser.h"

/* Standard GObject macros */
#define E_TYPE_GOOGLE_CHOOSER_BUTTON \
	(e_google_chooser_button_get_type ())
#define E_GOOGLE_CHOOSER_BUTTON(obj) \
	(G_TYPE_CHECK_INSTANCE_CAST \
	((obj), E_TYPE_GOOGLE_CHOOSER_BUTTON, EGoogleChooserButton))
#define E_GOOGLE_CHOOSER_BUTTON_CLASS(cls) \
	(G_TYPE_CHECK_CLASS_CAST \
	((cls), E_TYPE_GOOGLE_CHOOSER_BUTTON, EGoogleChooserButtonClass))
#define E_IS_GOOGLE_CHOOSER_BUTTON(obj) \
	(G_TYPE_CHECK_INSTANCE_TYPE \
	((obj), E_TYPE_GOOGLE_CHOOSER_BUTTON))
#define E_IS_GOOGLE_CHOOSER_BUTTON_CLASS(cls) \
	(G_TYPE_CHECK_CLASS_TYPE \
	((cls), E_TYPE_GOOGLE_CHOOSER_BUTTON))
#define E_GOOGLE_CHOOSER_BUTTON_GET_CLASS(obj) \
	(G_TYPE_INSTANCE_GET_CLASS \
	((obj), E_TYPE_GOOGLE_CHOOSER_BUTTON, EGoogleChooserButtonClass))

G_BEGIN_DECLS

typedef struct _EGoogleChooserButton EGoogleChooserButton;
typedef struct _EGoogleChooserButtonClass EGoogleChooserButtonClass;
typedef struct _EGoogleChooserButtonPrivate EGoogleChooserButtonPrivate;

struct _EGoogleChooserButton {
	GtkButton parent;
	EGoogleChooserButtonPrivate *priv;
};

struct _EGoogleChooserButtonClass {
	GtkButtonClass parent_class;
};

GType		e_google_chooser_button_get_type (void);
void		e_google_chooser_button_type_register
						(GTypeModule *type_module);
GtkWidget *	e_google_chooser_button_new	(ESource *source);
ESource *	e_google_chooser_button_get_source
						(EGoogleChooserButton *button);

G_END_DECLS

#endif /* E_GOOGLE_CHOOSER_BUTTON_H */

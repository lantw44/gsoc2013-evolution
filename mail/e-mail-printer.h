/*
 * Class for printing emails
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
 * Copyright (C) 2011 Dan Vratil <dvratil@redhat.com>
 */

#ifndef E_MAIL_PRINTER_H
#define E_MAIL_PRINTER_H

#include <em-format/e-mail-part-list.h>

/* Standard GObject macros */
#define E_TYPE_MAIL_PRINTER \
	(e_mail_printer_get_type ())
#define E_MAIL_PRINTER(obj) \
	(G_TYPE_CHECK_INSTANCE_CAST \
	((obj), E_TYPE_MAIL_PRINTER, EMailPrinter))
#define E_MAIL_PRINTER_CLASS(cls) \
	(G_TYPE_CHECK_CLASS_CAST \
	((cls), E_TYPE_MAIL_PRINTER, EMailPrinterClass))
#define E_IS_MAIL_PRINTER(obj) \
	(G_TYPE_CHECK_INSTANCE_TYPE \
	((obj), E_TYPE_MAIL_PRINTER))
#define E_IS_MAIL_PRINTER_CLASS(cls) \
	(G_TYPE_CHECK_CLASS_TYPE \
	((cls), E_TYPE_MAIL_PRINTER_CLASS))
#define E_MAIL_PRINTER_GET_CLASS(obj) \
	(G_TYPE_INSTANCE_GET_CLASS \
	((obj), E_TYPE_MAIL_PRINTER, EMailPrinterClass))

G_BEGIN_DECLS

typedef struct _EMailPrinter EMailPrinter;
typedef struct _EMailPrinterClass EMailPrinterClass;
typedef struct _EMailPrinterPrivate EMailPrinterPrivate;

struct _EMailPrinter {
	GObject parent;
	EMailPrinterPrivate *priv;
};

struct _EMailPrinterClass {
	GObjectClass parent_class;

	void	(*done)			(EMailPrinter *printer,
					 GtkPrintOperation *operation,
					 GtkPrintOperationResult *result,
					 gpointer user_data);
};

GType		e_mail_printer_get_type	(void);

EMailPrinter *  e_mail_printer_new	(EMailPartList *source);

void		e_mail_printer_print	(EMailPrinter *printer,
					 gboolean export,
					 GCancellable *cancellable);

void            e_mail_printer_set_export_filename
                                        (EMailPrinter *printer,
                                         const gchar *filename);

const gchar *    e_mail_printer_get_export_filename
                                        (EMailPrinter *printer);

G_END_DECLS

#endif /* E_MAIL_PRINTER_H */
/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*-
 * e-activity.h
 *
 * Copyright (C) 1999-2008 Novell, Inc. (www.novell.com)
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) version 3.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with this program; if not, write to the
 * Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

#ifndef E_ACTIVITY_H
#define E_ACTIVITY_H

#include <glib-object.h>

/* Standard GObject macros */
#define E_TYPE_ACTIVITY \
	(e_activity_get_type ())
#define E_ACTIVITY(obj) \
	(G_TYPE_CHECK_INSTANCE_CAST \
	((obj), E_TYPE_ACTIVITY, EActivity))
#define E_ACTIVITY_CLASS(cls) \
	(G_TYPE_CHECK_CLASS_CAST \
	((cls), E_TYPE_ACTIVITY, EActivityClass))
#define E_IS_ACTIVITY(obj) \
	(G_TYPE_CHECK_INSTANCE_TYPE \
	((obj), E_TYPE_ACTIVITY))
#define E_IS_ACTIVITY_CLASS(cls) \
	(G_TYPE_CHECK_CLASS_TYPE \
	((cls), E_TYPE_ACTIVITY))
#define E_ACTIVITY_GET_CLASS(obj) \
	(G_TYPE_INSTANCE_GET_CLASS \
	((obj), E_TYPE_ACTIVITY, EActivityClass))

G_BEGIN_DECLS

typedef struct _EActivity EActivity;
typedef struct _EActivityClass EActivityClass;
typedef struct _EActivityPrivate EActivityPrivate;

struct _EActivity {
	GObject parent;
	EActivityPrivate *priv;
};

struct _EActivityClass {
	GObjectClass parent_class;
};

GType		e_activity_get_type		(void);
EActivity *	e_activity_new			(const gchar *primary_text);
void		e_activity_cancel		(EActivity *activity);
void		e_activity_complete		(EActivity *activity);
gchar *		e_activity_describe		(EActivity *activity);
gboolean	e_activity_is_cancelled		(EActivity *activity);
gboolean	e_activity_is_completed		(EActivity *activity);
gboolean	e_activity_get_cancellable	(EActivity *activity);
void		e_activity_set_cancellable	(EActivity *activity,
						 gboolean cancellable);
const gchar *	e_activity_get_icon_name	(EActivity *activity);
void		e_activity_set_icon_name	(EActivity *activity,
						 const gchar *icon_name);
gdouble		e_activity_get_percent		(EActivity *activity);
void		e_activity_set_percent		(EActivity *activity,
						 gdouble percent);
const gchar *	e_activity_get_primary_text	(EActivity *activity);
void		e_activity_set_primary_text	(EActivity *activity,
						 const gchar *primary_text);
const gchar *	e_activity_get_secondary_text	(EActivity *activity);
void		e_activity_set_secondary_text	(EActivity *activity,
						 const gchar *secondary_text);

G_END_DECLS

#endif /* E_ACTIVITY_H */

#ifndef __GTK_COMPAT_H__
#define __GTK_COMPAT_H__

#include <gtk/gtk.h>

/* Provide a compatibility layer for accessor functions introduced
 * in GTK+ 2.21.1 which we need to build with sealed GDK.  That way it
 * is still possible to build with GTK+ 2.20. */

#if (GTK_MAJOR_VERSION == 2 && GTK_MINOR_VERSION < 21) \
	|| (GTK_MINOR_VERSION == 21 && GTK_MICRO_VERSION < 1)

#define gdk_drag_context_get_actions(context)		(context)->actions
#define gdk_drag_context_get_suggested_action(context)	(context)->suggested_action
#define gdk_drag_context_get_selected_action(context)	(context)->action
#define gdk_drag_context_list_targets(context)		(context)->targets
#define gdk_visual_get_depth(visual)			(visual)->depth

#define gtk_accessible_get_widget(accessible) \
	(GTK_ACCESSIBLE (accessible)->widget)
#endif

#if GTK_MAJOR_VERSION == 2 && GTK_MINOR_VERSION == 21 && GTK_MICRO_VERSION == 1
#define gdk_drag_context_get_selected_action(context)  gdk_drag_context_get_action(context)
#endif

#endif /* __GTK_COMPAT_H__ */
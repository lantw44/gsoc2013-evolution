/* Day view widget for gncal
 *
 * Copyright (C) 1998 The Free Software Foundation
 *
 * Author: Federico Mena <federico@nuclecu.unam.mx>
 */

#ifndef GNCAL_DAY_VIEW_H
#define GNCAL_DAY_VIEW_H


#include <gtk/gtkwidget.h>
#include <libgnome/gnome-defs.h>
#include "calendar.h"


BEGIN_GNOME_DECLS


#define GNCAL_DAY_VIEW(obj)         GTK_CHECK_CAST (obj, gncal_day_view_get_type (), GncalDayView)
#define GNCAL_DAY_VIEW_CLASS(klass) GTK_CHECK_CLASS_CAST (klass, gncal_day_view_get_type (), GncalDayViewClass)
#define GNCAL_IS_DAY_VIEW(obj)      GTK_CHECK_TYPE (obj, gncal_day_view_get_type ())


typedef struct _GncalDayView      GncalDayView;
typedef struct _GncalDayViewClass GncalDayViewClass;

struct _GncalDayView {
	GtkWidget widget;

	Calendar *calendar;	/* the calendar we are associated to */

	time_t lower;		/* lower and upper times to display */
	time_t upper;

	char *day_str;          /* what day is it? */
};

struct _GncalDayViewClass {
	GtkWidgetClass parent_class;
};


guint      gncal_day_view_get_type     (void);
GtkWidget *gncal_day_view_new          (Calendar *calendar, time_t lower, time_t upper);

void       gncal_day_view_update       (GncalDayView *dview);
void       gncal_day_view_set_bounds   (GncalDayView *dview, time_t lower, time_t upper);


END_GNOME_DECLS

#endif

/* Week view composite widget for gncal
 *
 * Copyright (C) 1998 The Free Software Foundation
 *
 * Author: Federico Mena <federico@nuclecu.unam.mx>
 */

#include "gncal-week-view.h"


static void gncal_week_view_init (GncalWeekView *wview);


guint
gncal_week_view_get_type (void)
{
	static guint week_view_type = 0;

	if (!week_view_type) {
		GtkTypeInfo week_view_info = {
			"GncalWeekView",
			sizeof (GncalWeekView),
			sizeof (GncalWeekViewClass),
			(GtkClassInitFunc) NULL,
			(GtkObjectInitFunc) gncal_week_view_init,
			(GtkArgSetFunc) NULL,
			(GtkArgGetFunc) NULL
		};

		week_view_type = gtk_type_unique (gtk_table_get_type (), &week_view_info);
	}

	return week_view_type;
}

static void
gncal_week_view_init (GncalWeekView *wview)
{
	int i;

	wview->calendar = NULL;
	memset (&wview->start_of_week, 0, sizeof (wview->start_of_week));

	for (i = 0; i < 7; i++)
		wview->days[i] = NULL;
}

GtkWidget *
gncal_week_view_new (Calendar *calendar, time_t start_of_week)
{
	GncalWeekView *wview;
	int i;
#if 0
	g_return_val_if_fail (calendar != NULL, NULL);
#endif
	wview = gtk_type_new (gncal_week_view_get_type ());

	wview->table.homogeneous = TRUE; /* FIXME: eeeeeeeeeek, GtkTable does not have a function to set this */

	wview->calendar = calendar;

	for (i = 0; i < 7; i++) {
		wview->days[i] = GNCAL_DAY_VIEW (gncal_day_view_new (calendar, 0, 0));

		if (i < 5)
			gtk_table_attach (GTK_TABLE (wview), GTK_WIDGET (wview->days[i]),
					  i, i + 1,
					  0, 1,
					  GTK_EXPAND | GTK_FILL | GTK_SHRINK,
					  GTK_EXPAND | GTK_FILL | GTK_SHRINK,
					  0, 0);
		else
			gtk_table_attach (GTK_TABLE (wview), GTK_WIDGET (wview->days[i]),
					  i - 2, i - 1,
					  1, 2,
					  GTK_EXPAND | GTK_FILL | GTK_SHRINK,
					  GTK_EXPAND | GTK_FILL | GTK_SHRINK,
					  0, 0);

		gtk_widget_show (GTK_WIDGET (wview->days[i]));
	}

	gncal_week_view_set (wview, start_of_week);

	return GTK_WIDGET (wview);
}

static void
update (GncalWeekView *wview, int update_days)
{
	int i;

	if (update_days)
		for (i = 0; i < 7; i++)
			gncal_day_view_update (wview->days[i]);

	/* FIXME: update extra widgets */
}

void
gncal_week_view_update (GncalWeekView *wview)
{
	g_return_if_fail (wview != NULL);
	g_return_if_fail (GNCAL_IS_WEEK_VIEW (wview));

	update (wview, TRUE);
}

void
gncal_week_view_set (GncalWeekView *wview, time_t start_of_week)
{
	struct tm tm;
	time_t day_start, day_end;

	g_return_if_fail (wview != NULL);
	g_return_if_fail (GNCAL_IS_WEEK_VIEW (wview));

	tm = *localtime (&start_of_week);
	
	/* back up to start of week */

	tm.tm_mday -= tm.tm_wday;

	/* Start of day */

	tm.tm_hour = 0;
	tm.tm_min  = 0;
	tm.tm_sec  = 0;

	day_start = mktime (&tm);

	for (i = 0; i < 7; i++) { /* rest of days */
		tm.tm_mday++;
		day_end = mktime (&tm);

		gncal_day_view_set_bounds (days[i], day_start, day_end - 1);

		day_start = day_end;
	}

	update (wview, FALSE);
}

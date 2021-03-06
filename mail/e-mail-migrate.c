/*
 * e-mail-migrate.c
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
 *
 * Copyright (C) 1999-2008 Novell, Inc. (www.novell.com)
 *
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "e-mail-migrate.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <utime.h>
#include <unistd.h>
#include <dirent.h>
#include <regex.h>
#include <errno.h>
#include <ctype.h>

#include <glib/gi18n.h>
#include <glib/gstdio.h>

#include <gtk/gtk.h>

#include <libxml/tree.h>
#include <libxml/parser.h>
#include <libxml/xmlmemory.h>

#include "shell/e-shell.h"
#include "shell/e-shell-migrate.h"

#include "libemail-engine/e-mail-folder-utils.h"

#include "e-mail-backend.h"
#include "em-utils.h"

#define d(x) x

/* 1.4 upgrade functions */

static GtkProgressBar *progress;

static void
em_migrate_set_progress (double percent)
{
	gchar text[5];

	snprintf (text, sizeof (text), "%d%%", (gint) (percent * 100.0f));

	gtk_progress_bar_set_fraction (progress, percent);
	gtk_progress_bar_set_text (progress, text);

	while (gtk_events_pending ())
		gtk_main_iteration ();
}

enum {
	CP_UNIQUE = 0,
	CP_OVERWRITE,
	CP_APPEND
};

static gint open_flags[3] = {
	O_WRONLY | O_CREAT | O_TRUNC,
	O_WRONLY | O_CREAT | O_TRUNC,
	O_WRONLY | O_CREAT | O_APPEND,
};

static gboolean
cp (const gchar *src,
    const gchar *dest,
    gboolean show_progress,
    gint mode)
{
	guchar readbuf[65536];
	gssize nread, nwritten;
	gint errnosav, readfd, writefd;
	gsize total = 0;
	struct stat st;
	struct utimbuf ut;

	/* if the dest file exists and has content, abort - we don't
	 * want to corrupt their existing data */
	if (g_stat (dest, &st) == 0 && st.st_size > 0 && mode == CP_UNIQUE) {
		errno = EEXIST;
		return FALSE;
	}

	if (g_stat (src, &st) == -1
	    || (readfd = g_open (src, O_RDONLY | O_BINARY, 0)) == -1)
		return FALSE;

	if ((writefd = g_open (dest, open_flags[mode] | O_BINARY, 0666)) == -1) {
		errnosav = errno;
		close (readfd);
		errno = errnosav;
		return FALSE;
	}

	do {
		do {
			nread = read (readfd, readbuf, sizeof (readbuf));
		} while (nread == -1 && errno == EINTR);

		if (nread == 0)
			break;
		else if (nread < 0)
			goto exception;

		do {
			nwritten = write (writefd, readbuf, nread);
		} while (nwritten == -1 && errno == EINTR);

		if (nwritten < nread)
			goto exception;

		total += nwritten;
		if (show_progress)
			em_migrate_set_progress (((gdouble) total) / ((gdouble) st.st_size));
	} while (total < st.st_size);

	if (fsync (writefd) == -1)
		goto exception;

	close (readfd);
	if (close (writefd) == -1)
		goto failclose;

	ut.actime = st.st_atime;
	ut.modtime = st.st_mtime;
	utime (dest, &ut);
	chmod (dest, st.st_mode);

	return TRUE;

 exception:

	errnosav = errno;
	close (readfd);
	close (writefd);
	errno = errnosav;

 failclose:

	errnosav = errno;
	unlink (dest);
	errno = errnosav;

	return FALSE;
}

static gboolean
emm_setup_initial (const gchar *data_dir)
{
	GDir *dir;
	const gchar *d;
	gchar *local = NULL, *base;
	const gchar * const *language_names;

	/* special-case - this means brand new install of evolution */
	/* FIXME: create default folders and stuff... */

	d (printf ("Setting up initial mail tree\n"));

	base = g_build_filename (data_dir, "local", NULL);
	if (g_mkdir_with_parents (base, 0700) == -1 && errno != EEXIST) {
		g_free (base);
		return FALSE;
	}

	/* e.g. try en-AU then en, etc */
	language_names = g_get_language_names ();
	while (*language_names != NULL) {
		local = g_build_filename (
			EVOLUTION_PRIVDATADIR, "default",
			*language_names, "mail", "local", NULL);
		if (g_file_test (local, G_FILE_TEST_EXISTS))
			break;
		g_free (local);
		language_names++;
	}

	/* Make sure we found one. */
	g_return_val_if_fail (*language_names != NULL, FALSE);

	dir = g_dir_open (local, 0, NULL);
	if (dir) {
		while ((d = g_dir_read_name (dir))) {
			gchar *src, *dest;

			src = g_build_filename (local, d, NULL);
			dest = g_build_filename (base, d, NULL);

			cp (src, dest, FALSE, CP_UNIQUE);
			g_free (dest);
			g_free (src);
		}
		g_dir_close (dir);
	}

	g_free (local);
	g_free (base);

	return TRUE;
}

static void
em_rename_view_in_folder (gpointer data,
                          gpointer user_data)
{
	const gchar *filename = data;
	const gchar *views_dir = user_data;
	gchar *folderpos, *dotpos;

	g_return_if_fail (filename != NULL);
	g_return_if_fail (views_dir != NULL);

	folderpos = strstr (filename, "-folder:__");
	if (!folderpos)
		folderpos = strstr (filename, "-folder___");
	if (!folderpos)
		return;

	/* points on 'f' from the "folder" word */
	folderpos++;
	dotpos = strrchr (filename, '.');
	if (folderpos < dotpos && g_str_equal (dotpos, ".xml")) {
		GChecksum *checksum;
		gchar *oldname, *newname, *newfile;
		const gchar *md5_string;

		*dotpos = 0;

		/* use MD5 checksum of the folder URI, to not depend on its length */
		checksum = g_checksum_new (G_CHECKSUM_MD5);
		g_checksum_update (checksum, (const guchar *) folderpos, -1);

		*folderpos = 0;
		md5_string = g_checksum_get_string (checksum);
		newfile = g_strconcat (filename, md5_string, ".xml", NULL);
		*folderpos = 'f';
		*dotpos = '.';

		oldname = g_build_filename (views_dir, filename, NULL);
		newname = g_build_filename (views_dir, newfile, NULL);

		g_rename (oldname, newname);

		g_checksum_free (checksum);
		g_free (oldname);
		g_free (newname);
		g_free (newfile);
	}
}

static void
em_rename_folder_views (EShellBackend *shell_backend)
{
	const gchar *config_dir;
	gchar *views_dir;
	GDir *dir;

	g_return_if_fail (shell_backend != NULL);

	config_dir = e_shell_backend_get_config_dir (shell_backend);
	views_dir = g_build_filename (config_dir, "views", NULL);

	dir = g_dir_open (views_dir, 0, NULL);
	if (dir) {
		GSList *to_rename = NULL;
		const gchar *filename;

		while (filename = g_dir_read_name (dir), filename) {
			if (strstr (filename, "-folder:__") ||
			    strstr (filename, "-folder___"))
				to_rename = g_slist_prepend (to_rename, g_strdup (filename));
		}

		g_dir_close (dir);

		g_slist_foreach (to_rename, em_rename_view_in_folder, views_dir);
		g_slist_free_full (to_rename, g_free);
	}

	g_free (views_dir);
}

gboolean
e_mail_migrate (EShellBackend *shell_backend,
                gint major,
                gint minor,
                gint micro,
                GError **error)
{
	const gchar *data_dir;

	data_dir = e_shell_backend_get_data_dir (shell_backend);

	if (major == 0)
		return emm_setup_initial (data_dir);

	if (major <= 2 || (major == 3 && minor < 4))
		em_rename_folder_views (shell_backend);

	return TRUE;
}

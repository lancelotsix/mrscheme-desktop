/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
 * mrscheme-desktop
 * Copyright (C) Lancelot SIX 2012 <lancelot.six@lip6.fr>
 * 
 * mrscheme-desktop is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published
 * by the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * mrscheme-desktop is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU Lesser General Public License for more details.
 * 
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.";
 */

#include "gtk-mr-scheme.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <JavaScriptCore/JSStringRef.h>
#include <JavaScriptCore/JSObjectRef.h>

G_DEFINE_TYPE (GtkMrScheme, gtk_mr_scheme, WEBKIT_TYPE_WEB_VIEW);

char*
escape_chars(char *str)
{
	long nb_a_echaper = 1; // le \0 final
	long curr         = 0;
	char *ret         = NULL;
	long curr_ret     = 0;

	for (curr = 0; str[curr] != '\0'; curr++)
	{
		switch (str[curr])
		{
			case '"':
			case '\n':
			case '\\':
				nb_a_echaper++;
			default:;
		}
	}

	ret = malloc ((strlen(str)+nb_a_echaper)*sizeof(char));

	for (curr = 0; str[curr] != '\0'; curr++)
	{
		switch (str[curr])
		{
			case '\n':
				ret[curr_ret++] = '\\';
				ret[curr_ret++] = 'n';
				break;
			case '"':
			case '\\':
				ret[curr_ret++] =  '\\';
			default:
				ret[curr_ret++] = str[curr];
		}
	}
	// termine par un \0
	ret[curr_ret] = '\0';
	return ret;
}

static void
after_load_web_view_cb(GObject *obj, gpointer data)
{
	GtkMrScheme *mrScheme = GTK_MR_SCHEME (obj);
	webkit_web_view_execute_script(WEBKIT_WEB_VIEW (mrScheme),
	"var elt = document.getElementById('Title'); elt.style.height = '0px'; elt.style.visibility = 'hidden'");
	webkit_web_view_execute_script(WEBKIT_WEB_VIEW (mrScheme),
	"var elt = document.getElementById('menu'); elt.style.height = '0px'; elt.style.visibility = 'hidden'");

}

static void
gtk_mr_scheme_init (GtkMrScheme *gtk_mr_scheme)
{
	webkit_web_view_load_uri (WEBKIT_WEB_VIEW (gtk_mr_scheme), MRECHEME_WEB_URI);
	g_signal_connect(gtk_mr_scheme, "load-finished", G_CALLBACK(after_load_web_view_cb), NULL);
}

static void
gtk_mr_scheme_finalize (GObject *object)
{
	G_OBJECT_CLASS (gtk_mr_scheme_parent_class)->finalize (object);
}

static void
gtk_mr_scheme_class_init (GtkMrSchemeClass *klass)
{
	GObjectClass* object_class = G_OBJECT_CLASS (klass);

	object_class->finalize = gtk_mr_scheme_finalize;
}

GtkWidget*
gtk_mr_scheme_new (void)
{
	return GTK_WIDGET ( g_object_new (GTK_TYPE_MR_SCHEME, NULL));
}

gchar*
gtk_mr_scheme_get_scm_program (GtkMrScheme *mrScheme)
{
	gchar *ret = NULL;
	WebKitWebFrame     *frame = webkit_web_view_get_focused_frame (WEBKIT_WEB_VIEW(mrScheme));
	JSGlobalContextRef ctx    = webkit_web_frame_get_global_context (frame);

	JSStringRef cmdJS = JSStringCreateWithUTF8CString ("return MrScheme.editor.getValue()");
	JSObjectRef fnJS  = JSObjectMakeFunction          (ctx, NULL, 0, NULL, cmdJS, NULL, 1, NULL);
	JSValueRef  val   = JSObjectCallAsFunction        (ctx, fnJS, NULL, 0, NULL, NULL);

	if (JSValueIsString (ctx, val))
	{
		JSStringRef raw_ret = JSValueToStringCopy (ctx,val,NULL);
		size_t      n = JSStringGetMaximumUTF8CStringSize (raw_ret);
		ret = (gchar*) g_malloc (n*sizeof(gchar));
		size_t      read = JSStringGetUTF8CString(raw_ret, ret, n);
		if (read > n)
			fprintf(stderr, "Unble to retreive val\n");
	}

	JSStringRelease (cmdJS);
	
	return ret;
}

void
gtk_mr_scheme_set_scm_program (GtkMrScheme *mrScheme, gchar* scm_prog)
{
	static gchar* head    = "MrScheme.editor.setValue(\"";
	static gchar* foot = "\")";

	char *escaped_pgm = escape_chars (scm_prog);
	guint sz = strlen (head) + strlen (foot) + strlen (escaped_pgm) + 1;
	gchar* cmd = (gchar*) malloc(sz*sizeof(gchar));

	strcpy(cmd, head);
	strcat(cmd, escaped_pgm);
	strcat(cmd, foot);

	webkit_web_view_execute_script(WEBKIT_WEB_VIEW (mrScheme), cmd);

	free (cmd);
	free (escaped_pgm);
}


void
gtk_mr_scheme_execute_program (GtkMrScheme *mrScheme)
{
	static const gchar* executeCmd = "menuExecute(MrScheme.editor)();";
	webkit_web_view_execute_script(WEBKIT_WEB_VIEW (mrScheme), executeCmd);
}
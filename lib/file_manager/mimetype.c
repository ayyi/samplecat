/**
* +----------------------------------------------------------------------+
* | This file is part of the Ayyi project. http://ayyi.org               |
* | copyright (C) 2011-2020 Tim Orford <tim@orford.org>                  |
* +----------------------------------------------------------------------+
* | ROX-Filer, filer for the ROX desktop project, v2.3                   |
* | Copyright (C) 2005, the ROX-Filer team.                              |
* +----------------------------------------------------------------------+
* | This program is free software; you can redistribute it and/or modify |
* | it under the terms of the GNU General Public License version 3       |
* | as published by the Free Software Foundation.                        |
* +----------------------------------------------------------------------+
*
*/
#include "config.h"
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <errno.h>
#include <ctype.h>
#include <time.h>
#include <sys/param.h>
#include <fnmatch.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>

#include <sys/stat.h>
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#include <gtk/gtk.h>
#pragma GCC diagnostic warning "-Wdeprecated-declarations"
#include "debug/debug.h"

#include "fscache.h"
#include "pixmaps.h"
#include "mimetype.h"
#include "diritem.h"
#include "support.h"

#define _g_object_unref0(var) ((var == NULL) ? NULL : (var = (g_object_unref (var), NULL)))

enum {SET_MEDIA, SET_TYPE};

char theme_name[64] = {'\0',};
GtkIconTheme* icon_theme = NULL;

static MIME_type* get_mime_type    (const gchar* type_name, gboolean can_create);
#if 0
#ifdef DEBUG
static void       print_icon_list  ();
#endif
static void      _set_icon_theme   (const char*);
#endif

/* Hash of all allocated MIME types, indexed by "media/subtype".
 * MIME_type structs are never freed; this table prevents memory leaks
 * when rereading the config files.
 */
static GHashTable* type_hash = NULL;

/* Text is the default type */
MIME_type* text_plain;
MIME_type* inode_directory;
MIME_type* inode_mountpoint;
MIME_type* inode_pipe;
MIME_type* inode_socket;
MIME_type* inode_block_dev;
MIME_type* inode_char_dev;
MIME_type* application_executable;
MIME_type* application_octet_stream;
MIME_type* application_x_shellscript;
MIME_type* application_x_desktop;
MIME_type* inode_unknown;
MIME_type* inode_door;


void
type_init (void)
{
	if (icon_theme || type_hash) return;

	type_hash = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, NULL);

	GdkScreen* screen = gdk_screen_get_default ();
	if (!screen) return;

	icon_theme = gtk_icon_theme_get_for_screen (screen);

#ifdef DEBUG
	gint n_elements;
	gchar** path[64];
	gtk_icon_theme_get_search_path(icon_theme, path, &n_elements);
	for(int i=0;i<n_elements;i++){
		dbg(2, "icon_theme_path=%s", path[0][i]);
	}
	g_strfreev(*path);
#endif

	text_plain = get_mime_type("text/plain", TRUE);
	inode_directory = get_mime_type("inode/directory", TRUE);
	inode_mountpoint = get_mime_type("inode/mount-point", TRUE);
	inode_pipe = get_mime_type("inode/fifo", TRUE);
	inode_socket = get_mime_type("inode/socket", TRUE);
	inode_block_dev = get_mime_type("inode/blockdevice", TRUE);
	inode_char_dev = get_mime_type("inode/chardevice", TRUE);
	application_executable = get_mime_type("application/x-executable", TRUE);
	application_octet_stream = get_mime_type("application/octet-stream", TRUE);
	application_x_shellscript = get_mime_type("application/x-shellscript", TRUE);
	application_x_desktop = get_mime_type("application/x-desktop", TRUE);
	application_x_desktop->executable = TRUE;
	inode_unknown = get_mime_type("inode/unknown", TRUE);
	inode_door = get_mime_type("inode/door", TRUE);

	//_set_icon_theme(NULL);
}


GdkPixbuf*
mime_type_get_pixbuf(MIME_type* mime_type)
{
	type_to_icon(mime_type);
	if (!mime_type->image) dbg(0, "no icon.\n");
	return mime_type->image->sm_pixbuf;
}


#if 0
static void
mime_type_on_theme_select(GtkMenuItem* menuitem, gpointer user_data)
{
	g_return_if_fail(menuitem);

	gchar* name = g_object_get_data(G_OBJECT(menuitem), "theme");

#ifdef DEBUG
	if(0) print_icon_list();
#endif
	_set_icon_theme(name);

	//the effects of changing the icon theme are confined to the filemanager only.
	//Changing icons application wide only makes sense if the mime fns are separated from the filemanager.
	dbg(0, "FIXME icons for current directory are not cleared.");
	dbg(0, "FIXME ensure theme selection is initiated by the application and not the file_manager lib.");
	extern void file_manager__update_all();
	file_manager__update_all();
}
#endif


#if 0
gboolean
can_be_executable()
{
	char* type = g_strconcat(type->media_type, "/", type->subtype, NULL);
	gboolean executable = g_content_type_can_be_executable(type);
	g_free(type);
	return executable;
}
#endif


/*
 *  Returns the MIME_type structure for the given type name. It is looked
 *  up in type_hash and returned if found. If not found (and can_create is
 *  TRUE) then a new MIME_type is made, added to type_hash and returned.
 *  NULL is returned if type_name is not in type_hash and can_create is
 *  FALSE, or if type_name does not contain a '/' character.
 */
static MIME_type*
get_mime_type (const gchar* type_name, gboolean can_create)
{
	g_return_val_if_fail(type_name, NULL);

	MIME_type* mtype = g_hash_table_lookup(type_hash, type_name);
	if (mtype || !can_create) return mtype;
	dbg(2, "not found in cache: %s", type_name);

	gchar* slash = strchr(type_name, '/');
	if (slash == NULL) {
		g_warning("MIME type '%s' does not contain a '/' character!", type_name);
		return NULL;
	}

	mtype = g_new(MIME_type, 1);
	mtype->media_type = g_strndup(type_name, slash - type_name);
	mtype->subtype = g_strdup(slash + 1);
	mtype->image = NULL;
	mtype->executable = g_content_type_can_be_executable(type_name);

	g_hash_table_insert(type_hash, g_strdup(type_name), mtype);

	return mtype;
}


static void
append_names (gpointer key, gpointer value, gpointer data)
{
	GList** list = (GList**)data;

	*list = g_list_prepend(*list, key);
}


/* Return list of all mime type names. Caller must free the list
 * but NOT the strings it contains (which are never freed).
 */
GList*
mime_type_name_list (void)
{
	GList* list = NULL;

	g_hash_table_foreach(type_hash, append_names, &list);
	list = g_list_sort(list, (GCompareFunc) strcmp);

	return list;
}


/*			MIME-type guessing 			*/

/* Get the type of this file - stats the file and uses that if
 * possible. For regular or missing files, uses the pathname.
 */
MIME_type*
type_get_type (const guchar* path)
{
	MIME_type* type = NULL;

	DirItem* item = diritem_new((guchar*)"");
	diritem_restat(path, item, NULL);
	if (item->base_type != TYPE_ERROR)
		type = item->mime_type;
	diritem_free(item);

	if (type)
		return type;

	type = type_from_path((char*)path);

	if (!type)
		return text_plain;

	return type;
}


/*
 *  Returns a pointer to the MIME-type.
 *
 *  Tries all enabled methods:
 *  - Look for extended attribute
 *  - If no attribute, check file name
 *  - If no name rule, check contents
 *
 *  NULL if we can't think of anything.
 */
MIME_type*
type_from_path (const char* path)
{
	MIME_type* mtype = NULL;

	gboolean uncertain;
	gchar* type_name = g_content_type_guess (path, NULL, 0, &uncertain);
	if (type_name){
		mtype = get_mime_type(type_name, TRUE);
		g_free(type_name);
	}
	return mtype;
}


/*
 *  Returns the file/dir in Choices for handling this type.
 *  NULL if there isn't one. g_free() the result.
 */
static char*
handler_for (MIME_type* type)
{
	return NULL;
	/*
	char	*type_name;
	char	*open;

	type_name = g_strconcat(type->media_type, "_", type->subtype, NULL);
	open = choices_find_xdg_path_load(type_name, "MIME-types", SITE);
	g_free(type_name);

	if (!open)
		open = choices_find_xdg_path_load(type->media_type, "MIME-types", SITE);

	return open;
	*/
}


MIME_type*
mime_type_lookup (const char* type)
{
	return get_mime_type(type, TRUE);
}


/*			Actions for types 			*/

gboolean
type_open (const char* path, MIME_type* type)
{
	char* open = handler_for(type);
	if (!open)
		return FALSE;

	/*
	gchar *argv[] = {NULL, NULL, NULL};

	argv[1] = (char *) path;

	struct stat	info;
	gboolean	retval;
	if (stat(open, &info))
	{
		report_error("stat(%s): %s", open, g_strerror(errno));
		g_free(open);
		return FALSE;
	}

	if (info.st_mode & S_IWOTH)
	{
		gchar *choices_dir;
		GList *paths;

		report_error(_("Executable '%s' is world-writeable! Refusing "
			"to run. Please change the permissions now (this "
			"problem may have been caused by a bug in earlier "
			"versions of the filer).\n\n"
			"Having (non-symlink) run actions world-writeable "
			"means that other people who use your computer can "
			"replace your run actions with malicious versions.\n\n"
			"If you trust everyone who could write to these files "
			"then you needn't worry. Otherwise, you should check, "
			"or even just delete, all the existing run actions."),
			open);
		choices_dir = g_path_get_dirname(open);
		paths = g_list_append(NULL, choices_dir);
		action_chmod(paths, TRUE, _("go-w (Fix security problem)"));
		g_free(choices_dir);
		g_list_free(paths);
		g_free(open);
		return TRUE;
	}

	if (S_ISDIR(info.st_mode))
		argv[0] = g_strconcat(open, "/AppRun", NULL);
	else
		argv[0] = open;

	retval = rox_spawn(home_dir, (const gchar **) argv) != 0;

	if (argv[0] != open)
		g_free(argv[0]);

	g_free(open);
	
	return retval;
	*/
	return FALSE;
}

/*
 *  Return the image for this type, loading it if needed.
 *
 *  Note: You must g_object_unref() the image afterwards.
 */
MaskedPixmap*
type_to_icon (MIME_type* type)
{
	if (!type){	g_object_ref(im_unknown); return im_unknown; }

	time_t now = time(NULL);
	// already got an image?
	if (type->image) {
		// Yes - don't recheck too often
		if (abs(now - type->image_time) < 2)
			return g_object_ref(type->image);
		_g_object_unref0(type->image);
	}

	if (type->image) goto out;

	char* type_name = g_strconcat(type->media_type, "/", type->subtype, NULL);
	GIcon* icon = g_content_type_get_icon (type_name);
					if(!strcmp(type_name, "inode/directory")){
						g_themed_icon_prepend_name(G_THEMED_ICON(icon), "folder");
					}
					else if(!strcmp(type_name, "inode/directory-open")){
						g_themed_icon_prepend_name(G_THEMED_ICON(icon), "folder-open");
					}
	GtkIconInfo* info = gtk_icon_theme_lookup_by_gicon((*theme_name) ? icon_theme : gtk_icon_theme_get_default(), icon, 16, GTK_ICON_LOOKUP_FORCE_SIZE);

	dbg(2, "theme=%s typename=%-20s gicon=%p info=%p", theme_name, type_name, icon, info);

	g_object_unref(icon);
	g_free(type_name);

	if (info) {
		/* Get the actual icon through our cache, not through GTK, because
		 * GTK doesn't cache icons.
		 */
		const char* icon_path = gtk_icon_info_get_filename(info);
		if (icon_path) type->image = g_fscache_lookup(pixmap_cache, icon_path);
		//if (icon_path) type->image = masked_pixmap_new(full);
		/* else shouldn't happen, because we didn't use
		 * GTK_ICON_LOOKUP_USE_BUILTIN.
		 */
		gtk_icon_info_free(info);
	}

out:
	if (!type->image) {
		dbg(2, "%s/%s failed! using im_unknown.", type->media_type, type->subtype);
		/* One ref from the type structure, one returned */
		type->image = im_unknown;
		g_object_ref(im_unknown);
	}

	type->image_time = now;
	
	g_object_ref(type->image);
	return type->image;
}


GdkAtom
type_to_atom (MIME_type* type)
{
	g_return_val_if_fail(type, GDK_NONE);

	char* str = g_strconcat(type->media_type, "/", type->subtype, NULL);
	GdkAtom retval = gdk_atom_intern(str, FALSE);
	g_free(str);
	
	return retval;
}


/* Called if the user clicks on the OK button. Returns FALSE if an error
 * was displayed instead of performing the action.
 */
#ifdef NEVER
static gboolean
set_shell_action (GtkWidget* dialog)
{
	gchar *tmp, *path;
	int	error = 0, len;
	int	fd;

	GtkEntry* entry = g_object_get_data(G_OBJECT(dialog), "shell_command");

	g_return_val_if_fail(entry, FALSE);

	const guchar* command = gtk_entry_get_text(entry);
	
	if (!strchr(command, '$'))
	{
		show_shell_help(NULL);
		return FALSE;
	}

	path = NULL;//get_action_save_path(dialog);
	if (!path)
		return FALSE;
		
	tmp = g_strdup_printf("#! /bin/sh\nexec %s\n", command);
	len = strlen(tmp);
	
	fd = open(path, O_CREAT | O_WRONLY, 0755);
	if (fd == -1)
		error = errno;
	else
	{
		FILE *file;

		file = fdopen(fd, "w");
		if (file)
		{
			if (fwrite(tmp, 1, len, file) < len)
				error = errno;
			if (fclose(file) && error == 0)
				error = errno;
		}
		else
			error = errno;
	}

	if (error)
		//report_error(g_strerror(error));
		errprintf("%s", g_strerror(error));

	g_free(tmp);
	g_free(path);

	gtk_widget_destroy(dialog);

	return TRUE;
}
#endif

/*
static void set_action_response(GtkWidget *dialog, gint response, gpointer data)
{
	if (response == GTK_RESPONSE_OK)
		if (!set_shell_action(dialog))
			return;
	gtk_widget_destroy(dialog);
}
*/

/* Return the path of the file in choices that handles this type and
 * radio setting.
 * NULL if nothing is defined for it.
 */
	/*
static guchar*
handler_for_radios (GObject* dialog)
{
	Radios* radios = g_object_get_data(G_OBJECT(dialog), "rox-radios");
	MIME_type* type = g_object_get_data(G_OBJECT(dialog), "mime_type");
	
	g_return_val_if_fail(radios != NULL, NULL);
	g_return_val_if_fail(type != NULL, NULL);
	
	switch (radios_get_value(radios))
	{
		case SET_MEDIA:
			return choices_find_xdg_path_load(type->media_type, "MIME-types", SITE);
		case SET_TYPE:
		{
			gchar *tmp, *handler;
			tmp = g_strconcat(type->media_type, "_",
					  type->subtype, NULL);
			handler = choices_find_xdg_path_load(tmp, "MIME-types", SITE);
			g_free(tmp);
			return handler;
		}
		default:
			g_warning("Bad type");
			return NULL;
	}
	return NULL;
}
	*/

/* (radios can be NULL if called from clear_run_action) */
	/*
static void run_action_update(Radios *radios, gpointer data)
{
	guchar *handler;
	DropBox *drop_box;
	GObject *dialog = G_OBJECT(data);

	drop_box = g_object_get_data(dialog, "rox-dropbox");

	g_return_if_fail(drop_box != NULL);

	handler = handler_for_radios(dialog);

	if (handler)
	{
		char *old = handler;

		handler = readlink_dup(old);
		if (handler)
			g_free(old);
		else
			handler = old;
	}

	drop_box_set_path(DROP_BOX(drop_box), handler);
	g_free(handler);
}
	*/

	/*
static void clear_run_action(GtkWidget *drop_box, GtkWidget *dialog)
{
	guchar *handler;

	handler = handler_for_radios(G_OBJECT(dialog));

	if (handler)
		remove_handler_with_confirm(handler);

	run_action_update(NULL, dialog);
}
	*/


/* Takes the st_mode field from stat() and returns the base type.
 * Should not be a symlink.
 */
int mode_to_base_type(int st_mode)
{
	return TYPE_FILE;
	/*
	if (S_ISREG(st_mode))
		return TYPE_FILE;
	else if (S_ISDIR(st_mode))
		return TYPE_DIRECTORY;
	else if (S_ISBLK(st_mode))
		return TYPE_BLOCK_DEVICE;
	else if (S_ISCHR(st_mode))
		return TYPE_CHAR_DEVICE;
	else if (S_ISFIFO(st_mode))
		return TYPE_PIPE;
	else if (S_ISSOCK(st_mode))
		return TYPE_SOCKET;
	else if (S_ISDOOR(st_mode))
		return TYPE_DOOR;

	return TYPE_ERROR;
	*/
}


#ifdef NOT_USED
static char**
get_xdg_data_dirs (int *n_dirs)
{
	const char *env;
	char **dirs;
	int i, n;

	env = getenv("XDG_DATA_DIRS");
	if (!env)
		env = "/usr/local/share/:/usr/share/";
	dirs = g_strsplit(env, ":", 0);
	g_return_val_if_fail(dirs != NULL, NULL);
	for (n = 0; dirs[n]; n++)
		;
	for (i = n; i > 0; i--)
		dirs[i] = dirs[i - 1];
	env = getenv("XDG_DATA_HOME");
	if (env)
		dirs[0] = g_strdup(env);
	else
		dirs[0] = g_build_filename(g_get_home_dir(), ".local",
					   "share", NULL);
	*n_dirs = n + 1;
	return dirs;
}
#endif


#if 0
#ifdef DEBUG
static void
print_icon_list()
{
	GList* contexts = gtk_icon_theme_list_contexts(icon_theme);
	GList* l = contexts;
	for(;l;l=l->next){
		dbg(0, "  %s", l->data);
		g_free(l->data);
	}
	g_list_free(contexts);

	GList* icon_list = gtk_icon_theme_list_icons(icon_theme, "MimeTypes");
	if(icon_list){
		GList* l = icon_list;
		dbg(0, "%s----------------------------------", theme_name);
		for(;l;l=l->next){
			char* icon = l->data;
			printf("%s\n", icon);
			g_free(icon);
		}
		g_list_free(icon_list);
		printf("-------------------------------------------------\n");
	}
	else warnprintf("icon_theme has no mimetype icons?\n");

}
#endif


/*static*/ void
_set_icon_theme(const char* name)
{
	g_hash_table_remove_all(type_hash);

	dbg(2, "setting theme: %s.", name);
	if(name && name[0]){
		if(!*theme_name) icon_theme = gtk_icon_theme_new();
		g_strlcpy(theme_name, name, 64);
		gtk_icon_theme_set_custom_theme(icon_theme, theme_name);
	}

#if 0
	// this test is disabled as its not reliable
	while (1)
	{
		GtkIconInfo* info = gtk_icon_theme_lookup_icon(icon_theme, "mime-application:postscript", ICON_HEIGHT, 0);
		if (!info)
		{
			info = gtk_icon_theme_lookup_icon(icon_theme, "gnome-mime-application-postscript", ICON_HEIGHT, 0);
		}
		if (info)
		{
			dbg(0, "got test icon ok. Using theme '%s'", theme_name);
			print_icon_list();
			gtk_icon_info_free(info);
			return;
		}

		if (strcmp(theme_name, "ROX") == 0) break;

		warnprintf("Icon theme '%s' does not contain MIME icons. Using ROX default theme instead.\n", theme_name);

		//theme_name = "ROX";
		strcpy(theme_name, "ROX");
	}
#endif

#if 0
	//const char *home_dir = g_get_home_dir();
	//char *icon_home;
	icon_home = g_build_filename(home_dir, ".icons", NULL);
	if (!file_exists(icon_home)) mkdir(icon_home, 0755);
	g_free(icon_home);
#endif

	//icon_home = g_build_filename(home_dir, ".icons", "ROX", NULL);
	//if (symlink(make_path(app_dir, "ROX"), icon_home))
	//	errprintf("Failed to create symlink '%s':\n%s\n\n"
	//	"(this may mean that the ROX theme already exists there, but "
	//	"the 'mime-application:postscript' icon couldn't be loaded for "
	//	"some reason)", icon_home, g_strerror(errno));
	//g_free(icon_home);

	gtk_icon_theme_rescan_if_needed(icon_theme);
}
#endif


void
mime_type_clear ()
{
	g_hash_table_remove_all(type_hash);
}



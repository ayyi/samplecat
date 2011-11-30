/*
 * $Id: type.c,v 1.169 2005/07/24 10:19:31 tal197 Exp $
 *
 * ROX-Filer, filer for the ROX desktop project, v2.3
 * Copyright (C) 2005, the ROX-Filer team.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation; either version 2 of the License, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program; if not, write to the Free Software Foundation, Inc., 59 Temple
 * Place, Suite 330, Boston, MA  02111-1307  USA
 */


/* type.c - code for dealing with filetypes */

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

#include <sys/stat.h>
#include <gtk/gtk.h>

//this gnome-vfs stuff seems to require bonobo!
#ifdef WITH_GNOMEVFS
# include <libgnomevfs/gnome-vfs.h>
# include <libgnomevfs/gnome-vfs-mime.h>
# include <libgnomevfs/gnome-vfs-mime-handlers.h>
# include <libgnomevfs/gnome-vfs-application-registry.h>
#endif
#include "string.h"

#include "file_manager/file_manager.h"
#include "rox_global.h"

#include "utils/fscache.h"
#include "utils/pixmaps.h"
#include "mimetype.h"
#include "rox_support.h"
#include "diritem.h"
#include "xdgmime.h"

#include "utils/ayyi_utils.h"
#include "utils/mime_type.h"
#include "observer.h"
extern unsigned debug;

#define TYPE_NS "http://www.freedesktop.org/standards/shared-mime-info"
enum {SET_MEDIA, SET_TYPE};

char theme_name[64] = "Amaranth";


/* Static prototypes */
//static void alloc_type_colours(void);
//static void options_changed(void);
//static char *get_action_save_path(GtkWidget *dialog);
static MIME_type *get_mime_type(const gchar *type_name, gboolean can_create);
//static gboolean remove_handler_with_confirm(const guchar *path);
/*static*/ void _set_icon_theme(void);
static GList *build_icon_theme(/*Option *option, xmlNode *node, */guchar *label);
static void print_icon_list();

/* Hash of all allocated MIME types, indexed by "media/subtype".
 * MIME_type structs are never freed; this table prevents memory leaks
 * when rereading the config files.
 */
static GHashTable *type_hash = NULL;

/* Most things on Unix are text files, so this is the default type */
MIME_type *text_plain;
MIME_type *inode_directory;
MIME_type *inode_mountpoint;
MIME_type *inode_pipe;
MIME_type *inode_socket;
MIME_type *inode_block_dev;
MIME_type *inode_char_dev;
MIME_type *application_executable;
MIME_type *application_octet_stream;
MIME_type *application_x_shellscript;
MIME_type *application_x_desktop;
MIME_type *inode_unknown;
MIME_type *inode_door;

//static Option o_display_colour_types;
//static Option o_icon_theme;

extern GtkIconTheme *icon_theme;
GList* themes = NULL; 


void type_init(void)
{
	icon_theme = gtk_icon_theme_new();
	//icon_theme = gtk_icon_theme_get_default();

	gint n_elements;
	gchar** path[64];
	gtk_icon_theme_get_search_path(icon_theme, path, &n_elements);
	//printf("type_init(): &path=%p path=%p path[0]=%p *path[0]=%p path1[1]=%s\n", &path, path, path[0], *path[0], path[0][1]);
	//printf("type_init(): count=%i path: %s\n", n_elements, *path[0]);
	int i;
	for(i=0;i<n_elements;i++){
		dbg(2, "icon_theme_path=%s", path[0][i]);
	}
	g_strfreev(*path);

	type_hash = g_hash_table_new(g_str_hash, g_str_equal);

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

	//option_add_string(&o_icon_theme, "icon_theme", "ROX");
	//option_add_int(&o_display_colour_types, "display_colour_types", TRUE);
	//option_register_widget("icon-theme-chooser", build_icon_theme);
	
	//int i;
	//for (i = 0; i < NUM_TYPE_COLOURS; i++)
	//	option_add_string(&o_type_colours[i],
	//			  opt_type_colours[i][0],
	//			  opt_type_colours[i][1]);
	//alloc_type_colours();

	_set_icon_theme();

	//option_add_notify(options_changed);

	guchar* label = g_new0(guchar, 64);
	themes = build_icon_theme(label);
}


static void
on_theme_select(GtkMenuItem* menuitem, gpointer user_data)
{
	g_return_if_fail(menuitem);

	gchar* name = g_object_get_data(G_OBJECT(menuitem), "theme");
	dbg(0, "theme=%s", name);

	if(name && strlen(name)) strncpy(theme_name, name, 64);

	print_icon_list();
	_set_icon_theme();
	observer__icon_theme(); //TODO replace this with a signal or callback
}

/* Read-load all the glob patterns.
 * Note: calls filer_update_all.
 */
void
reread_mime_files(void)
{
	//gtk_icon_theme_rescan_if_needed(icon_theme);

	xdg_mime_shutdown();
}

/* Returns the MIME_type structure for the given type name. It is looked
 * up in type_hash and returned if found. If not found (and can_create is
 * TRUE) then a new MIME_type is made, added to type_hash and returned.
 * NULL is returned if type_name is not in type_hash and can_create is
 * FALSE, or if type_name does not contain a '/' character.
 */
static MIME_type*
get_mime_type(const gchar *type_name, gboolean can_create)
{
	if(!type_name){ errprintf("%s(): bad arg: type_name NULL\n", __func__); return NULL; }
	gchar *slash;

	MIME_type* mtype = g_hash_table_lookup(type_hash, type_name);
	if (mtype || !can_create) return mtype;
	dbg(2, "not found in cache: %s", type_name);

	slash = strchr(type_name, '/');
	if (slash == NULL) {
		g_warning("MIME type '%s' does not contain a '/' character!", type_name);
		return NULL;
	}

	mtype = g_new(MIME_type, 1);
	mtype->media_type = g_strndup(type_name, slash - type_name);
	mtype->subtype = g_strdup(slash + 1);
	mtype->image = NULL;
	mtype->comment = NULL;

	mtype->executable = xdg_mime_mime_type_subclass(type_name, "application/x-executable");

	g_hash_table_insert(type_hash, g_strdup(type_name), mtype);

	return mtype;
}

	/*
const char *basetype_name(DirItem *item)
{
	return _("File");
	if (item->flags & ITEM_FLAG_SYMLINK)
		return _("Sym link");
	else if (item->flags & ITEM_FLAG_MOUNT_POINT)
		return _("Mount point");

	switch (item->base_type)
	{
		case TYPE_FILE:
			return _("File");
		case TYPE_DIRECTORY:
			return _("Dir");
		case TYPE_CHAR_DEVICE:
			return _("Char dev");
		case TYPE_BLOCK_DEVICE:
			return _("Block dev");
		case TYPE_PIPE:
			return _("Pipe");
		case TYPE_SOCKET:
			return _("Socket");
		case TYPE_DOOR:
			return _("Door");
	}
	
	return _("Unknown");
}
	*/

static void append_names(gpointer key, gpointer value, gpointer udata)
{
	GList **list = (GList **) udata;

	*list = g_list_prepend(*list, key);
}

/* Return list of all mime type names. Caller must free the list
 * but NOT the strings it contains (which are never freed).
 */
GList *mime_type_name_list(void)
{
	GList *list = NULL;

	g_hash_table_foreach(type_hash, append_names, &list);
	list = g_list_sort(list, (GCompareFunc) strcmp);

	return list;
}

/*			MIME-type guessing 			*/

/* Get the type of this file - stats the file and uses that if
 * possible. For regular or missing files, uses the pathname.
 */
MIME_type *type_get_type(const guchar *path)
{
	DirItem		*item;
	MIME_type	*type = NULL;

	item = diritem_new((guchar*)"");
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

/* Returns a pointer to the MIME-type.
 *
 * Tries all enabled methods:
 * - Look for extended attribute
 * - If no attribute, check file name
 * - If no name rule, check contents
 *
 * NULL if we can't think of anything.
 */
MIME_type*
type_from_path(const char *path)
{
	if(debug > 1) printf("type_from_path()...\n");

	/* Check for extended attribute first */
	/*
	MIME_type *mime_type = NULL;
	mime_type = xtype_get(path);
	if (mime_type) return mime_type;
	*/

	/* Try name and contents next */
	const char* type_name = xdg_mime_get_mime_type_for_file(path, NULL);
	if (type_name) return get_mime_type(type_name, TRUE);

	return NULL;
}

/* Returns the file/dir in Choices for handling this type.
 * NULL if there isn't one. g_free() the result.
 */
static char *handler_for(MIME_type *type)
{
	return NULL;
	/*
	char	*type_name;
	char	*open;

	type_name = g_strconcat(type->media_type, "_", type->subtype, NULL);
	open = choices_find_xdg_path_load(type_name, "MIME-types", SITE);
	g_free(type_name);

	if (!open)
		open = choices_find_xdg_path_load(type->media_type,
						  "MIME-types", SITE);

	return open;
	*/
}

MIME_type*
mime_type_lookup(const char *type)
{
	return get_mime_type(type, TRUE);
}

/*			Actions for types 			*/

gboolean type_open(const char *path, MIME_type *type)
{
	gchar *argv[] = {NULL, NULL, NULL};
	char		*open;

	argv[1] = (char *) path;

	open = handler_for(type);
	if (!open)
		return FALSE;

	/*
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

GdkAtom type_to_atom(MIME_type *type)
{
	char	*str;
	GdkAtom	retval;
	
	g_return_val_if_fail(type != NULL, GDK_NONE);

	str = g_strconcat(type->media_type, "/", type->subtype, NULL);
	retval = gdk_atom_intern(str, FALSE);
	g_free(str);
	
	return retval;
}

/* Called if the user clicks on the OK button. Returns FALSE if an error
 * was displayed instead of performing the action.
 */
#ifdef NEVER
static gboolean set_shell_action(GtkWidget *dialog)
{
	GtkEntry *entry;
	const guchar *command;
	gchar	*tmp, *path;
	int	error = 0, len;
	int	fd;

	entry = g_object_get_data(G_OBJECT(dialog), "shell_command");

	g_return_val_if_fail(entry != NULL, FALSE);

	command = gtk_entry_get_text(entry);
	
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
static guchar *handler_for_radios(GObject *dialog)
{
	Radios	*radios;
	MIME_type *type;

	radios = g_object_get_data(G_OBJECT(dialog), "rox-radios");
	type = g_object_get_data(G_OBJECT(dialog), "mime_type");
	
	g_return_val_if_fail(radios != NULL, NULL);
	g_return_val_if_fail(type != NULL, NULL);
	
	switch (radios_get_value(radios))
	{
		case SET_MEDIA:
			return choices_find_xdg_path_load(type->media_type,
							  "MIME-types", SITE);
		case SET_TYPE:
		{
			gchar *tmp, *handler;
			tmp = g_strconcat(type->media_type, "_",
					  type->subtype, NULL);
			handler = choices_find_xdg_path_load(tmp,
							     "MIME-types",
							     SITE);
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

/* Called when a URI list is dropped onto the box in the Set Run Action
 * dialog. Make sure it's an application, and make that the default
 * handler.
 */
	/*
static void drag_app_dropped(GtkWidget	*drop_box,
			     const guchar *app,
			     GtkWidget	*dialog)
{
	DirItem	*item;

	item = diritem_new("");
	diritem_restat(app, item, NULL);
	if (item->flags & EXECUTABLE_FILE(item))
	{
		guchar	*path;

		path = get_action_save_path(dialog);

		if (path)
		{
			if (symlink(app, path))
				delayed_error("symlink: %s",
						g_strerror(errno));
			else
				destroy_on_idle(dialog);

			g_free(path);
		}
	}
	else
		delayed_error(
			_("This is not a program! Give me an application "
			"instead!"));

	diritem_free(item);
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
static void expire_timer(gpointer key, gpointer value, gpointer data)
{
	MIME_type *type = value;

	type->image_time = 0;
}
#endif

/* Return a pointer to a (static) colour for this item. If colouring is
 * off, returns normal.
 */
GdkColor *type_get_colour(DirItem *item, GdkColor *normal)
{
	/*
	int type = item->base_type;

	if (!o_display_colour_types.int_value)
		return normal;

	if (EXECUTABLE_FILE(item))
		type = TYPE_EXEC;

	g_return_val_if_fail(type >= 0 && type < NUM_TYPE_COLOURS, normal);

	return &type_colours[type];
	*/
	return normal;
}

#ifdef NOT_USED
static char **get_xdg_data_dirs(int *n_dirs)
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

/* Try to fill in 'type->comment' from this document */
	/*
static void get_comment(MIME_type *type, const guchar *path)
{
	xmlNode *node;
	XMLwrapper *doc;
	
	doc = xml_cache_load(path);
	if (!doc)
		return;

	node = xml_get_section(doc, TYPE_NS, "comment");

	if (node)
	{
		char *val;
		g_return_if_fail(type->comment == NULL);
		val= xmlNodeListGetString(node->doc, node->xmlChildrenNode, 1);
		type->comment = g_strdup(val);
		xmlFree(val);
	}

	g_object_unref(doc);
	return;
}
	*/

const char*
mime_type_comment(MIME_type* type)
{
	if(!type){ errprintf("%s(): @type is NULL\n", __func__); return NULL; } 
	//if (!type->comment) find_comment(type);
	return type->comment;
}

#ifdef NOT_USED
static guchar *read_theme(Option *option)
{
	GtkOptionMenu *om = GTK_OPTION_MENU(option->widget);
	GtkLabel *item;

	item = GTK_LABEL(GTK_BIN(om)->child);

	g_return_val_if_fail(item != NULL, g_strdup("ROX"));

	return g_strdup(gtk_label_get_text(item));
}
#endif

static void
add_themes_from_dir(GPtrArray *names, const char *dir)
{
	GPtrArray *list;
	int i;

	if (access(dir, F_OK) != 0)	return;

	list = list_dir((guchar*)dir);
	g_return_if_fail(list != NULL);

	for (i = 0; i < list->len; i++){
		char *index_path;

		index_path = g_build_filename(dir, list->pdata[i], "index.theme", NULL);
		
		if (access(index_path, F_OK) == 0){
			g_ptr_array_add(names, list->pdata[i]);
		}
		else g_free(list->pdata[i]);

		g_free(index_path);
	}

	g_ptr_array_free(list, TRUE);
}


static GList*
build_icon_theme(/*Option *option, xmlNode *node, */guchar *label)
{
	//-appears to build a menu list of availabe themes.

	GtkWidget *menu;
	GPtrArray *names;
	gchar **theme_dirs = NULL;
	gint n_dirs = 0;
	int i;

	//g_return_val_if_fail(option != NULL, NULL);
	g_return_val_if_fail(label != NULL, NULL);

	//hbox = gtk_hbox_new(FALSE, 4);

	//gtk_box_pack_start(GTK_BOX(hbox), gtk_label_new(label),	FALSE, TRUE, 0);

	//button = gtk_option_menu_new();
	//gtk_box_pack_start(GTK_BOX(hbox), button, TRUE, TRUE, 0);

	menu = gtk_menu_new();
	//gtk_option_menu_set_menu(GTK_OPTION_MENU(button), menu);

	gtk_icon_theme_get_search_path(icon_theme, &theme_dirs, &n_dirs);
	names = g_ptr_array_new();
	for (i = 0; i < n_dirs; i++) add_themes_from_dir(names, theme_dirs[i]);
	g_strfreev(theme_dirs);

	g_ptr_array_sort(names, strcmp2);

	for (i = 0; i < names->len; i++) {
		char *name = names->pdata[i];
		dbg(2, "name=%s", name);

		GtkWidget* item = gtk_menu_item_new_with_label(name);
		gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
		gtk_widget_show_all(item);

		g_object_set_data(G_OBJECT(item), "theme", g_strdup(name)); //make sure this string is free'd when menu is updated.

		g_signal_connect(G_OBJECT(item), "activate", G_CALLBACK(on_theme_select), NULL);

		g_free(name);
	}

	g_ptr_array_free(names, TRUE);

	return g_list_append(NULL, menu);
}


static void
print_icon_list()
{
	GList* icon_list = gtk_icon_theme_list_icons(icon_theme, "MimeTypes");
	if(icon_list){
		dbg(0, "%s----------------------------------", theme_name);
		for(;icon_list;icon_list=icon_list->next){
			char* icon = icon_list->data;
			printf("%s\n", icon);
			g_free(icon);
		}
		g_list_free(icon_list);
		printf("-------------------------------------------------\n");
	}
	else warnprintf("icon_theme has no mimetype icons?\n");

}


/*static */void
_set_icon_theme()
{
	//const char *home_dir = g_get_home_dir();
	GtkIconInfo *info;
	//char *icon_home;

	//if (!theme_name || !*theme_name) theme_name = "ROX";
	if (/*!theme_name ||*/ !*theme_name) strcpy(theme_name, "ROX");

	g_hash_table_remove_all(type_hash);

	while (1)
	{
		dbg(1, "setting theme: %s.", theme_name);
		gtk_icon_theme_set_custom_theme(icon_theme, theme_name);
		return;

		//the test below is disabled as its not reliable

		info = gtk_icon_theme_lookup_icon(icon_theme, "mime-application:postscript", ICON_HEIGHT, 0);
		if (!info)
		{
			//printf("set_icon_theme(): looking up test icon...\n");
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

#if 0
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


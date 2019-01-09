/*
 * ROX-Filer, filer for the ROX desktop project
 * Copyright (C) 2006, Thomas Leonard and others (see changelog for details).
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

/* minibuffer.c - for handling the path entry box at the bottom */

#include "config.h"

#include <fnmatch.h>
#include <string.h>
#include <errno.h>
#include <ctype.h>
#include <glob.h>
#include <stdio.h>
#include <sys/types.h>
#include <pwd.h>
#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>
#include <debug/debug.h>

#include "filemanager.h"
#include "support.h"
#include "minibuffer.h"
#include "display.h"
#include "diritem.h"

#ifdef SHELL
static GList* shell_history = NULL;
#endif

static gint         mini_key_press_event (GtkWidget*, GdkEventKey*, AyyiFilemanager*);
static void         changed              (GtkEditable*, AyyiFilemanager*);
static gboolean     find_next_match      (AyyiFilemanager*, const char* pattern, int dir);
static gboolean     find_exact_match     (AyyiFilemanager*, const gchar* pattern);
static gboolean     matches              (ViewIter*, const char* pattern);
static void         search_in_dir        (AyyiFilemanager*, int dir);
static const gchar* mini_contents        (AyyiFilemanager*);
static gboolean     grab_focus           (GtkWidget*);
static gboolean     select_if_glob       (ViewIter*, gpointer data);

/****************************************************************
 *			EXTERNAL INTERFACE			*
 ****************************************************************/

void
minibuffer_init (void)
{
}


/*
 * Create the minibuffer widgets, setting the appropriate fields
 * in filer_window.
 */
void
create_minibuffer (AyyiFilemanager* fm)
{
	GtkWidget* hbox = gtk_hbox_new(FALSE, 0);
	gtk_widget_set_no_show_all(hbox, TRUE);
	
	//gtk_box_pack_start(GTK_BOX(hbox), new_help_button((HelpFunc) show_help, filer_window), FALSE, TRUE, 0);

	GtkWidget* label = gtk_label_new("");
	gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, TRUE, 2);

	GtkWidget* mini = gtk_entry_new();
	gtk_box_pack_start(GTK_BOX(hbox), mini, TRUE, TRUE, 0);
	gtk_widget_set_name(mini, "fixed-style");
	g_signal_connect(mini, "key_press_event", G_CALLBACK(mini_key_press_event), fm);
	g_signal_connect(mini, "changed", G_CALLBACK(changed), fm);

	// Grabbing focus musn't select the text...
	g_signal_connect_swapped(mini, "grab-focus", G_CALLBACK(grab_focus), mini);

	fm->mini = (Mini){
		.entry = mini,
		.label = label,
		.area = hbox
	};
}


void
minibuffer_show(AyyiFilemanager* fm, MiniType mini_type)
{
	int pos = -1;
	ViewIter cursor;
	
	g_return_if_fail(fm);
	g_return_if_fail(fm->mini.entry);

	GtkEntry* mini = GTK_ENTRY(fm->mini.entry);
#if 0
	entry_set_error(filer_window->minibuffer, FALSE);
#endif

	fm->mini.type = MINI_NONE;
	gtk_label_set_text(GTK_LABEL(fm->mini.label),
			mini_type == MINI_PATH ? "Goto:" :
			mini_type == MINI_SHELL ? "Shell:" :
#ifdef SELECT
			mini_type == MINI_SELECT_IF ? "Select If:" :
#endif
			mini_type == MINI_SELECT_BY_NAME ? "Select Named:" :
			mini_type == MINI_FILTER ? "Pattern:" :
			"?");

	switch (mini_type) {
		case MINI_PATH:
			view_show_cursor(fm->view);
			view_get_cursor(fm->view, &cursor);
			view_set_base(fm->view, &cursor);

			gtk_entry_set_text(mini, (gchar*)make_path(fm->sym_path, ""));
			if (fm->temp_show_hidden) {
				fm->temp_show_hidden = FALSE;
				display_update_hidden(fm);
			}
			break;
#ifdef SELECT
		case MINI_SELECT_IF:
			gtk_entry_set_text(mini, "");
			fm->mini.cursor_base = -1;	// History
			break;
#endif
		case MINI_SELECT_BY_NAME:
			view_show_cursor(fm->view);
			view_get_cursor(fm->view, &cursor);
			view_set_base(fm->view, &cursor);

			gtk_entry_set_text(mini, "*.");
			fm->mini.cursor_base = -1;	// History
			view_select_if(fm->view, select_if_glob, "*.");
			break;
		case MINI_FILTER:
			if(fm->filter != FILER_SHOW_GLOB || !fm->filter_string)
				gtk_entry_set_text(mini, "*");
			else
				gtk_entry_set_text(mini, fm->filter_string);
			break;
		case MINI_SHELL:
		{
			view_get_cursor(fm->view, &cursor);
			DirItem* item = cursor.peek(&cursor);
			pos = 0;
			if (view_count_selected(fm->view) > 0)
				gtk_entry_set_text(mini, " \"$@\"");
			else if (item)
			{
				guchar* escaped = shell_escape((guchar*)item->leafname);
				gchar* tmp = g_strconcat(" ", escaped, NULL);
				g_free(escaped);
				gtk_entry_set_text(mini, tmp);
				g_free(tmp);
			}
			else
				gtk_entry_set_text(mini, "");
			fm->mini.cursor_base = -1;	// History
			break;
		}
		default:
			g_warning("Bad minibuffer type\n");
			return;
	}
	
	fm->mini.type = mini_type;

	gtk_editable_set_position(GTK_EDITABLE(mini), pos);

	gtk_widget_set_no_show_all(fm->mini.area, FALSE);
	gtk_widget_show_all(fm->mini.area);

	gtk_widget_grab_focus(fm->mini.entry);
}


void
minibuffer_hide (AyyiFilemanager* fm)
{
	fm->mini.type = MINI_NONE;

	gtk_widget_hide(fm->mini.area);

	gtk_widget_child_focus(fm->window, GTK_DIR_TAB_FORWARD);

	if (fm->temp_show_hidden) {
		ViewIter iter;

		view_get_cursor(fm->view, &iter);
		DirItem* item = iter.peek(&iter);

		if (item == NULL || item->leafname[0] != '.'){
	        display_update_hidden(fm);
		}
		fm->temp_show_hidden = FALSE;
	}
}


/* Insert this leafname at the cursor (replacing the selection, if any).
 * Must be in SHELL mode.
 */
void
minibuffer_add (AyyiFilemanager* fm, const gchar* leafname)
{
	GtkEditable* edit = GTK_EDITABLE(fm->mini.entry);
	GtkEntry* entry = GTK_ENTRY(edit);

	g_return_if_fail(fm->mini.type == MINI_SHELL);

	gchar* esc = (gchar*)shell_escape((guchar*)leafname);

	gtk_editable_delete_selection(edit);
	int pos = gtk_editable_get_position(edit);

	if (pos > 0 && gtk_entry_get_text(entry)[pos - 1] != ' '){
		gtk_editable_insert_text(edit, " ", 1, &pos);
	}

	gtk_editable_insert_text(edit, esc, strlen(esc), &pos);
	gtk_editable_set_position(edit, pos);

	g_free(esc);
}


/****************************************************************
 *			INTERNAL FUNCTIONS			*
 ****************************************************************/

#if 0
static void show_help(Filer* filer_window)
{
	switch (filer_window->mini_type)
	{
		case MINI_PATH:
			info_message(
				_("Enter the name of a file and I'll display "
				"it for you. Press Tab to fill in the longest "
				"match. Escape to close the minibuffer."));
			break;
		case MINI_SHELL:
			info_message(
				_("Enter a shell command to execute. Click "
				"on a file to add it to the buffer."));
			break;
		case MINI_SELECT_BY_NAME:
			info_message(
				_("Enter a file name pattern to select all matching files:\n\n"
				"? means any character\n"
				"* means zero or more characters\n"
				"[aA] means 'a' or 'A'\n"
				"[a-z] means any character from a to z (lowercase)\n"
				"*.png means any name ending in '.png'"));
			break;
		case MINI_SELECT_IF:
			show_condition_help(NULL);
			break;
		case MINI_FILTER:
			info_message(
				_("Enter a pattern to match for files to "
				"be shown.  An empty filter turns the "
				  "filter off."));
			break;
		default:
			g_warning("Unknown minibuffer type!");
			break;
	}
}
#endif


/*			PATH ENTRY			*/

static void
path_return_pressed (AyyiFilemanager* fm, GdkEventKey* event)
{
	int flags = OPEN_FROM_MINI | OPEN_SAME_WINDOW;

	const gchar* path = gtk_entry_get_text(GTK_ENTRY(fm->mini.entry));
	char* pattern = g_path_get_basename(path);

	ViewIter iter;
	view_get_cursor(fm->view, &iter);

	DirItem* item = iter.peek(&iter);
	if (item == NULL || !matches(&iter, pattern))
	{
		gdk_beep();
		goto out;
	}
	
	if ((event->state & GDK_SHIFT_MASK) != 0)
		flags |= OPEN_SHIFT;

	fm__open_item(fm, &iter, flags);

  out:
	g_free(pattern);
}


/* Use the cursor item to fill in the minibuffer.
 * If there are multiple matches then fill in as much as possible and
 * (possibly) beep.
 */
static void
complete (AyyiFilemanager* fm)
{
	GtkEntry* entry;
	DirItem* other;
	int shortest_stem = -1;
	int current_stem;
	const gchar	*text, *leaf;
	ViewIter cursor, iter;

	view_get_cursor(fm->view, &cursor);
	DirItem* item = cursor.peek(&cursor);

	if (!item)
	{
		gdk_beep();
		return;
	}

	entry = GTK_ENTRY(fm->mini.entry);

	text = gtk_entry_get_text(entry);
	leaf = strrchr(text, '/');
	if (!leaf)
	{
		gdk_beep();
		return;
	}

	leaf++;
	if (!matches(&cursor, leaf))
	{
		gdk_beep();
		return;
	}
	
	current_stem = strlen(leaf);

	/* Find the longest other match of this name. If it's longer than
	 * the currently entered text then complete only up to that length.
	 */
	view_get_iter(fm->view, &iter, 0);
	while ((other = iter.next(&iter)))
	{
		int	stem = 0;

		if (iter.i == cursor.i)	/* XXX */
			continue;

		while (other->leafname[stem] && item->leafname[stem])
		{
			gchar	a, b;
			/* Like the matches() function below, the comparison of
			 * leafs must be case-insensitive.
			 */
		  	a = g_ascii_tolower(item->leafname[stem]);
		  	b = g_ascii_tolower(other->leafname[stem]);
			if (a != b)
				break;
			stem++;
		}

		/* stem is the index of the first difference */
		if (stem >= current_stem &&
				(shortest_stem == -1 || stem < shortest_stem))
			shortest_stem = stem;
	}

	if (current_stem == shortest_stem)
	{
		/* We didn't add anything... */
	}
	else if (current_stem < shortest_stem)
	{
		gint tmp_pos;
		
		/* Have to replace the leafname text in the minibuffer rather
		 * than just append to it.  Here's an example:
		 * Suppose we have two dirs, /tmp and /TMP.
		 * The user enters /t in the minibuffer and hits Tab.
		 * With the g_ascii_tolower() code above, this would result
		 * in /tMP being bisplayed in the minibuffer which isn't
		 * intuitive.  Therefore all text after the / must be replaced.
		 */
		tmp_pos = leaf - text; /* index of start of leaf */
		gtk_editable_delete_text(GTK_EDITABLE(entry),
					 tmp_pos, entry->text_length);
		gtk_editable_insert_text(GTK_EDITABLE(entry),
					 item->leafname, shortest_stem,
					 &tmp_pos);
		
		gtk_editable_set_position(GTK_EDITABLE(entry), -1);
	}
	else
	{
		const gchar* new = (gchar*)make_path(fm->sym_path, item->leafname);

		if (item->base_type == TYPE_DIRECTORY)
			new = (gchar*)make_path(new, "");

		gtk_entry_set_text(entry, new);
		gtk_editable_set_position(GTK_EDITABLE(entry), -1);
	}
}


static void
path_changed (AyyiFilemanager* fm)
{
	GtkWidget* mini = fm->mini.entry;
	char* path;
	char* new = NULL;
#if 0
	gboolean error = FALSE;
#endif

	const char* rawnew = gtk_entry_get_text(GTK_ENTRY(mini));
	if (!*rawnew)
	{
		/* Entry may be blank because we're in the middle of changing
		 * to something else...
		 */
#if 0
		entry_set_error(mini, FALSE);
#endif
		return;
	}

	switch (rawnew[0])
	{
	case '/':
		new = g_strdup(rawnew);
		break;

	case '~':
		if (!rawnew[1] || rawnew[1]=='/')
		{
			new = g_strconcat(g_get_home_dir(), "/", rawnew[1]? rawnew+2: "", "/", NULL);
		}
		else
		{
			const char *sl;
			gchar *username;
			struct passwd *passwd;

			
			/* Need to lookup user name */
			for(sl=rawnew+2; *sl && *sl!='/'; sl++)
				;
			username = g_strndup(rawnew+1, sl-rawnew-1);
			passwd = getpwnam(username);
			g_free(username);

			if(passwd)
			{
				new = g_strconcat(passwd->pw_dir, "/", sl+1, "/", NULL);
			}
			else
				new = g_strdup(rawnew);
		}
		break;

	default:
		new = g_strdup(rawnew);
		break;
	}

	char* leaf = g_path_get_basename(new);
	if (leaf == new)
		path = g_strdup("/");
	else
		path = g_path_get_dirname(new);

	if (strcmp(path, fm->sym_path) != 0)
	{
		/* The new path is in a different directory */
		struct stat info;

		if (stat_with_timeout(path, &info) == 0 && S_ISDIR(info.st_mode)) {
			fm__change_to(fm, path, leaf);
		}
#if 0
		else
			error = TRUE;
#endif
	}
	else
	{
		if (*leaf == '.' && !fm->temp_show_hidden) {
			fm->temp_show_hidden = TRUE;
			display_update_hidden(fm);
		}
		
		if (find_exact_match(fm, leaf) == FALSE && find_next_match(fm, leaf, 0) == FALSE)
#if 1
			;
#else
			error = TRUE;
#endif
	}

	g_free(leaf);
	g_free(new);
	g_free(path);

#if 0
	entry_set_error(mini, error);
#endif
}


/* Look for an exact match, and move the cursor to it if found.
 * TRUE on success.
 */
static gboolean
find_exact_match (AyyiFilemanager* fm, const gchar *pattern)
{
	DirItem* item;
	ViewIface* view = fm->view;

	ViewIter iter;
	view_get_iter(view, &iter, 0);

	while ((item = iter.next(&iter)))
	{
		if (strcmp(item->leafname, pattern) == 0)
		{
			view_cursor_to_iter(view, &iter);
			return TRUE;
		}
	}

	return FALSE;
}


/* Find the next item in the view that matches 'pattern'. Start from
 * cursor_base and loop at either end. dir is 1 for a forward search,
 * -1 for backwards. 0 means forwards, but may stay the same.
 *
 * Does not automatically update cursor_base.
 *
 * Returns TRUE if a match is found.
 */
static gboolean
find_next_match (AyyiFilemanager* fm, const char *pattern, int dir)
{
	ViewIface* view = fm->view;
	ViewIter iter;

	if (view_count_items(view) < 1)
		return FALSE;

	view_get_iter(view, &iter,
		VIEW_ITER_FROM_BASE |
		(dir >= 0 ? 0 : VIEW_ITER_BACKWARDS));

	if (dir != 0)
		iter.next(&iter);	/* Don't look at the base itself */

	while (iter.next(&iter))
	{
		if (matches(&iter, pattern))
		{
			view_cursor_to_iter(view, &iter);
			return TRUE;
		}
	}

	/* No matches (except possibly base itself) */
	view_get_iter(view, &iter,
		VIEW_ITER_FROM_BASE | VIEW_ITER_ONE_ONLY |
		(dir >= 0 ? 0 : VIEW_ITER_BACKWARDS));

	view_cursor_to_iter(view, &iter);

	return FALSE;
}


static gboolean matches(ViewIter *iter, const char *pattern)
{
	DirItem *item;
	
	item = iter->peek(iter);
	
	return strncasecmp(item->leafname, pattern, strlen(pattern)) == 0;
}

/* Find next match and set base for future matches. */
static void
search_in_dir (AyyiFilemanager* fm, int dir)
{
	ViewIter iter;

	const char* path = gtk_entry_get_text(GTK_ENTRY(fm->mini.entry));
	char* pattern = g_path_get_basename(path);
	
	view_get_cursor(fm->view, &iter);
	view_set_base(fm->view, &iter);
	find_next_match(fm, pattern, dir);
	view_get_cursor(fm->view, &iter);
	view_set_base(fm->view, &iter);

	g_free(pattern);
}


#ifdef SHELL

/*			SHELL COMMANDS			*/

static void
add_to_history (const gchar* line)
{
	guchar 	*last;
	
	last = shell_history ? (guchar *) shell_history->data : NULL;

	if (last && strcmp(last, line) == 0)
		return;			/* Duplicating last entry */
	
	shell_history = g_list_prepend(shell_history, g_strdup(line));
}


static void
shell_done (AyyiFilemanager* fm)
{
	if (fm__exists(fm))
		fm__update_dir(fm, TRUE);
}
#endif


/* Given a list of matches, return the longest stem. g_free() the result.
 * Special chars are escaped. If there is only a single (non-dir) match
 * then a trailing space is added.
 */
#ifdef SHELL
static guchar*
best_match (AyyiFilemanager* fm, glob_t *matches)
{
	gchar* first = matches->gl_pathv[0];
	gchar* path;

	int longest = strlen(first);

	int i; for (i = 1; i < matches->gl_pathc; i++)
	{
		int	j;
		guchar* m = (guchar*)matches->gl_pathv[i];

		for (j = 0; j < longest; j++)
			if (m[j] != first[j])
				longest = j;
	}

	int path_len = strlen(fm->sym_path);
	if (strncmp(fm->sym_path, first, path_len) == 0 &&
			first[path_len] == '/' && first[path_len + 1])
	{
		path = g_strndup(first + path_len + 1, longest - path_len - 1);
	}
	else
		path = g_strndup(first, longest);

	gchar* tmp = (gchar*)shell_escape((guchar*)path);
	g_free(path);

	if (matches->gl_pathc == 1 && tmp[strlen(tmp) - 1] != '/') {
		path = g_strdup_printf("%s ", tmp);
		g_free(tmp);
		return (guchar*)path;
	}
	
	return (guchar*)tmp;
}
#endif


#ifdef SHELL
static void
shell_tab (AyyiFilemanager* fm)
{
	glob_t matches;
	int	leaf_start;
	
	const gchar* entry = gtk_entry_get_text(GTK_ENTRY(fm->mini.entry));
	int pos = gtk_editable_get_position(GTK_EDITABLE(fm->mini.entry));
	GString* leaf = g_string_new(NULL);

	int	i; for (i = 0; i < pos; i++)
	{
		guchar	c = entry[i];
		
		if (leaf->len == 0)
			leaf_start = i;

		if (c == ' ')
		{
			g_string_truncate(leaf, 0);
			continue;
		}
		else if (c == '\\' && i + 1 < pos)
			c = entry[++i];
		else if (c == '"' || c == '\'')
		{
			for (++i; i < pos; i++)
			{
				guchar cc = entry[i];

				if (cc == '\\' && i + 1 < pos)
					cc = entry[++i];
				else if (cc == c)
					break;
				g_string_append_c(leaf, cc);
			}
			continue;
		}
		
		g_string_append_c(leaf, c);
	}

	if (leaf->len == 0)
		leaf_start = pos;

	if (leaf->str[0] != '/' && leaf->str[0] != '~')
	{
		g_string_prepend_c(leaf, '/');
		g_string_prepend(leaf, fm->sym_path);
	}

	g_string_append_c(leaf, '*');

	if (glob(leaf->str,
#ifdef GLOB_TILDE
			GLOB_TILDE |
#endif
			GLOB_MARK, NULL, &matches) == 0)
	{
		if (matches.gl_pathc > 0)
		{
			GtkEditable* edit = GTK_EDITABLE(fm->mini.entry);

			guchar* best = best_match(fm, &matches);

			gtk_editable_delete_text(edit, leaf_start, pos);
			gtk_editable_insert_text(edit, best, strlen(best), &leaf_start);
			gtk_editable_set_position(edit, leaf_start);

			g_free(best);
		}
		if (matches.gl_pathc != 1)
			gdk_beep();

		globfree(&matches);
	}

	g_string_free(leaf, TRUE);
}


static void
run_child (gpointer unused)
{
#if 0
	/* Ensure output is noticed - send stdout to stderr */
	dup2(STDERR_FILENO, STDOUT_FILENO);
	close_on_exec(STDOUT_FILENO, FALSE);
#endif
}


/* Either execute the command or make it the default run action */
static void
shell_return_pressed (AyyiFilemanager* fm)
{
	const gchar* entry = mini_contents(fm);

	if (!entry)
		goto out;

#if 0
	add_to_history(entry);
#endif

	GPtrArray* argv = g_ptr_array_new();
	g_ptr_array_add(argv, "sh");
	g_ptr_array_add(argv, "-c");
	g_ptr_array_add(argv, (gchar *) entry);
	g_ptr_array_add(argv, "sh");

	DirItem* item;
	ViewIter iter;
	view_get_iter(fm->view, &iter, 0);
	while ((item = iter.next(&iter)))
	{
		if (view_get_selected(fm->view, &iter)){
			g_ptr_array_add(argv, item->leafname);
		}
	}
	
	g_ptr_array_add(argv, NULL);

	pid_t child;
	GError* error = NULL;
	if (!g_spawn_async_with_pipes(fm->sym_path,
			(gchar**)argv->pdata, NULL,
			G_SPAWN_DO_NOT_REAP_CHILD |
			G_SPAWN_SEARCH_PATH,
			run_child, NULL,   // Child setup fn
			&child,            // Child PID
			NULL, NULL, NULL,  // Standard pipes
			&error))
	{
#if 0
		delayed_error("%s", error ? error->message : "(null)");
#else
		g_error("%s", error ? error->message : "(null)");
#endif
		g_error_free(error);
	}
#if 0
	else
		on_child_death(child, (CallbackFn) shell_done, filer_window);
#endif

	g_ptr_array_free(argv, TRUE);

out:
	minibuffer_hide(fm);
}
#endif // SHELL


#ifdef SHELL
/* Move through the shell history */
static void
shell_recall (Filer* filer_window, int dir)
{
	guchar	*command;
	int	pos = filer_window->mini_cursor_base;

	pos += dir;
	if (pos >= 0)
	{
		command = g_list_nth_data(shell_history, pos);
		if (!command)
			return;
	}
	else
		command = "";

	if (pos < -1)
		pos = -1;
	filer_window->mini_cursor_base = pos;

	gtk_entry_set_text(GTK_ENTRY(filer_window->minibuffer), command);
}
#endif

#ifdef SELECT

/*			SELECT IF			*/

typedef struct {
	FindInfo info;
	AyyiFilemanager* fm;
	FindCondition* cond;
} SelectData;

static gboolean
select_if_test(ViewIter* iter, gpointer user_data)
{
	DirItem *item;
	SelectData *data = user_data;

	item = iter->peek(iter);
	g_return_val_if_fail(item != NULL, FALSE);

	data->info.leaf = item->leafname;
	data->info.fullpath = make_path(data->filer_window->sym_path, data->info.leaf);

	return mc_lstat(data->info.fullpath, &data->info.stats) == 0 && find_test_condition(data->cond, &data->info);
}


static void
select_return_pressed (AyyiFilemanager* fm, guint etime)
{
	SelectData	data;

	const gchar* entry = mini_contents(filer_window);

	if (!entry)
		goto out;

	add_to_history(entry);

	data.cond = find_compile(entry);
	if (!data.cond)
	{
		delayed_error(_("Invalid Find condition"));
		return;
	}

	data.info.now = time(NULL);
	data.info.prune = FALSE;	/* (don't care) */
	data.filer_window = filer_window;

	view_select_if(fm->view, select_if_test, &data);

	find_condition_free(data.cond);
out:
	minibuffer_hide(fm);
}
#endif // select


static void
filter_return_pressed (AyyiFilemanager* fm, guint etime)
{
	const gchar* entry = mini_contents(fm);

	if (entry && *entry && strcmp(entry, "*") != 0) {
		display_set_filter(fm, FILER_SHOW_GLOB, entry);
	} else {
		display_set_filter(fm, FILER_SHOW_ALL, NULL);
	}
	minibuffer_hide(fm);
}


/*			EVENT HANDLERS			*/


static gint
mini_key_press_event(GtkWidget* widget, GdkEventKey* event, AyyiFilemanager* fm)
{
	if (event->keyval == GDK_Escape)
	{
#if 0
		if (filer_window->mini_type == MINI_SHELL)
		{
			const gchar* line = mini_contents(fm);
			if (line)
				add_to_history(line);
		}
#endif

		minibuffer_hide(fm);
		return TRUE;
	}

	switch (fm->mini.type)
	{
		case MINI_PATH:
			switch (event->keyval)
			{
				case GDK_Up:
					search_in_dir(fm, -1);
					break;
				case GDK_Down:
					search_in_dir(fm, 1);
					break;
				case GDK_Return:
				case GDK_KP_Enter:
					path_return_pressed(fm, event);
					break;
				case GDK_Tab:
					complete(fm);
					break;
				default:
					return FALSE;
			}
			break;
#ifdef SHELL
		case MINI_SHELL:
			switch (event->keyval)
			{
				case GDK_Up:
					shell_recall(fm, 1);
					break;
				case GDK_Down:
					shell_recall(fm, -1);
					break;
				case GDK_Tab:
					shell_tab(fm);
					break;
				case GDK_Return:
				case GDK_KP_Enter:
					shell_return_pressed(fm);
					break;
				default:
					return FALSE;
			}
			break;
#endif
#ifdef SELECT
		case MINI_SELECT_IF:
			switch (event->keyval)
			{
				case GDK_Up:
					shell_recall(fm, 1);
					break;
				case GDK_Down:
					shell_recall(fm, -1);
					break;
				case GDK_Tab:
					break;
				case GDK_Return:
				case GDK_KP_Enter:
					select_return_pressed(fm, event->time);
					break;
				default:
					return FALSE;
			}
			break;
#endif
		case MINI_SELECT_BY_NAME:
			switch (event->keyval)
			{
				case GDK_Up:
					fm__next_selected(fm, -1);
					break;
				case GDK_Down:
					fm__next_selected(fm, 1);
					break;
				case GDK_Tab:
					break;
				case GDK_Return:
				case GDK_KP_Enter:
					minibuffer_hide(fm);
					break;
				default:
					return FALSE;
			}
			break;

	        case MINI_FILTER:
			switch (event->keyval)
			{
				case GDK_Return:
				case GDK_KP_Enter:
					filter_return_pressed(fm, event->time);
					break;
				default:
					return FALSE;
			}
			break;
		default:
			break;
	}

	return TRUE;
}


static gboolean
select_if_glob (ViewIter* iter, gpointer data)
{
	const char* pattern = (char*)data;

	DirItem* item = iter->peek(iter);
	g_return_val_if_fail(item != NULL, FALSE);

	return fnmatch(pattern, item->leafname, 0) == 0;
}


static void
changed (GtkEditable* mini, AyyiFilemanager* fm)
{
#if 0
	// from action.c
	void set_find_string_colour(GtkWidget* widget, const guchar* string)
	{
		FindCondition* cond = find_compile(string);
		entry_set_error(widget, !cond);

		find_condition_free(cond);
	}
#endif

	ViewIter iter;

	switch (fm->mini.type)
	{
		case MINI_PATH:
			path_changed(fm);
			return;
		case MINI_SELECT_IF:
			//set_find_string_colour(GTK_WIDGET(mini), gtk_entry_get_text(GTK_ENTRY(filer_window->minibuffer)));
			return;
		case MINI_SELECT_BY_NAME:
			view_select_if(fm->view, select_if_glob, (gpointer)gtk_entry_get_text( GTK_ENTRY(fm->mini.entry)));
			view_get_iter(fm->view, &iter, VIEW_ITER_FROM_BASE | VIEW_ITER_SELECTED);
			if (iter.next(&iter))
				view_cursor_to_iter(fm->view, &iter);
			return;
		default:
			break;
	}
}


/* Returns a string (which must NOT be freed), or NULL if the buffer
 * is blank (whitespace only).
 */
static const gchar*
mini_contents (AyyiFilemanager* fm)
{
	const gchar* c;

	const gchar* entry = gtk_entry_get_text(GTK_ENTRY(fm->mini.entry));

	for (c = entry; *c; c++)
		if (!g_ascii_isspace(*c))
			return entry;

	return NULL;
}


/* This is a really ugly hack to get around Gtk+-2.0's broken auto-select
 * behaviour.
 */
static gboolean
grab_focus (GtkWidget *minibuffer)
{
	GtkWidgetClass *class;
	
	class = GTK_WIDGET_CLASS(gtk_type_class(GTK_TYPE_WIDGET));

	class->grab_focus(minibuffer);

	g_signal_stop_emission(minibuffer,
		g_signal_lookup("grab_focus", G_OBJECT_TYPE(minibuffer)), 0);


	return 1;
}


#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <sys/wait.h>
//#include <netdb.h>
//#include <ctype.h>
//#include <pwd.h>
//#include <grp.h>
//#include <fcntl.h>
#include <dirent.h>
#include <sys/param.h>
#include <errno.h>

#include "mysql/mysql.h"
#include <gtk/gtk.h>
#ifdef OLD
  #include <libart_lgpl/libart.h>
#endif

#include "dh-link.h"
#include "typedefs.h"
#include <gqview2/typedefs.h>
#include "support.h"
#include "main.h"
#include "gqview2/ui_fileops.h"
#include "rox/rox_global.h"
#include "rox/rox_support.h"

#define HEX_ESCAPE '%'

extern struct _app app;
extern unsigned debug;

static const char* action_leaf = NULL;
static int  from_parent = 0;

static void        do_move(const char *path, const char *dest);
static const char* make_dest_path(const char *object, const char *dir);
static void        send_check_path(const gchar *path);
static void        check_flags();
static void        process_flag(char flag);


void
errprintf(char *format, ...)
{
  //fn prints an error string, then passes arguments on to vprintf.

  va_list argp; //points to each unnamed arg in turn

  printf("%s ", err);

  va_start(argp, format); //make ap (arg pointer) point to 1st unnamed arg

  vprintf(format, argp);

  va_end(argp); //clean up
}


void
errprintf2(const char* func, char *format, ...)
{
  //fn prints an error string, then passes arguments on to vprintf.

  printf("%s %s(): ", err, func);

  va_list argp;
  va_start(argp, format);
  vprintf(format, argp);
  va_end(argp);
}


void
warnprintf(char *format, ...)
{
  //fn prints a warning string, then passes arguments on to vprintf.

  printf("%s ", warn);

  va_list argp;           //points to each unnamed arg in turn
  va_start(argp, format); //make ap (arg pointer) point to 1st unnamed arg
  vprintf(format, argp);
  va_end(argp);           //clean up
}


void
debug_printf(const char* func, int level, const char *format, ...)
{
	va_list args;

	va_start(args, format);
	if (level <= debug) {
		fprintf(stderr, "%s(): ", func);
		vfprintf(stderr, format, args);
		fprintf(stderr, "\n");
	}
	va_end(args);
}


void
samplerate_format(char* str, int samplerate)
{
	snprintf(str, 32, "%f", ((float)samplerate) / 1000);
	while(str[strlen(str)-1]=='0'){
		str[strlen(str)-1] = '\0';
	}
	if(str[strlen(str)-1]=='.') str[strlen(str)-1] = '\0';
}


#if 0
/* Used as the sort function for sorting GPtrArrays */
gint 
strcmp2(gconstpointer a, gconstpointer b)
{
	const char *aa = *(char **) a;
	const char *bb = *(char **) b;

	return g_strcasecmp(aa, bb);
}

/* Returns an array listing all the names in the directory 'path'.
 * The array is sorted.
 * '.' and '..' are skipped.
 * On error, the error is reported with g_warning and NULL is returned.
 */
GPtrArray*
list_dir(const guchar *path)
{
	GDir *dir;
	GError *error = NULL;
	const char *leaf;
	GPtrArray *names;
	
	dir = g_dir_open((char*)path, 0, &error);
	if (error)
	{
		g_warning("Can't list directory:\n%s", error->message);
		g_error_free(error);
		return NULL;
	}

	names = g_ptr_array_new();

	while ((leaf = g_dir_read_name(dir))) {
		if (leaf[0] != '.')
			g_ptr_array_add(names, g_strdup(leaf));
	}

	g_dir_close(dir);

	g_ptr_array_sort(names, strcmp2);

	return names;
}
#endif //0

/* Returns TRUE if the object exists, FALSE if it doesn't.
 * For symlinks, the file pointed to must exist.
 */
gboolean 
file_exists(const char *path)
{
	struct stat info;

	return !stat(path, &info);
}

gboolean
is_dir(const char *path)
{
	PF;
	struct stat info2;
	return lstat(path, &info2) == 0 && S_ISDIR(info2.st_mode);
}

gboolean
dir_is_empty(const char *path)
{
    DIR *dp;

    if (strcmp(path, "/") == 0) return FALSE;

    dp = opendir (path);

    int n = 0;
    while(readdir(dp) != NULL){
        n++;
        if (n > 2) {
            closedir(dp);
            return FALSE;
        }
    }
    closedir(dp);
    return TRUE;
}

void
file_extension(const char* path, char* extn)
{
	ASSERT_POINTER(path, "path");
	ASSERT_POINTER(extn, "extn");

	gchar** split = g_strsplit(path, ".", 0);
	if(split[0]){
		int i = 1;
		while(split[i]) i++;
		dbg(2, "extn=%s i=%i", split[i-1], i);
		strcpy(extn, split[i-1]);
		
	}else{
		dbg(0, "failed (%s)", path);
		memset(extn, '\0', 1);
	}

	g_strfreev(split);
}


/* TRUE iff `sub' is (or would be) an object inside the directory `parent',
 * (or the two are the same item/directory).
 * FALSE if parent doesn't exist.
 */
gboolean
is_sub_dir(const char *sub_obj, const char *parent)
{
	struct stat parent_info;
	char *sub;

	if (lstat(parent, &parent_info)) return FALSE; /* Parent doesn't exist */

	/* For checking Copy/Move operations do a realpath first on sub
	 * (the destination), since copying into a symlink is the same as
	 * copying into the thing it points to. Don't realpath 'parent' though;
	 * copying a symlink just makes a new symlink.
	 * 
	 * When checking if an icon depends on a file (parent), use realpath on
	 * sub (the icon) too.
	 */
	sub = pathdup(sub_obj);
	
	while (1)
	{
		char	    *slash;
		struct stat info;
		
		if (lstat(sub, &info) == 0)
		{
			if (info.st_dev == parent_info.st_dev &&
				info.st_ino == parent_info.st_ino)
			{
				g_free(sub);
				return TRUE;
			}
		}
		
		slash = strrchr(sub, '/');
		if (!slash)
			break;
		if (slash == sub)
		{
			if (sub[1])
				sub[1] = '\0';
			else
				break;
		}
		else
			*slash = '\0';
	}

	g_free(sub);

	return FALSE;
}

/* Move path to dest.
 * Check that path not moved into itself.
 */
void
file_move(const char* path, const char* dest)
{
	if (is_sub_dir(make_dest_path(path, dest), path)) dbg(0, "!ERROR: Can't move/rename object into itself");
	else {
		do_move(path, dest);
		send_check_path(dest);
	}
}


static void
do_move(const char *path, const char *dest)
{
	const char* argv[] = {"mv", "-f", NULL, NULL, NULL};
	struct stat	info2;

	check_flags();

	//const char* dest_path = make_dest_path(path, dest);

	gboolean is_dir = lstat(path, &info2) == 0 && S_ISDIR(info2.st_mode);

	if (access(dest, F_OK) == 0)
	{
		struct stat	info;
		if (lstat(dest, &info)) {
			//send_error();
			dbg(0, "error!");
			return;
		}

		if (!is_dir && /*o_newer &&*/ info2.st_mtime > info.st_mtime)
		{
			/* Newer; keep going */
		}
		else
		{
			statusbar_print(1, "file already exists.");
			return;
#if 0
			//printf_send("<%s", path);
			dbg(0, "<%s", path);
			//printf_send(">%s", dest_path);
			dbg(0, ">%s", dest_path);
			if (!printf_reply(from_parent, TRUE, "?'%s' already exists - overwrite?", dest_path)) return;
		}

		int err;
		if (S_ISDIR(info.st_mode)) err = rmdir(dest_path);
		else                       err = unlink(dest_path);

		if (err) {
			send_error();
			if (errno != ENOENT) return;
			printf_send("'Trying move anyway...\n");
#endif
		}
	}
#if 0
	else if (!quiet)
	{
		printf_send("<%s", path);
		printf_send(">");
		if (!printf_reply(from_parent, FALSE, "?Move %s as %s?", path, dest_path)) return;
	}
	else if (!o_brief) printf_send("'Moving %s as %s\n", path, dest_path);
#endif

	argv[2] = path;
	argv[3] = dest;

	char* err = fork_exec_wait(argv);
	if (err)
	{
		//printf_send("!%s\nFailed to move %s as %s\n", err, path, dest_path);
		dbg(0, "!%s\nFailed to move %s as %s\n", err, path, dest);
		dbg(0, "FIXME change to using print callback to application");
		statusbar_printf(1, "!%s: Failed to move %s as %s", err, path, dest);
		g_free(err);
	}
	else
	{
		send_check_path(dest);

		//if (is_dir) send_mount_path(path);
		if (is_dir) dbg(0, "?%s", path);
		else        statusbar_printf(1, "file '%s' moved.", path);
	}
}


#if 0
/* Like g_strdup, but does realpath() too (if possible) */
char*
pathdup(const char* path)
{
	char real[MAXPATHLEN];

	g_return_val_if_fail(path != NULL, NULL);

	if (realpath(path, real))
		return g_strdup(real);

	return g_strdup(path);
}
#endif


/* We want to copy 'object' into directory 'dir'. If 'action_leaf'
 * is set then that is the new leafname, otherwise the leafname stays
 * the same.
 */
static const char*
make_dest_path(const char *object, const char *dir)
{
	const char *leaf;

	if (action_leaf)
		leaf = action_leaf;
	else
	{
		leaf = strrchr(object, '/');
		if (!leaf)
			leaf = object;		/* Error? */
		else
			leaf++;
	}

	return (const char*)make_path(dir, leaf);
}


/* Notify the filer that this item has been updated */
static void
send_check_path(const gchar *path)
{
	//printf_send("s%s", path);
	dbg(0, "s%s", path);
}


/* If the parent has sent any flag toggles, read them */
static void
check_flags(void)
{
	fd_set set;
	int	got;
	char retval;
	struct timeval tv;

	FD_ZERO(&set);

	while (1)
	{
		FD_SET(from_parent, &set);
		tv.tv_sec = 0;
		tv.tv_usec = 0;
		got = select(from_parent + 1, &set, NULL, NULL, &tv);

		if (got == -1) g_error("select() failed: %s\n", g_strerror(errno));
		else if (!got) return;

		got = read(from_parent, &retval, 1);
		if (got != 1) g_error("read() error: %s\n", g_strerror(errno));

		process_flag(retval);
	}
}


static void process_flag(char flag)
{
	dbg(0, "...");
#if 0
	switch (flag)
	{
		case 'Q':
			quiet = !quiet;
			break;
		case 'F':
			o_force = !o_force;
			break;
		case 'R':
			o_recurse = !o_recurse;
			break;
		case 'B':
			o_brief = !o_brief;
			break;
	        case 'W':
		        o_newer = !o_newer;
			break;
		case 'E':
			read_new_entry_text();
			break;
		default:
			printf_send("!ERROR: Bad message '%c'\n", flag);
			break;
	}
#endif
}

#if 0
/* Join the path to the leaf (adding a / between them) and
 * return a pointer to a static buffer with the result. Buffer is valid
 * until the next call to make_path.
 * The return value may be used as 'dir' for the next call.
 */
const guchar*
make_path(const char *dir, const char *leaf)
{
	static GString *buffer = NULL;

	if (!buffer)
		buffer = g_string_new(NULL);

	g_return_val_if_fail(dir != NULL, buffer->str);
	g_return_val_if_fail(leaf != NULL, buffer->str);

	if (buffer->str != dir)
		g_string_assign(buffer, dir);

	if (dir[0] != '/' || dir[1] != '\0')
		g_string_append_c(buffer, '/');	/* For anything except "/" */

	g_string_append(buffer, leaf);

	return buffer->str;
}
#endif


/* Fork and exec argv. Wait and return the child's exit status.
 * -1 if spawn fails.
 * Returns the error string from the command if any, or NULL on success.
 * If the process returns a non-zero exit status without producing a message,
 * a suitable message is created.
 * g_free() the result.
 */
char*
fork_exec_wait(const char **argv)
{
	int	status;
	gchar	*errors = NULL;
	GError	*error = NULL;

	if (!g_spawn_sync(NULL, (char **) argv, NULL,
		     G_SPAWN_SEARCH_PATH | G_SPAWN_STDOUT_TO_DEV_NULL,
		     NULL, NULL,
		     NULL, &errors, &status, &error))
	{
		char* msg = g_strdup(error->message);
		g_error_free(error);
		return msg;
	}

	if (errors && !*errors) null_g_free(&errors);

	if (!WIFEXITED(status))
	{
		if (!errors) errors = g_strdup("(Subprocess crashed?)");
	}
	else if (WEXITSTATUS(status))
	{
		if (!errors) errors = g_strdup("ERROR");
	}

	if (errors) g_strstrip(errors);

	return errors;
}


gchar*
uri_text_from_list(GList *list, gint *len, gint plain_text)
{
    gchar *uri_text = NULL;
    GString *string;
    GList *work;

    if (!list) {
        if (len) *len = 0;
        return NULL;
    }

    string = g_string_new("");

    work = list;
    while (work) {
        const gchar *name8; /* dnd filenames are in utf-8 */

        name8 = work->data;

        if (!plain_text)
            {
            gchar *escaped;

            escaped = uri_text_escape(name8);
            g_string_append(string, "file:");
            g_string_append(string, escaped);
            g_free(escaped);

            g_string_append(string, "\r\n");
            }
        else
            {
            g_string_append(string, name8);
            if (work->next) g_string_append(string, "\n");
            }

        work = work->next;
        }

    uri_text = string->str;
    if (len) *len = string->len;
    g_string_free(string, FALSE);

    return uri_text;
}

static void
uri_list_parse_encoded_chars(GList *list)
{
    GList *work = list;

    while (work)
        {
        gchar *text = work->data;

        uri_text_decode(text);

        work = work->next;
        }
}

GList *uri_list_from_text(gchar *data, gint files_only)
{
    GList *list = NULL;
    gint b, e;

    b = e = 0;

    while (data[b] != '\0')
        {
        while (data[e] != '\r' && data[e] != '\n' && data[e] != '\0') e++;
        if (strncmp(data + b, "file:", 5) == 0)
            {
            gchar *path;
            b += 5;
            while (data[b] == '/' && data[b+1] == '/') b++;
            path = g_strndup(data + b, e - b);
            list = g_list_append(list, path_to_utf8(path));
            g_free(path);
            }
        else if (!files_only && strncmp(data + b, "http:", 5) == 0)
            {
            list = g_list_append(list, g_strndup(data + b, e - b));
            }
        else if (!files_only && strncmp(data + b, "ftp:", 3) == 0)
            {
            list = g_list_append(list, g_strndup(data + b, e - b));
            }
        while (data[e] == '\r' || data[e] == '\n') e++;
        b = e;
        }

    uri_list_parse_encoded_chars(list);

    return list;
}

static gint escape_char_list[] = {
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1,   /*   0 */
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1,   /*  10 */
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1,   /*  20 */
/*       spc !  "  #  $  %  &  '           */
    1, 1, 0, 0, 1, 1, 0, 1, 0, 0,   /*  30 */
/*  (  )  *  +  ,  -  .  /  0  1           */
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0,   /*  40 */
/*  2  3  4  5  6  7  8  9  :  ;           */
    0, 0, 0, 0, 0, 0, 0, 0, 0, 1,   /*  50 */
/*  <  =  >  ?  @  A  B  C  D  E           */
    1, 0, 1, 1, 0, 0, 0, 0, 0, 0,   /*  60 */
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0,   /*  70 */
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0,   /*  80 */
/*  Z  [  \  ]  ^  _  `  a  b  c           */
    0, 1, 1, 1, 1, 0, 1, 0, 0, 0,   /*  90 */
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0,   /* 100 */
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0,   /* 110 */
/*  x  y  z  {  |  }  ~ del                */
    0, 0, 0, 1, 1, 1, 0, 0      /* 120, 127 is end */
};

static gchar *hex_char = "0123456789ABCDEF";

static gint
escape_test(guchar c)
{
    if (c < 32 || c > 127) return TRUE;
    return (escape_char_list[c] != 0);
}


static const gchar*
escape_code(guchar c)
{
    static gchar text[4];

    text[0] = '%';
    text[1] = hex_char[c>>4];
    text[2] = hex_char[c%16];
    text[3] = '\0';

    return text;
}


gchar*
uri_text_escape(const gchar *text)
{
    GString *string;
    gchar *result;
    const gchar *p;

    if (!text) return NULL;

    string = g_string_new("");

    p = text;
    while (*p != '\0')
        {
        if (escape_test(*p))
            {
            g_string_append(string, escape_code(*p));
            }
        else
            {
            g_string_append_c(string, *p);
            }
        p++;
        }

    result = string->str;
    g_string_free(string, FALSE);

    /* dropped filenames are expected to be utf-8 compatible */
    if (!g_utf8_validate(result, -1, NULL))
        {
        gchar *tmp;

        tmp = g_locale_to_utf8(result, -1, NULL, NULL, NULL);
        if (tmp)
            {
            g_free(result);
            result = tmp;
            }
        }

    return result;
}

/* this operates on the passed string, decoding escaped characters */
void uri_text_decode(gchar *text)
{
    if (strchr(text, '%'))
        {
        gchar *w;
        gchar *r;

        w = r = text;

        while(*r != '\0')
            {
            if (*r == '%' && *(r + 1) != '\0' && *(r + 2) != '\0')
                {
                gchar t[3];
                gint n;

                r++;
                t[0] = *r;
                r++;
                t[1] = *r;
                t[2] = '\0';
                n = (gint)strtol(t, NULL, 16);
                if (n > 0 && n < 256)
                    {
                    *w = (gchar)n;
                    }
                else
                    {
                    /* invalid number, rewind and ignore this escape */
                    r -= 2;
                    *w = *r;
                    }
                }
            else if (w != r)
                {
                *w = *r;
                }
            r++;
            w++;
            }
        if (*w != '\0') *w = '\0';
        }
}


#if 0
//below is stuff from gnome-vfs-uri.c
//-i think eventually we might as well statically link the whole file.


/**
 * gnome_vfs_uri_list_parse:
 * @uri_list:
 *
 * Extracts a list of #GnomeVFSURI objects from a standard text/uri-list,
 * such as one you would get on a drop operation.  Use
 * #gnome_vfs_uri_list_free when you are done with the list.
 *
 * Return value: A GList of GnomeVFSURIs
 **/
GList*
gnome_vfs_uri_list_parse(const gchar* uri_list)
{
    /* Note that this is mostly very stolen from old libgnome/gnome-mime.c */

    const gchar *p, *q;
    gchar *retval;
    GnomeVFSURI/*gchar*/ *uri;
    GList *result = NULL;

    g_return_val_if_fail (uri_list != NULL, NULL);

    p = uri_list;

    /* We don't actually try to validate the URI according to RFC
     * 2396, or even check for allowed characters - we just ignore
     * comments and trim whitespace off the ends.  We also
     * allow LF delimination as well as the specified CRLF.
     */
    while (p != NULL) {
        if (*p != '#') {
            while (g_ascii_isspace (*p))
                p++;

            q = p;
            while ((*q != '\0')
                   && (*q != '\n')
                   && (*q != '\r'))
                q++;

            if (q > p) {
                q--;
                while (q > p
                       && g_ascii_isspace (*q))
                    q--;

                retval = g_malloc (q - p + 2);
                strncpy (retval, p, q - p + 1);
                retval[q - p + 1] = '\0';

                //uri = gnome_vfs_uri_new (retval);
                uri = malloc(128);
                snprintf(uri, 128, retval);

                g_free(retval);

                if(uri != NULL) result = g_list_prepend(result, uri);
            }
        }
        p = strchr (p, '\n');
        if (p != NULL)
            p++;
    }

    return g_list_reverse(result);
}


const gchar *
vfs_get_method_string(const gchar *substring, gchar **method_string)
{
	//get the first part of a uri, eg "file://"

    const gchar *p;
    char *method;

    for (p = substring;
         g_ascii_isalnum (*p) || *p == '+' || *p == '-' || *p == '.';
         p++)
        ;

    if (*p == ':') {
        /* Found toplevel method specification.  */
        method = g_strndup (substring, p - substring);
        *method_string = g_ascii_strdown (method, -1);
        g_free (method);
        p++;
    } else {
        *method_string = g_strdup ("file");
        p = substring;
    }
    return p;
}

/**
 * gnome_vfs_uri_list_free:
 * @list: list of GnomeVFSURI elements
 *
 * Decrements the reference count of each member of @list by one,
 * and frees the list itself.
 **/
void
gnome_vfs_uri_list_free (GList *list)
{
    g_list_free (gnome_vfs_uri_list_unref (list));
}

/**
 * gnome_vfs_uri_list_unref:
 * @list: list of GnomeVFSURI elements
 *
 * Decrements the reference count of the items in @list by one.
 * Note that the list is *not freed* even if each member of the list
 * is freed.
 *
 * Return value: @list
 **/
GList *
gnome_vfs_uri_list_unref (GList *list)
{
    g_list_foreach (list, (GFunc) gnome_vfs_uri_unref, NULL);
    return list;
}

/**
 * gnome_vfs_uri_unref:
 * @uri: A GnomeVFSURI.
 *
 * Decrement @uri's reference count.  If the reference count reaches zero,
 * @uri is destroyed.
 **/
void
gnome_vfs_uri_unref (GnomeVFSURI *uri)
{
    GnomeVFSURI *p, *parent;

    g_return_if_fail (uri != NULL);
    g_return_if_fail (uri->ref_count > 0);

    for (p = uri; p != NULL; p = parent) {
        parent = p->parent;
        g_assert (p->ref_count > 0);
        p->ref_count--;
        if (p->ref_count == 0)
            destroy_element (p);
    }
}

/* Destroy an URI element, but not its parent.  */
static void
destroy_element (GnomeVFSURI *uri)
{
    g_free (uri->text);
    g_free (uri->fragment_id);
    g_free (uri->method_string);

    if (uri->parent == NULL) {
        GnomeVFSToplevelURI *toplevel;

        toplevel = (GnomeVFSToplevelURI *) uri;
        g_free (toplevel->host_name);
        g_free (toplevel->user_name);
        g_free (toplevel->password);
    }

    g_free (uri);
}

/**
 * gnome_vfs_uri_get_toplevel:
 * @uri: A GnomeVFSURI.
 *
 * Retrieve the toplevel URI in @uri.
 *
 * Return value: A pointer to the toplevel URI object.
 **/
GnomeVFSToplevelURI *
gnome_vfs_uri_get_toplevel (const GnomeVFSURI *uri)
{
    const GnomeVFSURI *p;

    g_return_val_if_fail (uri != NULL, NULL);

    for (p = uri; p->parent != NULL; p = p->parent)
        ;

    return (GnomeVFSToplevelURI *) p;
}

#endif


void
pixbuf_clear(GdkPixbuf *pixbuf, GdkColor *colour)
{
	guint32 colour_rgba = ((colour->red/256)<< 24) | ((colour->green/256)<<16) | ((colour->blue/256)<<8) | (0xff); //
	gdk_pixbuf_fill(pixbuf, colour_rgba);
}


#ifdef OLD
void
pixbuf_draw_line(GdkPixbuf *pixbuf, struct _ArtDRect *pts, double line_width, GdkColor *colour)
{
  art_u8 *buffer = (art_u8*)gdk_pixbuf_get_pixels(pixbuf);
  int bufwidth  = gdk_pixbuf_get_width    (pixbuf);
  int bufheight = gdk_pixbuf_get_height   (pixbuf);
  int rowstride = gdk_pixbuf_get_rowstride(pixbuf);
  art_u32 color_fg = ((colour->red/256)<< 24) | ((colour->green/256)<<16) | ((colour->blue/256)<<8) | (0xff); //

  //define the line as a libart vector:
  ArtVpath *vec = art_new(ArtVpath, 10);
  vec[0].code = ART_MOVETO;
  vec[0].x = pts->x0;
  vec[0].y = pts->y0;
  vec[1].code = ART_LINETO;
  vec[1].x = pts->x1;
  vec[1].y = pts->y1;
  vec[2].code = ART_END;

  ArtSVP *svp = art_svp_vpath_stroke(vec,
                             ART_PATH_STROKE_JOIN_ROUND,//ArtPathStrokeJoinType join,
                             ART_PATH_STROKE_CAP_BUTT,  //ArtPathStrokeCapType cap,
                             line_width,                //double line_width,
                             1.0,                       //??????? double miter_limit,
                             1.0);                      //double flatness
  //render to buffer:
  art_rgb_svp_alpha(svp, 0, 0,
                    bufwidth, bufheight,                //width, height,
                    color_fg, buffer,
                    rowstride,                          //number of bytes in each row.
                    NULL);
  free(vec);
  free(svp);
}
#else
void
pixbuf_draw_line(cairo_t* cr, rect *pts, double line_width, GdkColor *colour)
{
  if(pts->y1 == pts->y2) return;
  //cairo_move_to (cr, pts->x1, pts->y1);
  //cairo_line_to (cr, pts->x2, pts->y2);
  cairo_rectangle(cr, pts->x1, pts->y1, pts->x2 - pts->x1 + 1, pts->y2 - pts->y1 + 1);
  cairo_fill (cr);
  cairo_stroke (cr);
}
#endif

/* Scale src down to fit in max_w, max_h and return the new pixbuf.
 * If src is small enough, then ref it and return that.
 */
#if 0
GdkPixbuf*
scale_pixbuf(GdkPixbuf *src, int max_w, int max_h)
{
	int	w, h;

	w = gdk_pixbuf_get_width(src);
	h = gdk_pixbuf_get_height(src);

	if (w <= max_w && h <= max_h)
	{
		gdk_pixbuf_ref(src);
		return src;
	}
	else
	{
		float scale_x = ((float) w) / max_w;
		float scale_y = ((float) h) / max_h;
		float scale = MAX(scale_x, scale_y);
		int dest_w = w / scale;
		int dest_h = h / scale;
		
		return gdk_pixbuf_scale_simple(src,
						MAX(dest_w, 1),
						MAX(dest_h, 1),
						GDK_INTERP_BILINEAR);
	}
}

/* Scale src up to fit in max_w, max_h and return the new pixbuf.
 * If src is that size or bigger, then ref it and return that.
 */
GdkPixbuf*
scale_pixbuf_up(GdkPixbuf *src, int max_w, int max_h)
{
	int	w, h;

	w = gdk_pixbuf_get_width(src);
	h = gdk_pixbuf_get_height(src);

	if (w == 0 || h == 0 || w >= max_w || h >= max_h)
	{
		gdk_pixbuf_ref(src);
		return src;
	}
	else
	{
		float scale_x = max_w / ((float) w);
		float scale_y = max_h / ((float) h);
		float scale = MIN(scale_x, scale_y);
		
		return gdk_pixbuf_scale_simple(src,
						w * scale,
						h * scale,
						GDK_INTERP_BILINEAR);
	}
}
#endif

/* Create a new pixbuf by colourizing 'src' to 'color'. If the function fails,
 * 'src' will be returned (with an increased reference count, so it is safe to
 * g_object_unref() the return value whether the function fails or not).
 */
GdkPixbuf*
create_spotlight_pixbuf(GdkPixbuf *src, GdkColor *color)
{
    guchar opacity = 192;
    guchar alpha = 255 - opacity;
    GdkPixbuf *dst;
    GdkColorspace colorspace;
    int width, height, src_rowstride, dst_rowstride, x, y;
    int n_channels, bps;
    int r, g, b;
    guchar *spixels, *dpixels, *src_pixels, *dst_pixels;
    gboolean has_alpha;

    has_alpha = gdk_pixbuf_get_has_alpha(src);
    colorspace = gdk_pixbuf_get_colorspace(src);
    n_channels = gdk_pixbuf_get_n_channels(src);
    bps = gdk_pixbuf_get_bits_per_sample(src);

    if ((colorspace != GDK_COLORSPACE_RGB) ||
        (!has_alpha && n_channels != 3) ||
        (has_alpha && n_channels != 4) ||
        (bps != 8))
        goto error;

    width = gdk_pixbuf_get_width(src);
    height = gdk_pixbuf_get_height(src);

    dst = gdk_pixbuf_new(colorspace, has_alpha, bps, width, height);
    if (dst == NULL)
        goto error;

    src_pixels = gdk_pixbuf_get_pixels(src);
    dst_pixels = gdk_pixbuf_get_pixels(dst);
    src_rowstride = gdk_pixbuf_get_rowstride(src);
    dst_rowstride = gdk_pixbuf_get_rowstride(dst);

    r = opacity * (color->red >> 8);
    g = opacity * (color->green >> 8);
    b = opacity * (color->blue >> 8);

    for (y = 0; y < height; y++)
    {
        spixels = src_pixels + y * src_rowstride;
        dpixels = dst_pixels + y * dst_rowstride;
        for (x = 0; x < width; x++)
        {
            *dpixels++ = (*spixels++ * alpha + r) >> 8;
            *dpixels++ = (*spixels++ * alpha + g) >> 8;
            *dpixels++ = (*spixels++ * alpha + b) >> 8;
            if (has_alpha)
                *dpixels++ = *spixels++;
        }
    }
    return dst;

error:
    g_object_ref(src);
    return src;
}


void
colour_get_style_fg(GdkColor *color, int state)
{
  //gives the default style foreground colour for the given widget state.

  //GtkStyle *style = NULL;
  //style = gtk_widget_get_default_style();
  GtkStyle *style = gtk_style_copy(gtk_widget_get_style(app.window));
  color->red   = style->fg[state].red;
  color->green = style->fg[state].green;
  color->blue  = style->fg[state].blue;
  g_free(style);
}

void
colour_get_style_bg(GdkColor *color, int state)
{
  //gives the default style foreground colour for the given widget state.

  GtkWidget *widget = app.window;

  GtkStyle *style = gtk_style_copy(gtk_widget_get_style(widget));
  //GtkStyle *style = NULL;
  //style = gtk_widget_get_default_style();
  color->red   = style->bg[state].red;
  color->green = style->bg[state].green;
  color->blue  = style->bg[state].blue;

  g_free(style);
}


void
colour_get_style_base(GdkColor *color, int state)
{
  //gives the default style base colour for the given widget state.

  GtkWidget *widget = app.window;

  GtkStyle *style = gtk_style_copy(gtk_widget_get_style(widget));
  color->red   = style->base[state].red;
  color->green = style->base[state].green;
  color->blue  = style->base[state].blue;

  g_free(style);
}


void
colour_get_style_text(GdkColor *color, int state)
{
  //gives the default style text colour for the given widget state.

  GtkWidget *widget = app.window;

  GtkStyle *style = gtk_style_copy(gtk_widget_get_style(widget));
  color->red   = style->text[state].red;
  color->green = style->text[state].green;
  color->blue  = style->text[state].blue;

  g_free(style);
}


void
colour_get_float(GdkColor* c, float* r, float* g, float* b, const unsigned char alpha)
{
	//convert GdkColor for use with Cairo.

	double _r = c->red;
	double _g = c->green;
	double _b = c->blue;

	*r = _r / 0xffff;
	*g = _g / 0xffff;
	*b = _b / 0xffff;
}


gchar*
gdkcolor_get_hexstring(GdkColor* c)
{
	return g_strdup_printf("%02x%02x%02x", c->red >> 8, c->green >> 8, c->blue >> 8);
}


void
hexstring_from_gdkcolor(char* hexstring, GdkColor* c)
{
	snprintf(hexstring, 7, "%02x%02x%02x", c->red >> 8, c->green >> 8, c->blue >> 8);
}


void
color_rgba_to_gdk(GdkColor* colour, uint32_t rgba)
{
  ASSERT_POINTER(colour, "colour");

  colour->red   = (rgba & 0xff000000) >> 16;
  colour->green = (rgba & 0x00ff0000) >> 8;
  colour->blue  = (rgba & 0x0000ff00);
}


gboolean
colour_lighter(GdkColor* lighter, GdkColor* colour)
{
	lighter->red   = MIN(colour->red   * 1.2 + 0x600, 0xffff);
	lighter->green = MIN(colour->green * 1.2 + 0x600, 0xffff);
	lighter->blue  = MIN(colour->blue  * 1.2 + 0x600, 0xffff);

	if(is_white(lighter)) return FALSE; else return TRUE;
}


gboolean
colour_darker(GdkColor* darker, GdkColor* colour)
{
	darker->red   = MAX(colour->red   * 0.8, 0x0000);
	darker->green = MAX(colour->green * 0.8, 0x0000);
	darker->blue  = MAX(colour->blue  * 0.8, 0x0000);

	if(is_black(darker)) return FALSE; else return TRUE;
}


gboolean
is_white(GdkColor* colour)
{
	if(colour->red >= 0xffff && colour->green >= 0xffff && colour->blue >= 0xffff) return TRUE;
	else return FALSE;
}


gboolean
is_black(GdkColor* colour)
{
	if(colour->red < 1 && colour->green < 1 && colour->blue < 1) return TRUE;
	else return FALSE;
}


gboolean
is_similar(GdkColor* colour1, GdkColor* colour2, char min_diff)
{
	GdkColor difference;
	difference.red   = ABS(colour1->red   - colour2->red);
	difference.green = ABS(colour1->green - colour2->green);
	difference.blue  = ABS(colour1->blue  - colour2->blue);

	if(difference.red + difference.green + difference.blue < (min_diff << 8)){
		dbg(2, "is similar! #%02x%02x%02x = %02x%02x%02x", colour1->red >> 8, colour1->green >> 8, colour1->blue >> 8, colour2->red >> 8, colour2->green >> 8, colour2->blue >> 8);
		return TRUE;
	}
	dbg(2, "not similar #%02x%02x%02x = #%02x%02x%02x", colour1->red >> 8, colour1->green >> 8, colour1->blue >> 8, colour2->red >> 8, colour2->green >> 8, colour2->blue >> 8);

	return FALSE;
}


gboolean
is_similar_rgb(unsigned colour1, unsigned colour2)
{
	char r1 = (colour1 & 0xff000000 ) >> 24; 
	char g1 = (colour1 & 0x00ff0000 ) >> 16; 
	char b1 = (colour1 & 0x0000ff00 ) >>  8; 
	char r2 = (colour1 & 0xff000000 ) >> 24; 
	char g2 = (colour1 & 0x00ff0000 ) >> 16; 
	char b2 = (colour1 & 0x0000ff00 ) >>  8; 

	int d_red = ABS(r1 - r2);
	int d_grn = ABS(g1 - g2);
	int d_blu = ABS(b1 - b2);

	if(d_red + d_grn + d_blu < 0x20){
		dbg(0, "is similar! %x = %x", r1, r2);
		return TRUE;
	}

	return FALSE;
}


void
format_time(char* length, char* milliseconds)
{
	if(!length){ errprintf("format_time()!\n"); return; }
	if(!milliseconds){ snprintf(length, 64, " "); return; }

	gchar secs_str[64] = "";
	gchar mins_str[64] = "";
	int t = atoi(milliseconds);
	int secs = t / 1000;
	int mins = secs / 60;
	if(mins){
		snprintf(mins_str, 64, "%i:", mins);
		secs = secs % 60;
		snprintf(secs_str, 64, "%02i", secs);
	}else snprintf(secs_str, 64, "%i", secs);
	
	snprintf(length, 64, "%s%s.%03i", mins_str, secs_str, t % 1000);
	//printf("format_time(): %s\n", length);
}


void
format_time_int(char* length, int milliseconds)
{
	if(!length){ errprintf("format_time()!\n"); return; }
	//if(!milliseconds){ snprintf(length, 64, " "); return; }

	snprintf(length, 64, "%i.%03i", milliseconds / 1000, milliseconds % 1000);
	//printf("format_time(): %s\n", length);
}


gint
treecell_get_row(GtkWidget *widget, GdkRectangle *cell_area)
{
	//return the row number for the cell with the given area.

	GtkTreePath *path;
	gint x = cell_area->x + 1;
	gint y = cell_area->y + 1;
	gint *cell_x = NULL; //not used.
	gint *cell_y = NULL;
	if(gtk_tree_view_get_path_at_pos(GTK_TREE_VIEW(app.view), x, y, &path, NULL, cell_x, cell_y)){
		gint *i;
		i = gtk_tree_path_get_indices(path);
		printf("treecell_get_row() i[0]=%i.\n", i[0]);
		gint row = i[0];
		gtk_tree_path_free(path);
		return row;
	}
	else warnprintf("treecell_get_row() no row found.\n");
	return 0;
}


void
statusbar_print(int n, char *s)
{
  //prints to one of the root statusbar contexts.
  GtkWidget *statusbar = NULL;
  if     (n==1) statusbar = app.statusbar;
  else if(n==2) statusbar = app.statusbar2;
  else { errprintf("statusbar_print(): bad statusbar index (%i)\n", n); n=1; }

  if((unsigned int)statusbar<1024) return; //window may not be open.

  gchar buff[128];
  gint cid = gtk_statusbar_get_context_id(GTK_STATUSBAR(statusbar), "dummy");
  snprintf(buff, 128, "  %s", s); //experimental padding.
  gtk_statusbar_push(GTK_STATUSBAR(statusbar), cid, buff);
}


void
statusbar_printf(int n, char* fmt, ...)
{
  if(n<1||n>3) n = 1;

  char s[128];
  va_list argp;

  va_start(argp, fmt);
  vsnprintf(s, 127, fmt, argp);
  va_end(argp);

  GtkWidget *statusbar = NULL;
  if     (n==1) statusbar = app.statusbar;
  else if(n==2) statusbar = app.statusbar2;
  else { errprintf("statusbar_print(): bad statusbar index (%i)\n", n); n=1; }

  if((unsigned)statusbar<1024) return; //window may not be open.

  gint cid = gtk_statusbar_get_context_id(GTK_STATUSBAR(statusbar), "dummy");
  gtk_statusbar_push(GTK_STATUSBAR(statusbar), cid, s);
}



//from Rox:

/* Convert a list of URIs as a string into a GList of EscapedPath URIs.
 * No unescaping is done.
 * Lines beginning with # are skipped.
 * The text block passed in is zero terminated (after the final CRLF)
 */
GList*
uri_list_to_glist(const char* uri_list)
{
	GList* list = NULL;

	while (*uri_list){
		char* linebreak;
		int	length;

		linebreak = strchr(uri_list, 13);

		if (!linebreak){ errprintf ("uri_list_to_glist(): %s: %s", "missing line break in text/uri-list data", uri_list); return list; }
		if (linebreak[1] != 10){
			errprintf ("uri_list_to_glist(): %s", "Incorrect line break in text/uri-list data");
			return list;
		}

		length = linebreak - uri_list;

		if (length && uri_list[0] != '#')
			list = g_list_append(list, g_strndup(uri_list, length));

		uri_list = linebreak + 2;
	}

	return list;
}


void
uri_list_free(GList* list)
{
  GList* l = list;
  for(;l;l=l->next){
    if(l->data) g_free(l->data);
  }
  g_list_free(list);
}


const gchar*
vfs_get_method_string(const gchar *substring, gchar **method_string)
{
	const gchar *p;
	char *method;
	
	for (p = substring;
	     g_ascii_isalnum (*p) || *p == '+' || *p == '-' || *p == '.';
	     p++)
		;

	if (*p == ':') {
		/* Found toplevel method specification.  */
		method = g_strndup (substring, p - substring);
		*method_string = g_ascii_strdown (method, -1);
		g_free (method);
		p++;
	} else {
		*method_string = g_strdup ("file");
		p = substring;
	}
	return p;
}


static int
hex_to_int (gchar c)
{
    return  c >= '0' && c <= '9' ? c - '0'
        : c >= 'A' && c <= 'F' ? c - 'A' + 10
        : c >= 'a' && c <= 'f' ? c - 'a' + 10
        : -1;
}


static int
unescape_character (const char *scanner)
{
    int first_digit;
    int second_digit;

    first_digit = hex_to_int (*scanner++);
    if (first_digit < 0) {
        return -1;
    }

    second_digit = hex_to_int (*scanner++);
    if (second_digit < 0) {
        return -1;
    }

    return (first_digit << 4) | second_digit;
}


/**
 * gnome_vfs_unescape_string:
 * @escaped_string: an escaped URI, path, or other string
 * @illegal_characters: a string containing a sequence of characters
 * considered "illegal" to be escaped, '\0' is automatically in this list.
 *
 * Decodes escaped characters (i.e. PERCENTxx sequences) in @escaped_string.
 * Characters are encoded in PERCENTxy form, where xy is the ASCII hex code
 * for character 16x+y.
 *
 * Return value: a newly allocated string with the unescaped
 * equivalents, or %NULL if @escaped_string contained an escaped
 * encoding of one of the characters in @illegal_characters.
 **/
char*
vfs_unescape_string (const gchar *escaped_string, const gchar *illegal_characters)
{
    const gchar *in;
    gchar *out, *result;
    gint character;

    if (escaped_string == NULL) {
        return NULL;
    }

    result = g_malloc (strlen (escaped_string) + 1);

    out = result;
    for (in = escaped_string; *in != '\0'; in++) {
        character = *in;
        if (*in == HEX_ESCAPE) {
            character = unescape_character (in + 1);

            /* Check for an illegal character. We consider '\0' illegal here. */
            if (character <= 0
                || (illegal_characters != NULL
                && strchr (illegal_characters, (char)character) != NULL)) {
                g_free (result);
                return NULL;
            }
            in += 2;
        }
        *out++ = (char)character;
    }

    *out = '\0';
    g_assert (out - result <= strlen (escaped_string));
    return result;

}



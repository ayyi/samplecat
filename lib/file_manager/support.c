/**
* +----------------------------------------------------------------------+
* | This file is part of the Ayyi project. http://ayyi.org               |
* | copyright (C) 2011-2017 Tim Orford <tim@orford.org>                  |
* +----------------------------------------------------------------------+
* | This program is free software; you can redistribute it and/or modify |
* | it under the terms of the GNU General Public License version 3       |
* | as published by the Free Software Foundation.                        |
* +----------------------------------------------------------------------+
*
*/
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <netdb.h>
#include <pwd.h>
#include <grp.h>
#include <dirent.h>
#include <sys/param.h>
#include <errno.h>
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#include <gtk/gtk.h>
#pragma GCC diagnostic warning "-Wdeprecated-declarations"
#include "debug/debug.h"
#include "file_manager/typedefs.h"
#include "support.h"

#define HEX_ESCAPE '%'

// printf format string to print file sizes
#ifdef LARGE_FILE_SUPPORT
# define SIZE_FMT G_GINT64_MODIFIER "d"
#else
# define SIZE_FMT G_GINT32_MODIFIER "d"
#endif

static const char* action_leaf = NULL;
static int from_parent = 0;

static GHashTable* uid_hash = NULL;	// UID -> User name
static GHashTable* gid_hash = NULL;	// GID -> Group name

static void        do_move         (const char* path, const char* dest);
static const char* make_dest_path  (const char* object, const char* dir);
static void        send_check_path (const gchar* path);
static void        check_flags     ();
static void        process_flag    (char flag);

static void        MD5Transform    (guint32 buf[4], guint32 const in[16]);


/*
 *  Like g_strdup, but does realpath() too (if possible)
 */
char*
pathdup(const char* path)
{
	char real[MAXPATHLEN];

	g_return_val_if_fail(path != NULL, NULL);

	if (realpath(path, real))
		return g_strdup(real);

	return g_strdup(path);
}


/*
 *  Join the path to the leaf (adding a / between them) and
 *  return a pointer to a static buffer with the result. Buffer is valid
 *  until the next call to make_path.
 *  The return value may be used as 'dir' for the next call.
 */
const guchar*
make_path (const char* dir, const char* leaf)
{
	static GString* buffer = NULL;

	if (!buffer)
		buffer = g_string_new(NULL);

	g_return_val_if_fail(dir, (guchar*)buffer->str);
	g_return_val_if_fail(leaf, (guchar*)buffer->str);

	if (buffer->str != dir)
		g_string_assign(buffer, dir);

	if (dir[0] != '/' || dir[1] != '\0')
		g_string_append_c(buffer, '/');	// For anything except "/"

	g_string_append(buffer, leaf);

	return (guchar*)buffer->str;
}


/*
 *  Return our complete host name for DND
 */
const char*
our_host_name_for_dnd (void)
{
#if 0
	if (o_dnd_no_hostnames.int_value)
		return "";
#endif
	return our_host_name();
}


/*
 *  Return our complete host name, unconditionally
 */
const char*
our_host_name (void)
{
	static char* name = NULL;

	if (!name)
	{
		char buffer[4096];

		if (gethostname(buffer, 4096) == 0)
		{
			/* gethostname doesn't always return the full name... */
			struct hostent *ent;

			buffer[4095] = '\0';
			ent = gethostbyname(buffer);
			name = g_strdup(ent ? ent->h_name : buffer);
		}
		else
		{
			g_warning("gethostname() failed - using localhost\n");
			name = g_strdup("localhost");
		}
	}

	return name;
}


const char*
user_name (uid_t uid)
{
	if (!uid_hash)
		uid_hash = g_hash_table_new(NULL, NULL);

	const char* retval = g_hash_table_lookup(uid_hash, GINT_TO_POINTER(uid));

	if (!retval) {
		struct passwd* passwd = getpwuid(uid);
		retval = passwd
			? g_strdup(passwd->pw_name)
			: g_strdup_printf("[%d]", (int) uid);
		g_hash_table_insert(uid_hash, GINT_TO_POINTER(uid), (gchar*)retval);
	}

	return retval;
}


const char*
group_name (gid_t gid)
{
	const char *retval;

	if (!gid_hash)
		gid_hash = g_hash_table_new(NULL, NULL);

	retval = g_hash_table_lookup(gid_hash, GINT_TO_POINTER(gid));

	if (!retval) {
		struct group* group = getgrgid(gid);
		retval = group
			? g_strdup(group->gr_name)
			: g_strdup_printf("[%d]", (int) gid);
		g_hash_table_insert(gid_hash, GINT_TO_POINTER(gid), (gchar*)retval);
	}

	return retval;
}


/*
 *  Return a string in the form '23 M' in a static buffer valid until
 *  the next call.
 */
const char*
format_size (off_t size)
{
	static	char *buffer = NULL;
	const char	*units;

	if (size >= PRETTY_SIZE_LIMIT)
	{
		size += 1023;
		size >>= 10;
		if (size >= PRETTY_SIZE_LIMIT)
		{
			size += 1023;
			size >>= 10;
			if (size >= PRETTY_SIZE_LIMIT)
			{
				size += 1023;
				size >>= 10;
				units = "G";
			}
			else
				units = "M";
		}
		else
			units = "K";
	}
	else
		units = "B";

	g_free(buffer);
	buffer = g_strdup_printf("%" SIZE_FMT " %s", (int)size, units);

	return buffer;
}


/*
 *  Converts a file's mode to a string. Result is a pointer
 *  to a static buffer, valid until the next call.
 */
const char*
pretty_permissions(mode_t m)
{
	static char buffer[] = "rwx,rwx,rwx/UG"
#ifdef S_ISVTX
	     "T"
#endif
	     ;

	buffer[0]  = m & S_IRUSR ? 'r' : '-';
	buffer[1]  = m & S_IWUSR ? 'w' : '-';
	buffer[2]  = m & S_IXUSR ? 'x' : '-';

	buffer[4]  = m & S_IRGRP ? 'r' : '-';
	buffer[5]  = m & S_IWGRP ? 'w' : '-';
	buffer[6]  = m & S_IXGRP ? 'x' : '-';

	buffer[8]  = m & S_IROTH ? 'r' : '-';
	buffer[9]  = m & S_IWOTH ? 'w' : '-';
	buffer[10] = m & S_IXOTH ? 'x' : '-';

	buffer[12] = m & S_ISUID ? 'U' : '-';
	buffer[13] = m & S_ISGID ? 'G' : '-';
#ifdef S_ISVTX
        buffer[14] = m & S_ISVTX ? 'T' : '-';
#endif

	return buffer;
}


/*
 *  Format the time nicely.
 *  g_free() the result.
 */
char*
pretty_time (const time_t *time)
{
	char time_buf[32];

	if (strftime(time_buf, sizeof(time_buf), TIME_FORMAT, localtime(time)) == 0) time_buf[0]= 0;

	return to_utf8(time_buf);
}


/*
 *  'word' has all special characters escaped so that it may be inserted
 *  into a shell command.
 *  Eg: 'My Dir?' becomes 'My\ Dir\?'. g_free() the result.
 */
guchar*
shell_escape (const guchar* word)
{
	GString* tmp = g_string_new(NULL);

	while (*word) {
		if (strchr(" ?*['\"$~\\|();!`&", *word))
			g_string_append_c(tmp, '\\');
		g_string_append_c(tmp, *word);
		word++;
	}

	guchar* retval = (guchar*)tmp->str;
	g_string_free(tmp, FALSE);
	return retval;
}


/*
 *  Return the pathname that this symlink points to.
 *  NULL on error (not a symlink, path too long) and errno set.
 *  g_free() the result.
 */
char*
readlink_dup(const char* source)
{
	char path[MAXPATHLEN + 1];

	int got = readlink(source, path, MAXPATHLEN);
	if (got < 0 || got > MAXPATHLEN)
		return NULL;

	return g_strndup(path, got);
}


/*
 *  Convert string 'src' from the current locale to UTF-8
 */
gchar*
to_utf8 (const gchar* src)
{
	if (!src)
		return NULL;

	gchar* retval = g_locale_to_utf8(src, -1, NULL, NULL, NULL);
	if (!retval)
		retval = g_convert_with_fallback(src, -1, "utf-8", "iso-8859-1", "?", NULL, NULL, NULL);

	return retval ? retval : g_strdup(src);
}


/*
 * This code implements the MD5 message-digest algorithm.
 * The algorithm is due to Ron Rivest. The original code was
 * written by Colin Plumb in 1993, and put in the public domain.
 *
 * Modified to use glib datatypes. Put under GPL to simplify
 * licensing for ROX-Filer. Taken from Debian's dpkg package.
 */

#define md5byte unsigned char

typedef struct _MD5Context MD5Context;

struct _MD5Context {
	guint32 buf[4];
	guint32 bytes[2];
	guint32 in[16];
};

#if G_BYTE_ORDER == G_BIG_ENDIAN
static void byteSwap(guint32 *buf, unsigned words)
{
	md5byte *p = (md5byte *)buf;

	do {
		*buf++ = (guint32)((unsigned)p[3] << 8 | p[2]) << 16 |
			((unsigned)p[1] << 8 | p[0]);
		p += 4;
	} while (--words);
}
#else
#define byteSwap(buf,words)
#endif


/*
 * Start MD5 accumulation. Set bit count to 0 and buffer to mysterious
 * initialization constants.
 */
static void
MD5Init(MD5Context *ctx)
{
	ctx->buf[0] = 0x67452301;
	ctx->buf[1] = 0xefcdab89;
	ctx->buf[2] = 0x98badcfe;
	ctx->buf[3] = 0x10325476;

	ctx->bytes[0] = 0;
	ctx->bytes[1] = 0;
}


/*
 * Update context to reflect the concatenation of another buffer full
 * of bytes.
 */
static void
MD5Update(MD5Context *ctx, md5byte const *buf, unsigned len)
{
	guint32 t;

	/* Update byte count */

	t = ctx->bytes[0];
	if ((ctx->bytes[0] = t + len) < t)
		ctx->bytes[1]++;	/* Carry from low to high */

	t = 64 - (t & 0x3f);	/* Space available in ctx->in (at least 1) */
	if (t > len) {
		memcpy((md5byte *)ctx->in + 64 - t, buf, len);
		return;
	}
	/* First chunk is an odd size */
	memcpy((md5byte *)ctx->in + 64 - t, buf, t);
	byteSwap(ctx->in, 16);
	MD5Transform(ctx->buf, ctx->in);
	buf += t;
	len -= t;

	/* Process data in 64-byte chunks */
	while (len >= 64) {
		memcpy(ctx->in, buf, 64);
		byteSwap(ctx->in, 16);
		MD5Transform(ctx->buf, ctx->in);
		buf += 64;
		len -= 64;
	}

	/* Handle any remaining bytes of data. */
	memcpy(ctx->in, buf, len);
}

/*
 * Final wrapup - pad to 64-byte boundary with the bit pattern
 * 1 0* (64-bit count of bits processed, MSB-first)
 * Returns the newly allocated string of the hash.
 */
static char*
MD5Final(MD5Context *ctx)
{
	char *retval;
	int i;
	int count = ctx->bytes[0] & 0x3f;	/* Number of bytes in ctx->in */
	md5byte *p = (md5byte *)ctx->in + count;
	guint8	*bytes;

	/* Set the first char of padding to 0x80.  There is always room. */
	*p++ = 0x80;

	/* Bytes of padding needed to make 56 bytes (-8..55) */
	count = 56 - 1 - count;

	if (count < 0) {	/* Padding forces an extra block */
		memset(p, 0, count + 8);
		byteSwap(ctx->in, 16);
		MD5Transform(ctx->buf, ctx->in);
		p = (md5byte *)ctx->in;
		count = 56;
	}
	memset(p, 0, count);
	byteSwap(ctx->in, 14);

	/* Append length in bits and transform */
	ctx->in[14] = ctx->bytes[0] << 3;
	ctx->in[15] = ctx->bytes[1] << 3 | ctx->bytes[0] >> 29;
	MD5Transform(ctx->buf, ctx->in);

	byteSwap(ctx->buf, 4);

	retval = g_malloc(33);
	bytes = (guint8 *) ctx->buf;
	for (i = 0; i < 16; i++)
		sprintf(retval + (i * 2), "%02x", bytes[i]);
	retval[32] = '\0';

	return retval;
}

# ifndef ASM_MD5

/* The four core functions - F1 is optimized somewhat */

/* #define F1(x, y, z) (x & y | ~x & z) */
#define F1(x, y, z) (z ^ (x & (y ^ z)))
#define F2(x, y, z) F1(z, x, y)
#define F3(x, y, z) (x ^ y ^ z)
#define F4(x, y, z) (y ^ (x | ~z))

/* This is the central step in the MD5 algorithm. */
#define MD5STEP(f,w,x,y,z,in,s) \
	 (w += f(x,y,z) + in, w = (w<<s | w>>(32-s)) + x)

/*
 * The core of the MD5 algorithm, this alters an existing MD5 hash to
 * reflect the addition of 16 longwords of new data.  MD5Update blocks
 * the data and converts bytes into longwords for this routine.
 */
static void
MD5Transform(guint32 buf[4], guint32 const in[16])
{
	register guint32 a, b, c, d;

	a = buf[0];
	b = buf[1];
	c = buf[2];
	d = buf[3];

	MD5STEP(F1, a, b, c, d, in[0] + 0xd76aa478, 7);
	MD5STEP(F1, d, a, b, c, in[1] + 0xe8c7b756, 12);
	MD5STEP(F1, c, d, a, b, in[2] + 0x242070db, 17);
	MD5STEP(F1, b, c, d, a, in[3] + 0xc1bdceee, 22);
	MD5STEP(F1, a, b, c, d, in[4] + 0xf57c0faf, 7);
	MD5STEP(F1, d, a, b, c, in[5] + 0x4787c62a, 12);
	MD5STEP(F1, c, d, a, b, in[6] + 0xa8304613, 17);
	MD5STEP(F1, b, c, d, a, in[7] + 0xfd469501, 22);
	MD5STEP(F1, a, b, c, d, in[8] + 0x698098d8, 7);
	MD5STEP(F1, d, a, b, c, in[9] + 0x8b44f7af, 12);
	MD5STEP(F1, c, d, a, b, in[10] + 0xffff5bb1, 17);
	MD5STEP(F1, b, c, d, a, in[11] + 0x895cd7be, 22);
	MD5STEP(F1, a, b, c, d, in[12] + 0x6b901122, 7);
	MD5STEP(F1, d, a, b, c, in[13] + 0xfd987193, 12);
	MD5STEP(F1, c, d, a, b, in[14] + 0xa679438e, 17);
	MD5STEP(F1, b, c, d, a, in[15] + 0x49b40821, 22);

	MD5STEP(F2, a, b, c, d, in[1] + 0xf61e2562, 5);
	MD5STEP(F2, d, a, b, c, in[6] + 0xc040b340, 9);
	MD5STEP(F2, c, d, a, b, in[11] + 0x265e5a51, 14);
	MD5STEP(F2, b, c, d, a, in[0] + 0xe9b6c7aa, 20);
	MD5STEP(F2, a, b, c, d, in[5] + 0xd62f105d, 5);
	MD5STEP(F2, d, a, b, c, in[10] + 0x02441453, 9);
	MD5STEP(F2, c, d, a, b, in[15] + 0xd8a1e681, 14);
	MD5STEP(F2, b, c, d, a, in[4] + 0xe7d3fbc8, 20);
	MD5STEP(F2, a, b, c, d, in[9] + 0x21e1cde6, 5);
	MD5STEP(F2, d, a, b, c, in[14] + 0xc33707d6, 9);
	MD5STEP(F2, c, d, a, b, in[3] + 0xf4d50d87, 14);
	MD5STEP(F2, b, c, d, a, in[8] + 0x455a14ed, 20);
	MD5STEP(F2, a, b, c, d, in[13] + 0xa9e3e905, 5);
	MD5STEP(F2, d, a, b, c, in[2] + 0xfcefa3f8, 9);
	MD5STEP(F2, c, d, a, b, in[7] + 0x676f02d9, 14);
	MD5STEP(F2, b, c, d, a, in[12] + 0x8d2a4c8a, 20);

	MD5STEP(F3, a, b, c, d, in[5] + 0xfffa3942, 4);
	MD5STEP(F3, d, a, b, c, in[8] + 0x8771f681, 11);
	MD5STEP(F3, c, d, a, b, in[11] + 0x6d9d6122, 16);
	MD5STEP(F3, b, c, d, a, in[14] + 0xfde5380c, 23);
	MD5STEP(F3, a, b, c, d, in[1] + 0xa4beea44, 4);
	MD5STEP(F3, d, a, b, c, in[4] + 0x4bdecfa9, 11);
	MD5STEP(F3, c, d, a, b, in[7] + 0xf6bb4b60, 16);
	MD5STEP(F3, b, c, d, a, in[10] + 0xbebfbc70, 23);
	MD5STEP(F3, a, b, c, d, in[13] + 0x289b7ec6, 4);
	MD5STEP(F3, d, a, b, c, in[0] + 0xeaa127fa, 11);
	MD5STEP(F3, c, d, a, b, in[3] + 0xd4ef3085, 16);
	MD5STEP(F3, b, c, d, a, in[6] + 0x04881d05, 23);
	MD5STEP(F3, a, b, c, d, in[9] + 0xd9d4d039, 4);
	MD5STEP(F3, d, a, b, c, in[12] + 0xe6db99e5, 11);
	MD5STEP(F3, c, d, a, b, in[15] + 0x1fa27cf8, 16);
	MD5STEP(F3, b, c, d, a, in[2] + 0xc4ac5665, 23);

	MD5STEP(F4, a, b, c, d, in[0] + 0xf4292244, 6);
	MD5STEP(F4, d, a, b, c, in[7] + 0x432aff97, 10);
	MD5STEP(F4, c, d, a, b, in[14] + 0xab9423a7, 15);
	MD5STEP(F4, b, c, d, a, in[5] + 0xfc93a039, 21);
	MD5STEP(F4, a, b, c, d, in[12] + 0x655b59c3, 6);
	MD5STEP(F4, d, a, b, c, in[3] + 0x8f0ccc92, 10);
	MD5STEP(F4, c, d, a, b, in[10] + 0xffeff47d, 15);
	MD5STEP(F4, b, c, d, a, in[1] + 0x85845dd1, 21);
	MD5STEP(F4, a, b, c, d, in[8] + 0x6fa87e4f, 6);
	MD5STEP(F4, d, a, b, c, in[15] + 0xfe2ce6e0, 10);
	MD5STEP(F4, c, d, a, b, in[6] + 0xa3014314, 15);
	MD5STEP(F4, b, c, d, a, in[13] + 0x4e0811a1, 21);
	MD5STEP(F4, a, b, c, d, in[4] + 0xf7537e82, 6);
	MD5STEP(F4, d, a, b, c, in[11] + 0xbd3af235, 10);
	MD5STEP(F4, c, d, a, b, in[2] + 0x2ad7d2bb, 15);
	MD5STEP(F4, b, c, d, a, in[9] + 0xeb86d391, 21);

	buf[0] += a;
	buf[1] += b;
	buf[2] += c;
	buf[3] += d;
}

# endif /* ASM_MD5 */


char*
md5_hash (const char* message)
{
	MD5Context ctx;

	MD5Init(&ctx);
	MD5Update(&ctx, (guchar*)message, strlen(message));
	return MD5Final(&ctx);
}


/*
 *  g_free() every element in the list, then free the list itself and
 *  NULL the pointer to the list.
 */
void
destroy_glist (GList** list)
{
	GList* l = *list;
	g_list_foreach(l, (GFunc)g_free, NULL);
	g_list_free(l);
	*list = NULL;
}


void
null_g_free (gpointer p)
{
	g_free(*(gpointer *)p);
	*(gpointer *)p = NULL;
}


typedef struct _CollatePart CollatePart;

struct _CollateKey {
	CollatePart *parts;
	gboolean caps;
};

struct _CollatePart {
	guchar *text;	/* NULL => end of list */
	long number;
};

/* Break 'name' (a UTF-8 string) down into a list of (text, number) pairs.
 * The text parts processed for collating. This allows any two names to be
 * quickly compared later for intelligent sorting (comparing names is
 * speed-critical).
 */
CollateKey*
collate_key_new (const guchar* name)
{
	const guchar *i;
	guchar *to_free = NULL;
	GArray *array;
	CollatePart new;
	CollateKey *retval;
	char *tmp;

	g_return_val_if_fail(name != NULL, NULL);

	array = g_array_new(FALSE, FALSE, sizeof(CollatePart));

	/* Ensure valid UTF-8 */
	if (!g_utf8_validate((gchar*)name, -1, NULL))
	{
		to_free = (guchar*)to_utf8((gchar*)name);
		name = to_free;
	}

	retval = g_new(CollateKey, 1);
	retval->caps = g_unichar_isupper(g_utf8_get_char((gchar*)name));

	for (i = name; *i; i = (guchar*)g_utf8_next_char(i))
	{
		gunichar first_char;

		/* We're in a (possibly blank) text section starting at 'name'.
		 * Find the end of it (the next digit, or end of string).
		 * Note: g_ascii_isdigit takes char, not unichar, while
		 * g_unicode_isdigit returns true for non ASCII digits.
		 */
		first_char = g_utf8_get_char((gchar*)i);
		if (first_char >= '0' && first_char <= '9')
		{
			char *endp;

			/* i -> first digit character */
			tmp = g_utf8_strdown((char*)name, i - name);
			new.text = (guchar*)g_utf8_collate_key(tmp, -1);
			g_free(tmp);
			new.number = strtol((char*)i, &endp, 10);

			g_array_append_val(array, new);

			g_return_val_if_fail(endp > (char *) i, NULL);

			name = (guchar*)endp;
			i = name - 1;
		}
	}

	tmp = (char*)g_utf8_strdown((char*)name, i - name);
	new.text = (guchar*)g_utf8_collate_key(tmp, -1);
	g_free(tmp);
	new.number = -1;
	g_array_append_val(array, new);

	new.text = NULL;
	g_array_append_val(array, new);

	retval->parts = (CollatePart *) array->data;
	g_array_free(array, FALSE);

	if (to_free)
		g_free(to_free);	/* Only taken for invalid UTF-8 */

	return retval;
}

void
collate_key_free(CollateKey *key)
{
	CollatePart *part;

	for (part = key->parts; part->text; part++) g_free(part->text);
	g_free(key->parts);
	g_free(key);
}

int
collate_key_cmp(const CollateKey *key1, const CollateKey *key2, gboolean caps_first)
{
	g_return_val_if_fail(key1, 0);
	g_return_val_if_fail(key2, 0);

	CollatePart *n1 = key1->parts;
	CollatePart *n2 = key2->parts;
	int r;

	if (caps_first)
	{
		if (key1->caps && !key2->caps) return -1;
		else if (key2->caps && !key1->caps) return 1;
	}

	while (1)
	{
		if (!n1->text)
			return n2->text ? -1 : 0;
		if (!n2->text)
			return 1;
		r = strcmp((char*)n1->text, (char*)n2->text);
		if (r)
			return r;

		if (n1->number < n2->number)
			return -1;
		if (n1->number > n2->number)
			return 1;

		n1++;
		n2++;
	}
	return 0;
}


int
stat_with_timeout (const char* path, struct stat* info)
{
	pid_t child = fork();
	if (child < 0)
	{
		g_warning("stat_with_timeout: fork(): %s", g_strerror(errno));
		return -1;
	}

	if (child == 0)
	{
		/* Child */
		alarm(3);
		_exit(stat(path, info) ? 1 : 0);
	}

	int status;
	waitpid(child, &status, 0);

	gboolean retval = (status == 0)
		? stat(path, info)
		: -1;

	return retval;
}


/*
 *  Returns an array listing all the names in the directory 'path'.
 *  The array is sorted.
 *  '.' and '..' are skipped.
 *  On error, the error is reported with g_warning and NULL is returned.
 */
GPtrArray*
list_dir (const guchar* path)
{
	GError* error = NULL;
	GDir* dir = g_dir_open((char*)path, 0, &error);
	if (error)
	{
		g_warning("Can't list directory:\n%s", error->message);
		g_error_free(error);
		return NULL;
	}

	GPtrArray* names = g_ptr_array_new();

	const char* leaf;
	while ((leaf = g_dir_read_name(dir))) {
		if (leaf[0] != '.')
			g_ptr_array_add(names, g_strdup(leaf));
	}

	g_dir_close(dir);

	g_ptr_array_sort(names, strcmp2);

	return names;
}


/*
 *  Used as the sort function for sorting GPtrArrays
 */
gint
strcmp2 (gconstpointer a, gconstpointer b)
{
	const char *aa = *(char **) a;
	const char *bb = *(char **) b;

	gchar* aaa = g_utf8_casefold(aa, -1);
	gchar* bbb = g_utf8_casefold(bb, -1);
	int diff = strcmp(aaa, bbb);
	g_free(aaa);
	g_free(bbb);

	return diff;
}


#if 0
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
#endif //0


/* TRUE if `sub' is (or would be) an object inside the directory `parent',
 * (or the two are the same item/directory).
 * FALSE if parent doesn't exist.
 */
gboolean
is_sub_dir(const char* sub_obj, const char* parent)
{
	struct stat parent_info;

	if (lstat(parent, &parent_info)) return FALSE; /* Parent doesn't exist */

	/* For checking Copy/Move operations do a realpath first on sub
	 * (the destination), since copying into a symlink is the same as
	 * copying into the thing it points to. Don't realpath 'parent' though;
	 * copying a symlink just makes a new symlink.
	 * 
	 * When checking if an icon depends on a file (parent), use realpath on
	 * sub (the icon) too.
	 */
	char* sub = pathdup(sub_obj);
	
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


/* 'from' and 'to' are complete pathnames of files (not dirs or symlinks).
 * This spawns 'cp' to do the copy.
 * Use with a wrapper fn where neccesary to check files and report errors.
 *
 * Returns an error string, or NULL on success. g_free() the result.
 */
guchar*
file_copy(const guchar* from, const guchar* to)
{
	const char *argv[] = {"cp", "-pRf", NULL, NULL, NULL};

	argv[2] = (char*)from;
	argv[3] = (char*)to;

	return (guchar*)fork_exec_wait(argv);
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
			//statusbar_print(1, "file already exists.");
			dbg(0, "FIXME add print callback to application");
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
		dbg(0, "FIXME add print callback to application");
		//statusbar_printf(1, "!%s: Failed to move %s as %s", err, path, dest);
		g_free(err);
	}
	else
	{
		send_check_path(dest);

		//if (is_dir) send_mount_path(path);
		if (is_dir) dbg(0, "?%s", path);
		//else        statusbar_printf(1, "file '%s' moved.", path);
		else dbg(0, "FIXME add print callback to application");
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


/* Fork and exec argv. Wait and return the child's exit status.
 * -1 if spawn fails.
 * Returns the error string from the command if any, or NULL on success.
 * If the process returns a non-zero exit status without producing a message,
 * a suitable message is created.
 * g_free() the result.
 */
char*
fork_exec_wait (const char** argv)
{
	int	status;
	gchar* errors = NULL;
	GError* error = NULL;

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

GList*
uri_list_from_text(gchar *data, gint files_only)
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


/*
 *  Create a new pixbuf by colourizing 'src' to 'color'. If the function fails,
 *  'src' will be returned (with an increased reference count, so it is safe to
 *  g_object_unref() the return value whether the function fails or not).
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

#if 0
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

#endif //0

gchar*
path_to_utf8 (const gchar *path)
{
	gchar *utf8;
	GError *error = NULL;

	if (!path) return NULL;

	utf8 = g_filename_to_utf8(path, -1, NULL, NULL, &error);
	if (error)
		{
		printf("Unable to convert filename to UTF-8:\n%s\n%s\n", path, error->message);
		g_error_free(error);
		//encoding_dialog(path);
		}
	if (!utf8)
		{
		/* just let it through, but bad things may happen */
		utf8 = g_strdup(path);
		}

	return utf8;
}

void
fm__escape_for_menu(char* string)
{
	//gtk uses underscores as nmemonic identifiers, so any literal underscores need to be escaped.

	//no memory is allocated. Caller must make sure @string is long enough to hold an expanded string.

	const char* oldpiece = "_";
	const char* newpiece = "__";
	int str_index, newstr_index, oldpiece_index, end, new_len, old_len, cpy_len;
	char* c;
	static char newstring[256];

	if((c = (char *) strstr(string, oldpiece)) == NULL) return;

	new_len        = strlen(newpiece);
	old_len        = strlen(oldpiece);
	end            = strlen(string)   - old_len;
	oldpiece_index = c - string;

	newstr_index = 0;
	str_index = 0;
	while(str_index <= end && c != NULL){
		//copy characters from the left of matched pattern occurence
		cpy_len = oldpiece_index-str_index;
		strncpy(newstring+newstr_index, string+str_index, cpy_len);
		newstr_index += cpy_len;
		str_index    += cpy_len;

		//copy replacement characters instead of matched pattern
		strcpy(newstring+newstr_index, newpiece);
		newstr_index += new_len;
		str_index    += old_len;

		//check for another pattern match
		if((c = (char *) strstr(string+str_index, oldpiece)) != NULL) oldpiece_index = c - string;
	}

	//copy remaining characters from the right of last matched pattern
	strcpy(newstring+newstr_index, string+str_index);
	strcpy(string, newstring);
}


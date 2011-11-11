/*

Copyright (C) Tim Orford 2007-2010

This software is licensed under the GPL. See accompanying file COPYING.

*/
#include "config.h"
#ifdef USE_TRACKER
#include <stdio.h>
#include <stdlib.h>
#define __USE_GNU
#include <string.h>
#include <unistd.h>
#include <getopt.h>
#include <sndfile.h>
#include <gtk/gtk.h>
#include <libtracker-client/tracker-client.h>

#include "typedefs.h"
#include "support.h"
#include "mimetype.h"
#include "listmodel.h"
#include "src/types.h"
#include "db/tracker.h"

extern int debug;

//static void tracker__on_dbus_timeout();
static void       clear_result              ();
static gchar*     get_fts_string            (GStrv, gboolean);
static GPtrArray* get_tags_for_file         (const char*);
static gint       str_in_array              (const gchar* str, gchar** array);
static gchar*     get_escaped_sparql_string (const gchar*);
static GStrv      get_uris                  (GStrv files);
static gchar*     get_filter_string         (GStrv files, gboolean files_are_urns, const gchar* tag);
static GPtrArray* get_file_urns             (TrackerClient*, GStrv uris, const gchar* tag);
static GPtrArray* get_tag_urns_for_label    (const char* label);

static TrackerClient* tc = NULL;
static Sample result;
struct _iter
{
	int        idx;
	GPtrArray* results;
};
struct _iter iter = {};


gboolean
tracker__init(gpointer callback)
{
	PF;
	//note: trackerd doesnt have to be running - it will be auto-started.
	if((tc = tracker_client_new(0, G_MAXINT))){
		if(callback){
			void (*fn)() = callback;
			fn();
		}
		//tracker_disconnect (tc); //FIXME we dont disconnect!
	}
	else pwarn("cant connect to tracker daemon.");
	return IDLE_STOP;
}


void
tracker__disconnect()
{
}


int
tracker__insert(Sample* sample, MIME_type* mimetype)
{
	return 0;
}


gboolean
tracker__delete_row(int id)
{
	return false;
}


gboolean
tracker__update_pixbuf(Sample* sample)
{
	return true;
}


static gint
str_in_array (const gchar *str, gchar **array)
{
	gchar **st;

	gint i; for(i = 0, st = array; *st; st++, i++){
		if(strcasecmp(*st, str) == 0){
			return i;
		}
	}

	return -1;
}


#define MAX_DISPLAY_ROWS 1000 //FIXME also defined in main.h
gboolean
tracker__search_iter_new(char* search, char* dir, const char* category, int* n_results)
{
	g_return_val_if_fail(tc, false);

	dbg(0, "search=%s category=%s.", search, category);

	char* tag_filter = "";
	if(category && category[0]){
		gwarn("TODO: category is ignored");

//TODO does this work?
		//tag_filter = g_strdup_printf("  nao:hasTag [ nao:prefLabel \"%s\" ] ; ", category);

#if 0
		//get tag urns:
		GPtrArray* tags = get_tag_urns_for_label(category);
		int i; for(i=0; i<tags->len; i++){
			gchar** row = g_ptr_array_index (tags, i);
			gchar* tag_urn = row[0];
			dbg(2, "  %s", tag_urn);

			//gchar* filter = get_filter_string (urns_strv, TRUE, tag_urn);
		}
#endif
	}

	gboolean use_or_operator = false;
	gint search_offset = 0;

	char* arr[2];
	arr[0] = search;
	arr[1] = NULL;
	GStrv s = arr;
	gchar* fts = get_fts_string (s, use_or_operator);

	gchar* query;
	if (fts) {
		query = g_strdup_printf ("SELECT nie:url (?file) ?mime WHERE {"
		                         "  ?file a nfo:FileDataObject ; "
		                         "  nie:mimeType ?mime ; "
		                         "  %s "
		                         //TODO how to get ALL audio types?
		                         "  fts:match \"%s\" . FILTER ( ?mime = \"audio/x-wav\" || ?mime = \"audio/x-flac\" )"
		                         "} "
		                         "ORDER BY ASC(?file) "
		                         "OFFSET %d "
		                         "LIMIT %d",
		                         tag_filter, fts,
		                         search_offset,
		                         MAX_DISPLAY_ROWS);
		//dbg(0, "q=%s", query);
	} else {
		query = g_strdup_printf ("SELECT ?document nie:url(?document) "
		                         "WHERE { "
		                         "  ?document a nfo:Document . "
		                         "} "
		                         "ORDER BY ASC(?document) "
		                         "OFFSET %d "
		                         "LIMIT %d",
		                         search_offset,
		                         MAX_DISPLAY_ROWS);
	}

#if 0
	void
	get_files_foreach (gpointer value, gpointer user_data)
	{
		gchar** data = value;

		if (data[0] && *data[0]) {
			g_print (" %s\n", data[0]);
		}
	}
#endif

	GError* error = NULL;
	iter.results = tracker_resources_sparql_query (tc, query, &error);

	if (error) {
		g_printerr ("%s, %s\n", "Could not get search results", error->message);
		g_error_free (error);
		return FALSE;
	}

	if (!iter.results) {
		dbg(0, "no files found");
	} else {
#if 0
		g_print (g_dngettext (NULL,
		                      "File: %d\n",
		                      "Files: %d\n",
		                      iter.results->len),
		         iter.results->len);

		g_ptr_array_foreach (iter.results, get_files_foreach, NULL);
#endif

		if (iter.results->len >= MAX_DISPLAY_ROWS) {
			dbg(0, "limit reached");
		}
	}
	g_free (query);
	g_free (fts);

	iter.idx = 0;

	return TRUE;
}


Sample*
tracker__search_iter_next()
{
	if(iter.idx >= iter.results->len) return NULL;

	gchar** data = g_ptr_array_index(iter.results, iter.idx);

	if(!data || !data[1]) return NULL;

	if (*data[0]) {
		clear_result();
		//dbg(2, "  %s", file);

		char* _file = g_path_get_basename(data[0]);
		char* file = vfs_unescape_string (_file, NULL);
		g_free(_file);

		char* _dir = g_path_get_dirname(data[0]);
		char* dir = vfs_unescape_string (_dir, NULL);
		dbg(0, "  dir=%s", dir);
		g_free(_dir);

		result.sample_name = file;
		result.dir = dir;
		result.id = iter.idx;           //TODO this idx is pretty meaningless.
		result.online = true;

		if(strstr(result.dir, "file://") == result.dir){
			char* o = result.dir;
			result.dir = g_strdup(o + 7);
			g_free(o);
		}

		if (data[1] && *data[1]) {
			result.mimetype = data[1];
		}else{
			const MIME_type* mime_type = type_from_path(file);
			result.mimetype = g_strdup_printf("%s/%s", mime_type->media_type, mime_type->subtype);
		}

		/*
		if(data[2] && *data[2]){
			g_print("    tag=%s\n", data[2]);
			if(data[3] && *data[3]){
				g_print("      tag=%s\n", data[3]);
			}
		}
		*/

		GPtrArray* tags = get_tags_for_file(file);
		if(tags){
			void append_tag (gpointer value, gpointer _result)
			{
				gchar** data = value;
				Sample* r = (Sample*)_result;

				if (data[1] && *data[1]) {
					if(r->keywords){
						char* old = r->keywords;
						r->keywords = g_strdup_printf("%s %s", old, data[1]);
						g_free(old);
					}
					else r->keywords = g_strdup(data[1]);
				}
			}
			g_ptr_array_foreach (tags, append_tag, &result);

			g_ptr_array_foreach (tags, (GFunc) g_strfreev, NULL);
			g_ptr_array_free (tags, TRUE);
		}
	}

	iter.idx++;

	return &result;
}


void
tracker__search_iter_free()
{
	#warning FIXME tracker__search_iter_free() leak
	static int i = 0; 

			void iter_free_debug (gpointer value, gpointer _i)
			{
				int* i = _i;
				gchar** data = value;
				dbg(0, "%i data=%p", *i, data);
				//g_strfreev(data);
			}

	//g_ptr_array_foreach (iter.results, (GFunc) g_strfreev, NULL); //SEGFAULT!
	g_ptr_array_foreach (iter.results, iter_free_debug, &i);

	g_ptr_array_free (iter.results, TRUE);
	iter.results = NULL;
	i++;
}


//see http://live.gnome.org/Tracker/Documentation/Examples/SPARQL/Tagging
static GPtrArray*
get_tags_for_file(const char* file)
{
	/* result must be freed:
		g_ptr_array_foreach (results, (GFunc) g_strfreev, NULL);
		g_ptr_array_free (results, TRUE);
	*/

	//dbg(0, "file=%s", file);

	gchar* query = g_strdup_printf (
			"SELECT ?tags ?labels "
			"WHERE { "
			"  ?f nie:isStoredAs ?as ; "
			"     nao:hasTag ?tags . "
			"  ?as nie:url '%s' . "
			"  ?tags a nao:Tag ; "
			"     nao:prefLabel ?labels . "
			"} ORDER BY ASC(?labels)", file);

	GError* error = NULL;
	GPtrArray* results = tracker_resources_sparql_query (tc, query, &error);

	if (error) {
		g_printerr ("%s, %s\n", "Could not get search results", error->message);
		g_error_free (error);

		return NULL;
	}

	if (!results) {
		g_print ("%s\n", "No tags were found");
	} else {
#if 0
		g_print (g_dngettext (NULL,
		                      "Tag: %d",
		                      "Tags: %d\n",
		                      results->len),
		                  results->len);
#endif

		void
		tags_foreach (gpointer value, gpointer user_data)
		{
			gchar** data = value;

			//g_print ("  %s", data[0]);

			if (data[1] && *data[1]) {
				g_print ("  tag=%s\n", data[1]);
			}

			//g_print ("\n");
		}

		g_ptr_array_foreach (results, tags_foreach, NULL);
	}
	g_free (query);

	return results;
}


void
tracker__dir_iter_new()
{
	gwarn("FIXME\n");
}


char*
tracker__dir_iter_next()
{
	return NULL;
}


void
tracker__dir_iter_free()
{
}


gboolean
tracker__update_colour(int id, int colour)
{
	pwarn("FIXME\n");
	return true;
}


static gchar*
get_escaped_sparql_string (const gchar* str)
{
	GString* sparql = g_string_new ("");
	g_string_append_c (sparql, '"');

	while (*str != '\0') {
		gsize len = strcspn (str, "\t\n\r\"\\");
		g_string_append_len (sparql, str, len);
		str += len;
		switch (*str) {
		case '\t':
			g_string_append (sparql, "\\t");
			break;
		case '\n':
			g_string_append (sparql, "\\n");
			break;
		case '\r':
			g_string_append (sparql, "\\r");
			break;
		case '"':
			g_string_append (sparql, "\\\"");
			break;
		case '\\':
			g_string_append (sparql, "\\\\");
			break;
		default:
			continue;
		}
		str++;
	}

	g_string_append_c (sparql, '"');

	return g_string_free (sparql, FALSE);
}


static GStrv
get_uris (GStrv files)
{
	GStrv uris;
	gint len, i;

	if (!files) {
		return NULL;
	}

	len = g_strv_length (files);

	if (len < 1) {
		return NULL;
	}

	uris = g_new0 (gchar *, len + 1);

	for (i = 0; files[i]; i++) {
		GFile *file;

		file = g_file_new_for_commandline_arg (files[i]);
		uris[i] = g_file_get_uri (file);
		g_object_unref (file);
	}

	return uris;
}


static gchar*
get_filter_string (GStrv files, gboolean files_are_urns, const gchar* tag)
{
	if (!files) return NULL;

	gint len = g_strv_length (files);

	if (len < 1) {
		return NULL;
	}

	GString* filter = g_string_new ("");

	g_string_append_printf (filter, "FILTER (");

	if (tag) {
		g_string_append (filter, "(");
	}

	gint i;
	for (i = 0; i < len; i++) {
		if (files_are_urns) {
			g_string_append_printf (filter, "?urn = <%s>", files[i]);
		} else {
			g_string_append_printf (filter, "?f = \"%s\"", files[i]);
		}

		if (i < len - 1) {
			g_string_append (filter, " || ");
		}
	}

	if (tag) {
		g_string_append_printf (filter, ") && ?t = <%s>", tag);
	}

	g_string_append (filter, ")");

	dbg(0, "%s", filter->str);
	return g_string_free (filter, FALSE);
}


static GPtrArray*
get_file_urns (TrackerClient* client, GStrv uris, const gchar* tag)
{
	gchar* filter = get_filter_string (uris, FALSE, tag);
	gchar* query = g_strdup_printf ("SELECT ?urn ?f "
	                         "WHERE { "
	                         "  ?urn "
	                         "    %s "
	                         "    nie:url ?f . "
	                         "  %s "
	                         "}",
	                         tag ? "nao:hasTag ?t ; " : "",
	                         filter);

	GError* error = NULL;
	GPtrArray* results = tracker_resources_sparql_query (client, query, &error);

	g_free (query);
	g_free (filter);

	if (error) {
		g_print ("    Could not get file URNs, %s\n", error->message);
		g_error_free (error);
		return NULL;
	}

	return results;
}


static GPtrArray*
get_tag_urns_for_label(const char* label)
{
	GError* error = NULL;

	gchar* query = g_strdup_printf ("SELECT ?tag "
		"WHERE { "
		"  ?tag a nao:Tag . "
		"  ?tag nao:prefLabel '%s' . "
		"}", label
	);
	GPtrArray* results = tracker_resources_sparql_query (tc, query, &error);
	if(results && !results->len){
		g_ptr_array_free (results, TRUE);
		results = NULL;
	}
	g_free(query);
	return results;
}


static gboolean
add_tag_for_label(const char* label)
{
	GError* error = NULL;

	gchar* tag_escaped = get_escaped_sparql_string (label);

	dbg(0, "%s", label);
	gchar* query = g_strdup_printf ("INSERT { "
		"  _:tag a nao:Tag ; "
		"        nao:prefLabel '%s' . "
		"} WHERE { "
		"  OPTIONAL { "
		"     ?tag a nao:Tag ; "
		"          nao:prefLabel '%s' "
		"  } . "
		"  FILTER (!bound(?tag)) "
		"} ",
		label,
		label
	);
	tracker_resources_sparql_update(tc, query, &error);
	//GPtrArray* results = tracker_resources_sparql_query (tc, query, &error);
	//dbg(0, "results=%p", results);
	g_free(query);
	g_free(tag_escaped);
	return true;
}


static GStrv
result_to_strv (GPtrArray* result, gint n_col)
{
	GStrv strv;
	gint i;

	if (!result || result->len == 0) {
		return NULL;
	}

	strv = g_new0 (gchar *, result->len + 1);

	for (i = 0; i < result->len; i++) {
		gchar** row = g_ptr_array_index (result, i);
		strv[i] = g_strdup (row[n_col]);
	}

	return strv;
}


static void
print_file_report (GPtrArray *urns, GStrv uris, const gchar *found_msg, const gchar *not_found_msg)
{
	gint i, j;

	if (!urns || !uris) {
		g_print ("  No files were modified.\n");
		return;
	}

	for (i = 0; uris[i]; i++) {
		gboolean found = FALSE;

		for (j = 0; j < urns->len; j++) {
			gchar **row;

			row = g_ptr_array_index (urns, j);

			if (g_strcmp0 (row[1], uris[i]) == 0) {
				found = TRUE;
				break;
			}
		}

		g_print ("  %s: %s\n",
		         found ? found_msg : not_found_msg,
		         uris[i]);
	}
}


static gboolean
remove_tag_for_file_urns (TrackerClient* client, GStrv files, const gchar* tag)
{
	PF;
	GPtrArray* urns;
	GError* error = NULL;
	gchar* query;

	gchar* tag_escaped = get_escaped_sparql_string (tag);
	GStrv uris = get_uris (files);

	if (uris && *uris) {

		/* Get all tags urns */
		query = g_strdup_printf ("SELECT ?tag "
		                         "WHERE {"
		                         "  ?tag a nao:Tag ."
		                         "  ?tag nao:prefLabel %s "
		                         "}",
		                         tag_escaped);

		GPtrArray* results = tracker_resources_sparql_query (client, query, &error);
		g_free (query);

		if (error) {
			g_printerr ("%s, %s\n", "Could not get tag by label", error->message);
			g_error_free (error);
			g_free (tag_escaped);

			return FALSE;
		}

		if (!results || !results->pdata || !results->pdata[0]) {
			g_print ("%s\n", "No tags were found by that name");

			g_free (tag_escaped);

			return TRUE;
		}

		const gchar* tag_urn = * (GStrv) results->pdata[0];

		GStrv uris = get_uris (files);
		urns = get_file_urns (client, uris, tag_urn);

		if (!urns || urns->len == 0) {
			g_print ("%s\n", "None of the files had this tag set");

			g_strfreev (uris);
			g_free (tag_escaped);

			return TRUE;
		}

		GStrv urns_strv = result_to_strv (urns, 0);
		gchar* filter = get_filter_string (urns_strv, TRUE, tag_urn);

		query = g_strdup_printf ("DELETE { "
		                         "  ?urn nao:hasTag ?t "
		                         "} "
		                         "WHERE { "
		                         "  ?urn nao:hasTag ?t . "
		                         "  %s "
		                         "}",
		                         filter);
		g_free (filter);
		g_strfreev (urns_strv);

		g_ptr_array_foreach (results, (GFunc) g_strfreev, NULL);
		g_ptr_array_free (results, TRUE);
	} else {
		/* Remove tag completely */
		query = g_strdup_printf ("DELETE { "
		                         "  ?tag a nao:Tag "
		                         "} "
		                         "WHERE {"
		                         "  ?tag nao:prefLabel %s "
		                         "}",
		                         tag_escaped);
	}

	g_free (tag_escaped);

	tracker_resources_sparql_update (client, query, &error);
	g_free (query);

	if (error) {
		g_printerr ("%s, %s\n", "Could not remove tag", error->message);
		g_error_free (error);

		return FALSE;
	}

	g_print ("%s\n", "Tag was removed successfully");

	if (urns) {
		print_file_report (urns, uris, "Untagged", "File not indexed or already untagged");

		g_ptr_array_foreach (urns, (GFunc) g_strfreev, NULL);
		g_ptr_array_free (urns, TRUE);
	}

	g_strfreev (uris);

	return TRUE;
}


gboolean
tracker__update_keywords(int id, const char* keywords)
{
	//backend interface is unfortunate here as tracker doesnt use an integer id, it uses filename.

	dbg(0, "tags=%s", keywords);

	gboolean ret = true;

	gchar** tags = g_strsplit(keywords, " ", 100);

	char* filename = listmodel__get_filename_from_id(id);
	if(filename){

		char* s[2] = {filename, NULL};
		GStrv files = s;
		GStrv uris = get_uris (files);

		//any original tags not in the new string need to be deleted.

		GPtrArray* old_tags = get_tags_for_file(uris[0]);
		//dbg(0, "old_tags=%i", old_tags->len);
		if(old_tags){
			int i; for(i=0;i<old_tags->len;i++){
				gchar** data = g_ptr_array_index(old_tags, i);
				gchar* urn; urn = data[0];
				gchar* label = data[1];

				gint y = str_in_array(label, tags);
				if(y < 0){
					dbg(0, "  tag not found - delete... '%s'", label);

					remove_tag_for_file_urns (tc, uris, label);
				}
			}

			g_ptr_array_foreach (old_tags, (GFunc) g_strfreev, NULL);
			g_ptr_array_free (old_tags, TRUE);
		}

		//---------------------------------------------------

		void urn_foreach (gpointer value, gpointer user_data)
		{
			PF;
			gchar** data = value;

			if (data[0] && *data[0]) {
				g_print ("  tag_urn=%s\n", data[0]);
			}
		}
		char* label = tags[0];
		GPtrArray* urn = get_tag_urns_for_label(label);
		if(urn){
			dbg(0, "have tag urn... %p", urn[0]);
			g_ptr_array_foreach (urn, urn_foreach, NULL);

			g_ptr_array_foreach (urn, (GFunc) g_strfreev, NULL);
			g_ptr_array_free (urn, TRUE);
		} else {
			dbg(0, "no tag urns for label: '%s'", label);

			if(add_tag_for_label(label)){

				GPtrArray* urns = get_file_urns (tc, uris, NULL);
				dbg(0, "urns->len=%i", urns ? urns->len : -1);

				/* First we check if the tag is already set and only add if it
				 * is, then we add the urns specified to the new tag.
				 */
				if (urns && urns->len > 0) {
					GStrv urns_strv;

					urns_strv = result_to_strv (urns, 0);
					gchar *filter = get_filter_string (urns_strv, TRUE, NULL);

					gchar* tag_escaped = get_escaped_sparql_string (label);

					/* Add tag to specific urns */
					dbg(0, "adding tag %s to urn...", tag_escaped);
					gchar* query = g_strdup_printf ("INSERT { "
													 "  ?urn nao:hasTag ?id "
													 "} "
													 "WHERE {"
													 "  ?urn nie:url ?f ."
													 "  ?id nao:prefLabel %s "
													 "  %s "
													 "}",
													 tag_escaped,
													 filter);

					GError* error = NULL;
					tracker_resources_sparql_update (tc, query, &error);
					g_strfreev (urns_strv);
					g_free (filter);
					g_free (query);

					if (error) {
						g_printerr ("%s, %s\n", "Could not add tag to files", error->message);
						g_error_free (error);
						g_free (tag_escaped);

						return FALSE;
					}
				}
			}
		}

		/*
		GError* error = NULL;

		tracker_keywords_remove_all(tc, SERVICE_FILES, filename, &error);
		if(error){
			ret = false;
			gwarn("tracker error: %s", error->message);
			g_error_free (error);
		}else{
			tracker_keywords_add(tc, SERVICE_FILES, filename, tags, &error);
			dbg(0, "track call done.");
			if(error){
				ret = false;
				gwarn("tracker error: %s", error->message);
				g_error_free (error);
			}
		}
		*/
		g_free(filename);
	}

	g_strfreev(tags);
	dbg(0, "done.");
	return ret;
}


gboolean
tracker__update_online(int id, gboolean online)
{
	return false;
}


gboolean
tracker__update_peaklevel(int id, float level)
{
	return false;
}


#ifdef TRACKER_0_6
static void
tracker__on_dbus_timeout()
{
	statusbar_print(1, "tracker: dbus timeout");
}
#endif


static void
clear_result()
{
	if(result.keywords) g_free(result.keywords);
	if(result.mimetype) g_free(result.mimetype);
	memset(&result, 0, sizeof(Sample));
}


static gchar*
get_fts_string (GStrv search_words, gboolean use_or_operator)
{
	if (!search_words) return NULL;

	GString* fts = g_string_new ("");
	gint len = g_strv_length (search_words);

	gint i;
	for (i = 0; i < len; i++) {
		g_string_append (fts, search_words[i]);
		g_string_append_c (fts, '*');

		if (i < len - 1) {
			if (use_or_operator) {
				g_string_append (fts, " OR ");
			} else {
				g_string_append (fts, " ");
			}
		}
	}

	return g_string_free (fts, FALSE);
}

#endif //USE_TRACKER

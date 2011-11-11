#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <gtk/gtk.h>
#include <gdk-pixbuf/gdk-pixdata.h>
#include <mysql/errmsg.h>
#include "file_manager.h"
#include "typedefs.h"
#include "support.h"
#include "mimetype.h"
#include "pixmaps.h"
#include "main.h"
#include "listview.h"
#include "sample.h"
#include "db/mysql.h"

//mysql table layout (database column numbers):
enum {
  MYSQL_ID = 0,
  MYSQL_NAME,
  MYSQL_DIR,
  MYSQL_KEYWORDS,
  MYSQL_PIXBUF,
  MYSQL_LENGTH,
  MYSQL_SAMPLERATE,
  MYSQL_CHANNELS,
  MYSQL_ONLINE,
  MYSQL_LAST_CHECKED,
  MYSQL_MIMETYPE,
  MYSQL_PEAKLEVEL,
};
#define MYSQL_NOTES 11
#define MYSQL_COLOUR 12

MYSQL mysql = {{NULL}, NULL, NULL};

extern struct _app app;
extern unsigned debug;

static gboolean is_connected = FALSE;
static MYSQL_RES *dir_iter_result = NULL;
static MYSQL_RES *search_result = NULL;
static Sample result;

static int  mysql__exec_sql (const char* sql);
static void clear_result    ();


/*
	CREATE DATABASE samplelib;

	CREATE TABLE `samples` (
	  `id` int(11) NOT NULL auto_increment,
	  `filename` text NOT NULL,
	  `filedir` text,
	  `keywords` varchar(60) default '',
	  `pixbuf` blob,
	  `length` int(22) default NULL,
	  `sample_rate` int(11) default NULL,
	  `channels` int(4) default NULL,
	  `online` int(1) default NULL,
	  `last_checked` datetime default NULL,
	  `mimetype` tinytext,
	  `notes` mediumtext,
	  `colour` tinyint(4) default NULL,
	  `peaklevel` float(24) default NULL,
	  PRIMARY KEY  (`id`)
	)

	BLOB can handle up to 64k.

*/

gboolean
mysql__connect()
{
	if(!mysql.host){
		if(!mysql_init(&mysql)){
			printf("Failed to initiate MySQL connection.\n");
			exit(1);
		}
	}
	dbg (1, "MySQL Client Version is %s", mysql_get_client_info());
	mysql.reconnect = true;

	if(!mysql_real_connect(&mysql, app.config.database_host, app.config.database_user, app.config.database_pass, app.config.database_name, 0, NULL, 0)){
		if(mysql_errno(&mysql) == CR_CONNECTION_ERROR){
			warnprintf("cannot connect to database: MYSQL server not online.\n");
		}else{
			warnprintf("cannot connect to database: %s\n", mysql_error(&mysql));
			//currently this won't be displayed, as the window is not yet opened.
			statusbar_print(1, "cannot connect to database: %s\n", mysql_error(&mysql));
		}
		return false;
	}
	if(debug) printf("MySQL Server Version is %s\n", mysql_get_server_info(&mysql));

	if(mysql_select_db(&mysql, app.config.database_name)){
		errprintf("Failed to connect to Database: %s\n", mysql_error(&mysql));
		return false;
	}

	is_connected = true;
	return true;
}
 

gboolean
mysql__is_connected()
{
	return is_connected;
}


void
mysql__disconnect()
{
	mysql_close(&mysql);
}


int
mysql__insert(Sample* sample, MIME_type *mime_type)
{
	int id = 0;
	gchar* filedir = g_path_get_dirname(sample->filename);
	gchar* filename = g_path_get_basename(sample->filename);

	gchar* sql = g_strdup_printf("INSERT INTO samples SET filename='%s', filedir='%s', length=%i, sample_rate=%i, channels=%i, mimetype='%s/%s' ", filename, filedir, sample->length, sample->sample_rate, sample->channels, mime_type->media_type, mime_type->subtype);
	dbg(1, "sql=%s", sql);

	if(mysql__exec_sql(sql)==0){
		dbg(1, "ok!");
		id = mysql_insert_id(&mysql);
	}else{
		perr("not ok...\n");
	}
	g_free(sql);
	g_free(filedir);
	g_free(filename);
	return id;
}


gboolean
mysql__delete_row(int id)
{
	gboolean ok = true;
	gchar* sql = g_strdup_printf("DELETE FROM samples WHERE id=%i", id);
	dbg(2, "row: sql=%s", sql);
	if(mysql_query(&mysql, sql)){
		perr("delete failed! sql=%s\n", sql);
		ok = false;
	}
	g_free(sql);
	return ok;
}


static int 
mysql__exec_sql(const char* sql)
{
	return mysql_real_query(&mysql, sql, strlen(sql));
}


gboolean
mysql__update_path(const char* old_path, const char* new_path)
{
	gboolean ok = false;

	char* filename = NULL; //FIXME
	char* old_dir = NULL; //FIXME

	char query[1024];
	snprintf(query, 1023, "UPDATE samples SET filedir='%s' WHERE filename='%s' AND filedir='%s'", new_path, filename, old_dir);
	dbg(0, "%s", query);

	if(!mysql__exec_sql(query)){
		ok = TRUE;
	}
	return ok;
}


gboolean
mysql__update_colour(int id, int colour)
{
	char* sql = g_strdup_printf("UPDATE samples SET colour=%u WHERE id=%i", colour, id);
	dbg(1, "sql=%s", sql);
	gboolean fail;
	if((fail = mysql_query(&mysql, sql))){
		perr("update failed! sql=%s", sql);
	}
	g_free(sql);
	return !fail;
}


gboolean
mysql__update_keywords(int id, const char* keywords)
{
	if(!id) gwarn("id not set.");
	char* sql = g_strdup_printf("UPDATE samples SET keywords='%s' WHERE id=%u", keywords, id);
	dbg(1, "sql=%s", sql);
	gboolean fail;
	if((fail = mysql_query(&mysql, sql))){
		perr("update failed! sql=%s\n", sql);
	}
	g_free(sql);
	return !fail;
}


gboolean
mysql__update_notes(int id, const char* notes)
{
	char sql[1024];
	snprintf(sql, 1024, "UPDATE samples SET notes='%s' WHERE id=%u", notes, id);
	if(mysql_query(&mysql, sql)){
		perr("update failed! sql=%s\n", sql);
		return false;
	}
	else return true;
}



#define SQL_LEN 66000
gboolean
mysql__update_pixbuf(Sample *sample)
{
	GdkPixbuf* pixbuf = sample->pixbuf;
	if(pixbuf){
		//serialise the pixbuf:
		GdkPixdata pixdata;
		gdk_pixdata_from_pixbuf(&pixdata, pixbuf, 0);
		guint length;
		guint8* ser = gdk_pixdata_serialize(&pixdata, &length);

		guint8 blob[SQL_LEN];
		mysql_real_escape_string(&mysql, (char*)blob, (char*)ser, length);

		char sql[SQL_LEN];
		snprintf(sql, SQL_LEN, "UPDATE samples SET pixbuf='%s' WHERE id=%i", blob, sample->id);
		if(mysql_query(&mysql, sql)){
			pwarn("update failed! sql=%s\n", sql);
			return false;
		}

		g_free(ser);

		//at this pt, refcount should be two, we make it 1 so that pixbuf is destroyed with the row:
		//g_object_unref(pixbuf); //FIXME
	}else perr("no pixbuf.\n");

	return true;
}


gboolean
mysql__update_online(int id, gboolean online)
{
	gchar* sql = g_strdup_printf("UPDATE samples SET online=%i, last_checked=NOW() WHERE id=%i", online, id);
	dbg(2, "row: sql=%s", sql);
	if(mysql_query(&mysql, sql)){
		perr("update failed! sql=%s\n", sql);
	}
	g_free(sql);
	return true;
}


gboolean
mysql__update_peaklevel(int id, float level)
{
	gboolean ok = true;
	gchar* sql = g_strdup_printf("UPDATE samples SET peaklevel=%f WHERE id=%i", level, id);
	dbg(2, "row: sql=%s", sql);
	if(mysql_query(&mysql, sql)){
		perr("update failed! sql=%s\n", sql);
		ok = false;
	}
	g_free(sql);
	return ok;
}


//-------------------------------------------------------------


gboolean
mysql__search_iter_new(char* search, char* dir, const char* category, int* n_results)
{
	//return TRUE on success.

	gboolean ok = true;

	if(search_result) gwarn("previous query not free'd?");

	GString* q = g_string_new("SELECT * FROM samples WHERE 1 ");
	if(strlen(search)) g_string_append_printf(q, "AND (filename LIKE '%%%s%%' OR filedir LIKE '%%%s%%' OR keywords LIKE '%%%s%%') ", search, search, search);
	if(dir && strlen(dir))
#ifdef DONT_SHOW_SUBDIRS //TODO
		g_string_append_printf(q, "AND filedir='%s' ", dir);
#else
		g_string_append_printf(q, "AND filedir LIKE '%s%%' ", dir);
#endif
	if(app.search_category) g_string_append_printf(q, "AND keywords LIKE '%%%s%%' ", category);

	dbg(1, "%s", q->str);

	int e; if((e = mysql__exec_sql(q->str)) != 0){
		statusbar_print(1, "Failed to find any records: %s", mysql_error(&mysql));

		if((e == CR_SERVER_GONE_ERROR) || (e == CR_SERVER_LOST)){ //default is to time out after 8 hours
			dbg(0, "mysql connection timed out. Reconnecting... (not tested!)");
			mysql__connect();
		}else{
			dbg(0, "2: Failed to find any records: %i: %s", e, mysql_error(&mysql));
		}
		ok = false;
	} else {
		search_result = mysql_store_result(&mysql);

		if(n_results){
			uint32_t tot_rows = (uint32_t)mysql_affected_rows(&mysql);
			*n_results = tot_rows;
		}
	}

	g_string_free(q, true);
	return ok;
}


MYSQL_ROW
mysql__search_iter_next(unsigned long** lengths)
{
	if(!search_result) return NULL;

	MYSQL_ROW row = mysql_fetch_row(search_result);
	if(!row) return NULL;
	*lengths = mysql_fetch_lengths(search_result); //free? 
	return row;
}


Sample*
mysql__search_iter_next_(unsigned long** lengths)
{
	if(!search_result) return NULL;

	clear_result();

	MYSQL_ROW row = mysql_fetch_row(search_result);
	if(!row) return NULL;

	*lengths = mysql_fetch_lengths(search_result); //free? 

	//deserialise the pixbuf field:
	GdkPixdata pixdata;
	GdkPixbuf* pixbuf = NULL;
	if(row[MYSQL_PIXBUF]){
		dbg(3, "pixbuf_length=%i", (*lengths)[MYSQL_PIXBUF]);
		if(gdk_pixdata_deserialize(&pixdata, (*lengths)[MYSQL_PIXBUF], (guint8*)row[MYSQL_PIXBUF], NULL)){
			pixbuf = gdk_pixbuf_from_pixdata(&pixdata, TRUE, NULL);
		}
	}

	int get_int(MYSQL_ROW row, int i)
	{
		return row[i] ? atoi(row[i]) : 0;
	}

	float get_float(MYSQL_ROW row, int i)
	{
		return row[i] ? atof(row[i]) : 0.0;
	}

	result.idx         = atoi(row[MYSQL_ID]);
	result.sample_name = row[MYSQL_NAME];
	result.dir         = row[MYSQL_DIR];
	result.keywords    = row[MYSQL_KEYWORDS];
	result.length      = get_int(row, MYSQL_LENGTH);
	result.sample_rate = get_int(row, MYSQL_SAMPLERATE);
	result.channels    = get_int(row, MYSQL_CHANNELS);
	result.peak_level  = get_float(row, MYSQL_PEAKLEVEL);
	result.overview    = pixbuf;
	result.notes       = row[MYSQL_NOTES];
	result.misc        = ""; // XXX TODO
	result.colour      = get_int(row, MYSQL_COLOUR);
	result.mimetype    = row[MYSQL_MIMETYPE];
	result.online      = get_int(row, MYSQL_ONLINE);
	result.updated     = row[MYSQL_LAST_CHECKED];
	return &result;
}


void
mysql__search_iter_free()
{
	if(search_result) mysql_free_result(search_result);
	search_result = NULL;
}


//-------------------------------------------------------------


void
mysql__dir_iter_new()
{
	#define DIR_LIST_QRY "SELECT DISTINCT filedir FROM samples ORDER BY filedir"

	if(dir_iter_result) gwarn("previous query not free'd?");

	if(!mysql__exec_sql(DIR_LIST_QRY)){
		dir_iter_result = mysql_store_result(&mysql);
		dbg(2, "num_rows=%i", mysql_num_rows(dir_iter_result));
	}
	else{
		dbg(0, "failed to find any records: %s", mysql_error(&mysql));
	}
}


char*
mysql__dir_iter_next()
{
	if(!dir_iter_result) return NULL;

	MYSQL_ROW row = mysql_fetch_row(dir_iter_result);

	return row ? row[0] : NULL;
}


void
mysql__dir_iter_free()
{
	if(dir_iter_result) mysql_free_result(dir_iter_result);
	dir_iter_result = NULL;
}


void
mysql__iter_to_result(Sample* result)
{
	memset(result, 0, sizeof(Sample));
}


void
mysql__add_row_to_model(MYSQL_ROW row, unsigned long* lengths)
{
	GdkPixbuf* iconbuf = NULL;
	char length[64];
	char keywords[256];
	char sample_name[256];
	float samplerate; char samplerate_s[32];
	unsigned channels, colour;
	gboolean online = FALSE;
	GtkTreeIter iter;

	//db__iter_to_result(result);

	//deserialise the pixbuf field:
	GdkPixdata pixdata;
	GdkPixbuf* pixbuf = NULL;
	if(row[MYSQL_PIXBUF]){
		if(gdk_pixdata_deserialize(&pixdata, lengths[MYSQL_PIXBUF], (guint8*)row[MYSQL_PIXBUF], NULL)){
			pixbuf = gdk_pixbuf_from_pixdata(&pixdata, TRUE, NULL);
		}
	}

	format_time(length, row[MYSQL_LENGTH]);
	if(row[MYSQL_KEYWORDS]) snprintf(keywords, 256, "%s", row[MYSQL_KEYWORDS]); else keywords[0] = 0;
	if(!row[MYSQL_SAMPLERATE]) samplerate = 0; else samplerate = atoi(row[MYSQL_SAMPLERATE]); samplerate_format(samplerate_s, samplerate);
	if(row[7]==NULL) channels   = 0; else channels   = atoi(row[7]);
	if(row[MYSQL_ONLINE]==NULL) online = 0; else online = atoi(row[MYSQL_ONLINE]);
	if(row[MYSQL_COLOUR]==NULL) colour = 0; else colour = atoi(row[MYSQL_COLOUR]);

	strncpy(sample_name, row[MYSQL_NAME], 255);
	//TODO markup should be set in cellrenderer, not model!
#if 0
	if(GTK_WIDGET_REALIZED(app.view)){
		//check colours dont clash:
		long c_num = strtol(app.config.colour[colour], NULL, 16);
		dbg(2, "rowcolour=%s", app.config.colour[colour]);
		GdkColor row_colour; color_rgba_to_gdk(&row_colour, c_num << 8);
		if(is_similar(&row_colour, &app.fg_colour, 0x60)){
			snprintf(sample_name, 255, "%s%s%s", "<span foreground=\"blue\">", row[MYSQL_NAME], "</span>");
		}
	}
#endif

#ifdef USE_AYYI
	//is the file loaded in the current Ayyi song?
	if(ayyi.got_song){
		gchar* fullpath = g_build_filename(row[MYSQL_DIR], sample_name, NULL);
		if(pool__file_exists(fullpath)) dbg(0, "exists"); else dbg(0, "doesnt exist");
		g_free(fullpath);
	}
#endif
	//icon (only shown if the sound file is currently available):
	if(online){
		MIME_type* mime_type = mime_type_lookup(row[MYSQL_MIMETYPE]);
		type_to_icon(mime_type);
		if ( ! mime_type->image ) dbg(0, "no icon.");
		iconbuf = mime_type->image->sm_pixbuf;
	} else iconbuf = NULL;

#if 0
	//strip the homedir from the dir string:
	char* path = strstr(row[MYSQL_DIR], g_get_home_dir());
	path = path ? path + strlen(g_get_home_dir()) + 1 : row[MYSQL_DIR];
#endif

	gtk_list_store_append(app.store, &iter);
	gtk_list_store_set(app.store, &iter, COL_ICON, iconbuf,
#ifdef USE_AYYI
	                   COL_AYYI_ICON, ayyi_icon,
#endif
	                   COL_IDX, atoi(row[MYSQL_ID]), COL_NAME, sample_name, COL_FNAME, row[MYSQL_DIR], COL_KEYWORDS, keywords, 
	                   COL_OVERVIEW, pixbuf, COL_LENGTH, length, COL_SAMPLERATE, samplerate_s, COL_CHANNELS, channels, 
	                   COL_MIMETYPE, row[MYSQL_MIMETYPE], COL_NOTES, row[MYSQL_NOTES], COL_COLOUR, colour, -1);
	if(pixbuf) g_object_unref(pixbuf);

#if 0
	if(app.no_gui){
		printf(" %s\n", sample_name);
	}
#endif
}


static void
clear_result()
{
	if(result.overview) g_object_unref(result.overview);
	memset(&result, 0, sizeof(Sample));
}



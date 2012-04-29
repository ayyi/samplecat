/*
  This file is part of Samplecat. http://samplecat.orford.org
  copyright (C) 2007-2012 Tim Orford and others.

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License version 3
  as published by the Free Software Foundation.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
*/
#define _GNU_SOURCE
#include "config.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <gtk/gtk.h>
#include <gdk-pixbuf/gdk-pixdata.h>
#include <mysql/errmsg.h>
#include "typedefs.h"
#include "support.h"
#include "main.h"
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
  MYSQL_NOTES,
  MYSQL_COLOUR,
  MYSQL_PEAKLEVEL,
  MYSQL_EBUR,
  MYSQL_FULL_PATH,
  MYSQL_MTIME,
  MYSQL_FRAMES,
  MYSQL_BITRATE,
  MYSQL_BITDEPTH,
  MYSQL_METADATA,
};

MYSQL mysql = {{NULL}, NULL, NULL};

extern unsigned debug;

static gboolean is_connected = FALSE;
static SamplecatModel* model = NULL;
static struct _mysql_config* config = NULL;
static MYSQL_RES *dir_iter_result = NULL;
static MYSQL_RES *search_result = NULL;
static Sample result;

static int  mysql__exec_sql (const char* sql);
static void clear_result    ();

#define MYSQL_ESCAPE(VAR,STR) \
	char *VAR; if (!(STR) || strlen(STR)==0) {VAR=calloc(1,sizeof(char));} else { \
		int sl=strlen(STR);\
		VAR = malloc((sl*2+1)*sizeof(char)); \
		mysql_real_escape_string(&mysql, VAR, (STR), sl); \
	}

struct ColumnDefinitions { const char* key; const char* def; };

static const struct ColumnDefinitions sct[] = {
	{"id",           "`id` int(11) NOT NULL auto_increment"},
	{"filename",     "`filename` text NOT NULL"},
	{"filedir",      "`filedir` text"},
	{"keywords",     "`keywords` varchar(60) default ''"},
	{"pixbuf",       "`pixbuf` blob"},
	{"length",       "`length` int(22) default NULL"},
	{"sample_rate",  "`sample_rate` int(11) default NULL"},
	{"channels",     "`channels` int(4) default NULL"},
	{"online",       "`online` int(1) default NULL"},
	{"last_checked", "`last_checked` datetime default NULL"},
	{"mimetype",     "`mimetype` tinytext"},
	{"notes",        "`notes` mediumtext"},
	{"colour",       "`colour` tinyint(4) default NULL"},
	{"peaklevel",    "`peaklevel` float(24) default NULL"},
	{"ebur",         "`ebur` text default NULL"},
	{"full_path",    "`full_path` text NOT NULL"},
	{"mtime",        "`mtime` int(22) default NULL"},
	{"frames",       "`frames` int(22) default NULL"},
	{"bit_rate",     "`bit_rate` int(11) default NULL"},
	{"bit_depth",    "`bit_depth` int(8) default NULL"},
	{"meta_data",    "`meta_data` text default NULL"},
};
#define COLCOUNT (21)

void
mysql__init(SamplecatModel* _model, void* _config)
{
	model = _model;
	config = _config;
}


gboolean
mysql__connect()
{
	g_return_val_if_fail(model, false);

	if(!mysql.host){
		if(!mysql_init(&mysql)){
			printf("Failed to initiate MySQL connection.\n");
			return 0;
		}
	}
	dbg (1, "MySQL Client Version is %s", mysql_get_client_info());
	mysql.reconnect = true;

	if(!mysql_real_connect(&mysql, config->host, config->user, config->pass, config->name, 0, NULL, 0)){
		if(mysql_errno(&mysql) == CR_CONNECTION_ERROR){
			warnprintf("cannot connect to database: MYSQL server not online.\n");
		}else{
			warnprintf("cannot connect to mysql database: %s\n", mysql_error(&mysql));
			//currently this won't be displayed, as the window is not yet opened.
			statusbar_print(1, "cannot connect to mysql database: %s\n", mysql_error(&mysql));
		}
		return false;
	}
	if(debug) printf("MySQL Server Version is %s\n", mysql_get_server_info(&mysql));

	if(mysql_select_db(&mysql, config->name)){
		errprintf("Failed to connect to Database: %s\n", mysql_error(&mysql));
		return false;
	}
	/* check if table exists */
	gboolean dbexists=false;
	int64_t colflag=0;

	MYSQL_ESCAPE(dbname, config->name);
	char* sql = malloc((100/*query string*/+strlen(dbname))*sizeof(char));
	sprintf(sql, "SELECT column_name from INFORMATION_SCHEMA.COLUMNS where table_schema='%s' AND table_name='samples';", config->name);
	if(!mysql_query(&mysql, sql)){
		MYSQL_RES *sr = mysql_store_result(&mysql);
		if (sr) {
			if (mysql_num_rows(sr) == COLCOUNT) {dbexists=true; colflag=(1<<(COLCOUNT))-1;}
			else if (mysql_num_rows(sr) != 0) {
				dbexists=true;
				/* ALTER TABLE */
				MYSQL_ROW row;
				while ((row = mysql_fetch_row(sr))) {
					int i; for (i=0;i<COLCOUNT;i++) {
						if (!strcmp(row[0], sct[i].key)) { colflag|=1<<i; break; }
					}
				}
			}
			mysql_free_result(sr);
		}
	}
	free(dbname); free(sql);

	if (!dbexists) {
		/* CREATE Table */
		int off=0; int i;
		sql = malloc(4096*sizeof(char)); // XXX current length is 643 chars
		off=sprintf(sql, "CREATE TABLE `samples` (");
		for (i=0;i<COLCOUNT;i++)
			off+=sprintf(sql+off, "%s, ",sct[i].def);
		sprintf(sql+off, "PRIMARY KEY  (`id`));");
		dbg(0, "%d: %s", off, sql);

		if (mysql_real_query(&mysql, sql, strlen(sql))) {
			warnprintf("cannot create database-table: %s\n", mysql_error(&mysql));
			mysql_close(&mysql);
			free(sql);
			return false;
		}
		free(sql);
	} else {
		/* Alter Table if neccesary */
		int i;
		sql = malloc(1024*sizeof(char));
		for (i=0;i<COLCOUNT;i++) if ((colflag&(1<<i))==0) {
			sprintf(sql, "ALTER TABLE samples ADD %s;", sct[i].def);
			dbg(0, "%s", sql);
		  if (mysql_real_query(&mysql, sql, strlen(sql))) {
				warnprintf("cannot update database-table to new model: %s\n", mysql_error(&mysql));
				mysql_close(&mysql);
				free(sql);
				return false;
			}
		}
		free(sql);
	}

	memset(&result, 0, sizeof(Sample));

	is_connected = true;
	return true;
}
 
#if 0
gboolean
mysql__is_connected()
{
	return is_connected;
}
#endif


void
mysql__disconnect()
{
	mysql_close(&mysql);
}

int
mysql__insert(Sample* sample)
{
	int id = 0;
	MYSQL_ESCAPE(full_path,   sample->full_path);
	MYSQL_ESCAPE(sample_name, sample->sample_name);
	MYSQL_ESCAPE(sample_dir,  sample->sample_dir);
	MYSQL_ESCAPE(mimetype,    sample->mimetype);
	MYSQL_ESCAPE(ebur,        sample->ebur);
	MYSQL_ESCAPE(meta_data,   sample->meta_data);

	gchar* sql = g_strdup_printf(
		"INSERT INTO samples(full_path,filename,filedir,length,sample_rate,channels,online,mimetype,ebur,peaklevel,colour,mtime,frames,bit_rate,bit_depth,meta_data) "
		"VALUES ('%s','%s','%s',%"PRIi64",'%i','%i','%i','%s','%s','%f','%i','%lu',%"PRIi64",'%i','%i','%s')",
			full_path, sample_name, sample_dir,
			sample->length, sample->sample_rate, sample->channels,
			sample->online, mimetype, ebur,
			sample->peaklevel, sample->colour_index, (unsigned long) sample->mtime,
			sample->frames, sample->bit_rate, sample->bit_depth, meta_data
		);
	dbg(1, "sql=%s", sql);

	if(mysql__exec_sql(sql)==0){
		dbg(1, "ok!");
		id = mysql_insert_id(&mysql);
	}else{
		perr("not ok...\n");
	}
	g_free(sql);
	free(full_path);
	free(sample_name);
	free(sample_dir);
	free(mimetype);
	free(ebur);
	free(meta_data);
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
mysql__update_string(int id, const char* key, const char* value)
{
	return mysql__update_blob(id, key, (const guint8*) (value?value:""), (const guint) value?strlen(value):0);
}


gboolean
mysql__update_int(int id, const char* key, const long int value)
{
	char sql[1024];
	snprintf(sql, 1024, "UPDATE samples SET %s=%li WHERE id=%d", key, value, id);
	if(mysql_query(&mysql, sql)){
		perr("update failed! sql=%s\n", sql);
		return false;
	}
	return true;
}


gboolean
mysql__update_float(int id, const char* key, const float value)
{
	char sql[1024];
	snprintf(sql, 1024, "UPDATE samples SET %s=%f WHERE id=%d", key, value, id);
	if(mysql_query(&mysql, sql)){
		perr("update failed! sql=%s\n", sql);
		return false;
	}
	return true;
}


gboolean
mysql__update_blob(int id, const char* key, const guint8* d, const guint len)
{
	char *blob = malloc((len*2+1)*sizeof(char));
	mysql_real_escape_string(&mysql, blob, (char*)d, len);
	char *sql = malloc((strlen(blob)+33/*query string*/+20 /*int*/+strlen(key))*sizeof(char));
	sprintf(sql, "UPDATE samples SET %s='%s' WHERE id=%i", key, blob, id);
	if(mysql_query(&mysql, sql)){
		free(blob); free(sql);
		pwarn("update failed! sql=%s\n", sql);
		return false;
	}
	free(blob); free(sql);
	return true;
}


gboolean
mysql__file_exists(const char* path, int *id)
{
	int rv=false;
	const int len = strlen(path);
	char *esc = malloc((len*2+1)*sizeof(char));
	mysql_real_escape_string(&mysql, esc, path, len);
	char *sql = malloc((43/*query string*/+strlen(esc))*sizeof(char));
	sprintf(sql, "SELECT id FROM samples WHERE full_path='%s';",esc);
	dbg(2,"%s",sql);
	if (id) *id=0;
	if(!mysql_query(&mysql, sql)){
		MYSQL_RES *sr = mysql_store_result(&mysql);
		if (sr) {
			MYSQL_ROW row = mysql_fetch_row(sr);
			if (row) {
				if (id) *id=atoi(row[0]);
				rv=true;
			}
			mysql_free_result(sr);
		}
	}
	free(sql); free(esc);
	return rv;
	
}

//-------------------------------------------------------------

GList*
mysql__filter_by_audio(Sample *s) 
{
	GList *rv = NULL;
	GString* sql = g_string_new("SELECT full_path FROM samples WHERE 1");
	if (s->channels>0)
		g_string_append_printf(sql, " AND channels=%i", s->channels);
	if (s->sample_rate>0)
		g_string_append_printf(sql, " AND sample_rate=%i", s->sample_rate);
	if (s->frames>0)
		g_string_append_printf(sql, " AND frames=%"PRIi64, s->frames);
	if (s->bit_rate>0)
		g_string_append_printf(sql, " AND bit_rate=%i", s->bit_rate);
	if (s->bit_depth>0)
		g_string_append_printf(sql, " AND bit_depth=%i", s->bit_depth);
	if (s->peaklevel>0)
		g_string_append_printf(sql, " AND peaklevel=%f", s->peaklevel);

	g_string_append_printf(sql, ";");
	dbg(2,"%s",sql->str);

	if(mysql_query(&mysql, sql->str)){
		g_string_free(sql, true);
		return NULL;
	}
	MYSQL_RES *sr = mysql_store_result(&mysql);
	if (sr) {
		MYSQL_ROW row;
		while ((row = mysql_fetch_row(sr))) {
			rv = g_list_prepend(rv, g_strdup(row[0]));
		}
		mysql_free_result(sr);
	}
	g_string_free(sql, true);
	return rv;
}

//-------------------------------------------------------------


gboolean
mysql__search_iter_new(char* X, char* dir, const char* category, int* n_results)
{
	//return TRUE on success.

	g_return_val_if_fail(model, false);

	gboolean ok = true;

	if(search_result) gwarn("previous query not free'd?");

	GString* q = g_string_new("SELECT * FROM samples WHERE 1 ");
	const char* search = model->filters.phrase;
	if(search && strlen(search)) {
#if 0
		g_string_append_printf(q, "AND (filename LIKE '%%%s%%' OR filedir LIKE '%%%s%%' OR keywords LIKE '%%%s%%') ", search, search, search);
#else
		gchar* where = NULL;
		char* sd = strdup(search);
		char* s  = sd;
		char* tok;
		while ((tok = strtok(s, " _")) != 0) {
			MYSQL_ESCAPE(esc, tok);
			gchar* tmp = g_strdup_printf("%s %s (filename LIKE '%%%s%%' OR filedir LIKE '%%%s%%' OR keywords LIKE '%%%s%%') ",
					where?where:"", where?"AND":"",
					esc, esc, esc);
			free(esc);
			if (where) g_free(where);
			where = tmp;
			s = NULL;
		}
		if (where) {
			g_string_append_printf(q, "AND (%s)", where);
			g_free(where);
		}
#endif
	}

	if(dir && strlen(dir)) {
		MYSQL_ESCAPE(esc, dir);
#ifdef DONT_SHOW_SUBDIRS //TODO
		g_string_append_printf(q, "AND filedir='%s' ", esc);
#else
		g_string_append_printf(q, "AND filedir LIKE '%s%%' ", esc);
#endif
		free(esc);
	}
	if(app.model.filters.category) {
		MYSQL_ESCAPE(esc, category);
		g_string_append_printf(q, "AND keywords LIKE '%%%s%%' ", esc);
		free(esc);
	}

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

	GdkPixbuf* pixbuf = NULL;
	if(row[MYSQL_PIXBUF]){
		pixbuf = blob_to_pixbuf((guint8*)row[MYSQL_PIXBUF], (*lengths)[MYSQL_PIXBUF]);
	}

	int get_int(MYSQL_ROW row, int i)
	{
		return row[i] ? atoi(row[i]) : 0;
	}

	float get_float(MYSQL_ROW row, int i)
	{
		return row[i] ? atof(row[i]) : 0.0;
	}

	static char full_path[PATH_MAX];
	if (row[MYSQL_FULL_PATH] && *(row[MYSQL_FULL_PATH])) {
		strcpy(full_path, row[MYSQL_FULL_PATH]);
	} else {
		snprintf(full_path, PATH_MAX, "%s/%s", row[MYSQL_DIR], row[MYSQL_NAME]);
		full_path[PATH_MAX-1]='\0';
	}

	result.id          = atoi(row[MYSQL_ID]);
	result.full_path   = full_path;
	result.sample_name = row[MYSQL_NAME];
	result.sample_dir  = row[MYSQL_DIR];
	result.keywords    = row[MYSQL_KEYWORDS];
	result.length      = get_int(row, MYSQL_LENGTH);
	result.sample_rate = get_int(row, MYSQL_SAMPLERATE);
	result.channels    = get_int(row, MYSQL_CHANNELS);
	result.peaklevel   = get_float(row, MYSQL_PEAKLEVEL);
	result.overview    = pixbuf;
	result.notes       = row[MYSQL_NOTES];
	result.ebur        = row[MYSQL_EBUR];
	result.colour_index= get_int(row, MYSQL_COLOUR);
	result.mimetype    = row[MYSQL_MIMETYPE];
	result.online      = get_int(row, MYSQL_ONLINE);
	result.mtime       = get_int(row, MYSQL_MTIME);
	result.bit_depth   = get_int(row, MYSQL_BITDEPTH);
	result.bit_rate    = get_int(row, MYSQL_BITRATE);
	result.meta_data   = row[MYSQL_METADATA];
	result.frames      = get_int(row, MYSQL_FRAMES); 
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


#if NEVER
void
mysql__iter_to_result(Sample* result)
{
memset(&result, 0, sizeof(Sample));
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
#endif


static void
clear_result()
{
	if(result.overview) g_object_unref(result.overview);
	memset(&result, 0, sizeof(Sample));
}



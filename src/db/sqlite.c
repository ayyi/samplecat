#include "../../config.h"
#include <stdio.h>
#include <string.h>
#include <sqlite3.h>
#include <gtk/gtk.h>
#include <gdk-pixbuf/gdk-pixdata.h>
#include "file_manager.h"
#include "typedefs.h"
#include "mimetype.h"
#include <gimp/gimpaction.h>
#include <gimp/gimpactiongroup.h>

#include "support.h"
#include "types.h"
#include "sample.h"
#include "db/sqlite.h"

static sqlite3* db;
sqlite3_stmt* ppStmt = NULL;
extern int debug;
#define MAX_LEN 256 //temp!

enum {
	COLUMN_ID,
	COLUMN_FILENAME,
	COLUMN_DIR,
	COLUMN_KEYWORDS,
	COLUMN_PIXBUF,
	COLUMN_LENGTH,
	COLUMN_SAMPLERATE,
	COLUMN_CHANNELS,
	COLUMN_ONLINE,
	COLUMN_LASTCHECKED,
	COLUMN_MIMETYPE,
	COLUMN_PEAKLEVEL,
	COLUMN_NOTES,
	COLUMN_COLOUR,
	COLUMN_EBUR,
	COLUMN_ABSPATH,
	COLUMN_MTIME,
};

static gboolean sqlite__update_string(int id, const char*, const char*);
static gboolean sqlite__update_int(int id, int, const char*);
static gboolean sqlite__update_float(int id, float, const char*);


void
sqlite__create_db()
{
}


int
sqlite__connect()
{
	PF;
	int rc;

	if(!ensure_config_dir()) return FALSE;
	char* db_name = g_strdup_printf("%s/.config/" PACKAGE "/" PACKAGE ".sqlite", g_get_home_dir());
	rc = sqlite3_open(db_name, &db); //if the file doesnt exist, it be created.
	if (rc) {
		fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db));
		fprintf(stderr, "file=%s", db_name);
		sqlite3_close(db);
		return FALSE;
	}
	g_free(db_name);

	int on_create_table(void* NotUsed, int argc, char** argv, char** azColName)
	{
		int i;
		for (i=0; i<argc; i++) {
			printf("%s = %s\n", azColName[i], argv[i] ? argv[i] : "NULL");
		}
		printf("\n");
		return 0;
	}

	int table_exists = FALSE;

  int rows,columns;
  char **table= NULL;
	char *errmsg= NULL;
	int n = sqlite3_get_table(db, "SELECT name, sql FROM sqlite_master WHERE type='table' AND name='samples';",
			&table,&rows,&columns,&errmsg);
	if(n==SQLITE_OK && (table != NULL) && (rows==1) && (columns==2)) {
		if (!strcmp(table[2], "samples")) {
			dbg(2, "found table 'samples'");
			table_exists=TRUE;
			if (!strstr(table[3], "ebur TEXT")) { 
				dbg(0, "updating to new model: ebur");
				sqlite3_exec(db, "ALTER TABLE samples add ebur TEXT;", NULL, NULL, &errmsg);
			}
			if (!strstr(table[3], "abspath TEXT")) { 
				dbg(0, "updating to new model: abspath");
				sqlite3_exec(db, "ALTER TABLE samples add abspath TEXT;", NULL, NULL, &errmsg);
			}
			if (!strstr(table[3], "mtime INT")) { 
				dbg(0, "updating to new model: mtime");
				sqlite3_exec(db, "ALTER TABLE samples add mtime INT;", NULL, NULL, &errmsg);
			}
		}
	} else {
		dbg(0, "SQL: %s", errmsg?errmsg:"-");
	}
	if (table) sqlite3_free_table(table);
	if (errmsg) sqlite3_free(errmsg);
	errmsg=NULL;

	if (!table_exists) {
		int n = sqlite3_exec(db, "CREATE TABLE samples ("
			"id INTEGER NOT NULL PRIMARY KEY AUTOINCREMENT, "
			"filename TEXT, filedir TEXT, keywords TEXT, pixbuf BLOB, length INT, sample_rate INT, channels INT, "
			"online INT, last_checked INT, mimetype TEXT, peaklevel REAL, notes TEXT, colour INT, "
			"ebur TEXT, abspath TEXT, mtime INT)",
			on_create_table, 0, &errmsg);
		if (n != SQLITE_OK) {
			if (!strstr(errmsg, "already exists")) {
				perr("Sqlite error: %s\n", errmsg);
				return FALSE;
			}
		}
		if (errmsg) sqlite3_free(errmsg);
		else return TRUE;
	}
	return TRUE;
}


void
sqlite__disconnect()
{
	sqlite3_close(db);
}


int
sqlite__insert(Sample* sample)
{
	char* errMsg = 0;
	sqlite3_int64 idx = -1;

	char* sql = sqlite3_mprintf(
		"INSERT INTO samples(abspath,filename,filedir,length,sample_rate,channels,online,mimetype,ebur,peaklevel,colour,mtime) "
		"VALUES ('%q','%q','%q',%"PRIi64",'%i','%i','%i','%q','%q','%f','%i','%i')",
		sample->full_path, sample->sample_name, sample->dir,
		sample->length, sample->sample_rate, sample->channels,
		sample->online, sample->mimetype, 
		sample->ebur?sample->ebur:"", 
		sample->peak_level, sample->colour_index, sample->mtime
	);

	if(sqlite3_exec(db, sql, NULL, NULL, &errMsg) != SQLITE_OK){
		gwarn("sqlite error: %s", errMsg);
		sqlite3_free(errMsg);
	} else {
		idx = sqlite3_last_insert_rowid(db);
	}

	sqlite3_free(sql);
	return (int)idx;
}

gboolean
sqlite__execwrap(char *sql)
{
	gboolean ok = true;
	char* errMsg = 0;
	if(sqlite3_exec(db, sql, NULL, NULL, &errMsg) != SQLITE_OK){
		ok = false;
		gwarn("sqlite error: %s\n", errMsg);
		gwarn("query: %s\n", sql);
		sqlite3_free(errMsg);
	}
	sqlite3_free(sql);
	return ok;
}

gboolean
sqlite__delete_row(int id)
{
	return sqlite__execwrap(sqlite3_mprintf("DELETE FROM samples WHERE id=%i", id));
}

static gboolean
sqlite__update_string(int id, const char* value, const char* field)
{
	return sqlite__execwrap(sqlite3_mprintf("UPDATE samples SET %s='%q' WHERE id=%u", field, value, id));
}


static gboolean
sqlite__update_int(int id, int value, const char* field)
{
	return sqlite__execwrap(sqlite3_mprintf("UPDATE samples SET %s=%i WHERE id=%i", field, value, id));
}

static gboolean
sqlite__update_float(int id, float value, const char* field)
{
	return sqlite__execwrap(sqlite3_mprintf("UPDATE samples SET %s=%f WHERE id=%i", field, value, id));
}


/* high level API */
gboolean
sqlite__update_colour(int id, int colour)
{
	return sqlite__update_int(id, colour, "colour");
}

gboolean
sqlite__update_keywords(int id, const char* keywords)
{
	return sqlite__update_string(id, keywords, "keywords");
}

gboolean
sqlite__update_ebur(int id, const char* ebur)
{
	return sqlite__update_string(id, ebur, "ebur");
}

gboolean
sqlite__update_notes(int id, const char* notes)
{
	return sqlite__update_string(id, notes, "notes");
}

gboolean
sqlite__update_pixbuf(Sample* sample)
{
	GdkPixbuf* pixbuf = sample->overview;
	g_return_val_if_fail(pixbuf, false);

	gboolean ok = true;
	sqlite3_stmt* ppStmt = NULL;
	char* sql = sqlite3_mprintf("UPDATE samples SET pixbuf=? WHERE id=%u", sample->id);
	int rc = sqlite3_prepare_v2(db, sql, -1, &ppStmt, 0);
	if (rc == SQLITE_OK && ppStmt) {
		GdkPixdata pixdata;
		gdk_pixdata_from_pixbuf(&pixdata, pixbuf, 0);
		guint length;
		guint8* buf = gdk_pixdata_serialize(&pixdata, &length); //this is free'd by the sqlite3_bind_blob callback

		sqlite3_bind_blob(ppStmt, 1, buf, length, g_free);
		while ((rc = sqlite3_step(ppStmt)) == SQLITE_ROW) {
		/*
			int i; for (i = 0; i < sqlite3_column_count(ppStmt); ++i){
				//print_col(ppStmt, i);
				dbg(0, "  column.");
			}
			printf("\n");
		*/
		}
		if(rc != SQLITE_DONE) pwarn("step: code=%i error=%s\n", rc, sqlite3_errmsg(db));
		if((rc = sqlite3_finalize(ppStmt)) != SQLITE_OK){
			pwarn("finalize error: %s", sqlite3_errmsg(db));
		}
		dbg(1, "update done");
	}
	else pwarn("prepare not ok! code=%i %s\n", rc, sqlite3_errmsg(db));
	if(sqlite3_changes(db) != 1){ pwarn("n_changes=%i\n", sqlite3_changes(db)); ok = false; }
	sqlite3_free(sql);

	return ok;
}


gboolean
sqlite__update_online(int id, gboolean online, time_t mtime)
{
	g_return_val_if_fail(id, false);

	int on_update(void* NotUsed, int argc, char** argv, char** azColName)
	{
		PF;
		return 0;
	}

	gboolean ok = true;
	gchar* sql = sqlite3_mprintf("UPDATE samples SET online=%i, mtime=%i, last_checked=datetime('now') WHERE id=%i", online, mtime, id);
	dbg(1, "sql=%s", sql);
	char* errMsg = 0;
	int n = sqlite3_exec(db, sql, on_update, 0, &errMsg);
	if (n != SQLITE_OK) {
		gwarn("sqlite error: %s", errMsg);
		ok = false;
	}
	sqlite3_free(sql);
	return ok;
}


gboolean
sqlite__update_peaklevel(int id, float level)
{
	return sqlite__update_float(id, level, "peaklevel");
}


gboolean
sqlite__file_exists(const char* path)
{
	PF;
	gboolean ok =false;
  int rows,columns;
  char **table = NULL;
	char *errmsg= NULL;
	char* sql = sqlite3_mprintf("SELECT abspath FROM samples WHERE abspath='%q'", path);
	int rc = sqlite3_get_table(db, sql, &table,&rows,&columns,&errmsg);
	if(rc==SQLITE_OK && (table != NULL) && (rows>=1)) {
		ok=true;
	}
	if (table) sqlite3_free_table(table);

	sqlite3_free(sql);
	return ok;
}

gboolean
sqlite__search_iter_new(char* search, char* dir, const char* category, int* n_results)
{
	PF;
	static int count = 0;

	void select_count(const char* where)
	{
		int select_callback(void* NotUsed, int argc, char** argv, char** azColName){ count = atoi(argv[0]); return 0; }

		char* sql = sqlite3_mprintf("SELECT COUNT(*) FROM samples WHERE 1 %s", where);
		char* errMsg = NULL;
		sqlite3_exec(db, sql, select_callback, 0, &errMsg);
		sqlite3_free(sql);
	}

	gboolean ok = true;

	char* where = sqlite3_mprintf("%s", "");
	if(search && strlen(search)){
		char* where2 = sqlite3_mprintf("%s AND (filename LIKE '%%%q%%' OR filedir LIKE '%%%q%%' OR keywords LIKE '%%%q%%') ", where, search, search, search);
		sqlite3_free(where);
		where = where2;
	}
	if(category){
		gchar* where2 = sqlite3_mprintf("%s AND keywords LIKE '%%%q%%' ", where, category);
		sqlite3_free(where);
		where = where2;
	}
	if(dir && strlen(dir)){
#ifdef DONT_SHOW_SUBDIRS //TODO
		gchar* where2 = sqlite3_mprintf("%s AND filedir='%q' ", where, dir);
#else
		//match the beginning of the dir path
		gchar* where2 = sqlite3_mprintf("%s AND filedir LIKE '%q%%' ", where, dir);
#endif
		sqlite3_free(where);
		where = where2;
	}

	char* sql = sqlite3_mprintf("SELECT * FROM samples WHERE 1 %s", where);
	dbg(2, "sql=%s", sql);

	if(ppStmt) gwarn("ppStmt not reset from previous query?");
	int n = sqlite3_prepare_v2(db, sql, -1, &ppStmt, NULL);
	if(n == SQLITE_OK && ppStmt){
		count = 0;
		if(n_results){
			select_count(where);
			*n_results = count;
		}
	}
	else{
		gwarn("failed to create prepared statement. sql=%s", sql);
		ok = false;
	}
	sqlite3_free(sql);
	sqlite3_free(where);
	return ok;
}


Sample*
sqlite__search_iter_next(unsigned long** lengths)
{

	int n = sqlite3_step(ppStmt);
	if(n != SQLITE_ROW) return NULL;

	//deserialise the pixbuf field:
	GdkPixdata pixdata;
	GdkPixbuf* pixbuf = NULL;
	const char* blob = sqlite3_column_blob(ppStmt, COLUMN_PIXBUF);
	if(blob){
		int length = sqlite3_column_bytes(ppStmt, COLUMN_PIXBUF);
		dbg(2, "pixbuf_length=%i", length);
		if(gdk_pixdata_deserialize(&pixdata, length, (guint8*)blob, NULL)){
			pixbuf = gdk_pixbuf_from_pixdata(&pixdata, TRUE, NULL);
		}
	}

	static Sample result;

	memset(&result, 0, sizeof(Sample));
	result.id          = sqlite3_column_int(ppStmt, COLUMN_ID);
	result.full_path   = (char*)(char*)sqlite3_column_text(ppStmt, COLUMN_ABSPATH);
	result.sample_name = (char*)sqlite3_column_text(ppStmt, COLUMN_FILENAME);
	result.dir         = (char*)sqlite3_column_text(ppStmt, COLUMN_DIR);
	result.keywords    = (char*)sqlite3_column_text(ppStmt, COLUMN_KEYWORDS);
	result.length      = sqlite3_column_int(ppStmt, COLUMN_LENGTH);
	result.sample_rate = sqlite3_column_int(ppStmt, COLUMN_SAMPLERATE);
	result.channels    = sqlite3_column_int(ppStmt, COLUMN_CHANNELS);
	result.overview    = (GdkPixbuf*)sqlite3_column_blob(ppStmt, COLUMN_PIXBUF);
	result.notes       = (char*)sqlite3_column_text(ppStmt, COLUMN_NOTES);
	result.ebur        = (char*)sqlite3_column_text(ppStmt, COLUMN_EBUR);
	result.colour_index= sqlite3_column_int(ppStmt, COLUMN_COLOUR);
	result.mimetype    = (char*)sqlite3_column_text(ppStmt, COLUMN_MIMETYPE);
	result.peak_level  = sqlite3_column_double(ppStmt, COLUMN_PEAKLEVEL);
	result.online      = sqlite3_column_int(ppStmt, COLUMN_ONLINE);
	result.mtime       = sqlite3_column_int(ppStmt, COLUMN_MTIME);
	result.overview    = pixbuf;

	/* backwards compat. */
	if (!result.full_path) {
		static char full_path[PATH_MAX];
		snprintf(full_path, PATH_MAX, "%s/%s", result.dir, result.sample_name); full_path[PATH_MAX-1]='\0';
		result.full_path  = full_path;
	}

	//sqlite3_result_blob(sqlite3_context*, const void*, int, void(*)(void*));

	return &result;
}


void
sqlite__search_iter_free()
{
	sqlite3_finalize(ppStmt);
	ppStmt = NULL;
}


void
sqlite__dir_iter_new()
{
	#define SQLITE_DIR_LIST_QRY "SELECT DISTINCT filedir FROM samples ORDER BY filedir"

	if(ppStmt) gwarn("ppStmt not reset from previous query?");
	int n = sqlite3_prepare_v2(db, SQLITE_DIR_LIST_QRY, -1, &ppStmt, NULL);
	if(ppStmt && n == SQLITE_OK){
		/*
		count = 0;
		if(n_results){
			select_count(where);
			*n_results = count;
		}
		*/
	}
	else{
		gwarn("failed to create prepared statement. sql=%s", SQLITE_DIR_LIST_QRY);
	}
}


char*
sqlite__dir_iter_next()
{
	int n = sqlite3_step(ppStmt);
	if(n == SQLITE_ROW){
		return (char*)sqlite3_column_text(ppStmt, 0);
	}
	else return NULL;
}


void
sqlite__dir_iter_free()
{
	sqlite3_finalize(ppStmt);
	ppStmt = NULL;
}

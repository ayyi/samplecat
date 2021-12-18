/*
 +----------------------------------------------------------------------+
 | This file is part of Samplecat. http://ayyi.github.io/samplecat/     |
 | copyright (C) 2007-2022 Tim Orford <tim@orford.org> and others       |
 +----------------------------------------------------------------------+
 | This program is free software; you can redistribute it and/or modify |
 | it under the terms of the GNU General Public License version 3       |
 | as published by the Free Software Foundation.                        |
 +----------------------------------------------------------------------+
 |
 */

#include "config.h"
#include <sqlite3.h>
#include <gdk-pixbuf/gdk-pixdata.h>
#include "debug/debug.h"
#include "samplecat/model.h"
#include "samplecat/support.h"
#include "db/db.h"
#include "db/sqlite.h"

#define backend() (samplecat.model->backend)

static void     sqlite__disconnect       ();

static bool     sqlite__search_iter_new  (int* n_results);
static Sample*  sqlite__search_iter_next (unsigned long** lengths);
static void     sqlite__search_iter_free ();

static void     sqlite__dir_iter_new     ();
static char*    sqlite__dir_iter_next    ();
static void     sqlite__dir_iter_free    ();

static bool     sqlite__update_string    (int id, const char*, const char*);
static bool     sqlite__update_int       (int id, const char*, const long int);
static bool     sqlite__update_float     (int id, const char*, const float);
static bool     sqlite__update_blob      (int id, const char*, const guint8*, const guint);

static int      sqlite__insert           (Sample*);
static bool     sqlite__delete_row       (int id);
static bool     sqlite__file_exists      (const char*, int *id);

static GList*   sqlite__filter_by_audio  (Sample *s);

static sqlite3* db;
sqlite3_stmt* ppStmt = NULL;
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
	COLUMN_FRAMES,
	COLUMN_BITRATE,
	COLUMN_BITDEPTH,
	COLUMN_METADATA,
};


void
sqlite__init (void* _config)
{
	//config = _config;
}


void
sqlite__set_as_backend (SamplecatBackend* backend)
{
	backend->init             = sqlite__init;

	backend->search_iter_new  = sqlite__search_iter_new;
	backend->search_iter_next = sqlite__search_iter_next;
	backend->search_iter_free = sqlite__search_iter_free;

	backend->dir_iter_new     = sqlite__dir_iter_new;
	backend->dir_iter_next    = sqlite__dir_iter_next;
	backend->dir_iter_free    = sqlite__dir_iter_free;

	backend->update_string    = sqlite__update_string;
	backend->update_float     = sqlite__update_float;
	backend->update_int       = sqlite__update_int;
	backend->update_blob      = sqlite__update_blob;

	backend->insert           = sqlite__insert;
	backend->remove           = sqlite__delete_row;
	backend->file_exists      = sqlite__file_exists;
	backend->filter_by_audio  = sqlite__filter_by_audio;

	backend->disconnect       = sqlite__disconnect;
}


void
sqlite__create_db ()
{
}


int
sqlite__connect ()
{
	PF;
	int rc;

	if (!ensure_config_dir()) {
		return false;
	}

	char* db_name = g_strdup_printf("%s/.config/" PACKAGE "/" PACKAGE ".sqlite", g_get_home_dir());
	rc = sqlite3_open(db_name, &db); //if the file doesnt exist, it will be created.
	if (rc) {
		fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db));
		fprintf(stderr, "file=%s", db_name);
		sqlite3_close(db);
		return FALSE;
	}
	g_free(db_name);

	int on_create_table (void* NotUsed, int argc, char** argv, char** azColName)
	{
		int i;
		for (i=0; i<argc; i++) {
			printf("%s = %s\n", azColName[i], argv[i] ? argv[i] : "NULL");
		}
		printf("\n");
		return 0;
	}

	int table_exists = FALSE;

	int rows, columns;
	char** table = NULL;
	char* errmsg = NULL;
	int n = sqlite3_get_table(
		db,
		"SELECT name, sql FROM sqlite_master WHERE type='table' AND name='samples';",
		&table,
		&rows,
		&columns,
		&errmsg
	);
	if (n == SQLITE_OK && table && (rows == 1) && (columns == 2)) {
		if (!strcmp(table[2], "samples")) {
			dbg(2, "found table 'samples'");
			table_exists = TRUE;
			if (!strstr(table[3], "ebur TEXT")) { 
				dbg(0, "updating to new model: ebur");
				int n = sqlite3_exec(db, "ALTER TABLE samples add ebur TEXT;", NULL, NULL, &errmsg);
				if (n != SQLITE_OK) perr("Sqlite error: %s\n", errmsg); if (errmsg) sqlite3_free(errmsg);
			}
			if (!strstr(table[3], "full_path TEXT")) { 
				dbg(0, "updating to new model: full_path");
				int n = sqlite3_exec(db, "ALTER TABLE samples add full_path TEXT;", NULL, NULL, &errmsg);
				if (n != SQLITE_OK) perr("Sqlite error: %s\n", errmsg); if (errmsg) sqlite3_free(errmsg);
			}
			if (!strstr(table[3], "mtime INT")) { 
				dbg(0, "updating to new model: mtime");
				int n = sqlite3_exec(db, "ALTER TABLE samples add mtime INT;", NULL, NULL, &errmsg);
				if (n != SQLITE_OK) perr("Sqlite error: %s\n", errmsg); if (errmsg) sqlite3_free(errmsg);
			}
			if (!strstr(table[3], "frames INT")) { 
				dbg(0, "updating to new model: frames");
				int n = sqlite3_exec(db, "ALTER TABLE samples add frames INT;", NULL, NULL, &errmsg);
				if (n != SQLITE_OK) perr("Sqlite error: %s\n", errmsg); if (errmsg) sqlite3_free(errmsg);
			}
			if (!strstr(table[3], "bit_rate INT")) { 
				dbg(0, "updating to new model: bit_rate");
				int n = sqlite3_exec(db, "ALTER TABLE samples add bit_rate INT;", NULL, NULL, &errmsg);
				if (n != SQLITE_OK) perr("Sqlite error: %s\n", errmsg); if (errmsg) sqlite3_free(errmsg);
			}
			if (!strstr(table[3], "bit_depth INT")) { 
				dbg(0, "updating to new model: bit_depth");
				int n = sqlite3_exec(db, "ALTER TABLE samples add bit_depth INT;", NULL, NULL, &errmsg);
				if (n != SQLITE_OK) perr("Sqlite error: %s\n", errmsg); if (errmsg) sqlite3_free(errmsg);
			}
			if (!strstr(table[3], "meta_data TEXT")) { 
				dbg(0, "updating to new model: meta_data");
				int n = sqlite3_exec(db, "ALTER TABLE samples add meta_data TEXT;", NULL, NULL, &errmsg);
				if (n != SQLITE_OK) perr("Sqlite error: %s\n", errmsg); if (errmsg) sqlite3_free(errmsg);
			}
		}
	} else {
		if (errmsg) dbg(0, "SQL: %s", errmsg);
	}
	if (table) sqlite3_free_table(table);
	if (errmsg) sqlite3_free(errmsg);
	errmsg = NULL;

	if (!table_exists) {
		int n = sqlite3_exec(db, "CREATE TABLE samples ("
			"id INTEGER NOT NULL PRIMARY KEY AUTOINCREMENT, "
			"filename TEXT,"
			"filedir TEXT,"
			"keywords TEXT,"
			"pixbuf BLOB,"
			"length INT,"
			"sample_rate INT,"
			"channels INT,"
			"online INT,"
			"last_checked INT,"
			"mimetype TEXT,"
			"peaklevel REAL,"
			"notes TEXT,"
			"colour INT, "
			"ebur TEXT,"
			"full_path TEXT,"
			"mtime INT,"
			"frames INT,"
			"bit_rate INT,"
			"bit_depth INT,"
			"meta_data TEXT)",
			on_create_table, 0,
			&errmsg
		);
		if (n != SQLITE_OK) perr("Sqlite error: %s\n", errmsg);
		if (errmsg) { sqlite3_free(errmsg); return FALSE;}

		n = sqlite3_exec(db, "CREATE UNIQUE INDEX idx1 ON samples (full_path);", NULL, NULL, &errmsg);
		if (n != SQLITE_OK) perr("Sqlite error: %s\n", errmsg); if (errmsg) sqlite3_free(errmsg);

		n = sqlite3_exec(db, "CREATE INDEX idx2 ON samples (keywords);", NULL, NULL, &errmsg);
		if (n != SQLITE_OK) perr("Sqlite error: %s\n", errmsg); if (errmsg) sqlite3_free(errmsg);

		n = sqlite3_exec(db, "CREATE INDEX idx3 ON samples (filename);", NULL, NULL, &errmsg);
		if (n != SQLITE_OK) perr("Sqlite error: %s\n", errmsg); if (errmsg) sqlite3_free(errmsg);

		n = sqlite3_exec(db, "CREATE INDEX idx4 ON samples (filedir);", NULL, NULL, &errmsg);
		if (n != SQLITE_OK) perr("Sqlite error: %s\n", errmsg); if (errmsg) sqlite3_free(errmsg);

	}
	return TRUE;
}


static void
sqlite__disconnect ()
{
	sqlite3_close(db);
}


static int
sqlite__insert (Sample* sample)
{
	char* errMsg = 0;
	sqlite3_int64 idx = -1;

	char* metadata = sample_get_metadata_str(sample);

	char* sql = sqlite3_mprintf(
		"INSERT INTO samples(full_path,filename,filedir,length,sample_rate,channels,online,mimetype,ebur,peaklevel,colour,mtime,frames,bit_rate,bit_depth,meta_data) "
		"VALUES ('%q','%q','%q',%"PRIi64",'%i','%i','%i','%q','%q','%f','%i','%lu',%"PRIi64",'%i','%i','%q')",
		sample->full_path, sample->name, sample->sample_dir,
		sample->length, sample->sample_rate, sample->channels,
		sample->online, sample->mimetype, 
		sample->ebur?sample->ebur:"", 
		sample->peaklevel, sample->colour_index, (unsigned long) sample->mtime,
		sample->frames, sample->bit_rate, sample->bit_depth,
		metadata ? metadata : ""
	);

	if (sqlite3_exec(db, sql, NULL, NULL, &errMsg) != SQLITE_OK) {
		pwarn("sqlite error: %s", errMsg);
		sqlite3_free(errMsg);
	} else {
		idx = sqlite3_last_insert_rowid(db);
	}

	if (metadata) g_free(metadata);
	sqlite3_free(sql);
	return (int)idx;
}


static bool
sqlite__execwrap (char* sql)
{
	bool ok = true;
	char* errMsg = 0;
	if (sqlite3_exec(db, sql, NULL, NULL, &errMsg) != SQLITE_OK) {
		ok = false;
		gwarn("sqlite error: %s\n", errMsg);
		gwarn("query: %s\n", sql);
		sqlite3_free(errMsg);
	}
	sqlite3_free(sql);
	return ok;
}


static bool
sqlite__delete_row (int id)
{
	return sqlite__execwrap(sqlite3_mprintf("DELETE FROM samples WHERE id=%i", id));
}


static bool
sqlite__update_string (int id, const char* key, const char* value)
{
	return sqlite__execwrap(sqlite3_mprintf("UPDATE samples SET %s='%q' WHERE id=%u", key, value?value:"", id));
}


static bool
sqlite__update_int (int id, const char* key, const long int value)
{
	return sqlite__execwrap(sqlite3_mprintf("UPDATE samples SET %s=%li WHERE id=%i", key, value, id));
}


static bool
sqlite__update_float (int id, const char* key, const float value)
{
	return sqlite__execwrap(sqlite3_mprintf("UPDATE samples SET %s=%f WHERE id=%i", key, value, id));
}


static bool
sqlite__update_blob (int id, const char* key, const guint8* d, const guint len)
{
	gboolean ok = true;
	sqlite3_stmt* ppStmt = NULL;

	char* sql = sqlite3_mprintf("UPDATE samples SET %s=? WHERE id=%u", key, id);
	int rc = sqlite3_prepare_v2(db, sql, -1, &ppStmt, 0);
	if (rc != SQLITE_OK || !ppStmt) {
		pwarn("prepare not ok! code=%i %s\n", rc, sqlite3_errmsg(db));
		g_free((gpointer)d);
		return false;
	}

	sqlite3_bind_blob(ppStmt, 1, d, len, g_free);
	while ((rc = sqlite3_step(ppStmt)) == SQLITE_ROW) { ; }
	if (rc != SQLITE_DONE) {
		pwarn("step: code=%i error=%s\n", rc, sqlite3_errmsg(db));
		ok = true;
	}
	if ((rc = sqlite3_finalize(ppStmt)) != SQLITE_OK) {
		pwarn("finalize error: %s", sqlite3_errmsg(db));
		ok = false;
	}
	sqlite3_free(sql);
	return ok;
}


static bool
sqlite__file_exists (const char* path, int *id)
{
	PF2;
	bool ok = false;
	int rows, columns;
	char** table = NULL;
	char* errmsg = NULL;

	char* sql = sqlite3_mprintf("SELECT id FROM samples WHERE full_path='%q'", path);
	int rc = sqlite3_get_table(db, sql, &table, &rows, &columns, &errmsg);
	if (rc == SQLITE_OK && (table != NULL) && (rows >= 1) && (columns == 1)) {
		if (id) *id = atoi(table[1]);
		ok = true;
	}
	if (table) sqlite3_free_table(table);

	sqlite3_free(sql);
	return ok;
}


static GList*
sqlite__filter_by_audio (Sample* s)
{
	GList* rv = NULL;
	GString* sql = g_string_new("SELECT full_path FROM samples WHERE 1");
	if (s->channels > 0)
		g_string_append_printf(sql, " AND channels=%i", s->channels);
	if (s->sample_rate > 0)
		g_string_append_printf(sql, " AND sample_rate=%i", s->sample_rate);
	if (s->frames > 0)
		g_string_append_printf(sql, " AND frames=%"PRIi64, s->frames);
	if (s->bit_rate > 0)
		g_string_append_printf(sql, " AND bit_rate=%i", s->bit_rate);
	if (s->bit_depth > 0)
		g_string_append_printf(sql, " AND bit_depth=%i", s->bit_depth);
	if (s->peaklevel > 0)
		g_string_append_printf(sql, " AND peaklevel=%f", s->peaklevel);

	g_string_append_printf(sql, ";");
	dbg(2, "%s", sql->str);

	int rows,columns;
	char **table= NULL;
	char *errmsg= NULL;
	int n = sqlite3_get_table(db, sql->str, &table, &rows, &columns, &errmsg);
	if (n != SQLITE_OK || (table == NULL) || (rows < 1) || (columns != 1)) {
		goto out;
	}
	for(int i=0;i<rows;i++) {
		rv = g_list_prepend(rv, g_strdup(table[i*columns+1]));
	}

  out:
	if (table) sqlite3_free_table(table);
	if (errmsg) sqlite3_free(errmsg);
	g_string_free(sql, true);

	return rv;
}


static bool
sqlite__search_iter_new (int* n_results)
{
	PF;
	static int count = 0;

	void select_count (const char* where)
	{
		int select_callback(void* NotUsed, int argc, char** argv, char** azColName){ count = atoi(argv[0]); return 0; }

		char* sql = sqlite3_mprintf("SELECT COUNT(*) FROM samples WHERE 1 %s", where);
		char* errMsg = NULL;
		sqlite3_exec(db, sql, select_callback, 0, &errMsg);
		sqlite3_free(sql);
	}

	gboolean ok = true;

	char* where = sqlite3_mprintf("%s", "");
	const char* search = samplecat.model->filters2.search->value.c;
	if (search && strlen(search)) {
#if 0
		char* where2 = sqlite3_mprintf("%s AND (filename LIKE '%%%q%%' OR filedir LIKE '%%%q%%' OR keywords LIKE '%%%q%%') ", where, search, search, search);
#else
		char* where2a = NULL;
		char* sd = strdup(search);
		char* s  = sd;
		char* tok;
		while ((tok = strtok(s, " _")) != 0) {
			char* tmp = sqlite3_mprintf("%s %s (filename LIKE '%%%q%%' OR filedir LIKE '%%%q%%' OR keywords LIKE '%%%q%%') ",
				where2a ? where2a : "",
				where2a ? "AND" : "",
				tok, tok, tok
			);
			if (where2a) sqlite3_free(where2a);
			where2a = tmp;
			s = NULL;
		}
		char* where2;
		if (where2a) {
			where2 = sqlite3_mprintf("%s AND (%s)", where, where2a);
			dbg(2, "%s", where2);
			sqlite3_free(where2a);
		} else {
			where2 = sqlite3_mprintf("%s", where);
		}
		free(sd);
#endif
		sqlite3_free(where);
		where = where2;
	}
	const char* category = samplecat.model->filters2.category->value.c;
	if (category) {
		gchar* where2 = sqlite3_mprintf("%s AND keywords LIKE '%%%q%%' ", where, category);
		sqlite3_free(where);
		where = where2;
	}
	const char* dir = samplecat.model->filters2.dir->value.c;
	if (dir && strlen(dir)) {
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

	if (ppStmt) gwarn("ppStmt not reset from previous query?");
	int n = sqlite3_prepare_v2(db, sql, -1, &ppStmt, NULL);
	if (n == SQLITE_OK && ppStmt) {
		count = 0;
		select_count(where);
		if (n_results) *n_results = count;
		backend().n_results = count;
	} else {
		gwarn("failed to create prepared statement. sql=%s", sql);
		ok = false;
	}
	sqlite3_free(sql);
	sqlite3_free(where);

	return ok;
}


static Sample result;

static Sample*
sqlite__search_iter_next (unsigned long** lengths)
{

	int n = sqlite3_step(ppStmt);
	if (n != SQLITE_ROW) return NULL;

	if (result.overview) g_object_unref(result.overview);

	//deserialise the pixbuf field:
	GdkPixbuf* pixbuf = NULL;
	const unsigned char* blob = sqlite3_column_blob(ppStmt, COLUMN_PIXBUF);
	if (blob) {
		int length = sqlite3_column_bytes(ppStmt, COLUMN_PIXBUF);
		pixbuf = blob_to_pixbuf(blob, length);
	}

	result = (Sample){
		.id          = sqlite3_column_int(ppStmt, COLUMN_ID),
		.full_path   = (char*)(char*)sqlite3_column_text(ppStmt, COLUMN_ABSPATH),
		.name        = (char*)sqlite3_column_text(ppStmt, COLUMN_FILENAME),
		.sample_dir  = (char*)sqlite3_column_text(ppStmt, COLUMN_DIR),
		.keywords    = (char*)sqlite3_column_text(ppStmt, COLUMN_KEYWORDS),
		.length      = sqlite3_column_int(ppStmt, COLUMN_LENGTH),
		.sample_rate = sqlite3_column_int(ppStmt, COLUMN_SAMPLERATE),
		.frames      = sqlite3_column_int(ppStmt, COLUMN_FRAMES),
		.channels    = sqlite3_column_int(ppStmt, COLUMN_CHANNELS),
		.notes       = (char*)sqlite3_column_text(ppStmt, COLUMN_NOTES),
		.ebur        = (char*)sqlite3_column_text(ppStmt, COLUMN_EBUR),
		.colour_index= sqlite3_column_int(ppStmt, COLUMN_COLOUR),
		.mimetype    = (char*)sqlite3_column_text(ppStmt, COLUMN_MIMETYPE),
		.peaklevel   = sqlite3_column_double(ppStmt, COLUMN_PEAKLEVEL),
		.online      = sqlite3_column_int(ppStmt, COLUMN_ONLINE),
		.mtime       = (unsigned long) sqlite3_column_int(ppStmt, COLUMN_MTIME),
		.bit_depth   = sqlite3_column_int(ppStmt, COLUMN_BITDEPTH),
		.bit_rate    = sqlite3_column_int(ppStmt, COLUMN_BITRATE),
		.overview    = pixbuf,
	};

	sample_set_metadata(&result, (char*)sqlite3_column_text(ppStmt, COLUMN_METADATA));

	/* backwards compat. */
	if (!result.full_path) {
		static char full_path[PATH_MAX];
		snprintf(full_path, PATH_MAX, "%s/%s", result.sample_dir, result.name); full_path[PATH_MAX-1]='\0';
		result.full_path  = full_path;
		dbg(1, "compat: filling in path by dir & filename");
	}
	if (!result.frames) {
		dbg(1, "compat: filling in frame-count by sample-rate & length");
		result.frames = result.length * result.sample_rate / 1000;
	}

	//sqlite3_result_blob(sqlite3_context*, const void*, int, void(*)(void*));

	return &result;
}


static void
sqlite__search_iter_free ()
{
	if (result.overview) g_clear_pointer(&result.overview, g_object_unref);

	g_clear_pointer(&ppStmt, sqlite3_finalize);
}


static void
sqlite__dir_iter_new ()
{
	#define SQLITE_DIR_LIST_QRY "SELECT DISTINCT filedir FROM samples ORDER BY filedir"

	if (ppStmt) gwarn("ppStmt not reset from previous query?");
	int n = sqlite3_prepare_v2(db, SQLITE_DIR_LIST_QRY, -1, &ppStmt, NULL);
	if (ppStmt && n == SQLITE_OK) {
		/*
		count = 0;
		if(n_results){
			select_count(where);
			*n_results = count;
		}
		*/
	} else {
		pwarn("failed to create prepared statement. sql=%s", SQLITE_DIR_LIST_QRY);
	}
}


static char*
sqlite__dir_iter_next ()
{
	int n = sqlite3_step(ppStmt);
	return (n == SQLITE_ROW)
		? (char*)sqlite3_column_text(ppStmt, 0)
		: NULL;
}


static void
sqlite__dir_iter_free ()
{
	g_clear_pointer(&ppStmt, sqlite3_finalize);
}

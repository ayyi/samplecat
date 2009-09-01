#include "../../config.h"
#include <stdio.h>
#include <string.h>
#include <sqlite3.h>
#include <gtk/gtk.h>
#include "dh-link.h"
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
static SamplecatResult result;
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
	COLUMN_NOTES,
	COLUMN_COLOUR,
};


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
	if (!table_exists) {
		char* errMsg = 0;
		//sqlite doesnt appear to have a date type so we use INTERGER instead. check the size.
		//TODO check VARCHAR is appropriate for Notes column.
		int n = sqlite3_exec(db, "CREATE TABLE samples ("
			"id INTEGER NOT NULL PRIMARY KEY AUTOINCREMENT, "
			"filename VARCHAR(100), filedir VARCHAR(100), keywords VARCHAR(60), pixbuf BLOB, length int(22), sample_rate int(11), channels int(4), "
			"online int(1), last_checked int(11), mimetype VARCHAR(32), notes VARCHAR(256), colour INTEGER(5))",
			on_create_table, 0, &errMsg);
		if (n != SQLITE_OK) {
			if (!strstr(errMsg, "already exists")) {
				perr("Sqlite error: %s\n", errMsg);
				return FALSE;
			}
		}
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
sqlite__insert(sample* sample, MIME_type *mime_type)
{
	//TODO is supposed to return the new id?

	int on_insert(void* NotUsed, int argc, char** argv, char** azColName)
	{
		PF;
		return 0;
	}

	int ret = 0;

	gchar* filedir = g_path_get_dirname(sample->filename);
	gchar* filename = g_path_get_basename(sample->filename);
	gchar* mime_str = g_strdup_printf("%s/%s", mime_type->media_type, mime_type->subtype);
	int colour = 0;
    gchar* sql = g_strdup_printf(
		"INSERT INTO samples(filename,filedir,length,sample_rate,channels,online,mimetype,colour) "
		"VALUES ('%s','%s','%i','%i','%i','%i','%s', '%i')",
		filename, filedir, sample->length, sample->sample_rate, sample->channels, 1, mime_str, colour
	);
	dbg(0, "sql=%s", sql);

	char* errMsg = 0;
    int n = sqlite3_exec(db, sql, on_insert, 0, &errMsg);
    if (n != SQLITE_OK) {
		gwarn("sqlite error: %s", errMsg);
		ret = -1;
	}

	g_free(sql);
	g_free(filedir);
	g_free(filename);
	g_free(mime_str);
	return ret;
}


#if 0
gboolean
sqlite__add_row (int argc, char *argv[])
{
    int ret = 0;
	char* errMsg = 0;

    /* Allocate memory */
    char* filename = g_malloc(MAX_LEN);
    char* filedir = g_malloc (MAX_LEN);
    char* query = g_malloc(MAX_LEN);

	int on_insert(void* NotUsed, int argc, char** argv, char** azColName)
	{
		PF;
		return 0;
	}

    /* Insert to database */
    sqlite3_snprintf (MAX_LEN, query, "INSERT INTO mypass VALUES ('%q','%q')", filename, filedir);
    printf ("query = %s\n", query);
    int n = sqlite3_exec(db, query, on_insert, 0, &errMsg);

    if (n != SQLITE_OK) {
        fprintf(stderr, "SQL error: %s\n", errMsg);
        ret = FALSE;
    }
    else
        ret = TRUE;

    g_free(filename);
    g_free(filedir);
    g_free(query);

    return ret;
}
#endif


gboolean
sqlite__update_colour(int id, int colour)
{
	return false;
}


gboolean
sqlite__update_keywords(int id, const char* keywords)
{
	return false;
}


gboolean
sqlite__update_notes(int id, const char* notes)
{
	return false;
}


#ifdef NEVER
static int
delete_row (int argc, char *argv[])
{
	int on_delete(void* NotUsed, int argc, char** argv, char** azColName)
	{
		PF;
		return 0;
	}

	int n = 0, ret = 0;
	char* errMsg = 0;

	/* Allocate memory */
	char* domain = g_malloc (MAX_LEN);
	char* query = g_malloc (MAX_LEN);

	/* Request user input */
	printf ("Enter domain, e.g. mail.yahoo.com: ");
	fgets (domain, MAX_LEN, stdin);

	/* remove trailing newline */
	domain[strlen(domain) - 1] = '\0';

	/* Execute to database */
	sqlite3_snprintf (MAX_LEN, query, "DELETE FROM mypass WHERE domain='%q'", domain);
	printf ("query = %s\n", query);

	/* Check return result */
	n = sqlite3_exec(db, query, on_delete, 0, &errMsg);
	if (n != SQLITE_OK) {
		fprintf(stderr, "SQL error: %s\n", errMsg);
		ret = 1;
	}
	else
		ret = 0;

	/* Free allocated memory */
	g_free(domain);
	g_free(query);
	return ret;
}
#endif


gboolean
sqlite__search_iter_new(char* search, char* dir)
{
	PF;
	char* where = "";
	char* sql = g_strdup_printf("SELECT * FROM samples WHERE 1 %s", where);
	dbg(2, "sql=%s", sql);

	if(ppStmt) gwarn("ppStmt not reset from previous query?");
	int n = sqlite3_prepare_v2(db, sql, -1, &ppStmt, NULL);
	if(n == SQLITE_OK && ppStmt){
	}
	else gwarn("failed to create prepared statement. sql=%s", sql);

#if 0
	int select_callback(void* NotUsed, int argc, char** argv, char** azColName)
	{
		//this is a per-row callback. If there are no selected rows, it will not be called.

		dbg(0, "n_columns=%i", argc);
		int i; for(i=0; i<argc; i++) printf("  %s = %s\n", azColName[i], argv[i] ? argv[i] : "NULL");
		printf ("\n");

		return 0; //non zero values cause SQLITE_ABORT to occur.
	}

	char* errMsg = NULL;
	n = sqlite3_exec(db, sql, select_callback, 0, &errMsg);
	g_free(sql);
	if(n != SQLITE_OK){
		gwarn("sqlite error: %i: %s", n, errMsg);
		sqlite3_free(errMsg);
		return FALSE;
	}
#endif
	return TRUE;
}


SamplecatResult*
sqlite__search_iter_next(unsigned long** lengths)
{
	memset(&result, 0, sizeof(SamplecatResult));

	int n = sqlite3_step(ppStmt);
	if(n == SQLITE_ROW){
		result.sample_name = (char*)sqlite3_column_text(ppStmt, COLUMN_FILENAME);
		result.dir         = (char*)sqlite3_column_text(ppStmt, COLUMN_DIR);
		result.keywords    = (char*)sqlite3_column_text(ppStmt, COLUMN_KEYWORDS);
		result.length      = sqlite3_column_int(ppStmt, COLUMN_LENGTH);
		result.sample_rate = sqlite3_column_int(ppStmt, COLUMN_SAMPLERATE);
		result.channels    = sqlite3_column_int(ppStmt, COLUMN_CHANNELS);
		result.overview    = (GdkPixbuf*)sqlite3_column_blob(ppStmt, COLUMN_PIXBUF);
		result.notes       = (char*)sqlite3_column_text(ppStmt, COLUMN_NOTES);
		result.colour      = sqlite3_column_int(ppStmt, COLUMN_COLOUR);
		result.mimetype    = (char*)sqlite3_column_text(ppStmt, COLUMN_MIMETYPE);
		return &result;
	}
	else return NULL;
}


void
sqlite__search_iter_free()
{
	sqlite3_finalize(ppStmt);
	ppStmt = NULL;
}



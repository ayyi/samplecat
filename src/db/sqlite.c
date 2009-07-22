#include "../../config.h"
#include <stdio.h>
#include <string.h>
#include <sqlite3.h>
#include <gtk/gtk.h>
#include "dh-link.h"
#include "typedefs.h"
#include <gqview2/typedefs.h>
#include <gimp/gimpaction.h>
#include <gimp/gimpactiongroup.h>

#include "support.h"
#include "db/sqlite.h"

static sqlite3* db;
extern int debug;
#define MAX_LEN 256 //temp!


void
sqlite__create_db()
{
}


int
sqlite__connect()
{
	PF;
	int rc;

	char* db_name = "test.sqlite";
	rc = sqlite3_open(db_name, &db); //if the file doesnt exist, it be created.
	if (rc) {
		fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db));
		sqlite3_close(db);
		return FALSE;
	}

/*
	char* zErrMsg = NULL;
	char* sql = "SELECT * FROM db";
	rc = sqlite3_exec(db, sql, callback, 0, &zErrMsg);
	if (rc != SQLITE_OK) {
		fprintf(stderr, "SQL error: %s\n", zErrMsg);
		sqlite3_free(zErrMsg);
		sqlite3_close(db);
		return FALSE;
	}
*/

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
		int n = sqlite3_exec(db, "CREATE TABLE samples (id INTEGER NOT NULL PRIMARY KEY AUTOINCREMENT, filename VARCHAR(100), filedir VARCHAR(100))", on_create_table, 0, &errMsg);
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
sqlite__insert(char* sql)
{
	PF;
	int on_insert(void* NotUsed, int argc, char** argv, char** azColName)
	{
		PF;
		return 0;
	}

	char* filename = "test";
	char* filedir = "/home/tim/tmp/ardour_sessions/new/interchange/new/audiofiles";
    gchar* q = g_strdup_printf("INSERT INTO samples(filename,filedir) VALUES ('%s','%s')", filename, filedir);

	char* errMsg = 0;
    int n = sqlite3_exec(db, q, on_insert, 0, &errMsg);
    if (n != SQLITE_OK) {
		gwarn("sqlite error: %s", errMsg);
		return -1;
	}
	return 0;
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

gboolean
sqlite__search_iter_new(char* search, char* dir)
{
	PF;
	int select_callback(void* NotUsed, int argc, char** argv, char** azColName)
	{
		//this appears to be a per-row callback! If there are no selected rows, it will not be called.

		dbg(0, "n_columns=%i", argc);
		int i; for(i=0; i<argc; i++) printf("  %s = %s\n", azColName[i], argv[i] ? argv[i] : "NULL");
		printf ("\n");

		return TRUE;
	}

	char* where = "";
	char* sql = g_strdup_printf("SELECT * FROM samples WHERE 1 %s", where);
	char* errMsg = NULL;
	dbg(0, "sql=%s", sql);
	int n = sqlite3_exec(db, sql, select_callback, 0, &errMsg);
	g_free(sql);
	if(n != SQLITE_OK){
		gwarn("sqlite error: %s", errMsg);
		return FALSE;
	}
	return TRUE;
}


void
sqlite__search_iter_free()
{
}



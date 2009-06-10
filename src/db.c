#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <gtk/gtk.h>
#ifdef OLD
  #include <libart_lgpl/libart.h>
#endif

#include <mysql/mysql.h>
#include <mysql/errmsg.h>
#include "dh-link.h"
#include "typedefs.h"
#include <gqview2/typedefs.h>
#include "support.h"
#include "main.h"
#include "db.h"

//#if defined(HAVE_STPCPY) && !defined(HAVE_mit_thread)
  #define strmov(A,B) stpcpy((A),(B))
//#endif

extern struct _app app;
extern char err [32];
extern char warn[32];
extern unsigned debug;

static gboolean is_connected = FALSE;
static MYSQL_RES *dir_iter_result = NULL;
static MYSQL_RES *search_result = NULL;


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
	  PRIMARY KEY  (`id`)
	)

	BLOB can handle up to 64k.

*/
	/*
	Calls:
	MYSQL *mysql_init(MYSQL *mysql)
	char *mysql_get_server_info(MYSQL *mysql)
	void mysql_close(MYSQL *mysql)
	*/
 
	/*
	MYSQL *mysql_real_connect(MYSQL *mysql, const char *host, const char *user, const char *passwd, const char *db,
			unsigned int port, const char *unix_socket, unsigned int client_flag)
	*/

gboolean
db__connect()
{
	MYSQL *mysql = &app.mysql;

	if(!app.mysql.host){
		if(!mysql_init(&app.mysql)){
			printf("Failed to initiate MySQL connection.\n");
			exit(1);
		}
	}
	dbg (1, "MySQL Client Version is %s", mysql_get_client_info());
	app.mysql.reconnect = true;

	if(!mysql_real_connect(mysql, app.config.database_host, app.config.database_user, app.config.database_pass, app.config.database_name, 0, NULL, 0)){
		errprintf("cannot connect to database: %s\n", mysql_error(mysql));
		//currently this wont be displayed, as the window is not yet opened.
		statusbar_printf(1, "cannot connect to database: %s\n", mysql_error(mysql));
		return false;
	}
	if(debug) printf("MySQL Server Version is %s\n", mysql_get_server_info(mysql));

	if(mysql_select_db(mysql, app.config.database_name)){
		errprintf("Failed to connect to Database: %s\n", mysql_error(mysql));
		return false;
	}

	is_connected = true;
	return true;
}
 

gboolean
db__is_connected()
{
	return is_connected;
}


int
db__insert(char *qry)
{
	MYSQL *mysql = &app.mysql;
	int id = 0;

	if(mysql_exec_sql(mysql, qry)==0){
		if(debug) printf("db_insert(): ok!\n");
		id = mysql_insert_id(mysql);
	}else{
		perr("not ok...\n");
		return 0;
	}
	return id;
}


gboolean
db__delete_row(int id)
{
	char sql[1024];
	snprintf(sql, 1024, "DELETE FROM samples WHERE id=%i", id);
	dbg(2, "row: sql=%s", sql);
	if(mysql_query(&app.mysql, sql)){
		perr("delete failed! sql=%s\n", sql);
		return false;
	}
	return true;
}


int 
mysql_exec_sql(MYSQL *mysql, const char *create_definition)
{
	return mysql_real_query(mysql, create_definition, strlen(create_definition));
}


gboolean
db__update_path(const char* old_path, const char* new_path)
{
	gboolean ok = false;
	MYSQL *mysql = &app.mysql;

	char* filename = NULL; //FIXME
	char* old_dir = NULL; //FIXME

	char query[1024];
	snprintf(query, 1023, "UPDATE samples SET filedir='%s' WHERE filename='%s' AND filedir='%s'", new_path, filename, old_dir);
	dbg(0, "%s", query);

	if(!mysql_exec_sql(mysql, query)){
		ok = TRUE;
	}
	return ok;
}


//-------------------------------------------------------------


gboolean
db__search_iter_new(char* search, char* dir)
{
	//return TRUE on success.

	MYSQL *mysql = &app.mysql;
	char query[1024];
	char where[512]="";
	char category[256]="";
	char where_dir[512]="";

	if(!search) search = app.search_phrase;
	if(!dir)    dir    = app.search_dir;

	if(search_result) gwarn("previous query not free'd?");

	if(app.search_category) snprintf(category, 256, "AND keywords LIKE '%%%s%%' ", app.search_category);

	//lets assume that the search_phrase filter should *always* be in effect.

	if(dir && strlen(dir)){
		snprintf(where_dir, 512, "AND filedir='%s' %s ", dir, category);
	}

	//strmov(dst, src) moves all the  characters  of  src to dst, and returns a pointer to the new closing NUL in dst. 
	//strmov(query_def, "SELECT * FROM samples"); //this is defined in mysql m_string.h
	//strcpy(query, "SELECT * FROM samples WHERE 1 ");

	if(strlen(search)){ 
		snprintf(where, 512, "AND (filename LIKE '%%%s%%' OR filedir LIKE '%%%s%%' OR keywords LIKE '%%%s%%') ", search, search, search); //FIXME duplicate category LIKE's
	}

	//append the dir-where part:
	char* a = where + strlen(where);
	strmov(a, where_dir);

	snprintf(query, 1024, "SELECT * FROM samples WHERE 1 %s", where);

	dbg(1, "%s", query);

	int e; if((e = mysql_exec_sql(mysql, query)) != 0){
		statusbar_printf(1, "Failed to find any records: %s", mysql_error(mysql));

		if((e == CR_SERVER_GONE_ERROR) || (e == CR_SERVER_LOST)){ //default is to time out after 8 hours
			dbg(0, "mysql connection timed out. Reconnecting... (not tested!)");
			db__connect();
		}else{
			dbg(0, "Failed to find any records: %s", mysql_error(mysql));
		}
		return false;
	}
	search_result = mysql_store_result(mysql);
	return true;
}


MYSQL_ROW
db__search_iter_next(unsigned long** lengths)
{
	if(!search_result) return NULL;

	MYSQL_ROW row;
	row = mysql_fetch_row(search_result);
	if(!row) return NULL;
	*lengths = mysql_fetch_lengths(search_result); //free? 
	return row;
}


void
db__search_iter_free()
{
	if(search_result) mysql_free_result(search_result);
	search_result = NULL;
}


//-------------------------------------------------------------


void
db__dir_iter_new()
{
	#define DIR_LIST_QRY "SELECT DISTINCT filedir FROM samples ORDER BY filedir"
	MYSQL *mysql = &app.mysql;

	if(dir_iter_result) gwarn("previous query not free'd?");

	//char qry[1024];
	//snprintf(qry, 1024, DIR_LIST_QRY);

	if(mysql_exec_sql(mysql, DIR_LIST_QRY) == 0){
		dir_iter_result = mysql_store_result(mysql);
		/*
		else{  // mysql_store_result() returned nothing
		  if(mysql_field_count(mysql) > 0){
				// mysql_store_result() should have returned data
				printf( "Error getting records: %s\n", mysql_error(mysql));
		  }
		}
		*/
		dbg(2, "num_rows=%i", mysql_num_rows(dir_iter_result));
	}
	else{
		dbg(0, "failed to find any records: %s", mysql_error(mysql));
	}
}


char*
db__dir_iter_next()
{
	if(!dir_iter_result) return NULL;

	MYSQL_ROW row;
	row = mysql_fetch_row(dir_iter_result);
	if(!row) return NULL;

	//printf("update_dir_node_list(): dir=%s\n", row[0]);

	/*
	char uri[256];
	snprintf(uri, 256, "%s", row[0]);

	return uri;
	*/
	return row[0];
}


void
db__dir_iter_free()
{
	if(dir_iter_result) mysql_free_result(dir_iter_result);
	dir_iter_result = NULL;
}



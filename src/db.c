#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <gtk/gtk.h>
#include <gdk-pixbuf/gdk-pixdata.h>

#include <mysql/mysql.h>
#include <mysql/errmsg.h>
#include "dh-link.h"
#include "typedefs.h"
#include "src/types.h"
#include <gqview2/typedefs.h>
#include "support.h"
#include "main.h"
#include "rox/rox_global.h"
#include "mimetype.h"
#include "pixmaps.h"
#include "listview.h"
#include "db.h"

//#if defined(HAVE_STPCPY) && !defined(HAVE_mit_thread)
  #define strmov(A,B) stpcpy((A),(B))
//#endif

//mysql table layout (database column numbers):
enum {
  MYSQL_NAME = 1,
  MYSQL_DIR = 2,
  MYSQL_KEYWORDS = 3,
  MYSQL_PIXBUF = 4,
  MYSQL_ONLINE = 8,
  MYSQL_MIMETYPE = 10,
};
#define MYSQL_NOTES 11
#define MYSQL_COLOUR 12

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


void
db__iter_to_result(SamplecatResult* result)
{
	memset(result, 0, sizeof(SamplecatResult));
}


void
db__add_row_to_model(MYSQL_ROW row, unsigned long* lengths)
{
	GdkPixbuf* iconbuf = NULL;
	GdkPixdata pixdata;
	char length[64];
	char keywords[256];
	char sample_name[256];
	float samplerate; char samplerate_s[32];
	unsigned channels, colour;
	gboolean online = FALSE;
	GtkTreeIter iter;

	//db__iter_to_result(result);

	/*
	for(i = 0; i < num_fields; i++){ 
		printf("[%s] ", row[i] ? row[i] : "NULL"); 
	}
	printf("\n"); 
	*/

	//deserialise the pixbuf field:
	GdkPixbuf* pixbuf = NULL;
	if(row[MYSQL_PIXBUF]){
		//printf("%s(): deserializing...\n", __func__); 
		if(gdk_pixdata_deserialize(&pixdata, lengths[4], (guint8*)row[MYSQL_PIXBUF], NULL)){
			pixbuf = gdk_pixbuf_from_pixdata(&pixdata, TRUE, NULL);
		}
	}

	format_time(length, row[5]);
	if(row[MYSQL_KEYWORDS]) snprintf(keywords, 256, "%s", row[MYSQL_KEYWORDS]); else keywords[0] = 0;
	//if(row[5]==NULL) length     = 0; else length     = atoi(row[5]);
	if(row[6]==NULL) samplerate = 0; else samplerate = atoi(row[6]); samplerate_format(samplerate_s, samplerate);
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

	//strip the homedir from the dir string:
	char* path = strstr(row[MYSQL_DIR], g_get_home_dir());
	path = path ? path + strlen(g_get_home_dir()) + 1 : row[MYSQL_DIR];

	gtk_list_store_append(app.store, &iter);
	gtk_list_store_set(app.store, &iter, COL_ICON, iconbuf,
#ifdef USE_AYYI
	                   COL_AYYI_ICON, ayyi_icon,
#endif
	                   COL_IDX, atoi(row[0]), COL_NAME, sample_name, COL_FNAME, path, COL_KEYWORDS, keywords, 
	                   COL_OVERVIEW, pixbuf, COL_LENGTH, length, COL_SAMPLERATE, samplerate_s, COL_CHANNELS, channels, 
	                   COL_MIMETYPE, row[MYSQL_MIMETYPE], COL_NOTES, row[MYSQL_NOTES], COL_COLOUR, colour, -1);
	if(pixbuf) g_object_unref(pixbuf);
}



#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <gtk/gtk.h>
#include <libart_lgpl/libart.h>

#include "mysql/mysql.h"
#include "dh-link.h"
#include "main.h"
#include "support.h"
#include "db.h"
extern struct _app app;
extern char err [32];
extern char warn[32];
extern unsigned debug;
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
 
gboolean
db_connect()
{
	MYSQL *mysql;

	if(mysql_init(&(app.mysql))==NULL){
		printf("Failed to initiate MySQL connection.\n");
		exit(1);
	}
	if(debug) printf("MySQL Client Version is %s\n", mysql_get_client_info());

	mysql = &app.mysql;

	if(!mysql_real_connect(mysql, app.config.database_host, app.config.database_user, app.config.database_pass, "test", 0, NULL, 0)){
		errprintf("cannot connect to database: %s\n", mysql_error(mysql));
		return FALSE;
		exit(1);
	}
	if(debug) printf("MySQL Server Version is %s\n", mysql_get_server_info(mysql));
	//printf("Logged on to database.\n");

	if(!mysql_select_db(mysql, app.config.database_name /*const char *db*/)==0)/*success*/{
		//printf( "Database Selected.\n");
	//else
		errprintf("Failed to connect to Database: Error: %s\n", mysql_error(mysql));
		return FALSE;
	}

	return TRUE;
}
 

int
db_insert(char *qry)
{
	MYSQL *mysql = &app.mysql;
	int id = 0;

	if(mysql_exec_sql(mysql, qry)==0){
		printf("db_insert(): ok!\n");
		id = mysql_insert_id(mysql);
	}else{
		errprintf("db_insert(): not ok...\n");
		return 0;
	}
	return id;
}


int 
mysql_exec_sql(MYSQL *mysql, const char *create_definition)
{
   return mysql_real_query(mysql,create_definition,strlen(create_definition));
}



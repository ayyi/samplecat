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

gboolean
db_connect()
{
	MYSQL *mysql;

	if(mysql_init(&(app.mysql))==NULL){
		printf("Failed to initiate MySQL connection.\n");
		exit(1);
	}
	printf("MySQL Client Version is %s\n", mysql_get_client_info());

	mysql = &app.mysql;

	if(!mysql_real_connect(mysql, "localhost", "root", "hlongporto", "test", 0, NULL, 0)){
		printf("cannot connect. %s\n", mysql_error(mysql));
		exit(1);
	}
	printf("MySQL Server Version is %s\n", mysql_get_server_info(mysql));
	//printf("Logged on to database.\n");

	if(!mysql_select_db(mysql, app.config.database_name /*const char *db*/)==0)/*success*/{
		//printf( "Database Selected.\n");
	//else
		printf( "Failed to connect to Database: Error: %s\n", mysql_error(mysql));
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



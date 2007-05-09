/*
 * (C)2007 Dustin Spicuzza
 *
 *
 *
 */

#include <config.h>
#include <linux/limits.h>

#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <unistd.h>

#include <mysql/mysql.h>
#include <gps.h>

// gpsd error timeout, in seconds
#define GPSD_TIMEOUT 10
// gpsd poll interval, in seconds
#define GPSD_POLL_INTERVAL 10

void child_function(void);

MYSQL mysql;

int main(int argc, char ** argv){

	pid_t pid;

	// validate args

	// ignore kids
	signal(SIGCLD,SIG_IGN);

	// establish a connection to the MySQL server, or die
	mysql_init(&mysql);

	if (mysql_real_connect(&mysql,"localhost","gpslog","somepassword","gpslog",0,NULL,0) != &mysql){
		fprintf(stderr,"Failed to connect to database: %s\n",mysql_error(&mysql));
		exit(EXIT_FAILURE);
	}

	// fork off :p
	//pid = fork();
	pid = 0;

	switch(pid){
		case 0:
			// close parent handles
			
			// call child function
			child_function();
			break;
		
		case -1:
			// this shouldn't generally happen
			perror("fork");
			exit(EXIT_FAILURE);
		default:
			// do nothing, we really don't care
			break;
	}	

	// cleanup
	mysql_library_end();

	exit(EXIT_SUCCESS);
}


/*
 *	This function does all the actual work
 */
void child_function(){

//	if (mysql_query(&mysql,"INSERT INTO points (data) VALUES ('boo')"))
//		fprintf(stderr,"Error in mysql query: %s\n",mysql_error(&mysql));

	struct gps_data_t * gps_data;

	while (1){

		// open a connection to gpsd
		gps_data = gps_open("localhost",DEFAULT_GPSD_PORT);

		// if it fails, then wait a timeout period and try again
		if (gps_data == NULL){
			// probably should use syslog..
			fprintf(stderr,"gps_open error: %d\n",errno);
		}else{

			// continuously loop and log
			while (1){
				ret = gps_query(

				// lost connection, try again
				if (ret == -1)
					break;


		}

		// wait some timeout period before trying to connect again
		sleep(GPSD_TIMEOUT);
	}

	mysql_library_end();
	exit(EXIT_SUCCESS);
}


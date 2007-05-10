/*
* Copyright (c) 2007, Dustin Spicuzza
* All rights reserved.
* Redistribution and use in source and binary forms, with or without
* modification, are permitted provided that the following conditions are met:
*
*     * Redistributions of source code must retain the above copyright
*       notice, this list of conditions and the following disclaimer.
*     * Redistributions in binary form must reproduce the above copyright
*       notice, this list of conditions and the following disclaimer in the
*       documentation and/or other materials provided with the distribution.
*     * Neither the name of Dustin Spicuzza nor the
*       names of any contributors may be used to endorse or promote products
*       derived from this software without specific prior written permission.
*
* THIS SOFTWARE IS PROVIDED BY DUSTIN SPICUZZA AND CONTRIBUTORS ``AS IS'' AND ANY
* EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
* WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
* DISCLAIMED. IN NO EVENT SHALL DUSTIN SPICUZZA AND CONTRIBUTORS BE LIABLE FOR ANY
* DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
* (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
* LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
* ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
* (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
* SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*
*/

#define _GNU_SOURCE

#include <config.h>

#include <stdint.h>
#include <linux/limits.h>

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <math.h>

#include <mysql/mysql.h>
#include <gps.h>

// gpsd error timeout, in seconds
#define GPSD_TIMEOUT 10
// gpsd poll interval, in seconds
#define GPSD_POLL_INTERVAL 10
// sql statement to be executed
#define SQL_INSERT_STMT "INSERT INTO points (time,latitude,longitude,course,speed,climb,altitude) VALUES (?,?,?,?,?,?,?)"


void child_function(void);

// globals.. ew
MYSQL mysql;
MYSQL_STMT * stmt;

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

	stmt = mysql_stmt_init(&mysql);

	// prepare the statement
	if (!stmt || mysql_stmt_prepare(stmt,SQL_INSERT_STMT,sizeof(SQL_INSERT_STMT))){
		fprintf(stderr,"Failed to create sql statement: %s\n",mysql_stmt_error(stmt));
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

	int ret,i;
	double nan = NAN;
	struct gps_data_t * gps_data;
	struct gps_fix_t * gps_fix;

	// initialize the bind structure
	MYSQL_BIND bind[7];
	memset(bind,0,sizeof(bind));
	for (i = 0;i < 7;i++){
		bind[i].buffer_type = MYSQL_TYPE_DOUBLE;
	}


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
				
				ret = gps_query(gps_data,"o\n");

				// lost connection, try again
				if (ret == -1){
					perror("gps_query");
					break;
				}

				// do something with the data we care about
				if (!gps_data->online){
					fprintf(stderr,"gpsd: offline\n");
				}else{
					
					gps_fix = &gps_data->fix;

					printf("gpsd response: time %f ",gps_fix->time);
					bind[0].buffer = &gps_fix->time;

					// the gps_fix_t structure contains the interesting info we want.. 
					switch (gps_fix->mode){
						case MODE_NOT_SEEN:
							printf("Fix: Searching...");
							break;

						case MODE_NO_FIX:
							printf("Fix: None");
							break;

						case MODE_2D:
							printf("Fix: 2d Lat: %f Long: %f Course: %f Speed: %f Climb: %f",
								gps_fix->latitude,
								gps_fix->longitude,
								gps_fix->track,
								gps_fix->speed,
								gps_fix->climb);

							bind[1].buffer = (char*)&(gps_fix->latitude);
							bind[2].buffer = (char*)&gps_fix->longitude;
							bind[3].buffer = (char*)&gps_fix->track;
							bind[4].buffer = (char*)&gps_fix->speed;
							bind[5].buffer = (char*)&gps_fix->climb;
							bind[6].buffer = (char*)&nan;

							if (mysql_stmt_bind_param(stmt,bind) || mysql_stmt_execute(stmt))
								fprintf(stderr,"error executing mysql statement: %s\n",mysql_stmt_error(stmt));

							break;

						case MODE_3D:
							printf("Fix: 3d Lat: %f Long: %f Course: %f Speed: %f Climb: %f Alt: %f",
								gps_fix->latitude,
								gps_fix->longitude,
								gps_fix->track,
								gps_fix->speed,
								gps_fix->climb,
								gps_fix->altitude);

                            bind[1].buffer = &gps_fix->latitude;
							bind[2].buffer = &gps_fix->longitude;
							bind[3].buffer = &gps_fix->track;
							bind[4].buffer = &gps_fix->speed;
							bind[5].buffer = &gps_fix->climb;
							bind[6].buffer = &gps_fix->altitude;

							if (mysql_stmt_bind_param(stmt,bind) || mysql_stmt_execute(stmt))
								fprintf(stderr,"error executing mysql statement: %s\n",mysql_stmt_error(stmt));

							break;

						default:
							fprintf(stderr,"invalid fix data!\n");		
					}

					printf("\n");

				}

				// wait some interval before doing it again
				sleep(GPSD_POLL_INTERVAL);
			}
		}

		// wait some timeout period before trying to connect again
		sleep(GPSD_TIMEOUT);
	}

	mysql_library_end();
	exit(EXIT_SUCCESS);
}


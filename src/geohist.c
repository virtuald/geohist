/*
* Copyright (c) 2007-2008, Dustin Spicuzza
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

#include <sys/types.h>
#include <pwd.h>
#include <time.h>

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

#include "geohist.h"

// globals.. ew
MYSQL mysql;
MYSQL_STMT * stmt;

void child_function(void);

int main(int argc, char ** argv){

	pid_t pid;
#ifndef DEBUG
	struct passwd * pw;

	// ignore kids, drop permissions
	signal(SIGCLD,SIG_IGN);
#endif

	// establish a connection to the MySQL server, or die
	mysql_init(&mysql);

	if (mysql_real_connect(&mysql,SQL_SERVER,SQL_USERNAME,SQL_PASSWORD,SQL_DATABASE,SQL_PORT,NULL,0) != &mysql){
		fprintf(stderr,"Failed to connect to database: %s\n",mysql_error(&mysql));
		exit(EXIT_FAILURE);
	}

	stmt = mysql_stmt_init(&mysql);

	// prepare the statement
	if (!stmt || mysql_stmt_prepare(stmt,SQL_INSERT_STMT,sizeof(SQL_INSERT_STMT))){
		fprintf(stderr,"Failed to create sql statement: %s\n",mysql_stmt_error(stmt));
		exit(EXIT_FAILURE);
	}

	    // ignore kids, drop permissions
#ifndef DEBUG
	
	signal(SIGCLD,SIG_IGN);

	// we need zero permissions to do what we need
	if ((pw = getpwnam("nobody"))){
		setuid(pw->pw_uid);
		setgid(pw->pw_gid);
	}

	// fork off :p
	pid = fork();

#else
	pid = 0;
#endif

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

	int ret,i,first_time = 1;
	double nan = NAN, last_fix_time;

	double gps_last_latitude = NAN, drift_latitude;
	double gps_last_longitude = NAN, drift_longitude;

	struct gps_data_t * gps_data;
	struct gps_fix_t * gps_fix;
	
	// last_track_id is the last one written to the SQL database, current is the one 
	// that should be written for the current SQL statement
	long long int last_track_id = 0, current_track_id = 0;
	double last_track_time = 0;
	
	int err;
	MYSQL_RES * result;
	MYSQL_ROW row;

	// drop the controlling tty and etc -- who cares if it fails though?
	setsid();

	// get the biggest track_id
	if ((err = mysql_query(&mysql, "SELECT track_id, time FROM points ORDER BY id DESC LIMIT 1")))
	{
		fprintf(stderr,"Aborting: error finding track_id: %s\n",mysql_error(&mysql));
		return;
	}
	
	if ((result = mysql_store_result(&mysql)))
	{
		if ((row = mysql_fetch_row(result)))
		{
			last_track_id = atoll(row[0]);
			last_track_time = atof(row[1]);
		}
		mysql_free_result(result);
	}
	
	GPSLOG_OUT("last track_id was %lld\n", last_track_id);
	
	// initialize the bind structure
	MYSQL_BIND bind[8];
	memset(bind,0,sizeof(bind));
	bind[0].buffer_type = MYSQL_TYPE_LONGLONG;
	for (i = 1;i < 8;i++)
		bind[i].buffer_type = MYSQL_TYPE_DOUBLE;
	
	last_fix_time = time(NULL);

	while (42){

		// open a connection to gpsd
		gps_data = gps_open("localhost",DEFAULT_GPSD_PORT);

		// if it fails, then wait a timeout period and try again
		if (gps_data == NULL){
			GPSLOG_OUT("gps_open error: %d\n",errno);
		}else{

			// continuously loop and log
			while (42){
				
				ret = gps_query(gps_data,"o\n");

				// lost connection, try again
				if (ret == -1){
					perror("gps_query");
					break;
				}

				// do something with the data we care about
				if (!gps_data->online){
					GPSLOG_OUT("gpsd: offline\n");
				}else{
					
					gps_fix = &gps_data->fix;

					// if the timestamp is wrong.. (before 1/1/2000), then try to compensate
					if (gps_fix->time < 946702800)
					{
						// we assume this is close enough
						GPSLOG_OUT("gpsd returned invalid timestamp: %f ", gps_fix->time);
						last_fix_time += 10;
					} else
						last_fix_time = gps_fix->time;
										
					// increment the track id if necessary
					if (last_track_time + TRACK_SEGMENT_TIMEOUT < last_fix_time)
						current_track_id = last_track_id + 1;
					else
						current_track_id = last_track_id;

					// write to the bind structure
					GPSLOG_OUT("timestamp for fix: time %f ", last_fix_time);
					bind[0].buffer = &current_track_id;
					bind[1].buffer = &last_fix_time;
						
					// sometimes, these numbers will be nonsense, but thats ok.. :)
					drift_latitude = fabs(gps_fix->latitude - gps_last_latitude);
					drift_longitude = fabs(gps_fix->longitude - gps_last_longitude);

					// the gps_fix_t structure contains the interesting info we want.. 
					switch (gps_fix->mode){
						case MODE_NOT_SEEN:
							GPSLOG_OUT("Fix: Searching...");
							break;

						case MODE_NO_FIX:
							GPSLOG_OUT("Fix: None");
							break;

						case MODE_2D:
							
							// decide whether we want to ignore this value or not
							// don't record the value if we ignored it
							if (!first_time && drift_latitude < GPS_DRIFT && drift_longitude < GPS_DRIFT){

								GPSLOG_OUT("Ignored! Drift val: lat %f long %f\n",drift_latitude,drift_longitude);
								break;
							}
							
							first_time = 0;

							GPSLOG_OUT("Fix: 2d Lat: %f Long: %f Course: %f Speed: %f Climb: %f",
								gps_fix->latitude,
								gps_fix->longitude,
								gps_fix->track,
								gps_fix->speed,
								gps_fix->climb);

							gps_last_latitude = gps_fix->latitude;
							gps_last_longitude = gps_fix->longitude; 

							bind[2].buffer = (char*)&(gps_fix->latitude);
							bind[3].buffer = (char*)&gps_fix->longitude;
							bind[4].buffer = (char*)&gps_fix->track;
							bind[5].buffer = (char*)&gps_fix->speed;
							bind[6].buffer = (char*)&gps_fix->climb;
							bind[7].buffer = (char*)&nan;

							if (mysql_stmt_bind_param(stmt,bind) || mysql_stmt_execute(stmt))
								fprintf(stderr,"error executing mysql statement: %s\n",mysql_stmt_error(stmt));

							// for keeping track of seperate tracks
							last_track_time = last_fix_time;
							last_track_id = current_track_id;
								
							break;

						case MODE_3D:

							// decide whether we want to ignore this value or not
							// don't record the value if we ignored it
							if (!first_time && drift_latitude < GPS_DRIFT && drift_longitude < GPS_DRIFT){
								GPSLOG_OUT("Ignored! Drift val: lat %f long %f\n",drift_latitude,drift_longitude);
								break;
							}

							first_time = 0;

							GPSLOG_OUT("Fix: 3d Lat: %f Long: %f Course: %f Speed: %f Climb: %f Alt: %f",
								gps_fix->latitude,
								gps_fix->longitude,
								gps_fix->track,
								gps_fix->speed,
								gps_fix->climb,
								gps_fix->altitude);

							gps_last_latitude = gps_fix->latitude;
							gps_last_longitude = gps_fix->longitude;

                            bind[2].buffer = &gps_fix->latitude;
							bind[3].buffer = &gps_fix->longitude;
							bind[4].buffer = &gps_fix->track;
							bind[5].buffer = &gps_fix->speed;
							bind[6].buffer = &gps_fix->climb;
							bind[7].buffer = &gps_fix->altitude;

							if (mysql_stmt_bind_param(stmt,bind) || mysql_stmt_execute(stmt))
								GPSLOG_OUT("error executing mysql statement: %s",mysql_stmt_error(stmt));

							// for keeping track of seperate tracks
							last_track_time = last_fix_time;
							last_track_id = current_track_id;
								
							break;

						default:
							GPSLOG_OUT("invalid fix data!");		
					}

					GPSLOG_OUT("\n");

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


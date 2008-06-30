/*
 *	Static configuration parameters, because I'm lazy. :)
 *
 */

#ifndef __GPSLOG_H
#define __GPSLOG_H


// these should all be in a configuration file somewhere...

// gpsd error timeout, in seconds
#define GPSD_TIMEOUT 10
// gpsd poll interval, in seconds
#define GPSD_POLL_INTERVAL 10
// drift value
#define GPS_DRIFT 0.0001

// track segment timeout: 10 minutes
#define TRACK_SEGMENT_TIMEOUT 600

// sql statement to be executed
#define SQL_INSERT_STMT "INSERT INTO points (track_id,time,latitude,longitude,course,speed,climb,altitude) VALUES (?,?,?,?,?,?,?,?)"
// sql server
#define SQL_SERVER "localhost"
// sql database
#define SQL_USERNAME "geohist"
// sql password
#define SQL_PASSWORD "somepassword"
// sql database
#define SQL_DATABASE "geohist"
// sql port
#define SQL_PORT 0


#ifdef DEBUG
#define GPSLOG_OUT(fmt,...) fprintf(stderr,fmt,## __VA_ARGS__)
#else
#define GPSLOG_OUT(fmt,...) //
#endif

#endif




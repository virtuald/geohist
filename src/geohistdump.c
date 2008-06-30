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
#include <time.h>
#include <stdint.h>
#include <getopt.h>

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <math.h>

#include <mysql/mysql.h>

#include "geohist.h"

// globals.. ew
MYSQL mysql;
MYSQL_STMT * stmt;

// decls
int process_tracks(char * tracks, int ** track_list, int * track_count);

void list_tracks()
{	
	int err;
	MYSQL_RES * result, * iresult;
	MYSQL_ROW row, irow;
	char * tmp, * tmpL, * tmpH;
	time_t tmh, tml;
	
	printf("Track\tPoints\tTime period\n-------------------------------\n");

	// get the list of track id's
	if (mysql_query(&mysql, "SELECT DISTINCT track_id FROM points ORDER BY track_id ASC"))
	{
		fprintf(stderr,"Aborting: error finding all track_id: %s\n",mysql_error(&mysql));
		return;
	}
	
	if (result = mysql_store_result(&mysql))
	{
		while (row = mysql_fetch_row(result))
		{
			asprintf(&tmp, "SELECT COUNT(id), MIN(time), MAX(time) FROM points WHERE track_id = %s", row[0]);
			if (err = mysql_query(&mysql, tmp))
			{
				fprintf(stderr, "Error retrieving times for track_id %s: %s\n", row[0], mysql_error(&mysql));
				break;
			}
			
			if (!(iresult = mysql_store_result(&mysql)))
			{
				fprintf(stderr, "Error storing times for track_id %s: %s\n", row[0], mysql_error(&mysql));
				break;
			}
			
			free(tmp);
			
			irow = mysql_fetch_row(iresult);
			tml = (time_t)atof(irow[1]);
			tmh = (time_t)atof(irow[2]);
			
			tmpL = strdup(ctime(&tml));
			tmpH = strdup(ctime(&tmh));
			
			// remove endline
			tmpL[strlen(tmpL)-1] = '\0';
			tmpH[strlen(tmpH)-1] = '\0';
			
			printf("%s\t%s\t%s to %s\n", row[0], irow[0], tmpL, tmpH);
			
			free(tmpL);
			free(tmpH);
			
			mysql_free_result(iresult);
		}	
		mysql_free_result(result);
	}
}

int delete_tracks(int * track_list, int track_count, int flag_f)
{
	printf("Deleting tracks! :o\n");
}

int output_tracks(char * filename, int * track_list, int track_count,
	char * name, char * author, char * desc, char * email)
{
	FILE * file;
	MYSQL_RES * result;
	MYSQL_ROW row;
	int i, m_err;
	char * tok, *tmp1, *tmp2;
	char ftime[100];
	time_t tm;
	long long int track_id, last_track_id = -1;
	
	if ( !strcmp(filename,"-"))
		file = stdout;
	else
		if ( !(file = fopen(filename, "w+")))
			perror("output_tracks");
			
	// output GPX header, then each track
	fprintf(file, 
		"<?xml version=\"1.0\"?>\n"
		"<gpx version=\"1.1\"\n"
		"  creator=\"%s\"\n"
		"  xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\" xmlns=\"http://www.topografix.com/GPX/1/1\"\n"
		"  xsi:schemaLocation=\"http://www.topografix.com/GPX/1/1 http://www.topografix.com/GPX/1/1/gpx.xsd\">\n",
		PACKAGE_STRING
	);
	
	fprintf(file, "<metadata>\n");
	
	// extra GPX stuff
	if (name)
		fprintf(file, "\t<name>%s</name>", name);
	if (desc)
		fprintf(file, "\t<desc>%s</desc>", name);
	if (author || email)
	{
		fprintf(file, "\t<author>\n");
		if (author)
			fprintf(file, "\t\t<name>%s</name>\n", author);
		if (email)
		{
			if (!(tok = strchr(email,'@')) || strchr(tok+1, '@'))
			{
				return EXIT_FAILURE;
			}
			*tok = '\0';
			fprintf(file, "\t\t<email id=\"%s\" domain=\"%s\" />\n", email, tok+1);
		}
		fprintf(file, "</author>\n");
	}
	
	tm = time(NULL);
	
	if (!strftime(ftime, sizeof(ftime), "%a %b %d %H:%M:%S %Y", localtime(&tm)))
	{
		fprintf(stderr,"Error creating timestamp\n");
		return EXIT_FAILURE;	
	}
	
	fprintf(file, "\t<time>%s</time>\n</metadata>\n", ftime);
	
	// go through the list of tracks, generate the GPX file
	// if track_count == 0, then we have to scan everything, otherwise
	// we just scan
	
	if (track_count == 0)
		m_err = mysql_query(&mysql, "SELECT track_id, time, latitude, longitude, course, speed, climb, altitude FROM points ORDER BY id ASC");
	else if (track_count == 1)
	{
		asprintf(&tmp1,"SELECT track_id, time, latitude, longitude, course, speed, climb, altitude FROM points WHERE track_id = %d ORDER BY id ASC", track_list[0]);
		m_err = mysql_query(&mysql, tmp1);
		free(tmp1);
	}
	else
	{
		// build a giant query string :o
		asprintf(&tmp2, "track_id = %d", track_list[0]);
		for (i = 1; i < track_count; i++)
		{
			if (track_list[i] != -1)
			{
				asprintf(&tmp1,"%s OR track_id = %d", tmp2, track_list[i]);
				free(tmp2);
				tmp2 = tmp1;
			}
		}
		
		asprintf(&tmp1, "SELECT track_id, time, latitude, longitude, course, speed, climb, altitude FROM points WHERE %s ORDER BY id ASC", tmp2);
		m_err = mysql_query(&mysql, tmp1);
		
		free(tmp1);
		free(tmp2);
	}
	
	if (m_err)
	{
		fprintf(stderr, "Error in mysql query: %s", mysql_error(&mysql));
		return EXIT_FAILURE;
	}

	fprintf(file,"\t<trk>\n");
	
	// now go through the result set and build XML out of it
	if (result = mysql_store_result(&mysql))
	{
		while (row = mysql_fetch_row(result))
		{
			track_id = atoll(row[0]);
			
			// check to see if the track_id changed, then we need to generate a new track segment
			if (last_track_id != track_id)
			{
				if (last_track_id != -1)
					fprintf(file, "\t\t</trkseg>\n");
					
				fprintf(file, "\t\t<trkseg>\n");
				
				last_track_id = track_id;
			}
			
			// output a trackpoint
			fprintf(file, "\t\t\t<trkpt lat=\"%s\" lon=\"%s\">\n"
						  "\t\t\t\t<time>%s</time>\n"
						  "\t\t\t\t<course>%s</course>\n"
						  "\t\t\t\t<speed>%s</speed>\n"
						  "\t\t\t\t<climb>%s</climb>\n"
			, row[2], row[3], row[1], row[4], row [5], row[6]);
			
			if (atof(row[7]) == 0)
				fprintf(file, "\t\t\t\t<fix>2d</fix>\n");
			else
				fprintf(file, "\t\t\t\t<fix>3d</fix>\n"
							  "\t\t\t\t<ele>%s</ele>\n",
							row[7]);
			
			fprintf(file, "\t\t\t</trkpt>\n");
		}
	}
	
	mysql_free_result(result);
	
	fprintf(file, "\t\t</trkseg>\n\t</trk>\n</gpx>\n");
	
	if (file != stdout)
		fclose(file);
	
	return EXIT_SUCCESS;
}


/* 
	Dump all of the points in the DB into GPX file(s) with timestamps 
*/
int main(int argc, char ** argv)
{
	int flag_l = 0, flag_r = 0, flag_f = 0, error = 0;
	int t1, t2 = -1, c, i , j, ret = EXIT_SUCCESS;
	
	char *tracks = NULL, *tracks_tmp = NULL, *tok, *itok;
	int *track_list = NULL, track_count = 0;
	
	char * name = NULL, * author = NULL, * desc = NULL, * email = NULL;

	// establish a connection to the MySQL server, or die
	mysql_init(&mysql);

	if (mysql_real_connect(&mysql,SQL_SERVER,SQL_USERNAME,SQL_PASSWORD,SQL_DATABASE,SQL_PORT,NULL,0) != &mysql){
		fprintf(stderr,"%s: Failed to connect to database: %s\n",argv[0], mysql_error(&mysql));
		exit(EXIT_FAILURE);
	}

	while ((c = getopt (argc, argv, "lrt:n:a:d:e:")) != -1)
	{
		switch(c)
		{
			case 'l': flag_l = 1; break;
			case 'r': flag_r = 1; break;
			case 'f': flag_f = 1; break;
			
			case 't': tracks = optarg; 
				break;
			case 'n': name = optarg; 
				break;
			case 'a': author = optarg;
				break;
			case 'd': desc = optarg;
				break;
			case 'e': desc = optarg;
				break;
			case '?':
				if (optopt == 't')
					fprintf(stderr, "%s: Option requires an argument -- t\n", argv[0]);
				else if (optopt == '?')
					printf("Usage: %s -ldf [-t tracklist] [output filename]\n\n"
						   "  -l\tlist tracks\n"
						   "  -d\tdelete tracks (requires -t)\n"
						   "  -f\tforce delete tracks\n\n"
						   "GPX Options\n\n"
						   "  -n\tFile name/title\n"
						   "  -a\tAuthor name\n"
						   "  -d\tDescription\n"
						   "  -e\tEmail address of author\n\n"
						   "Note that when you specify multiple tracks, they are actually written\n"
						   "to the GPX file as track segments, as part of one big track.", 
					argv[0]);
			default:
				error = 1;
				break;
		}
	}	
	
	
	if (!error && flag_l)
		list_tracks();
	else
	{
	
		
		if (!error && process_tracks(tracks, &track_list, &track_count))
		{
			// delete tracks
			if (flag_r)
				ret = delete_tracks(track_list, track_count, flag_f);
			// or output tracks to a file
			else if (argc - optind == 1)
				ret = output_tracks(argv[argc-1], track_list, track_count, name, author, desc, email);
			else
				fprintf(stderr,"Error: no output filename specified\n");
		
		} else 
			return EXIT_FAILURE;
			
	}
	
	if (error)
		ret = EXIT_FAILURE;
	
	if (track_list)
		free(track_list);
	
	mysql_library_end();
	exit(ret);
}

/*
	Process track list -- this is far too much effort for doing what I 
	want to do.
*/
int process_tracks(char * tracks, int ** track_list, int * track_count)
{
	int i, j, t1, t2 = -1;
	char *tracks_tmp = tracks, *tok, *itok;
	
	int * list = NULL, count = 0;
		
	if (!tracks)
		return 1;
		
	// split by comma
	tracks_tmp = tracks;
	while(tok = strtok(tracks_tmp,","))
	{
		tracks_tmp = NULL;
		
		// if there is a delimiter
		if (itok = strstr(tok, "-"))
		{
			// grab the two args
			*itok = '\0';
			errno = 0;
			t1 = strtol(tok, NULL, 10);
			if (errno)
			{
				fprintf(stderr,"Invalid track range number\n");
				return 0;
			}
			
			*itok = '-';
			errno = 0;
			t2 = strtol(itok+1, NULL, 10);
			if (errno)
			{
				fprintf(stderr,"Invalid track range number\n");
				return 0;
			}
			
		} else {
			errno = 0;
			t1 = strtol(tok, NULL, 10);
			if (errno)
			{
				fprintf(stderr,"Invalid track number\n");
				return 0;
			}
		}
		
		if (t2 != -1)
		{
			if (t2 <= t1)
			{
				fprintf(stderr,"Invalid track range %d-%d\n", t1, t2);
				return 0;
			}
			
			list = realloc(list, sizeof(int) * ((t2-t1) + count));
			
			for (i = t1; i <= t2; i++)
				list[count++] = i;
		} 
		else 
		{
			list = realloc(list, sizeof(int) * (++count));
			list[count-1] = t1;
		}
						
		t2 = -1;
	}
	
	// go through and find/remove duplicates -- yes its slow, who cares
	if (list)
		for (i = 0; i < count; i++)
			for (j = i+1; j < count; j++)
				if (list[i] == list[j])
					list[j] = -1;
	
	*track_list = list;
	*track_count = count;
	
	return 1;
}

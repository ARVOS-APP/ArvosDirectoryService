/*
 avLocation.c - location handling for arvos CGI directory service.

 Copyright (C) 2016   Tamiko Thiel and Peter Graf

 This file is part of ARVOS-APP - AR Viewer Open Source.
 ARVOS-APP is free software.

 This program is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program.  If not, see <http://www.gnu.org/licenses/>.

 For more information on the ARVOS-APP, Tamiko Thiel or Peter Graf,
 please see: http://www.arvos-app.com/.

 $Log: avLocation.c,v $
 Revision 1.2  2016/12/03 00:03:54  peter
 Cleanup after tests

 */

/*
 * Make sure "strings <exe> | grep Id | sort -u" shows the source file versions
 */
char * avLocation_c_id = "$Id: avLocation.c,v 1.2 2016/12/03 00:03:54 peter Exp $";

#include <stdio.h>
#include <memory.h>

#ifndef __APPLE__
#include <malloc.h>
#endif

#include <assert.h>
#include <stdlib.h>
#include <unistd.h>

#include "arvos.h"

/*****************************************************************************/
/* Functions                                                                 */
/*****************************************************************************/

/**
 * Try parsing a radius to a value.
 *
 * @return char * message != NULL: An error message.
 */
char * avGetRadius(char * rad, int * value)
{
	if (value)
	{
		*value = -1;
	}

	if (!rad || !*rad)
	{
		return "You need to enter the visibility radius of the channel.";
	}
	char * ptr = strchr(rad, '\t');
	if (ptr)
	{
		ptr = strchr(ptr + 1, '\t');
		if (!ptr)
		{
			return avSprintf("Cannot parse radius from '%s'.", rad);
		}
	}
	else if (strlen(rad) > 5)
	{
		return "The radius given is too long, it is longer than 5 characters.";
	}
	else
	{
		ptr = rad;
	}
	ptr = avTrim(ptr);

	if (!isdigit(*ptr))
	{
		return "The radius must be an integer value between 0 and 10000.";
	}

	errno = 0;
	long radius = strtol(ptr, NULL, 10);
	if (errno || radius < 0 || radius > 10000)
	{
		return "The radius must be an integer value between 0 and 10000.";
	}
	if (value)
	{
		*value = (int) radius;
	}
	return NULL;
}

/**
 * Try parsing an altitude to a value.
 *
 * @return char * message != NULL: An error message.
 */
char * avGetAltitude(char * alt, int * value)
{
	if (value)
	{
		*value = 0;
	}

	if (!alt || !*alt)
	{
		return NULL;
	}
	char * ptr = strchr(alt, '\t');
	if (ptr)
	{
		ptr = strchr(ptr + 1, '\t');
		if (!ptr)
		{
			return avSprintf("Cannot parse altitude from '%s'.", alt);
		}
		ptr = strchr(ptr + 1, '\t');
		if (!ptr)
		{
			return avSprintf("Cannot parse altitude from '%s'.", alt);
		}
	}
	else if (strlen(alt) > 4)
	{
		return "The altitude given is too long, it is longer than 4 characters.";
	}
	else
	{
		ptr = alt;
	}
	ptr = avTrim(ptr);

	if (*ptr != '-' && *ptr != '+' && !isdigit(*ptr))
	{
		return avSprintf("Cannot parse altitude from '%s'.", ptr);
	}

	errno = 0;
	long altitude = strtol(ptr, NULL, 10);
	if (errno || altitude < -100 || altitude > 9000)
	{
		return "The altitude must be an integer value between -100 and 9000.";
	}
	if (value)
	{
		*value = (int) altitude;
	}
	return NULL;
}

/**
 * Try parsing a latitude to a value.
 *
 * @return char * message != NULL: An error message.
 */
char * avGetLatitude(char * lat, double * value)
{
	if (value)
	{
		*value = 0;
	}

	if (!lat || !*lat)
	{
		return "You must enter the latitude.";
	}
	char * ptr = strchr(lat, '\t');
	if (!ptr && strlen(lat) > 10)
	{
		return "The latitude given is too long, it is longer than 10 characters.";
	}

	errno = 0;
	double latitude = strtod(lat, NULL);
	if (ptr && latitude < 0)
	{
		latitude = -100. - latitude;
	}
	if (errno || latitude < -80. || latitude > 80.)
	{
		return "The latitude must be a float value between -80 and 80.";
	}
	if (value)
	{
		*value = latitude;
	}
	return NULL;
}

/**
 * Try parsing a longitude to a value.
 *
 * @return char * message != NULL: An error message.
 */
char * avGetLongitude(char * lon, double * value)
{
	if (value)
	{
		*value = 0;
	}

	if (!lon || !*lon)
	{
		return "You must enter the longitude.";
	}
	char * ptr = strchr(lon, '\t');
	if (!ptr)
	{
		if (strlen(lon) > 11)
		{
			return "The longitude given is too long, it is longer than 11 characters.";
		}
		ptr = lon;
	}

	errno = 0;
	double longitude = strtod(ptr, NULL);
	if (errno || longitude < -180. || longitude > 180.)
	{
		return "The longitude must be a float value between -180 and 180.";
	}
	if (value)
	{
		*value = longitude;
	}
	return NULL;
}

/**
 * Save a location data to the database.
 *
 * @return char * message != NULL: An error message.
 */
char * avLocationSave(char * id, char * channel, char * lat, char * lon, char * rad, char * alt)
{
	double latitude;
	char * message = avGetLatitude(lat, &latitude);
	if (message)
	{
		return message;
	}

	double longitude;
	message = avGetLongitude(lon, &longitude);
	if (message)
	{
		return message;
	}

	int altitude;
	message = avGetAltitude(alt, &altitude);
	if (message)
	{
		return message;
	}

	int radius;
	message = avGetRadius(rad, &radius);
	if (message)
	{
		return message;
	}

	char * filler = "";
	if (latitude < 0.)
	{
		filler = "-";
		latitude = latitude + 100.;
	}

	char * position = avSprintf("%s%09.6f\t%03.6f\t%d\t%s", filler, latitude, longitude, (int) radius, alt);

	if (!id || !*id)
	{
		id = avDbLocationInsert(channel, position);
	}
	else
	{
		PblMap * map = avDbLocationGet(id);
		if (!map)
		{
			return avSprintf("Failed to access location with id '%s', pbl_errno %d.\n", id, pbl_errno);
		}
		avMapFree(map);

		avDbLocationUpdateColumn(AV_KEY_ID, id, AV_KEY_CHANNEL, channel);
		avDbLocationUpdateColumn(AV_KEY_ID, id, AV_KEY_POSITION, position);
	}

	char * updateKeys[] = { AV_KEY_LAT, AV_KEY_LON, AV_KEY_ALTITUDE, AV_KEY_RADIUS, NULL };
	char * updateValues[] = { lat, lon, alt, rad, NULL };
	avDbLocationUpdateValues(AV_KEY_ID, id, updateKeys, updateValues, NULL);

	return NULL;
}


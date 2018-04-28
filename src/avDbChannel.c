/*
 avDbChannel.c - channel related database interface for CGI directory service.

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

 $Log: avDbChannel.c,v $
 Revision 1.4  2018/03/11 00:34:52  peter
 Integration with pbl cgi code.

 Revision 1.3  2016/12/13 21:47:21  peter
 Commit after Win32 port

 Revision 1.2  2016/11/28 19:54:15  peter
 Initial

 */

/*
 * Make sure "strings <exe> | grep Id | sort -u" shows the source file versions
 */
char * avDbChannel_c_id = "$Id: avDbChannel.c,v 1.4 2018/03/11 00:34:52 peter Exp $";

#include <stdio.h>
#include <memory.h>

#ifndef __APPLE__
#include <malloc.h>
#endif

#include <assert.h>
#include <stdlib.h>

#include "arvos.h"

/*****************************************************************************/
/* Functions                                                                 */
/*****************************************************************************/

/**
 * Insert a channel with the given name, author and description.
 *
 * @return char * id: Id of the new channel.
 */
char * avDbChannelInsert(char * name, char * author, char * description, char * developerKey)
{
	char * id = NULL;

	char * sql = sqlite3_mprintf("INSERT INTO channel ( %s, %s, %s, %s, %s, %s ) "
			"VALUES ( NULL, %Q, %Q, %Q, %Q, %Q);"
			"SELECT ID FROM channel WHERE %s = %Q; ",
	AV_KEY_ID,
	AV_KEY_CHANNEL,
	AV_KEY_AUTHOR,
	AV_KEY_DESCRIPTION,
	AV_KEY_DEVELOPER_KEY,
	AV_KEY_VALUES, name, author, description, developerKey, "", AV_KEY_CHANNEL, name);

	avSqlExec(avSqliteDb, sql, avCallbackCellValue, &id);
	if (id == NULL)
	{
		pblCgiExitOnError("SQLite exec of '%s' did not create a new record.\n", sql);
	}
	sqlite3_free(sql);
	return id;
}

/**
 * Get the values of a channel, accessing the db by the key.
 */
PblMap * avDbChannelGetBy(char * key, char * value)
{
	PblMap * map = pblCgiNewMap();

	char * sql = sqlite3_mprintf("SELECT %s, %s, %s, %s, %s, %s FROM channel WHERE %s = %Q; ", AV_KEY_ID,
	AV_KEY_CHANNEL, AV_KEY_AUTHOR, AV_KEY_DESCRIPTION, AV_KEY_DEVELOPER_KEY, AV_KEY_VALUES, key, value);

	avSqlExec(avSqliteDb, sql, avCallbackRowValues, &map);
	sqlite3_free(sql);

	if (pblCgiMapIsEmpty(map))
	{
		pblCgiMapFree(map);
		return NULL;
	}
	return map;
}

/**
 * Get the values of a channel, accessing the db by id.
 */
PblMap * avDbChannelGet(char * id)
{
	return avDbChannelGetBy(AV_KEY_ID, id);
}

/**
 * Get the values of a channel, accessing the db by name.
 */
PblMap * avDbChannelGetByName(char * name)
{
	return avDbChannelGetBy(AV_KEY_CHANNEL, name);
}

static char * avDbChannelColumnNames[] = { AV_KEY_ID, AV_KEY_CHANNEL, AV_KEY_AUTHOR, AV_KEY_DESCRIPTION,
AV_KEY_DEVELOPER_KEY, NULL };

/**
 * Update a column of the record with a given key.
 */
void avDbChannelUpdateColumn(char * key, char * value, char * updateKey, char * updateValue)
{
	char * sql = sqlite3_mprintf("UPDATE channel SET %s = %Q WHERE %s = %Q; ", updateKey, updateValue, key, value);

	avSqlExec(avSqliteDb, sql, NULL, NULL);
	sqlite3_free(sql);
}

/**
 * Looks for the record with the given key, updates the values and returns the value of the returnKey
 */
char * avDbChannelUpdateValues(char * key, char * value, char ** updateKeys, char ** updateValues, char * returnKey)
{
	PblMap * map = pblCgiNewMap();

	char * sql = sqlite3_mprintf("SELECT %s FROM channel WHERE %s = %Q; ", AV_KEY_VALUES, key, value);

	avSqlExec(avSqliteDb, sql, avCallbackRowValues, &map);
	sqlite3_free(sql);

	map = avUpdateData(map, updateKeys, updateValues);
	char * data = avMapToDataStr(map);

	sql = sqlite3_mprintf("UPDATE channel SET %s = %Q WHERE %s = %Q; ", AV_KEY_VALUES, data, key, value);

	avSqlExec(avSqliteDb, sql, NULL, NULL);
	sqlite3_free(sql);
	PBL_FREE(data);

	char * rPtr = NULL;

	if (returnKey)
	{
		int index = pblCgiStrArrayContains(avDbChannelColumnNames, returnKey);
		if (index >= 0)
		{
			sql = sqlite3_mprintf("SELECT %s FROM channel WHERE %s = %Q; ", returnKey, key, value);
			avSqlExec(avSqliteDb, sql, avCallbackCellValue, &rPtr);
			sqlite3_free(sql);
		}
		else
		{
			rPtr = pblCgiStrDup(pblMapGetStr(map, returnKey));
		}
	}
	pblCgiMapFree(map);
	return rPtr;
}

void avDbChannelSetMapForIteration(PblMap * map, int iteration, char * location)
{
	char * author = pblMapGetStr(map, AV_KEY_AUTHOR);
	if (avUserIsAdministrator || (avUserIsAuthor && pblCgiStrEquals(author, avUserIsAuthor)))
	{
		pblCgiSetValueForIteration(AV_KEY_DELETE_ALLOWED, author, iteration);
		pblCgiSetValueForIteration(AV_KEY_EDIT_ALLOWED, author, iteration);
	}
	else
	{
		pblCgiUnSetValueForIteration(AV_KEY_DELETE_ALLOWED, iteration);
		pblCgiUnSetValueForIteration(AV_KEY_EDIT_ALLOWED, iteration);
	}

	if (pblCollectionAggregate(map, &iteration, avMapStrToValues) != 0)
	{
		pblCgiExitOnError("Failed to list aggregate channel values, pbl_errno = %d\n", pbl_errno);
	}

	char * id = pblMapGetStr(map, AV_KEY_ID);
	if (iteration < 0 && !location)
	{
		avDbLocationsListByChannel(pblCgiValueMap(), id);
	}
	else
	{
		PblMap * map;
		if (!location || !*location)
		{
			map = avDbLocationGetByChannel(id);
		}
		else
		{
			map = avDbLocationGet(location);
		}

		if (map)
		{
			pblCgiSetValueForIteration(AV_KEY_LOCATION, pblMapGetStr(map, AV_KEY_LOCATION), iteration);
			pblCgiSetValueForIteration(AV_KEY_LAT, pblMapGetStr(map, AV_KEY_LAT), iteration);
			pblCgiSetValueForIteration(AV_KEY_LON, pblMapGetStr(map, AV_KEY_LON), iteration);
			pblCgiSetValueForIteration(AV_KEY_RADIUS, pblMapGetStr(map, AV_KEY_RADIUS), iteration);
			pblCgiSetValueForIteration(AV_KEY_ALTITUDE, pblMapGetStr(map, AV_KEY_ALTITUDE), iteration);
			pblCgiMapFree(map);
		}
	}
}

/**
 * Delete a channel with the given id.
 */
void avDbChannelDelete(char * id)
{
	char * sql = sqlite3_mprintf("DELETE FROM channel WHERE %s = %Q; ", AV_KEY_ID, id);

	avSqlExec(avSqliteDb, sql, NULL, NULL);
	sqlite3_free(sql);
}

/**
 * Delete a channel with the given author id.
 */
void avDbChannelDeleteByAuthor(char * author)
{
	char * sql = sqlite3_mprintf("DELETE FROM channel WHERE %s = %Q; ", AV_KEY_AUTHOR, author);

	avSqlExec(avSqliteDb, sql, NULL, NULL);
	sqlite3_free(sql);
}

void avDbChannelSetValuesForIteration(char * id, int iteration, char * location)
{
	PblMap * map = avDbChannelGet(id);
	avDbChannelSetMapForIteration(map, iteration, location);
	pblCgiMapFree(map);
}

struct avChannelCallbackFilter
{
	char * authorFilter;
	char * descriptionFilter;
	char * channelFilter;
	double * longitudeFilter;
	double * latitudeFilter;

	char * developerKeyFilter;
	int offset;
	int n;
	int maxLength;
	int nearest;

	PblMap * map;
	PblHeap * list;
};

/**
 * SqLite callback that expects values for columns in multiple rows and sets them to the pointer struct's map
 */
int avCallbackChannelValues(void * callbackPtr, int nColums, char ** values, char ** headers)
{
	if (nColums != 3)
	{
		pblCgiExitOnError("SQLite callback avCallbackChannelValues called with %d columns\n", nColums);
	}
	struct avChannelCallbackFilter * filter = (struct avChannelCallbackFilter *) callbackPtr;
	if (!filter)
	{
		pblCgiExitOnError("SQLite callback avCallbackChannelValues called with no filter\n");
	}

	if (filter->n == 0)
	{
		return 1;
	}

	char * channelAuthor = values[1];
	char * channelDeveloperKey = values[2];
	if (!avUserIsAdministrator && !pblCgiStrIsNullOrWhiteSpace(channelDeveloperKey)
			&& !pblCgiStrEquals(avUserIsAuthor, channelAuthor))
	{
		if (!pblCgiStrEquals(channelDeveloperKey, filter->developerKeyFilter))
		{
			return 0;
		}
	}

	if (filter->offset > 0)
	{
		filter->offset--;
		return 0;
	}
	if (filter->n > 0)
	{
		filter->n--;
	}

	char * key = pblCgiSprintf("%d", pblMapSize(filter->map));
	pblCgiSetValueToMap(key, values[0], -1, filter->map);
	PBL_FREE(key);

	return 0;
}

/**
 * SqLite callback that expects values for columns in multiple rows and sets them to the pointer struct's map
 */
int avCallbackChannelFilteredValues(void * callbackPtr, int nColums, char ** values, char ** headers)
{
	if (nColums != 8)
	{
		pblCgiExitOnError("SQLite callback avCallbackChannelFilteredValues called with %d columns\n", nColums);
	}
	struct avChannelCallbackFilter * filter = (struct avChannelCallbackFilter *) callbackPtr;
	if (!filter)
	{
		pblCgiExitOnError("SQLite callback avCallbackChannelFilteredValues called with no filter\n");
	}

	// "SELECT location.ID as LOC, POS, channel.ID as ID, channel.CHN as CHN, AUT, DES, DEV, channel.VALS as VALS FROM location "
	char * position = values[1];
	char * channelName = values[3];
	char * channelAuthor = values[4];
	char * channelDescription = values[5];
	char * channelDeveloperKey = values[6];

	if (!avUserIsAdministrator && !pblCgiStrIsNullOrWhiteSpace(channelDeveloperKey)
			&& !pblCgiStrEquals(avUserIsAuthor, channelAuthor))
	{
		if (!pblCgiStrEquals(channelDeveloperKey, filter->developerKeyFilter))
		{
			return 0;
		}
	}

	char * ptr;
	double positionDistance = 100000000.;

	if (filter->n == 0 && !filter->nearest)
	{
		return 1;
	}

	double channelLatitude;
	char * message = avGetLatitude(position, &channelLatitude);
	if (message)
	{
		pblCgiSetValue(AV_KEY_REPLY, pblCgiSprintf("Bad latitude in position '%s'. %s", position, message));
		avPrintTemplate(avTemplateDirectory, "index.html", "text/html");
	}

	double channelLongitude;
	message = avGetLongitude(position, &channelLongitude);
	if (message)
	{
		pblCgiSetValue(AV_KEY_REPLY, pblCgiSprintf("Bad longitude in position '%s'. %s", position, message));
		avPrintTemplate(avTemplateDirectory, "index.html", "text/html");
	}

	int channelRadius;
	message = avGetRadius(position, &channelRadius);
	if (message)
	{
		pblCgiSetValue(AV_KEY_REPLY, pblCgiSprintf("Bad radius in position '%s'. %s", position, message));
		avPrintTemplate(avTemplateDirectory, "index.html", "text/html");
	}
	if (channelRadius < 1)
	{
		channelRadius = 1;
	}

	double latDistance = 0;
	if (filter->latitudeFilter)
	{
		double l = *(filter->latitudeFilter) + 90.;
		double cl = channelLatitude + 90.;

		latDistance = l - cl;
		if (latDistance < 0.)
		{
			latDistance *= -1.;
		}

		double maxDistance = 0.1 * (channelRadius / 10000.);
		if (latDistance > maxDistance)
		{
			if (channelLatitude < *(filter->latitudeFilter))
			{
				return 0;
			}
			else
			{
				return 1;
			}
		}
	}

	double lonDistance = 0;
	if (filter->longitudeFilter)
	{
		double l = *(filter->longitudeFilter) + 180.;
		double cl = channelLongitude + 180.;
		lonDistance = l - cl;
		if (lonDistance < 0)
		{
			lonDistance *= -1.;
		}
		if (lonDistance >= 359.9)
		{
			lonDistance -= 360.;
		}
		if (lonDistance < 0.)
		{
			lonDistance *= -1.;
		}

		double maxDistance = 0.1 * (channelRadius / 10000.);
		if (lonDistance > maxDistance)
		{
			return 0;
		}
	}

	double distance = latDistance > lonDistance ? latDistance : lonDistance;
	if (distance * 100000. < positionDistance)
	{
		positionDistance = distance * 100000.;
	}

	if (filter->authorFilter && *(filter->authorFilter))
	{
		char * author = pblCgiStrDup(channelAuthor);
		for (ptr = author; *ptr; ptr++)
		{
			if (isupper(*ptr))
			{
				*ptr = tolower(*ptr);
			}
		}
		if (!strstr(author, filter->authorFilter))
		{
			PBL_FREE(author);
			return 0;
		}
		PBL_FREE(author);
	}

	if (filter->channelFilter && *(filter->channelFilter))
	{
		char * name = pblCgiStrDup(channelName);
		for (ptr = name; *ptr; ptr++)
		{
			if (isupper(*ptr))
			{
				*ptr = tolower(*ptr);
			}
		}
		if (!strstr(name, filter->channelFilter))
		{
			PBL_FREE(name);
			return 0;
		}
		PBL_FREE(name);
	}

	if (filter->descriptionFilter && *(filter->descriptionFilter))
	{
		char * description = pblCgiStrDup(channelDescription);
		for (ptr = description; *ptr; ptr++)
		{
			if (isupper(*ptr))
			{
				*ptr = tolower(*ptr);
			}
		}
		if (!strstr(description, filter->descriptionFilter))
		{
			PBL_FREE(description);
			return 0;
		}
		PBL_FREE(description);
	}

	if (filter->n > 0)
	{
		filter->n--;
	}

	int size = pblHeapSize(filter->list);
	if (!filter->nearest || size < filter->maxLength - 1)
	{
		PblMap * map = pblCgiNewMap();
		avCallbackRowValues(&map, nColums, values, headers);
		ptr = pblCgiSprintf("%ld", (long) positionDistance);
		pblCgiSetValueToMap(AV_KEY_DISTANCE, ptr, -1, map);
		PBL_FREE(ptr);

		if (pblListAdd(filter->list, map) < 1)
		{
			pblCgiExitOnError("Failed to allocate %u bytes, pbl_errno %d, '%s'", sizeof(PblMap *), pbl_errno, pbl_errstr);
		}
	}
	else if (size == filter->maxLength - 1)
	{
		PblMap * map = pblCgiNewMap();
		avCallbackRowValues(&map, nColums, values, headers);
		ptr = pblCgiSprintf("%ld", (long) positionDistance);
		pblCgiSetValueToMap(AV_KEY_DISTANCE, ptr, -1, map);
		PBL_FREE(ptr);

		if (pblListAdd(filter->list, map) < 1)
		{
			pblCgiExitOnError("Failed to allocate %u bytes, pbl_errno %d, '%s'", sizeof(PblMap *), pbl_errno, pbl_errstr);
		}
		pblHeapConstruct(filter->list);
	}
	else
	{
		PblMap * furthest = pblHeapGetFirst(filter->list);
		long furthestDistance = atol(pblMapGetStr(furthest, AV_KEY_DISTANCE));
		if (positionDistance < furthestDistance)
		{
			PblMap * map = pblCgiNewMap();
			avCallbackRowValues(&map, nColums, values, headers);
			ptr = pblCgiSprintf("%ld", (long) positionDistance);
			pblCgiSetValueToMap(AV_KEY_DISTANCE, ptr, -1, map);
			PBL_FREE(ptr);

			pblListSetFirst(filter->list, map);
			pblHeapEnsureConditionFirst(filter->list);
			pblMapFree(furthest);
		}
	}
	return 0;
}

/**
 * List channels by name.
 *
 * The first offset channels are skipped, at most n channels are handled.
 */
int avDbChannelsListByName(int offset, int n)
{
	struct avChannelCallbackFilter filter;
	filter.developerKeyFilter = "";
	filter.map = pblCgiNewMap();
	filter.n = n;
	filter.offset = offset;

	int iteration = 0;

	char * sql = sqlite3_mprintf("SELECT %s, %s, %s FROM channel ORDER BY %s ASC; ",
	AV_KEY_ID, AV_KEY_AUTHOR, AV_KEY_DEVELOPER_KEY, AV_KEY_CHANNEL);
	avSqlExec(avSqliteDb, sql, avCallbackChannelValues, &filter);
	sqlite3_free(sql);

	for (int i = 0; i >= 0; i++)
	{
		char * key = pblCgiSprintf("%d", i);
		char * value = pblMapGetStr(filter.map, key);
		PBL_FREE(key);
		if (!value)
		{
			break;
		}
		avDbChannelSetValuesForIteration(value, iteration++, NULL);
	}

	pblCgiMapFree(filter.map);
	return iteration;
}

/**
 * Author is used for exact matches and the optional filter keys as well.
 *
 * The first offset channels are skipped, at most n channels are handled.
 */
int avDbChannelsListByAuthor(int offset, int n, char * author)
{
	struct avChannelCallbackFilter filter;
	filter.developerKeyFilter = "";
	filter.map = pblCgiNewMap();
	filter.n = n;
	filter.offset = offset;

	int iteration = 0;

	char * sql;

	if (author && *author)
	{
		sql = sqlite3_mprintf("SELECT %s, %s, %s FROM channel WHERE %s = %Q ORDER BY %s ASC; ",
		AV_KEY_ID, AV_KEY_AUTHOR, AV_KEY_DEVELOPER_KEY, AV_KEY_AUTHOR, author, AV_KEY_CHANNEL);
	}
	else
	{
		sql = sqlite3_mprintf("SELECT %s, %s, %s FROM channel ORDER BY %s ASC; ",
		AV_KEY_ID, AV_KEY_AUTHOR, AV_KEY_DEVELOPER_KEY, AV_KEY_AUTHOR);
	}
	avSqlExec(avSqliteDb, sql, avCallbackChannelValues, &filter);
	sqlite3_free(sql);

	for (int i = 0; i >= 0; i++)
	{
		char * key = pblCgiSprintf("%d", i);
		char * value = pblMapGetStr(filter.map, key);
		PBL_FREE(key);
		if (!value)
		{
			break;
		}
		avDbChannelSetValuesForIteration(value, iteration++, NULL);
	}

	pblCgiMapFree(filter.map);
	return iteration;
}

int avDbChannelMapCompareFunction(const void * left, /* The left value for comparison  */
const void * right /* The right value for comparison */
)
{
	PblMap * leftPointer = *(PblMap**) left;
	PblMap * rightPointer = *(PblMap**) right;

	long leftValue = atol(pblMapGetStr(leftPointer, AV_KEY_DISTANCE));
	long rightValue = atol(pblMapGetStr(rightPointer, AV_KEY_DISTANCE));
	return leftValue - rightValue;
}

/**
 * Lat and Lon are used for radius matches if given, author filter, channel filter and description filter match if contained.
 * If a developer key filter is given it has to be equal.
 *
 * At most n channels are returned as a list of maps containing the values.
 */
PblList * avDbChannelsToListByLocation(int n, char * lat, char * lon, char * authorFilter, char * channelFilter,
		char * descriptionFilter, char * developerKeyFilter, int nearest)
{
	PblHeap * list = pblHeapNew();
	if (!list)
	{
		pblCgiExitOnError("Failed to allocate %u bytes, pbl_errno %d, '%s'", sizeof(PblHeap), pbl_errno, pbl_errstr);
	}
	pblHeapSetCompareFunction(list, avDbChannelMapCompareFunction);

	char * ptr;

	if (authorFilter && *authorFilter)
	{
		authorFilter = pblCgiStrDup(authorFilter);
		for (ptr = authorFilter; *ptr; ptr++)
		{
			*ptr = tolower(*ptr);
		}
	}

	if (channelFilter && *channelFilter)
	{
		channelFilter = pblCgiStrDup(channelFilter);
		for (ptr = channelFilter; *ptr; ptr++)
		{
			*ptr = tolower(*ptr);
		}
	}

	if (descriptionFilter && *descriptionFilter)
	{
		descriptionFilter = pblCgiStrDup(descriptionFilter);
		for (ptr = descriptionFilter; *ptr; ptr++)
		{
			*ptr = tolower(*ptr);
		}
	}

	double longitude = 0;
	if (lon && *lon)
	{
		char * message = avGetLongitude(lon, &longitude);
		if (message)
		{
			pblCgiSetValue(AV_KEY_REPLY, message);
			avPrintTemplate(avTemplateDirectory, "index.html", "text/html");
		}
	}

	double latitude = 0;
	if (lat && *lat)
	{
		char * message = avGetLatitude(lat, &latitude);
		if (message)
		{
			pblCgiSetValue(AV_KEY_REPLY, message);
			avPrintTemplate(avTemplateDirectory, "index.html", "text/html");
		}
	}

	char * sql;

	if (lat && *lat)
	{
		char * filler = "";
		double minLatitude = latitude - 0.1;
		if (minLatitude < 0.)
		{
			filler = "-";
			minLatitude = minLatitude + 100.;
		}

		char * searchMinLatitude = pblCgiSprintf("%s%09.6f\t", filler, minLatitude);

		filler = "";
		double maxLatitude = latitude + 0.1;
		if (maxLatitude < 0.)
		{
			filler = "-";
			maxLatitude = maxLatitude + 100.;
		}

		char * searchMaxLatitude = pblCgiSprintf("%s%09.6f\t", filler, maxLatitude);

		sql =
				sqlite3_mprintf(
						"SELECT location.ID as LOC, POS, channel.ID as ID, channel.CHN as CHN, AUT, DES, DEV, channel.VALS as VALS FROM location "
								"INNER JOIN channel ON location.CHN = channel.ID "
								"WHERE POS > %Q AND POS < %Q ORDER BY POS ASC; ", searchMinLatitude,
						searchMaxLatitude);

		PBL_FREE(searchMinLatitude);
		PBL_FREE(searchMaxLatitude);
	}
	else
	{
		sql =
				sqlite3_mprintf(
						"SELECT location.ID as LOC, POS, channel.ID as ID, channel.CHN as CHN, AUT, DES, DEV, channel.VALS as VALS FROM location "
								"INNER JOIN channel ON location.CHN = channel.ID "
								"ORDER BY POS ASC; ");
	}

	struct avChannelCallbackFilter filter;

	filter.developerKeyFilter = developerKeyFilter;
	filter.map = pblCgiNewMap();
	filter.list = list;
	filter.n = n;
	filter.maxLength = n;
	filter.nearest = nearest;

	filter.authorFilter = authorFilter;
	filter.descriptionFilter = descriptionFilter;
	filter.channelFilter = channelFilter;
	filter.longitudeFilter = (lon && *lon) ? &longitude : NULL;
	filter.latitudeFilter = (lat && *lat) ? &latitude : NULL;

	avSqlExec(avSqliteDb, sql, avCallbackChannelFilteredValues, &filter);
	sqlite3_free(sql);

	return list;
}

/**
 * Lat and Lon are used for radius matches if given, author filter, channel filter and description filter match if contained.
 *
 * The first offset channels are skipped, at most n channels are handled.
 */
int avDbChannelsListByLocation(int offset, int n, char * lat, char * lon, char * authorFilter, char * channelFilter,
		char * descriptionFilter, char * developerKeyFilter)
{
	PblList * locationList;

	if ((lat && *lat) || (lon && *lon))
	{
		locationList = avDbChannelsToListByLocation(offset + n, lat, lon, authorFilter, channelFilter,
				descriptionFilter, developerKeyFilter, 1);
		pblListSort(locationList, avDbChannelMapCompareFunction);
	}
	else
	{
		locationList = avDbChannelsToListByLocation(offset + n, lat, lon, authorFilter, channelFilter,
				descriptionFilter, developerKeyFilter, 0);
	}

	int iteration = 0;
	for (; iteration < n && iteration + offset < pblListSize(locationList); iteration++)
	{
		PblMap * map = pblListGet(locationList, iteration + offset);
		avDbChannelSetMapForIteration(map, iteration, pblMapGetStr(map, AV_KEY_LOCATION));
	}

	while (!pblListIsEmpty(locationList))
	{
		pblMapFree(pblListPop(locationList));
	}
	pblListFree(locationList);

	return iteration;
}


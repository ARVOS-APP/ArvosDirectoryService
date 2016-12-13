/*
 avDbLocation.c - location related database interface for CGI directory service.

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

 $Log: avDbLocation.c,v $
 Revision 1.2  2016/11/28 19:54:16  peter
 Initial

 */

/*
 * Make sure "strings <exe> | grep Id | sort -u" shows the source file versions
 */
char * avDbLocation_c_id = "$Id: avDbLocation.c,v 1.2 2016/11/28 19:54:16 peter Exp $";

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
 * Insert a location with the given channel and position.
 *
 * @return char * id: Id of the new location.
 */
char * avDbLocationInsert(char * channel, char * position)
{
	char * id = NULL;

	char * sql = sqlite3_mprintf("INSERT INTO location ( %s, %s, %s, %s ) "
			"VALUES ( NULL, %Q, %Q, %Q); "
			"SELECT last_insert_rowid() as ID; ",
	AV_KEY_ID,
	AV_KEY_CHANNEL,
	AV_KEY_POSITION,
	AV_KEY_VALUES, channel, position, "");

	avSqlExec(sql, avCallbackCellValue, &id);
	if (id == NULL)
	{
		avExitOnError("SQLite exec of '%s' did not create a new record.\n", sql);
	}
	sqlite3_free(sql);
	return id;
}

static char * avDbLocationColumnNames[] = { AV_KEY_ID, AV_KEY_CHANNEL, AV_KEY_POSITION, NULL };

/**
 * Update a column of the record with a given key.
 */
void avDbLocationUpdateColumn(char * key, char * value, char * updateKey, char * updateValue)
{
	char * sql = sqlite3_mprintf("UPDATE location SET %s = %Q WHERE %s = %Q; ", updateKey, updateValue, key, value);

	avSqlExec(sql, NULL, NULL);
	sqlite3_free(sql);
}

/**
 * Looks for the record with the given key, updates the values and returns the value of the returnKey
 */
char * avDbLocationUpdateValues(char * key, char * value, char ** updateKeys, char ** updateValues, char * returnKey)
{
	PblMap * map = avNewMap();

	char * sql = sqlite3_mprintf("SELECT %s FROM location WHERE %s = %Q; ", AV_KEY_VALUES, key, value);

	avSqlExec(sql, avCallbackRowValues, &map);
	sqlite3_free(sql);

	map = avUpdateData(map, updateKeys, updateValues);
	char * data = avMapToDataStr(map);

	sql = sqlite3_mprintf("UPDATE location SET %s = %Q WHERE %s = %Q; ", AV_KEY_VALUES, data, key, value);

	avSqlExec(sql, NULL, NULL);
	sqlite3_free(sql);
	PBL_FREE(data);

	char * rPtr = NULL;

	if (returnKey)
	{
		int index = avStrArrayContains(avDbLocationColumnNames, returnKey);
		if (index >= 0)
		{
			sql = sqlite3_mprintf("SELECT %s FROM location WHERE %s = %Q; ", returnKey, key, value);
			avSqlExec(sql, avCallbackCellValue, &rPtr);
			sqlite3_free(sql);
		}
		else
		{
			rPtr = avStrDup(pblMapGetStr(map, returnKey));
		}
	}
	avMapFree(map);
	return rPtr;
}

/**
 * Get the values of a location, accessing the db by id.
 */
PblMap * avDbLocationGet(char * id)
{
	PblMap * map = avNewMap();

	char * sql = sqlite3_mprintf("SELECT %s, %s AS %s, %s, %s AS %s, %s, %s FROM location WHERE %s = %Q; ",
	AV_KEY_ID, AV_KEY_ID, AV_KEY_LOCATION, AV_KEY_CHANNEL, AV_KEY_CHANNEL, AV_KEY_LOCATION_CHANNEL,
	AV_KEY_POSITION, AV_KEY_VALUES, AV_KEY_ID, id);

	avSqlExec(sql, avCallbackRowValues, &map);
	sqlite3_free(sql);

	if (avMapIsEmpty(map))
	{
		avMapFree(map);
		return NULL;
	}
	return map;
}

/**
 * Get the values of a location, accessing the db by channel.
 */
PblMap * avDbLocationGetByChannel(char * channel)
{
	PblMap * map = avNewMap();

	char * sql = sqlite3_mprintf("SELECT %s, %s, %s, %s FROM location WHERE %s = %Q; ", AV_KEY_ID, AV_KEY_CHANNEL,
	AV_KEY_POSITION, AV_KEY_VALUES, AV_KEY_CHANNEL, channel);

	avSqlExec(sql, avCallbackRowValues, &map);
	sqlite3_free(sql);

	if (avMapIsEmpty(map))
	{
		avMapFree(map);
		return NULL;
	}
	return map;
}

void avDbLocationSetValuesToMap(char * id, int iteration, PblMap * map)
{
	PblMap * dataMap = avDbLocationGet(id);

	int hasNext;
	void * element;
	PblMapEntry * entry;

	PblIterator iterator;
	pblIteratorInit(dataMap, &iterator);

	while ((hasNext = pblIteratorHasNext(&iterator)) > 0)
	{
		element = pblIteratorNext(&iterator);
		if (element != (void*) -1)
		{
			entry = (PblMapEntry *) element;
			if (!entry)
			{
				continue;
			}
			if (entry->keyLength < 1)
			{
				continue;
			}

			if (entry->valueLength > 1)
			{
				avSetValueToMap(pblMapEntryKey(entry), pblMapEntryValue(entry), iteration, map);
			}
			else
			{
				avUnSetValueFromMap(pblMapEntryKey(entry), iteration, map);
			}
		}
	}
	avMapFree(dataMap);
}

/**
 * Delete a location with the given id.
 */
void avDbLocationDelete(char * id)
{
	char * sql = sqlite3_mprintf("DELETE FROM location WHERE %s = %Q; ", AV_KEY_ID, id);

	avSqlExec(sql, NULL, NULL);
	sqlite3_free(sql);
}

/**
 * Delete a location with the given channel id.
 */
void avDbLocationDeleteByChannel(char * channel)
{
	char * sql = sqlite3_mprintf("DELETE FROM location WHERE %s = %Q; ", AV_KEY_CHANNEL, channel);

	avSqlExec(sql, NULL, NULL);
	sqlite3_free(sql);
}

/**
 * Channel is used for exact matches.
 */
int avDbLocationsListByChannel(PblMap * targetMap, char * channel)
{
	int iteration = 0;
	PblMap * map = avNewMap();

	char * sql = sqlite3_mprintf("SELECT %s FROM location WHERE %s = %Q; ", AV_KEY_ID, AV_KEY_CHANNEL, channel);
	avSqlExec(sql, avCallbackColumnValues, &map);
	sqlite3_free(sql);

	for (int i = 0; i >= 0; i++)
	{
		char * key = avSprintf("%d", i);
		char * value = pblMapGetStr(map, key);
		PBL_FREE(key);
		if (!value)
		{
			break;
		}
		avDbLocationSetValuesToMap(value, iteration++, targetMap);
	}

	avMapFree(map);
	return iteration;
}

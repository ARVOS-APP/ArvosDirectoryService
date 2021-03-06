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
 Revision 1.5  2018/04/29 18:42:08  peter
 More work on service

 Revision 1.4  2018/03/11 00:34:52  peter
 Integration with pbl cgi code.

 Revision 1.3  2016/12/13 21:47:21  peter
 Commit after Win32 port

 Revision 1.2  2016/11/28 19:54:16  peter
 Initial

 */

/*
 * Make sure "strings <exe> | grep Id | sort -u" shows the source file versions
 */
char * avDbLocation_c_id = "$Id: avDbLocation.c,v 1.5 2018/04/29 18:42:08 peter Exp $";

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

	avSqlExec(avSqliteDb, sql, avCallbackCellValue, &id);
	if (id == NULL)
	{
		pblCgiExitOnError("SQLite exec of '%s' did not create a new record.\n", sql);
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

	avSqlExec(avSqliteDb, sql, NULL, NULL);
	sqlite3_free(sql);
}

/**
 * Looks for the record with the given key, updates the values and returns the value of the returnKey
 */
char * avDbLocationUpdateValues(char * key, char * value, char ** updateKeys, char ** updateValues, char * returnKey)
{
	PblMap * map = pblCgiNewMap();

	char * sql = sqlite3_mprintf("SELECT %s FROM location WHERE %s = %Q; ", AV_KEY_VALUES, key, value);

	avSqlExec(avSqliteDb, sql, avCallbackRowValues, &map);
	sqlite3_free(sql);

	map = avUpdateData(map, updateKeys, updateValues);
	char * data = avMapToDataStr(map);

	sql = sqlite3_mprintf("UPDATE location SET %s = %Q WHERE %s = %Q; ", AV_KEY_VALUES, data, key, value);

	avSqlExec(avSqliteDb, sql, NULL, NULL);
	sqlite3_free(sql);
	PBL_FREE(data);

	char * rPtr = NULL;

	if (returnKey)
	{
		int index = pblCgiStrArrayContains(avDbLocationColumnNames, returnKey);
		if (index >= 0)
		{
			sql = sqlite3_mprintf("SELECT %s FROM location WHERE %s = %Q; ", returnKey, key, value);
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

/**
 * Get the values of a location, accessing the db by id.
 */
PblMap * avDbLocationGet(char * id)
{
	PblMap * map = pblCgiNewMap();

	char * sql = sqlite3_mprintf("SELECT %s, %s AS %s, %s, %s AS %s, %s, %s FROM location WHERE %s = %Q; ",
	AV_KEY_ID, AV_KEY_ID, AV_KEY_LOCATION, AV_KEY_CHANNEL, AV_KEY_CHANNEL, AV_KEY_LOCATION_CHANNEL,
	AV_KEY_POSITION, AV_KEY_VALUES, AV_KEY_ID, id);

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
 * Get the values of a location, accessing the db by channel.
 */
PblMap * avDbLocationGetByChannel(char * channel)
{
	PblMap * map = pblCgiNewMap();

	char * sql = sqlite3_mprintf("SELECT %s, %s, %s, %s FROM location WHERE %s = %Q; ", AV_KEY_ID, AV_KEY_CHANNEL,
	AV_KEY_POSITION, AV_KEY_VALUES, AV_KEY_CHANNEL, channel);

	avSqlExec(avSqliteDb, sql, avCallbackRowValues, &map);
	sqlite3_free(sql);

	if (pblCgiMapIsEmpty(map))
	{
		pblCgiMapFree(map);
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
				pblCgiSetValueToMap(pblMapEntryKey(entry), pblMapEntryValue(entry), iteration, map);
			}
			else
			{
				pblCgiUnSetValueFromMap(pblMapEntryKey(entry), iteration, map);
			}
		}
	}
	pblCgiMapFree(dataMap);
}

/**
 * Delete a location with the given id.
 */
void avDbLocationDelete(char * id)
{
	char * sql = sqlite3_mprintf("DELETE FROM location WHERE %s = %Q; ", AV_KEY_ID, id);

	avSqlExec(avSqliteDb, sql, NULL, NULL);
	sqlite3_free(sql);
}

/**
 * Delete a location with the given channel id.
 */
void avDbLocationDeleteByChannel(char * channel)
{
	char * sql = sqlite3_mprintf("DELETE FROM location WHERE %s = %Q; ", AV_KEY_CHANNEL, channel);

	avSqlExec(avSqliteDb, sql, NULL, NULL);
	sqlite3_free(sql);
}

/**
 * Channel is used for exact matches.
 */
int avDbLocationsListByChannel(PblMap * targetMap, char * channel)
{
	int iteration = 0;
	PblMap * map = pblCgiNewMap();

	char * sql = sqlite3_mprintf("SELECT %s FROM location WHERE %s = %Q; ", AV_KEY_ID, AV_KEY_CHANNEL, channel);
	avSqlExec(avSqliteDb, sql, avCallbackColumnValues, &map);
	sqlite3_free(sql);

	for (int i = 0; i >= 0; i++)
	{
		char * key = pblCgiSprintf("%d", i);
		char * value = pblMapGetStr(map, key);
		PBL_FREE(key);
		if (!value)
		{
			break;
		}
		avDbLocationSetValuesToMap(value, iteration++, targetMap);
	}

	pblCgiMapFree(map);
	return iteration;
}

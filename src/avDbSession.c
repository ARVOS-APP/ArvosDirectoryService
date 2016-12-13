/*
 avDbSession.c - session related database interface for CGI directory service.

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

 $Log: avDbSession.c,v $
 Revision 1.2  2016/12/13 21:47:21  peter
 Commit after Win32 port

 Revision 1.1  2016/11/28 19:51:37  peter
 Initial

 */

/*
 * Make sure "strings <exe> | grep Id | sort -u" shows the source file versions
 */
char * avDbSession_c_id = "$Id: avDbSession.c,v 1.2 2016/12/13 21:47:21 peter Exp $";

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
 * Insert a session with the given author id, name and email and activation time.
 *
 * @return char * id: Id of the new session. The cookie is set to cookiePtr.
 */
char * avDbSessionInsert(char * authorId, char * name, char * email, char * timeActivated, char ** cookiePtr)
{
	char * expirationTime = avTimeToStr(time(NULL) - (60 * 60));
	unsigned char bufferForRandomBytes[32];
	char * cookie = avBufferToHexString(avRandomBytes(bufferForRandomBytes, 32), 32);
	PblMap * map = avNewMap();

	char * sql = sqlite3_mprintf("SELECT %s FROM session WHERE %s < %Q; ", AV_KEY_ID, AV_KEY_TIME_LAST_ACCESS,
			expirationTime);
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
		avDbSessionDelete(value);
	}
	avMapFree(map);

	for (;;)
	{
		int count = 0;
		char * sql = sqlite3_mprintf("SELECT %s FROM session WHERE %s = %Q; ", AV_KEY_COOKIE, AV_KEY_COOKIE, cookie);
		avSqlExec(sql, avCallbackCounter, &count);
		sqlite3_free(sql);

		if (count > 0)
		{
			PBL_FREE(cookie);
			cookie = avBufferToHexString(avRandomBytes(bufferForRandomBytes, 32), 32);
			continue;
		}
		break;
	}

	char * nowTimeStr = avNowStr();
	char * keyValues[] = { AV_KEY_TIME_CREATED, AV_KEY_NAME, AV_KEY_EMAIL, AV_KEY_TIME_ACTIVATED, NULL };
	char * dataValues[] = { nowTimeStr, name, email, timeActivated, NULL };
	map = avUpdateData(NULL, keyValues, dataValues);
	char * dataStr = avMapToDataStr(map);
	avMapFree(map);

	sql = sqlite3_mprintf("INSERT INTO session ( %s, %s, %s, %s, %s ) "
			"VALUES ( NULL, %Q, %Q, %Q, %Q );"
			"SELECT ID FROM session WHERE %s = %Q; ",
	AV_KEY_ID,
	AV_KEY_COOKIE,
	AV_KEY_TIME_LAST_ACCESS,
	AV_KEY_AUTHOR,
	AV_KEY_VALUES, cookie, nowTimeStr, authorId, dataStr, AV_KEY_COOKIE, cookie);

	char * id = NULL;
	avSqlExec(sql, avCallbackCellValue, &id);
	if (id == NULL)
	{
		avExitOnError("SQLite exec of '%s' did not create a new record.\n", sql);
	}
	sqlite3_free(sql);
	PBL_FREE(dataStr);
	PBL_FREE(nowTimeStr);
	if (cookiePtr)
	{
		*cookiePtr = cookie;
	}
	else
	{
		PBL_FREE(cookie);
	}
	return id;
}

/**
 * Delete a session with the given id.
 */
void avDbSessionDelete(char * id)
{
	char * sql = sqlite3_mprintf("DELETE FROM session WHERE %s = %Q; ", AV_KEY_ID, id);

	avSqlExec(sql, NULL, NULL);
	sqlite3_free(sql);
}

/**
 * Delete a session with the given author id.
 */
void avDbSessionDeleteByAuthor(char * authorId)
{
	char * sql = sqlite3_mprintf("DELETE FROM session WHERE %s = %Q; ", AV_KEY_AUTHOR, authorId);
	avSqlExec(sql, NULL, NULL);
	sqlite3_free(sql);
}

/**
 * Delete a session with the given cookie.
 */
void avDbSessionDeleteByCookie(char * cookie)
{
	char * sql = sqlite3_mprintf("DELETE FROM session WHERE %s = %Q; ", AV_KEY_COOKIE, cookie);
	avSqlExec(sql, NULL, NULL);
	sqlite3_free(sql);
}

/**
 * Get the values of a session, accessing the db by the key.
 */
PblMap * avDbSessionGetBy(char * key, char * value)
{
	PblMap * map = avNewMap();

	char * sql = sqlite3_mprintf("SELECT %s, %s, %s, %s, %s FROM session WHERE %s = %Q; ", AV_KEY_ID, AV_KEY_COOKIE,
	AV_KEY_TIME_LAST_ACCESS, AV_KEY_AUTHOR, AV_KEY_VALUES, key, value);

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
 * Get the values of a session, accessing the db by id.
 */
PblMap * avDbSessionGet(char * id)
{
	return avDbSessionGetBy(AV_KEY_ID, id);
}

/**
 * Get the values of a session, accessing the db by cookie.
 */
PblMap * avDbSessionGetByCookie(char * cookie)
{
	return avDbSessionGetBy(AV_KEY_COOKIE, cookie);
}

static char * avDbSessionColumnNames[] = { AV_KEY_ID, AV_KEY_COOKIE, AV_KEY_TIME_LAST_ACCESS, AV_KEY_AUTHOR, NULL };

/**
 * Update a column of the record with a given key.
 */
void avDbSessionUpdateColumn(char * key, char * value, char * updateKey, char * updateValue)
{
	char * sql = sqlite3_mprintf("UPDATE session SET %s = %Q WHERE %s = %Q; ", updateKey, updateValue, key, value);

	avSqlExec(sql, NULL, NULL);
	sqlite3_free(sql);
}

/**
 * Looks for the record with the given key, updates the values and returns the value of the returnKey
 */
char * avDbSessionUpdateValues(char * key, char * value, char ** updateKeys, char ** updateValues, char * returnKey)
{
	PblMap * map = avNewMap();

	char * sql = sqlite3_mprintf("SELECT %s FROM session WHERE %s = %Q; ", AV_KEY_VALUES, key, value);

	avSqlExec(sql, avCallbackRowValues, &map);
	sqlite3_free(sql);

	map = avUpdateData(map, updateKeys, updateValues);
	char * data = avMapToDataStr(map);

	sql = sqlite3_mprintf("UPDATE session SET %s = %Q WHERE %s = %Q; ", AV_KEY_VALUES, data, key, value);

	avSqlExec(sql, NULL, NULL);
	sqlite3_free(sql);
	PBL_FREE(data);

	char * rPtr = NULL;

	if (returnKey)
	{
		int index = avStrArrayContains(avDbSessionColumnNames, returnKey);
		if (index >= 0)
		{
			sql = sqlite3_mprintf("SELECT %s FROM session WHERE %s = %Q; ", returnKey, key, value);
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

static void avDbSessionSetValuesForIteration(char * id, int iteration)
{
	PblMap * map = avDbSessionGet(id);
	if (pblCollectionAggregate(map, &iteration, avMapStrToValues) != 0)
	{
		avExitOnError("Failed to aggregate session values, pbl_errno = %d\n", pbl_errno);
	}
	avMapFree(map);
}

/**
 * The first offset sessions are skipped, at most n sessions are handled.
 */
int avDbSessionsList(int offset, int n)
{
	int iteration = 0;
	PblMap * map = avNewMap();

	char * sql = sqlite3_mprintf("SELECT %s FROM session ORDER BY %s ASC; ", AV_KEY_ID, AV_KEY_TIME_LAST_ACCESS);
	avSqlExec(sql, avCallbackColumnValues, &map);
	sqlite3_free(sql);

	for (int i = 0; i >= 0; i++)
	{
		if (n == 0)
		{
			break;
		}
		char * key = avSprintf("%d", i);
		char * value = pblMapGetStr(map, key);
		PBL_FREE(key);
		if (!value)
		{
			break;
		}
		if (offset > 0)
		{
			offset--;
			continue;
		}
		if (n > 0)
		{
			n--;
		}
		avDbSessionSetValuesForIteration(value, iteration++);
	}

	avMapFree(map);
	return iteration;
}


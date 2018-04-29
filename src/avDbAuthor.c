/*
 avDbAuthor.c - author related database interface for CGI directory service.

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

 $Log: avDbAuthor.c,v $
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
char * avDbAuthor_c_id = "$Id: avDbAuthor.c,v 1.5 2018/04/29 18:42:08 peter Exp $";

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
 * Insert an author with the given name, email and password.
 */
char * avDbAuthorInsert(char * name, char * email, char * password)
{
	char * id = NULL;
	char * keyValues[] = { AV_KEY_COUNT, AV_KEY_TIME_CREATED, AV_KEY_PASSWORD, NULL };
	char * dataValues[] = { "0", avNowStr(), avHashPassword(password), NULL };

	PblMap * map = avUpdateData(NULL, keyValues, dataValues);
	char * dataStr = avMapToDataStr(map);

	char * sql = sqlite3_mprintf("INSERT INTO author ( %s, %s, %s, %s, %s ) "
			"VALUES ( NULL, %Q, %Q, %Q, %Q );"
			"SELECT last_insert_rowid() as ID; ",

	AV_KEY_ID,
	AV_KEY_NAME,
	AV_KEY_EMAIL,
	AV_KEY_TIME_ACTIVATED,
	AV_KEY_VALUES, name, email, AV_NOT_ACTIVATED, dataStr);

	avSqlExec(avSqliteDb, sql, avCallbackCellValue, &id);
	if (id == NULL)
	{
		pblCgiExitOnError("SQLite exec of '%s' did not create a new record.\n", sql);
	}
	sqlite3_free(sql);
	PBL_FREE(dataStr);
	return id;
}

/**
 * Delete an author with the given id and name.
 */
char * avDbAuthorDelete(char * id, char * name)
{
	char * reply = NULL;
	char * authorId = NULL;

	if (!avUserIsAdministrator && !pblCgiStrEquals(avUserId, id))
	{
		pblCgiExitOnError("You do not have permission to delete user '%s'\n", name);
	}

	char * sql = sqlite3_mprintf("DELETE FROM author WHERE %s = %Q; ", AV_KEY_ID, id);

	avSqlExec(avSqliteDb, sql, NULL, NULL);
	sqlite3_free(sql);

	authorId = pblCgiStrDup(id);

	if (authorId && *authorId)
	{
		avDbChannelDeleteByAuthor(authorId);
		reply = avSessionDeleteByAuthor(authorId);
	}
	PBL_FREE(authorId);

	return reply;
}

/**
 * Get the values of an author, accessing the db by the key.
 */
static PblMap * avDbAuthorGetBy(char * key, char * value)
{
	PblMap * map = pblCgiNewMap();

	char * sql = sqlite3_mprintf("SELECT %s, %s, %s, %s, %s FROM author WHERE %s = %Q; ",
	AV_KEY_ID, AV_KEY_NAME, AV_KEY_EMAIL, AV_KEY_TIME_ACTIVATED, AV_KEY_VALUES, key, value);

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
 * Get the values of an author, accessing the db by id.
 */
PblMap * avDbAuthorGet(char * id)
{
	return avDbAuthorGetBy(AV_KEY_ID, id);
}

/**
 * Get the values of an author, accessing the db by name.
 */
PblMap * avDbAuthorGetByName(char * name)
{
	return avDbAuthorGetBy(AV_KEY_NAME, name);
}

static char * avDbAuthorColumnNames[] = { AV_KEY_ID, AV_KEY_NAME, AV_KEY_EMAIL, AV_KEY_TIME_ACTIVATED, NULL };

/**
 * Update a column of the record with a given key.
 */
void avDbAuthorUpdateColumn(char * key, char * value, char * updateKey, char * updateValue)
{
	char * sql = sqlite3_mprintf("UPDATE author SET %s = %Q WHERE %s = %Q; ", updateKey, updateValue, key, value);

	avSqlExec(avSqliteDb, sql, NULL, NULL);
	sqlite3_free(sql);
}

/**
 * Looks for the record with the given key, updates the values and returns the value of the returnKey
 */
char * avDbAuthorUpdateValues(char * key, char * value, char ** updateKeys, char ** updateValues, char * returnKey)
{
	PblMap * map = pblCgiNewMap();

	char * sql = sqlite3_mprintf("SELECT %s FROM author WHERE %s = %Q; ", AV_KEY_VALUES, key, value);

	avSqlExec(avSqliteDb, sql, avCallbackRowValues, &map);
	sqlite3_free(sql);

	map = avUpdateData(map, updateKeys, updateValues);
	char * data = avMapToDataStr(map);

	sql = sqlite3_mprintf("UPDATE author SET %s = %Q WHERE %s = %Q; ", AV_KEY_VALUES, data, key, value);

	avSqlExec(avSqliteDb, sql, NULL, NULL);
	sqlite3_free(sql);
	PBL_FREE(data);

	char * rPtr = NULL;

	if (returnKey)
	{
		int index = pblCgiStrArrayContains(avDbAuthorColumnNames, returnKey);
		if (index >= 0)
		{
			sql = sqlite3_mprintf("SELECT %s FROM author WHERE %s = %Q; ", returnKey, key, value);
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

static void avDbAuthorSetValuesForIteration(char * id, int iteration)
{
	PblMap * map = avDbAuthorGet(id);

	char * name = pblMapGetStr(map, AV_KEY_NAME);
	if (avUserIsAdministrator || pblCgiStrEquals(name, avUserIsLoggedIn))
	{
		pblCgiSetValueForIteration(AV_KEY_DELETE_ALLOWED, name, iteration);
	}
	else
	{
		pblCgiUnSetValueForIteration(AV_KEY_DELETE_ALLOWED, iteration);
	}

	if (pblCollectionAggregate(map, &iteration, avMapStrToValues) != 0)
	{
		pblCgiExitOnError("Failed to aggregate author values, pbl_errno = %d\n", pbl_errno);
	}
	pblCgiMapFree(map);

	pblCgiSetValueForIteration(AV_KEY_PASSWORD, "-", iteration);
}

struct avAuthorCallbackFilter
{
	char * authorFilter;
	char * emailFilter;
	int offset;
	int n;

	PblMap * map;
};

/**
 * SqLite callback that expects values for columns in multiple rows and sets them to the pointer struct
 */
int avCallbackAuthorFilteredValues(void * callbackPtr, int nColums, char ** values, char ** headers)
{
	char * ptr;

	if (nColums != 3)
	{
		pblCgiExitOnError("SQLite callback avCallbackAuthorFilteredValues called with %d columns\n", nColums);
	}
	struct avAuthorCallbackFilter * filter = (struct avAuthorCallbackFilter *) callbackPtr;
	if (!filter)
	{
		pblCgiExitOnError("SQLite callback avCallbackAuthorFilteredValues called with no filter\n");
	}

	if (filter->n == 0)
	{
		return 1;
	}

	if (filter->authorFilter && *(filter->authorFilter))
	{
		char * author = pblCgiStrDup(values[1]);
		for (ptr = author; *ptr; ptr++)
		{
			*ptr = tolower(*ptr);
		}
		if (!strstr(author, filter->authorFilter))
		{
			PBL_FREE(author);
			return 0;
		}
		PBL_FREE(author);
	}

	if (filter->emailFilter && *(filter->emailFilter))
	{
		char * email = pblCgiStrDup(values[2]);
		for (ptr = email; *ptr; ptr++)
		{
			*ptr = tolower(*ptr);
		}
		if (!strstr(email, filter->emailFilter))
		{
			PBL_FREE(email);
			return 0;
		}
		PBL_FREE(email);
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
 * Authors are listed by name, author filter and email filter match if contained.
 *
 * The first offset authors are skipped, at most n authors are handled.
 */
int avDbAuthorsList(int offset, int n, char * authorFilter, char * emailFilter)
{
	char * ptr;

	if (authorFilter && *authorFilter)
	{
		authorFilter = pblCgiStrDup(authorFilter);
		for (ptr = authorFilter; *ptr; ptr++)
		{
			*ptr = tolower(*ptr);
		}
	}

	if (emailFilter && *emailFilter)
	{
		emailFilter = pblCgiStrDup(emailFilter);
		for (ptr = emailFilter; *ptr; ptr++)
		{
			*ptr = tolower(*ptr);
		}
	}

	struct avAuthorCallbackFilter filter;
	filter.authorFilter = authorFilter;
	filter.emailFilter = emailFilter;
	filter.map = pblCgiNewMap();
	filter.n = n;
	filter.offset = offset;

	int iteration = 0;

	char * sql = sqlite3_mprintf("SELECT %s, %s, %s FROM author ORDER BY %s ASC; ", AV_KEY_ID, AV_KEY_NAME, AV_KEY_EMAIL,
	AV_KEY_NAME);
	avSqlExec(avSqliteDb, sql, avCallbackAuthorFilteredValues, &filter);
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
		avDbAuthorSetValuesForIteration(value, iteration++);
	}

	pblCgiMapFree(filter.map);
	return iteration;
}

/**
 * Authors are listed by time activated, the given time of activation is used for exact matches.
 *
 * The first offset authors are skipped, at most n authors are handled.
 */
int avDbAuthorsListByTimeActivated(int offset, int n, char * timeActivated)
{
	int iteration = 0;
	PblMap * map = pblCgiNewMap();

	char * sql = sqlite3_mprintf("SELECT %s FROM author WHERE %s = %Q; ", AV_KEY_ID, AV_KEY_TIME_ACTIVATED, timeActivated);
	avSqlExec(avSqliteDb, sql, avCallbackColumnValues, &map);
	sqlite3_free(sql);

	for (int i = 0; i >= 0; i++)
	{
		if (n == 0)
		{
			break;
		}
		char * key = pblCgiSprintf("%d", i);
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
		avDbAuthorSetValuesForIteration(value, iteration++);
	}

	pblCgiMapFree(map);
	return iteration;
}

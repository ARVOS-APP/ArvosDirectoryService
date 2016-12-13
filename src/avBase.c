/*
 avBase.c - base functions for arvos CGI directory service.

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

 $Log: avBase.c,v $
 Revision 1.4  2016/12/13 21:47:21  peter
 Commit after Win32 port

 Revision 1.3  2016/12/03 00:03:54  peter
 Cleanup after tests

 */

/*
 * Make sure "strings <exe> | grep Id | sort -u" shows the source file versions
 */
char * avBase_c_id = "$Id: avBase.c,v 1.4 2016/12/13 21:47:21 peter Exp $";

#include <stdio.h>
#include <memory.h>

#ifndef __APPLE__
#include <malloc.h>
#endif

#include <assert.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "arvos.h"

/*****************************************************************************/
/* Variables                                                                 */
/*****************************************************************************/

PblMap * avConfigMap;
char * avTemplateDirectory;

char * avUserIsLoggedIn = NULL;
char * avUserIsAuthor = NULL;
char * avUserIsAdministrator = NULL;
char * avUserId = NULL;
char * avSessionId = NULL;

static PblList * avAdministratorNames = NULL;

static int avNumberOfCodeChars = -1;

static char * avCodeChars = "0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ_.";

/*****************************************************************************/
/* Functions                                                                 */
/*****************************************************************************/

/**
 * Return the current time as a readable string
 */
char * avNowStr()
{
	time_t now = time(NULL);
	return avTimeToStr(now);
}

/**
 * Create a random code string of length bytes.
 */
char * avRandomCode(size_t length)
{
	static char * tag = "avRandomCode";

	if (avNumberOfCodeChars < 0)
	{
		avNumberOfCodeChars = strlen(avCodeChars);
	}

	unsigned char * buffer = pbl_malloc(NULL, length + 1);
	if (!buffer)
	{
		avExitOnError("%s: pbl_errno = %d, message='%s'\n", tag, pbl_errno, pbl_errstr);
	}
	avRandomBytes(buffer, length);

	for (unsigned int i = 0; i < length; i++)
	{
		buffer[i] = avCodeChars[buffer[i] % avNumberOfCodeChars];
	}
	buffer[length] = '\0';
	return (char*) buffer;
}

/**
 * Create a random int string of length bytes.
 */
char * avRandomIntCode(size_t length)
{
	static char * tag = "avRandomIntCode";
	unsigned char * buffer = pbl_malloc(NULL, length + 1);
	if (!buffer)
	{
		avExitOnError("%s: pbl_errno = %d, message='%s'\n", tag, pbl_errno, pbl_errstr);
	}
	avRandomBytes(buffer, length);

	for (unsigned int i = 0; i < length; i++)
	{
		buffer[i] = '0' + (buffer[i] % 10);
	}
	buffer[length] = '\0';
	return (char*) buffer;
}

/**
 * Create a random hex string of length bytes.
 */
char * avRandomHexCode(size_t length)
{
	static char * tag = "avRandomHexCode";
	unsigned char * buffer = pbl_malloc(NULL, length + 1);
	if (!buffer)
	{
		avExitOnError("%s: pbl_errno = %d, message='%s'\n", tag, pbl_errno, pbl_errstr);
	}
	avRandomBytes(buffer, length);

	for (unsigned int i = 0; i < length; i++)
	{
		int c = buffer[i] % 16;
		if (c < 10)
		{
			buffer[i] = '0' + c;
		}
		else
		{
			buffer[i] = 'a' + c - 10;
		}
	}
	buffer[length] = '\0';
	return (char*) buffer;
}

/**
 * Get the value given for the key in the configuration.
 */
char * avConfigValue(char * key, char * defaultValue)
{
	if (!avConfigMap)
	{
		avExitOnError("The configuration file was never read!\n");
	}
	if (!key || !*key)
	{
		avExitOnError("Empty key not allowed!\n");
	}
	char * value = pblMapGetStr(avConfigMap, key);
	if (!value)
	{
		return defaultValue;
	}
	return value;
}

/**
 * Set the names of the authors that are administrator as defined in the config file.
 */
void avSetAdministratorNames()
{
	avAdministratorNames = avSplitToList(avConfigValue(AV_ADMINISTRATOR_NAMES, ""), ",");
	pblListSetCompareFunction(avAdministratorNames, pblCollectionStringCompareFunction);
}

static int avCheckPassword(char * password, char * hashedPassword)
{
	if (!hashedPassword || strlen(hashedPassword) < 65)
	{
		return 0;
	}

	char * salt = avStrDup(hashedPassword);
	salt[64] = '\0';

	char * saltedPassword = avStrCat(salt, password);
	char * hash = avSha256AsHexString((unsigned char*) saltedPassword, strlen(saltedPassword));
	char * saltedHash = avStrCat(salt, hash);

	int result = avStrEquals(hashedPassword, saltedHash);

	PBL_FREE(salt);
	PBL_FREE(saltedPassword);
	PBL_FREE(hash);
	PBL_FREE(saltedHash);

	return result;
}

/**
 * Hash a password.
 *
 * @return char * saltedHash: The salted and hashed password.
 */
char * avHashPassword(char * password)
{
	unsigned char buffer[32];
	avRandomBytes(buffer, 32);
	char * salt = avBufferToHexString(buffer, 32);

	char * saltedPassword = avStrCat(salt, password);
	char * hash = avSha256AsHexString((unsigned char*) saltedPassword, strlen(saltedPassword));
	char * saltedHash = avStrCat(salt, hash);

	PBL_FREE(salt);
	PBL_FREE(saltedPassword);
	PBL_FREE(hash);

	return saltedHash;
}

/**
 * Create an author with the given name, email and password.
 */
char * avAuthorCreate(char * name, char * email, char * password)
{
	PblMap * map = avDbAuthorGetByName(name);
	if (map)
	{
		avMapFree(map);
		return avSprintf("An author with the name '%s' already exists, please use a different name to register.", name);
	}

	char * authorId = avDbAuthorInsert(name, email, password);
	return avSessionCreate(authorId, name, email, "");
}

static void avLoginUser(char* name, char* authorId, char* timeActivated, char* email, char * sessionId, char* cookie)
{
	avUserIsLoggedIn = avStrDup(name);
	avUserId = avStrDup(authorId);
	avSessionId = avStrDup(sessionId);
	avSetValue(AV_KEY_USER_IS_LOGGED_IN, avUserIsLoggedIn);
	if (timeActivated && *timeActivated && !avStrEquals(AV_NOT_ACTIVATED, timeActivated))
	{
		avUserIsAuthor = avUserIsLoggedIn;
		avSetValue(AV_KEY_USER_IS_AUTHOR, avUserIsLoggedIn);
	}
	if (avAdministratorNames && pblListContains(avAdministratorNames, name))
	{
		if (!avUserIsAuthor)
		{
			avUserIsAuthor = avUserIsLoggedIn;
			avSetValue(AV_KEY_USER_IS_AUTHOR, avUserIsLoggedIn);
		}
		avUserIsAdministrator = avUserIsLoggedIn;
		avSetValue(AV_KEY_USER_IS_ADMIN, avUserIsLoggedIn);
	}
	avSetValue(AV_KEY_USER_ID, avUserId);
	avSetValue(AV_KEY_USER_NAME, avUserIsLoggedIn);
	avSetValue(AV_KEY_USER_EMAIL, email);
	avSetValue(AV_COOKIE, cookie);
	avSetValue(AV_COOKIE_PATH, "/");
	avSetValue(AV_COOKIE_DOMAIN, avGetEnv("SERVER_NAME"));
}

/**
 * Create a session with the given author id, name and email and activation time.
 */
char * avSessionCreate(char * authorId, char * name, char * email, char * timeActivated)
{
	char * cookie = NULL;
	char * sessionId = avDbSessionInsert(authorId, name, email, timeActivated, &cookie);
	avLoginUser(name, authorId, timeActivated, email, sessionId, cookie);
	return NULL;
}

/**
 * Delete a session with the given id.
 */
char * avSessionDelete(char * id)
{
	if (!avUserIsAdministrator || !id || !*id)
	{
		return NULL;
	}

	avDbSessionDelete(id);
	return NULL;
}

/**
 * Delete a session with the given author id.
 */
char * avSessionDeleteByAuthor(char * authorId)
{
	if (!avUserIsAdministrator || !authorId || !*authorId)
	{
		return NULL;
	}

	avDbSessionDeleteByAuthor(authorId);
	return NULL;
}

/**
 * Delete a session with the given cookie.
 */
char * avSessionDeleteByCookie(char * cookie)
{
	if (!cookie || !*cookie)
	{
		return NULL;
	}

	avDbSessionDeleteByCookie(cookie);
	return NULL;
}

/**
 * Check whether a cookie is valid and login the user if ok.
 */
void avCheckCookie(char * cookie)
{
	PblMap * map = avDbSessionGetByCookie(cookie);
	if (!map)
	{
		return;
	}

	char * sessionId = pblMapGetStr(map, AV_KEY_ID);
	if (!sessionId || !*sessionId)
	{
		return;
	}

	char * authorId = pblMapGetStr(map, AV_KEY_AUTHOR);
	if (!authorId || !*authorId)
	{
		return;
	}

	char * name = pblMapGetStr(map, AV_KEY_NAME);
	if (!name || !*name)
	{
		return;
	}

	char * email = pblMapGetStr(map, AV_KEY_EMAIL);
	if (!email || !*email)
	{
		return;
	}
	char * timeActivated = pblMapGetStr(map, AV_KEY_TIME_ACTIVATED);

	avSetValue(AV_KEY_FILTER_LAT, pblMapGetStr(map, AV_KEY_FILTER_LAT));
	avSetValue(AV_KEY_FILTER_LON, pblMapGetStr(map, AV_KEY_FILTER_LON));
	avSetValue(AV_KEY_FILTER_CHANNEL, pblMapGetStr(map, AV_KEY_FILTER_CHANNEL));
	avSetValue(AV_KEY_FILTER_AUTHOR, pblMapGetStr(map, AV_KEY_FILTER_AUTHOR));
	avSetValue(AV_KEY_FILTER_DESCRIPTION, pblMapGetStr(map, AV_KEY_FILTER_DESCRIPTION));
	avSetValue(AV_KEY_FILTER_EMAIL, pblMapGetStr(map, AV_KEY_FILTER_EMAIL));
	avSetValue(AV_KEY_FILTER_DEVELOPER_KEY, pblMapGetStr(map, AV_KEY_FILTER_DEVELOPER_KEY));

	avLoginUser(name, authorId, timeActivated, email, sessionId, cookie);

	char * nowTimeStr = avNowStr();

	avDbSessionUpdateColumn(AV_KEY_ID, pblMapGetStr(map, AV_KEY_ID), AV_KEY_TIME_LAST_ACCESS, nowTimeStr);

	PBL_FREE(nowTimeStr);
	avMapFree(map);
}

/**
 * Check whether name and password are valid.
 */
char * avCheckNameAndPassword(char * name, char * password)
{
	PblMap * map = avDbAuthorGetByName(name);
	if (!map)
	{
		sleep(5);
		return "Login failed.";
	}

	char * authorPassword = pblMapGetStr(map, AV_KEY_PASSWORD);
	if (!authorPassword || !avCheckPassword(password, authorPassword))
	{
		sleep(5);
		return "Login failed.";
	}

	avMapFree(map);
	return NULL;
}

/**
 * Check whether name and password are valid and login the user if ok.
 */
char * avCheckNameAndPasswordAndLogin(char * name, char * password)
{
	char * message = avCheckNameAndPassword(name, password);
	if (message)
	{
		return message;
	}

	PblMap * map = avDbAuthorGetByName(name);
	if (!map)
	{
		sleep(5);
		return "Login failed.";
	}

	char * email = pblMapGetStr(map, AV_KEY_EMAIL);
	char * id = pblMapGetStr(map, AV_KEY_ID);

	char * timeActivated = pblMapGetStr(map, AV_KEY_TIME_ACTIVATED);

	char * authorKeyValues[] = { AV_KEY_COUNT, AV_KEY_TIME_LAST_ACCESS, NULL };
	char * authorDataValues[] = { avValueIncrement, avNowStr(), NULL };

	avDbAuthorUpdateValues(AV_KEY_ID, id, authorKeyValues, authorDataValues, NULL);

	char * ptr = avSessionCreate(id, name, email, timeActivated);

	avMapFree(map);
	return ptr;
}

/**
 * Used as aggregation function to copy map values to values for an iteration.
 */
int avMapStrToValues(void * context, int index, void * element)
{
	int * iteration = (int*) context;

	PblMapEntry * entry = (PblMapEntry*) element;

	if (entry->keyLength < 1)
	{
		return 0;
	}

	if (entry->valueLength > 1)
	{
		avSetValueForIteration(pblMapEntryKey(entry), pblMapEntryValue(entry), *iteration);
	}
	else
	{
		avUnSetValueForIteration(pblMapEntryKey(entry), *iteration);
	}
	return 0;
}

/**
 * Print a template
 */
void avPrintTemplate(char * directory, char * fileName, char * contentType)
{
	if (avSqliteDb)
	{
		sqlite3_close(avSqliteDb);
	}
	avPrint(directory, fileName, contentType);
	exit(0);
}

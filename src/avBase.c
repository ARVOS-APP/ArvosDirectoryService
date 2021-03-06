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
 Revision 1.7  2018/04/29 18:42:08  peter
 More work on service

 Revision 1.6  2018/04/26 14:27:36  peter
 Working on the service

 Revision 1.5  2018/03/11 00:34:52  peter
 Integration with pbl cgi code.

 Revision 1.4  2016/12/13 21:47:21  peter
 Commit after Win32 port

 Revision 1.3  2016/12/03 00:03:54  peter
 Cleanup after tests

 */

/*
 * Make sure "strings <exe> | grep Id | sort -u" shows the source file versions
 */
char * avBase_c_id = "$Id: avBase.c,v 1.7 2018/04/29 18:42:08 peter Exp $";

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
	return pblCgiStrFromTime(now);
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

	unsigned char * bufferForRandomBytes = avMallocRandomBytes(tag, length + 1);
	for (unsigned int i = 0; i < length; i++)
	{
		bufferForRandomBytes[i] = avCodeChars[bufferForRandomBytes[i] % avNumberOfCodeChars];
	}
	bufferForRandomBytes[length] = '\0';
	return (char*) bufferForRandomBytes;
}

/**
 * Create a random int string of length bytes.
 */
char * avRandomIntCode(size_t length)
{
	static char * tag = "avRandomIntCode";

	unsigned char * bufferForRandomBytes = avMallocRandomBytes(tag, length + 1);
	for (unsigned int i = 0; i < length; i++)
	{
		bufferForRandomBytes[i] = '0' + (bufferForRandomBytes[i] % 10);
	}
	bufferForRandomBytes[length] = '\0';
	return (char*) bufferForRandomBytes;
}

/**
 * Create a random hex string of length bytes.
 */
char * avRandomHexCode(size_t length)
{
	static char * tag = "avRandomHexCode";

	unsigned char * bufferForRandomBytes = avMallocRandomBytes(tag, length + 1);
	for (unsigned int i = 0; i < length; i++)
	{
		int c = bufferForRandomBytes[i] % 16;
		if (c < 10)
		{
			bufferForRandomBytes[i] = '0' + c;
		}
		else
		{
			bufferForRandomBytes[i] = 'a' + c - 10;
		}
	}
	bufferForRandomBytes[length] = '\0';
	return (char*) bufferForRandomBytes;
}

/**
 * Set the names of the authors that are administrator as defined in the config file.
 */
void avSetAdministratorNames()
{
	avAdministratorNames = pblCgiStrSplitToList(pblCgiConfigValue(AV_ADMINISTRATOR_NAMES, ""), ",");
	pblListSetCompareFunction(avAdministratorNames, pblCollectionStringCompareFunction);
}

static int avCheckPassword(char * password, char * hashedPassword)
{
	if (!hashedPassword || strlen(hashedPassword) < 65)
	{
		return 0;
	}

	char * salt = pblCgiStrDup(hashedPassword);
	salt[64] = '\0';

	char * saltedPassword = pblCgiStrCat(salt, password);
	char * hash = avSha256AsHexString((unsigned char*) saltedPassword, strlen(saltedPassword));
	char * saltedHash = pblCgiStrCat(salt, hash);

	int result = pblCgiStrEquals(hashedPassword, saltedHash);

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
	unsigned char bufferForRandomBytes[32];
	avRandomBytes(bufferForRandomBytes, sizeof(bufferForRandomBytes));
	char * salt = pblCgiStrToHexFromBuffer(bufferForRandomBytes, sizeof(bufferForRandomBytes));

	char * saltedPassword = pblCgiStrCat(salt, password);
	char * hash = avSha256AsHexString((unsigned char*) saltedPassword, strlen(saltedPassword));
	char * saltedHash = pblCgiStrCat(salt, hash);

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
		pblCgiMapFree(map);
		return pblCgiSprintf("An author with the name '%s' already exists, please use a different name to register.", name);
	}

	char * authorId = avDbAuthorInsert(name, email, password);
	return avSessionCreate(authorId, name, email, "");
}

static void avLoginUser(char* name, char* authorId, char* timeActivated, char* email, char * sessionId, char* cookie)
{
	avUserIsLoggedIn = pblCgiStrDup(name);
	avUserId = pblCgiStrDup(authorId);
	avSessionId = pblCgiStrDup(sessionId);
	pblCgiSetValue(AV_KEY_USER_IS_LOGGED_IN, avUserIsLoggedIn);
	if (timeActivated && *timeActivated && !pblCgiStrEquals(AV_NOT_ACTIVATED, timeActivated))
	{
		avUserIsAuthor = avUserIsLoggedIn;
		pblCgiSetValue(AV_KEY_USER_IS_AUTHOR, avUserIsLoggedIn);
	}
	if (avAdministratorNames && pblListContains(avAdministratorNames, name))
	{
		if (!avUserIsAuthor)
		{
			avUserIsAuthor = avUserIsLoggedIn;
			pblCgiSetValue(AV_KEY_USER_IS_AUTHOR, avUserIsLoggedIn);
		}
		avUserIsAdministrator = avUserIsLoggedIn;
		pblCgiSetValue(AV_KEY_USER_IS_ADMIN, avUserIsLoggedIn);
	}
	pblCgiSetValue(AV_KEY_USER_ID, avUserId);
	pblCgiSetValue(AV_KEY_USER_NAME, avUserIsLoggedIn);
	pblCgiSetValue(AV_KEY_USER_EMAIL, email);
	pblCgiSetValue(PBL_CGI_COOKIE, cookie);
	pblCgiSetValue(PBL_CGI_COOKIE_PATH, "/");
	pblCgiSetValue(PBL_CGI_COOKIE_DOMAIN, pblCgiGetEnv("SERVER_NAME"));
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

	pblCgiSetValue(AV_KEY_FILTER_LAT, pblMapGetStr(map, AV_KEY_FILTER_LAT));
	pblCgiSetValue(AV_KEY_FILTER_LON, pblMapGetStr(map, AV_KEY_FILTER_LON));
	pblCgiSetValue(AV_KEY_FILTER_CHANNEL, pblMapGetStr(map, AV_KEY_FILTER_CHANNEL));
	pblCgiSetValue(AV_KEY_FILTER_AUTHOR, pblMapGetStr(map, AV_KEY_FILTER_AUTHOR));
	pblCgiSetValue(AV_KEY_FILTER_DESCRIPTION, pblMapGetStr(map, AV_KEY_FILTER_DESCRIPTION));
	pblCgiSetValue(AV_KEY_FILTER_EMAIL, pblMapGetStr(map, AV_KEY_FILTER_EMAIL));
	pblCgiSetValue(AV_KEY_FILTER_DEVELOPER_KEY, pblMapGetStr(map, AV_KEY_FILTER_DEVELOPER_KEY));

	avLoginUser(name, authorId, timeActivated, email, sessionId, cookie);

	char * nowTimeStr = avNowStr();

	avDbSessionUpdateColumn(AV_KEY_ID, pblMapGetStr(map, AV_KEY_ID), AV_KEY_TIME_LAST_ACCESS, nowTimeStr);

	PBL_FREE(nowTimeStr);
	pblCgiMapFree(map);
}

/**
 * Check whether name and password are valid.
 */
char * avCheckNameAndPassword(char * name, char * password)
{
	PblMap * map = avDbAuthorGetByName(name);
	if (!map)
	{
#ifdef WIN32
		_sleep(5000);
#else
		sleep(5);
#endif
		return "Login failed.";
	}

	char * authorPassword = pblMapGetStr(map, AV_KEY_PASSWORD);
	if (!authorPassword || !avCheckPassword(password, authorPassword))
	{
#ifdef WIN32
		_sleep(5000);
#else
		sleep(5);
#endif
		return "Login failed.";
	}

	pblCgiMapFree(map);
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
#ifdef WIN32
		_sleep(5000);
#else
		sleep(5);
#endif
		return "Login failed.";
	}

	char * email = pblMapGetStr(map, AV_KEY_EMAIL);
	char * id = pblMapGetStr(map, AV_KEY_ID);

	char * timeActivated = pblMapGetStr(map, AV_KEY_TIME_ACTIVATED);

	char * authorKeyValues[] = { AV_KEY_COUNT, AV_KEY_TIME_LAST_ACCESS, NULL };
	char * authorDataValues[] = { pblCgiValueIncrement, avNowStr(), NULL };

	avDbAuthorUpdateValues(AV_KEY_ID, id, authorKeyValues, authorDataValues, NULL);

	char * ptr = avSessionCreate(id, name, email, timeActivated);

	pblCgiMapFree(map);
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
		pblCgiSetValueForIteration(pblMapEntryKey(entry), pblMapEntryValue(entry), *iteration);
	}
	else
	{
		pblCgiUnSetValueForIteration(pblMapEntryKey(entry), *iteration);
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
	pblCgiPrint(directory, fileName, contentType);
	exit(0);
}

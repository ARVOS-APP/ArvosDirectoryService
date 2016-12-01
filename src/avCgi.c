/*
 avCgi.c - Common Gateway Interface functions for arvos directory service.

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

 $Log: avCgi.c,v $
 Revision 1.4  2016/12/01 22:25:53  peter
 Fixed a typo.

 */

/*
 * Make sure "strings <exe> | grep Id | sort -u" shows the source file versions
 */
char * avCgi_c_id = "$Id: avCgi.c,v 1.4 2016/12/01 22:25:53 peter Exp $";

#ifdef WIN32

// define _CRT_RAND_S prior to inclusion statement.
#define _CRT_RAND_S

#include <stdlib.h>
#include <stdio.h>
#include <limits.h>

#else

#include <stdlib.h>
#include <stdio.h>
#include <limits.h>
#include <unistd.h>

#endif

#include <memory.h>
#include <fcntl.h>

#ifndef __APPLE__
#include <malloc.h>
#endif

#include "arvosCgi.h"

/*****************************************************************************/
/* #defines                                                                  */
/*****************************************************************************/
#define AV_MAX_SIZE_OF_BUFFER_ON_STACK		(4 * 1024)
#define AV_MAX_KEY_LENGTH			        64
#define AV_MAX_QUERY_PARAMETERS_COUNT	    128
#define AV_MAX_POST_INPUT_LEN               (1024 * 1024)
#define AV_MAX_DIGEST_LEN                   32

#define AV_DATE_LEN                         14

/*****************************************************************************/
/* Variables                                                                 */
/*****************************************************************************/

static const char *avHexDigits = "0123456789abcdef";

static struct timeval avStartTime;

FILE * avTraceFile = NULL;

sqlite3 * avSqliteDb = NULL;

char * avValueIncrement = "i++";

/*****************************************************************************/
/* Functions                                                                 */
/*****************************************************************************/

/**
 * Wrapper for sqlite3_exec
 */
void avSqlExec(char * statement, int (*callback)(void*, int, char**, char**), void * parameter)
{
	if (!statement)
	{
		avExitOnError("Out of memory\n");
	}

	char * message = NULL;

	if (SQLITE_OK != sqlite3_exec(avSqliteDb, statement, callback, parameter, &message))
	{
		if (message)
		{
			if (strstr(message, "callback requested query abort"))
			{
				return;
			}
			avExitOnError("Failed to execute SQL statement \"%s\", message: %s\n", statement, message);
			sqlite3_free(message);
		}
	}
}

/**
 * SqLite callback that expects an int * as parameter and counts the calls made back
 */
int avCallbackCounter(void * ptr, int nColums, char ** values, char ** headers)
{
	int * counter = (int *) ptr;
	if (counter)
	{
		*counter += 1;
	}
	return 0;
}

/**
 * SqLite callback that expects a single cell value and duplicates it to the pointer
 */
int avCallbackCellValue(void * ptr, int nColums, char ** values, char ** headers)
{
	if (nColums != 1)
	{
		avExitOnError("SQLite callback avCallbackCellValue called with %d columns\n", nColums);
	}
	char ** cellValue = (char **) ptr;
	if (cellValue)
	{
		*cellValue = avStrDup(values[0]);
	}
	return 0;
}

/**
 * SqLite callback that expects column values of a single row and sets them to the pointer map
 */
int avCallbackRowValues(void * ptr, int nColums, char ** values, char ** headers)
{
	if (nColums < 1)
	{
		avExitOnError("SQLite callback avCallbackRowValues called with %d columns\n", nColums);
	}
	PblMap ** mapPtr = (PblMap **) ptr;
	if (mapPtr)
	{
		for (int i = 0; i < nColums; i++)
		{
			if (avStrEquals("VALS", headers[i]))
			{
				avDataStrToMap(*mapPtr, values[i]);
			}
			else
			{
				avSetValueToMap(headers[i], values[i], -1, *mapPtr);
			}
		}
	}
	return 0;
}

/**
 * SqLite callback that expects values of a single column in multiple rows and sets them to the pointer map
 */
int avCallbackColumnValues(void * ptr, int nColums, char ** values, char ** headers)
{
	if (nColums != 1)
	{
		avExitOnError("SQLite callback avCallbackColumnValues called with %d columns\n", nColums);
	}
	PblMap ** mapPtr = (PblMap **) ptr;
	if (mapPtr)
	{
		char * key = avSprintf("%d", pblMapSize(*mapPtr));
		avSetValueToMap(key, values[0], -1, *mapPtr);
		PBL_FREE(key);
	}
	return 0;
}

void avInit(struct timeval * startTime, char * traceFilePath, char * databasePath)
{
	avStartTime = *startTime;

	//
	// sqlite link object sqlite3.o was created with command:
	// CFLAGS="-Os -DSQLITE_THREADSAFE=0" ./configure; make
	//

	char * filePath = avStrCat(databasePath, "arvos.sqlite");
	if ( SQLITE_OK != sqlite3_open(filePath, &avSqliteDb))
	{
		if (!avSqliteDb)
		{
			avExitOnError("Can't open SQLite database '%s': Out of memory\n", filePath);
		}
		avExitOnError("Can't open SQLite database '%s': %s\n", filePath, sqlite3_errmsg(avSqliteDb));
		sqlite3_close(avSqliteDb);
	}

	char * sql = "SELECT name FROM sqlite_master WHERE type='table' AND name='session';";
	int count = 0;

	avSqlExec(sql, avCallbackCounter, &count);

	if (count == 0)
	{
		char * create =
				"CREATE TABLE session ( ID INTEGER PRIMARY KEY, COK TEXT UNIQUE, TLA TEXT, AUT TEXT, VALS TEXT );";

		avSqlExec(create, NULL, NULL);
		avSqlExec(sql, avCallbackCounter, &count);

		if (count == 0)
		{
			avExitOnError("Failed to create session table '%s'\n", create);
		}
	}

	sql = "SELECT name FROM sqlite_master WHERE type='table' AND name='author';";
	count = 0;

	avSqlExec(sql, avCallbackCounter, &count);

	if (count == 0)
	{
		char * create =
				"CREATE TABLE author ( ID INTEGER PRIMARY KEY, NAM TEXT UNIQUE, EML TEXT, TAC TEXT, VALS TEXT );";

		avSqlExec(create, NULL, NULL);
		avSqlExec(sql, avCallbackCounter, &count);

		if (count == 0)
		{
			avExitOnError("Failed to create author table '%s'\n", create);
		}
	}

	sql = "SELECT name FROM sqlite_master WHERE type='table' AND name='channel';";
	count = 0;

	avSqlExec(sql, avCallbackCounter, &count);

	if (count == 0)
	{
		char * create =
				"CREATE TABLE channel ( ID INTEGER PRIMARY KEY, CHN TEXT UNIQUE, AUT TEXT, DES TEXT, DEV TEXT, VALS TEXT );";

		avSqlExec(create, NULL, NULL);
		avSqlExec(sql, avCallbackCounter, &count);

		if (count == 0)
		{
			avExitOnError("Failed to create channel table '%s'\n", create);
		}
	}

	sql = "SELECT name FROM sqlite_master WHERE type='table' AND name='location';";
	count = 0;

	avSqlExec(sql, avCallbackCounter, &count);

	if (count == 0)
	{
		char * create = "CREATE TABLE location ( ID INTEGER PRIMARY KEY, CHN TEXT, POS TEXT, VALS TEXT ); "
				"CREATE INDEX location_POS_index ON location(POS);";

		avSqlExec(create, NULL, NULL);
		avSqlExec(sql, avCallbackCounter, &count);

		if (count == 0)
		{
			avExitOnError("Failed to create location table '%s'\n", create);
		}
	}

	if (traceFilePath && *traceFilePath)
	{
		FILE * stream;

#ifdef WIN32

		errno_t err = fopen_s(&stream, filePath, "r");
		if (err != 0)
		{
			return;
		}

#else

		if (!(stream = fopen(traceFilePath, "r")))
		{
			return;
		}

#endif

		fclose(stream);

		avTraceFile = avFopen(traceFilePath, "a");
		fputs("\n", avTraceFile);
		fputs("\n", avTraceFile);
		AV_TRACE("----------------------------------------> Started");

		extern char **environ;
		char ** envp = environ;

		while (envp && *envp)
		{
			AV_TRACE("ENV %s", *envp++);
		}
	}
}

void avTrace(const char * format, ...)
{
	if (!avTraceFile)
	{
		return;
	}

	va_list args;
	va_start(args, format);

	char buffer[AV_MAX_SIZE_OF_BUFFER_ON_STACK + 1];
	int rc = vsnprintf(buffer, sizeof(buffer) - 1, format, args);
	va_end(args);

	if (rc < 0)
	{
		avExitOnError("Printing of format '%s' and size %lu failed with errno=%d\n", format, sizeof(buffer) - 1,
		errno);
	}
	buffer[sizeof(buffer) - 1] = '\0';

	char * now = avTimeToStr(time(NULL));
	fputs(now, avTraceFile);
	PBL_FREE(now);

	fprintf(avTraceFile, " %d: ", getpid());

	fputs(buffer, avTraceFile);
	fputs("\n", avTraceFile);
	fflush(avTraceFile);
}

/**
 * Like fopen, needed for Windows port
 */
FILE * avFopen(char * filePath, char * openType)
{
	char * tag = "avFopen";

	FILE * stream;

#ifdef WIN32

	errno_t err = fopen_s(&stream, filePath, openType);
	if (err != 0)
	{
		avExitOnError("%s: Cannot open file '%s', err=%d, errno=%d\n", tag,
				filePath, err, errno);
	}

#else

	if (!(stream = fopen(filePath, openType)))
	{
		avExitOnError("%s: Cannot open file '%s', errno=%d\n", tag, filePath, errno);
	}

#endif
	return stream;
}

/**
 * Like getenv, needed for Windows port
 */
char * avGetEnv(char * name)
{
#ifdef WIN32

	char *value;
	size_t len;
	_dupenv_s(&value, &len, name);
	return value;

#else

	return getenv(name);

#endif
}

static char * avCookieKey = AV_COOKIE;
static char * avCookieTag = AV_COOKIE "=";

/**
 * Get the cookie, if any is given.
 */
char * avGetCoockie()
{
	char * cookie = avStrDup(avGetEnv("HTTP_COOKIE"));
	if (cookie && *cookie)
	{
		cookie = strstr(cookie, avCookieTag);
		if (cookie)
		{
			cookie += strlen(avCookieTag);
		}
	}
	else
	{
		cookie = avQueryValue(avCookieKey);
	}

	if (!cookie || !*cookie)
	{
		return NULL;
	}
	for (char * ptr = cookie; *ptr; ptr++)
	{
		if (!strchr(avHexDigits, *ptr))
		{
			*ptr = '\0';
			break;
		}
	}
	return cookie;
}

static char * contentType = NULL;
static void avSetContentType(char * type)
{
	if (!contentType)
	{
		char * cookie = avValue(AV_COOKIE);
		char * cookiePath = avValue(AV_COOKIE_PATH);
		char * cookieDomain = avValue(AV_COOKIE_DOMAIN);

		contentType = type;

		if (cookie && cookiePath && cookieDomain)
		{
			printf("Content-Type: %s\n", contentType);
			AV_TRACE("Content-Type: %s\n", contentType);

			printf("Set-Cookie: %s%s; Path=%s; DOMAIN=%s; HttpOnly\n\n", avCookieTag, cookie, cookiePath, cookieDomain);
			AV_TRACE("Set-Cookie: %s%s; Path=%s; DOMAIN=%s; HttpOnly\n\n", avCookieTag, cookie, cookiePath,
					cookieDomain);
		}
		else
		{
			printf("Content-Type: %s\n\n", contentType);
			AV_TRACE("Content-Type: %s\n", contentType);
		}
	}
}

/**
 * Print an error message and exit the program.
 */
void avExitOnError(const char * format, ...)
{
	avSetContentType("text/html");

	printf("<!DOCTYPE html>\n"
			"<html>\n"
			"<head>\n"
			"<title>Arvos-App Server</title>\n"
			"</head>\n"
			"<body>\n"
			"<h1>\n"
			"<a href=\"http://www.arvos-app.com/\"><img\n"
			"src=\"http://www.arvos-app.com/images/arvos_logo_rgb-weiss256.png\" WIDTH=64></IMG></a> \n"
			"Arvos - Augmented Reality Viewer Open Source\n"
			"</h1>\n"
			"<p><hr> <p>\n"
			"<h1>An error occurred</h1>\n");

	char * scriptName = avGetEnv("SCRIPT_NAME");
	if (!scriptName || !*scriptName)
	{
		scriptName = "unknown";
	}

	printf("<p>While accessing the script '%s'.\n", scriptName);
	printf("<p><b>\n");

	va_list args;
	va_start(args, format);

	char buffer[AV_MAX_SIZE_OF_BUFFER_ON_STACK + 1];
	int rc = vsnprintf(buffer, sizeof(buffer) - 1, format, args);
	va_end(args);

	if (rc < 0)
	{
		printf("Printing of format '%s' and size %lu failed with errno=%d\n", format, sizeof(buffer) - 1, errno);
	}
	else
	{
		buffer[sizeof(buffer) - 1] = '\0';
		printf("%s", buffer);
		AV_TRACE("%s", buffer);
	}

	printf("</b>\n");
	printf("<p>Please click your browser's back button to continue.\n");
	printf("<p><hr> <p>\n");
	printf("<small>Copyright &copy; 2016 - Tamiko Thiel and Peter Graf</small>\n");
	printf("</body></HTML>\n");

	AV_TRACE("%s exit(-1)", scriptName);
	exit(-1);
}

/**
 * Like sprintf, copies the value to the heap.
 */
char * avSprintf(const char * format, ...)
{
	va_list args;
	va_start(args, format);

	char buffer[AV_MAX_SIZE_OF_BUFFER_ON_STACK + 1];
	int rc = vsnprintf(buffer, sizeof(buffer) - 1, format, args);
	va_end(args);

	if (rc < 0)
	{
		avExitOnError("Printing of format '%s' and size %lu failed with errno=%d\n", format, sizeof(buffer) - 1,
		errno);
	}
	buffer[sizeof(buffer) - 1] = '\0';
	return avStrDup(buffer);
}

/**
 * Tests whether a NULL terminated string array contains a string
 */
int avStrArrayContains(char ** array, char * string)
{
	for (int i = 0;; i++)
	{
		char * ptr = array[i];
		if (!ptr)
		{
			break;
		}
		if (avStrEquals(ptr, string))
		{
			return i;
		}
	}
	return -1;
}

/**
 * Copies at most n bytes, the result is always 0 terminated
 */
char * avStrNCpy(char *dest, char *string, size_t n)
{
	char * ptr = dest;
	while (--n && (*ptr++ = *string++))
		;

	if (*ptr)
	{
		*ptr = '\0';
	}
	return dest;
}

static char * avTrimStart(char * string)
{
	for (; *string; string++)
	{
		if (!isspace(*string))
		{
			return string;
		}
	}
	return string;
}

static void avTrimEnd(char * string)
{
	if (!string)
	{
		return;
	}

	char * ptr = (string + strlen(string)) - 1;
	while (ptr >= string)
	{
		if (isspace(*ptr))
		{
			*ptr-- = '\0';
		}
		else
		{
			return;
		}
	}
}

/**
 * Trim the string, remove blanks at start and end.
 */
char * avTrim(char * string)
{
	avTrimEnd(string);
	return avTrimStart(string);
}

/**
 * Test if a string is NULL or white space only
 */
int avStrIsNullOrWhiteSpace(char * string)
{
	if (string)
	{
		for (; *string; string++)
		{
			if (!isspace(*string))
			{
				return (0);
			}
		}
	}
	return (1);
}

char * avRangeDup(char * start, char * end)
{
	char * tag = "avRangedup";

	start = avTrimStart(start);
	int length = end - start;
	if (length > 0)
	{
		char * value = pbl_memdup(tag, start, length + 1);
		if (!value)
		{
			avExitOnError("%s: pbl_errno = %d, message='%s'\n", tag, pbl_errno, pbl_errstr);
		}
		value[length] = '\0';
		avTrimEnd(value);
		return value;
	}
	return avStrDup("");
}

/**
 * String equals, handles NULLs.
 */
int avStrEquals(char * s1, char * s2)
{
	return !avStrCmp(s1, s2);
}

/**
 * Like strcmp, handles NULLs.
 */
int avStrCmp(char * s1, char * s2)
{
	if (!s1)
	{
		if (!s2)
		{
			return 0;
		}
		return -1;
	}
	if (!s2)
	{
		return 1;
	}
	return strcmp(s1, s2);
}

/**
 * Un-escape duplicate characters from a string, i.e. un-escape SQL query, e.g. turn '' into ', works in place.
 */
char * xxavStrUnescapeDuplicates(char * string, int c)
{
	char * to = NULL;
	char * from;

	for (from = string; *from; from++)
	{
		if (to)
		{
			*to++ = *from;
		}
		if (*from == c && *(from + 1) == c)
		{
			from++;
			if (!to)
			{
				to = from;
			}
		}
	}

	if (to)
	{
		*to = '\0';
	}
	return string;
}

/**
 * Like strdup.
 *
 * If an error occurs, the program exits with an error message.
 */
char * avStrDup(char * string)
{
	char * tag = "avStrDup";
	if (!string)
	{
		string = "";
	}
	string = pbl_strdup(tag, string);
	if (!string)
	{
		avExitOnError("%s: pbl_errno = %d, message='%s'\n", tag, pbl_errno, pbl_errstr);
	}
	return string;
}

/**
 * Like strcat, but the memory for the target is allocated.
 *
 * If an error occurs, the program exits with an error message.
 */
char * avStrCat(char * s1, char * s2)
{
	char * tag = "avStrCat";

	char * result = NULL;
	size_t len1 = 0;
	size_t len2 = 0;

	if (!s1 || !(*s1))
	{
		if (!s2 || !(*s2))
		{
			return avStrDup("");
		}
		return avStrDup(s2);
	}

	if (!s2 || !(*s2))
	{
		return avStrDup(s1);
	}

	len1 = strlen(s1);
	len2 = strlen(s2);
	result = pbl_malloc(tag, len1 + len2 + 1);
	if (!result)
	{
		avExitOnError("%s: pbl_errno = %d, message='%s'\n", tag, pbl_errno, pbl_errstr);
	}

	memcpy(result, s1, len1);
	memcpy(result + len1, s2, len2 + 1);
	return result;
}

/**
 * Like strcat, but the memory for the target is allocated.
 *
 * If an error occurs, the program exits with an error message.
 *
 * The pointer array strings must be NULL terminated.
 */
char * avStrNCat(char ** strings)
{
	char * tag = "avStrNCat";

	PblStringBuilder * stringBuilder = pblStringBuilderNew();
	if (!stringBuilder)
	{
		avExitOnError("%s: pbl_errno = %d, message='%s'\n", tag, pbl_errno, pbl_errstr);
	}

	for (int i = 0; i < AV_MAX_SIZE_OF_BUFFER_ON_STACK; i++)
	{
		char * ptr = strings[i];
		if ( NULL == ptr)
		{
			break;
		}
		if (pblStringBuilderAppendStr(stringBuilder, ptr) == ((size_t) -1))
		{
			avExitOnError("%s: pbl_errno = %d, message='%s'\n", tag, pbl_errno, pbl_errstr);
		}
	}

	char * result = pblStringBuilderToString(stringBuilder);
	if (!result)
	{
		avExitOnError("%s: pbl_errno = %d, message='%s'\n", tag, pbl_errno, pbl_errstr);
	}
	pblStringBuilderFree(stringBuilder);
	return result;
}

/**
 * Return a malloced time string.
 *
 * If an error occurs, the program exits with an error message.
 */
char * avTimeToStr(time_t t)
{
	struct tm *tm;
	tm = localtime((time_t *) &(t));

	return avSprintf("%02d.%02d.%02d %02d:%02d:%02d", (tm->tm_year + 1900) % 100, tm->tm_mon + 1, tm->tm_mday,
			tm->tm_hour, tm->tm_min, tm->tm_sec);
}

/**
 * Convert a time string to a variable of type time_t.
 *
 * If an error occurs, the program exits with an error message.
 */
time_t avStrToTime(char * string)
{
	if (!string || !*string)
	{
		return (0);
	}

	char timeString[AV_DATE_LEN + 1];

	avStrNCpy(timeString, string, sizeof(timeString));

	unsigned long top = 0x7fffffff;
	unsigned long bottom = 0x0;
	unsigned long middle = 0x40000000;

	for (;;)
	{
		if ((bottom + 1) >= top)
		{
			break;
		}

		char * ptr = avTimeToStr((time_t) middle);
		int rc = strcmp(ptr, timeString);
		PBL_FREE(ptr);

		if (rc == 0)
		{
			break;
		}

		if (rc < 0)
		{
			bottom = middle;
		}
		else
		{
			top = middle;
		}

		middle = (bottom + top) / 2;
	}
	return (middle);
}

/**
 * Like split.
 *
 * If an error occurs, the program exits with an error message.
 *
 * The results should be defined as:
 *
 * 	char * results[size + 1];
 */
int avSplit(char * string, char * splitString, size_t size, char * results[])
{
	unsigned int index = 0;
	if (size < 1)
	{
		return index;
	}

	size_t length = strlen(splitString);
	results[0] = NULL;

	char * ptr = string;

	for (;;)
	{
		if (index > size - 1)
		{
			return index;
		}

		char * ptr2 = strstr(ptr, splitString);
		if (!ptr2)
		{
			results[index] = avStrDup(ptr);
			results[++index] = NULL;

			return index;
		}
		results[index] = avRangeDup(ptr, ptr2);
		results[++index] = NULL;

		ptr = ptr2 + length;
	}
	return (index);
}

/**
 * Split a string to a list.
 */
PblList * avSplitToList(char * string, char * splitString)
{
	char * tag = "avSplitToList";

	PblList * list = pblListNewArrayList();
	if (!list)
	{
		avExitOnError("%s: pbl_errno = %d, message='%s'\n", tag, pbl_errno, pbl_errstr);
	}

	char * strings[AV_MAX_LINE_LENGTH + 1];
	int n = avSplit(string, splitString, AV_MAX_LINE_LENGTH, strings);
	for (int i = 0; i < n; i++)
	{
		if (pblListAdd(list, avStrDup(avTrim(strings[i]))) < 0)
		{
			avExitOnError("%s: pbl_errno = %d, message='%s'\n", tag, pbl_errno, pbl_errstr);
		}
	}
	return list;
}

/**
 * Convert a buffer to a hex byte string
 *
 * If an error occurs, the program exits with an error message.
 *
 * return The malloced hex string.
 */
char * avBufferToHexString(unsigned char * buffer, size_t length)
{
	char * tag = "avBufferToHexString";

	char * hexString = pbl_malloc(tag, 2 * length + 1);
	if (!hexString)
	{
		avExitOnError("%s: pbl_errno = %d, message='%s'\n", tag, pbl_errno, pbl_errstr);
	}

	unsigned char c;
	int stringIndex = 0;
	for (unsigned int i = 0; i < length; i++)
	{
		hexString[stringIndex++] = avHexDigits[((c = buffer[i]) >> 4) & 0x0f];
		hexString[stringIndex++] = avHexDigits[c & 0x0f];
	}
	hexString[stringIndex] = 0;

	return hexString;
}

#ifndef ARVOS_CRYPTOLOGIC_RANDOM

static int avRandomFirst = 1;

unsigned char * avRandomBytes(unsigned char * buffer, size_t length)
{
	if (avRandomFirst)
	{
		avRandomFirst = 0;

		struct timeval now;
		gettimeofday(&now, NULL);
		srand(now.tv_sec ^ now.tv_usec ^ getpid());
	}
	if (!rand() % 10)
	{
		struct timeval now;
		gettimeofday(&now, NULL);
		srand(rand() ^ now.tv_sec ^ now.tv_usec ^ getpid());
	}

	for (unsigned int i = 0; i < length; i++)
	{
		buffer[i] = rand() % 0xff;
	}
	return buffer;
}

#else

/**
 * Fill the buffer with random bytes.
 */
unsigned char * avRandomBytes(unsigned char * buffer, size_t length)
{
	char * tag = "avRandomBytes";

#ifdef WIN32

	unsigned int number;
	unsigned char * ptr = (unsigned char *) &number;

	for (unsigned int i = 0; i < length;)
	{
		if (rand_s(&number))
		{
			avExitOnError("%s: rand_s failed, errno=%d\n", tag, errno);
		}

		for (unsigned int j = 0; j < 4 && i < length; i++, j++)
		{
			buffer[i] = ptr[j];
		}
	}

#else

	int randomData = open("/dev/random", O_RDONLY);

	size_t randomDataLen = 0;
	while (randomDataLen < length)
	{
		ssize_t result = read(randomData, buffer + randomDataLen, length - randomDataLen);
		if (result < 0)
		{
			avExitOnError("%s: unable to read /dev/random, errno=%d\n", tag,
					errno);
		}
		randomDataLen += result;
	}
	close(randomData);

#endif
	return buffer;
}
#endif

/*
 * Sha256 implementation, as defined by NIST
 */
#define Sha256_min( x, y ) ( ( x ) < ( y ) ? ( x ) : ( y ) )

#define Sha256_S(x,n) ( ((x)>>(n)) | ((x)<<(32-(n))) )
#define Sha256_R(x,n) ( (x)>>(n) )

#define Sha256_Ch(x,y,z)  ( ((x) & (y)) ^ (~(x) & (z)) )
#define Sha256_Maj(x,y,z) ( ( (x) & (y) ) ^ ( (x) & (z) ) ^ ( (y) & (z) ) )

#define Sha256_SIG0(x) ( Sha256_S(x, 2) ^ Sha256_S(x,13) ^ Sha256_S(x,22) )
#define Sha256_SIG1(x) ( Sha256_S(x, 6) ^ Sha256_S(x,11) ^ Sha256_S(x,25) )
#define Sha256_sig0(x) ( Sha256_S(x, 7) ^ Sha256_S(x,18) ^ Sha256_R(x, 3) )
#define Sha256_sig1(x) ( Sha256_S(x,17) ^ Sha256_S(x,19) ^ Sha256_R(x,10) )

static unsigned int K[] = { 0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5, 0x3956c25b, 0x59f111f1, 0x923f82a4,
		0xab1c5ed5, 0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3, 0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174,
		0xe49b69c1, 0xefbe4786, 0x0fc19dc6, 0x240ca1cc, 0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da, 0x983e5152,
		0xa831c66d, 0xb00327c8, 0xbf597fc7, 0xc6e00bf3, 0xd5a79147, 0x06ca6351, 0x14292967, 0x27b70a85, 0x2e1b2138,
		0x4d2c6dfc, 0x53380d13, 0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85, 0xa2bfe8a1, 0xa81a664b, 0xc24b8b70,
		0xc76c51a3, 0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070, 0x19a4c116, 0x1e376c08, 0x2748774c, 0x34b0bcb5,
		0x391c0cb3, 0x4ed8aa4a, 0x5b9cca4f, 0x682e6ff3, 0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208, 0x90befffa,
		0xa4506ceb, 0xbef9a3f7, 0xc67178f2 };

#define Sha256_H1      0x6a09e667
#define Sha256_H2      0xbb67ae85
#define Sha256_H3      0x3c6ef372
#define Sha256_H4      0xa54ff53a
#define Sha256_H5      0x510e527f
#define Sha256_H6      0x9b05688c
#define Sha256_H7      0x1f83d9ab
#define Sha256_H8      0x5be0cd19

static unsigned int H[8] = { Sha256_H1, Sha256_H2, Sha256_H3, Sha256_H4, Sha256_H5, Sha256_H6, Sha256_H7,
Sha256_H8 };

/* convert to big endian where needed */

static void convert_to_bigendian(void* data, int len)
{
	/* test endianness */
	unsigned int test_value = 0x01;
	unsigned char* test_as_bytes = (unsigned char*) &test_value;
	int little_endian = test_as_bytes[0];

	unsigned int temp;
	unsigned char* temp_as_bytes = (unsigned char*) &temp;
	unsigned int* data_as_words = (unsigned int*) data;
	unsigned char* data_as_bytes;
	int i;

	if (little_endian)
	{
		len /= 4;
		for (i = 0; i < len; i++)
		{
			temp = data_as_words[i];
			data_as_bytes = (unsigned char*) &(data_as_words[i]);

			data_as_bytes[0] = temp_as_bytes[3];
			data_as_bytes[1] = temp_as_bytes[2];
			data_as_bytes[2] = temp_as_bytes[1];
			data_as_bytes[3] = temp_as_bytes[0];
		}
	}
	/* on big endian machines do nothing as the CPU representation
	 automatically does the right thing for SHA1 */
}

/*
 * sha256 context
 */
typedef struct avSha256_ctx_s
{
	unsigned int H[8];
	unsigned int hbits;
	unsigned int lbits;
	unsigned char M[64];
	unsigned char mlen;

} avSha256_ctx_t;

static void avSha256_init(avSha256_ctx_t* ctx)
{
	memcpy(ctx->H, H, 8 * sizeof(unsigned int));
	ctx->lbits = 0;
	ctx->hbits = 0;
	ctx->mlen = 0;
}

static void Sha256_transform(avSha256_ctx_t* ctx)
{
	int j;
	unsigned int A = ctx->H[0];
	unsigned int B = ctx->H[1];
	unsigned int C = ctx->H[2];
	unsigned int D = ctx->H[3];
	unsigned int E = ctx->H[4];
	unsigned int F = ctx->H[5];
	unsigned int G = ctx->H[6];
	unsigned int H = ctx->H[7];
	unsigned int T1, T2;
	unsigned int W[64];

	memcpy(W, ctx->M, 64);

	for (j = 16; j < 64; j++)
	{
		W[j] = Sha256_sig1(W[j - 2]) + W[j - 7] + Sha256_sig0(W[j - 15]) + W[j - 16];
	}

	for (j = 0; j < 64; j++)
	{
		T1 = H + Sha256_SIG1(E) + Sha256_Ch(E, F, G) + K[j] + W[j];
		T2 = Sha256_SIG0(A) + Sha256_Maj(A, B, C);
		H = G;
		G = F;
		F = E;
		E = D + T1;
		D = C;
		C = B;
		B = A;
		A = T1 + T2;
	}

	ctx->H[0] += A;
	ctx->H[1] += B;
	ctx->H[2] += C;
	ctx->H[3] += D;
	ctx->H[4] += E;
	ctx->H[5] += F;
	ctx->H[6] += G;
	ctx->H[7] += H;
}

static void avSha256_update(avSha256_ctx_t* ctx, const void* vdata, unsigned int data_len)
{
	const unsigned char* data = vdata;
	unsigned int use;
	unsigned int low_bits;

	/* convert data_len to bits and add to the 64 bit word formed by lbits
	 and hbits */

	ctx->hbits += data_len >> 29;
	low_bits = data_len << 3;
	ctx->lbits += low_bits;
	if (ctx->lbits < low_bits)
	{
		ctx->hbits++;
	}

	/* deal with first block */

	use = (unsigned int) (Sha256_min((unsigned int )(64 - ctx->mlen), data_len));
	memcpy(ctx->M + ctx->mlen, data, use);
	ctx->mlen += (unsigned char) use;
	data_len -= use;
	data += use;

	while (ctx->mlen == 64)
	{
		convert_to_bigendian((unsigned int*) ctx->M, 64);
		Sha256_transform(ctx);
		use = Sha256_min(64, data_len);
		memcpy(ctx->M, data, use);
		ctx->mlen = (unsigned char) use;
		data_len -= use;
		data += use;
	}
}

static void avSha256_final(avSha256_ctx_t* ctx)
{
	if (ctx->mlen < 56)
	{
		ctx->M[ctx->mlen] = 0x80;
		ctx->mlen++;
		memset(ctx->M + ctx->mlen, 0x00, 56 - ctx->mlen);
		convert_to_bigendian(ctx->M, 56);
	}
	else
	{
		ctx->M[ctx->mlen] = 0x80;
		ctx->mlen++;
		memset(ctx->M + ctx->mlen, 0x00, 64 - ctx->mlen);
		convert_to_bigendian(ctx->M, 64);
		Sha256_transform(ctx);
		memset(ctx->M, 0x00, 56);
	}

	memcpy(ctx->M + 56, (void*) (&(ctx->hbits)), 8);
	Sha256_transform(ctx);
}

void avSha256_digest(avSha256_ctx_t* ctx, unsigned char* digest)
{
	if (digest)
	{
		memcpy(digest, ctx->H, 8 * sizeof(unsigned int));
		convert_to_bigendian(digest, 8 * sizeof(unsigned int));
	}
}

/**
 * Return the sha256 digest of a given buffer of bytes.
 *
 * If an error occurs, the program exits with an error message.
 *
 * @return AV_DIGEST_LEN bytes of static binary data
 */
unsigned char * avSha256(unsigned char * buffer, size_t length)
{
	char * tag = "avSha256";

	unsigned char * digest = pbl_malloc(tag, AV_MAX_DIGEST_LEN);
	if (!digest)
	{
		avExitOnError("%s: pbl_errno = %d, message='%s'\n", tag, pbl_errno, pbl_errstr);
	}
	avSha256_ctx_t context;

	avSha256_init(&context);
	avSha256_update(&context, buffer, length);
	avSha256_final(&context);
	avSha256_digest(&context, digest);

	return (digest);
}

/**
 * Return the sha256 digest of a given buffer of bytes as hex string.
 *
 * If an error occurs, the program exits with an error message.
 *
 * @return The digest as a malloced hex string.
 */
char * avSha256AsHexString(unsigned char * buffer, size_t length)
{
	unsigned char * digest = avSha256(buffer, length);
	char * string = avBufferToHexString(digest, AV_MAX_DIGEST_LEN);
	PBL_FREE(digest);
	return string;
}

static char avCheckChar(char c)
{
	if (c < ' ' || c == '<')
	{
		return ' ';
	}
	return c;
}

static char * avDecode(char * source)
{
	static char * tag = "avDecode";
	char * sourcePtr = source;
	char * destinationPtr;
	char * destination;
	char buffer[3];
	int i;

	destinationPtr = destination = pbl_malloc(tag, strlen(source) + 1);
	if (!destinationPtr)
	{
		avExitOnError("%s: pbl_errno = %d, message='%s'\n", tag, pbl_errno, pbl_errstr);
	}

	while (*sourcePtr)
	{
		if (*sourcePtr == '+')
		{
			*destinationPtr++ = ' ';
			sourcePtr++;
			continue;
		}

		if ((*sourcePtr == '%') && *(sourcePtr + 1) && *(sourcePtr + 2)
				&& isxdigit((int )(*(sourcePtr + 1))) && isxdigit( (int)(*(sourcePtr + 2))))
		{
			avStrNCpy(buffer, sourcePtr + 1, 3);
			buffer[2] = '\0';
#ifdef WIN32
			sscanf_s(buffer, "%x", &i);
#else
			sscanf(buffer, "%x", &i);
#endif
			*destinationPtr++ = (char) avCheckChar(i);
			sourcePtr += 3;
		}
		else
		{
			*destinationPtr++ = avCheckChar(*sourcePtr++);
			continue;
		}
	}
	*destinationPtr = '\0';
	return (destination);
}

/**
 * Reads a two column file into a string map.
 *
 * The format is
 *
 *   # Comment
 *   key   value
 *
 *   If the parameter map is NULL, a new string map is created.
 *   The pointer to the map used is returned.
 *
 * @return PblMap * retPtr != NULL: The pointer to the map.
 */
PblMap * avFileToMap(PblMap * map, char * filePath)
{
	char * tag = "avFileToMap";

	if (!map)
	{
		if (!(map = pblMapNewHashMap()))
		{
			avExitOnError("Failed to create a map, pbl_errno = %d\n", pbl_errno);
		}
	}

	FILE * stream = avFopen(filePath, "r");
	char line[AV_MAX_LINE_LENGTH + 1];

	while ((fgets(line, sizeof(line) - 1, stream)))
	{
		char * ptr = line;

		while (*ptr && isspace(*ptr))
		{
			ptr++;
		}

		if (!*ptr)
		{
			continue;
		}

		if (*ptr == '#')
		{
			continue;
		}

		char * key = ptr;
		while (*ptr && (!isspace(*ptr)))
		{
			ptr++;
		}

		if (*ptr)
		{
			*ptr++ = '\0';
		}

		char * value = avTrim(ptr);

		if (pblMapAddStrStr(map, key, value) < 0)
		{
			avExitOnError("%s: pbl_errno = %d, message='%s'\n", tag, pbl_errno, pbl_errstr);
		}
	}
	fclose(stream);

	return (map);
}

static PblMap * queryMap = NULL;
static void avSetQueryValue(char * key, char * value)
{
	char * tag = "avSetQueryValue";

	if (!queryMap)
	{
		if (!(queryMap = pblMapNewHashMap()))
		{
			avExitOnError("Failed to create a map, pbl_errno = %d\n", pbl_errno);
		}
	}
	if (!key || !*key)
	{
		return;
	}
	if (!value)
	{
		value = "";
	}

	if (pblMapAddStrStr(queryMap, key, value) < 0)
	{
		avExitOnError("%s: pbl_errno = %d, message='%s'\n", tag, pbl_errno, pbl_errstr);
	}
	AV_TRACE("In %s=%s", key, value);
}

/**
 * Get the value for an iteration given for the key in the query.
 */
char * avQueryValueForIteration(char * key, int iteration)
{
	char * tag = "avQueryValueForIteration";

	if (!queryMap)
	{
		return "";
	}
	if (!key || !*key)
	{
		avExitOnError("%s: Empty key not allowed!\n", tag, pbl_errstr);
	}
	if (iteration >= 0)
	{
		char * iterationKey = avSprintf("%s_%d", key, iteration);
		char * value = pblMapGetStr(queryMap, iterationKey);
		PBL_FREE(iterationKey);
		return value ? value : "";
	}
	char * value = pblMapGetStr(queryMap, key);
	return value ? value : "";
}

/**
 * Get the value given for the key in the query.
 */
char * avQueryValue(char * key)
{
	return avQueryValueForIteration(key, -1);
}

static PblMap * valueMap = NULL;

PblMap * avValueMap()
{
	if (!valueMap)
	{
		if (!(valueMap = pblMapNewHashMap()))
		{
			avExitOnError("Failed to create value map, pbl_errno = %d\n", pbl_errno);
		}
	}
	return valueMap;
}
/**
 * Set a value for the given key.
 */
void avSetValue(char * key, char * value)
{
	avSetValueForIteration(key, value, -1);
}

/**
 * Set a value for the given key for a loop iteration.
 */
void avSetValueForIteration(char * key, char * value, int iteration)
{
	avSetValueToMap(key, value, iteration, avValueMap());
}

/**
 * Set a value for the given key for a loop iteration into a map.
 */
void avSetValueToMap(char * key, char * value, int iteration, PblMap * map)
{
	char * tag = "avSetValueToMap";

	if (!key || !*key)
	{
		return;
	}
	if (!value)
	{
		value = "";
	}
	if (iteration >= 0)
	{
		char * iteratedKey = avSprintf("%s_%d", key, iteration);

		if (pblMapAddStrStr(map, iteratedKey, value) < 0)
		{
			avExitOnError("%s: pbl_errno = %d, message='%s'\n", tag, pbl_errno, pbl_errstr);
		}
		if (map == valueMap)
		{
			AV_TRACE("Out %s=%s", iteratedKey, value);
		}

		PBL_FREE(iteratedKey);

		iteratedKey = avSprintf("IDX_%d", iteration);
		char * index = pblMapGetStr(map, iteratedKey);
		if (!index)
		{
			index = avSprintf("%d", iteration);
			if (pblMapAddStrStr(map, iteratedKey, index) < 0)
			{
				avExitOnError("%s: pbl_errno = %d, message='%s'\n", tag, pbl_errno, pbl_errstr);
			}
			if (map == valueMap)
			{
				AV_TRACE("Out %s=%s", iteratedKey, index);
			}

			PBL_FREE(index);
		}
		PBL_FREE(iteratedKey);
	}
	else
	{
		if (pblMapAddStrStr(map, key, value) < 0)
		{
			avExitOnError("%s: pbl_errno = %d, message='%s'\n", tag, pbl_errno, pbl_errstr);
		}
		if (map == valueMap)
		{
			AV_TRACE("Out %s=%s", key, value);
		}
	}
}

/**
 * Clear the value for the given key.
 */
void avUnSetValue(char * key)
{
	avUnSetValueForIteration(key, -1);
}

/**
 * Clear all values.
 */
void avClearValues()
{
	if (!valueMap)
	{
		return;
	}

	AV_TRACE("Out cleared");
	pblMapClear(valueMap);
}

/**
 * Clear the value for the given key for a loop iteration.
 */
void avUnSetValueForIteration(char * key, int iteration)
{
	if (!valueMap)
	{
		return;
	}
	avUnSetValueFromMap(key, iteration, valueMap);
}

/**
 * Clear the value for the given key for a loop iteration from a map.
 */
void avUnSetValueFromMap(char * key, int iteration, PblMap * map)
{
	if (!key || !*key)
	{
		return;
	}

	if (iteration >= 0)
	{
		char * iteratedKey = avSprintf("%s_%d", key, iteration);

		pblMapUnmapStr(map, iteratedKey);
		AV_TRACE("Del %s=", iteratedKey);
		PBL_FREE(iteratedKey);

	}
	else
	{
		pblMapUnmapStr(map, key);
		AV_TRACE("Del %s=", key);
	}
}

/**
 * Get the value for the given key.
 */
char * avValue(char * key)
{
	return avValueForIteration(key, -1);
}

static char * avDurationKey = AV_KEY_DURATION;

/**
 * Get the value for the given key for a loop iteration.
 */
char * avValueForIteration(char * key, int iteration)
{
	if (*key && *key == *avDurationKey && !strcmp(avDurationKey, key))
	{
		return avValueFromMap(key, iteration, valueMap);
	}

	if (!valueMap)
	{
		return NULL;
	}
	return avValueFromMap(key, iteration, valueMap);
}

/**
 * Get the value for the given key for a loop iteration from a map.
 */
char * avValueFromMap(char * key, int iteration, PblMap * map)
{
	char * tag = "avValueFromMap";

	if (*key && *key == *avDurationKey && !strcmp(avDurationKey, key))
	{
		struct timeval now;
		gettimeofday(&now, NULL);

		unsigned long duration = now.tv_sec * 1000000 + now.tv_usec;
		duration -= avStartTime.tv_sec * 1000000 + avStartTime.tv_usec;
		char * string = avSprintf("%lu", duration);
		AV_TRACE("Duration=%s microseconds", string);
		return string;
	}

	if (!key || !*key)
	{
		avExitOnError("%s: Empty key not allowed!\n", tag, pbl_errstr);
	}

	if (iteration >= 0)
	{
		char * iteratedKey = avSprintf("%s_%d", key, iteration);
		char * value = pblMapGetStr(map, iteratedKey);
		PBL_FREE(iteratedKey);
		return value;
	}

	return pblMapGetStr(map, key);
}

/**
 * Reads in GET or POST query data, converts it to non-escaped text,
 * and saves each parameter in the query map.
 *
 *  If there's no variables that indicates GET or POST, it is assumed
 *  that the CGI script was started from the command line, specifying
 *  the query string like
 *
 *      script 'key1=value1&key2=value2'
 *
 */
void avParseQuery(int argc, char * argv[])
{
	char * tag = "avParseQuery";

	char * ptr = NULL;
	char * queryString = NULL;

#ifdef WIN32
	_setmode (_fileno( stdin ), _O_BINARY);
	_setmode (_fileno( stdout ), _O_BINARY);
#endif

	ptr = avGetEnv("REQUEST_METHOD");
	if (!ptr || !*ptr)
	{
		if (argc < 2)
		{
			avExitOnError("%s: test usage: %s querystring\n", tag, argv[0]);
		}
		queryString = avStrDup(argv[1]);
	}
	else if (!strcmp(ptr, "GET"))
	{
		ptr = avGetEnv("QUERY_STRING");
		if (!ptr || !*ptr)
		{
			queryString = avStrDup("");
		}
		else
		{
			queryString = avStrDup(ptr);
		}
	}
	else if (!strcmp(ptr, "POST"))
	{
		size_t length = 0;

		ptr = avGetEnv("QUERY_STRING");
		if (!ptr || !*ptr)
		{
			queryString = avStrDup("");
		}
		else
		{
			queryString = avStrCat(ptr, "&");
			length = strlen(queryString);
		}

		ptr = avGetEnv("CONTENT_LENGTH");
		if (ptr && *ptr)
		{
			int contentLength = atoi(ptr);
			if (contentLength > 0)
			{
				if (length + contentLength >= AV_MAX_POST_INPUT_LEN)
				{
					avExitOnError("%s: POST input too long, %d bytes\n", tag, length + contentLength);
				}

				queryString = realloc(queryString, length + contentLength + 1);
				if (!queryString)
				{
					avExitOnError("%s: Out of memory\n", tag);
				}

				int rc;
				ptr = queryString + length;
				while (contentLength-- > 0)
				{
					if ((rc = getchar()) == EOF)
					{
						break;
					}
					*ptr++ = rc;
				}
				*ptr = '\0';
			}
		}
	}
	else
	{
		avExitOnError("%s: Unknown REQUEST_METHOD '%s'\n", tag, ptr);
	}

	AV_TRACE("In %s", queryString);

	char * keyValuePairs[AV_MAX_QUERY_PARAMETERS_COUNT + 1];
	char * keyValuePair[2 + 1];

	avSplit(queryString, "&", AV_MAX_QUERY_PARAMETERS_COUNT + 1, keyValuePairs);
	for (int i = 0; keyValuePairs[i]; i++)
	{
		avSplit(keyValuePairs[i], "=", 2, keyValuePair);
		if (keyValuePair[0] && keyValuePair[1] && !keyValuePair[2])
		{
			char * key = avDecode(keyValuePair[0]);
			char * value = avDecode(keyValuePair[1]);

			avSetQueryValue(avTrim(key), avTrim(value));

			PBL_FREE(key);
			PBL_FREE(value);
		}

		PBL_FREE(keyValuePair[0]);
		PBL_FREE(keyValuePair[1]);
		PBL_FREE(keyValuePairs[i]);
	}

	PBL_FREE(queryString);
}

static char * avReplaceLowerThan(char * string, char *ptr2)
{
	char * tag = "avReplaceLowerThan";

	char * ptr = string;

	PblStringBuilder * stringBuilder = pblStringBuilderNew();
	if (!stringBuilder)
	{
		avExitOnError("%s: pbl_errno = %d, message='%s'\n", tag, pbl_errno, pbl_errstr);
	}

	for (int i = 0; i < 1000000; i++)
	{
		size_t length = ptr2 - ptr;
		if (length > 0)
		{
			if (pblStringBuilderAppendStrN(stringBuilder, length, ptr) == ((size_t) -1))
			{
				avExitOnError("%s: pbl_errno = %d, message='%s'\n", tag, pbl_errno, pbl_errstr);
			}
		}

		if (pblStringBuilderAppendStr(stringBuilder, "&lt;") == ((size_t) -1))
		{
			avExitOnError("%s: pbl_errno = %d, message='%s'\n", tag, pbl_errno, pbl_errstr);
		}
		ptr = ptr2 + 1;
		ptr2 = strstr(ptr, "<");
		if (!ptr2)
		{
			if (pblStringBuilderAppendStr(stringBuilder, ptr) == ((size_t) -1))
			{
				avExitOnError("%s: pbl_errno = %d, message='%s'\n", tag, pbl_errno, pbl_errstr);
			}

			char * result = pblStringBuilderToString(stringBuilder);
			if (!result)
			{
				avExitOnError("%s: pbl_errno = %d, message='%s'\n", tag, pbl_errno, pbl_errstr);
			}
			pblStringBuilderFree(stringBuilder);
			return result;
		}
	}
	avExitOnError("%s: There are more than 1000000 '<' characters to be replaced in one string.\n", tag);
	return NULL;
}

static char * avReplaceVariable(char * string, int iteration)
{
	static char * tag = "avReplaceVariable";
	static char * startPattern = "<!--?";
	static char * startPattern2 = "<?";

	int startPatternLength;
	char * endPattern;
	int endPatternLength;

	char * ptr = strchr(string, '<');
	if (!ptr)
	{
		return string;
	}

	size_t length = ptr - string;

	char * ptr2 = strstr(ptr, startPattern);
	if (!ptr2)
	{
		ptr2 = strstr(ptr, startPattern2);
		if (!ptr2)
		{
			return string;
		}
		else
		{
			startPatternLength = 2;
			endPattern = ">";
			endPatternLength = 1;
		}
	}
	else
	{
		char * ptr3 = strstr(ptr, startPattern2);
		if (ptr3 && ptr3 < ptr2)
		{
			ptr2 = ptr3;
			startPatternLength = 2;
			endPattern = ">";
			endPatternLength = 1;
		}
		else
		{
			startPatternLength = 5;
			endPattern = "-->";
			endPatternLength = 3;
		}
	}

	PblStringBuilder * stringBuilder = pblStringBuilderNew();
	if (!stringBuilder)
	{
		avExitOnError("%s: pbl_errno = %d, message='%s'\n", tag, pbl_errno, pbl_errstr);
	}

	if (length > 0)
	{
		if (pblStringBuilderAppendStrN(stringBuilder, length, string) == ((size_t) -1))
		{
			avExitOnError("%s: pbl_errno = %d, message='%s'\n", tag, pbl_errno, pbl_errstr);
		}
	}

	char * result;

	for (int i = 0; i < 1000000; i++)
	{
		length = ptr2 - ptr;
		if (length > 0)
		{
			if (pblStringBuilderAppendStrN(stringBuilder, length, ptr) == ((size_t) -1))
			{
				avExitOnError("%s: pbl_errno = %d, message='%s'\n", tag, pbl_errno, pbl_errstr);
			}
		}

		ptr = ptr2 + startPatternLength;
		ptr2 = strstr(ptr, endPattern);
		if (!ptr2)
		{
			result = pblStringBuilderToString(stringBuilder);
			if (!result)
			{
				avExitOnError("%s: pbl_errno = %d, message='%s'\n", tag, pbl_errno, pbl_errstr);
			}
			pblStringBuilderFree(stringBuilder);
			return result;
		}

		char * key = avRangeDup(ptr, ptr2);
		char * value = NULL;
		if (iteration >= 0)
		{
			value = avValueForIteration(key, iteration);
		}
		if (!value)
		{
			value = avValue(key);
		}
		PBL_FREE(key);

		if (value && *value)
		{
			char * pointer = strstr(value, "<");
			if (!pointer)
			{
				if (pblStringBuilderAppendStr(stringBuilder, value) == ((size_t) -1))
				{
					avExitOnError("%s: pbl_errno = %d, message='%s'\n", tag, pbl_errno, pbl_errstr);
				}
			}
			else
			{
				value = avReplaceLowerThan(value, pointer);
				if (value && *value)
				{
					if (pblStringBuilderAppendStr(stringBuilder, value) == ((size_t) -1))
					{
						avExitOnError("%s: pbl_errno = %d, message='%s'\n", tag, pbl_errno, pbl_errstr);
					}
				}
				PBL_FREE(value);
			}
		}
		ptr = ptr2 + endPatternLength;
		ptr2 = strstr(ptr, startPattern);
		if (!ptr2)
		{
			ptr2 = strstr(ptr, startPattern2);
			if (!ptr2)
			{
				if (pblStringBuilderAppendStr(stringBuilder, ptr) == ((size_t) -1))
				{
					avExitOnError("%s: pbl_errno = %d, message='%s'\n", tag, pbl_errno, pbl_errstr);
				}
				result = pblStringBuilderToString(stringBuilder);
				if (!result)
				{
					avExitOnError("%s: pbl_errno = %d, message='%s'\n", tag, pbl_errno, pbl_errstr);
				}
				pblStringBuilderFree(stringBuilder);
				return result;
			}
			else
			{
				startPatternLength = 2;
				endPattern = ">";
				endPatternLength = 1;
			}
		}
		else
		{
			char * ptr3 = strstr(ptr, startPattern2);
			if (ptr3 && ptr3 < ptr2)
			{
				ptr2 = ptr3;
				startPatternLength = 2;
				endPattern = ">";
				endPatternLength = 1;
			}
			else
			{
				startPatternLength = 5;
				endPattern = "-->";
				endPatternLength = 3;
			}
		}
	}
	avExitOnError("%s: There are more than 1000000 variables defined in one string.\n", tag);
	return NULL;
}

static char * avPrintStr(char * string, FILE * outStream, int iteration);

static char * avSkip(char * string, char * skipKey, FILE * outStream, int iteration)
{
	string = avReplaceVariable(string, -1);

	for (;;)
	{
		char * ptr = strstr(string, "<!--#ENDIF");
		if (!ptr)
		{
			return skipKey;
		}
		ptr += 10;

		char *ptr2 = strstr(ptr, "-->");
		if (ptr2)
		{
			char * key = avRangeDup(ptr, ptr2);
			if (!strcmp(skipKey, key))
			{
				PBL_FREE(skipKey);
				PBL_FREE(key);
				return avPrintStr(ptr2 + 3, outStream, iteration);
			}

			PBL_FREE(key);
			string = ptr2 + 3;
			continue;
		}
		break;
	}
	return skipKey;
}

static char * avPrintStr(char * string, FILE * outStream, int iteration)
{
	string = avReplaceVariable(string, iteration);

	for (;;)
	{
		char * ptr = strstr(string, "<!--#");
		if (!ptr)
		{
			fputs(string, outStream);
			return NULL;
		}

		while (string < ptr)
		{
			fputc(*string++, outStream);
		}

		if (!memcmp(string, "<!--#IFDEF", 10))
		{
			ptr += 10;

			char *ptr2 = strstr(ptr, "-->");
			if (ptr2)
			{
				char * key = avRangeDup(ptr, ptr2);
				if (!avValue(key) && !avValueForIteration(key, iteration))
				{
					return avSkip(ptr2 + 1, key, outStream, iteration);
				}
				PBL_FREE(key);
				string = ptr2 + 3;
				continue;
			}
			return NULL;
		}
		if (!memcmp(string, "<!--#IFNDEF", 11))
		{
			ptr += 11;

			char *ptr2 = strstr(ptr, "-->");
			if (ptr2)
			{
				char * key = avRangeDup(ptr, ptr2);
				if (avValue(key) || avValueForIteration(key, iteration))
				{
					return avSkip(ptr2 + 1, key, outStream, iteration);
				}
				PBL_FREE(key);
				string = ptr2 + 3;
				continue;
			}
			return NULL;
		}
		if (!memcmp(string, "<!--#ENDIF", 10))
		{
			ptr += 7;

			char *ptr2 = strstr(ptr, "-->");
			if (ptr2)
			{
				string = ptr2 + 3;
				continue;
			}
			return NULL;
		}
		fputs(string, outStream);
		break;
	}
	return NULL;
}

static char * avHandleFor(PblList * list, char * line, char * forKey)
{
	char * tag = "avHandleFor";

	char * ptr = strstr(line, "<!--#");
	if (!ptr)
	{
		if (pblListAdd(list, avStrDup(line)) < 0)
		{
			avExitOnError("%s: pbl_errno = %d, message='%s'\n", tag, pbl_errno, pbl_errstr);
		}
		return forKey;
	}
	if (!memcmp(ptr, "<!--#ENDFOR", 11))
	{
		if (ptr > line)
		{
			int length = ptr - line;
			char * value = pbl_memdup(tag, line, length + 1);
			if (!value)
			{
				avExitOnError("%s: pbl_errno = %d, message='%s'\n", tag, pbl_errno, pbl_errstr);
			}
			value[length] = '\0';

			if (pblListAdd(list, value) < 0)
			{
				avExitOnError("%s: pbl_errno = %d, message='%s'\n", tag, pbl_errno, pbl_errstr);
			}
		}
		ptr += 11;

		char * ptr2 = strstr(ptr, "-->");
		if (ptr2)
		{
			char * key = avRangeDup(ptr, ptr2);
			if (!strcmp(forKey, key))
			{
				PBL_FREE(key);
				return NULL;
			}
			PBL_FREE(key);
		}
	}
	else
	{
		if (pblListAdd(list, avStrDup(line)) < 0)
		{
			avExitOnError("%s: pbl_errno = %d, message='%s'\n", tag, pbl_errno, pbl_errstr);
		}
	}
	return forKey;
}

static PblList * avReadFor(char * inputLine, char * forKey, FILE * stream)
{
	char * tag = "avReadFor";

	char line[AV_MAX_LINE_LENGTH + 1];

	PblList * list = pblListNewLinkedList();
	if (!list)
	{
		avExitOnError("%s: pbl_errno = %d, message='%s'\n", tag, pbl_errno, pbl_errstr);
	}

	if (!avStrIsNullOrWhiteSpace(inputLine))
	{
		if (!avHandleFor(list, inputLine, forKey))
		{
			return list;
		}
	}

	while ((fgets(line, sizeof(line) - 1, stream)))
	{
		if (!avHandleFor(list, line, forKey))
		{
			break;
		}
	}
	return list;
}

static void avPrintFor(PblList * lines, char * forKey, FILE * outStream)
{
	char * tag = "avPrintFor";

	int hasNext;

	for (unsigned long iteration = 0; 1; iteration++)
	{
		if (!avValueForIteration(forKey, iteration))
		{
			return;
		}

		char * skipKey = NULL;
		PblIterator iteratorBuffer;
		PblIterator * iterator = &iteratorBuffer;

		if (pblIteratorInit((PblCollection *) lines, iterator) < 0)
		{
			avExitOnError("%s: pbl_errno = %d, message='%s'\n", tag, pbl_errno, pbl_errstr);
		}
		while ((hasNext = pblIteratorHasNext(iterator)) > 0)
		{
			char * line = (char*) pblIteratorNext(iterator);
			if (line == (void*) -1)
			{
				avExitOnError("%s: pbl_errno = %d, message='%s'\n", tag, pbl_errno, pbl_errstr);
			}
			if (line)
			{
				char * ptr = strstr(line, "<!--#");
				if (!ptr)
				{
					if (!skipKey)
					{
						fputs(avReplaceVariable(line, iteration), outStream);
					}
					continue;
				}

				if (skipKey)
				{
					skipKey = avSkip(line, skipKey, outStream, iteration);
					continue;
				}
				skipKey = avPrintStr(line, outStream, iteration);
			}
		}
		if (hasNext < 0)
		{
			avExitOnError("%s: pbl_errno = %d, message='%s'\n", tag, pbl_errno, pbl_errstr);
		}
	}
}

/**
 * Print a template
 */
void avPrint(char * directory, char * fileName, char * contentType)
{
	char * tag = "avPrint";

	AV_TRACE("Directory=%s", directory);
	AV_TRACE("FileName=%s", fileName);
	AV_TRACE("ContentType=%s", contentType);

	char * filePath = avStrCat(directory, fileName);
	char * skipKey = NULL;
	FILE * stream = avFopen(filePath, "r");
	PBL_FREE(filePath);

	if (contentType)
	{
		avSetContentType(contentType);
	}

	char line[AV_MAX_LINE_LENGTH + 1];

	while ((fgets(line, sizeof(line) - 1, stream)))
	{
		char * start = line;
		char * ptr = strstr(line, "<!--#");
		if (!ptr)
		{
			if (!skipKey)
			{
				fputs(avReplaceVariable(line, -1), stdout);
			}
			continue;
		}

		if (skipKey)
		{
			skipKey = avSkip(line, skipKey, stdout, -1);
			continue;
		}

		if (!memcmp(ptr, "<!--#INCLUDE", 12))
		{
			while (start < ptr)
			{
				fputc(*start++, stdout);
			}
			ptr += 12;

			char * ptr2 = strstr(ptr, "-->");
			if (ptr2)
			{
				char * includeFileName = avRangeDup(ptr, ptr2);
				avPrint(directory, includeFileName, NULL);
				PBL_FREE(includeFileName);

				skipKey = avPrintStr(ptr2 + 3, stdout, -1);
			}
			continue;
		}
		if (!memcmp(ptr, "<!--#FOR", 8))
		{
			while (start < ptr)
			{
				fputc(*start++, stdout);
			}
			ptr += 8;

			char * ptr2 = strstr(ptr, "-->");
			if (ptr2)
			{
				char * forKey = avRangeDup(ptr, ptr2);
				PblList * lines = avReadFor(ptr2 + 3, forKey, stream);
				if (lines)
				{
					avPrintFor(lines, forKey, stdout);
					while (pblListSize(lines))
					{
						char * p = pblListPop(lines);
						if ((void*) -1 == p)
						{
							avExitOnError("%s: pbl_errno = %d, message='%s'\n", tag, pbl_errno, pbl_errstr);
						}
						PBL_FREE(p);
					}
					pblListFree(lines);
				}
				PBL_FREE(forKey);
			}
			continue;
		}
		while (start < ptr)
		{
			fputc(*start++, stdout);
		}
		skipKey = avPrintStr(ptr, stdout, -1);
	}
	fclose(stream);
}

/**
 * Create a new empty map
 */
PblMap * avNewMap()
{
	PblMap * map = pblMapNewHashMap();
	if (!map)
	{
		avExitOnError("Failed to create a map, pbl_errno = %d\n", pbl_errno);
	}
	return map;
}

/**
 * Test if a map is empty
 */
int avMapIsEmpty(PblMap * map)
{
	return 0 == pblMapSize(map);
}

/**
 * Release the memory used by a map
 */
void avMapFree(PblMap * map)
{
	if (map)
	{
		pblMapFree(map);
	}
}

/**
 * Get the values from the data of this string as a map.
 *
 * If an error occurs, the program exits with an error message.
 *
 * @return char * map: The map as malloced memory.
 */
PblMap * avDataValues(char * buffer)
{
	return avDataStrToMap(NULL, buffer);
}

/**
 * Update values from the data of the map.
 *
 * If an error occurs, the program exits with an error message.
 *
 * @return char * map: The map after the update as malloced memory.
 */
PblMap * avUpdateData(PblMap * map, char ** keys, char ** values)
{
	char * tag = "avUpdateData";

	if (!map)
	{
		map = avNewMap();
	}

	for (int i = 0; keys[i]; i++)
	{
		char * value = values[i];
		if (value == avValueIncrement)
		{
			value = pblMapGetStr(map, keys[i]);
			if (value && *value)
			{
				int n = atoi(value);
				int length = strlen(value) + 1;
				value = pbl_malloc0(tag, length + 2);
				snprintf(value, length + 1, "%d", ++n);
				if (pblMapAddStrStr(map, keys[i], value) < 0)
				{
					avExitOnError("%s: Failed to save string '%s', value '%s' in the map, pbl_errno %d\n", tag, keys[i],
							value, pbl_errno);
				}
				PBL_FREE(value);
			}
			else if (pblMapAddStrStr(map, keys[i], "1") < 0)
			{
				avExitOnError("%s: Failed to save string '%s' in the map, pbl_errno %d\n", tag, keys[i], pbl_errno);
			}
		}
		else if (value)
		{
			char * ptr = value;
			while ((ptr = strchr(ptr, '\t')))
			{
				*ptr++ = ' ';
			}

			if (pblMapAddStrStr(map, keys[i], value) < 0)
			{
				avExitOnError("%s: Failed to save string '%s' in the map, pbl_errno %d\n", tag, keys[i], pbl_errno);
			}
		}
		else
		{
			pblMapUnmapStr(map, keys[i]);
		}
	}
	return map;
}

/**
 * Write values of the map to a string.
 *
 * If an error occurs, the program exits with an error message.
 *
 * @return char * string: The string as malloced memory.
 */
char * avMapToDataStr(PblMap * map)
{
	char * tag = "avMapToDataStr";

	PblStringBuilder * stringBuilder = pblMapStrStrToStringBuilder(map, "\t", "=");
	if (!stringBuilder)
	{
		avExitOnError("%s: Failed to convert map to a string builder, pbl_errno %d\n", tag, pbl_errno);
	}

	char * data = pblStringBuilderToString(stringBuilder);
	if (!data)
	{
		avExitOnError("%s: Failed to convert string builder to a string, pbl_errno %d\n", tag, pbl_errno);
	}
	pblStringBuilderFree(stringBuilder);

	return data;
}

/**
 * Get the values from the data of this string to a map.
 *
 * If an error occurs, the program exits with an error message.
 *
 * @return char * map: The map as malloced memory.
 */
PblMap * avDataStrToMap(PblMap * map, char * buffer)
{
	char * tag = "avDataStrToMap";

	if (!map)
	{
		map = avNewMap();
	}

	char * keyValuePairs[AV_MAX_LINE_LENGTH + 1];
	char * keyValuePair[2 + 1];

	avSplit(buffer, "\t", AV_MAX_LINE_LENGTH, keyValuePairs);
	for (int i = 0; keyValuePairs[i]; i++)
	{
		avSplit(keyValuePairs[i], "=", 2, keyValuePair);
		if (keyValuePair[0] && keyValuePair[1] && !keyValuePair[2])
		{
			if (pblMapAddStrStr(map, keyValuePair[0], keyValuePair[1]) < 0)
			{
				avExitOnError("%s: Failed to save string '%s' in the map, pbl_errno %d\n", tag, keyValuePair[0],
						pbl_errno);
			}
		}

		PBL_FREE(keyValuePair[0]);
		PBL_FREE(keyValuePair[1]);
		PBL_FREE(keyValuePairs[i]);
	}
	return map;
}

#ifndef _ARVOS_CGI_H_
#define _ARVOS_CGI_H_
/*
 arvosCgi.h - include file for Common Gateway Interface functions for arvos directory service.

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

 $Log: arvosCgi.h,v $
 Revision 1.2  2016/12/03 00:03:54  peter
 Cleanup after tests

 */

#ifdef __cplusplus
extern "C"
{
#endif

#include <stdarg.h>
#include <errno.h>
#include <ctype.h>
#include <time.h>

#ifdef _WIN32

#include <winsock2.h>

#else
#include <sys/time.h>
#include <unistd.h>
#endif

#include "pbl.h"
#include "sqlite3.h"

/*****************************************************************************/
/* #defines                                                                  */
/*****************************************************************************/
#define AV_COOKIE                         "AV_COOKIE"
#define AV_COOKIE_PATH                    "AV_COOKIE_PATH"
#define AV_COOKIE_DOMAIN                  "AV_COOKIE_DOMAIN"

#define AV_KEY_DURATION                   "avDURATION"

#define AV_MAX_LINE_LENGTH			      (4 * 1024)

#define AV_TRACE if(avTraceFile) avTrace

/*****************************************************************************/
/* Variable declarations                                                     */
/*****************************************************************************/

extern FILE * avTraceFile;

extern sqlite3 * avSqliteDb;

extern char * avValueIncrement;

/*****************************************************************************/
/* Function declarations                                                     */
/*****************************************************************************/

extern void avSqlExec(char * statement, int (*callback)(void*, int, char**, char**), void * parameter);
extern int avCallbackCounter(void * ptr, int nColums, char ** values, char ** headers);
extern int avCallbackCellValue(void * ptr, int nColums, char ** values, char ** headers);
extern int avCallbackRowValues(void * ptr, int nColums, char ** values, char ** headers);
extern int avCallbackColumnValues(void * ptr, int nColums, char ** values, char ** headers);

extern void avInit(struct timeval * startTime, char * traceFilePath, char * databasePath);
extern void avTrace(const char * format, ...);

extern FILE * avFopen(char * traceFilePath, char * openType);
extern char * avGetEnv(char * name);
extern char * avGetCoockie();

extern void avExitOnError(const char * format, ...);

extern char * avSprintf(const char * format, ...);
extern int avStrArrayContains(char ** array, char * string);
extern char * avStrNCpy(char *dest, char *string, size_t n);
extern char * avTrim(char * string);
extern int avStrIsNullOrWhiteSpace(char * string);
extern char * avRangeDup(char * start, char * end);
extern char * avStrDup(char * string);
extern int avStrEquals(char * s1, char * s2);
extern int avStrCmp(char * s1, char * s2);
extern char * avStrCat(char * s1, char * s2);
extern char * avTimeToStr(time_t t);

extern int avSplit(char * string, char * splitString, size_t size, char * result[]);
extern PblList * avSplitToList(char * string, char * splitString);
extern char * avBufferToHexString(unsigned char * buffer, size_t length);
extern unsigned char * avRandomBytes(unsigned char * buffer, size_t length);

extern unsigned char * avSha256(unsigned char * buffer, size_t length);
extern char * avSha256AsHexString(unsigned char * buffer, size_t length);

extern PblMap * avNewMap();
extern int avMapIsEmpty(PblMap * map);
extern void avMapFree(PblMap * map);
extern PblMap * avFileToMap(PblMap * map, char * traceFilePath);

extern void avParseQuery(int argc, char * argv[]);
extern char * avQueryValue(char * key);
extern char * avQueryValueForIteration(char * key, int iteration);

extern PblMap * avValueMap();
extern void avSetValue(char * key, char * value);
extern void avSetValueForIteration(char * key, char * value, int iteration);
extern void avSetValueToMap(char * key, char * value, int iteration, PblMap * map);
extern void avUnSetValue(char * key);
extern void avUnSetValueForIteration(char * key, int iteration);
extern void avUnSetValueFromMap(char * key, int iteration, PblMap * map);
extern void avClearValues();
extern char * avValue(char * key);
extern char * avValueForIteration(char * key, int iteration);
extern char * avValueFromMap(char * key, int iteration, PblMap * map);

extern PblMap * avDataValues(char * buffer);
extern PblMap * avUpdateData(PblMap * map, char ** keys, char ** values);
extern char * avMapToDataStr(PblMap * map);
extern PblMap * avDataStrToMap(PblMap * map, char * buffer);

extern void avPrint(char * directory, char * fileName, char * contentType);

#ifdef __cplusplus
}
#endif

#endif

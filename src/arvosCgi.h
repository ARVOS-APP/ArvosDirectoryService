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
 Revision 1.5  2018/04/26 14:27:36  peter
 Working on the service

 Revision 1.4  2018/03/11 00:34:52  peter
 Integration with pbl cgi code.

 Revision 1.3  2016/12/13 21:47:21  peter
 Commit after Win32 port

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

#include "pblCgi.h"
#include "sqlite3.h"

/*****************************************************************************/
/* #defines                                                                  */
/*****************************************************************************/

#define PBL_CGI_COOKIE                         "PBL_CGI_COOKIE"
#define PBL_CGI_COOKIE_PATH                    "PBL_CGI_COOKIE_PATH"
#define PBL_CGI_COOKIE_DOMAIN                  "PBL_CGI_COOKIE_DOMAIN"

/*****************************************************************************/
/* Variable declarations                                                     */
/*****************************************************************************/

extern sqlite3 * avSqliteDb;

extern char * pblCgiCookieKey;
extern char * pblCgiCookieTag;

/*****************************************************************************/
/* Function declarations                                                     */
/*****************************************************************************/

extern void avSqlExec (sqlite3 * sqlliteDb, char * statement, int (*callback)(void*, int, char**, char**), void * parameter);
extern int avCallbackCounter(void * ptr, int nColums, char ** values, char ** headers);
extern int avCallbackCellValue(void * ptr, int nColums, char ** values, char ** headers);
extern int avCallbackRowValues(void * ptr, int nColums, char ** values, char ** headers);
extern int avCallbackColumnValues(void * ptr, int nColums, char ** values, char ** headers);

extern void avInit(char * databasePath);

extern unsigned char * avMallocRandomBytes(char * tag, size_t length);
extern unsigned char * avRandomBytes(unsigned char * buffer, size_t length);

extern unsigned char * avSha256(unsigned char * buffer, size_t length);
extern char * avSha256AsHexString(unsigned char * buffer, size_t length);

extern PblMap * avDataValues(char * buffer);
extern PblMap * avUpdateData(PblMap * map, char ** keys, char ** values);
extern char * avMapToDataStr(PblMap * map);
extern PblMap * avDataStrToMap(PblMap * map, char * buffer);

#ifdef __cplusplus
}
#endif

#endif

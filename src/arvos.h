#ifndef _ARVOS_H_
#define _ARVOS_H_
/*
 arvos.h - include file for arvos Common Gateway Interface - CGI - directory service.

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

 $Log: arvos.h,v $
 Revision 1.2  2018/03/11 00:34:52  peter
 Integration with pbl cgi code.

 Revision 1.1  2016/11/28 19:51:37  peter
 Initial

 */

#ifdef __cplusplus
extern "C"
{
#endif

#include <stdarg.h>
#include <errno.h>
#include <ctype.h>
#include <time.h>

#include "arvosCgi.h"

/*****************************************************************************/
/* #defines                                                                  */
/*****************************************************************************/

#define AV_DB_KEY_LENGTH                     256
#define AV_MAX_NAME_LENGTH                   64
#define AV_MAX_KEY_LENGTH                    240
#define AV_MAX_TEXT_LENGTH                   1024
#define AV_MAX_URL_LENGTH                    256

#define AV_TEMPLATE_DIRECTORY                "TemplateDirectory"
#define AV_DATABASE_DIRECTORY                "DataBaseDirectory"
#define AV_ADMINISTRATOR_NAMES               "AdministratorNames"

#define AV_NOT_ACTIVATED                     "Not activated"
#define AV_KEY_ADD_LOCATION                  "AddLocation"

#define AV_KEY_APPLY_FILTERS                 "ApplyFilters"
#define AV_KEY_USE_FILTERS                   "UseFilters"

#define AV_KEY_DELETE_ALLOWED                "DELETE_ALLOWED"
#define AV_KEY_EDIT_ALLOWED                  "EDIT_ALLOWED"

#define AV_KEY_INDEX                         "IDX"
#define AV_KEY_ACTION                        "ACT"

#define AV_KEY_ID                            "ID"
#define AV_KEY_VALUES                        "VALS"

#define AV_KEY_NAME                          "NAM"
#define AV_KEY_EMAIL                         "EML"
#define AV_KEY_EMAIL2                        "EML2"

#define AV_KEY_PASSWORD                      "PWD"
#define AV_KEY_PASSWORD2                     "PWD2"
#define AV_KEY_OLD_PASSWORD                  "OPWD"
#define AV_KEY_ACTIVATION_CODE               "ANC"
#define AV_KEY_COOKIE                        "COK"

#define AV_KEY_CONFIRM                       "CFM"
#define AV_KEY_DATA                          "DAT"
#define AV_KEY_DATA2                         "DAT2"

#define AV_KEY_COUNT                         "CNT"
#define AV_KEY_REPLY                         "RPL"

#define AV_KEY_TIME_CREATED                  "TCR"
#define AV_KEY_TIME_LAST_ACCESS              "TLA"
#define AV_KEY_TIME_ACTIVATED                "TAC"
#define AV_KEY_TIME_CONFIRMED                "TCF"

#define AV_KEY_CHANNEL                       "CHN"
#define AV_KEY_AUTHOR                        "AUT"
#define AV_KEY_DESCRIPTION                   "DES"
#define AV_KEY_URL                           "URL"
#define AV_KEY_THUMBNAIL                     "THB"
#define AV_KEY_HAS_THUMBNAIL                 "HAS_THB"
#define AV_KEY_INFORMATION                   "INF"
#define AV_KEY_HAS_INFORMATION               "HAS_INF"
#define AV_KEY_DEVELOPER_KEY                 "DEV"
#define AV_KEY_VERSION                       "VER"

#define AV_KEY_LOCATION                      "LOC"
#define AV_KEY_LOCATION_CHANNEL              "LCH"
#define AV_KEY_LAT                           "LAT"
#define AV_KEY_LON                           "LON"
#define AV_KEY_ALTITUDE                      "ALT"
#define AV_KEY_RADIUS                        "RAD"
#define AV_KEY_POSITION                      "POS"
#define AV_KEY_DISTANCE                      "DIS"

#define AV_KEY_FILTER_LAT                    "FLAT"
#define AV_KEY_FILTER_LON                    "FLON"
#define AV_KEY_FILTER_CHANNEL                "FCHN"
#define AV_KEY_FILTER_AUTHOR                 "FAUT"
#define AV_KEY_FILTER_DESCRIPTION            "FDES"
#define AV_KEY_FILTER_EMAIL                  "FEML"
#define AV_KEY_FILTER_DEVELOPER_KEY          "FDEV"

// These are set to the cgi variables during initialization
//
#define AV_KEY_USER_IS_LOGGED_IN             "USER_IS_LOGGED_IN"
#define AV_KEY_USER_IS_AUTHOR                "USER_IS_AUTHOR"
#define AV_KEY_USER_IS_ADMIN                 "USER_IS_ADMIN"
#define AV_KEY_USER_ID                       "USER_ID"
#define AV_KEY_USER_NAME                     "USER_NAME"
#define AV_KEY_USER_EMAIL                    "USER_EMAIL"

/*****************************************************************************/
/* Types defined                                                                  */
/*****************************************************************************/

/*****************************************************************************/
/* Global variables                                                          */
/*****************************************************************************/
extern char * avTemplateDirectory;

// These are set to the author name during initialization
//
extern char * avUserIsLoggedIn;
extern char * avUserIsAuthor;
extern char * avUserIsAdministrator;

extern char * avUserId;
extern char * avSessionId;

/*****************************************************************************/
/* Function declarations                                                     */
/*****************************************************************************/
extern char * avNowStr();
extern char * avRandomCode(size_t length);
extern char * avRandomIntCode(size_t length);
extern char * avRandomHexCode(size_t length);
extern void avSetAdministratorNames();
extern void avPrintTemplate(char * directory, char * fileName, char * contentType);

extern void avCheckCookie(char * cookie);
extern char * avCheckNameAndPassword(char * name, char * password);
extern char * avCheckNameAndPasswordAndLogin(char * name, char * password);
extern int avMapStrToValues(void * context, int index, void * element);
extern char * avHashPassword(char * password);

extern char * avDbAuthorInsert(char * name, char * email, char * password);
extern char * avDbAuthorDelete(char * id, char * name);
extern PblMap * avDbAuthorGet(char * id);
extern PblMap * avDbAuthorGetByName(char * name);
extern char * avDbAuthorUpdateValues(char * key, char * value, char ** updateKeys, char ** updateValues,
		char * returnKey);
extern int avDbAuthorsList(int offset, int n, char * authorFilter, char * emailFilter);
extern int avDbAuthorsListByTimeActivated(int offset, int n, char * timeActivated);
extern void avDbAuthorUpdateColumn(char * key, char * value, char * updateKey, char * updateValue);

extern char * avDbSessionInsert(char * authorId, char * name, char * email, char * timeActivated, char ** cookiePtr);
extern void avDbSessionDelete(char * id);
extern void avDbSessionDeleteByAuthor(char * authorId);
extern void avDbSessionDeleteByCookie(char * cookie);
extern PblMap * avDbSessionGet(char * id);
extern PblMap * avDbSessionGetByCookie(char * cookie);
extern int avDbSessionsList(int offset, int n);
extern void avDbSessionUpdateColumn(char * key, char * value, char * updateKey, char * updateValue);
extern char * avDbSessionUpdateValues(char * key, char * value, char ** updateKeys, char ** updateValues,
		char * returnKey);

extern char * avDbChannelInsert(char * name, char * author, char * description, char * developerKey);
extern PblMap * avDbChannelGet(char * id);
extern PblMap * avDbChannelGetByName(char * name);
extern void avDbChannelSetValuesForIteration(char * id, int iteration, char * location);
extern void avDbChannelSetMapForIteration(PblMap * map, int iteration, char * location);
extern void avDbChannelUpdateColumn(char * key, char * value, char * updateKey, char * updateValue);
extern char * avDbChannelUpdateValues(char * key, char * value, char ** updateKeys, char ** updateValues,
		char * returnKey);
extern int avDbChannelsListByName(int offset, int n);
extern int avDbChannelsListByAuthor(int offset, int n, char * author);
extern int avDbChannelsListByLocation(int offset, int n, char * lat, char * lon, char * authorFilter,
		char * channelFilter, char * descriptionFilter, char * developerKeyFilter);
extern void avDbChannelDelete(char * id);
extern void avDbChannelDeleteByAuthor(char * author);

extern char * avDbLocationInsert(char * channel, char * position);
extern PblMap * avDbLocationGet(char * id);
extern PblMap * avDbLocationGetByChannel(char * channel);
extern void avDbLocationDelete(char * id);
extern void avDbLocationDeleteByChannel(char * channel);
extern void avDbLocationSetValuesToMap(char * id, int iteration, PblMap * map);
extern void avDbLocationUpdateColumn(char * key, char * value, char * updateKey, char * updateValue);
extern char * avDbLocationUpdateValues(char * key, char * value, char ** updateKeys, char ** updateValues,
		char * returnKey);
extern int avDbLocationsListByChannel(PblMap * map, char * channel);

extern char * avAuthorCreate(char * name, char * email, char * password);
extern char * avSessionCreate(char * authorId, char * name, char * email, char * timeActivated);
extern char * avSessionDelete(char * id);
extern char * avSessionDeleteByCookie(char * cookie);
extern char * avSessionDeleteByAuthor(char * authorId);

extern char * avLocationSave(char * id, char * channel, char * lat, char * lon, char * rad, char * alt);

extern char * avGetRadius(char * rad, int * value);
extern char * avGetAltitude(char * alt, int * value);
extern char * avGetLatitude(char * lat, double * value);
extern char * avGetLongitude(char * lon, double * value);

extern int actionEditChannel();
extern int actionListChannels();
extern int actionListChannelsByAuthor();
extern int actionShowChannel();
extern int actionDeleteChannel();

#ifdef __cplusplus
}
#endif

#endif

/*
 avDirectoryService.c - main for arvos Common Gateway Interface - CGI - directory service.

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

 $Log: avDirectoryService.c,v $
 Revision 1.3  2016/12/13 21:47:21  peter
 Commit after Win32 port

 Revision 1.2  2016/12/01 20:59:09  peter
 Cleanup of gpl version 3 license

 Revision 1.1  2016/11/28 19:51:37  peter
 Initial

 */

/*
 * Make sure "strings <exe> | grep Id | sort -u" shows the source file versions
 */
char * avDirectoryService_c_id = "$Id: avDirectoryService.c,v 1.3 2016/12/13 21:47:21 peter Exp $";

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

static int actionFillDatabase()
{
	char * authors[0x1000];
	for (int i = 0; i < 0x1000; i++)
	{
		authors[i] = avRandomCode(10);

		if (avAuthorCreate(authors[i], avSprintf("%s@%s.com", avRandomCode(8), avRandomCode(7)), "11111111"))
		{
			i--;
			continue;
		}
	}

	for (int i = 0; i < 0x10000; i++)
	{
		char * id = avDbChannelInsert(avRandomCode(10), authors[i % 0x1000], avRandomCode(64), " ");

		char * updateKeys[] = { AV_KEY_TIME_CREATED, AV_KEY_URL, AV_KEY_THUMBNAIL, AV_KEY_INFORMATION,
		AV_KEY_VERSION, NULL };

		char * updateValues[] = { avNowStr(), "", "http://www.arvos-app.com/images/arvos_logo_rgb-weiss256.png",
				"http://mission-base.com/", "1", NULL };

		avDbChannelUpdateValues(AV_KEY_ID, id, updateKeys, updateValues, NULL);
	}

	for (int i = 0; i < 0x11000; i++)
	{
		avLocationSave("", avSprintf("%d", rand() % 10000), avSprintf("48.%s", avRandomIntCode(6)),
				avSprintf("11.%s", avRandomIntCode(6)), "10000", "100");
	}

	avPrintTemplate(avTemplateDirectory, "index.html", "text/html");
	return 0;
}

static int actionLogout()
{
	if (avUserIsLoggedIn)
	{
		char * cookie = avValue(AV_COOKIE);
		if (cookie && *cookie)
		{
			avSessionDeleteByCookie(cookie);
		}
	}
	avUserIsLoggedIn = NULL;
	avUserIsAuthor = NULL;
	avUserIsAdministrator = NULL;
	avUserId = NULL;
	avSessionId = NULL;

	avClearValues();
	avSetValue(AV_COOKIE, "X");
	avSetValue(AV_COOKIE_PATH, "/");
	avSetValue(AV_COOKIE_DOMAIN, avGetEnv("SERVER_NAME"));

	avPrintTemplate(avTemplateDirectory, "login.html", "text/html");

	return 0;
}

static void actionRegister()
{
	char * name = avQueryValue(AV_KEY_NAME);
	char * email = avQueryValue(AV_KEY_EMAIL);
	char * email2 = avQueryValue(AV_KEY_EMAIL2);
	char * password = avQueryValue(AV_KEY_PASSWORD);
	char * password2 = avQueryValue(AV_KEY_PASSWORD2);

	avSetValue(AV_KEY_NAME, name);
	avSetValue(AV_KEY_EMAIL, email);
	avSetValue(AV_KEY_EMAIL2, email2);

	if (!name || !*name)
	{
		if (avStrEquals("Yes", avQueryValue(AV_KEY_CONFIRM)))
		{
			avSetValue(AV_KEY_REPLY, "You need to enter an author name.");
		}
		avPrintTemplate(avTemplateDirectory, "register.html", "text/html");
	}
	if (strlen(name) > AV_MAX_NAME_LENGTH)
	{
		avSetValue(AV_KEY_REPLY, "The chosen author name is too long, it is longer than 64 characters.");
		avPrintTemplate(avTemplateDirectory, "register.html", "text/html");
	}
	int hasAlnum = 0;
	char * ptr;
	for (ptr = name; *ptr; ptr++)
	{
		if (*ptr == ' ')
		{
			continue;
		}
		if (isalnum(*ptr))
		{
			hasAlnum = 1;
			continue;
		}
		avSetValue(AV_KEY_REPLY, "The chosen author name can only contain alphanumeric characters.");
		avPrintTemplate(avTemplateDirectory, "register.html", "text/html");
	}
	if (!hasAlnum)
	{
		avSetValue(AV_KEY_REPLY, "The chosen author name must contain at least one alphanumeric character.");
		avPrintTemplate(avTemplateDirectory, "register.html", "text/html");
	}

	if (!email || !*email)
	{
		if (avStrEquals("Yes", avQueryValue(AV_KEY_CONFIRM)))
		{
			avSetValue(AV_KEY_REPLY, "You need to enter an email address.");
		}
		avPrintTemplate(avTemplateDirectory, "register.html", "text/html");
	}
	if (strlen(email) > AV_MAX_KEY_LENGTH)
	{
		avSetValue(AV_KEY_REPLY, "The chosen email address is too long.");
		avPrintTemplate(avTemplateDirectory, "register.html", "text/html");
	}

	if (!avStrEquals(email, email2))
	{
		if (avStrEquals("Yes", avQueryValue(AV_KEY_CONFIRM)))
		{
			avSetValue(AV_KEY_REPLY, "The two emails you entered differ.");
		}
		avPrintTemplate(avTemplateDirectory, "register.html", "text/html");
	}

	if (!password || strlen(password) < 8)
	{
		if (avStrEquals("Yes", avQueryValue(AV_KEY_CONFIRM)))
		{
			avSetValue(AV_KEY_REPLY, "The password you enter must be at least 8 characters long.");
		}
		avPrintTemplate(avTemplateDirectory, "register.html", "text/html");
	}

	if (!avStrEquals(password, password2))
	{
		if (avStrEquals("Yes", avQueryValue(AV_KEY_CONFIRM)))
		{
			avSetValue(AV_KEY_REPLY, "The two passwords you entered differ.");
		}
		avPrintTemplate(avTemplateDirectory, "register.html", "text/html");
	}

	char * response = avAuthorCreate(name, email, password);
	if (response)
	{
		avSetValue(AV_KEY_REPLY, response);
		avPrintTemplate(avTemplateDirectory, "register.html", "text/html");
	}

	if (!avUserIsLoggedIn)
	{
		avPrintTemplate(avTemplateDirectory, "register.html", "text/html");
	}

	avSetValue(AV_KEY_REPLY, "Your registration was successful.");
	avPrintTemplate(avTemplateDirectory, "registerComplete.html", "text/html");
}

static int actionActivate()
{
	char * name = avQueryValue(AV_KEY_NAME);
	char * activationCode = avQueryValue(AV_KEY_ACTIVATION_CODE);

	avSetValue(AV_KEY_NAME, name);
	avSetValue(AV_KEY_ACTIVATION_CODE, activationCode);

	if (!name || !*name)
	{
		if (avStrEquals("Yes", avQueryValue(AV_KEY_CONFIRM)))
		{
			avSetValue(AV_KEY_REPLY, "You need to enter an author name.");
		}
		avPrintTemplate(avTemplateDirectory, "activate.html", "text/html");
	}
	if (strlen(name) > AV_MAX_KEY_LENGTH)
	{
		avSetValue(AV_KEY_REPLY, "The name entered is too long.");
	}

	if (!activationCode || !*activationCode)
	{
		if (avStrEquals("Yes", avQueryValue(AV_KEY_CONFIRM)))
		{
			avSetValue(AV_KEY_REPLY, "You need to enter the activation code.");
		}
		avPrintTemplate(avTemplateDirectory, "activate.html", "text/html");
	}

	char * nowTimeStr = avNowStr();

	char *updateKeys[] = { AV_KEY_TIME_LAST_ACCESS, NULL };
	char *updateValues[] = { nowTimeStr, NULL };

	char * dbActivationCode = avDbAuthorUpdateValues(AV_KEY_NAME, name, updateKeys, updateValues,
	AV_KEY_ACTIVATION_CODE);

	if (!avStrEquals(activationCode, dbActivationCode))
	{
		sleep(3);
		avSetValue(AV_KEY_REPLY, "Bad activation code.");
		avPrintTemplate(avTemplateDirectory, "activate.html", "text/html");
	}

	avDbAuthorUpdateColumn(AV_KEY_NAME, name, AV_KEY_TIME_ACTIVATED, nowTimeStr);
	PBL_FREE(nowTimeStr);

	updateKeys[0] = AV_KEY_ACTIVATION_CODE;
	updateValues[0] = "";

	avDbAuthorUpdateValues(AV_KEY_NAME, name, updateKeys, updateValues, NULL);
	return 0;
}

static int actionConfirmAuthor()
{
	if (!avUserIsAdministrator)
	{
		avSetValue(AV_KEY_REPLY, avSprintf("You cannot confirm authors."));

		avPrintTemplate(avTemplateDirectory, "index.html", "text/html");
	}

	char * name = avQueryValue(AV_KEY_NAME);
	char * id = avQueryValue(AV_KEY_ID);
	if (!name || !*name || !id || !*id)
	{
		avPrintTemplate(avTemplateDirectory, "registrationList.html", "text/html");
	}

	char * activationCode = avRandomCode(12);
	char *updateKeys[] = { AV_KEY_TIME_CONFIRMED, AV_KEY_ACTIVATION_CODE, NULL };
	char *updateValues[] = { avNowStr(), activationCode, NULL };

	char * email = avDbAuthorUpdateValues(AV_KEY_ID, id, updateKeys, updateValues, AV_KEY_EMAIL);
	if (!email || !*email)
	{
		avPrintTemplate(avTemplateDirectory, "registrationList.html", "text/html");
	}

	avSetValue(AV_KEY_NAME, name);
	avSetValue(AV_KEY_EMAIL, email);
	avSetValue(AV_KEY_ACTIVATION_CODE, activationCode);

	avPrintTemplate(avTemplateDirectory, "confirmationMail.html", "text/html");
	return 0;
}

static int actionChangePassword()
{
	char * confirmation = avQueryValue(AV_KEY_CONFIRM);
	if (!confirmation || !*confirmation || !avStrEquals("Yes", confirmation))
	{
		avSetValue(AV_KEY_NAME, avUserIsLoggedIn);
		avPrintTemplate(avTemplateDirectory, "changePassword.html", "text/html");
		return 0;
	}

	char * name = avQueryValue(AV_KEY_NAME);
	char * oldPassword = avQueryValue(AV_KEY_OLD_PASSWORD);
	char * password = avQueryValue(AV_KEY_PASSWORD);
	char * password2 = avQueryValue(AV_KEY_PASSWORD2);

	if (!password || strlen(password) < 8)
	{
		avSetValue(AV_KEY_REPLY, "The password you enter must be at least 8 characters long.");
		avSetValue(AV_KEY_NAME, avUserIsLoggedIn);
		avPrintTemplate(avTemplateDirectory, "changePassword.html", "text/html");
		return 0;
	}

	if (!avStrEquals(password, password2))
	{
		avSetValue(AV_KEY_REPLY, "The two passwords you entered differ.");
		avSetValue(AV_KEY_NAME, avUserIsLoggedIn);
		avPrintTemplate(avTemplateDirectory, "changePassword.html", "text/html");
		return 0;
	}

	if (!avStrEquals(name, avUserIsLoggedIn) && !avUserIsAdministrator)
	{
		avSetValue(AV_KEY_REPLY, avSprintf("Change of password not allowed!"));
		avSetValue(AV_KEY_NAME, avUserIsLoggedIn);
		avPrintTemplate(avTemplateDirectory, "changePassword.html", "text/html");
		return 0;
	}

	if (!avUserIsAdministrator || avStrEquals(name, avUserIsLoggedIn))
	{
		char * message = avCheckNameAndPassword(name, oldPassword);
		if (message)
		{
			avSetValue(AV_KEY_REPLY, avSprintf("Old password did not match!"));

			avSetValue(AV_KEY_NAME, avUserIsLoggedIn);
			avPrintTemplate(avTemplateDirectory, "changePassword.html", "text/html");
			return 0;
		}
	}

	char *updateKeys[] = { AV_KEY_PASSWORD, NULL };
	char *updateValues[] = { avHashPassword(password), NULL };

	avDbAuthorUpdateValues(AV_KEY_NAME, name, updateKeys, updateValues, NULL);
	avSetValue(AV_KEY_REPLY, avSprintf("The password has been changed."));

	avSetValue(AV_KEY_NAME, avUserIsLoggedIn);
	avPrintTemplate(avTemplateDirectory, "changePassword.html", "text/html");
	return 0;
}

static int actionDeleteAuthor()
{
	char * confirmation = avQueryValue(AV_KEY_CONFIRM);
	if (confirmation && *confirmation && !avStrEquals("Yes", confirmation))
	{
		if (!avUserIsAdministrator)
		{
			avPrintTemplate(avTemplateDirectory, "index.html", "text/html");
		}
		avDbAuthorsList(0, 100, NULL, NULL);
		avPrintTemplate(avTemplateDirectory, "authorList.html", "text/html");
	}

	if (!confirmation || !*confirmation)
	{
		char * id = avQueryValue(AV_KEY_ID);
		char * name = avQueryValue(AV_KEY_AUTHOR);
		if (!name)
		{
			name = "";
		}
		if (!id || !*id)
		{
			if (!avUserIsAdministrator)
			{
				avPrintTemplate(avTemplateDirectory, "index.html", "text/html");
			}
			avDbAuthorsList(0, 100, NULL, NULL);
			avPrintTemplate(avTemplateDirectory, "authorList.html", "text/html");
		}
		if (!avUserIsAdministrator && !avStrEquals(id, avUserId))
		{
			avSetValue(AV_KEY_REPLY, avSprintf("You cannot delete the author '%s'!", name));

			avPrintTemplate(avTemplateDirectory, "index.html", "text/html");
		}

		if (avUserIsAdministrator)
		{
			avSetValue(AV_KEY_REPLY,
					avSprintf("Do you really want to delete the author with id '%s' and name '%s'?", id, name));
		}
		else
		{
			avSetValue(AV_KEY_REPLY, avSprintf("Do you really want to delete your author account?", name));
		}
		avSetValue(AV_KEY_DATA, id);
		avSetValue(AV_KEY_DATA2, name);
		avSetValue(AV_KEY_ACTION, "DeleteAuthor");
		avPrintTemplate(avTemplateDirectory, "confirm.html", "text/html");
	}

	char * id = avQueryValue(AV_KEY_DATA);
	char * name = avQueryValue(AV_KEY_DATA2);
	if (!name)
	{
		name = "";
	}
	if (!id || !*id)
	{
		if (!avUserIsAdministrator)
		{
			avPrintTemplate(avTemplateDirectory, "index.html", "text/html");
		}
		avDbAuthorsList(0, 100, NULL, NULL);
		avPrintTemplate(avTemplateDirectory, "authorList.html", "text/html");
	}

	if (!avUserIsAdministrator && !avStrEquals(id, avUserId))
	{
		avSetValue(AV_KEY_REPLY, avSprintf("You cannot delete the author '%s'!", name));

		avPrintTemplate(avTemplateDirectory, "index.html", "text/html");
	}

	char * message = avDbAuthorDelete(id, name);
	if (message)
	{
		avSetValue(AV_KEY_REPLY, message);
	}
	if (!avUserIsAdministrator)
	{
		actionLogout();
		exit(0);
	}

	avDbAuthorsList(0, 100, NULL, NULL);
	avPrintTemplate(avTemplateDirectory, "authorList.html", "text/html");
	return 0;
}

static int actionDeleteSession()
{
	if (!avUserIsAdministrator)
	{
		avPrintTemplate(avTemplateDirectory, "index.html", "text/html");
	}

	char * confirmation = avQueryValue(AV_KEY_CONFIRM);
	if (!confirmation || !*confirmation)
	{
		char * id = avQueryValue(AV_KEY_ID);
		if (!id || !*id)
		{
			avDbSessionsList(0, 10000);
			avPrintTemplate(avTemplateDirectory, "sessionList.html", "text/html");
		}

		avSetValue(AV_KEY_REPLY, avSprintf("Do you really want to delete the session '%s'?", id));
		avSetValue(AV_KEY_DATA, id);
		avSetValue(AV_KEY_ACTION, "DeleteSession");
		avPrintTemplate(avTemplateDirectory, "confirm.html", "text/html");
	}

	char * id = avQueryValue(AV_KEY_DATA);
	if (!avStrEquals("Yes", confirmation) || !id || !*id)
	{
		avDbSessionsList(0, 10000);
		avPrintTemplate(avTemplateDirectory, "sessionList.html", "text/html");
	}

	char * message = avSessionDelete(id);
	if (message)
	{
		avSetValue(AV_KEY_REPLY, message);
	}

	avDbSessionsList(0, 10000);
	avPrintTemplate(avTemplateDirectory, "sessionList.html", "text/html");
	return 0;
}

static void actionCheckLogin(int forceLogin)
{
	if (avUserIsLoggedIn)
	{
		return;
	}

	char * cookie = avGetCoockie();
	if (cookie && *cookie)
	{
		avCheckCookie(cookie);
	}

	if (!avUserIsLoggedIn && forceLogin)
	{
		char * password = avQueryValue(AV_KEY_PASSWORD);
		char * name = avQueryValue(AV_KEY_NAME);
		if (name && *name && password && *password)
		{
			char * message = avCheckNameAndPasswordAndLogin(name, password);
			if (message)
			{
				avSetValue(AV_KEY_REPLY, message);
				char * name = avQueryValue(AV_KEY_NAME);
				if (name && *name)
				{
					avSetValue(AV_KEY_NAME, name);
				}
				avPrintTemplate(avTemplateDirectory, "login.html", "text/html");
			}
		}
		else
		{
			char * name = avQueryValue(AV_KEY_NAME);
			if (name && *name)
			{
				avSetValue(AV_KEY_NAME, name);
			}
			avPrintTemplate(avTemplateDirectory, "login.html", "text/html");
		}
	}
}

static void actionListAuthors()
{
	char * filterAuthor = avQueryValue(AV_KEY_FILTER_AUTHOR);
	char * filterEmail = avQueryValue(AV_KEY_FILTER_EMAIL);

	avSetValue(AV_KEY_FILTER_AUTHOR, filterAuthor);
	avSetValue(AV_KEY_FILTER_EMAIL, filterEmail);

	avDbAuthorsList(0, 1000, filterAuthor, filterEmail);
	avPrintTemplate(avTemplateDirectory, "authorList.html", "text/html");
}

static void actionListRegistrations()
{
	avDbAuthorsListByTimeActivated(0, 10000, AV_NOT_ACTIVATED);
	avPrintTemplate(avTemplateDirectory, "registrationList.html", "text/html");
}

static void actionListSessions()
{
	avDbSessionsList(0, 10000);
	avPrintTemplate(avTemplateDirectory, "sessionList.html", "text/html");
}

int main(int argc, char * argv[])
{
	struct timeval startTime;
	gettimeofday(&startTime, NULL);

	avConfigMap = avFileToMap(NULL, "./config/arvosconfig.txt");

	avSetAdministratorNames();

	avTemplateDirectory = avConfigValue(AV_TEMPLATE_DIRECTORY, "../templates/");

	avInit(&startTime, avConfigValue(AV_TRACE_FILE, ""), avConfigValue(AV_DATABASE_DIRECTORY, "../database/"));

	avParseQuery(argc, argv);

	char * action = avQueryValue(AV_KEY_ACTION);

	actionCheckLogin(0);

	// Actions possible without a login
	//

	if (avStrEquals("Register", action))
	{
		actionRegister();
	}

	if (avStrEquals("Activate", action))
	{
		if (actionActivate())
		{
			return -1;
		}
	}

	if (avStrEquals("ListChannels", action))
	{
		return actionListChannels();
	}

	if (avStrEquals("ShowChannel", action))
	{
		return actionShowChannel();
	}

	actionCheckLogin(1);

	// Actions without login should go above

	if (!avUserIsLoggedIn)
	{
		avSetValue(AV_KEY_REPLY, "You need to log in in order to access this site.");
		char * name = avQueryValue(AV_KEY_NAME);
		if (name && *name)
		{
			avSetValue(AV_KEY_NAME, name);
		}
		avPrintTemplate(avTemplateDirectory, "login.html", "text/html");
	}

	// Actions possible with login

	if (avStrEquals("Logout", action))
	{
		return actionLogout();
	}

	if (avStrEquals("ChangePassword", action))
	{
		return actionChangePassword();
	}

	if (avStrEquals("DeleteAuthor", action))
	{
		return actionDeleteAuthor();
	}

	// Actions possible with user login should go above

	if (!avUserIsAuthor)
	{
		avPrintTemplate(avTemplateDirectory, "index.html", "text/html");
	}

	// Actions for authors

	if (avStrEquals("ListChannelsByAuthor", action))
	{
		return actionListChannelsByAuthor();
	}

	if (avStrEquals("EditChannel", action))
	{
		return actionEditChannel();
	}

	if (avStrEquals("DeleteChannel", action))
	{
		return actionDeleteChannel();
	}

	// Actions for authors go above

	if (!avUserIsAdministrator)
	{
		avPrintTemplate(avTemplateDirectory, "index.html", "text/html");
	}

	// Actions for administrators are below

	if (avStrEquals("ListAuthors", action))
	{
		actionListAuthors();
	}

	if (avStrEquals("ListRegistrations", action))
	{
		actionListRegistrations();
	}

	if (avStrEquals("ConfirmAuthor", action))
	{
		return actionConfirmAuthor();
	}

	if (avStrEquals("ListSessions", action))
	{
		actionListSessions();
	}

	if (avStrEquals("DeleteSession", action))
	{
		return actionDeleteSession();
	}

	if (avStrEquals("FillDatabase", action))
	{
		return actionFillDatabase();
	}

	avPrintTemplate(avTemplateDirectory, "index.html", "text/html");
	return 0;
}


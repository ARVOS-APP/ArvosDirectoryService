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
 Revision 1.4  2018/03/11 00:34:52  peter
 Integration with pbl cgi code.

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
char * avDirectoryService_c_id = "$Id: avDirectoryService.c,v 1.4 2018/03/11 00:34:52 peter Exp $";

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

		if (avAuthorCreate(authors[i], pblCgiSprintf("%s@%s.com", avRandomCode(8), avRandomCode(7)), "11111111"))
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
		avLocationSave("", pblCgiSprintf("%d", rand() % 10000), pblCgiSprintf("48.%s", avRandomIntCode(6)),
				pblCgiSprintf("11.%s", avRandomIntCode(6)), "10000", "100");
	}

	avPrintTemplate(avTemplateDirectory, "index.html", "text/html");
	return 0;
}

static int actionLogout()
{
	if (avUserIsLoggedIn)
	{
		char * cookie = pblCgiValue(AV_COOKIE);
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

	pblCgiClearValues();
	pblCgiSetValue(AV_COOKIE, "X");
	pblCgiSetValue(AV_COOKIE_PATH, "/");
	pblCgiSetValue(AV_COOKIE_DOMAIN, pblCgiGetEnv("SERVER_NAME"));

	avPrintTemplate(avTemplateDirectory, "login.html", "text/html");

	return 0;
}

static void actionRegister()
{
	char * name = pblCgiQueryValue(AV_KEY_NAME);
	char * email = pblCgiQueryValue(AV_KEY_EMAIL);
	char * email2 = pblCgiQueryValue(AV_KEY_EMAIL2);
	char * password = pblCgiQueryValue(AV_KEY_PASSWORD);
	char * password2 = pblCgiQueryValue(AV_KEY_PASSWORD2);

	pblCgiSetValue(AV_KEY_NAME, name);
	pblCgiSetValue(AV_KEY_EMAIL, email);
	pblCgiSetValue(AV_KEY_EMAIL2, email2);

	if (!name || !*name)
	{
		if (pblCgiStrEquals("Yes", pblCgiQueryValue(AV_KEY_CONFIRM)))
		{
			pblCgiSetValue(AV_KEY_REPLY, "You need to enter an author name.");
		}
		avPrintTemplate(avTemplateDirectory, "register.html", "text/html");
	}
	if (strlen(name) > AV_MAX_NAME_LENGTH)
	{
		pblCgiSetValue(AV_KEY_REPLY, "The chosen author name is too long, it is longer than 64 characters.");
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
		pblCgiSetValue(AV_KEY_REPLY, "The chosen author name can only contain alphanumeric characters.");
		avPrintTemplate(avTemplateDirectory, "register.html", "text/html");
	}
	if (!hasAlnum)
	{
		pblCgiSetValue(AV_KEY_REPLY, "The chosen author name must contain at least one alphanumeric character.");
		avPrintTemplate(avTemplateDirectory, "register.html", "text/html");
	}

	if (!email || !*email)
	{
		if (pblCgiStrEquals("Yes", pblCgiQueryValue(AV_KEY_CONFIRM)))
		{
			pblCgiSetValue(AV_KEY_REPLY, "You need to enter an email address.");
		}
		avPrintTemplate(avTemplateDirectory, "register.html", "text/html");
	}
	if (strlen(email) > AV_MAX_KEY_LENGTH)
	{
		pblCgiSetValue(AV_KEY_REPLY, "The chosen email address is too long.");
		avPrintTemplate(avTemplateDirectory, "register.html", "text/html");
	}

	if (!pblCgiStrEquals(email, email2))
	{
		if (pblCgiStrEquals("Yes", pblCgiQueryValue(AV_KEY_CONFIRM)))
		{
			pblCgiSetValue(AV_KEY_REPLY, "The two emails you entered differ.");
		}
		avPrintTemplate(avTemplateDirectory, "register.html", "text/html");
	}

	if (!password || strlen(password) < 8)
	{
		if (pblCgiStrEquals("Yes", pblCgiQueryValue(AV_KEY_CONFIRM)))
		{
			pblCgiSetValue(AV_KEY_REPLY, "The password you enter must be at least 8 characters long.");
		}
		avPrintTemplate(avTemplateDirectory, "register.html", "text/html");
	}

	if (!pblCgiStrEquals(password, password2))
	{
		if (pblCgiStrEquals("Yes", pblCgiQueryValue(AV_KEY_CONFIRM)))
		{
			pblCgiSetValue(AV_KEY_REPLY, "The two passwords you entered differ.");
		}
		avPrintTemplate(avTemplateDirectory, "register.html", "text/html");
	}

	char * response = avAuthorCreate(name, email, password);
	if (response)
	{
		pblCgiSetValue(AV_KEY_REPLY, response);
		avPrintTemplate(avTemplateDirectory, "register.html", "text/html");
	}

	if (!avUserIsLoggedIn)
	{
		avPrintTemplate(avTemplateDirectory, "register.html", "text/html");
	}

	pblCgiSetValue(AV_KEY_REPLY, "Your registration was successful.");
	avPrintTemplate(avTemplateDirectory, "registerComplete.html", "text/html");
}

static int actionActivate()
{
	char * name = pblCgiQueryValue(AV_KEY_NAME);
	char * activationCode = pblCgiQueryValue(AV_KEY_ACTIVATION_CODE);

	pblCgiSetValue(AV_KEY_NAME, name);
	pblCgiSetValue(AV_KEY_ACTIVATION_CODE, activationCode);

	if (!name || !*name)
	{
		if (pblCgiStrEquals("Yes", pblCgiQueryValue(AV_KEY_CONFIRM)))
		{
			pblCgiSetValue(AV_KEY_REPLY, "You need to enter an author name.");
		}
		avPrintTemplate(avTemplateDirectory, "activate.html", "text/html");
	}
	if (strlen(name) > AV_MAX_KEY_LENGTH)
	{
		pblCgiSetValue(AV_KEY_REPLY, "The name entered is too long.");
	}

	if (!activationCode || !*activationCode)
	{
		if (pblCgiStrEquals("Yes", pblCgiQueryValue(AV_KEY_CONFIRM)))
		{
			pblCgiSetValue(AV_KEY_REPLY, "You need to enter the activation code.");
		}
		avPrintTemplate(avTemplateDirectory, "activate.html", "text/html");
	}

	char * nowTimeStr = avNowStr();

	char *updateKeys[] = { AV_KEY_TIME_LAST_ACCESS, NULL };
	char *updateValues[] = { nowTimeStr, NULL };

	char * dbActivationCode = avDbAuthorUpdateValues(AV_KEY_NAME, name, updateKeys, updateValues,
	AV_KEY_ACTIVATION_CODE);

	if (!pblCgiStrEquals(activationCode, dbActivationCode))
	{
		sleep(3);
		pblCgiSetValue(AV_KEY_REPLY, "Bad activation code.");
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
		pblCgiSetValue(AV_KEY_REPLY, pblCgiSprintf("You cannot confirm authors."));

		avPrintTemplate(avTemplateDirectory, "index.html", "text/html");
	}

	char * name = pblCgiQueryValue(AV_KEY_NAME);
	char * id = pblCgiQueryValue(AV_KEY_ID);
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

	pblCgiSetValue(AV_KEY_NAME, name);
	pblCgiSetValue(AV_KEY_EMAIL, email);
	pblCgiSetValue(AV_KEY_ACTIVATION_CODE, activationCode);

	avPrintTemplate(avTemplateDirectory, "confirmationMail.html", "text/html");
	return 0;
}

static int actionChangePassword()
{
	char * confirmation = pblCgiQueryValue(AV_KEY_CONFIRM);
	if (!confirmation || !*confirmation || !pblCgiStrEquals("Yes", confirmation))
	{
		pblCgiSetValue(AV_KEY_NAME, avUserIsLoggedIn);
		avPrintTemplate(avTemplateDirectory, "changePassword.html", "text/html");
		return 0;
	}

	char * name = pblCgiQueryValue(AV_KEY_NAME);
	char * oldPassword = pblCgiQueryValue(AV_KEY_OLD_PASSWORD);
	char * password = pblCgiQueryValue(AV_KEY_PASSWORD);
	char * password2 = pblCgiQueryValue(AV_KEY_PASSWORD2);

	if (!password || strlen(password) < 8)
	{
		pblCgiSetValue(AV_KEY_REPLY, "The password you enter must be at least 8 characters long.");
		pblCgiSetValue(AV_KEY_NAME, avUserIsLoggedIn);
		avPrintTemplate(avTemplateDirectory, "changePassword.html", "text/html");
		return 0;
	}

	if (!pblCgiStrEquals(password, password2))
	{
		pblCgiSetValue(AV_KEY_REPLY, "The two passwords you entered differ.");
		pblCgiSetValue(AV_KEY_NAME, avUserIsLoggedIn);
		avPrintTemplate(avTemplateDirectory, "changePassword.html", "text/html");
		return 0;
	}

	if (!pblCgiStrEquals(name, avUserIsLoggedIn) && !avUserIsAdministrator)
	{
		pblCgiSetValue(AV_KEY_REPLY, pblCgiSprintf("Change of password not allowed!"));
		pblCgiSetValue(AV_KEY_NAME, avUserIsLoggedIn);
		avPrintTemplate(avTemplateDirectory, "changePassword.html", "text/html");
		return 0;
	}

	if (!avUserIsAdministrator || pblCgiStrEquals(name, avUserIsLoggedIn))
	{
		char * message = avCheckNameAndPassword(name, oldPassword);
		if (message)
		{
			pblCgiSetValue(AV_KEY_REPLY, pblCgiSprintf("Old password did not match!"));

			pblCgiSetValue(AV_KEY_NAME, avUserIsLoggedIn);
			avPrintTemplate(avTemplateDirectory, "changePassword.html", "text/html");
			return 0;
		}
	}

	char *updateKeys[] = { AV_KEY_PASSWORD, NULL };
	char *updateValues[] = { avHashPassword(password), NULL };

	avDbAuthorUpdateValues(AV_KEY_NAME, name, updateKeys, updateValues, NULL);
	pblCgiSetValue(AV_KEY_REPLY, pblCgiSprintf("The password has been changed."));

	pblCgiSetValue(AV_KEY_NAME, avUserIsLoggedIn);
	avPrintTemplate(avTemplateDirectory, "changePassword.html", "text/html");
	return 0;
}

static int actionDeleteAuthor()
{
	char * confirmation = pblCgiQueryValue(AV_KEY_CONFIRM);
	if (confirmation && *confirmation && !pblCgiStrEquals("Yes", confirmation))
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
		char * id = pblCgiQueryValue(AV_KEY_ID);
		char * name = pblCgiQueryValue(AV_KEY_AUTHOR);
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
		if (!avUserIsAdministrator && !pblCgiStrEquals(id, avUserId))
		{
			pblCgiSetValue(AV_KEY_REPLY, pblCgiSprintf("You cannot delete the author '%s'!", name));

			avPrintTemplate(avTemplateDirectory, "index.html", "text/html");
		}

		if (avUserIsAdministrator)
		{
			pblCgiSetValue(AV_KEY_REPLY,
					pblCgiSprintf("Do you really want to delete the author with id '%s' and name '%s'?", id, name));
		}
		else
		{
			pblCgiSetValue(AV_KEY_REPLY, pblCgiSprintf("Do you really want to delete your author account?", name));
		}
		pblCgiSetValue(AV_KEY_DATA, id);
		pblCgiSetValue(AV_KEY_DATA2, name);
		pblCgiSetValue(AV_KEY_ACTION, "DeleteAuthor");
		avPrintTemplate(avTemplateDirectory, "confirm.html", "text/html");
	}

	char * id = pblCgiQueryValue(AV_KEY_DATA);
	char * name = pblCgiQueryValue(AV_KEY_DATA2);
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

	if (!avUserIsAdministrator && !pblCgiStrEquals(id, avUserId))
	{
		pblCgiSetValue(AV_KEY_REPLY, pblCgiSprintf("You cannot delete the author '%s'!", name));

		avPrintTemplate(avTemplateDirectory, "index.html", "text/html");
	}

	char * message = avDbAuthorDelete(id, name);
	if (message)
	{
		pblCgiSetValue(AV_KEY_REPLY, message);
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

	char * confirmation = pblCgiQueryValue(AV_KEY_CONFIRM);
	if (!confirmation || !*confirmation)
	{
		char * id = pblCgiQueryValue(AV_KEY_ID);
		if (!id || !*id)
		{
			avDbSessionsList(0, 10000);
			avPrintTemplate(avTemplateDirectory, "sessionList.html", "text/html");
		}

		pblCgiSetValue(AV_KEY_REPLY, pblCgiSprintf("Do you really want to delete the session '%s'?", id));
		pblCgiSetValue(AV_KEY_DATA, id);
		pblCgiSetValue(AV_KEY_ACTION, "DeleteSession");
		avPrintTemplate(avTemplateDirectory, "confirm.html", "text/html");
	}

	char * id = pblCgiQueryValue(AV_KEY_DATA);
	if (!pblCgiStrEquals("Yes", confirmation) || !id || !*id)
	{
		avDbSessionsList(0, 10000);
		avPrintTemplate(avTemplateDirectory, "sessionList.html", "text/html");
	}

	char * message = avSessionDelete(id);
	if (message)
	{
		pblCgiSetValue(AV_KEY_REPLY, message);
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
		char * password = pblCgiQueryValue(AV_KEY_PASSWORD);
		char * name = pblCgiQueryValue(AV_KEY_NAME);
		if (name && *name && password && *password)
		{
			char * message = avCheckNameAndPasswordAndLogin(name, password);
			if (message)
			{
				pblCgiSetValue(AV_KEY_REPLY, message);
				char * name = pblCgiQueryValue(AV_KEY_NAME);
				if (name && *name)
				{
					pblCgiSetValue(AV_KEY_NAME, name);
				}
				avPrintTemplate(avTemplateDirectory, "login.html", "text/html");
			}
		}
		else
		{
			char * name = pblCgiQueryValue(AV_KEY_NAME);
			if (name && *name)
			{
				pblCgiSetValue(AV_KEY_NAME, name);
			}
			avPrintTemplate(avTemplateDirectory, "login.html", "text/html");
		}
	}
}

static void actionListAuthors()
{
	char * filterAuthor = pblCgiQueryValue(AV_KEY_FILTER_AUTHOR);
	char * filterEmail = pblCgiQueryValue(AV_KEY_FILTER_EMAIL);

	pblCgiSetValue(AV_KEY_FILTER_AUTHOR, filterAuthor);
	pblCgiSetValue(AV_KEY_FILTER_EMAIL, filterEmail);

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

	avConfigMap = pblCgiFileToMap(NULL, "./config/arvosconfig.txt");

	avSetAdministratorNames();

	avTemplateDirectory = avConfigValue(AV_TEMPLATE_DIRECTORY, "../templates/");

	avInit(&startTime, avConfigValue(PBL_CGI_TRACE_FILE, ""), avConfigValue(AV_DATABASE_DIRECTORY, "../database/"));

	pblCgiParseQuery(argc, argv);

	char * action = pblCgiQueryValue(AV_KEY_ACTION);

	actionCheckLogin(0);

	// Actions possible without a login
	//

	if (pblCgiStrEquals("Register", action))
	{
		actionRegister();
	}

	if (pblCgiStrEquals("Activate", action))
	{
		if (actionActivate())
		{
			return -1;
		}
	}

	if (pblCgiStrEquals("ListChannels", action))
	{
		return actionListChannels();
	}

	if (pblCgiStrEquals("ShowChannel", action))
	{
		return actionShowChannel();
	}

	actionCheckLogin(1);

	// Actions without login should go above

	if (!avUserIsLoggedIn)
	{
		pblCgiSetValue(AV_KEY_REPLY, "You need to log in in order to access this site.");
		char * name = pblCgiQueryValue(AV_KEY_NAME);
		if (name && *name)
		{
			pblCgiSetValue(AV_KEY_NAME, name);
		}
		avPrintTemplate(avTemplateDirectory, "login.html", "text/html");
	}

	// Actions possible with login

	if (pblCgiStrEquals("Logout", action))
	{
		return actionLogout();
	}

	if (pblCgiStrEquals("ChangePassword", action))
	{
		return actionChangePassword();
	}

	if (pblCgiStrEquals("DeleteAuthor", action))
	{
		return actionDeleteAuthor();
	}

	// Actions possible with user login should go above

	if (!avUserIsAuthor)
	{
		avPrintTemplate(avTemplateDirectory, "index.html", "text/html");
	}

	// Actions for authors

	if (pblCgiStrEquals("ListChannelsByAuthor", action))
	{
		return actionListChannelsByAuthor();
	}

	if (pblCgiStrEquals("EditChannel", action))
	{
		return actionEditChannel();
	}

	if (pblCgiStrEquals("DeleteChannel", action))
	{
		return actionDeleteChannel();
	}

	// Actions for authors go above

	if (!avUserIsAdministrator)
	{
		avPrintTemplate(avTemplateDirectory, "index.html", "text/html");
	}

	// Actions for administrators are below

	if (pblCgiStrEquals("ListAuthors", action))
	{
		actionListAuthors();
	}

	if (pblCgiStrEquals("ListRegistrations", action))
	{
		actionListRegistrations();
	}

	if (pblCgiStrEquals("ConfirmAuthor", action))
	{
		return actionConfirmAuthor();
	}

	if (pblCgiStrEquals("ListSessions", action))
	{
		actionListSessions();
	}

	if (pblCgiStrEquals("DeleteSession", action))
	{
		return actionDeleteSession();
	}

	if (pblCgiStrEquals("FillDatabase", action))
	{
		return actionFillDatabase();
	}

	avPrintTemplate(avTemplateDirectory, "index.html", "text/html");
	return 0;
}


/*
Augments.c - main for arvos (Common Gateway Interface - CGI) augments service.

Copyright (C) 2018   Tamiko Thiel and Peter Graf

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

$Log: Augments.c,v $
Revision 1.1  2018/04/29 18:42:08  peter
More work on service

*/

/*
* Make sure "strings <exe> | grep Id | sort -u" shows the source file versions
*/
char * Augments_c_id = "$Id: Augments.c,v 1.1 2018/04/29 18:42:08 peter Exp $";

#include <stdio.h>
#include <memory.h>

#ifndef __APPLE__
#include <malloc.h>
#endif

#include <assert.h>
#include <stdlib.h>

#include "arvos.h"

int main(int argc, char * argv[])
{
	struct timeval startTime;
	gettimeofday(&startTime, NULL);

	pblCgiConfigMap = pblCgiFileToMap(NULL, "../config/augmentsconfig.txt");

	avSetAdministratorNames();

	avTemplateDirectory = pblCgiConfigValue(AV_TEMPLATE_DIRECTORY, "../templates/");

	char * traceFile = pblCgiConfigValue(PBL_CGI_TRACE_FILE, "");
	pblCgiInitTrace(&startTime, traceFile);

	char * databaseDirectory = pblCgiConfigValue(AV_DATABASE_DIRECTORY, "../database/");
	avInit(databaseDirectory);

	pblCgiParseQuery(argc, argv);

	char * action = pblCgiQueryValue(AV_KEY_ACTION);

	//actionCheckLogin(0);

	// Actions possible without a login
	//

	if (pblCgiStrEquals("Register", action))
	{
		//actionRegister();
	}
	return 0;
}
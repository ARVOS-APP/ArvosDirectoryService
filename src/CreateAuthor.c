/*
CreateAuthor.c - main for adding an author and password to the author database.

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

$Log: CreateAuthor.c,v $
Revision 1.1  2018/04/27 16:21:03  peter
Added the tool to create an author


*/

/*
* Make sure "strings <exe> | grep Id | sort -u" shows the source file versions
*/
char * CreateAuthor_c_id = "$Id: CreateAuthor.c,v 1.1 2018/04/27 16:21:03 peter Exp $";

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
	if (argc != 4)
	{
		fprintf(stderr, "Usage %s AuthorName Password PasswordFile", argv[0]);
		exit(-1);
	}

	char * saltedHash = avHashPassword(argv[2]);
	FILE * passwordFile = pblCgiFopen(argv[3], "a+");

	fprintf(passwordFile, "%s\t%s\n", argv[1], saltedHash);

	return 0;
}
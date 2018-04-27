/*
 avChannel.c - channel handling for arvos CGI directory service.

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

 $Log: avChannel.c,v $
 Revision 1.4  2018/03/11 00:34:52  peter
 Integration with pbl cgi code.

 Revision 1.3  2016/12/13 21:47:21  peter
 Commit after Win32 port

 Revision 1.2  2016/11/28 19:54:16  peter
 Initial

 */

/*
 * Make sure "strings <exe> | grep Id | sort -u" shows the source file versions
 */
char * avChannel_c_id = "$Id: avChannel.c,v 1.4 2018/03/11 00:34:52 peter Exp $";

#include <stdio.h>
#include <memory.h>

#ifndef __APPLE__
#include <malloc.h>
#endif

#include <assert.h>
#include <stdlib.h>

#include "arvos.h"

/*****************************************************************************/
/* Global variables                                                          */
/*****************************************************************************/

static char * avChannelVersion = "1";

/*****************************************************************************/
/* Functions                                                                 */
/*****************************************************************************/

int actionEditChannel()
{
	char * id = pblCgiQueryValue(AV_KEY_ID);
	char * name = pblCgiQueryValue(AV_KEY_CHANNEL);
	char * description = pblCgiQueryValue(AV_KEY_DESCRIPTION);
	char * url = pblCgiQueryValue(AV_KEY_URL);
	char * thumbNail = pblCgiQueryValue(AV_KEY_THUMBNAIL);
	char * information = pblCgiQueryValue(AV_KEY_INFORMATION);
	char * developerKey = pblCgiQueryValue(AV_KEY_DEVELOPER_KEY);

	pblCgiSetValue(AV_KEY_ACTION, "EditChannel");
	pblCgiSetValue(AV_KEY_ID, id);
	pblCgiSetValue(AV_KEY_CHANNEL, name);
	pblCgiSetValue(AV_KEY_DESCRIPTION, description);
	pblCgiSetValue(AV_KEY_URL, url);
	pblCgiSetValue(AV_KEY_THUMBNAIL, thumbNail);
	if (!pblCgiStrIsNullOrWhiteSpace(thumbNail))
	{
		pblCgiSetValue(AV_KEY_HAS_THUMBNAIL, thumbNail);
	}
	pblCgiSetValue(AV_KEY_INFORMATION, information);
	if (!pblCgiStrIsNullOrWhiteSpace(information))
	{
		pblCgiSetValue(AV_KEY_HAS_INFORMATION, information);
	}
	pblCgiSetValue(AV_KEY_DEVELOPER_KEY, developerKey);

	if (*id)
	{
		if (strlen(id) > 17)
		{
			pblCgiSetValue(AV_KEY_REPLY, "The channel id given is too long, it is longer than 17 characters.");
			avPrintTemplate(avTemplateDirectory, "channel.html", "text/html");
		}

		PblMap * map = avDbChannelGet(id);
		if (!map)
		{
			pblCgiSetValue(AV_KEY_REPLY, pblCgiSprintf("Update failed, cannot find channel with id %s.", id));
			avPrintTemplate(avTemplateDirectory, "channel.html", "text/html");
		}

		if (!avUserIsAdministrator && !pblCgiStrEquals(avUserIsAuthor, pblMapGetStr(map, AV_KEY_AUTHOR)))
		{
			pblCgiSetValue(AV_KEY_REPLY,
					pblCgiSprintf("You cannot update channel with id %s, because you are not its author.", id));
			avPrintTemplate(avTemplateDirectory, "channel.html", "text/html");
		}

		pblCgiSetValue(AV_KEY_EDIT_ALLOWED, "Yes");
		pblCgiSetValue(AV_KEY_AUTHOR, pblMapGetStr(map, AV_KEY_AUTHOR));
		pblCgiMapFree(map);
	}
	else
	{
		pblCgiSetValue(AV_KEY_EDIT_ALLOWED, "Yes");
	}

	int deleted = 0;
	int iteration = 0;
	for (; 1; iteration++)
	{
		char * location = pblCgiQueryValueForIteration(AV_KEY_LOCATION, iteration);
		if (!location || !*location)
		{
			break;
		}
		char * lat = pblCgiQueryValueForIteration(AV_KEY_LAT, iteration);
		char * lon = pblCgiQueryValueForIteration(AV_KEY_LON, iteration);
		char * alt = pblCgiQueryValueForIteration(AV_KEY_ALTITUDE, iteration);
		char * rad = pblCgiQueryValueForIteration(AV_KEY_RADIUS, iteration);

		if (avGetRadius(rad, NULL))
		{
			if (strcmp("New", location))
			{
				avDbLocationDelete(location);
			}
			deleted++;
			continue;
		}
		pblCgiSetValueForIteration(AV_KEY_LOCATION, location, iteration - deleted);
		pblCgiSetValueForIteration(AV_KEY_LAT, lat, iteration - deleted);
		pblCgiSetValueForIteration(AV_KEY_LON, lon, iteration - deleted);
		pblCgiSetValueForIteration(AV_KEY_ALTITUDE, alt, iteration - deleted);
		pblCgiSetValueForIteration(AV_KEY_RADIUS, rad, iteration - deleted);
	}

	iteration -= deleted;

	char * addLocation = pblCgiQueryValue(AV_KEY_ADD_LOCATION);
	if (addLocation && *addLocation)
	{
		pblCgiSetValueForIteration(AV_KEY_LOCATION, "New", iteration);
		pblCgiSetValueForIteration(AV_KEY_LAT, "0", iteration);
		pblCgiSetValueForIteration(AV_KEY_LON, "0", iteration);
		pblCgiSetValueForIteration(AV_KEY_ALTITUDE, "0", iteration);
		pblCgiSetValueForIteration(AV_KEY_RADIUS, "0", iteration);

		avPrintTemplate(avTemplateDirectory, "channel.html", "text/html");
	}

	if (*id)
	{
		if (strlen(id) > 17)
		{
			pblCgiSetValue(AV_KEY_REPLY, "The channel id given is too long, it is longer than 17 characters.");
			avPrintTemplate(avTemplateDirectory, "channel.html", "text/html");
		}

		PblMap * map = avDbChannelGet(id);
		if (!map)
		{
			pblCgiSetValue(AV_KEY_REPLY, pblCgiSprintf("Update failed, cannot find channel with id %s.", id));
			avPrintTemplate(avTemplateDirectory, "channel.html", "text/html");
		}
		if (!avUserIsAdministrator && !pblCgiStrEquals(avUserIsAuthor, pblMapGetStr(map, AV_KEY_AUTHOR)))
		{
			pblCgiSetValue(AV_KEY_REPLY,
					pblCgiSprintf("You cannot update channel with id %s, because you are not its author.", id));
			avPrintTemplate(avTemplateDirectory, "channel.html", "text/html");
		}
		pblCgiSetValue(AV_KEY_EDIT_ALLOWED, "Yes");
		pblCgiSetValue(AV_KEY_AUTHOR, pblMapGetStr(map, AV_KEY_AUTHOR));
		pblCgiMapFree(map);
	}
	else
	{
		pblCgiSetValue(AV_KEY_EDIT_ALLOWED, "Yes");
	}

	if (!*name)
	{
		if (pblCgiStrEquals("Yes", pblCgiQueryValue(AV_KEY_CONFIRM)))
		{
			pblCgiSetValue(AV_KEY_REPLY, "You need to enter a channel name.");
		}
		avPrintTemplate(avTemplateDirectory, "channel.html", "text/html");
	}
	if (strlen(name) > AV_MAX_NAME_LENGTH)
	{
		pblCgiSetValue(AV_KEY_REPLY, "The channel name given is too long, it is longer than 64 characters.");
		avPrintTemplate(avTemplateDirectory, "channel.html", "text/html");
	}

	if (strlen(description) > AV_MAX_KEY_LENGTH)
	{
		pblCgiSetValue(AV_KEY_REPLY, "The description given is too long, it is longer than 240 characters.");
		avPrintTemplate(avTemplateDirectory, "channel.html", "text/html");
	}

	if (!*url)
	{
		if (pblCgiStrEquals("Yes", pblCgiQueryValue(AV_KEY_CONFIRM)))
		{
			pblCgiSetValue(AV_KEY_REPLY, "You need to enter the url to retrieve the augments.");
		}
		avPrintTemplate(avTemplateDirectory, "channel.html", "text/html");
	}
	if (strlen(url) > AV_MAX_URL_LENGTH)
	{
		pblCgiSetValue(AV_KEY_REPLY, "The url to retrieve the augments given is too long, it is longer than 256 characters.");
		avPrintTemplate(avTemplateDirectory, "channel.html", "text/html");
	}

	if (strlen(thumbNail) > AV_MAX_URL_LENGTH)
	{
		pblCgiSetValue(AV_KEY_REPLY, "The thumbnail url given is too long, it is longer than 256 characters.");
		avPrintTemplate(avTemplateDirectory, "channel.html", "text/html");
	}

	if (strlen(information) > AV_MAX_URL_LENGTH)
	{
		pblCgiSetValue(AV_KEY_REPLY, "The information url given is too long, it is longer than 256 characters.");
		avPrintTemplate(avTemplateDirectory, "channel.html", "text/html");
	}

	if (*id)
	{
		if (strlen(id) > 17)
		{
			pblCgiSetValue(AV_KEY_REPLY, "The channel id given is too long, it is longer than 17 characters.");
			avPrintTemplate(avTemplateDirectory, "channel.html", "text/html");
		}
		PblMap * map = avDbChannelGet(id);
		if (!map)
		{
			pblCgiSetValue(AV_KEY_REPLY, pblCgiSprintf("Update failed, cannot find channel with id %s.", id));
			avPrintTemplate(avTemplateDirectory, "channel.html", "text/html");
		}
		if (!avUserIsAdministrator && !pblCgiStrEquals(avUserIsAuthor, pblMapGetStr(map, AV_KEY_AUTHOR)))
		{
			pblCgiSetValue(AV_KEY_REPLY,
					pblCgiSprintf("You cannot update channel with id %s, because you are not its author.", id));
			avPrintTemplate(avTemplateDirectory, "channel.html", "text/html");
		}
		avDbChannelUpdateColumn(AV_KEY_ID, id, AV_KEY_CHANNEL, name);
		avDbChannelUpdateColumn(AV_KEY_ID, id, AV_KEY_DESCRIPTION, *description ? description : " ");
		avDbChannelUpdateColumn(AV_KEY_ID, id, AV_KEY_DEVELOPER_KEY, *developerKey ? developerKey : " ");
		pblCgiMapFree(map);
	}
	else
	{
		PblMap * map = avDbChannelGetByName(name);
		if (map)
		{
			pblCgiMapFree(map);
			pblCgiSetValue(AV_KEY_REPLY,
					pblCgiSprintf("There is already a channel with name '%s', please enter a different name.", name));
			avPrintTemplate(avTemplateDirectory, "channel.html", "text/html");
		}

		id = avDbChannelInsert(name, avUserIsAuthor, *description ? description : " ",
				*developerKey ? developerKey : " ");
		pblCgiSetValue(AV_KEY_ID, id);
		pblCgiSetValue(AV_KEY_EDIT_ALLOWED, "Yes");
		pblCgiSetValue(AV_KEY_AUTHOR, avUserIsAuthor);
	}

	char * updateKeys[] =
			{ AV_KEY_TIME_CREATED, AV_KEY_URL, AV_KEY_THUMBNAIL, AV_KEY_INFORMATION, AV_KEY_VERSION, NULL };

	char * updateValues[] = { avNowStr(), url, thumbNail, information, avChannelVersion, NULL };

	avDbChannelUpdateValues(AV_KEY_ID, id, updateKeys, updateValues, NULL);

	for (iteration = 0; 1; iteration++)
	{
		char * location = pblCgiValueForIteration(AV_KEY_LOCATION, iteration);
		if (!location || !*location)
		{
			break;
		}

		if (!strcmp(location, "New"))
		{
			location = "";
		}

		char * lat = pblCgiValueForIteration(AV_KEY_LAT, iteration);
		char * lon = pblCgiValueForIteration(AV_KEY_LON, iteration);
		char * alt = pblCgiValueForIteration(AV_KEY_ALTITUDE, iteration);
		char * rad = pblCgiValueForIteration(AV_KEY_RADIUS, iteration);

		char * message = avLocationSave(location, id, lat, lon, rad, alt);
		if (message)
		{
			pblCgiSetValue(AV_KEY_REPLY, message);
			avPrintTemplate(avTemplateDirectory, "channel.html", "text/html");
		}
	}

	avDbChannelSetValuesForIteration(id, -1, NULL);

	pblCgiSetValue(AV_KEY_REPLY, "The values of the channel were successfully saved.");
	avPrintTemplate(avTemplateDirectory, "channel.html", "text/html");
	return 0;
}

int actionListChannelsByAuthor()
{
	avDbChannelsListByAuthor(0, 100, avUserIsAuthor);

	avPrintTemplate(avTemplateDirectory, "channelList.html", "text/html");
	return 0;
}

int actionListChannels()
{
	char * filterLat = "";
	char * filterLon = "";
	char * filterChannel = "";
	char * filterAuthor = "";
	char * filterDescription = "";
	char * filterDeveloperKey = "";

	char * applyFilters = pblCgiQueryValue(AV_KEY_APPLY_FILTERS);
	if (applyFilters && *applyFilters)
	{
		filterLat = pblCgiQueryValue(AV_KEY_FILTER_LAT);
		filterLon = pblCgiQueryValue(AV_KEY_FILTER_LON);
		filterChannel = pblCgiQueryValue(AV_KEY_FILTER_CHANNEL);
		filterAuthor = pblCgiQueryValue(AV_KEY_FILTER_AUTHOR);
		filterDescription = pblCgiQueryValue(AV_KEY_FILTER_DESCRIPTION);
		filterDeveloperKey = pblCgiQueryValue(AV_KEY_FILTER_DEVELOPER_KEY);

		pblCgiSetValue(AV_KEY_FILTER_LAT, filterLat);
		pblCgiSetValue(AV_KEY_FILTER_LON, filterLon);
		pblCgiSetValue(AV_KEY_FILTER_CHANNEL, filterChannel);
		pblCgiSetValue(AV_KEY_FILTER_AUTHOR, filterAuthor);
		pblCgiSetValue(AV_KEY_FILTER_DESCRIPTION, filterDescription);
		pblCgiSetValue(AV_KEY_FILTER_DEVELOPER_KEY, filterDeveloperKey);

		if (avSessionId)
		{
			char * updateKeys[] = { AV_KEY_FILTER_LAT, AV_KEY_FILTER_LON, AV_KEY_FILTER_CHANNEL, AV_KEY_FILTER_AUTHOR,
			AV_KEY_FILTER_DESCRIPTION, AV_KEY_FILTER_DEVELOPER_KEY, NULL };
			char * updateValues[] = { filterLat, filterLon, filterChannel, filterAuthor, filterDescription,
					filterDeveloperKey, NULL };

			avDbSessionUpdateValues(AV_KEY_ID, avSessionId, updateKeys, updateValues, NULL);
		}
	}
	else
	{
		char * useFilters = pblCgiQueryValue(AV_KEY_USE_FILTERS);
		if (useFilters && *useFilters && avSessionId)
		{
			filterLat = pblCgiValue(AV_KEY_FILTER_LAT);
			filterLon = pblCgiValue(AV_KEY_FILTER_LON);
			filterChannel = pblCgiValue(AV_KEY_FILTER_CHANNEL);
			filterAuthor = pblCgiValue(AV_KEY_FILTER_AUTHOR);
			filterDescription = pblCgiValue(AV_KEY_FILTER_DESCRIPTION);
			filterDeveloperKey = pblCgiValue(AV_KEY_FILTER_DEVELOPER_KEY);
		}
	}

	if (*filterLat || *filterLon || *filterChannel || *filterAuthor || *filterDescription || *filterDeveloperKey)
	{
		avDbChannelsListByLocation(0, 100, filterLat, filterLon, filterAuthor, filterChannel, filterDescription,
				filterDeveloperKey);
	}
	else
	{
		avDbChannelsListByName(0, 100);
	}

	avPrintTemplate(avTemplateDirectory, "channelList.html", "text/html");
	return 0;
}

int actionShowChannel()
{
	char * id = pblCgiQueryValue(AV_KEY_ID);
	if (!id || !*id)
	{
		return actionListChannels();
	}

	PblMap * map = avDbChannelGet(id);
	if (!map)
	{
		return actionListChannels();
	}

	avDbChannelSetMapForIteration(map, -1, NULL);

	char * thumbNail = pblCgiValue(AV_KEY_THUMBNAIL);
	if (!pblCgiStrIsNullOrWhiteSpace(thumbNail))
	{
		pblCgiSetValue(AV_KEY_HAS_THUMBNAIL, thumbNail);
	}
	char * information = pblCgiValue(AV_KEY_INFORMATION);
	if (!pblCgiStrIsNullOrWhiteSpace(information))
	{
		pblCgiSetValue(AV_KEY_HAS_INFORMATION, information);
	}

	pblCgiMapFree(map);

	pblCgiSetValue(AV_KEY_ACTION, "EditChannel");
	avPrintTemplate(avTemplateDirectory, "channel.html", "text/html");
	return 0;
}

int actionDeleteChannel()
{
	char * confirmation = pblCgiQueryValue(AV_KEY_CONFIRM);
	if (confirmation && *confirmation && !pblCgiStrEquals("Yes", confirmation))
	{
		return actionListChannels();
	}

	if (!confirmation || !*confirmation)
	{
		char * id = pblCgiQueryValue(AV_KEY_ID);
		char * name = pblCgiQueryValue(AV_KEY_CHANNEL);
		if (!name)
		{
			name = "";
		}
		if (!id || !*id)
		{
			return actionListChannels();
		}
		PblMap * map = avDbChannelGet(id);
		if (!map)
		{
			return actionListChannels();
		}

		if (!avUserIsAdministrator && !pblCgiStrEquals(avUserIsAuthor, pblMapGetStr(map, AV_KEY_AUTHOR)))
		{
			pblCgiSetValue(AV_KEY_REPLY,
					pblCgiSprintf("You cannot delete the channel '%s', because you are not its author!", name));

			avPrintTemplate(avTemplateDirectory, "index.html", "text/html");
		}

		pblCgiSetValue(AV_KEY_REPLY,
				pblCgiSprintf("Do you really want to delete the channel with id '%s' and name '%s'?", id, name));

		pblCgiSetValue(AV_KEY_DATA, id);
		pblCgiSetValue(AV_KEY_DATA2, name);
		pblCgiSetValue(AV_KEY_ACTION, "DeleteChannel");
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
		return actionListChannels();
	}
	PblMap * map = avDbChannelGet(id);
	if (!map)
	{
		return actionListChannels();
	}

	if (!avUserIsAdministrator && !pblCgiStrEquals(avUserIsAuthor, pblMapGetStr(map, AV_KEY_AUTHOR)))
	{
		pblCgiSetValue(AV_KEY_REPLY,
				pblCgiSprintf("You cannot delete the channel '%s', because you are not its author!", name));

		avPrintTemplate(avTemplateDirectory, "index.html", "text/html");
	}
	pblCgiMapFree(map);

	avDbChannelDelete(id);
	avDbLocationDeleteByChannel(id);
	return actionListChannels();
}


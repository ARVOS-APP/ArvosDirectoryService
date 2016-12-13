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
 Revision 1.3  2016/12/13 21:47:21  peter
 Commit after Win32 port

 Revision 1.2  2016/11/28 19:54:16  peter
 Initial

 */

/*
 * Make sure "strings <exe> | grep Id | sort -u" shows the source file versions
 */
char * avChannel_c_id = "$Id: avChannel.c,v 1.3 2016/12/13 21:47:21 peter Exp $";

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
	char * id = avQueryValue(AV_KEY_ID);
	char * name = avQueryValue(AV_KEY_CHANNEL);
	char * description = avQueryValue(AV_KEY_DESCRIPTION);
	char * url = avQueryValue(AV_KEY_URL);
	char * thumbNail = avQueryValue(AV_KEY_THUMBNAIL);
	char * information = avQueryValue(AV_KEY_INFORMATION);
	char * developerKey = avQueryValue(AV_KEY_DEVELOPER_KEY);

	avSetValue(AV_KEY_ACTION, "EditChannel");
	avSetValue(AV_KEY_ID, id);
	avSetValue(AV_KEY_CHANNEL, name);
	avSetValue(AV_KEY_DESCRIPTION, description);
	avSetValue(AV_KEY_URL, url);
	avSetValue(AV_KEY_THUMBNAIL, thumbNail);
	if (!avStrIsNullOrWhiteSpace(thumbNail))
	{
		avSetValue(AV_KEY_HAS_THUMBNAIL, thumbNail);
	}
	avSetValue(AV_KEY_INFORMATION, information);
	if (!avStrIsNullOrWhiteSpace(information))
	{
		avSetValue(AV_KEY_HAS_INFORMATION, information);
	}
	avSetValue(AV_KEY_DEVELOPER_KEY, developerKey);

	if (*id)
	{
		if (strlen(id) > 17)
		{
			avSetValue(AV_KEY_REPLY, "The channel id given is too long, it is longer than 17 characters.");
			avPrintTemplate(avTemplateDirectory, "channel.html", "text/html");
		}

		PblMap * map = avDbChannelGet(id);
		if (!map)
		{
			avSetValue(AV_KEY_REPLY, avSprintf("Update failed, cannot find channel with id %s.", id));
			avPrintTemplate(avTemplateDirectory, "channel.html", "text/html");
		}

		if (!avUserIsAdministrator && !avStrEquals(avUserIsAuthor, pblMapGetStr(map, AV_KEY_AUTHOR)))
		{
			avSetValue(AV_KEY_REPLY,
					avSprintf("You cannot update channel with id %s, because you are not its author.", id));
			avPrintTemplate(avTemplateDirectory, "channel.html", "text/html");
		}

		avSetValue(AV_KEY_EDIT_ALLOWED, "Yes");
		avSetValue(AV_KEY_AUTHOR, pblMapGetStr(map, AV_KEY_AUTHOR));
		avMapFree(map);
	}
	else
	{
		avSetValue(AV_KEY_EDIT_ALLOWED, "Yes");
	}

	int deleted = 0;
	int iteration = 0;
	for (; 1; iteration++)
	{
		char * location = avQueryValueForIteration(AV_KEY_LOCATION, iteration);
		if (!location || !*location)
		{
			break;
		}
		char * lat = avQueryValueForIteration(AV_KEY_LAT, iteration);
		char * lon = avQueryValueForIteration(AV_KEY_LON, iteration);
		char * alt = avQueryValueForIteration(AV_KEY_ALTITUDE, iteration);
		char * rad = avQueryValueForIteration(AV_KEY_RADIUS, iteration);

		if (avGetRadius(rad, NULL))
		{
			if (strcmp("New", location))
			{
				avDbLocationDelete(location);
			}
			deleted++;
			continue;
		}
		avSetValueForIteration(AV_KEY_LOCATION, location, iteration - deleted);
		avSetValueForIteration(AV_KEY_LAT, lat, iteration - deleted);
		avSetValueForIteration(AV_KEY_LON, lon, iteration - deleted);
		avSetValueForIteration(AV_KEY_ALTITUDE, alt, iteration - deleted);
		avSetValueForIteration(AV_KEY_RADIUS, rad, iteration - deleted);
	}

	iteration -= deleted;

	char * addLocation = avQueryValue(AV_KEY_ADD_LOCATION);
	if (addLocation && *addLocation)
	{
		avSetValueForIteration(AV_KEY_LOCATION, "New", iteration);
		avSetValueForIteration(AV_KEY_LAT, "0", iteration);
		avSetValueForIteration(AV_KEY_LON, "0", iteration);
		avSetValueForIteration(AV_KEY_ALTITUDE, "0", iteration);
		avSetValueForIteration(AV_KEY_RADIUS, "0", iteration);

		avPrintTemplate(avTemplateDirectory, "channel.html", "text/html");
	}

	if (*id)
	{
		if (strlen(id) > 17)
		{
			avSetValue(AV_KEY_REPLY, "The channel id given is too long, it is longer than 17 characters.");
			avPrintTemplate(avTemplateDirectory, "channel.html", "text/html");
		}

		PblMap * map = avDbChannelGet(id);
		if (!map)
		{
			avSetValue(AV_KEY_REPLY, avSprintf("Update failed, cannot find channel with id %s.", id));
			avPrintTemplate(avTemplateDirectory, "channel.html", "text/html");
		}
		if (!avUserIsAdministrator && !avStrEquals(avUserIsAuthor, pblMapGetStr(map, AV_KEY_AUTHOR)))
		{
			avSetValue(AV_KEY_REPLY,
					avSprintf("You cannot update channel with id %s, because you are not its author.", id));
			avPrintTemplate(avTemplateDirectory, "channel.html", "text/html");
		}
		avSetValue(AV_KEY_EDIT_ALLOWED, "Yes");
		avSetValue(AV_KEY_AUTHOR, pblMapGetStr(map, AV_KEY_AUTHOR));
		avMapFree(map);
	}
	else
	{
		avSetValue(AV_KEY_EDIT_ALLOWED, "Yes");
	}

	if (!*name)
	{
		if (avStrEquals("Yes", avQueryValue(AV_KEY_CONFIRM)))
		{
			avSetValue(AV_KEY_REPLY, "You need to enter a channel name.");
		}
		avPrintTemplate(avTemplateDirectory, "channel.html", "text/html");
	}
	if (strlen(name) > AV_MAX_NAME_LENGTH)
	{
		avSetValue(AV_KEY_REPLY, "The channel name given is too long, it is longer than 64 characters.");
		avPrintTemplate(avTemplateDirectory, "channel.html", "text/html");
	}

	if (strlen(description) > AV_MAX_KEY_LENGTH)
	{
		avSetValue(AV_KEY_REPLY, "The description given is too long, it is longer than 240 characters.");
		avPrintTemplate(avTemplateDirectory, "channel.html", "text/html");
	}

	if (strlen(url) > AV_MAX_URL_LENGTH)
	{
		avSetValue(AV_KEY_REPLY, "The url given is too long, it is longer than 256 characters.");
		avPrintTemplate(avTemplateDirectory, "channel.html", "text/html");
	}

	if (strlen(thumbNail) > AV_MAX_URL_LENGTH)
	{
		avSetValue(AV_KEY_REPLY, "The thumbnail url given is too long, it is longer than 256 characters.");
		avPrintTemplate(avTemplateDirectory, "channel.html", "text/html");
	}

	if (strlen(information) > AV_MAX_URL_LENGTH)
	{
		avSetValue(AV_KEY_REPLY, "The information url given is too long, it is longer than 256 characters.");
		avPrintTemplate(avTemplateDirectory, "channel.html", "text/html");
	}

	if (*id)
	{
		if (strlen(id) > 17)
		{
			avSetValue(AV_KEY_REPLY, "The channel id given is too long, it is longer than 17 characters.");
			avPrintTemplate(avTemplateDirectory, "channel.html", "text/html");
		}
		PblMap * map = avDbChannelGet(id);
		if (!map)
		{
			avSetValue(AV_KEY_REPLY, avSprintf("Update failed, cannot find channel with id %s.", id));
			avPrintTemplate(avTemplateDirectory, "channel.html", "text/html");
		}
		if (!avUserIsAdministrator && !avStrEquals(avUserIsAuthor, pblMapGetStr(map, AV_KEY_AUTHOR)))
		{
			avSetValue(AV_KEY_REPLY,
					avSprintf("You cannot update channel with id %s, because you are not its author.", id));
			avPrintTemplate(avTemplateDirectory, "channel.html", "text/html");
		}
		avDbChannelUpdateColumn(AV_KEY_ID, id, AV_KEY_CHANNEL, name);
		avDbChannelUpdateColumn(AV_KEY_ID, id, AV_KEY_DESCRIPTION, *description ? description : " ");
		avDbChannelUpdateColumn(AV_KEY_ID, id, AV_KEY_DEVELOPER_KEY, *developerKey ? developerKey : " ");
		avMapFree(map);
	}
	else
	{
		PblMap * map = avDbChannelGetByName(name);
		if (map)
		{
			avMapFree(map);
			avSetValue(AV_KEY_REPLY,
					avSprintf("There is already a channel with name '%s', please enter a different name.", name));
			avPrintTemplate(avTemplateDirectory, "channel.html", "text/html");
		}

		id = avDbChannelInsert(name, avUserIsAuthor, *description ? description : " ",
				*developerKey ? developerKey : " ");
		avSetValue(AV_KEY_ID, id);
		avSetValue(AV_KEY_EDIT_ALLOWED, "Yes");
		avSetValue(AV_KEY_AUTHOR, avUserIsAuthor);
	}

	char * updateKeys[] =
			{ AV_KEY_TIME_CREATED, AV_KEY_URL, AV_KEY_THUMBNAIL, AV_KEY_INFORMATION, AV_KEY_VERSION, NULL };

	char * updateValues[] = { avNowStr(), url, thumbNail, information, avChannelVersion, NULL };

	avDbChannelUpdateValues(AV_KEY_ID, id, updateKeys, updateValues, NULL);

	for (iteration = 0; 1; iteration++)
	{
		char * location = avValueForIteration(AV_KEY_LOCATION, iteration);
		if (!location || !*location)
		{
			break;
		}

		if (!strcmp(location, "New"))
		{
			location = "";
		}

		char * lat = avValueForIteration(AV_KEY_LAT, iteration);
		char * lon = avValueForIteration(AV_KEY_LON, iteration);
		char * alt = avValueForIteration(AV_KEY_ALTITUDE, iteration);
		char * rad = avValueForIteration(AV_KEY_RADIUS, iteration);

		char * message = avLocationSave(location, id, lat, lon, rad, alt);
		if (message)
		{
			avSetValue(AV_KEY_REPLY, message);
			avPrintTemplate(avTemplateDirectory, "channel.html", "text/html");
		}
	}

	avDbChannelSetValuesForIteration(id, -1, NULL);

	avSetValue(AV_KEY_REPLY, "The values of the channel were successfully saved.");
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

	char * applyFilters = avQueryValue(AV_KEY_APPLY_FILTERS);
	if (applyFilters && *applyFilters)
	{
		filterLat = avQueryValue(AV_KEY_FILTER_LAT);
		filterLon = avQueryValue(AV_KEY_FILTER_LON);
		filterChannel = avQueryValue(AV_KEY_FILTER_CHANNEL);
		filterAuthor = avQueryValue(AV_KEY_FILTER_AUTHOR);
		filterDescription = avQueryValue(AV_KEY_FILTER_DESCRIPTION);
		filterDeveloperKey = avQueryValue(AV_KEY_FILTER_DEVELOPER_KEY);

		avSetValue(AV_KEY_FILTER_LAT, filterLat);
		avSetValue(AV_KEY_FILTER_LON, filterLon);
		avSetValue(AV_KEY_FILTER_CHANNEL, filterChannel);
		avSetValue(AV_KEY_FILTER_AUTHOR, filterAuthor);
		avSetValue(AV_KEY_FILTER_DESCRIPTION, filterDescription);
		avSetValue(AV_KEY_FILTER_DEVELOPER_KEY, filterDeveloperKey);

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
		char * useFilters = avQueryValue(AV_KEY_USE_FILTERS);
		if (useFilters && *useFilters && avSessionId)
		{
			filterLat = avValue(AV_KEY_FILTER_LAT);
			filterLon = avValue(AV_KEY_FILTER_LON);
			filterChannel = avValue(AV_KEY_FILTER_CHANNEL);
			filterAuthor = avValue(AV_KEY_FILTER_AUTHOR);
			filterDescription = avValue(AV_KEY_FILTER_DESCRIPTION);
			filterDeveloperKey = avValue(AV_KEY_FILTER_DEVELOPER_KEY);
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
	char * id = avQueryValue(AV_KEY_ID);
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

	char * thumbNail = avValue(AV_KEY_THUMBNAIL);
	if (!avStrIsNullOrWhiteSpace(thumbNail))
	{
		avSetValue(AV_KEY_HAS_THUMBNAIL, thumbNail);
	}
	char * information = avValue(AV_KEY_INFORMATION);
	if (!avStrIsNullOrWhiteSpace(information))
	{
		avSetValue(AV_KEY_HAS_INFORMATION, information);
	}

	avMapFree(map);

	avSetValue(AV_KEY_ACTION, "EditChannel");
	avPrintTemplate(avTemplateDirectory, "channel.html", "text/html");
	return 0;
}

int actionDeleteChannel()
{
	char * confirmation = avQueryValue(AV_KEY_CONFIRM);
	if (confirmation && *confirmation && !avStrEquals("Yes", confirmation))
	{
		return actionListChannels();
	}

	if (!confirmation || !*confirmation)
	{
		char * id = avQueryValue(AV_KEY_ID);
		char * name = avQueryValue(AV_KEY_CHANNEL);
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

		if (!avUserIsAdministrator && !avStrEquals(avUserIsAuthor, pblMapGetStr(map, AV_KEY_AUTHOR)))
		{
			avSetValue(AV_KEY_REPLY,
					avSprintf("You cannot delete the channel '%s', because you are not its author!", name));

			avPrintTemplate(avTemplateDirectory, "index.html", "text/html");
		}

		avSetValue(AV_KEY_REPLY,
				avSprintf("Do you really want to delete the channel with id '%s' and name '%s'?", id, name));

		avSetValue(AV_KEY_DATA, id);
		avSetValue(AV_KEY_DATA2, name);
		avSetValue(AV_KEY_ACTION, "DeleteChannel");
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
		return actionListChannels();
	}
	PblMap * map = avDbChannelGet(id);
	if (!map)
	{
		return actionListChannels();
	}

	if (!avUserIsAdministrator && !avStrEquals(avUserIsAuthor, pblMapGetStr(map, AV_KEY_AUTHOR)))
	{
		avSetValue(AV_KEY_REPLY,
				avSprintf("You cannot delete the channel '%s', because you are not its author!", name));

		avPrintTemplate(avTemplateDirectory, "index.html", "text/html");
	}
	avMapFree(map);

	avDbChannelDelete(id);
	avDbLocationDeleteByChannel(id);
	return actionListChannels();
}


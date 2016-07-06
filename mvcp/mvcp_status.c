/*
 * mvcp_status.c -- Unit Status Handling
 * Copyright (C) 2002-2015 Meltytech, LLC
 * Author: Charles Yates <charles.yates@pandora.be>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

/* System header files */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Application header files */
#include "mvcp_status.h"
#include "mvcp_tokeniser.h"
#include "mvcp_util.h"

/** Parse a unit status string.
*/

void mvcp_status_parse( mvcp_status status, char *text )
{
	mvcp_tokeniser tokeniser = mvcp_tokeniser_init( );
	if ( mvcp_tokeniser_parse_new( tokeniser, text, " " ) == 17 )
	{
		status->unit = atoi( mvcp_tokeniser_get_string( tokeniser, 0 ) );
		strncpy( status->clip, mvcp_util_strip( mvcp_tokeniser_get_string( tokeniser, 2 ), '\"' ), sizeof( status->clip ) );
		status->position = atol( mvcp_tokeniser_get_string( tokeniser, 3 ) );
		status->speed = atoi( mvcp_tokeniser_get_string( tokeniser, 4 ) );
		status->fps = atof( mvcp_tokeniser_get_string( tokeniser, 5 ) );
		status->in = atol( mvcp_tokeniser_get_string( tokeniser, 6 ) );
		status->out = atol( mvcp_tokeniser_get_string( tokeniser, 7 ) );
		status->length = atol( mvcp_tokeniser_get_string( tokeniser, 8 ) );

		strncpy( status->tail_clip, mvcp_util_strip( mvcp_tokeniser_get_string( tokeniser, 9 ), '\"' ), sizeof( status->tail_clip ) );
		status->tail_position = atol( mvcp_tokeniser_get_string( tokeniser, 10 ) );
		status->tail_in = atol( mvcp_tokeniser_get_string( tokeniser, 11 ) );
		status->tail_out = atol( mvcp_tokeniser_get_string( tokeniser, 12 ) );
		status->tail_length = atol( mvcp_tokeniser_get_string( tokeniser, 13 ) );
		status->seek_flag = atoi( mvcp_tokeniser_get_string( tokeniser, 14 ) );
		status->generation = atoi( mvcp_tokeniser_get_string( tokeniser, 15 ) );
		status->clip_index = atoi( mvcp_tokeniser_get_string( tokeniser, 16 ) );

		if ( !strcmp( mvcp_tokeniser_get_string( tokeniser, 1 ), "unknown" ) )
			status->status = unit_unknown;
		else if ( !strcmp( mvcp_tokeniser_get_string( tokeniser, 1 ), "undefined" ) )
			status->status = unit_undefined;
		else if ( !strcmp( mvcp_tokeniser_get_string( tokeniser, 1 ), "offline" ) )
			status->status = unit_offline;
		else if ( !strcmp( mvcp_tokeniser_get_string( tokeniser, 1 ), "not_loaded" ) )
			status->status = unit_not_loaded;
		else if ( !strcmp( mvcp_tokeniser_get_string( tokeniser, 1 ), "stopped" ) )
			status->status = unit_stopped;
		else if ( !strcmp( mvcp_tokeniser_get_string( tokeniser, 1 ), "paused" ) )
			status->status = unit_paused;
		else if ( !strcmp( mvcp_tokeniser_get_string( tokeniser, 1 ), "playing" ) )
			status->status = unit_playing;
		else if ( !strcmp( mvcp_tokeniser_get_string( tokeniser, 1 ), "disconnected" ) )
			status->status = unit_disconnected;
	}
	else
	{
		memset( status, 0, sizeof( mvcp_status_t ) );
		fprintf( stderr, "Status thread changed?\n" );
	}
	mvcp_tokeniser_close( tokeniser );
}

/** Serialise a status into a string.
*/

char *mvcp_status_serialise( mvcp_status status, char *text, int length )
{
	const char *status_string = NULL;

	switch( status->status )
	{
		case unit_undefined:
			status_string = "undefined";
			break;

		case unit_offline:
			status_string = "offline";
			break;

		case unit_not_loaded:
			status_string = "not_loaded";
			break;

		case unit_stopped:
			status_string = "stopped";
			break;

		case unit_playing:
			status_string = "playing";
			break;

		case unit_unknown:
			status_string = "unknown";
			break;

		case unit_paused:
			status_string = "paused";
			break;

		case unit_disconnected:
			status_string = "disconnected";
			break;
	}

	snprintf( text, length, "%d %s \"%s\" %d %d %.2f %d %d %d \"%s\" %d %d %d %d %d %d %d\r\n",
							status->unit,
							status_string,
							status->clip,
							status->position, 
							status->speed,
							status->fps,
							status->in,
							status->out,
							status->length,
							status->tail_clip,
							status->tail_position, 
							status->tail_in,
							status->tail_out,
							status->tail_length,
							status->seek_flag,
							status->generation,
							status->clip_index );

	return text;
}

/** Compare two status codes for changes.
*/

int mvcp_status_compare( mvcp_status status1, mvcp_status status2 )
{
	return memcmp( status1, status2, sizeof( mvcp_status_t ) );
}

/** Copy status code info from dest to src.
*/

mvcp_status mvcp_status_copy( mvcp_status dest, mvcp_status src )
{
	return memcpy( dest, src, sizeof( mvcp_status_t ) );
}

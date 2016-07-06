/*
 * mvcp.c -- High Level Client API for Melted
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
#include <stdarg.h>

/* Application header files */
#include "mvcp.h"
#include "mvcp_tokeniser.h"
#include "mvcp_util.h"

/** Initialise the mvcp structure.
*/

mvcp mvcp_init( mvcp_parser parser )
{
	mvcp this = malloc( sizeof( mvcp_t ) );
	if ( this != NULL )
	{
		memset( this, 0, sizeof( mvcp_t ) );
		this->parser = parser;
	}
	return this;
}

/** Set the response structure associated to the last command.
*/

static void mvcp_set_last_response( mvcp this, mvcp_response response )
{
	if ( this != NULL )
	{
		if ( this->last_response != NULL )
			mvcp_response_close( this->last_response );
		this->last_response = response;
	}
}

/** Connect to the parser.
*/

mvcp_error_code mvcp_connect( mvcp this )
{
	mvcp_error_code error = mvcp_server_unavailable;
	mvcp_response response = mvcp_parser_connect( this->parser );
	if ( response != NULL )
	{
		mvcp_set_last_response( this, response );
		if ( mvcp_response_get_error_code( response ) == 100 )
			error = mvcp_ok;
	}
	return error;
}

/** Interpret a non-context sensitive error code.
*/

static mvcp_error_code mvcp_get_error_code( mvcp this, mvcp_response response )
{
	(void) this; // unused

	mvcp_error_code error = mvcp_server_unavailable;
	switch( mvcp_response_get_error_code( response ) )
	{
		case -1:
			error = mvcp_server_unavailable;
			break;
		case -2:
			error = mvcp_no_response;
			break;
		case 200:
		case 201:
		case 202:
			error = mvcp_ok;
			break;
		case 400:
			error = mvcp_invalid_command;
			break;
		case 401:
			error = mvcp_server_timeout;
			break;
		case 402:
			error = mvcp_missing_argument;
			break;
		case 403:
			error = mvcp_unit_unavailable;
			break;
		case 404:
			error = mvcp_invalid_file;
			break;
		default:
		case 500:
			error = mvcp_unknown_error;
			break;
	}
	return error;
}

/** Execute a command.
*/

mvcp_error_code mvcp_execute( mvcp this, size_t size, const char *format, ... )
{
	mvcp_error_code error = mvcp_server_unavailable;
	char *command = malloc( size );
	if ( this != NULL && command != NULL )
	{
		va_list list;
		va_start( list, format );
		if ( vsnprintf( command, size, format, list ) != 0 )
		{
			mvcp_response response = mvcp_parser_execute( this->parser, command );
			mvcp_set_last_response( this, response );
			error = mvcp_get_error_code( this, response );
		}
		else
		{
			error = mvcp_invalid_command;
		}
		va_end( list );
	}
	else
	{
		error = mvcp_malloc_failed;
	}
	free( command );
	return error;
}

/** Execute a command.
*/

mvcp_error_code mvcp_receive( mvcp this, char *doc, size_t size, const char *format, ... )
{
	mvcp_error_code error = mvcp_server_unavailable;
	char *command = malloc( size );
	if ( this != NULL && command != NULL )
	{
		va_list list;
		va_start( list, format );
		if ( vsnprintf( command, size, format, list ) != 0 )
		{
			mvcp_response response = mvcp_parser_received( this->parser, command, doc );
			mvcp_set_last_response( this, response );
			error = mvcp_get_error_code( this, response );
		}
		else
		{
			error = mvcp_invalid_command;
		}
		va_end( list );
	}
	else
	{
		error = mvcp_malloc_failed;
	}
	free( command );
	return error;
}

/** Execute a command.
*/

mvcp_error_code mvcp_push( mvcp this, mlt_service service, size_t size, const char *format, ... )
{
	mvcp_error_code error = mvcp_server_unavailable;
	char *command = malloc( size );
	if ( this != NULL && command != NULL )
	{
		va_list list;
		va_start( list, format );
		if ( vsnprintf( command, size, format, list ) != 0 )
		{
			mvcp_response response = mvcp_parser_push( this->parser, command, service );
			mvcp_set_last_response( this, response );
			error = mvcp_get_error_code( this, response );
		}
		else
		{
			error = mvcp_invalid_command;
		}
		va_end( list );
	}
	else
	{
		error = mvcp_malloc_failed;
	}
	free( command );
	return error;
}

/** Set a global property.
*/

mvcp_error_code mvcp_set( mvcp this, char *property, char *value )
{
	return mvcp_execute( this, 1024, "SET %s=%s", property, value );
}

/** Get a global property.
*/

mvcp_error_code mvcp_get( mvcp this, char *property, char *value, int length )
{
	mvcp_error_code error = mvcp_execute( this, 1024, "GET %s", property );
	if ( error == mvcp_ok )
	{
		mvcp_response response = mvcp_get_last_response( this );
		strncpy( value, mvcp_response_get_line( response, 1 ), length );
	}
	return error;
}

/** Run a script.
*/

mvcp_error_code mvcp_run( mvcp this, char *file )
{
	return mvcp_execute( this, 10240, "RUN \"%s\"", file );
}

/** Add a unit.
*/

mvcp_error_code mvcp_unit_add( mvcp this, char *guid, int *unit )
{
	mvcp_error_code error = mvcp_execute( this, 1024, "UADD %s", guid );
	if ( error == mvcp_ok )
	{
		int length = mvcp_response_count( this->last_response );
		char *line = mvcp_response_get_line( this->last_response, length - 1 );
		if ( line == NULL || sscanf( line, "U%d", unit ) != 1 )
			error = mvcp_unit_creation_failed;
	}
	else
	{
		if ( error == mvcp_unknown_error )
			error = mvcp_unit_creation_failed;
	}
	return error;
}

/** Load a file on the specified unit.
*/

mvcp_error_code mvcp_unit_load( mvcp this, int unit, char *file )
{
	return mvcp_execute( this, 10240, "LOAD U%d \"%s\"", unit, file );
}

static void mvcp_interpret_clip_offset( char *output, mvcp_clip_offset offset, int clip )
{
	switch( offset )
	{
		case mvcp_absolute:
			sprintf( output, "%d", clip );
			break;
		case mvcp_relative:
			if ( clip < 0 )
				sprintf( output, "%d", clip );
			else
				sprintf( output, "+%d", clip );
			break;
	}
}

/** Load a file on the specified unit with the specified in/out points.
*/

mvcp_error_code mvcp_unit_load_clipped( mvcp this, int unit, char *file, int32_t in, int32_t out )
{
	return mvcp_execute( this, 10240, "LOAD U%d \"%s\" %d %d", unit, file, in, out );
}

/** Load a file on the specified unit at the end of the current pump.
*/

mvcp_error_code mvcp_unit_load_back( mvcp this, int unit, char *file )
{
	return mvcp_execute( this, 10240, "LOAD U%d \"!%s\"", unit, file );
}

/** Load a file on the specified unit at the end of the pump with the specified in/out points.
*/

mvcp_error_code mvcp_unit_load_back_clipped( mvcp this, int unit, char *file, int32_t in, int32_t out )
{
	return mvcp_execute( this, 10240, "LOAD U%d \"!%s\" %d %d", unit, file, in, out );
}

/** Append a file on the specified unit.
*/

mvcp_error_code mvcp_unit_append( mvcp this, int unit, char *file, int32_t in, int32_t out )
{
	return mvcp_execute( this, 10240, "APND U%d \"%s\" %d %d", unit, file, in, out );
}

/** Push a service on to a unit.
*/

mvcp_error_code mvcp_unit_receive( mvcp this, int unit, char *command, char *doc )
{
	return mvcp_receive( this, doc, 10240, "PUSH U%d %s", unit, command );
}

/** Push a service on to a unit.
*/

mvcp_error_code mvcp_unit_push( mvcp this, int unit, char *command, mlt_service service )
{
	return mvcp_push( this, service, 10240, "PUSH U%d %s", unit, command );
}

/** Clean the unit - this function removes all but the currently playing clip.
*/

mvcp_error_code mvcp_unit_clean( mvcp this, int unit )
{
	return mvcp_execute( this, 1024, "CLEAN U%d", unit );
}

/** Clear the unit - this function removes all clips.
*/

mvcp_error_code mvcp_unit_clear( mvcp this, int unit )
{
	return mvcp_execute( this, 1024, "CLEAR U%d", unit );
}

/** Wipe the unit - this function removes all clips before the current one.
*/

mvcp_error_code mvcp_unit_wipe( mvcp this, int unit )
{
	return mvcp_execute( this, 1024, "WIPE U%d", unit );
}

/** Move clips on the units playlist.
*/

mvcp_error_code mvcp_unit_clip_move( mvcp this, int unit, mvcp_clip_offset src_offset, int src, mvcp_clip_offset dest_offset, int dest )
{
	char temp1[ 100 ];
	char temp2[ 100 ];
	mvcp_interpret_clip_offset( temp1, src_offset, src );
	mvcp_interpret_clip_offset( temp2, dest_offset, dest );
	return mvcp_execute( this, 1024, "MOVE U%d %s %s", unit, temp1, temp2 );
}

/** Remove clip at the specified position.
*/

mvcp_error_code mvcp_unit_clip_remove( mvcp this, int unit, mvcp_clip_offset offset, int clip )
{
	char temp[ 100 ];
	mvcp_interpret_clip_offset( temp, offset, clip );
	return mvcp_execute( this, 1024, "REMOVE U%d %s", unit, temp );
}

/** Remove the currently playing clip.
*/

mvcp_error_code mvcp_unit_remove_current_clip( mvcp this, int unit )
{
	return mvcp_execute( this, 1024, "REMOVE U%d", unit );
}

/** Insert clip at the specified position.
*/

mvcp_error_code mvcp_unit_clip_insert( mvcp this, int unit, mvcp_clip_offset offset, int clip, char *file, int32_t in, int32_t out )
{
	char temp[ 100 ];
	mvcp_interpret_clip_offset( temp, offset, clip );
	return mvcp_execute( this, 1024, "INSERT U%d \"%s\" %s %d %d", unit, file, temp, in, out );
}

/** Play the unit at normal speed.
*/

mvcp_error_code mvcp_unit_play( mvcp this, int unit )
{
	return mvcp_execute( this, 1024, "PLAY U%d 1000", unit );
}

/** Play the unit at specified speed.
*/

mvcp_error_code mvcp_unit_play_at_speed( mvcp this, int unit, int speed )
{
	return mvcp_execute( this, 10240, "PLAY U%d %d", unit, speed );
}

/** Stop playback on the specified unit.
*/

mvcp_error_code mvcp_unit_stop( mvcp this, int unit )
{
	return mvcp_execute( this, 1024, "STOP U%d", unit );
}

/** Pause playback on the specified unit.
*/

mvcp_error_code mvcp_unit_pause( mvcp this, int unit )
{
	return mvcp_execute( this, 1024, "PAUSE U%d", unit );
}

/** Rewind the specified unit.
*/

mvcp_error_code mvcp_unit_rewind( mvcp this, int unit )
{
	return mvcp_execute( this, 1024, "REW U%d", unit );
}

/** Fast forward the specified unit.
*/

mvcp_error_code mvcp_unit_fast_forward( mvcp this, int unit )
{
	return mvcp_execute( this, 1024, "FF U%d", unit );
}

/** Step by the number of frames on the specified unit.
*/

mvcp_error_code mvcp_unit_step( mvcp this, int unit, int32_t step )
{
	return mvcp_execute( this, 1024, "STEP U%d %d", unit, step );
}

/** Goto the specified frame on the specified unit.
*/

mvcp_error_code mvcp_unit_goto( mvcp this, int unit, int32_t position )
{
	return mvcp_execute( this, 1024, "GOTO U%d %d", unit, position );
}

/** Goto the specified frame in the clip on the specified unit.
*/

mvcp_error_code mvcp_unit_clip_goto( mvcp this, int unit, mvcp_clip_offset offset, int clip, int32_t position )
{
	char temp[ 100 ];
	mvcp_interpret_clip_offset( temp, offset, clip );
	return mvcp_execute( this, 1024, "GOTO U%d %d %s", unit, position, temp );
}

/** Set the in point of the loaded file on the specified unit.
*/

mvcp_error_code mvcp_unit_set_in( mvcp this, int unit, int32_t in )
{
	return mvcp_execute( this, 1024, "SIN U%d %d", unit, in );
}

/** Set the in point of the clip on the specified unit.
*/

mvcp_error_code mvcp_unit_clip_set_in( mvcp this, int unit, mvcp_clip_offset offset, int clip, int32_t in )
{
	char temp[ 100 ];
	mvcp_interpret_clip_offset( temp, offset, clip );
	return mvcp_execute( this, 1024, "SIN U%d %d %s", unit, in, temp );
}

/** Set the out point of the loaded file on the specified unit.
*/

mvcp_error_code mvcp_unit_set_out( mvcp this, int unit, int32_t out )
{
	return mvcp_execute( this, 1024, "SOUT U%d %d", unit, out );
}

/** Set the out point of the clip on the specified unit.
*/

mvcp_error_code mvcp_unit_clip_set_out( mvcp this, int unit, mvcp_clip_offset offset, int clip, int32_t in )
{
	char temp[ 100 ];
	mvcp_interpret_clip_offset( temp, offset, clip );
	return mvcp_execute( this, 1024, "SOUT U%d %d %s", unit, in, temp );
}

/** Clear the in point of the loaded file on the specified unit.
*/

mvcp_error_code mvcp_unit_clear_in( mvcp this, int unit )
{
	return mvcp_execute( this, 1024, "SIN U%d -1", unit );
}

/** Clear the out point of the loaded file on the specified unit.
*/

mvcp_error_code mvcp_unit_clear_out( mvcp this, int unit )
{
	return mvcp_execute( this, 1024, "SOUT U%d -1", unit );
}

/** Clear the in and out points on the loaded file on the specified unit.
*/

mvcp_error_code mvcp_unit_clear_in_out( mvcp this, int unit )
{
	mvcp_error_code error = mvcp_unit_clear_out( this, unit );
	if ( error == mvcp_ok )
		error = mvcp_unit_clear_in( this, unit );
	return error;
}

/** Set a unit configuration property.
*/

mvcp_error_code mvcp_unit_set( mvcp this, int unit, const char *name, const char *value )
{
	return mvcp_execute( this, 1024, "USET U%d %s=%s", unit, name, value );
}

/** Get a unit configuration property.
*/

mvcp_error_code mvcp_unit_get( mvcp this, int unit, char *name, char *value, int length )
{
	mvcp_error_code error = mvcp_execute( this, 1024, "UGET U%d %s", unit, name );
	if ( error == mvcp_ok )
	{
		mvcp_response response = mvcp_get_last_response( this );
		strncpy( value, mvcp_response_get_line( response, 1 ), length );
	}
	return error;
}

/** Get a units status.
*/

mvcp_error_code mvcp_unit_status( mvcp this, int unit, mvcp_status status )
{
	mvcp_error_code error = mvcp_execute( this, 1024, "USTA U%d", unit );
	int error_code = mvcp_response_get_error_code( this->last_response );

	memset( status, 0, sizeof( mvcp_status_t ) );
	status->unit = unit;
	if ( error_code == 202 && mvcp_response_count( this->last_response ) == 2 )
		mvcp_status_parse( status, mvcp_response_get_line( this->last_response, 1 ) );
	else if ( error_code == 403 )
		status->status = unit_undefined;

	return error;
}

/** Transfer the current settings of unit src to unit dest.
*/

mvcp_error_code mvcp_unit_transfer( mvcp this, int src, int dest )
{
	return mvcp_execute( this, 1024, "XFER U%d U%d", src, dest );
}

/** Obtain the parsers notifier.
*/

mvcp_notifier mvcp_get_notifier( mvcp this )
{
	if ( this != NULL )
		return mvcp_parser_get_notifier( this->parser );
	else
		return NULL;
}

/** List the contents of the specified directory.
*/

mvcp_dir mvcp_dir_init( mvcp this, const char *directory )
{
	mvcp_dir dir = malloc( sizeof( mvcp_dir_t ) );
	if ( dir != NULL )
	{
		memset( dir, 0, sizeof( mvcp_dir_t ) );
		dir->directory = strdup( directory );
		dir->response = mvcp_parser_executef( this->parser, "CLS \"%s\"", directory );
	}
	return dir;
}

/** Return the error code associated to the dir.
*/

mvcp_error_code mvcp_dir_get_error_code( mvcp_dir dir )
{
	if ( dir != NULL )
		return mvcp_get_error_code( NULL, dir->response );
	else
		return mvcp_malloc_failed;
}

/** Get a particular file entry in the directory.
*/

mvcp_error_code mvcp_dir_get( mvcp_dir dir, int index, mvcp_dir_entry entry )
{
	mvcp_error_code error = mvcp_ok;
	memset( entry, 0, sizeof( mvcp_dir_entry_t ) );
	if ( index < mvcp_dir_count( dir ) )
	{
		char *line = mvcp_response_get_line( dir->response, index + 1 );
		mvcp_tokeniser tokeniser = mvcp_tokeniser_init( );
		mvcp_tokeniser_parse_new( tokeniser, line, " " );

		if ( mvcp_tokeniser_count( tokeniser ) > 0 )
		{
			mvcp_util_strip( mvcp_tokeniser_get_string( tokeniser, 0 ), '\"' );
			strcpy( entry->full, dir->directory );
			if ( entry->full[ strlen( entry->full ) - 1 ] != '/' )
				strcat( entry->full, "/" );
			strcpy( entry->name, mvcp_tokeniser_get_string( tokeniser, 0 ) );
			strcat( entry->full, entry->name );

			switch ( mvcp_tokeniser_count( tokeniser ) )
			{
				case 1:
					entry->dir = 1;
					break;
				case 2:
					entry->size = strtoull( mvcp_tokeniser_get_string( tokeniser, 1 ), NULL, 10 );
					break;
				default:
					error = mvcp_invalid_file;
					break;
			}
		}
		mvcp_tokeniser_close( tokeniser );
	}
	return error;
}

/** Get the number of entries in the directory
*/

int mvcp_dir_count( mvcp_dir dir )
{
	if ( dir != NULL && mvcp_response_count( dir->response ) >= 2 )
		return mvcp_response_count( dir->response ) - 2;
	else
		return -1;
}

/** Close the directory structure.
*/

void mvcp_dir_close( mvcp_dir dir )
{
	if ( dir != NULL )
	{
		free( dir->directory );
		mvcp_response_close( dir->response );
		free( dir );
	}
}

/** List the playlist of the specified unit.
*/

mvcp_list mvcp_list_init( mvcp this, int unit )
{
	mvcp_list list = calloc( 1, sizeof( mvcp_list_t ) );
	if ( list != NULL )
	{
		list->response = mvcp_parser_executef( this->parser, "LIST U%d", unit );
		if ( mvcp_response_count( list->response ) >= 2 )
			list->generation = atoi( mvcp_response_get_line( list->response, 1 ) );
	}
	return list;
}

/** Return the error code associated to the list.
*/

mvcp_error_code mvcp_list_get_error_code( mvcp_list list )
{
	if ( list != NULL )
		return mvcp_get_error_code( NULL, list->response );
	else
		return mvcp_malloc_failed;
}

/** Get a particular file entry in the list.
*/

mvcp_error_code mvcp_list_get( mvcp_list list, int index, mvcp_list_entry entry )
{
	mvcp_error_code error = mvcp_ok;
	memset( entry, 0, sizeof( mvcp_list_entry_t ) );
	if ( index < mvcp_list_count( list ) )
	{
		char *line = mvcp_response_get_line( list->response, index + 2 );
		mvcp_tokeniser tokeniser = mvcp_tokeniser_init( );
		mvcp_tokeniser_parse_new( tokeniser, line, " " );

		if ( mvcp_tokeniser_count( tokeniser ) > 6 )
		{
			entry->clip = atoi( mvcp_tokeniser_get_string( tokeniser, 0 ) );
			mvcp_util_strip( mvcp_tokeniser_get_string( tokeniser, 1 ), '\"' );
			strcpy( entry->full, mvcp_tokeniser_get_string( tokeniser, 1 ) );
			entry->in = atol( mvcp_tokeniser_get_string( tokeniser, 2 ) );
			entry->out = atol( mvcp_tokeniser_get_string( tokeniser, 3 ) );
			entry->max = atol( mvcp_tokeniser_get_string( tokeniser, 4 ) );
			entry->size = atol( mvcp_tokeniser_get_string( tokeniser, 5 ) );
			entry->fps = atof( mvcp_tokeniser_get_string( tokeniser, 6 ) );
		}
		else
		{
			error = mvcp_unknown_error;
		}
		mvcp_tokeniser_close( tokeniser );
	}
	return error;
}

/** Get the number of entries in the list
*/

int mvcp_list_count( mvcp_list list )
{
	if ( list != NULL && mvcp_response_count( list->response ) >= 3 )
		return mvcp_response_count( list->response ) - 3;
	else
		return -1;
}

/** Close the list structure.
*/

void mvcp_list_close( mvcp_list list )
{
	if ( list != NULL )
	{
		mvcp_response_close( list->response );
		free( list );
	}
}

/** List the currently connected nodes.
*/

mvcp_nodes mvcp_nodes_init( mvcp this )
{
	mvcp_nodes nodes = malloc( sizeof( mvcp_nodes_t ) );
	if ( nodes != NULL )
	{
		memset( nodes, 0, sizeof( mvcp_nodes_t ) );
		nodes->response = mvcp_parser_executef( this->parser, "NLS" );
	}
	return nodes;
}

/** Return the error code associated to the nodes list.
*/

mvcp_error_code mvcp_nodes_get_error_code( mvcp_nodes nodes )
{
	if ( nodes != NULL )
		return mvcp_get_error_code( NULL, nodes->response );
	else
		return mvcp_malloc_failed;
}

/** Get a particular node entry.
*/

mvcp_error_code mvcp_nodes_get( mvcp_nodes nodes, int index, mvcp_node_entry entry )
{
	mvcp_error_code error = mvcp_ok;
	memset( entry, 0, sizeof( mvcp_node_entry_t ) );
	if ( index < mvcp_nodes_count( nodes ) )
	{
		char *line = mvcp_response_get_line( nodes->response, index + 1 );
		mvcp_tokeniser tokeniser = mvcp_tokeniser_init( );
		mvcp_tokeniser_parse_new( tokeniser, line, " " );

		if ( mvcp_tokeniser_count( tokeniser ) == 3 )
		{
			entry->node = atoi( mvcp_tokeniser_get_string( tokeniser, 0 ) );
			strncpy( entry->guid, mvcp_tokeniser_get_string( tokeniser, 1 ), sizeof( entry->guid ) );
			mvcp_util_strip( mvcp_tokeniser_get_string( tokeniser, 2 ), '\"' );
			strncpy( entry->name, mvcp_tokeniser_get_string( tokeniser, 2 ), sizeof( entry->name ) );
		}

		mvcp_tokeniser_close( tokeniser );
	}
	return error;
}

/** Get the number of nodes
*/

int mvcp_nodes_count( mvcp_nodes nodes )
{
	if ( nodes != NULL && mvcp_response_count( nodes->response ) >= 2 )
		return mvcp_response_count( nodes->response ) - 2;
	else
		return -1;
}

/** Close the nodes structure.
*/

void mvcp_nodes_close( mvcp_nodes nodes )
{
	if ( nodes != NULL )
	{
		mvcp_response_close( nodes->response );
		free( nodes );
	}
}

/** List the currently defined units.
*/

mvcp_units mvcp_units_init( mvcp this )
{
	mvcp_units units = malloc( sizeof( mvcp_units_t ) );
	if ( units != NULL )
	{
		memset( units, 0, sizeof( mvcp_units_t ) );
		units->response = mvcp_parser_executef( this->parser, "ULS" );
	}
	return units;
}

/** Return the error code associated to the nodes list.
*/

mvcp_error_code mvcp_units_get_error_code( mvcp_units units )
{
	if ( units != NULL )
		return mvcp_get_error_code( NULL, units->response );
	else
		return mvcp_malloc_failed;
}

/** Get a particular unit entry.
*/

mvcp_error_code mvcp_units_get( mvcp_units units, int index, mvcp_unit_entry entry )
{
	mvcp_error_code error = mvcp_ok;
	memset( entry, 0, sizeof( mvcp_unit_entry_t ) );
	if ( index < mvcp_units_count( units ) )
	{
		char *line = mvcp_response_get_line( units->response, index + 1 );
		mvcp_tokeniser tokeniser = mvcp_tokeniser_init( );
		mvcp_tokeniser_parse_new( tokeniser, line, " " );

		if ( mvcp_tokeniser_count( tokeniser ) == 4 )
		{
			entry->unit = atoi( mvcp_tokeniser_get_string( tokeniser, 0 ) + 1 );
			entry->node = atoi( mvcp_tokeniser_get_string( tokeniser, 1 ) );
			strncpy( entry->guid, mvcp_tokeniser_get_string( tokeniser, 2 ), sizeof( entry->guid ) );
			entry->online = atoi( mvcp_tokeniser_get_string( tokeniser, 3 ) );
		}

		mvcp_tokeniser_close( tokeniser );
	}
	return error;
}

/** Get the number of units
*/

int mvcp_units_count( mvcp_units units )
{
	if ( units != NULL && mvcp_response_count( units->response ) >= 2 )
		return mvcp_response_count( units->response ) - 2;
	else
		return -1;
}

/** Close the units structure.
*/

void mvcp_units_close( mvcp_units units )
{
	if ( units != NULL )
	{
		mvcp_response_close( units->response );
		free( units );
	}
}

/** Get the response of the last command executed.
*/

mvcp_response mvcp_get_last_response( mvcp this )
{
	return this->last_response;
}

/** Obtain a printable message associated to the error code provided.
*/

const char *mvcp_error_description( mvcp_error_code error )
{
	const char *msg = "Unrecognised error";
	switch( error )
	{
		case mvcp_ok:
			msg = "OK";
			break;
		case mvcp_malloc_failed:
			msg = "Memory allocation error";
			break;
		case mvcp_unknown_error:
			msg = "Unknown error";
			break;
		case mvcp_no_response:
			msg = "No response obtained";
			break;
		case mvcp_invalid_command:
			msg = "Invalid command";
			break;
		case mvcp_server_timeout:
			msg = "Communications with server timed out";
			break;
		case mvcp_missing_argument:
			msg = "Missing argument";
			break;
		case mvcp_server_unavailable:
			msg = "Unable to communicate with server";
			break;
		case mvcp_unit_creation_failed:
			msg = "Unit creation failed";
			break;
		case mvcp_unit_unavailable:
			msg = "Unit unavailable";
			break;
		case mvcp_invalid_file:
			msg = "Invalid file";
			break;
		case mvcp_invalid_position:
			msg = "Invalid position";
			break;
	}
	return msg;
}

/** Close the mvcp structure.
*/

void mvcp_close( mvcp this )
{
	if ( this != NULL )
	{
		mvcp_set_last_response( this, NULL );
		free( this );
	}
}

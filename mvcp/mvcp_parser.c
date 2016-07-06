/*
 * mvcp_parser.c -- MVCP Parser for Melted
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
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

/* Application header files */
#include "mvcp_parser.h"
#include "mvcp_util.h"

/** Connect to the parser.
*/

mvcp_response mvcp_parser_connect( mvcp_parser parser )
{
	parser->notifier = mvcp_notifier_init( );
	return parser->connect( parser->real );
}

/** Execute a command via the parser.
*/

mvcp_response mvcp_parser_execute( mvcp_parser parser, char *command )
{
	return parser->execute( parser->real, command );
}

/** Push a service via the parser.
*/

mvcp_response mvcp_parser_received( mvcp_parser parser, char *command, char *doc )
{
	return parser->received != NULL ? parser->received( parser->real, command, doc ) : NULL;
}

/** Push a service via the parser.
*/

mvcp_response mvcp_parser_push( mvcp_parser parser, char *command, mlt_service service )
{
	return parser->push( parser->real, command, service );
}

/** Execute a formatted command via the parser.
*/

mvcp_response mvcp_parser_executef( mvcp_parser parser, const char *format, ... )
{
	char *command = malloc( 10240 );
	mvcp_response response = NULL;
 	if ( command != NULL )
	{
		va_list list;
		va_start( list, format );
		if ( vsnprintf( command, 10240, format, list ) != 0 )
			response = mvcp_parser_execute( parser, command );
		va_end( list );
		free( command );
	}
	return response;
}

/** Execute the contents of a file descriptor.
*/

mvcp_response mvcp_parser_run_file( mvcp_parser parser, FILE *file )
{
	mvcp_response response = mvcp_response_init( );
	if ( response != NULL )
	{
		char command[ 1024 ];
		mvcp_response_set_error( response, 201, "OK" );
		while ( mvcp_response_get_error_code( response ) == 201 && fgets( command, 1024, file ) )
		{
			mvcp_util_trim( mvcp_util_chomp( command ) );
			if ( strcmp( command, "" ) && command[ 0 ] != '#' )
			{
				mvcp_response temp = NULL;
				mvcp_response_printf( response, 1024, "%s\n", command );
				temp = mvcp_parser_execute( parser, command );
				if ( temp != NULL )
				{
					int index = 0;
					for ( index = 0; index < mvcp_response_count( temp ); index ++ )
						mvcp_response_printf( response, 10240, "%s\n", mvcp_response_get_line( temp, index ) );
					mvcp_response_close( temp );
				}
				else
				{
					mvcp_response_set_error( response, 500, "Batch execution failed" );
				}
			}
		}
	}
	return response;
}

/** Execute the contents of a file. Note the special case mvcp_response returned.
*/

mvcp_response mvcp_parser_run( mvcp_parser parser, char *filename )
{
	mvcp_response response;
	FILE *file = fopen( filename, "r" );
	if ( file != NULL )
	{
		response = mvcp_parser_run_file( parser, file );
		fclose( file );
	}
	else
	{
		response = mvcp_response_init( );
		mvcp_response_set_error( response, 404, "File not found." );
	}
	return response;
}

/** Get the notifier associated to the parser.
*/

mvcp_notifier mvcp_parser_get_notifier( mvcp_parser parser )
{
	if ( parser->notifier == NULL )
		parser->notifier = mvcp_notifier_init( );
	return parser->notifier;
}

/** Close the parser.
*/

void mvcp_parser_close( mvcp_parser parser )
{
	if ( parser != NULL )
	{
		parser->close( parser->real );
		mvcp_notifier_close( parser->notifier );
		free( parser );
	}
}

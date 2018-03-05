/*
 * mvcp_response.c -- Response
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
#include <stdarg.h>
#include <string.h>

/* Application header files */
#include "mvcp_response.h"

/** Construct a new MVCP response.
*/

mvcp_response mvcp_response_init( )
{
	mvcp_response response = malloc( sizeof( mvcp_response_t ) );
	if ( response != NULL )
		memset( response, 0, sizeof( mvcp_response_t ) );
	return response;
}

/** Clone a MVCP response
*/

mvcp_response mvcp_response_clone( mvcp_response response )
{
	mvcp_response clone = mvcp_response_init( );
	if ( clone != NULL && response != NULL )
	{
		int index = 0;
		for ( index = 0; index < mvcp_response_count( response ); index ++ )
		{
			char *line = mvcp_response_get_line( response, index );
			mvcp_response_printf( clone, strlen( line ) + 2, "%s\n", line );
		}
	}
	return clone;
}

/** Get the error code associated to the response.
*/

int mvcp_response_get_error_code( mvcp_response response )
{
	int error_code = -1;
	if ( response != NULL )
	{
		if ( response->count > 0 )
		{
			if ( sscanf( response->array[ 0 ], "%d", &error_code ) != 1 )
				error_code = 0;
		}
		else
		{
			error_code = -2;
		}
	}
	return error_code;
}

/** Get the error description associated to the response.
*/

const char *mvcp_response_get_error_string( mvcp_response response )
{
	const char *error_string = "No message specified";
	if ( response->count > 0 )
	{
		char *ptr = strchr( response->array[ 0 ], ' ' ) ;
		if ( ptr != NULL )
			error_string = ptr + 1;
	}
	return error_string;
}

/** Get a line of text at the given index. Note that the text itself is
	terminated only with a NUL char and it is the responsibility of the
	the user of the returned data to use a LF or CR/LF as appropriate.
*/

char *mvcp_response_get_line( mvcp_response response, int index )
{
	if ( index < response->count )
		return response->array[ index ];
	else
		return NULL;
}

/** Return the number of lines of text in the response.
*/

int mvcp_response_count( mvcp_response response )
{
	if ( response != NULL )
		return response->count;
	else
		return 0;
}

/** Set the error and description associated to the response.
*/

void mvcp_response_set_error( mvcp_response response, int error_code, const char *error_string )
{
	if ( response->count == 0 )
	{
		mvcp_response_printf( response, 10240, "%d %s\n", error_code, error_string );
	}
	else
	{
		char temp[ 10240 ];
		int length = sprintf( temp, "%d %s", error_code, error_string );
		response->array[ 0 ] = realloc( response->array[ 0 ], length + 1 );
		strcpy( response->array[ 0 ], temp );
	}
}

/** Write formatted text to the response. 
*/

int mvcp_response_printf( mvcp_response response, size_t size, const char *format, ... )
{
	int length = 0;
	char *text = malloc( size );
	if ( text != NULL )
	{
		va_list list;
		va_start( list, format );
		length = vsnprintf( text, size, format, list );
		if ( length != 0 )
			mvcp_response_write( response, text, length );
		va_end( list );
		free( text );
	}
	return length;
}

/** Write text to the response.
*/

int mvcp_response_write( mvcp_response response, const char *text, int size )
{
	int ret = 0;
	const char *ptr = text;

	while ( size > 0 )
	{
		int index = response->count - 1;
		const char *lf = strchr( ptr, '\n' );
		int length_of_string = 0;

		/* Make sure we have space in the dynamic array. */
		if ( !response->append && response->count >= response->size - 1 )
		{
			response->size += 50;
			response->array = realloc( response->array, response->size * sizeof( char * ) );
		}

		/* Make sure the array is valid, or we're really in trouble */
		if ( response->array == NULL )
		{
			ret = 0;
			break;
		}

		/* Now, if we're appending to the previous write (ie: if it wasn't
		   terminated by a LF), then use the index calculated above, otherwise
		   go to the next one and ensure it's NULLed. */

		if ( !response->append )
		{
			response->array[ ++ index ] = NULL;
			response->count ++;
		}
		else
		{
			length_of_string = strlen( response->array[ index ] );
		}

		/* Now we need to know how to handle the current ptr with respect to lf. */
		/* TODO: tidy up and error check... sigh... tested for many, many 1000s of lines */

		if ( lf == NULL )
		{
			response->array[ index ] = realloc( response->array[ index ], length_of_string + size + 1 );
			memcpy( response->array[ index ] + length_of_string, ptr, size );
			response->array[ index ][ length_of_string + size ] = '\0';
			if ( ( length_of_string + size ) > 0 && response->array[ index ][ length_of_string + size - 1 ] == '\r' )
				response->array[ index ][ length_of_string + size - 1 ] = '\0';
			size = 0;
			ret += size;
			response->append = 1;
		}
		else
		{
			int chars = lf - ptr;
			response->array[ index ] = realloc( response->array[ index ], length_of_string + chars + 1 );
			memcpy( response->array[ index ] + length_of_string, ptr, chars );
			response->array[ index ][ length_of_string + chars ] = '\0';
			if ( ( length_of_string + chars ) > 0 && response->array[ index ][ length_of_string + chars - 1 ] == '\r' )
				response->array[ index ][ length_of_string + chars - 1 ] = '\0';
			ptr = ptr + chars + 1;
			size -= ( chars + 1 );
			response->append = 0;
			ret += chars + 1;
		}
	}

	return ret;
}

/** Close the response.
*/

void mvcp_response_close( mvcp_response response )
{
	if ( response != NULL )
	{
		int index = 0;
		for ( index = 0; index < response->count; index ++ )
			free( response->array[ index ] );
		free( response->array );
		free( response );
	}
}

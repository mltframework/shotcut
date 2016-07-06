/*
 * mvcp_util.c -- General Purpose Client Utilities
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
#include <string.h>
#include <ctype.h>

/* Application header files */
#include "mvcp_util.h"

/** Remove LF or CR/LF terminations from the input string.
*/

char *mvcp_util_chomp( char *input )
{
	if ( input != NULL )
	{
		int length = strlen( input );
		if ( length && input[ length - 1 ] == '\n' )
			input[ length - 1 ] = '\0';
		if ( length > 1 && input[ length - 2 ] == '\r' )
			input[ length - 2 ] = '\0';
	}
	return input;
}

/** Remove leading and trailing spaces from the input string.
*/

char *mvcp_util_trim( char *input )
{
	if ( input != NULL )
	{
		int length = strlen( input );
		int first = 0;
		while( first < length && isspace( input[ first ] ) )
			first ++;
		memmove( input, input + first, length - first + 1 );
		length = length - first;
		while ( length > 0 && isspace( input[ length - 1 ] ) )
			input[ -- length ] = '\0';
	}
	return input;
}

/** Strip the specified string of leading and trailing 'value' (ie: ").
*/

char *mvcp_util_strip( char *input, char value )
{
	if ( input != NULL )
	{
		char *ptr = strrchr( input, value );
		if ( ptr != NULL )
			*ptr = '\0';
		if ( input[ 0 ] == value )
			memmove( input, input + 1, strlen( input ) );
	}
	return input;
}

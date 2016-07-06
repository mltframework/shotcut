/*
 * mvcp_response.h -- Response
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

#ifndef _MVCP_RESPONSE_H_
#define _MVCP_RESPONSE_H_

#include <stdio.h>

#ifdef __cplusplus
extern "C"
{
#endif

/** Structure for the response
*/

typedef struct
{
	char **array;
	int size;
	int count;
	int append;
}
*mvcp_response, mvcp_response_t;

/** API for accessing the response structure.
*/

extern mvcp_response mvcp_response_init( );
extern mvcp_response mvcp_response_clone( mvcp_response );
extern int mvcp_response_get_error_code( mvcp_response );
extern const char *mvcp_response_get_error_string( mvcp_response );
extern char *mvcp_response_get_line( mvcp_response, int );
extern int mvcp_response_count( mvcp_response );
extern void mvcp_response_set_error( mvcp_response, int, const char * );
extern int mvcp_response_printf( mvcp_response, size_t, const char *, ... );
extern int mvcp_response_write( mvcp_response, const char *, int );
extern void mvcp_response_close( mvcp_response );

#ifdef __cplusplus
}
#endif

#endif

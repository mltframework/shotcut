/*
 * mvcp_parser.h -- MVCP Parser for Melted
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

#ifndef _MVCP_PARSER_H_
#define _MVCP_PARSER_H_

/* MLT Header files */
#ifndef MVCP_EMBEDDED
#include <framework/mlt.h>
#else
#define mlt_service void *
#endif

/* Application header files */
#include "mvcp_response.h"
#include "mvcp_notifier.h"

#ifdef __cplusplus
extern "C"
{
#endif

/** Callbacks to define the parser.
*/

typedef mvcp_response (*parser_connect)( void * );
typedef mvcp_response (*parser_execute)( void *, char * );
typedef mvcp_response (*parser_received)( void *, char *, char * );
typedef mvcp_response (*parser_push)( void *, char *, mlt_service );
typedef void (*parser_close)( void * );

/** Structure for the mvcp parser.
*/

typedef struct
{
	parser_connect connect;
	parser_execute execute;
	parser_push push;
	parser_received received;
	parser_close close;
	void *real;
	mvcp_notifier notifier;
}
*mvcp_parser, mvcp_parser_t;

/** API for the parser - note that no constructor is defined here.
*/

extern mvcp_response mvcp_parser_connect( mvcp_parser );
extern mvcp_response mvcp_parser_push( mvcp_parser, char *, mlt_service );
extern mvcp_response mvcp_parser_received( mvcp_parser, char *, char * );
extern mvcp_response mvcp_parser_execute( mvcp_parser, char * );
extern mvcp_response mvcp_parser_executef( mvcp_parser, const char *, ... );
extern mvcp_response mvcp_parser_run_file( mvcp_parser parser, FILE *file );
extern mvcp_response mvcp_parser_run( mvcp_parser, char * );
extern mvcp_notifier mvcp_parser_get_notifier( mvcp_parser );
extern void mvcp_parser_close( mvcp_parser );

#ifdef __cplusplus
}
#endif

#endif

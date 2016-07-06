/*
 * mvcp_tokeniser.h -- String tokeniser
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

#ifndef _MVCP_TOKENISER_H_
#define _MVCP_TOKENISER_H_

#ifdef __cplusplus
extern "C"
{
#endif

/** Structure for tokeniser.
*/

typedef struct
{
	char *input;
	char **tokens;
	int count;
	int size;
}
*mvcp_tokeniser, mvcp_tokeniser_t;

/** Remote parser API.
*/

extern mvcp_tokeniser mvcp_tokeniser_init( );
extern int mvcp_tokeniser_parse_new( mvcp_tokeniser, char *, const char * );
extern char *mvcp_tokeniser_get_input( mvcp_tokeniser );
extern int mvcp_tokeniser_count( mvcp_tokeniser );
extern char *mvcp_tokeniser_get_string( mvcp_tokeniser, int );
extern void mvcp_tokeniser_close( mvcp_tokeniser );

#ifdef __cplusplus
}
#endif

#endif

/*
 * mvcp_status.h -- Unit Status Handling
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

#ifndef _MVCP_STATUS_H_
#define _MVCP_STATUS_H_

#include <stdint.h>

#ifdef __cplusplus
extern "C"
{
#endif

/** Status codes
*/

typedef enum
{
	unit_unknown = 0,
	unit_undefined,
	unit_offline,
	unit_not_loaded,
	unit_stopped,
	unit_playing,
	unit_paused,
	unit_disconnected
}
unit_status;

/** Status structure.
*/

typedef struct
{
	int unit;
	unit_status status;
	char clip[ 2048 ];
	int32_t position;
	int speed;
	double fps;
	int32_t in;
	int32_t out;
	int32_t length;
	char tail_clip[ 2048 ];
	int32_t tail_position;
	int32_t tail_in;
	int32_t tail_out;
	int32_t tail_length;
	int seek_flag;
	int generation;
	int clip_index;
	int dummy;
}
*mvcp_status, mvcp_status_t;

/** MVCP Status API
*/

extern void mvcp_status_parse( mvcp_status, char * );
extern char *mvcp_status_serialise( mvcp_status, char *, int );
extern int mvcp_status_compare( mvcp_status, mvcp_status );
extern mvcp_status mvcp_status_copy( mvcp_status, mvcp_status );

#ifdef __cplusplus
}
#endif

#endif

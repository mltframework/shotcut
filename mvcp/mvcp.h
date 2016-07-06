/*
 * mvcp.h -- High Level Client API for Melted
 * Copyright (C) 2002-2015 Meltytech, LLC
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

#ifndef _MVCP_H_
#define _MVCP_H_

/* System header files */
#include <limits.h>

/* MLT Header files. */
#ifndef MVCP_EMBEDDED
#include <framework/mlt.h>
#else
#define mlt_service void *
#endif

/* Application header files */
#include "mvcp_parser.h"
#include "mvcp_status.h"
#include "mvcp_notifier.h"


#ifndef PATH_MAX
#define PATH_MAX (4096)
#endif
#ifndef NAME_MAX
#define NAME_MAX (256)
#endif

#ifdef __cplusplus
extern "C"
{
#endif

/** Client error conditions
*/

typedef enum
{
	mvcp_ok = 0,
	mvcp_malloc_failed,
	mvcp_unknown_error,
	mvcp_no_response,
	mvcp_invalid_command,
	mvcp_server_timeout,
	mvcp_missing_argument,
	mvcp_server_unavailable,
	mvcp_unit_creation_failed,
	mvcp_unit_unavailable,
	mvcp_invalid_file,
	mvcp_invalid_position
}
mvcp_error_code;

/** Clip index specification.
*/

typedef enum
{
	mvcp_absolute = 0,
	mvcp_relative
}
mvcp_clip_offset;

/** Client structure.
*/

typedef struct
{
	mvcp_parser parser;
	mvcp_response last_response;
}
*mvcp, mvcp_t;

/** Client API.
*/

extern mvcp mvcp_init( mvcp_parser );

/* Connect to the mvcp parser instance */
extern mvcp_error_code mvcp_connect( mvcp );

/* Global functions */
extern mvcp_error_code mvcp_set( mvcp, char *, char * );
extern mvcp_error_code mvcp_get( mvcp, char *, char *, int );
extern mvcp_error_code mvcp_run( mvcp, char * );

/* Unit functions */
extern mvcp_error_code mvcp_unit_add( mvcp, char *, int * );
extern mvcp_error_code mvcp_unit_load( mvcp, int, char * );
extern mvcp_error_code mvcp_unit_load_clipped( mvcp, int, char *, int32_t, int32_t );
extern mvcp_error_code mvcp_unit_load_back( mvcp, int, char * );
extern mvcp_error_code mvcp_unit_load_back_clipped( mvcp, int, char *, int32_t, int32_t );
extern mvcp_error_code mvcp_unit_append( mvcp, int, char *, int32_t, int32_t );
extern mvcp_error_code mvcp_unit_receive( mvcp, int, char *, char * );
extern mvcp_error_code mvcp_unit_push( mvcp, int, char *, mlt_service );
extern mvcp_error_code mvcp_unit_clean( mvcp, int );
extern mvcp_error_code mvcp_unit_wipe( mvcp, int );
extern mvcp_error_code mvcp_unit_clear( mvcp, int );
extern mvcp_error_code mvcp_unit_clip_move( mvcp, int, mvcp_clip_offset, int, mvcp_clip_offset, int );
extern mvcp_error_code mvcp_unit_clip_remove( mvcp, int, mvcp_clip_offset, int );
extern mvcp_error_code mvcp_unit_remove_current_clip( mvcp, int );
extern mvcp_error_code mvcp_unit_clip_insert( mvcp, int, mvcp_clip_offset, int, char *, int32_t, int32_t );
extern mvcp_error_code mvcp_unit_play( mvcp, int );
extern mvcp_error_code mvcp_unit_play_at_speed( mvcp, int, int );
extern mvcp_error_code mvcp_unit_stop( mvcp, int );
extern mvcp_error_code mvcp_unit_pause( mvcp, int );
extern mvcp_error_code mvcp_unit_rewind( mvcp, int );
extern mvcp_error_code mvcp_unit_fast_forward( mvcp, int );
extern mvcp_error_code mvcp_unit_step( mvcp, int, int32_t );
extern mvcp_error_code mvcp_unit_goto( mvcp, int, int32_t );
extern mvcp_error_code mvcp_unit_clip_goto( mvcp, int, mvcp_clip_offset, int, int32_t );
extern mvcp_error_code mvcp_unit_clip_set_in( mvcp, int, mvcp_clip_offset, int, int32_t );
extern mvcp_error_code mvcp_unit_clip_set_out( mvcp, int, mvcp_clip_offset, int, int32_t );
extern mvcp_error_code mvcp_unit_set_in( mvcp, int, int32_t );
extern mvcp_error_code mvcp_unit_set_out( mvcp, int, int32_t );
extern mvcp_error_code mvcp_unit_clear_in( mvcp, int );
extern mvcp_error_code mvcp_unit_clear_out( mvcp, int );
extern mvcp_error_code mvcp_unit_clear_in_out( mvcp, int );
extern mvcp_error_code mvcp_unit_set( mvcp, int, const char *, const char * );
extern mvcp_error_code mvcp_unit_get( mvcp, int, char *, char *, int );
extern mvcp_error_code mvcp_unit_status( mvcp, int, mvcp_status );
extern mvcp_error_code mvcp_unit_transfer( mvcp, int, int );

/* Notifier functionality. */
extern mvcp_notifier mvcp_get_notifier( mvcp );

/** Structure for the directory.
*/

typedef struct
{
	char *directory;
	mvcp_response response;
}
*mvcp_dir, mvcp_dir_t;

/** Directory entry structure.
*/

typedef struct
{
	int dir;
	char name[ NAME_MAX ];
	char full[ PATH_MAX + NAME_MAX ];
	unsigned long long size;
}
*mvcp_dir_entry, mvcp_dir_entry_t;

/* Directory reading. */
extern mvcp_dir mvcp_dir_init( mvcp, const char * );
extern mvcp_error_code mvcp_dir_get_error_code( mvcp_dir );
extern mvcp_error_code mvcp_dir_get( mvcp_dir, int, mvcp_dir_entry );
extern int mvcp_dir_count( mvcp_dir );
extern void mvcp_dir_close( mvcp_dir );

/** Structure for the list.
*/

typedef struct
{
	int generation;
	mvcp_response response;
}
*mvcp_list, mvcp_list_t;

/** List entry structure.
*/

typedef struct
{
	int clip;
	char full[ PATH_MAX + NAME_MAX ];
	int32_t in;
	int32_t out;
	int32_t max;
	int32_t size;
	float fps;
}
*mvcp_list_entry, mvcp_list_entry_t;

/* List reading. */
extern mvcp_list mvcp_list_init( mvcp, int );
extern mvcp_error_code mvcp_list_get_error_code( mvcp_list );
extern mvcp_error_code mvcp_list_get( mvcp_list, int, mvcp_list_entry );
extern int mvcp_list_count( mvcp_list );
extern void mvcp_list_close( mvcp_list );

/** Structure for nodes.
*/

typedef struct
{
	mvcp_response response;
}
*mvcp_nodes, mvcp_nodes_t;

/** Node entry structure.
*/

typedef struct
{
	int node;
	char guid[ 17 ];
	char name[ 1024 ];
}
*mvcp_node_entry, mvcp_node_entry_t;

/* Node reading. */
extern mvcp_nodes mvcp_nodes_init( mvcp );
extern mvcp_error_code mvcp_nodes_get_error_code( mvcp_nodes );
extern mvcp_error_code mvcp_nodes_get( mvcp_nodes, int, mvcp_node_entry );
extern int mvcp_nodes_count( mvcp_nodes );
extern void mvcp_nodes_close( mvcp_nodes );

/** Structure for units.
*/

typedef struct
{
	mvcp_response response;
}
*mvcp_units, mvcp_units_t;

/** Unit entry structure.
*/

typedef struct
{
	int unit;
	int node;
	char guid[ 512 ];
	int online;
}
*mvcp_unit_entry, mvcp_unit_entry_t;

/* Unit reading. */
extern mvcp_units mvcp_units_init( mvcp );
extern mvcp_error_code mvcp_units_get_error_code( mvcp_units );
extern mvcp_error_code mvcp_units_get( mvcp_units, int, mvcp_unit_entry );
extern int mvcp_units_count( mvcp_units );
extern void mvcp_units_close( mvcp_units );

/* Miscellaenous functions */
extern mvcp_response mvcp_get_last_response( mvcp );
extern const char *mvcp_error_description( mvcp_error_code );

/* Courtesy functions. */
extern mvcp_error_code mvcp_execute( mvcp, size_t, const char *, ... );
extern mvcp_error_code mvcp_push( mvcp, mlt_service, size_t, const char *, ... );

/* Close function. */
extern void mvcp_close( mvcp );

#ifdef __cplusplus
}
#endif

#endif

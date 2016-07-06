/*
 * mvcp_remote.c -- Remote Parser
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
#include <signal.h>
#include <errno.h>
#include <pthread.h>

/* Application header files */
#ifndef MVCP_EMBEDDED
#include <framework/mlt.h>
#else
#define mlt_service void *
#endif
#include "mvcp_remote.h"
#include "mvcp_socket.h"
#include "mvcp_tokeniser.h"
#include "mvcp_util.h"

/** Private mvcp_remote structure.
*/

typedef struct
{
	int terminated;
	char *server;
	int port;
	mvcp_socket socket;
	mvcp_socket status;
	pthread_t thread;
	mvcp_parser parser;
	pthread_mutex_t mutex;
	int connected;
}
*mvcp_remote, mvcp_remote_t;

/** Forward declarations.
*/

static mvcp_response mvcp_remote_connect( mvcp_remote );
static mvcp_response mvcp_remote_execute( mvcp_remote, char * );
static mvcp_response mvcp_remote_receive( mvcp_remote, char *, char * );
static mvcp_response mvcp_remote_push( mvcp_remote, char *, mlt_service );
static void mvcp_remote_close( mvcp_remote );
static int mvcp_remote_read_response( mvcp_socket, mvcp_response );

/** MVCP Parser constructor.
*/

mvcp_parser mvcp_parser_init_remote( char *server, int port )
{
	mvcp_parser parser = calloc( 1, sizeof( mvcp_parser_t ) );
	mvcp_remote remote = calloc( 1, sizeof( mvcp_remote_t ) );

	if ( parser != NULL )
	{
		parser->connect = (parser_connect)mvcp_remote_connect;
		parser->execute = (parser_execute)mvcp_remote_execute;
		parser->push = (parser_push)mvcp_remote_push;
		parser->received = (parser_received)mvcp_remote_receive;
		parser->close = (parser_close)mvcp_remote_close;
		parser->real = remote;

		if ( remote != NULL )
		{
			remote->parser = parser;
			remote->server = strdup( server );
			remote->port = port;
			pthread_mutex_init( &remote->mutex, NULL );
		}
	}
	return parser;
}

/** Thread for receiving and distributing the status information.
*/

static void *mvcp_remote_status_thread( void *arg )
{
	mvcp_remote remote = arg;
	char temp[ 10240 ];
	int length = 0;
	int offset = 0;
	mvcp_tokeniser tokeniser = mvcp_tokeniser_init( );
	mvcp_notifier notifier = mvcp_parser_get_notifier( remote->parser );
	mvcp_status_t status;
	int index = 0;

	mvcp_socket_write_data( remote->status, "STATUS\r\n", 8 );
	temp[ 0 ] = '\0';
	while ( !remote->terminated && 
			( length = mvcp_socket_read_data( remote->status, temp + offset, sizeof( temp ) - 1 ) ) >= 0 )
	{
		if ( length < 0 )
			break;
		temp[ length ] = '\0';
		if ( strchr( temp, '\n' ) == NULL )
		{
			offset = length;
			continue;
		}
		offset = 0;
		mvcp_tokeniser_parse_new( tokeniser, temp, "\n" );
		for ( index = 0; index < mvcp_tokeniser_count( tokeniser ); index ++ )
		{
			char *line = mvcp_tokeniser_get_string( tokeniser, index );
			if ( line[ strlen( line ) - 1 ] == '\r' )
			{
				mvcp_util_chomp( line );
				mvcp_status_parse( &status, line );
				mvcp_notifier_put( notifier, &status );
			}
			else
			{
				strcpy( temp, line );
				offset = strlen( temp );
			}
		}
	}

	mvcp_notifier_disconnected( notifier );
	mvcp_tokeniser_close( tokeniser );
	remote->terminated = 1;

	return NULL;
}

/** Forward reference.
*/

static void mvcp_remote_disconnect( mvcp_remote remote );

/** Connect to the server.
*/

static mvcp_response mvcp_remote_connect( mvcp_remote remote )
{
	mvcp_response response = NULL;

	mvcp_remote_disconnect( remote );

	if ( !remote->connected )
	{
#ifndef WIN32
		signal( SIGPIPE, SIG_IGN );
#endif
		remote->socket = mvcp_socket_init( remote->server, remote->port );
		remote->status = mvcp_socket_init( remote->server, remote->port );

		if ( mvcp_socket_connect( remote->socket ) == 0 )
		{
			response = mvcp_response_init( );
			mvcp_remote_read_response( remote->socket, response );
		}

		if ( 0 && response != NULL && mvcp_socket_connect( remote->status ) == 0 )
		{
			mvcp_response status_response = mvcp_response_init( );
			mvcp_remote_read_response( remote->status, status_response );
			if ( mvcp_response_get_error_code( status_response ) == 100 )
				pthread_create( &remote->thread, NULL, mvcp_remote_status_thread, remote );
			mvcp_response_close( status_response );
			remote->connected = 1;
		}
	}

	return response;
}

/** Execute the command.
*/

static mvcp_response mvcp_remote_execute( mvcp_remote remote, char *command )
{
	mvcp_response response = NULL;
	pthread_mutex_lock( &remote->mutex );
	if ( (size_t) mvcp_socket_write_data( remote->socket, command, strlen( command ) ) == strlen( command ) )
	{
		response = mvcp_response_init( );
		mvcp_socket_write_data( remote->socket, "\r\n", 2 );
		mvcp_remote_read_response( remote->socket, response );
	}
	pthread_mutex_unlock( &remote->mutex );
	return response;
}

/** Push a MLT XML document to the server.
*/

static mvcp_response mvcp_remote_receive( mvcp_remote remote, char *command, char *buffer )
{
	mvcp_response response = NULL;
	pthread_mutex_lock( &remote->mutex );
	if ( (size_t) mvcp_socket_write_data( remote->socket, command, strlen( command ) ) == strlen( command ) )
	{
		char temp[ 20 ];
		int length = strlen( buffer );
		response = mvcp_response_init( );
		mvcp_socket_write_data( remote->socket, "\r\n", 2 );
		sprintf( temp, "%d", length );
		mvcp_socket_write_data( remote->socket, temp, strlen( temp ) );
		mvcp_socket_write_data( remote->socket, "\r\n", 2 );
		mvcp_socket_write_data( remote->socket, buffer, length );
		mvcp_socket_write_data( remote->socket, "\r\n", 2 );
		mvcp_remote_read_response( remote->socket, response );
	}
	pthread_mutex_unlock( &remote->mutex );
	return response;
}

/** Push a producer to the server.
*/

static mvcp_response mvcp_remote_push( mvcp_remote remote, char *command, mlt_service service )
{
	(void) remote; // unused
	(void) command; // unused
	(void) service; // unused

	mvcp_response response = NULL;
#ifndef MVCP_EMBEDDED
	if ( service != NULL )
	{
		mlt_consumer consumer = mlt_factory_consumer( NULL, "xml", "buffer" );
		mlt_properties properties = MLT_CONSUMER_PROPERTIES( consumer );
		char *buffer = NULL;
		// Temporary hack
		mlt_properties_set( properties, "store", "nle_" );
		mlt_consumer_connect( consumer, service );
		mlt_consumer_start( consumer );
		buffer = mlt_properties_get( properties, "buffer" );
		response = mvcp_remote_receive( remote, command, buffer );
		mlt_consumer_close( consumer );
	}
#endif
	return response;
}

/** Disconnect.
*/

static void mvcp_remote_disconnect( mvcp_remote remote )
{
	if ( remote != NULL && remote->terminated )
	{
		if ( remote->connected )
			pthread_join( remote->thread, NULL );
		mvcp_socket_close( remote->status );
		mvcp_socket_close( remote->socket );
		remote->connected = 0;
		remote->terminated = 0;
	}
}

/** Close the parser.
*/

static void mvcp_remote_close( mvcp_remote remote )
{
	if ( remote != NULL )
	{
		remote->terminated = 1;
		mvcp_remote_disconnect( remote );
		pthread_mutex_destroy( &remote->mutex );
		free( remote->server );
		free( remote );
	}
}

/** Read response. 
*/

static int mvcp_remote_read_response( mvcp_socket socket, mvcp_response response )
{
	char temp[ 10240 ];
	int length;
	int terminated = 0;

	temp[ 0 ] = '\0';
	while ( !terminated && ( length = mvcp_socket_read_data( socket, temp, sizeof( temp ) - 1 ) ) >= 0 )
	{
		int position = 0;
		temp[ length ] = '\0';
		mvcp_response_write( response, temp, length );
		position = mvcp_response_count( response ) - 1;
		if ( position < 0 || temp[ strlen( temp ) - 1 ] != '\n' )
			continue;
		switch( mvcp_response_get_error_code( response ) )
		{
			case 201:
			case 500:
				terminated = !strcmp( mvcp_response_get_line( response, position ), "" );
				break;
			case 202:
				terminated = mvcp_response_count( response ) >= 2;
				break;
			default:
				terminated = 1;
				break;
		}
	}

	return 0;
}

/*
 * mvcp_socket.c -- Client Socket
 * Copyright (C) 2002-2009 Ushodaya Enterprises Limited
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

/* System header files */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <errno.h>
#include <netinet/in.h>
#include <sys/time.h>

/* Application header files */
#include "mvcp_socket.h"

/** Initialise the socket.
*/

mvcp_socket mvcp_socket_init( char *server, int port )
{
	mvcp_socket socket = malloc( sizeof( mvcp_socket_t ) );
	if ( socket != NULL )
	{
		memset( socket, 0, sizeof( mvcp_socket_t ) );
		socket->fd = -1;
		socket->server = strdup( server );
		socket->port = port;
	}
	return socket;
}

/** Connect to the server.
*/

int mvcp_socket_connect( mvcp_socket connection )
{
	int ret = 0;
    struct hostent *host;
    struct sockaddr_in sock;

	if ( connection->server != NULL )
	{		
		host = gethostbyname( connection->server );
	
		memset( &sock, 0, sizeof( struct sockaddr_in ) );
		memcpy( &sock.sin_addr, host->h_addr, host->h_length );
		sock.sin_family = host->h_addrtype;
		sock.sin_port = htons( connection->port );
	
		if ( ( connection->fd = socket( AF_INET, SOCK_STREAM, 0 ) ) != -1 )
			ret = connect( connection->fd, (const struct sockaddr *)&sock, sizeof( struct sockaddr_in ) );
		else
			ret = -1;
	}
	
	return ret;	
}

/** Convenience constructor for a connected file descriptor.
*/

mvcp_socket mvcp_socket_init_fd( int fd )
{
	mvcp_socket socket = malloc( sizeof( mvcp_socket_t ) );
	if ( socket != NULL )
	{
		memset( socket, 0, sizeof( mvcp_socket_t ) );
		socket->fd = fd;
		socket->no_close = 1;
	}
	return socket;
}

/** Read an arbitrarily formatted block of data from the server.
*/

int mvcp_socket_read_data( mvcp_socket socket, char *data, int length )
{
    struct timeval tv = { 1, 0 };
    fd_set rfds;
	int used = 0;

	data[ 0 ] = '\0';

    FD_ZERO( &rfds );
    FD_SET( socket->fd, &rfds );

	if ( select( socket->fd + 1, &rfds, NULL, NULL, &tv ) )
	{
		used = read( socket->fd, data, length - 1 );
		if ( used > 0 )
			data[ used ] = '\0';
		else
			used = -1;
	}

	return used;
}	

/** Write an arbitrarily formatted block of data to the server.
*/

int mvcp_socket_write_data( mvcp_socket socket, const char *data, int length )
{
	int used = 0;
	
	while ( used >=0 && used < length )
	{
		struct timeval tv = { 1, 0 };
		fd_set rfds;
		fd_set wfds;
		fd_set efds;
	
		FD_ZERO( &rfds );
		FD_SET( socket->fd, &rfds );
		FD_ZERO( &wfds );
		FD_SET( socket->fd, &wfds );
		FD_ZERO( &efds );
		FD_SET( socket->fd, &efds );
	
		errno = 0;

		if ( select( socket->fd + 1, &rfds, &wfds, &efds, &tv ) )
		{
			if ( errno != 0 || FD_ISSET( socket->fd, &efds ) || FD_ISSET( socket->fd, &rfds ) )
			{
				used = -1;
			}
			else if ( FD_ISSET( socket->fd, &wfds ) )
			{
				int inc = write( socket->fd, data + used, length - used );
				if ( inc > 0 )
					used += inc;
				else
					used = -1;
			}
		}
	}

	return used;
}

/** Close the socket.
*/

void mvcp_socket_close( mvcp_socket socket )
{
	if ( socket->fd > 0 && !socket->no_close )
		close( socket->fd );
	free( socket->server );
	free( socket );
}

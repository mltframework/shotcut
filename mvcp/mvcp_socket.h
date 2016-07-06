/*
 * mvcp_socket.h -- Client Socket
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
 * Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#ifndef _MVCP_SOCKET_H_
#define _MVCP_SOCKET_H_

#ifdef __cplusplus
extern "C"
{
#endif

/** Structure for socket.
*/

typedef void *mvcp_socket;

/** Remote parser API.
*/

extern mvcp_socket mvcp_socket_init( char *, int );
extern int mvcp_socket_connect( mvcp_socket );
extern mvcp_socket mvcp_socket_init_fd( int );
extern int mvcp_socket_read_data( mvcp_socket, char *, int );
extern int mvcp_socket_write_data( mvcp_socket, const char *, int );
extern void mvcp_socket_close( mvcp_socket );

#ifdef __cplusplus
}
#endif

#endif

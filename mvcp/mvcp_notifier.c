/*
 * mvcp_notifier.c -- Unit Status Notifier Handling
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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

/* System header files */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/time.h>

/* Application header files */
#include "mvcp_notifier.h"

/** Notifier initialisation.
*/

mvcp_notifier mvcp_notifier_init( )
{
	mvcp_notifier this = calloc( 1, sizeof( mvcp_notifier_t ) );
	if ( this != NULL )
	{
		int index = 0;
		pthread_mutex_init( &this->mutex, NULL );
		pthread_cond_init( &this->cond, NULL );
		for ( index = 0; index < MAX_UNITS; index ++ )
			this->store[ index ].unit = index;
	}
	return this;
}

/** Get a stored status for the specified unit.
*/

void mvcp_notifier_get( mvcp_notifier this, mvcp_status status, int unit )
{
	pthread_mutex_lock( &this->mutex );
	if ( unit >= 0 && unit < MAX_UNITS )
		mvcp_status_copy( status, &this->store[ unit ] );
	else
		memset( status, 0, sizeof( mvcp_status_t ) );
	status->unit = unit;
	status->dummy = time( NULL );
	pthread_mutex_unlock( &this->mutex );
}

/** Wait on a new status.
*/

int mvcp_notifier_wait( mvcp_notifier this, mvcp_status status )
{
	struct timeval now;
	struct timespec timeout;
	int error = 0;

	memset( status, 0, sizeof( mvcp_status_t ) );
	gettimeofday( &now, NULL );
	timeout.tv_sec = now.tv_sec + 1;
	timeout.tv_nsec = now.tv_usec * 1000;
	pthread_mutex_lock( &this->mutex );
	error = pthread_cond_timedwait( &this->cond, &this->mutex, &timeout );
	if ( !error )
		mvcp_status_copy( status, &this->last );
	pthread_mutex_unlock( &this->mutex );

	return error;
}

/** Put a new status.
*/

void mvcp_notifier_put( mvcp_notifier this, mvcp_status status )
{
	pthread_mutex_lock( &this->mutex );
	mvcp_status_copy( &this->store[ status->unit ], status );
	mvcp_status_copy( &this->last, status );
	pthread_cond_broadcast( &this->cond );
	pthread_mutex_unlock( &this->mutex );
}

/** Communicate a disconnected status for all units to all waiting.
*/

void mvcp_notifier_disconnected( mvcp_notifier notifier )
{
	int unit = 0;
	mvcp_status_t status;
	for ( unit = 0; unit < MAX_UNITS; unit ++ )
	{
		mvcp_notifier_get( notifier, &status, unit );
		status.status = unit_disconnected;
		mvcp_notifier_put( notifier, &status );
	}
}

/** Close the notifier - note that all access must be stopped before we call this.
*/

void mvcp_notifier_close( mvcp_notifier this )
{
	if ( this != NULL )
	{
		pthread_mutex_destroy( &this->mutex );
		pthread_cond_destroy( &this->cond );
		free( this );
	}
}

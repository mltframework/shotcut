/*
 * mvcp_notifier.h -- Unit Status Notifier Handling
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

#ifndef _MVCP_NOTIFIER_H_
#define _MVCP_NOTIFIER_H_

/* System header files */
#include <pthread.h>

/* Application header files */
#include "mvcp_status.h"

#ifdef __cplusplus
extern "C"
{
#endif

#define MAX_UNITS 16

/** Status notifier definition.
*/

typedef struct
{
	pthread_mutex_t mutex;
	pthread_cond_t cond;
	mvcp_status_t last;
	mvcp_status_t store[ MAX_UNITS ];
}
*mvcp_notifier, mvcp_notifier_t;

extern mvcp_notifier mvcp_notifier_init( );
extern void mvcp_notifier_get( mvcp_notifier, mvcp_status, int );
extern int mvcp_notifier_wait( mvcp_notifier, mvcp_status );
extern void mvcp_notifier_put( mvcp_notifier, mvcp_status );
extern void mvcp_notifier_disconnected( mvcp_notifier );
extern void mvcp_notifier_close( mvcp_notifier );

#ifdef __cplusplus
}
#endif

#endif

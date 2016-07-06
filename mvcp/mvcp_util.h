/*
 * mvcp_util.h -- General Purpose Client Utilities
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

#ifndef _MVCP_UTIL_H_
#define _MVCP_UTIL_H_

#ifdef __cplusplus
extern "C"
{
#endif

extern char *mvcp_util_chomp( char * );
extern char *mvcp_util_trim( char * );
extern char *mvcp_util_strip( char *, char );

#ifdef __cplusplus
}
#endif

#endif

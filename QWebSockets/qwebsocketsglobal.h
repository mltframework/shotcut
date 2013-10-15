/*
QWebSockets implements the WebSocket protocol as defined in RFC 6455.
Copyright (C) 2013 Kurt Pattyn (pattyn.kurt@gmail.com)

This library is free software; you can redistribute it and/or
modify it under the terms of the GNU Lesser General Public
License as published by the Free Software Foundation; either
version 2.1 of the License, or (at your option) any later version.

This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public
License along with this library; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/

#ifndef QWEBSOCKETSGLOBAL_H
#define QWEBSOCKETSGLOBAL_H

#include <QtCore/qglobal.h>

QT_BEGIN_NAMESPACE

#ifndef QT_STATIC
#  if defined(QT_BUILD_WEBSOCKETS_LIB)
#    define Q_WEBSOCKETS_EXPORT Q_DECL_EXPORT
#  else
#    define Q_WEBSOCKETS_EXPORT Q_DECL_IMPORT
#  endif
#else
#  define Q_WEBSOCKETS_EXPORT
#endif

// The macro has been available only since Qt 5.0
#ifndef Q_DECL_OVERRIDE
#define Q_DECL_OVERRIDE
#endif

QT_END_NAMESPACE
#endif // QWEBSOCKETSGLOBAL_H

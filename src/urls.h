/*
 * Copyright (c) 2011-2026 Meltytech, LLC
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef URLS_H
#define URLS_H

#include <QString>

// Centralized site base URLs so no literal domain is hardcoded at call sites.
//
// kBaseSite / kBaseForum / kBaseCheck point at the real, live upstream Shotcut
// project. Its docs/forum/check-file content still applies to this fork, so
// help links use it until Snapflow has its own equivalent site.
//
// kCurrentSite is Snapflow's own site. It is not live yet; it is reserved for
// this project's own identity (About dialog, source download, update checks) so
// that those don't silently point at a different product's site.
namespace Urls {
static const QString kBaseSite = QStringLiteral("https://www.shotcut.org");
static const QString kBaseForum = QStringLiteral("https://forum.shotcut.org");
static const QString kBaseCheck = QStringLiteral("https://check.shotcut.org");

static const QString kCurrentSite = QStringLiteral("https://www.snapflow.org");
static const QString kCurrentCheck = QStringLiteral("https://check.snapflow.org");
} // namespace Urls

#endif // URLS_H

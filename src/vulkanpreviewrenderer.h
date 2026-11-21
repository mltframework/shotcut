/*
 * Copyright (c) 2026 Meltytech, LLC
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

#ifndef VULKANPREVIEWRENDERER_H
#define VULKANPREVIEWRENDERER_H

class QQuickRhiItemRenderer;

// The player and the HDR window each get one of these. It imports MLT's
// exported image on the window's VkDevice and draws it.
QQuickRhiItemRenderer *createVulkanPreviewRenderer();

#endif

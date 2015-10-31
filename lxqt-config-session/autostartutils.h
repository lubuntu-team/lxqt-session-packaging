/* This file is part of the LXQt project. <http://lxqt.org>
 * Copyright (C) 2015 Luís Pereira <luis.artur.pereira@gmail.com>
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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301 USA
 */

#ifndef AUTOSTARTUTILS_H
#define AUTOSTARTUTILS_H

class XdgDesktopFile;

class AutostartUtils
{
public:
    static bool showOnlyInLXQt(const XdgDesktopFile& file);
    static bool isLXQtModule(const XdgDesktopFile& file);
};

#endif // AUTOSTARTUTILS_H

/***************************************************************************
 *   Copyright (C) 2010-2013 Kai Heitkamp                                  *
 *   dynup@ymail.com | http://dynup.de.vu                                  *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 3 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/

#ifndef GLOBAL_H
#define GLOBAL_H

const QString AppName("WiiBaFu");
const QString AppOrg("dynup");
const QString AppVersion("2.0");

#ifdef Q_OS_WIN
    #define WIIBAFU_SETTINGS QSettings("WiiBaFu.ini", QSettings::IniFormat)
#else
    #define WIIBAFU_SETTINGS QSettings("WiiBaFu", "wiibafu")
#endif

#ifdef Q_OS_WIN
    #define DEFAULT_WIT_PATH QDir::homePath().append("/wit/bin")
    #define DEFAULT_TITLES_PATH QDir::homePath().append("/wit/bin")
#else
    #define DEFAULT_WIT_PATH QString("/usr/local/bin")
    #define DEFAULT_TITLES_PATH QString("/usr/local/share/wit")
#endif

#endif // GLOBAL_H

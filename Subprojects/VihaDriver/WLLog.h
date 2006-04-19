/* $Id: WLLog.h,v 1.2 2003/11/18 21:11:00 kismac Exp $ */

/* Copyright (C) 2003 Dino Dai Zovi <ddz@theta44.org>
 *
 * This file is part of Viha.
 *
 * Viha is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * Viha is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public
 * License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Viha; if not, write to the Free Software Foundation,
 * Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 */

/*
 * Wireless LAN driver logging functions
 */

#ifndef WLLOG_H
#define WLLOG_H

#include <IOKit/IOLib.h>

//#define DEBUG

#if defined(DEBUG)
  #define logLevel 7
#else
  #define logLevel 2
#endif

#define WLLog(level, ...) \
    if (level <= logLevel) { IOLog("WLanDriver: "); IOLog(__VA_ARGS__); }

#define WLLogEmerg(...)   WLLog(0, __VA_ARGS__)
#define WLLogAlert(...)   WLLog(1, __VA_ARGS__)
#define WLLogCrit(...)    WLLog(2, __VA_ARGS__)
#define WLLogErr(...)     WLLog(3, __VA_ARGS__)
#define WLLogWarn(...)    WLLog(4, __VA_ARGS__)
#define WLLogNotice(...)  WLLog(5, __VA_ARGS__)
#define WLLogInfo(...)    WLLog(6, __VA_ARGS__)
#define WLLogDebug(...)   WLLog(7, __VA_ARGS__)

#endif

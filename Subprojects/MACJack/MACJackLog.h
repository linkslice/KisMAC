/*
        
        File:			MACJackLog.h
        Program:		MACJack
	Author:			Dino Dai Zovi, Michael Rossberg
				mick@binaervarianz.de
	Description:		KisMAC is a wireless stumbler for MacOS X.
                
        This file is part of KisMAC.

        This file is mostly taken from the Viha driver.

    KisMAC is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    KisMAC is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with KisMAC; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

/*
 * Wireless LAN driver logging functions
 */

#ifndef MACJACKLOG_H
#define MACJACKLOG_H

#include <IOKit/IOLib.h>

//#define DEBUG

#if defined(DEBUG)
  #define logLevel 7
#else
  #define logLevel 3
#endif

#define WLLog(level, ...) if (level <= logLevel) { IOLog("MACJack: "); IOLog(__VA_ARGS__); }

#define WLLogEmerg(...)   WLLog(0, __VA_ARGS__)
#define WLLogAlert(...)   WLLog(1, __VA_ARGS__)
#define WLLogCrit(...)    WLLog(2, __VA_ARGS__)
#define WLLogErr(...)     WLLog(3, __VA_ARGS__)
#define WLLogWarn(...)    WLLog(4, __VA_ARGS__)
#define WLLogNotice(...)  WLLog(5, __VA_ARGS__)
#define WLLogInfo(...)    WLLog(6, __VA_ARGS__)
#define WLLogDebug(...)   WLLog(7, __VA_ARGS__)

#endif

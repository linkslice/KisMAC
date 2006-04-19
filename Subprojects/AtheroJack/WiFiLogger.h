/*
        
        File:			WiFiLogger.h
        Program:		AtheroJack
		Author:			Michael Rossberg
						mick@binaervarianz.de
		Description:	AtheroJack is a free driver monitor mode driver for Atheros cards.
                
        This file is part of AtheroJack.

    AtheroJack is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    AtheroJack is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with AtheroJack; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

/*
 * Wireless LAN driver logging functions
 */

#include <IOKit/IOLib.h>

#ifndef WiFiDriverLOG_H
#define WiFiDriverLOG_H

//#define DEBUG

#if defined(DEBUG)
  #define logLevel 7
#else
  #define logLevel 3
#endif

#if defined(DEBUG)
#define WLLog(level, ...) \
    if (level <= logLevel) { IOLog("AtheroJack: %s %s: %u: ", __FILE__ ,__FUNCTION__, __LINE__); IOLog(__VA_ARGS__); IOLog("\n"); }
#else
#define WLLog(level, ...) \
    if (level <= logLevel) { IOLog("AtheroJack: %s: %u: ",__FUNCTION__, __LINE__); IOLog(__VA_ARGS__); IOLog("\n"); }
#endif

#define WLLogEmerg(...)   WLLog(0, __VA_ARGS__)
#define WLLogAlert(...)   WLLog(1, __VA_ARGS__)
#define WLLogCrit(...)    WLLog(2, __VA_ARGS__)
#define WLLogErr(...)     WLLog(3, __VA_ARGS__)

#if defined(DEBUG)
    #define WLLogWarn(...)    WLLog(4, __VA_ARGS__)
    #define WLLogNotice(...)  WLLog(5, __VA_ARGS__)
    #define WLLogInfo(...)    WLLog(6, __VA_ARGS__)
    #define WLLogDebug(...)   WLLog(7, __VA_ARGS__)
#else
    #define WLLogWarn(...) 
    #define WLLogNotice(...)
    #define WLLogInfo(...) 
    #define WLLogDebug(...)
#endif

#if defined(DEBUG)
    #define WLEnter() \
        WLLogDebug("entered");

    #define WLEndProtStart() __debug = 0;
    #define WLEndProtMaxLoops(s) \
        if (s < (++__debug)) { \
            WLLogEmerg("WARNING: endless loop detected!!!") \
            break; \
        }
#else
    #define WLEnter() WLLogDebug("entered");
#endif
    
#define WLExit()  WLLogDebug("returned")
#define WLReturn(s) { WLLogDebug("returned"); return s; }


#endif

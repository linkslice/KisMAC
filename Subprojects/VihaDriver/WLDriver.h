/* $Id: WLDriver.h,v 1.2 2003/11/18 21:11:00 kismac Exp $ */

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
 * Wireless LAN Driver for Darwin/OS X
 */

#ifndef WLANDRIVER_H
#define WLANDRIVER_H

#include <IOKit/IOCommandGate.h>
#include <IOKit/IOService.h>
#include <IOKit/IOInterruptEventSource.h>
#include <IOKit/IOTimerEventSource.h>
#include <IOKit/platform/AppleMacIODevice.h>
#include <IOKit/IODeviceMemory.h>
#include <IOKit/IODataQueue.h>

#include "WLLog.h"
#include "WLCard.h"

class WLanDriver : public IOService {
    OSDeclareDefaultStructors(WLanDriver);

public:
    // driver startup and shutdown
    virtual bool init(OSDictionary * = 0);
    virtual bool start(IOService* provider);
    virtual void stop(IOService* provider);
    virtual void free();

    virtual IOWorkLoop* getWorkLoop();

    bool handleOpen(IOService*, IOOptionBits, void*);
    void handleClose(IOService*, IOOptionBits);

    WLCard* getWLCard() { return _wlCard; }
    IODataQueue* getPacketQueue() { return _packetQueue; }
    
    static void _interruptOccurred(OSObject*, IOInterruptEventSource*, int);
    static void _timeoutOccurred(OSObject*, IOTimerEventSource*);

    void handleInterrupt(IOInterruptEventSource*, int);
    void handleTimeout(IOTimerEventSource*);

    // Power management methods
    virtual IOReturn
	powerStateDidChangeTo(IOPMPowerFlags, unsigned long, IOService*);

    virtual IOReturn
	powerStateWillChangeTo(IOPMPowerFlags, unsigned long, IOService*);

private:
    /*
     * Private methods
     */
    bool _initEventSources();

    bool _openProvider();
    void _closeProvider();

    /*
     * Private data members
     */
    AppleMacIODevice*       _nub;
    IOService*              _keyLargo;
    IOVirtualAddress        _klioBase;
    SInt32                  _openCount;
    IOWorkLoop*             _workLoop;
    IOInterruptEventSource* _interruptEventSource;
    IOTimerEventSource*     _timerEventSource;
    IOCommandGate*          _userCommandGate;

    WLCard*                 _wlCard;
    IODataQueue*            _packetQueue;
};

#endif

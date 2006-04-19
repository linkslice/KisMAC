/*
        
        File:			AiroJackDriver.h
        Program:		AiroJack
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
 * Wireless LAN Driver for Darwin/OS X
 */

#ifndef AiroJackDRIVER_H
#define AiroJackDRIVER_H

#include <IOKit/IOCommandGate.h>
#include <IOKit/IOService.h>
#include <IOKit/IOInterruptEventSource.h>
#include <IOKit/IOTimerEventSource.h>
#include <IOKit/pccard/IOPCCard.h>
#include <IOKit/IODeviceMemory.h>
#include <IOKit/IODataQueue.h>

#include "AiroJackLog.h"
#include "AiroJackCard.h"

class AiroJackDriver : public IOService {
    OSDeclareDefaultStructors(AiroJackDriver);

public:
    // driver startup and shutdown
    virtual bool init(OSDictionary * = 0);
    virtual bool start(IOService* provider);
    virtual void stop(IOService* provider);
    virtual void free();

    virtual IOWorkLoop* getWorkLoop();
    virtual IOReturn message(UInt32 type, IOService *provider, void *argument = 0);

    bool handleOpen(IOService*, IOOptionBits, void*);
    void handleClose(IOService*, IOOptionBits);

    AiroJackCard* getWLCard() { return _wlCard; }
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
    IOPCCard16Device*       _nub;
    IOPCCardBridge*          _parent;
    SInt32                  _openCount;
    IOWorkLoop*             _workLoop;
    IOInterruptEventSource* _interruptEventSource;
    IOTimerEventSource*     _timerEventSource;
    IOCommandGate*          _userCommandGate;

    UInt32		    currentPowerState;
    AiroJackCard*            _wlCard;
    IODataQueue*            _packetQueue;
    IOMemoryMap*            _map;
};

#endif

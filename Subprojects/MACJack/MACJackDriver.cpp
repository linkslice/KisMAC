/*
        
        File:			MACJackDriver.cpp
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


#include <IOKit/IODeviceTreeSupport.h>
#include "MACJackDriver.h"
#include "MACJackUserClient.h"

#include <IOKit/pccard/cs.h>
#include <IOKit/pccard/version.h>
#include <IOKit/pccard/cs_types.h>
#include <IOKit/pccard/ss.h>
#include <IOKit/pccard/cs.h>
#include <IOKit/pccard/bulkmem.h>
#include <IOKit/pccard/cistpl.h>
#include <IOKit/pccard/cisreg.h>
#include <IOKit/pccard/bus_ops.h>

#define super IOService

OSDefineMetaClassAndStructors(MACJackDriver, IOService);

bool MACJackDriver::init(OSDictionary* properties) {
    WLLogInfo("MACJackDriver::init: start\n");
    
    if (super::init(properties) == false) {
	WLLogErr("MACJackDriver::init: super::init failed\n");
	return false;
    }

    /*
     * FIXME - Figure out what personality we are
     * (AirPort/Orinoco/PrismII/Aironet) and configure ourselves
     * accordingly.
     */
    
    return true;
}

/*
 * The start() function is responsible for setting up the resources
 * the driver needs all the time, whether the driver is enabled or
 * disabled.  These include the network interface object, a work loop,
 * and an output queue, along with whatever specific resources the
 * driver needs.
 */
bool MACJackDriver::start(IOService* provider) {
    WLLogInfo("MACJackDriver::start: start\n");
    
    if (!super::start(provider)) {
	WLLogErr("MACJackDriver::start: super::start failed\n");
	return false;
    }

    /*
     * Verify that the provider is non-null and is the appropriate
     * class.
     *
     * XXX: AppleMacIODevice is valid for the AirPort, does it work
     * PC cards too?
     */
    _nub = OSDynamicCast(IOPCCard16Device, provider);
    if (!_nub) {
	WLLogCrit("MACJackDriver::start: Null nub\n");
	return false;
    }
    
    // Open provider
    if (!_openProvider()) {
	WLLogErr("MACJackDriver::start: Couldn't open provider\n");
	return false;
    }

    /*
     * From this point on, we must close the provider if something
     * fails and we abort.
     */
    bool ret = true;
    do {
        WLLogDebug("MACJackDriver::start: Configuring the card\n");
        _nub->configure();
         
        /*
	 * Get a memory map and virtual base address and use the base
	 * address to instantiate a WLCard.  The WLCard will
	 * figure out what card/vendor/revision it is and act
	 * accordingly.
	 */
	WLLogDebug("MACJackDriver::start: Mapping the card\n");
        _map = _nub->mapDeviceMemoryWithIndex(0);
	if (!_map) {
	    WLLogErr("MACJackDriver::start: Couldn't map device memory\n");
	    ret = false;
	    break;
	}
	
        WLLogDebug("MACJackDriver::start: Getting the virtual address of the card\n");
        IOVirtualAddress ioBase = _map->getVirtualAddress();
        if (ioBase==0) {
            WLLogErr("MACJackDriver::start: Got an empty ioBase!\n");
	    ret = false;
	    break;
	
        }
        
        WLLogDebug("MACJackDriver::start: Get the parent\n");
        _parent =
	    OSDynamicCast(IOService, _nub->getParentEntry(gIOServicePlane));

	if (!_parent) {
	    WLLogErr("MACJackDriver::start: Couldn't get parent reference\n");
	    ret = false;
	    break;
	}
        
        WLLogDebug("MACJackDriver::start: Register for Power Management\n");
        _parent->registerInterestedDriver(this);
        
	/*
	 * Instantiate AirPort card
	 */
        WLLogDebug("MACJackDriver::start: Start the card\n");
	_wlCard = new MACJackCard((void*)ioBase, NULL, this);

        /*
         * Create a packet queue
         */
        UInt32 pqSize = 16 * 1024;
        _packetQueue = IODataQueue::withCapacity(pqSize);
        
	/*
	 * Initialize work loop and event sources
	 */
	ret = _initEventSources();
	if (!ret)
	    break;

        /*
         * Register driver in registry
         */	
	registerService();
    } while (0);

    if (!ret) {
        _nub->unconfigure();
        _closeProvider();
    }
    return ret;
}

bool MACJackDriver::_initEventSources() {
    /*
     * Get our work loop and add interrupt, watchdog timer, and user
     * client command event sources to it.
     */
    IOWorkLoop* wl = getWorkLoop();
    if (!wl) {
	WLLogErr("MACJackDriver::start: Couldn't get IOWorkLoop\n");
	return false;
    }    

    /* Create an interrupt event source */
    _interruptEventSource = IOInterruptEventSource::
	interruptEventSource(this, (IOInterruptEventAction)
			     &MACJackDriver::_interruptOccurred, _nub, 0);
    
    if (!_interruptEventSource) {
	WLLogErr("MACJackDriver::start: Couldn't get interrupt event source\n");
	return false;
    }
    else if (wl->addEventSource(_interruptEventSource) != kIOReturnSuccess) {
	WLLogErr("MACJackDriver::start: Couldn't add interrupt event source "
                 "to work loop\n");
	return false;
    }
    
    /* Create a timer event source */
    _timerEventSource = IOTimerEventSource::
	timerEventSource(this, (IOTimerEventSource::Action)
			 &MACJackDriver::_timeoutOccurred);
    if (!_timerEventSource) {
	WLLogErr("MACJackDriver::start: Couldn't get timer event source\n");
	return false;
    }
    else if (wl->addEventSource(_timerEventSource) != kIOReturnSuccess) {
	WLLogErr("MACJackDriver::start: Couldn't add timer event source "
                 "to work loop\n");
	return false;
    }
    
    /* Create a command gate for user client commands */
    _userCommandGate = IOCommandGate::commandGate(this);
    if (!_userCommandGate) {
	WLLogErr("MACJackDriver::start: Couldn't get command gate\n");
	return false;
    }
    else if (wl->addEventSource(_userCommandGate) != kIOReturnSuccess) {
	WLLogErr("MACJackDriver::start: Couldn't add cmd gate to work loop\n");
	return false;
    }

    return true;
}

/*
 * The stop() function simply reverses all the actions taken by
 * start(), releasing the nub and disposing of any resources created
 * in start().
 */
void MACJackDriver::stop(IOService* provider) {
    
    if (_userCommandGate) {
        getWorkLoop()->removeEventSource(_userCommandGate);
        _userCommandGate->release();
        _userCommandGate = 0;
    }
    
    if (_timerEventSource) {
	_timerEventSource->disable();
	getWorkLoop()->removeEventSource(_timerEventSource);
	_timerEventSource->release();
	_timerEventSource = 0;
    }

    if (_interruptEventSource) {
	_interruptEventSource->disable();
        getWorkLoop()->removeEventSource(_interruptEventSource);
        _interruptEventSource->release();
        _interruptEventSource = 0;
    }
    
    if (_workLoop) {
        _workLoop->release();
        _workLoop = 0;
    }
    
    if (_map) {
        _map->release();
        _map = 0;
    }
    
    if (_nub) {
        _nub->unconfigure();
    }
    
    if (_parent) {
        _parent->setPowerState(kIOPCCard16DeviceOffState,_parent);
	//_parent->requestCardEjection(_parent);
        _parent->deRegisterInterestedDriver(this);
	_parent->setPowerState(kIOPCCard16DeviceOnState,_parent);
	_parent = 0;
    }
    
    _closeProvider();
    
    super::stop(provider);
}

void MACJackDriver::free() {
    WLLogInfo("MACJackDriver::free()\n");
    super::free();
}

IOWorkLoop* MACJackDriver::getWorkLoop() {
    if (!_workLoop)
        _workLoop = IOWorkLoop::workLoop();
        
    return _workLoop;
}

IOReturn MACJackDriver::message(UInt32 type, IOService * provider, void * argument) {
    if (type == kIOPCCardCSEventMessage) {
	WLLogInfo("MACJackDriver::message: nub=%p, CS event received type=0x%x.\n", _nub, (unsigned int)argument);

        switch ((unsigned int)argument) {
	    case CS_EVENT_CARD_REMOVAL:
		WLLogInfo("MACJackDriver::message: PC Card was removed\n");
                
                if (_wlCard) _wlCard->cardGone();
		
		// Message our clients and tell them we're no longer available...
		super::message(kIOMessageServiceIsTerminated, this, 0);
                this->terminate(kIOServiceRequired); // we're gone, lets save memory.
		break;
            case CS_EVENT_CARD_INSERTION:
                WLLogInfo("MACJackDriver::message: PC Card was inserted. \n");
                //TODO Handle this              
		break;
	    default:
		WLLogInfo("MACJackDriver::message: PC Card generated untrapped message: %ud\n", (unsigned int)argument);
		break;
        }
    }

    return super::message(type, provider, argument);
}

bool MACJackDriver::
handleOpen(IOService* forClient, IOOptionBits options, void* arg) {
    if (!super::handleOpen(forClient, options, arg)) {
	WLLogErr("MACJackDriver::handleOpen: super::handleOpen failed.\n");
	return false;
    }

    if (!_openProvider()) {
	WLLogErr("MACJackDriver::handleOpen: _openProvider failed\n");
	super::handleClose(forClient, 0);
	return false;
    }

    WLLogInfo("MACJackDriver::handleOpen\n");

    return true;
}

void MACJackDriver::
handleClose(IOService* forClient, IOOptionBits options) {
    WLLogInfo("MACJackDriver::handleClose\n");
    _closeProvider();
    super::handleClose(forClient, options);
}

bool MACJackDriver::
_openProvider() {
    if (OSIncrementAtomic(&_openCount) > 0)
        return true;
        
    if (!_nub->open(this)) {
        WLLogErr("MACJackDriver::_openProvider: Couldn't open nub\n");
        return false;
    }
    
    return true;
}

void MACJackDriver::
_closeProvider() {
    if (OSDecrementAtomic(&_openCount) > 1)
	return;
    _nub->close(this);
}

/*
 * Static event-handlers just call the virtual handler functions
 */
void MACJackDriver::
_interruptOccurred(OSObject* o, IOInterruptEventSource* src, int n) {
    ((MACJackDriver*)o)->handleInterrupt(src, n);
}

void MACJackDriver::
_timeoutOccurred(OSObject* o, IOTimerEventSource* src) {
    ((MACJackDriver*)o)->handleTimeout(src);
}

void MACJackDriver::
handleInterrupt(IOInterruptEventSource* eventSource, int count) {
    _wlCard->handleInterrupt(); 
}

void MACJackDriver::
handleTimeout(IOTimerEventSource* eventSource) {
    WLLogInfo("MACJackDriver::handleTimeout()\n");
}


/*********************************************************************
 *                         Power Management                          *
 *********************************************************************/

IOReturn MACJackDriver::
powerStateWillChangeTo(IOPMPowerFlags capabilities,
		      unsigned long stateNumber,
		      IOService* whatDevice) {
    WLLogNotice("MACJackDriver::powerStateWillChangeTo: %ld\n", stateNumber);

    return IOPMAckImplied;
}

IOReturn MACJackDriver::
powerStateDidChangeTo(IOPMPowerFlags capabilities,
		      unsigned long stateNumber,
		      IOService* whatDevice) {
    WLLogNotice("MACJackDriver::powerStateDidChangeTo: %ld\n", stateNumber);

    if (stateNumber == kIOPCCard16DeviceOnState) {
        IOSleep(200);
        _wlCard->wakeUp();
        _wlCard->_reset();
    } else {
        _wlCard->goSleep();
    }

    return IOPMAckImplied;
}

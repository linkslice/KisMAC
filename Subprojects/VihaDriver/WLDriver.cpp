/* $Id: WLDriver.cpp,v 1.2 2003/11/18 21:11:00 kismac Exp $ */

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

#include <IOKit/IODeviceTreeSupport.h>
#include "WLDriver.h"
#include "WLUserClient.h"

#define super IOService

OSDefineMetaClassAndStructors(WLanDriver, IOService);

bool WLanDriver::init(OSDictionary* properties) {
    if (super::init(properties) == false) {
	WLLogErr("WLanDriver::init: super::init failed\n");
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
bool WLanDriver::start(IOService* provider) {
    if (!super::start(provider)) {
	WLLogErr("WLanDriver::start: super::start failed\n");
	return false;
    }

    /*
     * Verify that the provider is non-null and is the appropriate
     * class.
     *
     * XXX: AppleMacIODevice is valid for the AirPort, does it work
     * PC cards too?
     */
    _nub = OSDynamicCast(AppleMacIODevice, provider);
    if (!_nub) {
	WLLogCrit("WLanDriver::start: Null nub\n");
	return false;
    }

    // Open provider
    if (!_openProvider()) {
	WLLogErr("WLanDriver::start: Couldn't open provider\n");
	return false;
    }

    /*
     * From this point on, we must close the provider if something
     * fails and we abort.
     */
    bool ret = true;
    do {
	/*
	 * Get a memory map and virtual base address and use the base
	 * address to instantiate a WLCard.  The WLCard will
	 * figure out what card/vendor/revision it is and act
	 * accordingly.
	 */
	IOMemoryMap* map = _nub->mapDeviceMemoryWithIndex(0);
	if (!map) {
	    WLLogErr("WLanDriver::start: Couldn't map device memory\n");
	    ret = false;
	    break;
	}
	IOVirtualAddress ioBase = map->getVirtualAddress();

	/*
	 * Get a reference to KeyLargo base address
	 */
	_keyLargo =
	    OSDynamicCast(IOService,
			  _nub->getParentEntry(gIODTPlane));

	if (!_keyLargo) {
	    WLLogErr("WLanDriver::start: Couldn't get KeyLargo ref\n");
	    ret = false;
	    break;
	}

        _keyLargo->registerInterestedDriver(this);
        
	IOMemoryMap * klmap = _keyLargo->mapDeviceMemoryWithIndex(0);
	if (!klmap) {
	    WLLogErr("WLanDriver::start: Couldn't get KeyLargo map\n");
	    ret = false;
	    break;
	}
	_klioBase = klmap->getVirtualAddress();

	/*
	 * Instantiate AirPort card
	 */
	_wlCard = new WLCard((void*)ioBase, (void*)_klioBase, this);

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

    _closeProvider();
    return ret;
}

bool WLanDriver::_initEventSources() {
    /*
     * Get our work loop and add interrupt, watchdog timer, and user
     * client command event sources to it.
     */
    IOWorkLoop* wl = getWorkLoop();
    if (!wl) {
	WLLogErr("WLanDriver::start: Couldn't get IOWorkLoop\n");
	return false;
    }    

    /* Create an interrupt event source */
    _interruptEventSource = IOInterruptEventSource::
	interruptEventSource(this, (IOInterruptEventAction)
			     &WLanDriver::_interruptOccurred, _nub, 0);
    
    if (!_interruptEventSource) {
	WLLogErr("WLanDriver::start: Couldn't get interrupt event source\n");
	return false;
    }
    else if (wl->addEventSource(_interruptEventSource) != kIOReturnSuccess) {
	WLLogErr("WLanDriver::start: Couldn't add interrupt event source "
                 "to work loop\n");
	return false;
    }
    
    /* Create a timer event source */
    _timerEventSource = IOTimerEventSource::
	timerEventSource(this, (IOTimerEventSource::Action)
			 &WLanDriver::_timeoutOccurred);
    if (!_timerEventSource) {
	WLLogErr("WLanDriver::start: Couldn't get timer event source\n");
	return false;
    }
    else if (wl->addEventSource(_timerEventSource) != kIOReturnSuccess) {
	WLLogErr("WLanDriver::start: Couldn't add timer event source "
                 "to work loop\n");
	return false;
    }
    
    /* Create a command gate for user client commands */
    _userCommandGate = IOCommandGate::commandGate(this);
    if (!_userCommandGate) {
	WLLogErr("WLanDriver::start: Couldn't get command gate\n");
	return false;
    }
    else if (wl->addEventSource(_userCommandGate) != kIOReturnSuccess) {
	WLLogErr("WLanDriver::start: Couldn't add cmd gate to work loop\n");
	return false;
    }

    return true;
}

/*
 * The stop() function simply reverses all the actions taken by
 * start(), releasing the nub and disposing of any resources created
 * in start().
 */
void WLanDriver::stop(IOService* provider) {
    if (_keyLargo) {
	_keyLargo->deRegisterInterestedDriver(this);
	_keyLargo = 0;
    }

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
    
    super::stop(provider);
}

void WLanDriver::free() {
    WLLogInfo("WLanDriver::free()\n");
    super::free();
}

IOWorkLoop* WLanDriver::getWorkLoop() {
    if (!_workLoop)
        _workLoop = IOWorkLoop::workLoop();
        
    return _workLoop;
}

bool WLanDriver::
handleOpen(IOService* forClient, IOOptionBits options, void* arg) {
    if (!super::handleOpen(forClient, options, arg)) {
	WLLogErr("WLanDriver::handleOpen: super::handleOpen failed.\n");
	return false;
    }

    if (!_openProvider()) {
	WLLogErr("WLanDriver::handleOpen: _openProvider failed\n");
	super::handleClose(forClient, 0);
	return false;
    }

    WLLogInfo("WLanDriver::handleOpen\n");

    return true;
}

void WLanDriver::
handleClose(IOService* forClient, IOOptionBits options) {
    WLLogInfo("WLanDriver::handleClose\n");
    _closeProvider();
    super::handleClose(forClient, options);
}

bool WLanDriver::
_openProvider() {
    if (OSIncrementAtomic(&_openCount) > 0)
        return true;
        
    if (!_nub->open(this)) {
        WLLogErr("WLanDriver::_openProvider: Couldn't open nub\n");
        return false;
    }
    
    return true;
}

void WLanDriver::
_closeProvider() {
    if (OSDecrementAtomic(&_openCount) > 1)
	return;
    _nub->close(this);
}

/*
 * Static event-handlers just call the virtual handler functions
 */
void WLanDriver::
_interruptOccurred(OSObject* o, IOInterruptEventSource* src, int n) {
    ((WLanDriver*)o)->handleInterrupt(src, n);
}

void WLanDriver::
_timeoutOccurred(OSObject* o, IOTimerEventSource* src) {
    ((WLanDriver*)o)->handleTimeout(src);
}

void WLanDriver::
handleInterrupt(IOInterruptEventSource* eventSource, int count) {
    _wlCard->handleInterrupt(); 
}

void WLanDriver::
handleTimeout(IOTimerEventSource* eventSource) {
    WLLogInfo("WLanDriver::handleTimeout()\n");
}


/*********************************************************************
 *                         Power Management                          *
 *********************************************************************/

IOReturn WLanDriver::
powerStateWillChangeTo(IOPMPowerFlags capabilities,
		      unsigned long stateNumber,
		      IOService* whatDevice) {
    WLLogInfo("WLanDriver::powerStateWillChangeTo: %ld\n", stateNumber);

    /* XXX: Change to symbolic constants */
    if (stateNumber == 0)
        _wlCard->_powerOff();
    else if (stateNumber == 2) {
        _wlCard->_powerOn();
        IODelay(200 * 1000);
        _wlCard->_reset();
    }

    
    return IOPMAckImplied;
}

IOReturn WLanDriver::
powerStateDidChangeTo(IOPMPowerFlags capabilities,
		      unsigned long stateNumber,
		      IOService* whatDevice) {
    WLLogInfo("WLanDriver::powerStateDidChangeTo: %ld\n", stateNumber);

    return IOPMAckImplied;
}

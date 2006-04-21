/*
        
        File:			WiFiController.cpp
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

#include "WiFiController.h"

#include <IOKit/pccard/IOPCCard.h>

#define ONE_SECOND_TICKS			1000
#define LOAD_STATISTICS_INTERVAL	(30 * ONE_SECOND_TICKS)

#define super IOEthernetController
OSDefineMetaClassAndStructors(WiFiController, IOEthernetController)

extern "C" {
#include <sys/sockio.h>
}

//---------------------------------------------------------------------------
// Function: initDriver
//
// Create and initialize driver objects before the hardware is 
// enabled.
//
// Returns true on sucess, and false if initialization failed.

bool WiFiController::_initDriver(IOService * provider) {
    //currentMediumType = MEDIUM_TYPE_INVALID;

    // This driver will allocate and use an IOGatedOutputQueue.
    //
    _transmitQueue = getOutputQueue();
    if (_transmitQueue == NULL) return false;

    // Allocate two IOMbufLittleMemoryCursor instances. One for transmit and
    // the other for receive.
    //
    //rxMbufCursor = IOMbufLittleMemoryCursor::withSpecification(MAX_BUF_SIZE,1);
    //txMbufCursor = IOMbufLittleMemoryCursor::withSpecification(MAX_BUF_SIZE, 
    //                                                           TBDS_PER_TCB);
    //if (!rxMbufCursor || !txMbufCursor)
    //        return false;

    // Get a handle to our superclass' workloop.
    //
    IOWorkLoop *myWorkLoop = (IOWorkLoop *) getWorkLoop();
    if (!myWorkLoop) return false;

    // Create and register an interrupt event source. The provider will
    // take care of the low-level interrupt registration stuff.
    //
    _interruptSrc = IOInterruptEventSource::interruptEventSource(
               (OSObject *) this,
               (IOInterruptEventAction) & WiFiController::interruptOccurred,
               provider);

    if (!_interruptSrc ||
		(myWorkLoop->addEventSource(_interruptSrc) != kIOReturnSuccess))
		return false;

    // This is important. If the interrupt line is shared with other devices,
    // then the interrupt vector will be enabled only if all corresponding
    // interrupt event sources are enabled. To avoid masking interrupts for
    // other devices that are sharing the interrupt line, the event source
    // is enabled immediately.

    _interruptSrc->enable();

    // Register a timer event source. This is used as a watchdog timer.
    //
    _timerSrc = IOTimerEventSource::timerEventSource(this,
           (IOTimerEventSource::Action)&WiFiController::timeoutOccurred);
    if (!_timerSrc ||
    	(myWorkLoop->addEventSource(_timerSrc) != kIOReturnSuccess))
    	return false;

    // Create a dictionary to hold IONetworkMedium objects.
    //
    _mediumDict = OSDictionary::withCapacity(MEDIUM_TYPE_INVALID + 1);
    if (!_mediumDict)
    	return false;

    _addMediumType(kIOMediumIEEE80211None,  0,  MEDIUM_TYPE_NONE);
    _addMediumType(kIOMediumIEEE80211Auto,  0,  MEDIUM_TYPE_AUTO);
    /*_addMediumType(kIOMediumIEEE80211DS1,   1000000, MEDIUM_TYPE_1MBIT);
    _addMediumType(kIOMediumIEEE80211DS2,   2000000, MEDIUM_TYPE_2MBIT);
    _addMediumType(kIOMediumIEEE80211DS5,   5500000, MEDIUM_TYPE_5MBIT);
    _addMediumType(kIOMediumIEEE80211DS11, 11000000, MEDIUM_TYPE_11MBIT);
    _addMediumType(kIOMediumIEEE80211,     54000000, MEDIUM_TYPE_54MBIT, "OFDM54");
     */   
    _currentMediumType = MEDIUM_TYPE_AUTO;

    setLinkStatus(kIONetworkLinkValid, _getMediumWithType(MEDIUM_TYPE_NONE));

    return true;
}

//---------------------------------------------------------------------------
// Function: start <IOService>
//
// Hardware was detected and initialized, start the driver.

bool WiFiController::start(IOService * provider) {
    bool ret     = false;
    bool started = false;

    memset(_ssid, 0, sizeof(_ssid));
    _ssidLength = 0;
    _linkSpeed = 0;
    _currentState = stateIntializing;
    _currentMode = modeClient;
    _currentFrequency = 2412;
    _packetQueue = IODataQueue::withCapacity(16384); // allocate 16KB for caching output in monitor mode
    
    do {
        started = true;
        
        // Start our superclass first.
        if (super::start(provider) == false)
            break;

        if (startProvider(provider) == false) break;
        
        // Initialize the driver's event sources and other support objects.
        if (_initDriver(provider) == false) break;
        
        // Get default driver settings (stored in property table).
        //if (getDefaultSettings() == false) break;

        // Execute one-time initialization code.
        if (startHardware() == false) break;
        
        if (publishMediumDictionary(_mediumDict) == false) {
            WLLogErr("publishMediumDictionary failed\n");
            break;
        }
        
        ret = true;
    }
    while (false);

    _cardGone = false;
    
    // Close our provider, it will be re-opened on demand when
    // our enable() method is called.
    closeProvider();

    do {
        if (ret == false) break;

        ret = false;

        // Allocate and attach an IOEthernetInterface instance to this driver
        // object.

        if (attachInterface((IONetworkInterface **) &_netif, false) == false) break;
        //_netif->setARPHeaderMode(DLT_IEEE802_11);
 
        // Start matching for clients of IONetworkInterface.
        _netif->registerService();
        registerService();
        
        ret = true;
    }
    while (false);

    // Issue a stop on failure.
    if (started && !ret) super::stop(provider);

    return ret;
}

//---------------------------------------------------------------------------
// Function: createWorkLoop <IONetworkController>
//
// Override IONetworkController::createWorkLoop() method to create and return
// a new work loop object.

bool WiFiController::createWorkLoop() {
    _workLoop = IOWorkLoop::workLoop();

    return (_workLoop != NULL);
}

//---------------------------------------------------------------------------
// Function: getWorkLoop <IOService>
//
// Override IOService::getWorkLoop() method and return a reference to our
// work loop.

IOWorkLoop * WiFiController::getWorkLoop() const {
    return _workLoop;
}

//---------------------------------------------------------------------------
// Function: configureInterface <IONetworkController>
//
// Configure a newly instantiated IONetworkInterface object.

bool WiFiController::configureInterface(IONetworkInterface * netif) {
    IONetworkData * data;
    
    if (super::configureInterface(netif) == false)
            return false;
    
    // Get the generic network statistics structure.

    data = netif->getParameter(kIONetworkStatsKey);
    if (!data || !(_netStats = (IONetworkStats *)data->getBuffer())) {
            return false;
    }

    // Get the Ethernet statistics structure.

    data = netif->getParameter(kIOEthernetStatsKey);
    if (!data || !(_etherStats = (IOEthernetStats *)data->getBuffer())) {
            return false;
    }
    
    return true;
}

//---------------------------------------------------------------------------
// Function: free <IOService>
//
// Deallocate all resources and destroy the instance.

void WiFiController::free() {
    #define RELEASE(x) do { if(x) { (x)->release(); (x) = 0; } } while(0)

    RELEASE(_netif);

    if (_interruptSrc && _workLoop) {
        _workLoop->removeEventSource(_interruptSrc);
    }

    freeHardware();

    RELEASE(_interruptSrc);
    RELEASE(_timerSrc    );
    RELEASE(_mediumDict  );
    RELEASE(_workLoop    );
    RELEASE(_packetQueue );
    
    freeProvider();
    
    if (_powerOffThreadCall) {
        thread_call_free(_powerOffThreadCall);
        _powerOffThreadCall = 0;
    }
    
    if (_powerOnThreadCall) {
        thread_call_free(_powerOnThreadCall);
        _powerOnThreadCall = 0;
    }

    super::free();	// pass it to our superclass
}

//---------------------------------------------------------------------------
// Function: enableAdapter
//
// Enables the adapter & driver to the given level of support.

bool WiFiController::enableAdapter() {
    // Open provider.
    //
    if (!openProvider()) {
        WLLogErr("Could not open Provider");
        return false;
    }
    if (!enableHardware()) {
        WLLogErr("Could not enable Hardware");
        return false;
    }
    
    // Start our IOOutputQueue object.
    //
    _transmitQueue->setCapacity(TRANSMIT_QUEUE_SIZE);
    _transmitQueue->start();

    _timerSrc->setTimeoutMS(LOAD_STATISTICS_INTERVAL);
    
    return true;
}

//---------------------------------------------------------------------------
// Function: disableAdapter
//
// Disables the adapter & driver to the given level of support.

bool WiFiController::disableAdapter() {        
    WLEnter();
    
    _timerSrc->cancelTimeout();
    _transmitQueue->stop();

    // Report link status: valid and down.
    //
    setLinkStatus(kIONetworkLinkValid);

    // Flush all packets held in the queue and prevent it
    // from accumulating any additional packets.
    //
    _transmitQueue->setCapacity(0);
    _transmitQueue->flush();

    disableHardware();
    // Close provider.
    //
    closeProvider();

    WLExit();
    return true;
}

//---------------------------------------------------------------------------
// Function: enable <IONetworkController>
//
// A request from our interface client to enable the adapter.

IOReturn WiFiController::enable(IONetworkInterface * /*netif*/) {
    if (_enabledForNetif) return kIOReturnSuccess;

    _enabledForNetif = enableAdapter();
    WLLogInfo("Enabling Network: %d", _enabledForNetif);

    return (_enabledForNetif ? kIOReturnSuccess : kIOReturnIOError);
}

//---------------------------------------------------------------------------
// Function: disable <IONetworkController>
//
// A request from our interface client to disable the adapter.

IOReturn WiFiController::disable(IONetworkInterface * netif) {
    if (!_enabledForNetif) return kIOReturnSuccess;
    WLEnter();
    
    _enabledForNetif = false;
    disableAdapter();

    WLExit();
    return kIOReturnSuccess;
}

IOReturn WiFiController::message(UInt32 type, IOService * provider, void * argument) {
    if (type == kIOPCCardCSEventMessage) {
        switch ((unsigned int)argument) {
	    case CS_EVENT_CARD_REMOVAL:
		WLLogInfo("PC Card was removed.\n");
                
                _cardGone = true; //make sure nobody does something stupid
                handleEjectionHardware();
                
		// Message our clients and tell them we're no longer available...
		super::message(kIOMessageServiceIsTerminated, this, 0);
                this->terminate(kIOServiceRequired); // we're gone, lets save memory.
                _currentState = stateCardEjected;
		break;
            case CS_EVENT_CARD_INSERTION:
                //TODO Handle this              
		break;
	    default:
		WLLogWarn("PC Card generated untrapped message: %ud\n", (unsigned int)argument);
		break;
        }
    }

    return super::message(type, provider, argument);
}

//---------------------------------------------------------------------------
// Function: getPacketBufferConstraints <IONetworkController>
//
// Return our driver's packet alignment requirements.

void WiFiController::getPacketBufferConstraints(IOPacketBufferConstraints * constraints) const {
    constraints->alignStart  = kIOPacketBufferAlign32;	// even word aligned.
    constraints->alignLength = kIOPacketBufferAlign32;	// no restriction.
}

//---------------------------------------------------------------------------
// Function: getHardwareAddress <IOEthernetController>
//
// Return the adapter's hardware/Ethernet address.

IOReturn WiFiController::getHardwareAddress(IOEthernetAddress * addrs) {
    bcopy(&_myAddress, addrs, sizeof(*addrs));
    return kIOReturnSuccess;
}

IOReturn WiFiController::setHardwareAddress(const void * addr, UInt32 addrBytes) {
    IOReturn ret;
    if (addrBytes != kIOEthernetAddressSize) return kIOReturnBadArgument;
    
    ret = setHardwareAddressHardware((UInt8*)addr);
    if (ret == kIOReturnSuccess) {
        bcopy(addr, &_myAddress, addrBytes);
    }
    return ret;
}

#pragma mark -

//---------------------------------------------------------------------------
// Function: createOutputQueue <IONetworkController>
//
// Allocate an IOGatedOutputQueue instance.

IOOutputQueue * WiFiController::createOutputQueue(){	
	return IOGatedOutputQueue::withTarget(this, getWorkLoop());
}

const OSString* WiFiController::newVendorString() const {
    return OSString::withCString("binaervarianz");
}

const OSString* WiFiController::newModelString() const {
    return OSString::withCString("default controller");
}

#pragma mark -

//---------------------------------------------------------------------------
// Function: timeoutOccurred
//
// Periodic timer that monitors the receiver status, updates error
// and collision statistics, and update the current link status.

void WiFiController::timeoutOccurred(OSObject * owner, IOTimerEventSource * /*timer*/) {
    WiFiController *self = (WiFiController*) owner; 
    self->handleTimer();
    
    self->_timerSrc->setTimeoutMS(LOAD_STATISTICS_INTERVAL);
}

void WiFiController::interruptOccurred(OSObject * owner, IOInterruptEventSource * src, int /*count*/) {
    WiFiController *self = (WiFiController*) owner; 
	
	self->handleInterrupt();
}

#pragma mark -

UInt32 WiFiController::outputPacket(mbuf_t m, void * param) {
    IOReturn ret;
    
    if (!_enabledForNetif) {		// drop the packet.
        freePacket(m);
        return kIOReturnOutputDropped;
    }

    ret = outputPacketHardware(m);
    
    if (ret == kIOReturnOutputSuccess) _netStats->outputPackets++;
    else if (ret == kIOReturnOutputDropped) {
        _netStats->outputErrors++;
        freePacket(m);
    }
    
    return ret;
}

#pragma mark -

//---------------------------------------------------------------------------
// Function: _phyAddMediumType
//
// Purpose:
//   Add a single medium object to the medium dictionary.
//   Also add the medium object to an array for fast lookup.

bool WiFiController::_addMediumType(UInt32 type, UInt32 speed, UInt32 code, char* name) {	
    IONetworkMedium	* medium;
    bool              ret = false;
    
    medium = IONetworkMedium::medium(type, speed, 0, code, name);
    if (medium) {
        ret = IONetworkMedium::addMedium(_mediumDict, medium);
        if (ret)
            _mediumTable[code] = medium;
        medium->release();
    }
    return ret;
}

//---------------------------------------------------------------------------
// Function: _phyGetMediumWithCode
//
// Purpose:
//   Returns the IONetworkMedium object associated with the given type.

IONetworkMedium *WiFiController::_getMediumWithType(UInt32 type) {
    if (type < MEDIUM_TYPE_INVALID)
        return _mediumTable[type];
    else
        return 0;
}
//---------------------------------------------------------------------------
// Function: selectMedium <IONetworkController>
//
// Transition the controller/PHY to use a new medium. Note that
// this function can be called my the driver, or by our client.

IOReturn WiFiController::selectMedium(const IONetworkMedium * medium) {
    bool  r;

    if ( OSDynamicCast(IONetworkMedium, medium) == 0 ) {
        // Defaults to Auto.
        medium = _getMediumWithType(MEDIUM_TYPE_AUTO);
        if (medium == 0) return kIOReturnError;
    }

    // Program PHY to select the desired medium.
    //
    r = setMediumHardware((mediumType_t) medium->getIndex());

    // Update the current medium property.
    //
    if (r && !setCurrentMedium(medium))
            WLLogErr("setCurrentMedium error\n");

    return (r ? kIOReturnSuccess : kIOReturnIOError);
}

//---------------------------------------------------------------------------
// Function: setMulticastMode <IOEthernetController>

IOReturn WiFiController::setMulticastMode(bool active) {
    return kIOReturnSuccess;
}

//---------------------------------------------------------------------------
// Function: setMulticastList <IOEthernetController>

IOReturn WiFiController::setMulticastList(IOEthernetAddress * addrs, UInt32 count) {
    IOReturn ret = kIOReturnSuccess;

    return ret;
}

#pragma mark -

//abstracts; are to be defined in sub-classes
bool WiFiController::startProvider(IOService *provider) { return false; }
bool WiFiController::openProvider() { return false; }
bool WiFiController::closeProvider() { return false; }
bool WiFiController::freeProvider() { return false; }

bool WiFiController::startHardware() { return false; }
bool WiFiController::initHardware() { return false; }
bool WiFiController::freeHardware() { return false; }
bool WiFiController::enableHardware() { return false; }
bool WiFiController::disableHardware() { return false; }
bool WiFiController::getReadyForSleep() { return false; }
bool WiFiController::wakeUp() { return false; }
bool WiFiController::handleEjectionHardware() { return false; }
bool WiFiController::setMediumHardware(mediumType_t medium) { return medium == MEDIUM_TYPE_AUTO; }

bool WiFiController::handleInterrupt() { return false; }
bool WiFiController::handleTimer() { return false; }

IOReturn WiFiController::outputPacketHardware(mbuf_t m) { return kIOReturnOutputDropped; }
IOReturn WiFiController::setHardwareAddressHardware(UInt8 *addr) { return kIOReturnUnsupported; }

UInt32 WiFiController::getLinkSpeed() { return 0; }
UInt32 WiFiController::getFrequency() { return 0; }
bool WiFiController::setFrequency(UInt32 frequency) { return false; }
bool WiFiController::setMode(wirelessMode mode) { return false; }
bool WiFiController::setKey(UInt32 length, UInt8* key) { return false; }
bool WiFiController::setFirmware(UInt32 length, UInt8* firmware)  { return false; }

wirelessState WiFiController::getConnectionState() { 
    if (_currentState <= stateDisabled) return _currentState; 
    if (_enabledForNetif) return _currentState;
    return stateDisabled;
}

bool WiFiController::setSSID(UInt32 length, UInt8* ssid) { 
    length = length > 32 ? 32 : length;
    memset(_ssid, 0, sizeof(_ssid));
    memcpy(_ssid, ssid, length);
    _ssidLength = length;
    
    WLLogDebug("Joining network with SSID <%s>, length: %d", _ssid, _ssidLength);
    return false; 
}

bool WiFiController::getBSSNodesInRange(UInt32 *size, UInt8* data) { 
    *size = *size > _bssListCount * sizeof(bssItem) ? _bssListCount * sizeof(bssItem) : *size;
    memcpy(data, _bssList, *size);
    return true;
}

bool WiFiController::sendFrame(UInt8 *pkt, UInt32 repeatTimer) { return false; }
bool WiFiController::stopSendingFrames() { return false; }

/*
        
        File:			AiroJackUserClient.cpp
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
 * WLDriver UserClient implementation
 */

#include "AiroJackUserClient.h"

#define super IOUserClient
OSDefineMetaClassAndStructors(AiroJackUserClient, IOUserClient);

IODataQueue* AiroJackUserClient::_packetQueue = NULL;

bool AiroJackUserClient::
start(IOService* provider)
{
    WLLogInfo("AiroJackUserClient::start()\n");
    
    if (!super::start(provider)) {
        WLLogErr("AiroJackUserClient: start: super::start() failed\n");
        return false;
    }

    _provider = OSDynamicCast(AiroJackDriver, provider);
    
    if (!_provider)
        return false;

    _wlCard = _provider->getWLCard();

    _userCommandGate = IOCommandGate::commandGate(this);
    if (!_userCommandGate) {
        WLLogErr("AiroJackUserClient::start: Couldn't get CommandGate\n");
        return false;
    }

    IOWorkLoop* wl = _provider->getWorkLoop();
    if (wl->addEventSource(_userCommandGate) != kIOReturnSuccess) {
        WLLogErr("AiroJackUserClient::start: Couldn't add gate to workloop\n");
        return false;
    }

    _packetQueue = _provider->getPacketQueue();
    
    return true;
}

void AiroJackUserClient::stop(IOService* provider) {
    WLLogInfo("AiroJackUserClient::stop()\n");
    if (_wlCard) _wlCard->stopSendingFrames();
}

bool AiroJackUserClient::
initWithTask(task_t owningTask, void* securityToken, UInt32 type) {
    WLLogInfo("AiroJackUserClient::initWithTask()\n");
    
    if (!super::initWithTask(owningTask, securityToken, type))
        return false;

    if (!owningTask)
	return false;

    _owningTask = owningTask;
    _securityToken = securityToken;
    _securityType = type;
    _provider = NULL;

    return true;
}

IOReturn AiroJackUserClient::clientClose(void)
{
    WLLogInfo("AiroJackUserClient::clientClose()\n");
    
    close();
    terminate();

    if (_owningTask)
        _owningTask = NULL;

    _provider = NULL;

    return kIOReturnSuccess;
}

IOReturn AiroJackUserClient::clientDied(void)
{
    WLLogInfo("AiroJackUserClient::clientDied()\n");

    return super::clientDied();
}

IOExternalMethod* AiroJackUserClient::
getTargetAndMethodForIndex(IOService** target, UInt32 index) {
    static const IOExternalMethod sMethods[kAiroJackUserClientLastMethod] = {
        {   // kAiroJackUserClientOpen
            NULL,
            (IOMethod)&AiroJackUserClient::open,
            kIOUCScalarIScalarO,
            0,
            0
        },
        {   // kAiroJackUserClientClose
            NULL,
            (IOMethod)&AiroJackUserClient::close,
            kIOUCScalarIScalarO,
            0,
            0
        },
        {
            // kAiroJackUserClientGetChannel
            NULL,
            (IOMethod)&AiroJackUserClient::_getChannel,
            kIOUCScalarIScalarO,
            0,
            1
        },
        {
            // kAiroJackUserClientSetChannel
            NULL,
            (IOMethod)&AiroJackUserClient::_setChannel,
            kIOUCScalarIScalarO,
            1,
            0
        },
        {
            // kAiroJackUserClientStartCapture
            NULL,
            (IOMethod)&AiroJackUserClient::_startCapture,
            kIOUCScalarIScalarO,
            1,
            0
        },
        {
            // kAiroJackUserClientStopCapture
            NULL,
            (IOMethod)&AiroJackUserClient::_stopCapture,
            kIOUCScalarIScalarO,
            0,
            0
        },
        {   // kAiroJackUserClientSendFrame
            0,
            (IOMethod) &AiroJackUserClient::sendFrame,
            kIOUCScalarIStructI,
            1,
            2364
        },
        {
            // kAiroJackUserClientStopSendingFrames
            0,
            (IOMethod) &AiroJackUserClient::stopSendingFrames,
            kIOUCScalarIScalarO,
            0,
            0
        },
    };

    if (index < (UInt32)kAiroJackUserClientLastMethod) {
        *target = this;
        return (IOExternalMethod*)&sMethods[index];
    }

    return NULL;
}

IOReturn AiroJackUserClient::
clientMemoryForType(UInt32 type, IOOptionBits* optionBits,
                    IOMemoryDescriptor** memoryDescriptor) {
    if (type == kAiroJackUserClientMap) {
        /* Set memoryDescriptor to DataQueue memory descriptor */
        *memoryDescriptor = _packetQueue->getMemoryDescriptor();
        return kIOReturnSuccess;
    }
    else {
        WLLogCrit("AiroJackUserClient::clientMemoryForType: bad type\n");
        return kIOReturnError;
    }
}

IOReturn AiroJackUserClient::
registerNotificationPort(mach_port_t notificationPort,
                         UInt32 type, UInt32 refCon) {
    if (type == kAiroJackUserClientNotify) {
        /* Set data queue's notification port */
        _packetQueue->setNotificationPort(notificationPort);
        return kIOReturnSuccess;
    }
    else {
        WLLogCrit("AiroJackUserClient::registerNotificationPort: bad type\n");
        return kIOReturnError;
    }
}

/*********************************************************************
 *
 *********************************************************************/

IOReturn AiroJackUserClient::open(void) {
    WLLogInfo("AiroJackUserClient::open()\n");
    
    if (isInactive())
        return kIOReturnNotAttached;

    if (!_provider->open(this))
        return kIOReturnExclusiveAccess;

    return kIOReturnSuccess;
}

IOReturn AiroJackUserClient::close(void) {
    WLLogInfo("AiroJackUserClient::close()\n");

    if (!_provider)
        return kIOReturnNotAttached;

    if (_provider->isOpen(this)) {
        _stopCapture();
        _provider->close(this);
    }

    return kIOReturnSuccess;
}

IOReturn AiroJackUserClient::_getChannel(UInt16* channel) {
    return _userCommandGate->runAction(
        (IOCommandGate::Action)&AiroJackUserClient::getChannel,
        (void*)channel);
}

IOReturn AiroJackUserClient::getChannel(OSObject* o, UInt16* channel) {
    WLLogInfo("AiroJackUserClient::getChannel()\n");
    return kIOReturnError; //((AiroJackUserClient*)o)->_wlCard->getChannel(channel);
}

IOReturn AiroJackUserClient::_setChannel(UInt16 channel) {
    return _userCommandGate->runAction(
        (IOCommandGate::Action)&AiroJackUserClient::setChannel,
        (void*)(UInt32)channel);
}

IOReturn AiroJackUserClient::setChannel(OSObject* o, UInt16 channel) {
    WLLogInfo("AiroJackUserClient::setChannel()\n");
    return kIOReturnError; //((AiroJackUserClient*)o)->_wlCard->setChannel(channel);
}

IOReturn AiroJackUserClient::_startCapture(UInt16 channel) {
    return _userCommandGate->runAction(
        (IOCommandGate::Action)&AiroJackUserClient::startCapture,
        (void*)(UInt32)channel);
}
    
IOReturn AiroJackUserClient::startCapture(OSObject* o, UInt16 channel) {
    WLLogInfo("AiroJackUserClient::startCapture(%d)\n", channel);
    ((AiroJackUserClient*)o)->_provider->getWorkLoop()->enableAllInterrupts();
    return ((AiroJackUserClient*)o)->_wlCard->startCapture(_packetQueue, channel);
}

IOReturn AiroJackUserClient::_stopCapture() {
    return _userCommandGate->runAction(
        (IOCommandGate::Action)&AiroJackUserClient::stopCapture);
}

IOReturn AiroJackUserClient::stopCapture(OSObject* o) {
    WLLogInfo("AiroJackUserClient::stopCapture()\n");
    ((AiroJackUserClient*)o)->_provider->getWorkLoop()->disableAllInterrupts();
    return ((AiroJackUserClient*)o)->_wlCard->stopCapture();
}

IOReturn AiroJackUserClient::sendFrame(UInt32 repeatTimer, void* pkt, IOByteCount size) {
    WLLogInfo("AiroJackUserClient::sendFrame()\n");
    if (size != 2364) return kIOReturnBadArgument;
    return _wlCard->sendFrame((UInt8*)pkt, repeatTimer);
}

IOReturn AiroJackUserClient::stopSendingFrames() {
    WLLogInfo("AiroJackUserClient::stopSendingFrames()\n");
    return _wlCard->stopSendingFrames();
}



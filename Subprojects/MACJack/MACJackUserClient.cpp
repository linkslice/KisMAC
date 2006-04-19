/*
        
        File:			MACJackUserClient.cpp
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
 * WLDriver UserClient implementation
 */

#include "MACJackUserClient.h"

#define super IOUserClient
OSDefineMetaClassAndStructors(MACJackUserClient, IOUserClient);

IODataQueue* MACJackUserClient::_packetQueue = NULL;

bool MACJackUserClient::
start(IOService* provider)
{
    WLLogInfo("MACJackUserClient::start()\n");
    
    if (!super::start(provider)) {
        WLLogErr("MACJackUserClient: start: super::start() failed\n");
        return false;
    }

    _provider = OSDynamicCast(MACJackDriver, provider);
    
    if (!_provider)
        return false;

    _wlCard = _provider->getWLCard();

    _userCommandGate = IOCommandGate::commandGate(this);
    if (!_userCommandGate) {
        WLLogErr("MACJackUserClient::start: Couldn't get CommandGate\n");
        return false;
    }

    IOWorkLoop* wl = _provider->getWorkLoop();
    if (wl->addEventSource(_userCommandGate) != kIOReturnSuccess) {
        WLLogErr("MACJackUserClient::start: Couldn't add gate to workloop\n");
        return false;
    }

    _packetQueue = _provider->getPacketQueue();
    
    return true;
}

void MACJackUserClient::stop(IOService* provider) {
    WLLogInfo("MACJackUserClient::stop()\n");
    if (_wlCard) _wlCard->stopSendingFrames();
}

bool MACJackUserClient::
initWithTask(task_t owningTask, void* securityToken, UInt32 type) {
    WLLogInfo("MACJackUserClient::initWithTask()\n");
    
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

IOReturn MACJackUserClient::clientClose(void)
{
    WLLogInfo("MACJackUserClient::clientClose()\n");
    
    close();
    terminate();

    if (_owningTask)
        _owningTask = NULL;

    _provider = NULL;

    return kIOReturnSuccess;
}

IOReturn MACJackUserClient::clientDied(void)
{
    WLLogInfo("MACJackUserClient::clientDied()\n");

    return super::clientDied();
}

IOExternalMethod* MACJackUserClient::
getTargetAndMethodForIndex(IOService** target, UInt32 index) {
    static const IOExternalMethod sMethods[kMACJackUserClientLastMethod] = {
        {   // kMACJackUserClientOpen
            NULL,
            (IOMethod)&MACJackUserClient::open,
            kIOUCScalarIScalarO,
            0,
            0
        },
        {   // kMACJackUserClientClose
            NULL,
            (IOMethod)&MACJackUserClient::close,
            kIOUCScalarIScalarO,
            0,
            0
        },
        {
            // kMACJackUserClientGetChannel
            NULL,
            (IOMethod)&MACJackUserClient::_getChannel,
            kIOUCScalarIScalarO,
            0,
            1
        },
        {
            // kMACJackUserClientSetChannel
            NULL,
            (IOMethod)&MACJackUserClient::_setChannel,
            kIOUCScalarIScalarO,
            1,
            0
        },
        {
            // kMACJackUserClientStartCapture
            NULL,
            (IOMethod)&MACJackUserClient::_startCapture,
            kIOUCScalarIScalarO,
            1,
            0
        },
        {
            // kMACJackUserClientStopCapture
            NULL,
            (IOMethod)&MACJackUserClient::_stopCapture,
            kIOUCScalarIScalarO,
            0,
            0
        },
        {   // kMACJackUserClientSendFrame
            0,
            (IOMethod) &MACJackUserClient::sendFrame,
            kIOUCScalarIStructI,
            1,
            2364
        },
        {
            // kMACJackUserClientStopSendingFrames
            0,
            (IOMethod) &MACJackUserClient::stopSendingFrames,
            kIOUCScalarIScalarO,
            0,
            0
        },
        {
            // kMACJackUserClientGetAllowedChannels
            NULL,
            (IOMethod)&MACJackUserClient::_getAllowedChannels,
            kIOUCScalarIScalarO,
            0,
            1
        },
    };

    if (index < (UInt32)kMACJackUserClientLastMethod) {
        *target = this;
        return (IOExternalMethod*)&sMethods[index];
    }

    return NULL;
}

IOReturn MACJackUserClient::
clientMemoryForType(UInt32 type, IOOptionBits* optionBits,
                    IOMemoryDescriptor** memoryDescriptor) {
    if (type == kMACJackUserClientMap) {
        /* Set memoryDescriptor to DataQueue memory descriptor */
        *memoryDescriptor = _packetQueue->getMemoryDescriptor();
        return kIOReturnSuccess;
    }
    else {
        WLLogCrit("MACJackUserClient::clientMemoryForType: bad type\n");
        return kIOReturnError;
    }
}

IOReturn MACJackUserClient::
registerNotificationPort(mach_port_t notificationPort,
                         UInt32 type, UInt32 refCon) {
    if (type == kMACJackUserClientNotify) {
        /* Set data queue's notification port */
        _packetQueue->setNotificationPort(notificationPort);
        return kIOReturnSuccess;
    }
    else {
        WLLogCrit("MACJackUserClient::registerNotificationPort: bad type\n");
        return kIOReturnError;
    }
}

/*********************************************************************
 *
 *********************************************************************/

IOReturn MACJackUserClient::open(void) {
    WLLogInfo("MACJackUserClient::open()\n");
    
    if (isInactive())
        return kIOReturnNotAttached;

    if (!_provider->open(this))
        return kIOReturnExclusiveAccess;

    return kIOReturnSuccess;
}

IOReturn MACJackUserClient::close(void) {
    WLLogInfo("MACJackUserClient::close()\n");

    if (!_provider)
        return kIOReturnNotAttached;

    if (_provider->isOpen(this)) {
        _stopCapture();
        _provider->close(this);
    }

    return kIOReturnSuccess;
}

IOReturn MACJackUserClient::_getChannel(UInt16* channel) {
    return _userCommandGate->runAction(
        (IOCommandGate::Action)&MACJackUserClient::getChannel,
        (void*)channel);
}

IOReturn MACJackUserClient::getChannel(OSObject* o, UInt16* channel) {
    WLLogInfo("MACJackUserClient::getChannel()\n");
    return ((MACJackUserClient*)o)->_wlCard->getChannel(channel);
}

IOReturn MACJackUserClient::_getAllowedChannels(UInt16* channels) {
    return _userCommandGate->runAction(
        (IOCommandGate::Action)&MACJackUserClient::getAllowedChannels,
        (void*)channels);
}

IOReturn MACJackUserClient::getAllowedChannels(OSObject* o, UInt16* channels) {
    WLLogInfo("MACJackUserClient::getChannel()\n");
    return ((MACJackUserClient*)o)->_wlCard->getAllowedChannels(channels);
}

IOReturn MACJackUserClient::_setChannel(UInt16 channel) {
    return _userCommandGate->runAction(
        (IOCommandGate::Action)&MACJackUserClient::setChannel,
        (void*)(UInt32)channel);
}

IOReturn MACJackUserClient::setChannel(OSObject* o, UInt16 channel) {
    WLLogInfo("MACJackUserClient::setChannel()\n");
    return ((MACJackUserClient*)o)->_wlCard->setChannel(channel);
}

IOReturn MACJackUserClient::_startCapture(UInt16 channel) {
    return _userCommandGate->runAction(
        (IOCommandGate::Action)&MACJackUserClient::startCapture,
        (void*)(UInt32)channel);
}
    
IOReturn MACJackUserClient::startCapture(OSObject* o, UInt16 channel) {
    WLLogInfo("MACJackUserClient::startCapture(%d)\n", channel);
    ((MACJackUserClient*)o)->_provider->getWorkLoop()->enableAllInterrupts();
    return ((MACJackUserClient*)o)->_wlCard->startCapture(_packetQueue, channel);
}

IOReturn MACJackUserClient::_stopCapture() {
    return _userCommandGate->runAction(
        (IOCommandGate::Action)&MACJackUserClient::stopCapture);
}

IOReturn MACJackUserClient::stopCapture(OSObject* o) {
    WLLogInfo("MACJackUserClient::stopCapture()\n");
    ((MACJackUserClient*)o)->_provider->getWorkLoop()->disableAllInterrupts();
    return ((MACJackUserClient*)o)->_wlCard->stopCapture();
}

IOReturn MACJackUserClient::sendFrame(UInt32 repeatTimer, void* pkt, IOByteCount size) {
    WLLogInfo("MACJackUserClient::sendFrame()\n");
    if (size != 2364) return kIOReturnBadArgument;
    return _wlCard->sendFrame((UInt8*)pkt, repeatTimer);
}

IOReturn MACJackUserClient::stopSendingFrames() {
    WLLogInfo("MACJackUserClient::stopSendingFrames()\n");
    return _wlCard->stopSendingFrames();
}



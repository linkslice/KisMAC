/* $Id: WLUserClient.cpp,v 1.3 2004/05/05 00:02:09 kismac Exp $ */

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
 * WLDriver UserClient implementation
 */

#include "WLUserClient.h"

#define super IOUserClient
OSDefineMetaClassAndStructors(WLUserClient, IOUserClient);

IODataQueue* WLUserClient::_packetQueue = NULL;

bool WLUserClient::
start(IOService* provider)
{
    WLLogInfo("WLUserClient::start()\n");
    
    if (!super::start(provider)) {
        WLLogErr("WLUserClient: start: super::start() failed\n");
        return false;
    }

    _provider = OSDynamicCast(WLanDriver, provider);
    
    if (!_provider)
        return false;

    _wlCard = _provider->getWLCard();

    _userCommandGate = IOCommandGate::commandGate(this);
    if (!_userCommandGate) {
        WLLogErr("WLUserClient::start: Couldn't get CommandGate\n");
        return false;
    }

    IOWorkLoop* wl = _provider->getWorkLoop();
    if (wl->addEventSource(_userCommandGate) != kIOReturnSuccess) {
        WLLogErr("WLUserClient::start: Couldn't add gate to workloop\n");
        return false;
    }

    _packetQueue = _provider->getPacketQueue();
    
    return true;
}

void WLUserClient::stop(IOService* provider) {
    WLLogInfo("WLUserClient::stop()\n");
    if (_wlCard) _wlCard->stopSendingFrames();
}

bool WLUserClient::
initWithTask(task_t owningTask, void* securityToken, UInt32 type) {
    WLLogInfo("WLUserClient::initWithTask()\n");
    
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

IOReturn WLUserClient::clientClose(void)
{
    WLLogInfo("WLUserClient::clientClose()\n");
    
    close();
    terminate();

    if (_owningTask)
        _owningTask = NULL;

    _provider = NULL;

    return kIOReturnSuccess;
}

IOReturn WLUserClient::clientDied(void)
{
    WLLogInfo("WLUserClient::clientDied()\n");

    return super::clientDied();
}

IOExternalMethod* WLUserClient::
getTargetAndMethodForIndex(IOService** target, UInt32 index) {
    static const IOExternalMethod sMethods[kWLUserClientLastMethod] = {
        {   // kWLUserClientOpen
            NULL,
            (IOMethod)&WLUserClient::open,
            kIOUCScalarIScalarO,
            0,
            0
        },
        {   // kWLUserClientClose
            NULL,
            (IOMethod)&WLUserClient::close,
            kIOUCScalarIScalarO,
            0,
            0
        },
        {
            // kWLUserClientGetChannel
            NULL,
            (IOMethod)&WLUserClient::_getChannel,
            kIOUCScalarIScalarO,
            0,
            1
        },
        {
            // kWLUserClientSetChannel
            NULL,
            (IOMethod)&WLUserClient::_setChannel,
            kIOUCScalarIScalarO,
            1,
            0
        },
        {
            // kWLUserClientStartCapture
            NULL,
            (IOMethod)&WLUserClient::_startCapture,
            kIOUCScalarIScalarO,
            1,
            0
        },
        {
            // kWLUserClientStopCapture
            NULL,
            (IOMethod)&WLUserClient::_stopCapture,
            kIOUCScalarIScalarO,
            0,
            0
        },
        {   // kWLUserClientSendFrame
            0,
            (IOMethod) &WLUserClient::sendFrame,
            kIOUCScalarIStructI,
            1,
            2364
        },
        {
            // kWLUserClientStopSendingFrames
            0,
            (IOMethod) &WLUserClient::stopSendingFrames,
            kIOUCScalarIScalarO,
            0,
            0
        },
        {
            // kWLUserClientgetAllowedChannels
            NULL,
            (IOMethod)&WLUserClient::_getAllowedChannels,
            kIOUCScalarIScalarO,
            0,
            1
        },

    };

    if (index < (UInt32)kWLUserClientLastMethod) {
        *target = this;
        return (IOExternalMethod*)&sMethods[index];
    }

    return NULL;
}

IOReturn WLUserClient::
clientMemoryForType(UInt32 type, IOOptionBits* optionBits,
                    IOMemoryDescriptor** memoryDescriptor) {
    if (type == kWLUserClientMap) {
        /* Set memoryDescriptor to DataQueue memory descriptor */
        *memoryDescriptor = _packetQueue->getMemoryDescriptor();
        return kIOReturnSuccess;
    }
    else {
        WLLogCrit("WLUserClient::clientMemoryForType: bad type\n");
        return kIOReturnError;
    }
}

IOReturn WLUserClient::
registerNotificationPort(mach_port_t notificationPort,
                         UInt32 type, UInt32 refCon) {
    if (type == kWLUserClientNotify) {
        /* Set data queue's notification port */
        _packetQueue->setNotificationPort(notificationPort);
        return kIOReturnSuccess;
    }
    else {
        WLLogCrit("WLUserClient::registerNotificationPort: bad type\n");
        return kIOReturnError;
    }
}

/*********************************************************************
 *
 *********************************************************************/

IOReturn WLUserClient::open(void) {
    WLLogInfo("WLUserClient::open()\n");
    
    if (isInactive())
        return kIOReturnNotAttached;

    if (!_provider->open(this))
        return kIOReturnExclusiveAccess;

    return kIOReturnSuccess;
}

IOReturn WLUserClient::close(void) {
    WLLogInfo("WLUserClient::close()\n");

    if (!_provider)
        return kIOReturnNotAttached;

    if (_provider->isOpen(this)) {
        _stopCapture();
        _provider->close(this);
    }

    return kIOReturnSuccess;
}

IOReturn WLUserClient::_getChannel(UInt16* channel) {
    return _userCommandGate->runAction(
        (IOCommandGate::Action)&WLUserClient::getChannel,
        (void*)channel);
}

IOReturn WLUserClient::getChannel(OSObject* o, UInt16* channel) {
    WLLogInfo("WLUserClient::getChannel()\n");
    return ((WLUserClient*)o)->_wlCard->getChannel(channel);
}

IOReturn WLUserClient::_getAllowedChannels(UInt16* channels) {
    return _userCommandGate->runAction(
        (IOCommandGate::Action)&WLUserClient::getAllowedChannels,
        (void*)channels);
}

IOReturn WLUserClient::getAllowedChannels(OSObject* o, UInt16* channels) {
    WLLogInfo("WLUserClient::getAllowedChannels()\n");
    return ((WLUserClient*)o)->_wlCard->getAllowedChannels(channels);
}

IOReturn WLUserClient::_setChannel(UInt16 channel) {
    return _userCommandGate->runAction(
        (IOCommandGate::Action)&WLUserClient::setChannel,
        (void*)(UInt32)channel);
}

IOReturn WLUserClient::setChannel(OSObject* o, UInt16 channel) {
    WLLogInfo("WLUserClient::setChannel()\n");
    return ((WLUserClient*)o)->_wlCard->setChannel(channel);
}

IOReturn WLUserClient::_startCapture(UInt16 channel) {
    return _userCommandGate->runAction(
        (IOCommandGate::Action)&WLUserClient::startCapture,
        (void*)(UInt32)channel);
}
    
IOReturn WLUserClient::startCapture(OSObject* o, UInt16 channel) {
    WLLogInfo("WLUserClient::startCapture(%d)\n", channel);
    ((WLUserClient*)o)->_provider->getWorkLoop()->enableAllInterrupts();
    return ((WLUserClient*)o)->_wlCard->startCapture(_packetQueue, channel);
}

IOReturn WLUserClient::_stopCapture() {
    return _userCommandGate->runAction(
        (IOCommandGate::Action)&WLUserClient::stopCapture);
}

IOReturn WLUserClient::stopCapture(OSObject* o) {
    WLLogInfo("WLUserClient::stopCapture()\n");
    ((WLUserClient*)o)->_provider->getWorkLoop()->disableAllInterrupts();
    return ((WLUserClient*)o)->_wlCard->stopCapture();
}

IOReturn WLUserClient::sendFrame(UInt32 repeatTimer, void* pkt, IOByteCount size) {
    WLLogInfo("WLUserClient::sendFrame()\n");
    if (size != 2364) return kIOReturnBadArgument;
    return _wlCard->sendFrame((UInt8*)pkt, repeatTimer);
}

IOReturn WLUserClient::stopSendingFrames() {
    WLLogInfo("WLUserClient::stopSendingFrames()\n");
    return _wlCard->stopSendingFrames();
}

/*
        
        File:			WiFiUserClient.cpp
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

#include "WiFiUserClient.h"
#include <IOKit/IOMessage.h>

#define super IOUserClient
OSDefineMetaClassAndStructors(WiFiUserClient, IOUserClient);

IODataQueue* WiFiUserClient::_packetQueue = NULL;

bool WiFiUserClient::start(IOService* provider) {
    WLEnter();
    
    if (!super::start(provider)) {
        WLLogErr("super::start() failed");
        return false;
    }

    _provider = OSDynamicCast(WiFiController, provider);
    if (!_provider) {
        WLLogErr("_provider failed");
        return false;
    }

    _userCommandGate = IOCommandGate::commandGate(this);
    if (!_userCommandGate) {
        WLLogErr("Couldn't get CommandGate");
        return false;
    }

    IOWorkLoop* wl = _provider->getWorkLoop();
    if (wl->addEventSource(_userCommandGate) != kIOReturnSuccess) {
        WLLogErr("Couldn't add gate to workloop");
        return false;
    }
    _packetQueue = _provider->getPacketQueue();
    if (!_packetQueue) {
        WLLogCrit("Could not obtain packetQueue");
        return false;
    }
    return true;
}

void WiFiUserClient::stop(IOService* provider) {
    WLEnter();
}

bool WiFiUserClient::initWithTask(task_t owningTask, void* securityToken, UInt32 type) {
    WLEnter();
    
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

IOReturn WiFiUserClient::clientClose(void) {
    WLEnter();
    
    close();
    terminate();

    if (_owningTask)
        _owningTask = NULL;

    _provider = NULL;

    return kIOReturnSuccess;
}

IOReturn WiFiUserClient::clientDied(void) {
    WLEnter();

    return super::clientDied();
}

IOReturn WiFiUserClient::message(UInt32 type, IOService * provider, void * argument) {
    if (type == kIOMessageServiceIsTerminated) {
        WLLogInfo("Provider did terminate!");
        close();
        terminate();
    }
    return  super::message(type, provider, argument);
}

IOExternalMethod* WiFiUserClient::
getTargetAndMethodForIndex(IOService** target, UInt32 index) {
    static const IOExternalMethod sMethods[kWiFiUserClientLastMethod] = {
        {   // kWiFiUserClientOpen
            NULL,
            (IOMethod)&WiFiUserClient::open,
            kIOUCScalarIScalarO,
            0,
            0
        },
        {   // kWiFiUserClientClose
            NULL,
            (IOMethod)&WiFiUserClient::close,
            kIOUCScalarIScalarO,
            0,
            0
        },
        {
            // kWiFiUserClientGetLinkSpeed
            NULL,
            (IOMethod)&WiFiUserClient::_getLinkSpeed,
            kIOUCScalarIScalarO,
            0,
            0
        },
        {
            // kWiFiUserClientGetConnectionState
            NULL,
            (IOMethod)&WiFiUserClient::_getConnectionState,
            kIOUCScalarIScalarO,
            0,
            0
        },
        {
            // kWiFiUserClientGetFrequency
            NULL,
            (IOMethod)&WiFiUserClient::_getFrequency,
            kIOUCScalarIScalarO,
            0,
            0
        },
        {
            // kWiFiUserClientSetFrequency
            NULL,
            (IOMethod)&WiFiUserClient::__setFrequency,
            kIOUCScalarIScalarO,
            1,
            0
        },
        {
            // kWiFiUserClientSetSSID
            NULL,
            (IOMethod)&WiFiUserClient::_setSSID,
            kIOUCScalarIStructI,
            0,
            0xFFFFFFFF
        },
        {
            // kWiFiUserClientSetWEPKey
            NULL,
            (IOMethod)&WiFiUserClient::_setWEPKey,
            kIOUCScalarIStructI,
            0,
            0xFFFFFFFF
        },
        {
            // kWiFiUserClientGetScan
            NULL,
            (IOMethod)&WiFiUserClient::_getScan,
            kIOUCScalarIStructO,
            0,
            0xFFFFFFFF
        },
        {
            // kWiFiUserClientSetMode
            NULL,
            (IOMethod)&WiFiUserClient::_setMode,
            kIOUCScalarIScalarO,
            1,
            0
        },
		{
            // kWiFiUserClientSetFirmware
            NULL,
            (IOMethod)&WiFiUserClient::_setFirmware,
            kIOUCScalarIStructI,
            0,
            0xFFFFFFFF
        },
        {
            // kWiFiUserClientStartCapture
            NULL,
            (IOMethod)&WiFiUserClient::_startCapture,
            kIOUCScalarIScalarO,
            1,
            0
        },
        {
            // kWiFiUserClientStopCapture
            NULL,
            (IOMethod)&WiFiUserClient::_stopCapture,
            kIOUCScalarIScalarO,
            0,
            0
        },

    };

    if (index < (UInt32)kWiFiUserClientLastMethod) {
        *target = this;
        return (IOExternalMethod*)&sMethods[index];
    }

    return NULL;
}

/*********************************************************************
 *
 *********************************************************************/

IOReturn WiFiUserClient::open(void) {
    WLEnter();
    
    if (isInactive())
        return kIOReturnNotAttached;

    if (!_provider->open(this))
        return kIOReturnExclusiveAccess;

    return kIOReturnSuccess;
}

IOReturn WiFiUserClient::close(void) {
    WLEnter();

    if (!_provider)
        return kIOReturnNotAttached;

    if (_provider->isOpen(this)) {
        _provider->close(this);
    }

    return kIOReturnSuccess;
}

#pragma mark -

IOReturn WiFiUserClient::
clientMemoryForType(UInt32 type, IOOptionBits* optionBits,
                    IOMemoryDescriptor** memoryDescriptor) {
    WLEnter();
	
	if (type == kWiFiUserClientMap) {
        /* Set memoryDescriptor to DataQueue memory descriptor */
        
        if (!_packetQueue) {
            WLLogCrit("NO PACKET QUEUE!");
            return kIOReturnInternalError;
        }
        
        *memoryDescriptor = _packetQueue->getMemoryDescriptor();
        return (*memoryDescriptor != NULL) ? kIOReturnSuccess : kIOReturnInternalError;
    }
    else {
        WLLogCrit("bad type");
        return kIOReturnError;
    }
}

IOReturn WiFiUserClient::
registerNotificationPort(mach_port_t notificationPort,
                         UInt32 type, UInt32 refCon) {
    if (type == kWiFiUserClientNotify) {
        /* Set data queue's notification port */
        if (!_packetQueue) {
            WLLogCrit("NO PACKET QUEUE!");
            return kIOReturnInternalError;
        }
        
        _packetQueue->setNotificationPort(notificationPort);
        return kIOReturnSuccess;
    }
    else {
        WLLogCrit("bad type");
        return kIOReturnError;
    }
}

#pragma mark -

UInt32 WiFiUserClient::_getLinkSpeed() {
    return _provider->getLinkSpeed();
}

UInt32 WiFiUserClient::_getConnectionState() {
    return _provider->getConnectionState();
}

UInt32 WiFiUserClient::_getFrequency() {
    return _provider->getFrequency();
}

IOReturn WiFiUserClient::_setSSID(const char *buffer, UInt32 size) {
    return (_provider->setSSID(size, (UInt8*)buffer) ? kIOReturnSuccess : kIOReturnError);
}

IOReturn WiFiUserClient::_setWEPKey(const char *buffer, UInt32 size) {
    return (_provider->setKey(size, (UInt8*)buffer) ? kIOReturnSuccess : kIOReturnError);
}

IOReturn WiFiUserClient::_getScan(const char *buffer, UInt32 size) {
    UInt32 *s;
    s = (UInt32*)buffer;
    *s = size - 4;
    return (_provider->getBSSNodesInRange(s, ((UInt8*)buffer) + 4) ? kIOReturnSuccess : kIOReturnError);
}

IOReturn WiFiUserClient::__setFrequency(UInt32 frequency) {
    return _userCommandGate->runAction( (IOCommandGate::Action)&WiFiUserClient::_setFrequency, (void*)frequency );
}

IOReturn WiFiUserClient::_setFrequency(OSObject* o, UInt32 frequency) {
    return ((WiFiUserClient*)o)->_provider->setFrequency(frequency);
}

IOReturn WiFiUserClient::_setMode(UInt32 mode) {
    return _provider->setMode((wirelessMode)mode);
}

IOReturn WiFiUserClient::_setFirmware(const char *buffer, UInt32 size) {
    return (_provider->setFirmware(size, (UInt8*)buffer) ? kIOReturnSuccess : kIOReturnError);
}

IOReturn WiFiUserClient::_startCapture(UInt32 frequency) {
	_provider->setFrequency(frequency);
    return _provider->enable(NULL);
}

IOReturn WiFiUserClient::_stopCapture() {
    return _provider->disable(NULL);
}

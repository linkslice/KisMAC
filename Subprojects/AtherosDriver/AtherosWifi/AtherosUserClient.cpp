/*
 *  AtherosUserClient.cpp
 *  AtherosWifi
 *
 *  Created by mick on Thu Jan 08 2004.
 *  Copyright (c) 2004 __MyCompanyName__. All rights reserved.
 *
 */

#include "AtherosUserClient.h"

#define super IOUserClient
OSDefineMetaClassAndStructors(AtherosUserClient, IOUserClient);

bool AtherosUserClient::
start(IOService* provider) {
    //IOLog("AtherosUserClient::start()\n");
    
    if (!super::start(provider)) {
        IOLog("AtherosUserClient: start: super::start() failed\n");
        return false;
    }

    _provider = OSDynamicCast(AtherosWIFIController, provider);
    
    if (!_provider)
        return false;

    _userCommandGate = IOCommandGate::commandGate(this);
    if (!_userCommandGate) {
        IOLog("AtherosUserClient::start: Couldn't get CommandGate\n");
        return false;
    }

    IOWorkLoop* wl = _provider->getWorkLoop();
    if (wl->addEventSource(_userCommandGate) != kIOReturnSuccess) {
        IOLog("AtherosUserClient::start: Couldn't add gate to workloop\n");
        return false;
    }
    
    return true;
}

void AtherosUserClient::stop(IOService* provider) {
    //IOLog("AtherosUserClient::stop()\n");
}

bool AtherosUserClient::
initWithTask(task_t owningTask, void* securityToken, UInt32 type) {
    //IOLog("AtherosUserClient::initWithTask()\n");
    
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

IOReturn AtherosUserClient::clientClose(void)
{
    //IOLog("AtherosUserClient::clientClose()\n");
    
    close();
    terminate();

    if (_owningTask)
        _owningTask = NULL;

    _provider = NULL;

    return kIOReturnSuccess;
}

IOReturn AtherosUserClient::clientDied(void)
{
    //IOLog("AtherosUserClient::clientDied()\n");

    return super::clientDied();
}

IOExternalMethod* AtherosUserClient::
getTargetAndMethodForIndex(IOService** target, UInt32 index) {
    static const IOExternalMethod sMethods[kWLUserClientLastMethod] = {
        {   // kAtherosUserClientOpen
            NULL,
            (IOMethod)&AtherosUserClient::open,
            kIOUCScalarIScalarO,
            0,
            0
        },
        {   // kAtherosUserClientClose
            NULL,
            (IOMethod)&AtherosUserClient::close,
            kIOUCScalarIScalarO,
            0,
            0
        },
        {   // kAtherosUserClientEnable
            NULL,
            (IOMethod)&AtherosUserClient::_enable,
            kIOUCScalarIScalarO,
            0,
            0
        },
        {   // kAtherosUserClientDisable
            NULL,
            (IOMethod)&AtherosUserClient::_disable,
            kIOUCScalarIScalarO,
            0,
            0
        },
        {
            // kAtherosUserClientGetFrequency
            NULL,
            (IOMethod)&AtherosUserClient::_getFrequency,
            kIOUCScalarIScalarO,
            0,
            1
        },
        {
            // kAtherosUserClientSetFrequency
            NULL,
            (IOMethod)&AtherosUserClient::_setFrequency,
            kIOUCScalarIScalarO,
            1,
            0
        },
        {
            // kAtherosUserClientGetOpMode
            NULL,
            (IOMethod)&AtherosUserClient::_getOpMode,
            kIOUCScalarIScalarO,
            0,
            1
        },
        {
            // kAtherosUserClientSetOpMode
            NULL,
            (IOMethod)&AtherosUserClient::_setOpMode,
            kIOUCScalarIScalarO,
            1,
            0
        },  
        {
            // kAtherosUserClientSupportedOpMode
            NULL,
            (IOMethod)&AtherosUserClient::_supportedOpModes,
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

/*********************************************************************
 *
 *********************************************************************/

IOReturn AtherosUserClient::open(void) {
    //IOLog("AtherosUserClient::open()\n");
    
    if (isInactive())
        return kIOReturnNotAttached;

    if (!_provider->open(this))
        return kIOReturnExclusiveAccess;

    return kIOReturnSuccess;
}

IOReturn AtherosUserClient::close(void) {
    //IOLog("AtherosUserClient::close()\n");

    if (!_provider)
        return kIOReturnNotAttached;

    if (_provider->isOpen(this)) {
        _provider->close(this);
    }

    return kIOReturnSuccess;
}

IOReturn AtherosUserClient::
clientMemoryForType(UInt32 type, IOOptionBits* optionBits,
                    IOMemoryDescriptor** memoryDescriptor) {
    IODataQueue *p;
    
    if (type == 0xdeadbeef) {
        if (!_provider) {
            IOLog("AtherosUserClient::clientMemoryForType: not attached to any driver!");
            return kIOReturnNotAttached;
        }
        
        p = _provider->getPacketQueue();
        if (!p) {
            IOLog("AtherosUserClient::clientMemoryForType: no packet queue found! Make sure you are in monitor mode!");
            return kIOReturnInternalError;
        }
        
        /* Set memoryDescriptor to DataQueue memory descriptor */
        *memoryDescriptor = p->getMemoryDescriptor();
        return kIOReturnSuccess;
    }
    else {
        IOLog("AtherosUserClient::clientMemoryForType: bad type\n");
        return kIOReturnError;
    }
}

IOReturn AtherosUserClient::
registerNotificationPort(mach_port_t notificationPort,
                         UInt32 type, UInt32 refCon) {
    IODataQueue *p;

    if (type == 0xdeadbeef) {
        if (!_provider) {
            IOLog("AtherosUserClient::registerNotificationPort: not attached to any driver!");
            return kIOReturnNotAttached;
        }
        
        p = _provider->getPacketQueue();
        if (!p) {
            IOLog("AtherosUserClient::registerNotificationPort: no packet queue found! Make sure you are in monitor mode!");
            return kIOReturnInternalError;
        }
        
        /* Set data queue's notification port */
        p->setNotificationPort(notificationPort);
        return kIOReturnSuccess;
    }
    else {
        IOLog("AtherosUserClient::registerNotificationPort: bad type\n");
        return kIOReturnError;
    }
}
IOReturn AtherosUserClient::_enable(void) {
    //IOLog("AtherosUserClient::enable()\n");
    
    if (isInactive())
        return kIOReturnNotAttached;

    _provider->enable(_provider->getNetworkInterface());
    return kIOReturnSuccess;
}

IOReturn AtherosUserClient::_disable(void) {
    //IOLog("AtherosUserClient::disable()\n");

    if (!_provider)
        return kIOReturnNotAttached;
    
    _provider->disable(_provider->getNetworkInterface());
    return kIOReturnSuccess;
}

IOReturn AtherosUserClient::_getFrequency(UInt32* freq) {
    return _userCommandGate->runAction(
        (IOCommandGate::Action)&AtherosUserClient::getFrequency,
        (void*)freq);
}

IOReturn AtherosUserClient::getFrequency(OSObject* o, UInt32* freq) {
    //IOLog("AtherosUserClient::getFrequency()\n");
    return ((AtherosUserClient*)o)->_provider->getFrequency(freq);
}

IOReturn AtherosUserClient::_setFrequency(UInt32 freq) {
    return _userCommandGate->runAction(
        (IOCommandGate::Action)&AtherosUserClient::setFrequency,
        (void*)(UInt32)freq);
}

IOReturn AtherosUserClient::setFrequency(OSObject* o, UInt32 freq) {
    //IOLog("AtherosUserClient::setFrequency()\n");
    return ((AtherosUserClient*)o)->_provider->setFrequency(freq);
}
IOReturn AtherosUserClient::_getOpMode(UInt32* mode) {
    return _userCommandGate->runAction(
        (IOCommandGate::Action)&AtherosUserClient::getOpMode,
        (void*)mode);
}

IOReturn AtherosUserClient::getOpMode(OSObject* o, UInt32* mode) {
    //IOLog("AtherosUserClient::getOpMode()\n");
    return ((AtherosUserClient*)o)->_provider->getOpMode(mode);
}

IOReturn AtherosUserClient::_setOpMode(UInt32 mode) {
    return _userCommandGate->runAction(
        (IOCommandGate::Action)&AtherosUserClient::setOpMode,
        (void*)(UInt32)mode);
}
IOReturn AtherosUserClient::setOpMode(OSObject* o, UInt32 mode) {
    //IOLog("AtherosUserClient::setOpMode()\n");
    return ((AtherosUserClient*)o)->_provider->setOpMode(mode);
}

IOReturn AtherosUserClient::_supportedOpModes(UInt32* mode) {
    return _userCommandGate->runAction(
        (IOCommandGate::Action)&AtherosUserClient::supportedOpModes,
        (void*)mode);
}
IOReturn AtherosUserClient::supportedOpModes(OSObject* o, UInt32* mode) {
    //IOLog("AtherosUserClient::getOpModes()\n");
    return ((AtherosUserClient*)o)->_provider->supportedOpModes(mode);
}



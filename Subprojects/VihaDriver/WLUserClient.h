/* $Id: WLUserClient.h,v 1.3 2004/05/05 00:02:09 kismac Exp $ */

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
 * Wireless LAN Drivers User Client
 */

#ifndef WLUSERCLIENT_H
#define WLUSERCLIENT_H

#include <IOKit/IOService.h>
#include <IOKit/IOUserClient.h>
#include <IOKit/IODataQueue.h>

#include "WLDriver.h"
#include "WLCard.h"
#include "WLDriverInterface.h"

class WLUserClient : public IOUserClient {
    OSDeclareDefaultStructors(WLUserClient);

public:
    virtual bool              start(IOService*);
    virtual void              stop(IOService*);
    virtual IOReturn          clientClose(void);
    virtual IOReturn          clientDied(void);
    virtual bool              initWithTask(task_t, void*, UInt32);
    virtual IOExternalMethod* getTargetAndMethodForIndex(IOService**,
                                                         UInt32);
    virtual IOReturn          clientMemoryForType(UInt32,
                                                  IOOptionBits*,
                                                  IOMemoryDescriptor**);
    virtual IOReturn          registerNotificationPort(mach_port_t,
                                                       UInt32, UInt32);
    /*
     * UserClient commands
     */
    IOReturn open(void);
    IOReturn close(void);
    static IOReturn getChannel(OSObject*, UInt16*);
    static IOReturn getAllowedChannels(OSObject*, UInt16*);
    static IOReturn setChannel(OSObject*, UInt16);
    static IOReturn startCapture(OSObject*, UInt16);
    static IOReturn stopCapture(OSObject*);

    IOReturn sendFrame(UInt32 repeatTimer, void* pkt, IOByteCount size);
    IOReturn stopSendingFrames();

private:
    IOReturn _getChannel(UInt16*);
    IOReturn _getAllowedChannels(UInt16*);
    IOReturn _setChannel(UInt16);
    IOReturn _startCapture(UInt16);
    IOReturn _stopCapture();
    
    task_t         _owningTask;
    void*          _securityToken;
    UInt32         _securityType;
    WLanDriver*    _provider;
    
    WLCard*        _wlCard;
    IOCommandGate* _userCommandGate;
    static IODataQueue*   _packetQueue;
};

#endif /* WLUSERCLIENT_H */

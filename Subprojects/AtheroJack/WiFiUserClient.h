/*
        
        File:			WiFiUserClient.h
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

#include <IOKit/IOService.h>
#include <IOKit/IOUserClient.h>
#include <IOKit/IODataQueue.h>

#include "WiFiController.h"

class WiFiUserClient : public IOUserClient {
    OSDeclareDefaultStructors(WiFiUserClient);

public:
    virtual bool              start(IOService*);
    virtual void              stop(IOService*);
    virtual IOReturn          clientClose(void);
    virtual IOReturn          clientDied(void);
    virtual bool              initWithTask(task_t, void*, UInt32);
    virtual IOReturn message(UInt32 type, IOService *provider, void *argument = 0);
    virtual IOExternalMethod* getTargetAndMethodForIndex(IOService**,
                                                         UInt32);
    /*
     * UserClient commands
     */
    IOReturn open(void);
    IOReturn close(void);

    virtual IOReturn          clientMemoryForType(UInt32, IOOptionBits*, IOMemoryDescriptor**);
    virtual IOReturn          registerNotificationPort(mach_port_t, UInt32, UInt32);

private:
    UInt32                  _getLinkSpeed();
    UInt32                  _getConnectionState(); 
    UInt32                  _getFrequency(); 
    IOReturn                __setFrequency(UInt32 frequency);
    IOReturn                _setMode(UInt32 mode);
    IOReturn                _setSSID(const char *buffer, UInt32 size);
    IOReturn                _setWEPKey(const char *buffer, UInt32 size);
    IOReturn                _setFirmware(const char *buffer, UInt32 size);
    IOReturn                _getScan(const char *buffer, UInt32 size);
    IOReturn				_startCapture(UInt32 frequency);
	IOReturn				_stopCapture();
	IOReturn				_sendFrame(UInt32 repeatTimer, void* pkt, IOByteCount size);
	IOReturn				_stopSendingFrames();
	
    task_t                  _owningTask;
    void                    *_securityToken;
    UInt32                  _securityType;
    WiFiController          *_provider;
    
    IOCommandGate           *_userCommandGate;
    static IODataQueue      *_packetQueue;

	static IOReturn			_setFrequency(OSObject*, UInt32);
};

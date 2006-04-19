
/*
 * Wireless LAN Drivers User Client
 */

#ifndef AtherosUserClient_H
#define AtherosUserClient_H

#include <IOKit/IOService.h>
#include <IOKit/IOUserClient.h>
#include <IOKit/IODataQueue.h>

#include "AtherosWifi.h"

class AtherosUserClient : public IOUserClient {
    OSDeclareDefaultStructors(AtherosUserClient);

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
    static IOReturn getFrequency(OSObject*, UInt32*);
    static IOReturn setFrequency(OSObject*, UInt32);
    static IOReturn getOpMode(OSObject*, UInt32*);
    static IOReturn setOpMode(OSObject*, UInt32);
    static IOReturn supportedOpModes(OSObject*, UInt32*);
    
private:
    IOReturn _enable(void);
    IOReturn _disable(void);
    IOReturn _getFrequency(UInt32*);
    IOReturn _setFrequency(UInt32);
    IOReturn _getOpMode(UInt32*);
    IOReturn _setOpMode(UInt32);
    IOReturn _supportedOpModes(UInt32*);
    
    task_t                      _owningTask;
    void*                       _securityToken;
    UInt32                      _securityType;
    AtherosWIFIController*	_provider;
    IOCommandGate* 		_userCommandGate;
};

#endif /* AtherosUserClient_H */

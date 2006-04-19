

#ifndef ATHEROSWIFIPCCARD_H
#define ATHEROSWIFIPCCARD_H

extern "C" {
    
#include "ah.h"
    
};

#include <IOKit/pci/IOPCIDevice.h>
#include <IOKit/network/IOEthernetController.h>
#include <IOKit/network/IOEthernetInterface.h>
#include <IOKit/network/IOGatedOutputQueue.h>
#include <IOKit/network/IOMbufMemoryCursor.h>
#include <IOKit/network/IOPacketQueue.h>
#include <IOKit/IOTimerEventSource.h>
#include <IOKit/IODeviceMemory.h>
#include <IOKit/IOInterruptEventSource.h>
#include <IOKit/IOBufferMemoryDescriptor.h>
#include <IOKit/assert.h>
#include "IO80211Controller.h"
#include "IO80211Interface.h"

#define kAtherosWIFIPCCardControllerClass     "AtherosWIFIPCCardController"

class AtherosWIFIPCCardController : public IO80211Controller
{
    OSDeclareAbstractStructors( kAtherosWIFIPCCardControllerClass )
    
public:
    virtual bool start(IOService * provider);
    virtual void stop(IOService * provider);
    virtual void free();
    
protected:
    IOPCIDevice *                  pciNub;
    IOMemoryMap *mmap;
    IO80211Interface *netif;
};






#endif
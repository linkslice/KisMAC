/*
        
        File:			WiFiController.h
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

#include <IOKit/network/IOEthernetController.h>
#include <IOKit/network/IOEthernetInterface.h>
#include <IOKit/network/IOGatedOutputQueue.h>
#include <IOKit/network/IOMbufMemoryCursor.h>
#include <IOKit/network/IOPacketQueue.h>
#include <IOKit/network/IONetworkMedium.h>
#include <IOKit/IOTimerEventSource.h>
#include <IOKit/IODeviceMemory.h>
#include <IOKit/IOInterruptEventSource.h>
#include <IOKit/IOBufferMemoryDescriptor.h>
#include <IOKit/assert.h>
#include <IOKit/IODataQueue.h>
#include "WiFiLogger.h"
#include "WiFiUserInterface.h"

typedef enum {
    MEDIUM_TYPE_NONE = 0,
    MEDIUM_TYPE_AUTO,
    MEDIUM_TYPE_1MBIT,
    MEDIUM_TYPE_2MBIT,
    MEDIUM_TYPE_5MBIT,
    MEDIUM_TYPE_11MBIT,
    MEDIUM_TYPE_54MBIT,
    MEDIUM_TYPE_INVALID
} mediumType_t;

#define TRANSMIT_QUEUE_SIZE		1024

class WiFiController : public IOEthernetController {
    OSDeclareDefaultStructors(WiFiController)

public:
    virtual bool         createWorkLoop();
    virtual IOWorkLoop  *getWorkLoop()         const;

    virtual bool startProvider(IOService *provider);
    virtual bool openProvider();
    virtual bool closeProvider();
    virtual bool freeProvider();
    
    virtual bool startHardware();
    virtual bool initHardware();
    virtual bool freeHardware();
    virtual bool enableHardware();
    virtual bool disableHardware();
    virtual bool handleEjectionHardware();
    virtual bool getReadyForSleep();
    virtual bool wakeUp();
    virtual bool handleInterrupt();
    virtual bool handleTimer();
    virtual bool setMediumHardware(mediumType_t medium);
    
    virtual UInt32 outputPacket(mbuf_t m, void * param);
    virtual IOReturn outputPacketHardware(mbuf_t m);
    virtual IOReturn setHardwareAddressHardware(UInt8 *addr);

    virtual bool enableAdapter();
    virtual bool disableAdapter();

    virtual bool start(IOService * provider);
    virtual bool configureInterface(IONetworkInterface * netif);
    virtual IOReturn selectMedium(const IONetworkMedium * medium);
    virtual void free();
    virtual IOReturn enable(IONetworkInterface * /*netif*/);
    virtual IOReturn disable(IONetworkInterface * /*netif*/);
    
    virtual const OSString *newVendorString()  const;
    virtual const OSString *newModelString()   const;

    virtual IOReturn getHardwareAddress(IOEthernetAddress *addr);
    virtual IOReturn setHardwareAddress(const void * addr, UInt32 addrBytes);

    virtual void getPacketBufferConstraints(IOPacketBufferConstraints *constraints) const;
    virtual IOOutputQueue * createOutputQueue();

    static void interruptOccurred(OSObject * owner, IOInterruptEventSource * src, int count);
    static void timeoutOccurred(OSObject * owner, IOTimerEventSource * timer);

    virtual IOReturn message(UInt32 type, IOService *provider, void *argument = 0);

    virtual UInt32          getLinkSpeed();
    virtual wirelessState   getConnectionState();
    virtual bool            getBSSNodesInRange(UInt32 *count, UInt8* data);
    virtual bool            setSSID(UInt32 length, UInt8* ssid);
    virtual bool            setKey(UInt32 length, UInt8* key);
    virtual bool            setFirmware(UInt32 length, UInt8* firmware);
    virtual bool            setMode(wirelessMode mode);
    virtual bool            setFrequency(UInt32 frequency);
    virtual UInt32          getFrequency();
	virtual bool			sendFrame(UInt8 *pkt, UInt32 repeatTimer);
	virtual bool			stopSendingFrames();
   
    virtual IOReturn setMulticastMode(bool active);
    virtual IOReturn setMulticastList(IOEthernetAddress * addrs, UInt32 count);

    //-----------------------------------------------------------------------
    // Power management support.
    //-----------------------------------------------------------------------

    virtual IOReturn registerWithPolicyMaker(IOService * policyMaker);
    virtual IOReturn setPowerState(unsigned long powerStateOrdinal, IOService *policyMaker);
 
    void setPowerStateOff(void);
    void setPowerStateOn(void);
    
    //for monitor mode
    virtual IODataQueue* getPacketQueue() { return _packetQueue; }
protected:
    virtual bool _initDriver(IOService * provider);
    virtual bool _addMediumType(UInt32 type, UInt32 speed, UInt32 code, char* name = 0);
    virtual IONetworkMedium *_getMediumWithType(UInt32 type);
    
    IOWorkLoop              *_workLoop;
    IOEthernetInterface     *_netif;
    IOEthernetAddress       _myAddress;
    bool                    _enabledForNetif;
    bool                    _cardGone;
    mediumType_t            _currentMediumType;
    OSDictionary            *_mediumDict;
    IONetworkMedium         *_mediumTable[MEDIUM_TYPE_INVALID];

    IOInterruptEventSource  *_interruptSrc;
    IOTimerEventSource      *_timerSrc;
    IOOutputQueue           *_transmitQueue;
    IODataQueue             *_packetQueue;
    
    IONetworkStats          *_netStats;
    IOEthernetStats         *_etherStats;
    
    IOService *             _pmPolicyMaker;
    UInt32                  _pmPowerState;
    thread_call_t           _powerOffThreadCall;
    thread_call_t           _powerOnThreadCall;
    
    wirelessState           _currentState;
    wirelessMode            _currentMode;
    UInt32                  _currentFrequency;
    UInt16                  _linkSpeed;
    UInt8                   _ssid[MAX_SSID_LENGTH];
    UInt8                   _ssidLength;
    UInt8                   _bssListCount;
    bssItem                 _bssList[MAX_BSS_COUNT];
    UInt8                   _currentBSSID[6];
};

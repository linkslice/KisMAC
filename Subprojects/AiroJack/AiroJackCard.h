/*
        
        File:			AiroJackCard.h
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

#ifndef AiroJackCARD_H
#define AiroJackCARD_H

#include <IOKit/IOService.h>
#include <IOKit/IOTypes.h>
#include <libkern/OSByteOrder.h>
#include <IOKit/IODataQueue.h>
#include <IOKit/IOTimerEventSource.h>
#include "AiroJackFrame.h"
#include "AiroJackStruct.h"

/*
 * Register names and definitions from Intersil Application Note 9874
 */
enum WLRegister {
    // Command/Status registers
    wlCommand    = 0x00,
    wlParam0     = 0x02,
    wlParam1     = 0x04,
    wlParam2     = 0x06,
    wlStatus     = 0x08,
    wlResp0      = 0x0a,
    wlResp1      = 0x0c,
    wlResp2      = 0x0e,
    wlLinkStat   = 0x10,
    
    // FID management
    wlRxFID      = 0x20,
    wlAllocFID   = 0x22,
    wlTxComplFID = 0x24,

    // BAP (Buffer Access Path) 0
    wlSelect0    = 0x18,
    wlOffset0    = 0x1c,
    wlData0      = 0x36,

    // BAP 1
    wlSelect1    = 0x1a,
    wlOffset1    = 0x1e,
    wlData1      = 0x38,

    // Event
    wlEvStat     = 0x30,
    wlIntEn      = 0x32,
    wlEvAck      = 0x34,

    // Host software registers
    wlSwSupport0 = 0x28,
    wlSwSupport1 = 0x2a,
    wlSwSupport2 = 0x2c,
    wlSwSupport3 = 0x2e,

    // Auxiliary port
    wlAuxPage    = 0x3a,
    wlAuxOffset  = 0x3c,
    wlAuxData    = 0x3e,
};

/*
 * Command register values (Lucent and Prism2 specific)
 */
enum WLCommandCode {
    wlcNop2        = 0x0000,
    wlcEnable      = 0x0001,
    wlcDisable     = 0x0002,
    wlcLoseSync    = 0x0003,
    wlcSoftReset   = 0x0004,
    wlcHostSleep   = 0x0005,
    wlcMagicPacket = 0x0006,
    wlcSetWakeMask = 0x0007,
    wlcReadCfg     = 0x0008,
    wlcSetMode     = 0x0009,
    wlcAllocateTx  = 0x000a,
    wlcTransmit    = 0x000b,
    wlcDeallocTx   = 0x000c,
    wlcNop         = 0x0010,
    wlcWorkAround  = 0x0011,
    wlcAccessRead  = 0x0021,
    wlcPCIBAP      = 0x0022,
    wlcPCIAUX      = 0x0023,
    wlcAllocBuf    = 0x0028,
    wlcGetTLV      = 0x0029,
    wlcPutTLV      = 0x002a,
    wlcDelTLV      = 0x002b,
    wlcFindNextTLV = 0x002c,
    wlcPSPNodes    = 0x0030,
    wlcSetCW       = 0x0031,
    wlcSetPCF      = 0x0032,
    wlcSetPhyReg   = 0x003e,
    wlcTxTest      = 0x003f,
    wlcEnableTx    = 0x0101,
    wlcListBSS     = 0x0103,
    wlcSaveCFG     = 0x0108,
    wlcEnableAUX   = 0x0111,
    wlcAccessWrite = 0x0121,
    wlcUsePSPNodes = 0x0130,
    wlcEnableRx    = 0x0201
};
	
/*
 * Event flags for wlEvStat, wlInten, and wlEvAck
 */
enum WLEvent {
    wleRx        = 0x1,
    wleTx        = 0x2,
    wleTxExc     = 0x4,
    wleAlloc     = 0x8,
    wleCmd       = 0x10,
    wleLink      = 0x80,
    wleAwake     = 0x100,
    wleUnknown   = 0x800,
    wleMIC       = 0x1000,
    wleClearBusy = 0x4000,
    wleTick      = 0x8000,
};

enum WLBAP {
    wlbBAP0      = 0x0,
    wlbBAP1      = 0x2,
};

enum WLBAPStatus {
    wlsBusy	 = 0x8000,
    wlsError     = 0x4000,
    wlsDone      = 0x2000,
};

enum WLOpMode {
    wlomIBSS     = 0x0000,
    wlomESS      = 0x0001,
    wlomAP       = 0x0002,
    wlomRepeater = 0x0003,
    
    wlomEthernetHost = 0x0000,
    wlomLLCHost      = 0x0100,
    wlomAiroNetExt   = 0x0200,
    wlomAPInterface  = 0x0400,
    wlomAntennaAlign = 0x0800,
    wlomEtherLLC     = 0x1000,
    wlomLeafNode     = 0x2000,
    wlomCFPollable   = 0x4000,
    wlomMIC          = 0x8000,
};

#define wlResetTries 100
#define wlTimeout 6000000
#define wlDelay   100

class AiroJackCard {
public:
    AiroJackCard(void* ioBase, void* keyLargoBase, IOService* parent);
    ~AiroJackCard();

    IOReturn _reset();
        
    IOReturn startCapture(IODataQueue*, UInt16);
    IOReturn stopCapture();
    IOReturn sendFrame(UInt8* data, UInt32 repeat);
    IOReturn stopSendingFrames();
    IOReturn cardGone();
    
    void goSleep() { if(_cardPresent==1) _cardPresent=2; }
    void wakeUp()  { if(_cardPresent==2) _cardPresent=1; }
    
    /*
     * Register accessors (return/accept values in host byte order)
     */
    inline UInt16 getRegister(int r, bool littleEndian = true) {
        if (_cardPresent!=1) return wlsError;
        if (littleEndian)
            return OSReadLittleInt16(_ioBase, r);
        else
            return OSReadBigInt16(_ioBase, r);
    }
    inline void setRegister(int r, UInt16 v,
                            bool littleEndian = true) {
        if (_cardPresent!=1) return;
        if (littleEndian)
            OSWriteLittleInt16(_ioBase, r, v);
        else
            OSWriteBigInt16(_ioBase, r, v);
	
        OSSynchronizeIO();
    }    
    
    /*
     * LTV Record accessors
     * Length and type are native byte order, value is little-endian.
     * The Length is always one more than the number of 16-bit words.
     */
    IOReturn getValue(UInt16 rid, UInt16*);
    IOReturn setValue(UInt16 rid, UInt16);
    IOReturn getString(UInt16 rid, char*, size_t*);
    IOReturn setString(UInt16 rid, char*, size_t);
    IOReturn getRecord(UInt16 rid, void*, size_t, bool swapBytes = true);
    IOReturn setRecord(UInt16 rid, const void*, size_t, bool swapBytes = true);
    
    void enableInterrupts();
    void disableInterrupts();
    void ackAllInterrupts();
    IOReturn handleInterrupt();
    
private:
    /*
     * Private methods
     */
    static void 	_myTimeoutHandler(OSObject *owner, IOTimerEventSource *sender);
    IOReturn	 	_sendFrame(UInt8* data, IOByteCount size);
    IOReturn 		_enable();
    IOReturn 		_disable();
    
    /*
     * Buffer Access Path (BAP) access
     */
    IOReturn _readData(UInt16 id, UInt16 off, UInt16* buf, UInt16* n);
    IOReturn _writeData(UInt16 id, UInt16 off, UInt16* buf, UInt16 n);

    /*
     * Event handlers
     */
    IOReturn _handleRx();
    IOReturn _handleTx();
    IOReturn _handleTxExc();
    IOReturn _handleAlloc();
    IOReturn _handleCmd();
    IOReturn _handleInfo();

    /*
     * Miscellaneous operations
     */
    
    IOReturn _doCommand(WLCommandCode c, UInt16 p0,
			UInt16 p1 = 0, UInt16 p2 = 0, UInt16* ret = NULL);
    IOReturn _waitForNotBusy(WLRegister cmd, UInt16 cmd);
    IOReturn _waitForEvent(WLEvent e);
    IOReturn _readStats(StatsRid *sr);
    IOReturn _readStatus(StatusRid *statr);
    IOReturn _readCapability(CapabilityRid *capr);
    IOReturn _readSSID(SsidRid *ssidr);
    IOReturn _writeSSID(SsidRid *ssidr);
    IOReturn _readConfig(bool factoryConfig = false);
    IOReturn _writeConfig();
    void _printDebugInfo();
    
    IOReturn _bapsetup(UInt16 rid, WLBAP bap, int offset);
    /*
     * Private data members
     */
    void*  _ioBase;
    UInt8  _wcReg[5];
    UInt16 _interrupts;
    ConfigRid _cfg;

    int    _cardPresent;
    bool   _isEnabled;
    
    IODataQueue* _packetQueue;
    
    IOTimerEventSource	*_timedSendSource;
    UInt8		_data[2364];
    IOByteCount		_dataSize;
    UInt32		_timeout;
    UInt32		_failures;
    UInt16		_channel;
    IOWorkLoop		*_workLoop;
    
    IOService		*_parent;
};

#endif /* AiroJackCARD_H */

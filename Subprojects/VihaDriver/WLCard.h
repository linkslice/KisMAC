/* $Id: WLCard.h,v 1.4 2004/05/05 00:02:09 kismac Exp $ */

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
 * Wireless LAN Card
 *
 * All available wireless cards fall into three families: Cisco
 * Aironet, Lucent Hermes, and Intersil Prism.  The three families
 * have the same host software interface, which is implemented by the
 * WLCard class.
 *
 */

#ifndef WLCARD_H
#define WLCARD_H

#include <IOKit/IOService.h>
#include <IOKit/IOTypes.h>
#include <libkern/OSByteOrder.h>
#include <IOKit/IODataQueue.h>
#include <IOKit/IOTimerEventSource.h>
#include "WLFrame.h"

#define	_TX_CFPOLL			(0x1000)
#define	_TX_PRST                        (0x0800)
#define	_TX_MACPORT			(0x0700)
#define	_TX_NOENCRYPT			(0x0080)
#define	_TX_RETRYSTRAT			(0x0060)
#define	_TX_STRUCTYPE			(0x0018)
#define	_TX_TXEX                        (0x0004)
#define	_TX_TXOK                        (0x0001)
#define	_TX_SET(v,m,s)			((((UInt16)(v))<<((UInt16)(s)))&((UInt16)(m)))
#define	_TX_RETRYSTRAT_SET(v)		_TX_SET(v, _TX_RETRYSTRAT, 5)
#define	_TX_CFPOLL_SET(v)		_TX_SET(v, _TX_CFPOLL,12)
#define	_TX_MACPORT_SET(v)		_TX_SET(v, _TX_MACPORT, 8)
#define	_TX_TXEX_SET(v)			_TX_SET(v, _TX_TXEX, 2)
#define	_TX_TXOK_SET(v)			_TX_SET(v, _TX_TXOK, 1)

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

    // FID management
    wlInfoFID    = 0x10,
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

    // Control
    wlControl    = 0x14,

    // Host software registers
    wlSwSupport0 = 0x28,
    wlSwSupport1 = 0x2a,
    wlSwSupport2 = 0x2c,

    // Auxiliary port
    wlAuxPage    = 0x3a,
    wlAuxOffset  = 0x3c,
    wlAuxData    = 0x3e,
};

/*
 * Command register values (Lucent and Prism2 specific)
 */
enum WLCommandCode {
    wlcInit        = 0x0000,
    wlcEnable      = 0x0001,
    wlcDisable     = 0x0002,
    wlcDiag        = 0x0003,
    wlcAllocMem    = 0x000a,
    wlcTransmit    = 0x000b,
    wlcNotify      = 0x0010,
    wlcInquire     = 0x0011,
    wlcAccessRead  = 0x0021,
    wlcAccessWrite = 0x0121,
    wlcProgram     = 0x0022,
    wlcMonitorOn   = 0x0B38,
    wlcMonitorOff  = 0x0F38
};

/*
 * Event flags for wlEvStat, wlInten, and wlEvAck
 */
enum WLEvent {
    wleRx      = 0x1,
    wleTx      = 0x2,
    wleTxExc   = 0x4,
    wleAlloc   = 0x8,
    wleCmd     = 0x10,
    wleInfo    = 0x80,
    wleInfDrop = 0x2000,
    wleWTErr   = 0x4000,
    wleTick    = 0x8000,
};

/*
 * Register value bitfields
 */
struct WLCommand {
    int busy:1;
    int info:7;
    int code:8;
} __attribute__((packed));

struct WLStatus {
    int unused:1;
    int result:7;
    int unused2:2;
    int code:6;
} __attribute__((packed));

struct WLOffset {
    int busy:1;
    int error:1;
    int offset:14;
} __attribute__((packed));

#define wlTimeout 65536
#define wlDelay   100

struct WLHardwareAddress {
    UInt8 bytes[6];
};

struct WLIdentity {
    UInt16 vendor;
    UInt16 variant;
    UInt16 major;
    UInt16 minor;
};

class WLCard {
public:
    WLCard(void* ioBase, void* keyLargoBase, IOService* parent);
    ~WLCard();

    IOReturn _reset();
    IOReturn _powerOn();
    IOReturn _powerOff();
    
    /*
     * High-level Wireless network adapter API
     */
    IOReturn getHardwareAddress(WLHardwareAddress*);
    IOReturn getIdentity(WLIdentity*);
    
    IOReturn getChannel(UInt16*);
    IOReturn getAllowedChannels(UInt16*);
    IOReturn setChannel(UInt16);
    // void getFilter();
    // void setFilter();
    IOReturn startCapture(IODataQueue*, UInt16);
    IOReturn stopCapture();

    IOReturn sendFrame(UInt8* data, UInt32 repeat);
    IOReturn stopSendingFrames();
    
    /*
     * Register accessors (return/accept values in host byte order)
     */
    inline UInt16 getRegister(WLRegister r, bool littleEndian = true) {
        if (littleEndian)
            return OSReadLittleInt16(_ioBase, r);
        else
            return OSReadBigInt16(_ioBase, r);
    }
    inline void setRegister(WLRegister r, UInt16 v,
                            bool littleEndian = true) {
        if (littleEndian)
            OSWriteLittleInt16(_ioBase, r, v);
        else
            OSWriteBigInt16(_ioBase, r, v);
	OSSynchronizeIO();
    }

    inline UInt8 getKLRegister(unsigned long offset) {
	return *(UInt8*)((UInt8*)_keyLargoBase + offset);
    }

    inline void setKLRegister(unsigned long offset, UInt8 data) {
	*(UInt8*)((UInt8*)_keyLargoBase + offset) = data;
	eieio();
    }
    inline UInt32 getKLRegister32(unsigned long offset) {
	return OSReadLittleInt32(_keyLargoBase, offset);
    }
    inline void setKLRegister32(unsigned long offset, UInt32 data) {
	OSWriteLittleInt32(_keyLargoBase, offset, data);
	eieio();
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
    IOReturn getRecord(UInt16 rid, void*, size_t*, bool swapBytes = true);
    IOReturn setRecord(UInt16 rid, void*, size_t, bool swapBytes = true);
    
    void enableInterrupts();
    void disableInterrupts();
    void ackAllInterrupts();
    IOReturn handleInterrupt();
    
private:
    /*
     * Private methods
     */
    IOReturn _enable();
    IOReturn _disable();
  
    static void 	_myTimeoutHandler(OSObject *owner, IOTimerEventSource *sender);
    IOReturn	 	_sendFrame(UInt8* data, IOByteCount size);
  
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
    IOReturn _handleInfDrop();
    IOReturn _handleWTErr();
    IOReturn _handleTick();

    /*
     * Miscellaneous operations
     */

    IOReturn _doCommand(WLCommandCode c, UInt16 p0,
			UInt16 p1 = 0, UInt16 p2 = 0);
    IOReturn _waitForNotBusy(WLRegister r);
    IOReturn _waitForEvent(WLEvent e);
    
    /*
     * Private data members
     */
    void*  _ioBase;
    void*  _keyLargoBase;
    UInt8  _wcReg[5];
    UInt16 _channel;
    
    UInt16 _interrupts;

    bool   _isEnabled;
    
    IODataQueue* _packetQueue;

    IOTimerEventSource	*_timedSendSource;
    UInt8		_data[2364];
    IOByteCount		_dataSize;
    UInt32		_timeout;
    UInt32		_failures;
    IOWorkLoop		*_workLoop;
    
    IOService		*_parent;
};

#endif /* WLANCARD_H */

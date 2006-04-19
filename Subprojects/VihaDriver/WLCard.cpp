/* $Id: WLCard.cpp,v 1.4 2004/05/05 00:02:09 kismac Exp $ */

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

#include <IOKit/IOLib.h>
#include <libkern/OSByteOrder.h>
#include "WLCard.h"
#include "WLLog.h"
#include "WLDriver.h"

WLCard::
WLCard(void* ioBase, void* klBase, IOService* parent) : _ioBase(ioBase),
                                     _keyLargoBase(klBase),
        			     _interrupts(0),
                                     _isEnabled(false),
                                      _parent(parent)
{
    _powerOn();
    IODelay(200 * 1000);
    _reset();

    /*
     * Get firmware vendor and version
     */
    WLIdentity ident;
    if (getIdentity(&ident) != kIOReturnSuccess) {
        WLLogErr("WLCard::WLCard: Couldn't read card identity\n");
        return;
    }

    WLLogNotice("WLCard: Firmware vendor %d, variant %d, version %d.%d\n",
                ident.vendor, ident.variant, ident.major, ident.minor);

    WLHardwareAddress macAddr;
    if (getHardwareAddress(&macAddr) != kIOReturnSuccess) {
        WLLogErr("WLCard::WLCard: Couldn't read MAC address\n");
        return;
    }
    
    _workLoop = _parent->getWorkLoop();
    if (!_workLoop) {
        WLLogErr("WLCard::WLCard: Failed to create workloop.\n");
        return;
    }
    
    _timedSendSource = IOTimerEventSource::timerEventSource(_parent, &WLCard::_myTimeoutHandler);
    if (!_timedSendSource) {
        WLLogErr("WLCard::WLCard: Failed to create timer event source.\n");
        return;
    }

    if (_workLoop->addEventSource(_timedSendSource) != kIOReturnSuccess) {
        WLLogErr("WLCard::WLCard: Failed to register timer event source.\n");
        return;
    }
}

WLCard::
~WLCard()
{
    if (_timedSendSource) {
        _timedSendSource->cancelTimeout();
        if (_workLoop) _workLoop->removeEventSource(_timedSendSource);
	_timedSendSource->release();
    }
    _reset();
    _powerOff();
}

IOReturn WLCard::getHardwareAddress(WLHardwareAddress* addr)
{
    size_t size = sizeof(WLHardwareAddress);
    
    if (getRecord(0xFC01, addr, &size, false) != kIOReturnSuccess) {
	WLLogErr("WLCard::getHardwareAddress: getRecord error\n");
	return kIOReturnError;
    }

    WLLogDebug("WLCard: MAC 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x\n",
                addr->bytes[0], addr->bytes[1], addr->bytes[2],
                addr->bytes[3], addr->bytes[4], addr->bytes[5]);

    return kIOReturnSuccess;
}

IOReturn WLCard::getIdentity(WLIdentity* wli)
{
    size_t size = sizeof(WLIdentity);
    if (getRecord(0xFD20, (UInt16*)wli, &size) != kIOReturnSuccess) {
        WLLogErr("WLCard::getIdentity: getRecord failed\n");
        return kIOReturnError;
    }

    return kIOReturnSuccess;
}

IOReturn WLCard::getChannel(UInt16* channel) {
    if (getValue(0xFDC1, channel) != kIOReturnSuccess) {
        WLLogErr("WLCard::getChannel: getValue error\n");
        return kIOReturnError;
    }

    _channel = *channel;
    return kIOReturnSuccess;
}

IOReturn WLCard::getAllowedChannels(UInt16* channel) {
    if (getValue(0xFD10, channel) != kIOReturnSuccess) {
        WLLogErr("WLCard::getAllowedChannels: getValue error\n");
        return kIOReturnError;
    }

    return kIOReturnSuccess;
}

IOReturn WLCard::setChannel(UInt16 channel) {
    if (setValue(0xFC03, channel) != kIOReturnSuccess) {
        WLLogErr("WLCard::setChannel: setValue error\n");
        return kIOReturnError;
    }

    if (_isEnabled) {
        if (_disable() != kIOReturnSuccess) {
            WLLogErr("WLCard::setChannel: Couldn't disable card\n");
            return kIOReturnError;
        }
        if (_enable() != kIOReturnSuccess) {
            WLLogErr("WLCard::setChannel: Couldn't enable card\n");
            return kIOReturnError;
        }
    }
    
    _channel = channel;
    return kIOReturnSuccess;
}

IOReturn WLCard::startCapture(IODataQueue* dq, UInt16 channel) {
    _packetQueue = dq;
    
    if (_disable() != kIOReturnSuccess) {
        WLLogErr("WLCard::startCapture: Couldn't disable card\n");
        return kIOReturnError;
    }
    
    if (setChannel(channel) != kIOReturnSuccess) {
        WLLogErr("WLCard::startCapture: setChannel(%d) failed\n",
                 channel);
        return kIOReturnError;
    }

    if (_doCommand(wlcMonitorOn, 0) != kIOReturnSuccess) {
        WLLogErr("WLCard::startCapture: _doCommand(wlcMonitorOn) failed\n");
        return kIOReturnError;
    }

    if (_enable() != kIOReturnSuccess) {
        WLLogErr("WLCard::startCapture: Couldn't enable card\n");
        return kIOReturnError;
    }
    
    _channel = channel;
    return kIOReturnSuccess;
}

IOReturn WLCard::stopCapture() {
    //test
    if (_doCommand(wlcMonitorOff, 0) != kIOReturnSuccess) {
        WLLogErr("WLCard::stopCapture: _doCommand(wlcMonitorOff) failed\n");
        return kIOReturnError;
    }

    return kIOReturnSuccess;
    return _disable();
}

IOReturn WLCard::sendFrame(UInt8* data, UInt32 repeat) {
    WLFrame *frameDescriptor;
    UInt8 aData[2364];
    IOByteCount pktsize;
    
    WLLogInfo("WLCard::sendFrame()\n");

    memcpy(aData, data, sizeof(WLFrame));
    frameDescriptor = (WLFrame*)aData;
    switch(frameDescriptor->frameControl & 0x0c) {
        case 0x08:
            WLLogInfo("WLCard:: send Data Packet\n");
        case 0x00:
            WLLogInfo("WLCard:: send Management Packet\n");
            pktsize = frameDescriptor->dataLen;
            WLLogInfo("WLCard:: data len: %u\n", (int)pktsize);
            if ((pktsize + sizeof(WLFrame)) > 2364) return kIOReturnBadArgument;
            frameDescriptor->dataLen=OSSwapHostToLittleInt16(frameDescriptor->dataLen);
            break;
        case 0x04:
            WLLogInfo("WLCard:: send Control Packet\n");
            pktsize = 0;
            frameDescriptor->dataLen = 0;
            break;
        default:
            WLLogErr("WARNING! WLCard: sendFrame: Unknown Packettype: 0x%x\n",frameDescriptor->frameControl);
            return kIOReturnBadArgument;
    }

    frameDescriptor->txControl=OSSwapHostToLittleInt16(0x08 | _TX_RETRYSTRAT_SET(3)| _TX_CFPOLL_SET(1) | _TX_TXEX_SET(1) | _TX_TXOK_SET(1) | _TX_MACPORT_SET(0));
    
    //frameDescriptor->txControl=OSSwapHostToLittleInt16(0x08);
    frameDescriptor->rate = 0x6e;	//11 MBit/s
    frameDescriptor->tx_rate = 0x6e;	//11 MBit/s

    memcpy(aData + 0x3C, data + sizeof(WLFrame), pktsize);

    if (_sendFrame(aData, pktsize + 0x3C) != kIOReturnSuccess) {
        WLLogCrit("WLCard::sendFrame() transmittion failed\n");
        return kIOReturnError;
    }
    
    if ((repeat != 0)&&(_timedSendSource != NULL)) {
        _timedSendSource->cancelTimeout();
        _dataSize = pktsize + 0x3C;
        memcpy(_data, aData, _dataSize);
        _timeout = repeat;
        _failures = 0;
        _timedSendSource->setTimeoutMS(_timeout);
    }

    WLLogInfo("WLCard::sendFrame(): sendFrame returning with success.\n");
    
    return kIOReturnSuccess;
}

IOReturn WLCard::stopSendingFrames() {
    if (_timedSendSource) _timedSendSource->cancelTimeout();
    _failures = 99;
    return kIOReturnSuccess;
}

#pragma mark -

IOReturn WLCard::getValue(UInt16 rid, UInt16* v)
{
    size_t n = 2;
    return getRecord(rid, v, &n);
}

IOReturn WLCard::setValue(UInt16 rid, UInt16 v)
{
    UInt16 value = v;
    IOReturn ret = setRecord(rid, &value, 2);

    if (ret != kIOReturnSuccess)
        return ret;

    ret = getValue(rid, &value);

    if (ret != kIOReturnSuccess)
        return ret;

    if (value != v) {
        WLLogErr("WLCard::setValue: Failed to set value (0x%x != 0x%x)\n",
                 value, v);
        return kIOReturnError;
    }

    return kIOReturnSuccess;
}

IOReturn WLCard::getString(UInt16 rid, char* str, size_t* n)
{
    /*
     * Strings are stored as a little-endian 16-bit length followed by
     * the string data (big-endian).
     */
    IOReturn ret = getRecord(rid, str, n, false);
    if (ret == kIOReturnSuccess) {
	UInt16 length = ((UInt8)str[1] << 8) | (UInt8)str[0];
	str[length] = '\0';
    }
    return ret;
}

IOReturn WLCard::setString(UInt16 rid, char* str, size_t n)
{
    return kIOReturnSuccess;
}

/*
 * Read a record, specified by rid, into memory of size n bytes
 * pointed to by buf.  The number of bytes read is stored in n before
 * returning.  If swapBytes is true, the data is assumed to be
 * little-endian and is swapped.
 */
IOReturn
WLCard::getRecord(UInt16 rid, void* buf, size_t* n, bool swapBytes)
{
    if (_doCommand(wlcAccessRead, rid) != kIOReturnSuccess) {
	WLLogErr("WLCard::getRecord: Read command failed\n");
	return kIOReturnError;
    }
    
    setRegister(wlSelect0, rid);
    setRegister(wlOffset0, 0);

    if (_waitForNotBusy(wlOffset0) != kIOReturnSuccess) {
	WLLogErr("WLCard::getRecord: _waitForNotBusy error\n");
	return kIOReturnError;
    }

    WLOffset off;
    *((UInt16*)&off) = getRegister(wlOffset0);
    if (off.error) {
	WLLogErr("WLCard::getRecord: Error\n");
	return kIOReturnError;
    }

    /*
     * Read and check the length
     */
    UInt16 length = getRegister(wlData0);
    if (length > *n / 2 + 1) {
	WLLogCrit("WLCard::getRecord: Length too large (%d > %ld)\n",
                  (length - 1) * 2, *n);
	return kIOReturnError;
    }

    /*
     * Read and check type
     */
    UInt16 type = getRegister(wlData0);
    if (type != rid) {
	WLLogCrit("WLCard::getRecord: Type != RID (0x%x != 0x%x)\n",
                  type, rid);
	return kIOReturnError;
    }
    
    UInt16* ui16Buf = (UInt16*)buf;
    for (int i = 0; i < (length - 1); i++) {
	if (swapBytes)
	    ui16Buf[i] = getRegister(wlData0);
	else
	    ui16Buf[i] = getRegister(wlData0,false);//ui16Buf[i] = *(UInt16*)((UInt16*)_ioBase + (unsigned long)wlData0);
    }

    *n = (length - 1) * 2;
    
    return kIOReturnSuccess;
}

IOReturn
WLCard::setRecord(UInt16 rid, void* buf, size_t n, bool swapBytes)
{
    setRegister(wlSelect0, rid);
    setRegister(wlOffset0, 0);

    if (_waitForNotBusy(wlOffset0) != kIOReturnSuccess) {
	WLLogErr("WLCard::getRecord: _waitForNotBusy error\n");
	return kIOReturnError;
    }

    UInt16 length = (n / 2) + 1;
    setRegister(wlData0, length);
    setRegister(wlData0, rid);
    
    UInt16* ui16buf = (UInt16*)buf;
    for (int i = 0; i < length; i++) {
	if (swapBytes)
	    setRegister(wlData0, ui16buf[i]);
	else
	    setRegister(wlData0, ui16buf[i], false); //*(UInt16*)((UInt16*)_ioBase + (unsigned long)wlData0) = ui16buf[i];
    }

    if (_doCommand(wlcAccessWrite, rid) != kIOReturnSuccess) {
	WLLogErr("WLCard::setRecord: Write command failed\n");
	return kIOReturnError;
    }
    
    return kIOReturnSuccess;
}

IOReturn WLCard::_enable()
{
    if (_doCommand(wlcEnable, 0) != kIOReturnSuccess) {
        WLLogErr("WLCard::startCapture: _doCommand(wlcEnable) failed\n");
        return kIOReturnError;
    }
    _isEnabled = true;

    enableInterrupts();

    return kIOReturnSuccess;
}

IOReturn WLCard::_disable()
{
    disableInterrupts();
    ackAllInterrupts();
    
    if (_doCommand(wlcDisable, 0) != kIOReturnSuccess) {
        WLLogErr("WLCard::_disable: _doCommand(wlcDisable) failed\n");
        return kIOReturnError;
    }
    _isEnabled = false;

    return kIOReturnSuccess;
}

void WLCard::_myTimeoutHandler(OSObject *owner, IOTimerEventSource *sender) {
    WLanDriver *me;
    WLCard *card;
    int backoff=0;

    WLLogInfo("WLCard::_myTimeoutHandler()\n");

    // Make sure that the owner of the timer is us.
    me = OSDynamicCast(WLanDriver, owner);
    if (!me) return;
 
    card = me->getWLCard();
    
    if (!card) {
        WLLogErr("WLCard::_myTimeoutHandler: WARNING could not get WLCard reference.\n");
        return;
    };
    
    if (card->_failures>25) {
        WLLogErr("WLCard::_myTimeoutHandler: WARNING timed sendFrame terminated because of general failure.\n");
        return;
    };
    
    if (card->_sendFrame(card->_data,card->_dataSize)!=kIOReturnSuccess)  {
        card->_failures++;
        if (card->_failures%10) {
            card->_reset();
            IODelay(50);
        }
        backoff=10;
    } else card->_failures=0;
    
    // reset the timer
    sender->setTimeoutMS(card->_timeout + backoff);
}

IOReturn WLCard::_powerOn()
{
    /*
     * Magic monkey mojo gleamed from OpenBSD sources
     */

    /*
     * XXX: ???
     */
    UInt32 x;
    
    x = getKLRegister32(0x40);
    x |= 0x4;
    setKLRegister32(0x40, x);
    
    /*
     * Enable card slot.
     */
    setKLRegister(0x6a + 0x0f, 5);
    IODelay(10 * 1000);
    setKLRegister(0x6a + 0x0f, 4);
    IODelay(10 * 1000);

    /*
     * XXX: ???
     */
    x = getKLRegister32(0x40);
    x &= ~0x8000000;
    setKLRegister32(0x40, x);
    IODelay(10);
    setKLRegister(0x58 + 0xb, 0);
    IODelay(10);
    setKLRegister(0x58 + 0xa, 0x28);
    IODelay(10);
    setKLRegister(0x58 + 0xd, 0x28);
    IODelay(10);
    setKLRegister(0x6a + 0xd, 0x28);
    IODelay(10);
    setKLRegister(0x6a + 0xe, 0x28);
    IODelay(10);
    setKLRegister32(0x1c000, 0);
    IODelay(1 * 1000);
    
    /*
     * Initialize the card.
     */
    setKLRegister32(0x1a3e0, 0x41);
    IODelay(10);
    x = getKLRegister32(0x40);
    x |= 0x8000000;
    setKLRegister32(0x40, x);
    IODelay(100 * 1000);
    
    return kIOReturnSuccess;
}

IOReturn WLCard::_powerOff()
{
    UInt32 x = getKLRegister32(0x40);
    x &= ~0x8000000;
    setKLRegister32(0x40, x);

    setKLRegister(0x58 + 0xa, 0);
    setKLRegister(0x58 + 0xd, 0);
    setKLRegister(0x6a + 0xd, 0);
    setKLRegister(0x6a + 0xe, 0);
    setKLRegister(0x6a + 0xf, 0);
    
    return kIOReturnSuccess;
}

IOReturn WLCard::
_reset()
{
    if (_doCommand(wlcInit, 0) != kIOReturnSuccess) {
        WLLogErr("WLCard::_reset: _doCommand(wlcInit, 0) failed\n");
        return kIOReturnError;
    }

    if (setValue(0xFC00, 0x3) != kIOReturnSuccess) {
        WLLogErr("WLCard::_reset: could not set port type\n");
        return kIOReturnError;
    }

    disableInterrupts();
    ackAllInterrupts();

    /*
     * Write and check a magic number to the Software0 register
     */
    UInt16 magic = 0x1ee7;
    setRegister(wlSwSupport0, magic);
    if (getRegister(wlSwSupport0) != 0x1ee7) {
        WLLogCrit("WLCard::_reset: Magic check failed\n");
        return kIOReturnError;
    }   
    
    /*
     * Set list of interesting events
     */
    _interrupts = wleRx;

    _enable();

    return kIOReturnSuccess;
}

IOReturn WLCard::
_doCommand(WLCommandCode cmd, UInt16 param0, UInt16 param1, UInt16 param2)
{
    /*
     * Wait for busy bit to clear on command register
     */
    if (_waitForNotBusy(wlCommand) != kIOReturnSuccess)
	WLLogErr("WLCard::_doCommand: _waitForNotBusy error\n");

    /*
     * Write parameters and command code to registers
     */
    setRegister(wlParam0, param0);
    setRegister(wlParam1, param1);
    setRegister(wlParam2, param2);
    setRegister(wlCommand, cmd);

    if (_waitForNotBusy(wlCommand) != kIOReturnSuccess)
	WLLogErr("WLCard::_doCommand: _waitForNotBusy error\n");

    /*
     * When the command is complete, the wlCmd bit will be set in the
     * wlEvStat register.  Polling it for it here is easier than
     * waiting for the interrupt.  Once the command is complete, we
     * can read the result code out of the wlStatus register.
     */
    if (_waitForEvent(wleCmd) != kIOReturnSuccess) {
	WLLogErr("WLCard::_doCommand: Command 0x%x timed out\n", cmd);
	return kIOReturnError;
    }
    
    UInt16 status;
    *((UInt16*)&status) = getRegister(wlStatus);

    setRegister(wlEvAck, wleCmd);
    
   if ((status & 0x3F) != (cmd & 0x3F)) {
        WLLogCrit("WLCard::_doCommand: Status code (0x%x) != cmd (0x%x)\n",
                  status & 0x3F, cmd & 0x3F);
        return kIOReturnError;
    }

    if (status & 0x7F00) {
	WLLogErr("WLCard::_doCommand: Command result 0x%x\n",
                 status & 0x7F00);
    }

    return kIOReturnSuccess;
}

/*********************************************************************
 *                          Event Handling                           *
 *********************************************************************/
 
/*
 * Wait for busy bit to clear on register wlCommand or wlOffset
 */
IOReturn WLCard::
_waitForNotBusy(WLRegister reg)
{
    int i;

    if (!(reg == wlCommand || reg == wlOffset0 || reg == wlOffset1)) {
        WLLogCrit("WLCard::_waitForNotBusy: Register has no busy bit!\n");
        return kIOReturnError;
    }
    
    for (i = 0; i < wlTimeout; i++) {
	UInt16 val = getRegister(reg);
        if (val & 0x4000)
            return kIOReturnError;
	if ((val & 0x8000) == 0)
	    break;
    }
    
    if (i == wlTimeout) {
	WLLogWarn("WLCard::_waitForNotBusy: Timeout\n");
	return kIOReturnTimeout;
    }
    
    return kIOReturnSuccess;
}

/*
 * Wait for a specific event to occur
 */
IOReturn WLCard::
_waitForEvent(WLEvent e)
{
    int i;
    for (i = 0; i < wlTimeout; i++) {
	UInt16 evStat = getRegister(wlEvStat);
	if (evStat & e) {
	    break;
        }
    }
    if (i == wlTimeout) {
        WLLogWarn("WLCard::_waitForEvent: timeout\n");
	return kIOReturnError;
    }
    
    return kIOReturnSuccess;
}

#define WI_HERMES_AUTOINC_WAR

IOReturn WLCard::
_sendFrame(UInt8* data, IOByteCount size)
{
    int id;

    if (size%2) {
        data[size]=0;
        size++;
    }
    
    WLLogInfo("WLCard::_sendFrame: %u\n", (int)size);
    
    if (_doCommand(wlcAllocMem, size)  != kIOReturnSuccess) {
        WLLogCrit("WLCard::_sendFrame: failed to allocate %u bytes on NIC\n", (int)size);
        return kIOReturnError;
    }

    if (_waitForEvent(wleAlloc) != kIOReturnSuccess) {
        WLLogCrit("WLCard::_sendFrame: timeout failure\n");
        return kIOReturnError;
    }
    
    setRegister(wlEvAck, wleAlloc);    
    id = getRegister(wlAllocFID);
    WLLogInfo("WLCard::_sendFrame: FID is 0x%x\n", id);
    
#ifdef WI_HERMES_AUTOINC_WAR
again:
#endif
    setRegister(wlSelect0, id);
    setRegister(wlOffset0, 0);

    if (_waitForNotBusy(wlOffset0) != kIOReturnSuccess) {
	WLLogErr("WLCard::_sendFrame: _waitForNotBusy error\n");
	return kIOReturnError;
    }
    
    for (UInt32 i = 0; i < size; i+=2)
        setRegister(wlData0, *((UInt16*)&data[i]), false); //*(UInt16*)((UInt16*)_ioBase + (unsigned long)(wlData0>>1)) = *((UInt16*)&data[i]);
    
#ifdef WI_HERMES_AUTOINC_WAR

    setRegister(wlSelect0, id);
    setRegister(wlOffset0, 0);

    if (_waitForNotBusy(wlOffset0) != kIOReturnSuccess) {
	WLLogErr("WLCard::_sendFrame: _waitForNotBusy error\n");
	return kIOReturnError;
    }
    
    for (UInt32 i = 0;i<size;i+=2) 
        if (getRegister(wlData0, false) != *((UInt16*)&data[i])) {
            WLLogCrit("WLCard::_sendFrame: autoinc error. Retrying...\n");
        goto again;
        }
    
#endif

    if (_doCommand(wlcTransmit, id)  != kIOReturnSuccess) {
        WLLogCrit("WLCard::_sendFrame: could not transmit packet\n");
        return kIOReturnError;
    }

    return kIOReturnSuccess;
}

void WLCard::
enableInterrupts()
{
    setRegister(wlIntEn, _interrupts);
}

void WLCard::
disableInterrupts()
{
    setRegister(wlIntEn, ~_interrupts);
}

void WLCard::
ackAllInterrupts() {
    /*
     * Acknowledge all interrupts by writing a "spam ack"
     */
    setRegister(wlEvAck, 0xFFFF);
}

IOReturn WLCard::
handleInterrupt()
{
    disableInterrupts();
    UInt16 evStat = getRegister(wlEvStat);
    setRegister(wlEvAck, ~_interrupts);

    /*
     * Check for "phantom interrupt"
     */
    if (!evStat)
	return kIOReturnSuccess;

    if (evStat & wleRx) {
	_handleRx();
        setRegister(wlEvAck, wleRx);
    }
    if (evStat & wleTx) {
	_handleTx();
        setRegister(wlEvAck, wleTx);
    }
    if (evStat & wleTxExc) {
	_handleTxExc();
        setRegister(wlEvAck, wleTxExc);
    }
    if (evStat & wleAlloc) {
	_handleAlloc();
        setRegister(wlEvAck, wleAlloc);
    }
    if (evStat & wleCmd) {
	_handleCmd();
        setRegister(wlEvAck, wleCmd);
    }
    if (evStat & wleInfo) {
	_handleInfo();
        setRegister(wlEvAck, wleInfo);
    }
    if (evStat & wleInfDrop) {
	_handleInfDrop();
        setRegister(wlEvAck, wleInfDrop);
    }
    if (evStat & wleWTErr) {
	_handleWTErr();
        setRegister(wlEvAck, wleWTErr);
    }
    if (evStat & wleTick) {
	_handleTick();
        setRegister(wlEvAck, wleTick);
    }

    enableInterrupts();

    return kIOReturnSuccess;
}

IOReturn WLCard::
_handleRx() {
    UInt16 packet[1182];    /* 60 byte header + 2304 data bytes */
    WLFrame* frameDescriptor = (WLFrame*)packet;
    UInt16* frameData = &packet[30];
    
    UInt16 FID = getRegister(wlRxFID);
    
    setRegister(wlSelect0, FID);
    setRegister(wlOffset0, 0);
    
    if (_waitForNotBusy(wlOffset0) != kIOReturnSuccess) {
	WLLogWarn("WLCard::_handleRx: Timeout or error\n");
	return kIOReturnError;
    }

    /*
     * Read frame descriptor correcting the byte order as we go (the
     * control and 802.11 fields are little endian, the 802.3 fields are
     * big endian).
     */
    int i;
    for (i = 0; i < 30; i++)
        packet[i] = getRegister(wlData0, false);
    frameDescriptor->status = OSSwapLittleToHostInt16(frameDescriptor->status);
    frameDescriptor->dataLen = OSSwapLittleToHostInt16(frameDescriptor->dataLen);
    frameDescriptor->channel = _channel;

    /*
     * If the frame has an FCS error, is received on a MAC port other
     * than the monitor mode port, or is a message type other than
     * normal, we don't want it.
     */
    if (frameDescriptor->status & 0x1 ||
        (frameDescriptor->status & 0x700) != 0x700 ||
        frameDescriptor->status & 0xe000) {
        return kIOReturnSuccess;
    }

    if (frameDescriptor->dataLen > 2304) {
        WLLogCrit("WLCard::_handleRx: Oversized packet (%d bytes)\n",
                  frameDescriptor->dataLen);
        return kIOReturnSuccess;
    }

    /*
     * Read in the packet data.  Read 4 extra words for IV + ICV if
     * applicable.
     */
    UInt16 dataLength = (frameDescriptor->dataLen / 2) + 4;
    for (i = 0; i < dataLength; i++)
        frameData[i] = getRegister(wlData0, false);

    if (_packetQueue) {
        UInt32 packetLength = sizeof(WLFrame) + (dataLength * 2);
        if (!_packetQueue->enqueue(packet, packetLength))
            WLLogCrit("WLCard::_handleRx: packet queue overflow\n");
    }
    
    return kIOReturnSuccess;
}

IOReturn WLCard::
_handleTx() {
    return kIOReturnSuccess;
}

IOReturn WLCard::
_handleTxExc() {
    return kIOReturnSuccess;
}

IOReturn WLCard::
_handleAlloc() {
    return kIOReturnSuccess;
}

IOReturn WLCard::
_handleCmd() {
    return kIOReturnSuccess;
}

IOReturn WLCard::
_handleInfo() {
    return kIOReturnSuccess;
}

IOReturn WLCard::
_handleInfDrop() {
    return kIOReturnSuccess;
}

IOReturn WLCard::
_handleWTErr() {
    return kIOReturnSuccess;
}

IOReturn WLCard::
_handleTick() {
    return kIOReturnSuccess;
}

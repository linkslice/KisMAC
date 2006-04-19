/*
        
        File:			MACJackCard.cpp
        Program:		MACJack
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
#include <IOKit/IOWorkLoop.h>
#include <IOKit/IOLib.h>
#include <libkern/OSByteOrder.h>
#include <AvailabilityMacros.h>
#include <kern/clock.h>
#include "MACJackCard.h"
#include "MACJackLog.h"
#include "MACJackDriver.h"
#include "MACJackCardIDs.h"

struct wi_card_ident {
	const u_int16_t	card_id;
	const char	*card_name;
	const u_int8_t	firm_type;
};

const struct wi_card_ident wi_card_ident[] = {
	WI_CARD_IDS
};
    
MACJackCard::
MACJackCard(void* ioBase, void* klBase, IOService* parent) : _ioBase(ioBase),
        			     _interrupts(0),
                                     _cardPresent(1),
                                     _isEnabled(false),
                                     _firmwareType(-1),
                                     _parent(parent)
{
    IODelay(200*1000);
    
    for (int i = 0; i< wlResetTries; i++) {
        if (_reset() == kIOReturnSuccess) break;
    }
    
    /*
     * Get firmware vendor and version
     */
     
    WLLogDebug("MACJackDriver::start: Reset of the card complete\n");
    WLIdentity ident;
    if (getIdentity(&ident) != kIOReturnSuccess) {
        WLLogCrit("MACJackCard::MACJackCard: Couldn't read card identity\n");
        return;
    }

	WLLogCrit("MACJack version from %s %s\n",  __DATE__, __TIME__);
    WLLogCrit("MACJackCard: Firmware vendor %d, variant %d, version %d.%d\n",
                ident.vendor, ident.variant, ident.major, ident.minor);

    WLHardwareAddress macAddr;
    if (getHardwareAddress(&macAddr) != kIOReturnSuccess) {
        WLLogCrit("MACJackCard::MACJackCard: Couldn't read MAC address\n");
        return;
    }
    
    _workLoop = _parent->getWorkLoop();
    if (!_workLoop) {
        WLLogCrit("MACJackCard::MACJackCard: Failed to create workloop.\n");
        return;
    }
    
    _timedSendSource = IOTimerEventSource::timerEventSource(_parent, &MACJackCard::_myTimeoutHandler);
    if (!_timedSendSource) {
        WLLogCrit("MACJackCard::MACJackCard: Failed to create timer event source.\n");
        return;
    }

    if (_workLoop->addEventSource(_timedSendSource) != kIOReturnSuccess) {
        WLLogCrit("MACJackCard::MACJackCard: Failed to register timer event source.\n");
        return;
    }
}

MACJackCard::
~MACJackCard()
{
    if (_timedSendSource) {
        _timedSendSource->cancelTimeout();
        if (_workLoop) _workLoop->removeEventSource(_timedSendSource);
	_timedSendSource->release();
    }
    //_reset();
}

#pragma mark -

IOReturn MACJackCard::getHardwareAddress(WLHardwareAddress* addr)
{
    size_t size = sizeof(WLHardwareAddress);
    
    if (getRecord(0xFC01, (UInt16*)addr, &size, false) != kIOReturnSuccess) {
	WLLogErr("MACJackCard::getHardwareAddress: getRecord error\n");
	return kIOReturnError;
    }

    WLLogDebug("MACJackCard: MAC 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x\n",
                addr->bytes[0], addr->bytes[1], addr->bytes[2],
                addr->bytes[3], addr->bytes[4], addr->bytes[5]);

    return kIOReturnSuccess;
}

IOReturn MACJackCard::getIdentity(WLIdentity* wli)
{
    size_t size = sizeof(WLIdentity);
    if (getRecord(0xFD20, (UInt16*)wli, &size) != kIOReturnSuccess) {
        WLLogErr("MACJackCard::getIdentity: getRecord failed\n");
        return kIOReturnError;
    }

    return kIOReturnSuccess;
}

IOReturn MACJackCard::getChannel(UInt16* channel) {
    if (getValue(0xFDC1, channel) != kIOReturnSuccess) {
        WLLogErr("MACJackCard::getChannel: getValue error\n");
        return kIOReturnError;
    }
    
    _channel = *channel;
    return kIOReturnSuccess;
}

IOReturn MACJackCard::getAllowedChannels(UInt16* channels) {
    if (getValue(0xFD10, channels) != kIOReturnSuccess) {
        WLLogErr("MACJackCard::getAllowedChannels: getValue error\n");
        return kIOReturnError;
    }
    
    return kIOReturnSuccess;
}

IOReturn MACJackCard::setChannel(UInt16 channel) {
    if (setValue(0xFC03, channel) != kIOReturnSuccess) {
        if (setValue(0xFC03, channel) != kIOReturnSuccess) {
            WLLogInfo("MACJackCard::setChannel: setValue error\n");
            return kIOReturnError;
        }
    }

    if (_isEnabled) {
        if (_disable() != kIOReturnSuccess) {
            WLLogErr("MACJackCard::setChannel: Couldn't disable card\n");
            return kIOReturnError;
        }
        if (_enable() != kIOReturnSuccess) {
            WLLogErr("MACJackCard::setChannel: Couldn't enable card\n");
            return kIOReturnError;
        }
    }
    
    _channel = channel;
    return kIOReturnSuccess;
}

IOReturn MACJackCard::startCapture(IODataQueue* dq, UInt16 channel) {
    _packetQueue = dq;
    
    if (_disable() != kIOReturnSuccess) {
        WLLogErr("MACJackCard::startCapture: Couldn't disable card\n");
        return kIOReturnError;
    }
    
    if (setChannel(channel) != kIOReturnSuccess) {
        WLLogErr("MACJackCard::startCapture: setChannel(%d) failed\n",
                 channel);
        return kIOReturnError;
    }

#ifdef USEHOSTAP
    if ((_firmwareType == WI_LUCENT) && (_doCommand(wlcMonitorOn, 0) != kIOReturnSuccess)) {
#else
    if (_doCommand(wlcMonitorOn, 0) != kIOReturnSuccess) {
#endif
        WLLogErr("MACJackCard::startCapture: _doCommand(wlcMonitorOn) failed\n");
        return kIOReturnError;
    }

    if (_enable() != kIOReturnSuccess) {
        WLLogErr("MACJackCard::startCapture: Couldn't enable card\n");
        return kIOReturnError;
    }
    
    _channel = channel;
    return kIOReturnSuccess;
}

IOReturn MACJackCard::stopCapture() {
    //test
#ifdef USEHOSTAP
    if ((_firmwareType == WI_LUCENT) && (_doCommand(wlcMonitorOff, 0) != kIOReturnSuccess)) {
#else
    if (_doCommand(wlcMonitorOff, 0) != kIOReturnSuccess) {
#endif
        WLLogErr("MACJackCard::stopCapture: _doCommand(wlcMonitorOff) failed\n");
        return kIOReturnError;
    }

    return kIOReturnSuccess;
    return _disable();
}

IOReturn MACJackCard::sendFrame(UInt8* data, UInt32 repeat) {
    WLFrame *frameDescriptor;
    UInt8 aData[2364];
    IOByteCount pktsize;
    
    WLLogInfo("MACJackCard::sendFrame()\n");

    memcpy(aData, data, sizeof(WLFrame));
    frameDescriptor = (WLFrame*)aData;
    switch(frameDescriptor->frameControl & 0x0c) {
        case 0x08:
            WLLogInfo("MACJackCard:: send Data Packet\n");
        case 0x00:
            WLLogInfo("MACJackCard:: send Management Packet\n");
            pktsize = frameDescriptor->dataLen;
            WLLogInfo("MACJackCard:: data len: %u\n", (int)pktsize);
            if ((pktsize + sizeof(WLFrame)) > 2364) return kIOReturnBadArgument;
            frameDescriptor->dataLen=OSSwapHostToLittleInt16(frameDescriptor->dataLen);
            break;
        case 0x04:
            WLLogInfo("MACJackCard:: send Control Packet\n");
            pktsize = 0;
            frameDescriptor->dataLen = 0;
            break;
        default:
            WLLogErr("WARNING! MACJackCard: sendFrame: Unknown Packettype: 0x%x\n",frameDescriptor->frameControl);
            return kIOReturnBadArgument;
    }

    frameDescriptor->txControl=OSSwapHostToLittleInt16(0x8);
    
    //frameDescriptor->txControl=OSSwapHostToLittleInt16(0x08);
    frameDescriptor->rate = 0x6e;	//11 MBit/s
    frameDescriptor->tx_rate = 0x6e;	//11 MBit/s

    memcpy(aData + 0x3C, data + sizeof(WLFrame), pktsize);

    if (_sendFrame(aData, pktsize + 0x3C) != kIOReturnSuccess) {
        WLLogInfo("MACJackCard::sendFrame() transmittion failed\n");
        return kIOReturnError;
    } else {
        WLLogInfo("MACJackCard::sendFrame() transmittion successful\n");
    }
    
    if ((repeat != 0)&&(_timedSendSource != NULL)) {
        _timedSendSource->cancelTimeout();
        _dataSize = pktsize + 0x3C;
        memcpy(_data, aData, _dataSize);
        _timeout = repeat;
        _failures = 0;
        _timedSendSource->setTimeoutMS(_timeout);
    }

    WLLogInfo("MACJackCard::sendFrame(): sendFrame returning with success.\n");
    
    return kIOReturnSuccess;
}

IOReturn MACJackCard::stopSendingFrames() {
    if (_timedSendSource) _timedSendSource->cancelTimeout();
    _failures = 99;
    return kIOReturnSuccess;
}

IOReturn MACJackCard::cardGone() {
    _cardPresent = 0;
    return stopSendingFrames();
}

#pragma mark -

IOReturn MACJackCard::getValue(UInt16 rid, UInt16* v)
{
    size_t n = 2;
    return getRecord(rid, v, &n);
}

IOReturn MACJackCard::setValue(UInt16 rid, UInt16 v)
{
    UInt16 value = v;
    IOReturn ret = setRecord(rid, &value, 2);

    if (ret != kIOReturnSuccess)
        return ret;

    ret = getValue(rid, &value);

    if (ret != kIOReturnSuccess)
        return ret;

    if (value != v) {
        WLLogErr("MACJackCard::setValue: Failed to set value (0x%x != 0x%x)\n",
                 value, v);
        return kIOReturnError;
    }

    return kIOReturnSuccess;
}

IOReturn MACJackCard::getString(UInt16 rid, char* str, size_t* n)
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

IOReturn MACJackCard::setString(UInt16 rid, char* str, size_t n)
{
    if (n > 32) return kIOReturnError;
 
    char* s = new char[n+3];
    s[0] = (n) & 0xFF;
    s[1] = (n >> 8) & 0xFF;
    
    memcpy(&s[2], str, n);
    
    IOReturn ret = setRecord(rid, s, n+3, false);
    delete [] s;
    
    return ret;
}

/*
 * Read a record, specified by rid, into memory of size n bytes
 * pointed to by buf.  The number of bytes read is stored in n before
 * returning.  If swapBytes is true, the data is assumed to be
 * little-endian and is swapped.
 */
IOReturn
MACJackCard::getRecord(UInt16 rid, void* buf, size_t* n, bool swapBytes)
{
    if (_doCommand(wlcAccessRead, rid) != kIOReturnSuccess) {
	WLLogErr("MACJackCard::getRecord: Read command failed\n");
	return kIOReturnError;
    }
    
    setRegister(wlSelect0, rid);
    setRegister(wlOffset0, 0);

    if (_waitForNotBusy(wlOffset0) != kIOReturnSuccess) {
	setRegister(wlSelect0, rid);
        setRegister(wlOffset0, 0);
        if (_waitForNotBusy(wlOffset0) != kIOReturnSuccess) {
            WLLogWarn("MACJackCard::getRecord: _waitForNotBusy error\n");
            return kIOReturnError;
        }
    }

    WLOffset off;
    *((UInt16*)&off) = getRegister(wlOffset0);
    if (off.error) {
	WLLogErr("MACJackCard::getRecord: Error\n");
	return kIOReturnError;
    }

    /*
     * Read and check the length
     */
    UInt16 length = getRegister(wlData0);
    if (length > *n / 2 + 1) {
	WLLogCrit("MACJackCard::getRecord: Length too large (%d > %ld)\n",
                  (length - 1) * 2, *n);
	return kIOReturnError;
    }

    /*
     * Read and check type
     */
    UInt16 type = getRegister(wlData0);
    if (type != rid) {
	WLLogCrit("MACJackCard::getRecord: Type != RID (0x%x != 0x%x)\n",
                  type, rid);
	return kIOReturnError;
    }
    
    UInt16* ui16Buf = (UInt16*)buf;
    for (int i = 0; i < (length - 1); i++) {
	if (swapBytes)
	    ui16Buf[i] = getRegister(wlData0);
	else
            ui16Buf[i] = getRegister(wlData0, false); //ui16Buf[i] = *(UInt16*)((UInt16*)_ioBase + (unsigned long)wlData0);
    }

    *n = (length - 1) * 2;
    
    return kIOReturnSuccess;
}

IOReturn
MACJackCard::setRecord(UInt16 rid, const void* buf, size_t n, bool swapBytes)
{
    setRegister(wlSelect0, rid);
    setRegister(wlOffset0, 0);

    if (_waitForNotBusy(wlOffset0) != kIOReturnSuccess) {
	WLLogErr("MACJackCard::getRecord: _waitForNotBusy error\n");
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
	WLLogErr("MACJackCard::setRecord: Write command failed\n");
	return kIOReturnError;
    }
    
    return kIOReturnSuccess;
}

#pragma mark -

IOReturn MACJackCard::_enable()
{
    if (_doCommand(wlcEnable, 0) != kIOReturnSuccess) {
        WLLogErr("MACJackCard::startCapture: _doCommand(wlcEnable) failed\n");
        return kIOReturnError;
    }
    _isEnabled = true;

    enableInterrupts();

    return kIOReturnSuccess;
}

IOReturn MACJackCard::_disable()
{
    disableInterrupts();
    ackAllInterrupts();
    
    if (_doCommand(wlcDisable, 0) != kIOReturnSuccess) {
        WLLogErr("MACJackCard::_disable: _doCommand(wlcDisable) failed\n");
        return kIOReturnError;
    }
    _isEnabled = false;

    return kIOReturnSuccess;
}

void MACJackCard::_myTimeoutHandler(OSObject *owner, IOTimerEventSource *sender) {
    MACJackDriver *me;
    MACJackCard *card;
    int backoff=0;

    WLLogInfo("MACJackCard::_myTimeoutHandler()\n");

    // Make sure that the owner of the timer is us.
    me = OSDynamicCast(MACJackDriver, owner);
    if (!me) return;
 
    card = me->getWLCard();
    
    if (!card) {
        WLLogErr("MACJackCard::_myTimeoutHandler: WARNING could not get WLCard reference.\n");
        return;
    };
    
    if (card->_failures>25) {
        WLLogErr("MACJackCard::_myTimeoutHandler: WARNING timed sendFrame terminated because of general failure.\n");
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

IOReturn MACJackCard::
_reset()
{
    int i;
    
    WLLogDebug("MACJackDriver::start: Intitialize the card\n");
    if (_doCommand(wlcInit, 0) != kIOReturnSuccess) {
        WLLogErr("MACJackCard::_reset: _doCommand(wlcInit, 0) failed\n");
        return kIOReturnError;
    }

    if (_firmwareType == -1) _firmwareType = _getFirmwareType();
    
    for (i = 0; i < wlTimeout; i++) {
        WLLogDebug("MACJackDriver::start: trying to set the port type for the %u time.\n", i);
        if (setValue(0xFC00, (_firmwareType == WI_LUCENT) ? 0x3 : 0x5) == kIOReturnSuccess) break;
    }

//#ifdef USEHOSTAP
    if (_firmwareType == WI_INTERSIL) {
        //setString(0xFC02,"", 0);  //desired SSID
        //setString(0xFC04,"x", 1); //own SSID
        //setValue(0xFC03, 3); //channel
        
        setValue(0xFC06, 1); //syscale
        setValue(0xFC07, 2304); //max data len
        setValue(0xFC09, 0); //pm off!
        setValue(0xFC84, 3); //default tx rate
        setValue(0xFC85, 0); //promiscous mode
        setValue(0xFC2A, 1); //auth type
        setValue(0xFC2D, 1); //roaming by firmware
        //setValue(0xFC81,1); //create ibss
		
		//UInt16 key[] = { 0,0,0 };
		//setRecord(0xFC24, &key, 5);
        setValue(0xFC28, 0x90); //set wep ignore
    }
	
//#endif
    
    if (i==wlTimeout) {
        WLLogErr("MACJackCard::_reset: could not set port type\n");
        return kIOReturnError;
    }
    
    WLLogDebug("MACJackDriver::start: Disable the interrupts of the card\n");
    disableInterrupts();
    WLLogDebug("MACJackDriver::start: Ack the interrupts of the card\n");
    ackAllInterrupts();

    /*
     * Write and check a magic number to the Software0 register
     */
    
    WLLogDebug("MACJackDriver::start: Doing magic check\n");
    UInt16 magic = 0x1ee7;
    setRegister(wlSwSupport0, magic);
    if (getRegister(wlSwSupport0) != 0x1ee7) {
        WLLogCrit("MACJackCard::_reset: Magic check failed\n");
        return kIOReturnError;
    }   
    
    /*
     * Set list of interesting events
     */
    _interrupts = wleRx;  //| wleTx | wleTxExc | wleAlloc | wleInfo | wleInfDrop | wleCmd | wleWTErr | wleTick;

    WLLogDebug("MACJackDriver::start: Enabling the card\n");
    _enable();
    
    _isSending = false;
    
    return kIOReturnSuccess;
}

IOReturn MACJackCard::
_doCommand(WLCommandCode cmd, UInt16 param0, UInt16 param1, UInt16 param2)
{
    /*
     * Wait for busy bit to clear on command register
     */
    if (_waitForNotBusy(wlCommand) != kIOReturnSuccess)
	WLLogDebug("MACJackCard::_doCommand: _waitForNotBusy error\n");

    /*
     * Write parameters and command code to registers
     */
    setRegister(wlParam0, param0);
    setRegister(wlParam1, param1);
    setRegister(wlParam2, param2);
    setRegister(wlCommand, cmd);

    if (_waitForNotBusy(wlCommand) != kIOReturnSuccess)
	WLLogErr("MACJackCard::_doCommand: _waitForNotBusy error\n");

    /*
     * When the command is complete, the wlCmd bit will be set in the
     * wlEvStat register.  Polling it for it here is easier than
     * waiting for the interrupt.  Once the command is complete, we
     * can read the result code out of the wlStatus register.
     */
    if (_waitForEvent(wleCmd) != kIOReturnSuccess) {
	WLLogErr("MACJackCard::_doCommand: Command 0x%x timed out\n", cmd);
	return kIOReturnError;
    }
    
    UInt16 status;
    *((UInt16*)&status) = getRegister(wlStatus);

    setRegister(wlEvAck, wleCmd);
    
   if ((status & 0x3F) != (cmd & 0x3F)) {
        WLLogCrit("MACJackCard::_doCommand: Status code (0x%x) != cmd (0x%x)\n",
                  status & 0x3F, cmd & 0x3F);
        return kIOReturnError;
    }

    if (status & 0x7F00) {
	WLLogErr("MACJackCard::_doCommand: Command result 0x%x\n",
                 status & 0x7F00);
        return kIOReturnError;
    }

    return kIOReturnSuccess;
}

/*********************************************************************
 *                          Event Handling                           *
 *********************************************************************/
 
/*
 * Wait for busy bit to clear on register wlCommand or wlOffset
 */
IOReturn MACJackCard::
_waitForNotBusy(WLRegister reg)
{
    int i;

    if (!(reg == wlCommand || reg == wlOffset0 || reg == wlOffset1)) {
        WLLogCrit("MACJackCard::_waitForNotBusy: Register has no busy bit!\n");
        return kIOReturnError;
    }
    
    for (i = 0; i < wlTimeout; i++) {
	UInt16 val = getRegister(reg);
        if (val & 0x4000)
            return kIOReturnError;
	if ((val & 0x8000) == 0)
	    break;
        IODelay(5);
    }
    
    if (i == wlTimeout) {
	WLLogInfo("MACJackCard::_waitForNotBusy: Timeout\n");
	return kIOReturnTimeout;
    }
    
    return kIOReturnSuccess;
}

/*
 * Wait for a specific event to occur
 */
IOReturn MACJackCard::
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
        WLLogWarn("MACJackCard::_waitForEvent: timeout\n");
	return kIOReturnError;
    }
    
    return kIOReturnSuccess;
}

int MACJackCard::
_getFirmwareType() {
    UInt16 card_id;
    size_t size = 8;
    const struct wi_card_ident *idx;
    UInt8 d[8];
    
    if (getRecord(0xFD0B, (UInt16*)d, &size) != kIOReturnSuccess) {
        WLLogErr("MACJackCard::getIdentity: getRecord failed\n");
        return kIOReturnError;
    }

    card_id = *(UInt16*)d;
    for ((idx = wi_card_ident); (idx->firm_type != WI_NOTYPE); idx++)
            if (card_id == idx->card_id) break;
    
    WLLogInfo("MACJackCard::_getFirmwareType(): got: %x %x %x %x %x %x %x %x card id is: %x\n",d[0], d[1], d[2], d[3], d[4], d[5], d[6], d[7], card_id);
    
    if (idx->firm_type != WI_NOTYPE) {
        WLLogNotice("MACJackCard::_getFirmwareType(): detected a known card: %s (%u)\n",idx->card_name, idx->firm_type);
        return idx->firm_type;
    } else if (card_id & 0x8000) {
        WLLogWarn("MACJackCard::_getFirmwareType(): WARNING detected UNKNOWN Prism2 card\n");
        return WI_INTERSIL;
    } else {
        WLLogWarn("MACJackCard::_getFirmwareType(): WARNING detected UNKNOWN Lucent card\n");
        return WI_LUCENT;
    }
}

//#define WI_HERMES_AUTOINC_WAR

IOReturn MACJackCard::
_sendFrame(UInt8* data, IOByteCount size)
{
    int id;
	UInt64 t;

    if (size%2) {
        data[size]=0;
        size++;
    }
    
    //if (_isSending) {
#ifndef MAC_OS_X_VERSION_10_4
    clock_get_uptime((AbsoluteTime*)&t);
#else
    clock_get_uptime((UInt64*)&t);
#endif
    if ((t-100000)<_lastSending) {
        return kIOReturnBusy;
    }
    //}
    
    WLLogInfo("MACJackCard::_sendFrame: %u\n", (int)size);
    
    if (_doCommand(wlcAllocMem, size)  != kIOReturnSuccess) {
        WLLogCrit("MACJackCard::_sendFrame: failed to allocate %u bytes on NIC. Resetting...\n", (int)size);
        _reset();
        startCapture(_packetQueue, _channel);
        return kIOReturnError;
    }

    if (_waitForEvent(wleAlloc) != kIOReturnSuccess) {
        WLLogCrit("MACJackCard::_sendFrame: timeout failure\n");
        return kIOReturnError;
    }
    
    setRegister(wlEvAck, wleAlloc);    
    id = getRegister(wlAllocFID);
    WLLogInfo("MACJackCard::_sendFrame: FID is 0x%x\n", id);
    
#ifdef WI_HERMES_AUTOINC_WAR
again:
#endif
    setRegister(wlSelect0, id);
    setRegister(wlOffset0, 0);

    if (_waitForNotBusy(wlOffset0) != kIOReturnSuccess) {
	WLLogErr("MACJackCard::_sendFrame: _waitForNotBusy error\n");
	return kIOReturnError;
    }
    
    for (UInt32 i = 0; i < size; i+=2)
        setRegister(wlData0, *((UInt16*)&data[i]), false); //*(UInt16*)((UInt16*)_ioBase + (unsigned long)(wlData0>>1)) = *((UInt16*)&data[i]);
    
#ifdef WI_HERMES_AUTOINC_WAR

    setRegister(wlSelect0, id);
    setRegister(wlOffset0, 0);

    if (_waitForNotBusy(wlOffset0) != kIOReturnSuccess) {
	WLLogErr("MACJackCard::_sendFrame: _waitForNotBusy error\n");
	return kIOReturnError;
    }
    
    for (UInt32 i = 0;i<size;i+=2) 
        if (getRegister(wlData0, false) != *((UInt16*)&data[i]))
            goto again;
    
#endif

    if (_doCommand(wlcTransmit, id)  != kIOReturnSuccess) {
        WLLogCrit("MACJackCard::_sendFrame: could not transmit packet\n");
        return kIOReturnError;
    }

#ifndef MAC_OS_X_VERSION_10_4
    clock_get_uptime((AbsoluteTime*)&_lastSending);
#else
    clock_get_uptime((UInt64*)&_lastSending);
#endif
    //_isSending = true;
    return kIOReturnSuccess;
}

void MACJackCard::
enableInterrupts()
{
    setRegister(wlIntEn, _interrupts);
}

void MACJackCard::
disableInterrupts()
{
    setRegister(wlIntEn, 0);
}

void MACJackCard::
ackAllInterrupts() {
    /*
     * Acknowledge all interrupts by writing a "spam ack"
     */
    setRegister(wlEvAck, 0xFFFF);
}

IOReturn MACJackCard::
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

IOReturn MACJackCard::
_handleRx() {
    UInt16 packet[1182];    /* 60 byte header + 2304 data bytes */
    WLFrame* frameDescriptor = (WLFrame*)packet;
    UInt16* frameData = &packet[30];
    
    UInt16 FID = getRegister(wlRxFID);
    
    setRegister(wlSelect0, FID);
    setRegister(wlOffset0, 0);
    
    if (_waitForNotBusy(wlOffset0) != kIOReturnSuccess) {
	WLLogWarn("MACJackCard::_handleRx: Timeout or error\n");
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
        WLLogWarn("MACJackCard::_handleRx: Oversized packet (%d bytes)\n",
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
            WLLogCrit("MACJackCard::_handleRx: packet queue overflow\n");
    }
    
    return kIOReturnSuccess;
}

IOReturn MACJackCard::
_handleTx() {
    _isSending = false;
    
    WLLogErr("MACJackCard::_handleTx: packet sent\n");
    return kIOReturnSuccess;
}

IOReturn MACJackCard::
_handleTxExc() {
    _isSending = false;
    
    WLLogErr("MACJackCard::_handleTxExc: packet sent\n");
    return kIOReturnSuccess;
}

IOReturn MACJackCard::
_handleAlloc() {

    WLLogInfo("MACJackCard::_handleAlloc called\n");
    return kIOReturnSuccess;
}

IOReturn MACJackCard::
_handleCmd() {

    WLLogInfo("MACJackCard::_handleCmd called\n");
    return kIOReturnSuccess;
}

IOReturn MACJackCard::
_handleInfo() {

    WLLogInfo("MACJackCard::_handleInfo called\n");
    return kIOReturnSuccess;
}

IOReturn MACJackCard::
_handleInfDrop() {
    WLLogErr("MACJackCard::_handleInfDrop called\n");
    return kIOReturnSuccess;
}

IOReturn MACJackCard::
_handleWTErr() {
    WLLogErr("MACJackCard::_handleWTErr called\n");
    return kIOReturnSuccess;
}

IOReturn MACJackCard::
_handleTick() {
    WLLogInfo("MACJackCard::_handleTick called\n");
    return kIOReturnSuccess;
}

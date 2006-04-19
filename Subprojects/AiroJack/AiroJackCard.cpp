/*
        
        File:			AiroJackCard.cpp
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

#include <IOKit/IOWorkLoop.h>
#include <IOKit/IOLib.h>
#include <libkern/OSByteOrder.h>
#include "AiroJackCard.h"
#include "AiroJackLog.h"
#include "AiroJackDriver.h"

#define DEBUG

static char *get_rmode(u16 mode) {
        switch(mode&0xff) {
        case RXMODE_RFMON:  return "rfmon";
        case RXMODE_RFMON_ANYBSS:  return "yna (any) bss rfmon";
        case RXMODE_LANMON:  return "lanmon";
        }
        return "ESS";
}


void AiroJackCard::
_printDebugInfo() {
    StatusRid stat;
    CapabilityRid cap;

    _readConfig();
    WLLogDebug(	    "Mode: %s\n"
                    "Radio: %s\n"
                    "NodeName: %-16s\n"
                    "PowerMode: %s\n"
                    "DataRates: %d %d %d %d %d %d %d %d\n"
                    "Channel: %d\n"
                    "XmitPower: %d\n"
                    "ScanMode: %d\n",
                    (_cfg.opmode & 0xFF) == 0 ? "adhoc" :
                    (_cfg.opmode & 0xFF) == 1 ? get_rmode(_cfg.rmode):
                    (_cfg.opmode & 0xFF) == 2 ? "AP" :
                    (_cfg.opmode & 0xFF) == 3 ? "AP RPTR" : "Error",
                    _isEnabled ? "on" : "off",
                    _cfg.nodeName,
                    _cfg.powerSaveMode == 0 ? "CAM" :
                    _cfg.powerSaveMode == 1 ? "PSP" :
                    _cfg.powerSaveMode == 2 ? "PSPCAM" : "Error",
                    (int)_cfg.rates[0],
                    (int)_cfg.rates[1],
                    (int)_cfg.rates[2],
                    (int)_cfg.rates[3],
                    (int)_cfg.rates[4],
                    (int)_cfg.rates[5],
                    (int)_cfg.rates[6],
                    (int)_cfg.rates[7],
                    (int)_cfg.channelSet,
                    (int)_cfg.txPower,
                    (int)_cfg.scanMode
            );

    WLLogDebug( "LongRetryLimit: %d\n"
                "ShortRetryLimit: %d\n"
                "RTSThreshold: %d\n"
                "TXMSDULifetime: %d\n"
                "RXMSDULifetime: %d\n"
                "TXDiversity: %s\n"
                "RXDiversity: %s\n"
                "FragThreshold: %d\n"
                "WEP: %s\n"
                "Modulation: %s\n"
                "Preamble: %s\n",
                (int)_cfg.longRetryLimit,
                (int)_cfg.shortRetryLimit,
                (int)_cfg.rtsThres,
                (int)_cfg.txLifetime,
                (int)_cfg.rxLifetime,
                _cfg.txDiversity == 1 ? "left" :
                _cfg.txDiversity == 2 ? "right" : "both",
                _cfg.rxDiversity == 1 ? "left" :
                _cfg.rxDiversity == 2 ? "right" : "both",
                (int)_cfg.fragThresh,
                _cfg.authType == AUTH_ENCRYPT ? "encrypt" :
                _cfg.authType == AUTH_SHAREDKEY ? "shared" : "open",
                _cfg.modulation == MOD_DEFAULT ? "default" :
                _cfg.modulation == MOD_CCK ? "cck" :
                _cfg.modulation == MOD_MOK ? "mok" : "error",
                _cfg.preamble == PREAMBLE_AUTO ? "auto" :
                _cfg.preamble == PREAMBLE_LONG ? "long" :
                _cfg.preamble == PREAMBLE_SHORT ? "short" : "error"
            );

    _readStatus(&stat);
    _readCapability(&cap);
    WLLogDebug("Status: %s%s%s%s%s%s%s%s%s\n",
                    stat.mode & 1 ? "CFG ": "",
                    stat.mode & 2 ? "ACT ": "",
                    stat.mode & 0x10 ? "SYN ": "",
                    stat.mode & 0x20 ? "LNK ": "",
                    stat.mode & 0x40 ? "LEAP ": "",
                    stat.mode & 0x80 ? "PRIV ": "",
                    stat.mode & 0x100 ? "KEY ": "",
                    stat.mode & 0x200 ? "WEP ": "",
                    stat.mode & 0x8000 ? "ERR ": "");
    WLLogDebug(  "Mode: 0x%x\n"
		 "Signal Strength: %d\n"
		 "Signal Quality: %d\n"
		 "SSID: %-.*s\n"
		 "AP: %-.16s\n"
		 "Freq: %d\n"
                 "SetFreq: %d\n"
                 "Association: %d\n"
		 "BitRate: %dmbs\n" 
                 "Device: %s\nManufacturer: %s\nFirmware Version: %s\n"
		 "Radio type: %x\nCountry: %x\nHardware Version: %x\n"
		 "Software Version: %x\nSoftware Subversion: %x\n"
		 "Boot block version: %x\n"
                 "TxPowerLevels: %d %d %d %d %d %d %d %d\n",
		 (int)stat.mode,
		 (int)stat.normalizedSignalStrength,
		 (int)stat.signalQuality,
		 (int)stat.SSIDlen,
		 stat.SSID,
		 stat.apName,
		 (int)stat.channel,
		 (int)stat.channelSet,
                 (int)stat.assocStatus,
		 (int)stat.currentXmitRate/2,
                 cap.prodName,
		 cap.manName,
		 cap.prodVer,
		 cap.radioType,
		 cap.country,
		 cap.hardVer,
		 (int)cap.softVer,
		 (int)cap.softSubVer,
		 (int)cap.bootBlockVer,
                 (int)cap.txPowerLevels[0],
                 (int)cap.txPowerLevels[1],
                 (int)cap.txPowerLevels[2],
                 (int)cap.txPowerLevels[3],
                 (int)cap.txPowerLevels[4],
                 (int)cap.txPowerLevels[5],
                 (int)cap.txPowerLevels[6],
                 (int)cap.txPowerLevels[7]);
    
    WLLogErr("AiroJackCard: MAC 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x\n",
                _cfg.macAddr[0], _cfg.macAddr[1], _cfg.macAddr[2],
                _cfg.macAddr[3], _cfg.macAddr[4], _cfg.macAddr[5]);

}

        	
AiroJackCard::
AiroJackCard(void* ioBase, void* klBase, IOService* parent) : _ioBase(ioBase),
        			     _interrupts(0),
                                     _cardPresent(1),
                                     _isEnabled(false),
                                     _parent(parent)
{    
    _reset();
    //_enable();
    
    _printDebugInfo();
    
    _workLoop = _parent->getWorkLoop();
    if (!_workLoop) {
        WLLogCrit("AiroJackCard::AiroJackCard: Failed to create workloop.\n");
        return;
    }
    
    _timedSendSource = IOTimerEventSource::timerEventSource(_parent, &AiroJackCard::_myTimeoutHandler);
    if (!_timedSendSource) {
        WLLogCrit("AiroJackCard::AiroJackCard: Failed to create timer event source.\n");
        return;
    }

    if (_workLoop->addEventSource(_timedSendSource) != kIOReturnSuccess) {
        WLLogCrit("AiroJackCard::AiroJackCard: Failed to register timer event source.\n");
        return;
    }
}

AiroJackCard::
~AiroJackCard()
{
    if (_timedSendSource) {
        _timedSendSource->cancelTimeout();
        if (_workLoop) _workLoop->removeEventSource(_timedSendSource);
	_timedSendSource->release();
    }
    if (_cardPresent && _isEnabled) _disable();
}

#pragma mark -

IOReturn AiroJackCard::startCapture(IODataQueue* dq, UInt16 channel) {
    _packetQueue = dq;
    _channel = channel;
    
    if (_reset() != kIOReturnSuccess) {
        WLLogErr("AiroJackCard::startCapture: Couldn't disable card\n");
        return kIOReturnError;
    }
    
    if (_enable() != kIOReturnSuccess) {
        WLLogErr("AiroJackCard::startCapture: Couldn't enable card\n");
        return kIOReturnError;
    }
    
    return kIOReturnSuccess;
}

IOReturn AiroJackCard::stopCapture() {
    return _disable();
}

IOReturn AiroJackCard::sendFrame(UInt8* data, UInt32 repeat) {
    WLFrame *frameDescriptor;
    AiroFrame *airo;
    UInt8 aData[2364];
    IOByteCount pktsize;
    
    WLLogInfo("AiroJackCard::sendFrame()\n");

    memcpy(aData, data, sizeof(WLFrame));
    memset(aData, 0, sizeof(AiroFrame));
    frameDescriptor = (WLFrame*)data;
    airo = (AiroFrame*) aData;
    switch(frameDescriptor->frameControl & 0x0c) {
        case 0x08:
            WLLogInfo("AiroJackCard:: send Data Packet\n");
        case 0x00:
            WLLogInfo("AiroJackCard:: send Management Packet\n");
            pktsize = frameDescriptor->dataLen;
            WLLogInfo("AiroJackCard:: data len: %u\n", (int)pktsize);
            if ((pktsize + sizeof(WLFrame)) > 2364) return kIOReturnBadArgument;
            airo->len = OSSwapHostToLittleInt16(frameDescriptor->dataLen);
            break;
        case 0x04:
            WLLogInfo("AiroJackCard:: send Control Packet\n");
            pktsize = 0;
            airo->len = 0;
            break;
        default:
            WLLogErr("WARNING! AiroJackCard: sendFrame: Unknown Packettype: 0x%x\n", frameDescriptor->frameControl);
            return kIOReturnBadArgument;
    }
    airo->channel = _channel;
    airo->status = OSSwapHostToLittleInt16(TXCTL_TXOK | TXCTL_TXEX | TXCTL_802_11
			| TXCTL_ETHERNET);
    
    memcpy(aData + 0x3C, data + sizeof(WLFrame), pktsize);

    if (_sendFrame(aData, pktsize + 0x3C) != kIOReturnSuccess) {
        WLLogCrit("AiroJackCard::sendFrame() transmittion failed\n");
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

    WLLogInfo("AiroJackCard::sendFrame(): sendFrame returning with success.\n");
    
    return kIOReturnSuccess;
}

IOReturn AiroJackCard::stopSendingFrames() {
    if (_timedSendSource) _timedSendSource->cancelTimeout();
    _failures = 99;
    return kIOReturnSuccess;
}

IOReturn AiroJackCard::cardGone() {
    _cardPresent = 0;
    return stopSendingFrames();
}

#pragma mark -

/*
 * Read a record, specified by rid, into memory of size n bytes
 * pointed to by buf.  The number of bytes read is stored in n before
 * returning.  If swapBytes is true, the data is assumed to be
 * little-endian and is swapped.
 */
IOReturn
AiroJackCard::getRecord(UInt16 rid, void* buf, size_t n, bool swapBytes)
{
    WLLogDebug("AiroJackCard::getRecord:  Rid %x has a length of %d\n", (int)rid, (int)n);
    if (n < 2) return kIOReturnError;

    if (_doCommand(wlcAccessRead, rid) != kIOReturnSuccess) {
	WLLogErr("AiroJackCard::getRecord: Read command failed\n");
	return kIOReturnError;
    }
    
    if (_bapsetup(rid, wlbBAP1, 0) != kIOReturnSuccess) {
    	WLLogErr("AiroJackCard::getRecord: could not seek to record!\n");
	return kIOReturnError;
    }

    /* Read and check the length */
    ((UInt16*)buf)[0] = getRegister(wlData1);
    WLLogDebug("AiroJackCard::getRecord: size of Record is %d\n", (int)((UInt16*)buf)[0]);
    
    // length for remaining part of rid
    int len = min(n, ((UInt16*)buf)[0]) - 2;

    if (len <= 2) {
        WLLogErr("AiroJackCard::getRecord:  Rid %x has a length of %d which is too short\n",
			(int)rid,
			(int)len);
	return kIOReturnError;
    }

    // read remainder of the rid    
    for (int i = 2; i <= (len); i+=2) {
        ((UInt16*)buf)[i/2] = getRegister(wlData1, swapBytes);
    }

    return kIOReturnSuccess;
}

IOReturn
AiroJackCard::setRecord(UInt16 rid, const void* buf, size_t n, bool swapBytes)
{
    WLLogDebug("AiroJackCard::setRecord:  Rid %x has a length of %d\n", (int)rid, (int)n);
    if (n < 2) return kIOReturnError;

    if (_doCommand(wlcAccessRead, rid) != kIOReturnSuccess) {
	WLLogErr("AiroJackCard::setRecord: Read command failed\n");
	return kIOReturnError;
    }
    
    if (_bapsetup(rid, wlbBAP1, 0) != kIOReturnSuccess) {
    	WLLogErr("AiroJackCard::setRecord: could not seek to record!\n");
	return kIOReturnError;
    }

    UInt16* ui16buf = (UInt16*)buf;
    for (unsigned int i = 0; i < (n / 2); i++)
	    setRegister(wlData1, ui16buf[i], swapBytes);

    if (_doCommand(wlcAccessWrite, rid) != kIOReturnSuccess) {
	WLLogErr("AiroJackCard::setRecord: Write command failed\n");
	return kIOReturnError;
    }
    
    return kIOReturnSuccess;
}

#pragma mark -

IOReturn AiroJackCard::_enable()
{
    if (_doCommand(wlcEnable, 0) != kIOReturnSuccess) {
        WLLogErr("AiroJackCard::_enable: _doCommand(wlcEnable) failed\n");
        return kIOReturnError;
    }
    
    _isEnabled = true;

    enableInterrupts();

    return kIOReturnSuccess;
}

IOReturn AiroJackCard::_disable()
{
    disableInterrupts();
    ackAllInterrupts();
    
    if (_doCommand(wlcDisable, 0) != kIOReturnSuccess) {
        WLLogErr("AiroJackCard::_disable: _doCommand(wlcDisable) failed\n");
        return kIOReturnError;
    }
    _isEnabled = false;

    return kIOReturnSuccess;
}

void AiroJackCard::_myTimeoutHandler(OSObject *owner, IOTimerEventSource *sender) {
    AiroJackDriver *me;
    AiroJackCard *card;
    int backoff=0;

    WLLogInfo("AiroJackCard::_myTimeoutHandler()\n");

    // Make sure that the owner of the timer is us.
    me = OSDynamicCast(AiroJackDriver, owner);
    if (!me) return;
 
    card = me->getWLCard();
    
    if (!card) {
        WLLogErr("AiroJackCard::_myTimeoutHandler: WARNING could not get WLCard reference.\n");
        return;
    };
    
    if (card->_failures>25) {
        WLLogErr("AiroJackCard::_myTimeoutHandler: WARNING timed sendFrame terminated because of general failure.\n");
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

IOReturn AiroJackCard::
_reset()
{
    int i = 10;
    SsidRid mySsid;
    
    WLLogDebug("AiroJackDriver::_reset: Intitialize the card\n");

    _doCommand(wlcDisable, 0);
    setRegister(wlCommand, wlcSoftReset);
    int delay = 0;
    while ((getRegister (wlCommand) & 0x8000) & (delay < 10000)) {
        IODelay (10);
        if (++delay % 20)
            setRegister(wlEvAck, 0x4000);
    }
    IOSleep(100);
    setRegister(wlCommand, wlcNop2);
    IOSleep(100);
    
    /*
     * Write and check a magic number to the Software0 register
     */
    
    WLLogDebug("AiroJackDriver::_reset: Doing magic check\n");
    UInt16 magic = 0x1ee7;
    setRegister(wlSwSupport0, magic);
    if (getRegister(wlSwSupport0) != 0x1ee7) {
        WLLogCrit("AiroJackCard::_reset: Magic check failed\n");
        return kIOReturnError;
    }   

    //_disable();
    
    memset(&_cfg,0,sizeof(_cfg));
    while ( (_readConfig() != kIOReturnSuccess) && ((i--) > 0) ) IOSleep(100);
    
    if (i == 0) return kIOReturnError;
    
    _cfg.opmode = wlomESS;
    _cfg.rmode  =  RXMODE_RFMON_ANYBSS | RXMODE_DISABLE_802_3_HEADER;
    _cfg.scanMode = SCANMODE_PASSIVE; 
    _writeConfig();
    
    memset(&mySsid, 0, sizeof(mySsid));
    _writeSSID(&mySsid);
    
    _interrupts = wleRx | wleLink | wleAwake;
    
    if (_isEnabled) _enable();
            
    return kIOReturnSuccess;
}

IOReturn AiroJackCard::
_doCommand(WLCommandCode cmd, UInt16 param0, UInt16 param1, UInt16 param2, UInt16* ret)
{
    /*
     * Write parameters and command code to registers
     */
    setRegister(wlParam0, param0);
    setRegister(wlParam1, param1);
    setRegister(wlParam2, param2);
    setRegister(wlCommand, cmd);

    if (_waitForNotBusy(wlCommand, cmd) != kIOReturnSuccess)
	WLLogErr("AiroCardCard::_doCommand: _waitForNotBusy error\n");

    /*
     * When the command is complete, the wlCmd bit will be set in the
     * wlEvStat register.  Polling it for it here is easier than
     * waiting for the interrupt.  Once the command is complete, we
     * can read the result code out of the wlStatus register.
     */
    if (_waitForEvent(wleCmd) != kIOReturnSuccess) {
	WLLogErr("AiroJackCard::_doCommand: Command 0x%x timed out\n", cmd);
	return kIOReturnError;
    }
    
    UInt16 status;
    status =  getRegister(wlStatus);
    
    if (getRegister(wlCommand) & 0x8000)
        setRegister(wlEvAck, wleClearBusy);
    
    // acknowledge processing the status/response        
    setRegister(wlEvAck, wleCmd);
    
    if ((cmd == wlcAccessRead) && ((status & 0x7F00) != 0))  {
	WLLogErr("AiroCardCard::_doCommand: Command result 0x%x\n",
                 status & 0x7F00);
        return kIOReturnError;
    }
    
    if (ret) *ret = status;
    
    return kIOReturnSuccess;
}

/*********************************************************************
 *                          Event Handling                           *
 *********************************************************************/
 
/*
 * Wait for busy bit to clear on register wlCommand or wlOffset
 */
IOReturn AiroJackCard::
_waitForNotBusy(WLRegister reg, UInt16 cmd)
{
    int i;

    for (i = 0; i < wlTimeout; i++) {
	UInt16 val = getRegister(reg);
        if (val == cmd) setRegister(reg, cmd);
        else if (val & 0x4000)
            return kIOReturnError;
	else if ((val & 0x8000) == 0)
	    break;
        IODelay(5);
    }
    
    if (i == wlTimeout) {
	WLLogWarn("AiroCardCard::_waitForNotBusy: Timeout\n");
	return kIOReturnTimeout;
    }
    
    return kIOReturnSuccess;
}

/*
 * Wait for a specific event to occur
 */
IOReturn AiroJackCard::
_waitForEvent(WLEvent e)
{
    int i = wlTimeout;

    while (i-- && (getRegister(wlEvStat) & wleCmd) == 0);
    
    if (i == -1) {
        WLLogWarn("AiroJackCard::_waitForEvent: timeout\n");
	return kIOReturnError;
    }
    
    return kIOReturnSuccess;
}

IOReturn AiroJackCard::
_bapsetup(UInt16 rid, WLBAP bap, int offset) {
    int timeout = 50;
    int max_tries = 3;

    setRegister(wlSelect1, rid);
    setRegister(wlOffset1, offset);

    while (1) {
        int status = getRegister(wlOffset1);
        if (status & wlsBusy) {
            /* This isn't really a timeout, but its kinda
                close */
            if (timeout--) continue;
        } else if ( status & wlsError) {
            /* invalid rid or offset */
            WLLogErr("AiroJackCard::getRecord: BAP error %x %d\n", status, bap );
            return kIOReturnError;
        } else if (status & wlsDone) { // success
            break;
        }
	
        if (!(max_tries--)) {
            WLLogErr("AiroJackCard::getRecord: BAP setup error too many retries\n" );
            return kIOReturnError;
        }
        
        // -- PC4500 missed it, try again
        setRegister(wlSelect1, rid);
        setRegister(wlOffset1, offset);
        timeout = 50;
    }
    
    return kIOReturnSuccess;
}

IOReturn AiroJackCard::
_readStats(StatsRid *sr) {
	IOReturn rc = getRecord(0xFF68, sr, sizeof(*sr), false);
	UInt32 *i;

	sr->len = OSSwapLittleToHostInt16(sr->len);
	for(i = &sr->vals[0]; i <= &sr->vals[99]; i++) *i = OSSwapLittleToHostInt32(*i);
	return rc;
}

IOReturn AiroJackCard::
_readStatus(StatusRid *statr) {
	IOReturn rc = getRecord(0xFF50, statr, sizeof(*statr), false);
	UInt16 *s;

	statr->len = OSSwapLittleToHostInt16(statr->len);
	for(s = &statr->mode; s <= &statr->SSIDlen; s++) *s = OSSwapLittleToHostInt16(*s);

	for(s = &statr->beaconPeriod; s <= &statr->shortPreamble; s++)
		*s = OSSwapLittleToHostInt16(*s);
	statr->load = OSSwapLittleToHostInt16(statr->load);
	statr->assocStatus = OSSwapLittleToHostInt16(statr->assocStatus);
	return rc;
}

IOReturn AiroJackCard::
_readCapability(CapabilityRid *capr) {
	IOReturn rc = getRecord(0xFF00, capr, sizeof(*capr), false);
	UInt16 *s;

	capr->len = OSSwapLittleToHostInt16(capr->len);
	capr->prodNum = OSSwapLittleToHostInt16(capr->prodNum);
	capr->radioType = OSSwapLittleToHostInt16(capr->radioType);
	capr->country = OSSwapLittleToHostInt16(capr->country);
	for(s = &capr->txPowerLevels[0]; s <= &capr->requiredHard; s++)
		*s = OSSwapLittleToHostInt16(*s);
	return rc;
}

IOReturn AiroJackCard::
_readSSID(SsidRid *ssidr) {
	IOReturn rc = getRecord(0xFF11, ssidr, sizeof(*ssidr), false);

	ssidr->len = OSSwapLittleToHostInt16(ssidr->len);
	for(int i = 0; i < 3; i++)
		ssidr->ssids[i].len = OSSwapLittleToHostInt16(ssidr->ssids[i].len);
        
	return rc;
}

IOReturn AiroJackCard::
_writeSSID(SsidRid *pssidr) {
	int rc;
	int i;
	SsidRid ssidr;
        
        memcpy(&ssidr,pssidr,sizeof(ssidr));

	ssidr.len = OSSwapHostToLittleInt16(ssidr.len);
	for(i = 0; i < 3; i++) {
		ssidr.ssids[i].len = OSSwapHostToLittleInt16(ssidr.ssids[i].len);
	}
	rc = setRecord(0xFF11, &ssidr, sizeof(ssidr), false);
	return rc;
}

IOReturn AiroJackCard::
_readConfig (bool factoryConfig) {
	int rc;
	UInt16 *s;
        int i;
        
        WLLogDebug("AiroJackCard::_readConfig: called\n" );
	//if (_cfg.len)
	//	return kIOReturnSuccess;

	for (i = 0; i < 20; i++) {
            rc = getRecord(factoryConfig ? 0xFF21 : 0xFF20, &_cfg, sizeof(ConfigRid), false);
            if (rc == kIOReturnSuccess) break;
        }
        
        if (rc != kIOReturnSuccess)
		return rc;

	for(s = &_cfg.len; s <= &_cfg.rtsThres; s++) *s = OSSwapLittleToHostInt16(*s);

	for(s = &_cfg.shortRetryLimit; s <= &_cfg.radioType; s++)
		*s = OSSwapLittleToHostInt16(*s);

	for(s = &_cfg.txPower; s <= &_cfg.radioSpecific; s++)
		*s = OSSwapLittleToHostInt16(*s);

	for(s = &_cfg.arlThreshold; s <= &_cfg.autoWake; s++)
		*s = OSSwapLittleToHostInt16(*s);

	return kIOReturnSuccess;
}

IOReturn AiroJackCard::
_writeConfig () {
	ConfigRid cfg;
        UInt16 *s;
        
        memcpy(&cfg,&_cfg,sizeof(ConfigRid));
        
	for(s = &cfg.len; s <= &cfg.rtsThres; s++) *s = OSSwapHostToLittleInt16(*s);

	for(s = &cfg.shortRetryLimit; s <= &cfg.radioType; s++)
		*s = OSSwapHostToLittleInt16(*s);

	for(s = &cfg.txPower; s <= &cfg.radioSpecific; s++)
		*s = OSSwapHostToLittleInt16(*s);

	for(s = &cfg.arlThreshold; s <= &cfg.autoWake; s++)
		*s = OSSwapHostToLittleInt16(*s);

	return setRecord(0xFF10, &cfg, sizeof(cfg), false);
}

IOReturn AiroJackCard::
_sendFrame(UInt8* data, IOByteCount size)
{
    int id;

    if (size%2) {
        data[size]=0;
        size++;
    }
    
    WLLogInfo("AiroJackCard::_sendFrame: %u\n", (int)size);
    
    if (_doCommand(wlcAllocateTx, size)  != kIOReturnSuccess) {
        WLLogCrit("AiroJackCard::_sendFrame: failed to allocate %u bytes on NIC\n", (int)size);
        return kIOReturnError;
    }

    while ((getRegister(wlEvStat) & wleAlloc) == 0) ;
    // get the allocated fid and acknowledge
    id = getRegister(wlAllocFID);
    setRegister(wlEvAck, wleAlloc);

    WLLogDebug("AiroJackCard::_sendFrame: FID is 0x%x\n", id);
    
    setRegister(wlSelect0, id);
    setRegister(wlOffset0, 0);

    if (_waitForNotBusy(wlOffset0,0) != kIOReturnSuccess) {
	WLLogErr("AiroJackCard::_sendFrame: _waitForNotBusy error\n");
	return kIOReturnError;
    }
    
    for (UInt32 i = 0; i < size; i+=2)
        setRegister(wlData0, *((UInt16*)&data[i]), false); // w*(UInt16*)((UInt16*)_ioBase + (unsigned long)(wlData0>>1)) = *((UInt16*)&data[i]);
    
    if (_doCommand(wlcTransmit, id)  != kIOReturnSuccess) {
        WLLogCrit("AiroJackCard::_sendFrame: could not transmit packet\n");
        return kIOReturnError;
    }
    
    return kIOReturnSuccess;
}

void AiroJackCard::
enableInterrupts()
{
    UInt16 status = getRegister(wlEvStat);
    setRegister(wlEvAck, status);

    setRegister(wlIntEn, _interrupts);
}

void AiroJackCard::
disableInterrupts()
{
    setRegister(wlIntEn, 0);
}

void AiroJackCard::
ackAllInterrupts() {
    /*
     * Acknowledge all interrupts by writing a "spam ack"
     */
    setRegister(wlEvAck, 0xFFFF);
}

IOReturn AiroJackCard::
handleInterrupt()
{
    //WLLogDebug("AiroJackCard::handleInterrupt: called!\n");
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
    if (evStat & wleLink) {
	_handleInfo();
        setRegister(wlEvAck, wleLink);
    }
    
    enableInterrupts();

    return kIOReturnSuccess;
}

IOReturn AiroJackCard::
_handleRx() {

    //WLLogWarn("AiroJackCard::_handleRx: called!\n");
    
    UInt16 packet[1182];    /* 60 byte header + 2304 data bytes */
    WLFrame* frameDescriptor = (WLFrame*)packet;
    UInt16* frameData = &packet[30];
    UInt16 FID = getRegister(wlRxFID);
    AiroFrame hdr;
    
    setRegister(wlSelect0, FID);
    setRegister(wlOffset0, 0);
    
    if (_waitForNotBusy(wlOffset0,0) != kIOReturnSuccess) {
	WLLogWarn("AiroJackCard::_handleRx: Timeout or error\n");
	return kIOReturnError;
    }

    /*
     * Read frame descriptor correcting the byte order as we go (the
     * control and 802.11 fields are little endian, the 802.3 fields are
     * big endian).
     */
    unsigned int i;
    UInt16 *x = (UInt16*)&hdr;
    for (i = 0; i < sizeof(hdr)/2; i++)
        x[i] = getRegister(wlData0, false);
    
    memset(packet,0,1182*2);
    
    frameDescriptor->status = OSSwapLittleToHostInt16(hdr.status);
    frameDescriptor->silence = (hdr.signal) + 10; //actually switched...
    frameDescriptor->signal = (hdr.silence);
    frameDescriptor->channel = hdr.channel;
    frameDescriptor->dataLen = OSSwapLittleToHostInt16(hdr.len);
    
    /*
     * If the frame has an FCS error, is received on a MAC port other
     * than the monitor mode port, or is a message type other than
     * normal, we don't want it.
     */
    if (frameDescriptor->status & 0x2) {
        return kIOReturnSuccess;
    }
 
    if (frameDescriptor->dataLen > 2312) {
        WLLogWarn("AiroJackCard::_handleRx: Oversized packet (%d bytes)\n",
                  frameDescriptor->dataLen);
        return kIOReturnSuccess;
    }

    /*
     * Read in the packet data.  Read 4 extra words for IV + ICV if
     * applicable.
     */
    for (i = 7; i < 22; i++)
        packet[i] = getRegister(wlData0, false);
    getRegister(wlData0, false); //throw away empty data len

    if (frameDescriptor->frameControl & 0x0040) getRegister(wlData0, false); //throw away one more if WEPed packet

    UInt16 dataLength = (frameDescriptor->dataLen / 2);
    for (i = 0; i < dataLength; i++)
        frameData[i] = getRegister(wlData0, false);
    
    if (_packetQueue) {
        UInt32 packetLength = sizeof(WLFrame) + (dataLength * 2);
        if (!_packetQueue->enqueue(packet, packetLength))
            WLLogCrit("AiroJackCard::_handleRx: packet queue overflow\n");
    }
    
    return kIOReturnSuccess;
}

IOReturn AiroJackCard::
_handleTx() {
    WLLogInfo("AiroJackCard::_handleTx: packet sent\n");
    return kIOReturnSuccess;
}

IOReturn AiroJackCard::
_handleTxExc() {
    WLLogInfo("AiroJackCard::_handleTxExc: packet sent\n");
    return kIOReturnSuccess;
}

IOReturn AiroJackCard::
_handleAlloc() {
    return kIOReturnSuccess;
}

IOReturn AiroJackCard::
_handleCmd() {
    return kIOReturnSuccess;
}

IOReturn AiroJackCard::
_handleInfo() {
    return kIOReturnSuccess;
}

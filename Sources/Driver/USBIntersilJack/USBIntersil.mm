/*
        
        File:			USBIntersil.mm
        Program:		KisMAC
		Author:			Michael Rossberg
						mick@binaervarianz.de
		Description:	KisMAC is a wireless stumbler for MacOS X.
                
        This file is part of KisMAC.

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
	
#include <unistd.h>
#include <CoreFoundation/CoreFoundation.h>
#include <IOKit/IOKitLib.h>
#include <IOKit/IOCFPlugIn.h>

#include "USBIntersil.h"

#define wlcDeviceGone   (int)0xe000404f
#define align64(a)      (((a)+63)&~63)

struct identStruct {
    UInt16 vendor;
    UInt16 device;
};

static struct identStruct devices[] = {
    { 0x04bb, 0x0922}, //1 IOData AirPort WN-B11/USBS
    { 0x07aa, 0x0012}, //2 Corega Wireless LAN USB Stick-11
    { 0x09aa, 0x3642}, //3 Prism2.x 11Mbps WLAN USB Adapter
    { 0x1668, 0x0408}, //4 Actiontec Prism2.5 11Mbps WLAN USB Adapter
    { 0x1668, 0x0421}, //5 Actiontec Prism2.5 11Mbps WLAN USB Adapter
    { 0x066b, 0x2212}, //6 Linksys WUSB11v2.5 11Mbps WLAN USB Adapter
    { 0x066b, 0x2213}, //7 Linksys WUSB12v1.1 11Mbps WLAN USB Adapter
    { 0x067c, 0x1022}, //8 Siemens SpeedStream 1022 11Mbps WLAN USB Adapter
    { 0x049f, 0x0033}, //9 Compaq/Intel W100 PRO/Wireless 11Mbps multiport WLAN Adapter
    { 0x0411, 0x0016}, //10 Melco WLI-USB-S11 11Mbps WLAN Adapter
    { 0x08de, 0x7a01}, //11 PRISM25 IEEE 802.11 Mini USB Adapter
    { 0x8086, 0x1111}, //12 Intel PRO/Wireless 2011B LAN USB Adapter
    { 0x0d8e, 0x7a01}, //13 PRISM25 IEEE 802.11 Mini USB Adapter
    { 0x045e, 0x006e}, //14 Microsoft MN510 Wireless USB Adapter
    { 0x0967, 0x0204}, //15 Acer Warplink USB Adapter
    { 0x0cde, 0x0002}, //16 Z-Com 725/726 Prism2.5 USB/USB Integrated
    { 0x413c, 0x8100}, //17 Dell TrueMobile 1180 Wireless USB Adapter
    { 0x0b3b, 0x1601}, //18 ALLNET 0193 11Mbps WLAN USB Adapter
    { 0x0b3b, 0x1602}, //19 ZyXEL ZyAIR B200 Wireless USB Adapter
    { 0x0baf, 0x00eb}, //20 USRobotics USR1120 Wireless USB Adapter
    { 0x0411, 0x0027}, //21 Melco WLI-USB-KS11G 11Mbps WLAN Adapter
    { 0x04f1, 0x3009}, //22 JVC MP-XP7250 Builtin USB WLAN Adapter
    { 0x03f3, 0x0020}, //23 Adaptec AWN-8020 USB WLAN Adapter
    { 0x0ace, 0x1201}, //24 ZyDAS ZD1201 Wireless USB Adapter
    { 0x2821, 0x3300}, //25 ASUS-WL140 Wireless USB Adapter
    { 0x2001, 0x3700}, //26 DWL-122 Wireless USB Adapter
    { 0x0846, 0x4110}, //27 NetGear MA111
    { 0x0772, 0x5731}, //28 MacSense WUA-700
    { 0x124a, 0x4017}, //29 AirVast WN-220?
    { 0x9016, 0x182d}, //30 Sitecom WL-022 - new version
	{ 0x0707, 0xee04}, //31 SMC WUSB32
	{ 0x1915, 0x2236}, //32 WUSB11 version 3.0
    { 0x083a, 0x3503}, //33 T-Sinus 111 data
};

#define dDeviceCount 33

#define dbgOutPutBuf(a) NSLog( @"0x%.4x 0x%.4x 0x%.4x 0x%.4x%.4x", NSSwapLittleShortToHost(*((UInt16*)&(a) )), NSSwapLittleShortToHost(*((UInt16*)&(a)+1)), NSSwapLittleShortToHost(*((UInt16*)&(a)+2)), NSSwapLittleShortToHost(*((UInt16*)&(a)+3)), NSSwapLittleShortToHost(*((UInt16*)&(a)+4)) );

#define kInterruptPipe          3
#define kOutPipe                2
#define kInPipe                 1

#import <mach/mach_types.h>
#import <mach/mach_error.h>


bool USBIntersilJack::startCapture(UInt16 channel) {
    if (!_devicePresent) return false;
    if (!_deviceInit) return false;
    	
    if ((!_isEnabled) && (_disable() != kIOReturnSuccess)) {
        NSLog(@"USBIntersilJack::::startCapture: Couldn't disable card\n");
        return false;
    }
    
    if (setChannel(channel) == false) {
        NSLog(@"USBIntersilJack::::startCapture: setChannel(%d) failed - resetting...\n",
                 channel);
		_reset();
        return false;
    }

    if (_doCommand(wlcMonitorOn, 0) != kIOReturnSuccess) {
        NSLog(@"USBIntersilJack::::startCapture: _doCommand(wlcMonitorOn) failed\n");
        return false;
    }

    if (_enable() != kIOReturnSuccess) {
        NSLog(@"USBIntersilJack::::startCapture: Couldn't enable card\n");
        return false;
    }
    
    _channel = channel;
    return true;
}

bool USBIntersilJack::stopCapture() {
    _channel = 0;
    
    if (!_devicePresent) return false;
    if (!_deviceInit) return false;
    
    if (_doCommand(wlcMonitorOff, 0) != kIOReturnSuccess) {
        NSLog(@"USBIntersilJack::stopCapture: _doCommand(wlcMonitorOff) failed\n");
        return false;
    }

    return true;
}

bool USBIntersilJack::getChannel(UInt16* channel) {
    if (!_devicePresent) return false;
    if (!_deviceInit) return false;
    
    if (_getValue(0xFDC1, channel) != kIOReturnSuccess) {
        NSLog(@"USBIntersilJack::getChannel: getValue error\n");
        return false;
    }
    
    _channel = *channel;
    return true;
}

bool USBIntersilJack::getAllowedChannels(UInt16* channels) {
    if (!_devicePresent) return false;
    if (!_deviceInit) return false;
    
    if (_getValue(0xFD10, channels) != kIOReturnSuccess) {
        NSLog(@"USBIntersilJack::getAllowedChannels: getValue error\n");
        return false;
    }
    
     return true;
}

bool USBIntersilJack::setChannel(UInt16 channel) {
    if (!_devicePresent) return false;
    if (!_deviceInit) return false;
    
    if (_setValue(0xFC03, channel) != kIOReturnSuccess) {
        usleep(10000);
        if (_setValue(0xFC03, channel) != kIOReturnSuccess) {
            NSLog(@"USBIntersilJack::setChannel: setValue error\n");
            return false;
        }
    }

    if (_isEnabled) {
        if (_disable() != kIOReturnSuccess) {
            NSLog(@"USBIntersilJack::setChannel: Couldn't disable card\n");
            return false;
        }
        if (_enable() != kIOReturnSuccess) {
            NSLog(@"USBIntersilJack::setChannel: Couldn't enable card\n");
            return false;
        }
    }
    
    _channel = channel;
    return true;
}

bool USBIntersilJack::devicePresent() {
    return _devicePresent;
}

WLFrame *USBIntersilJack::receiveFrame() {
    WLFrame* ret;
    
    if (!_devicePresent) return NULL;
    
    pthread_mutex_lock(&_recv_mutex);
    pthread_cond_wait(&_recv_cond, &_recv_mutex);
    if (_frameSize==0) ret = NULL;
    else {
        ret = (WLFrame*)_frameBuffer;
    }
    pthread_mutex_unlock(&_recv_mutex);
    return ret;
}

bool USBIntersilJack::sendFrame(UInt8* data) {
    WLFrame *frameDescriptor;
    UInt8 aData[2364];
    IOByteCount pktsize;
    
    memcpy(aData, data, sizeof(WLFrame));
    frameDescriptor = (WLFrame*)aData;
    switch(frameDescriptor->frameControl & 0x0c) {
        case 0x08:
        case 0x00:
            pktsize = frameDescriptor->dataLen;
            if ((pktsize + sizeof(WLFrame)) > 2364) return kIOReturnBadArgument;
            frameDescriptor->dataLen=NSSwapHostShortToLittle(frameDescriptor->dataLen);
            break;
        case 0x04:
            pktsize = 0;
            frameDescriptor->dataLen = 0;
            break;
        default:
            return kIOReturnBadArgument;
    }

    frameDescriptor->txControl=NSSwapHostShortToLittle(0x08 | _TX_RETRYSTRAT_SET(3)| _TX_CFPOLL_SET(1) | _TX_TXEX_SET(0) | _TX_TXOK_SET(0) | _TX_MACPORT_SET(0));
    
    frameDescriptor->rate = 0x6e;	//11 MBit/s
    frameDescriptor->tx_rate = 0x6e;	//11 MBit/s

    memcpy(aData + 0x3C, data + sizeof(WLFrame), pktsize);

    if (_sendFrame(aData, pktsize + 0x3C) != kIOReturnSuccess)
        return NO;
    
    return YES;
}

#pragma mark -

IOReturn USBIntersilJack::_getHardwareAddress(struct WLHardwareAddress* addr) {
    UInt32 size = sizeof(struct WLHardwareAddress);
    
    if (_getRecord(0xFC01, (UInt16*)addr, &size, false) != kIOReturnSuccess) {
	NSLog(@"USBIntersilJack::_getHardwareAddress: getRecord error\n");
	return kIOReturnError;
    }

    NSLog(@"USBIntersilJack: MAC 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x\n",
                addr->bytes[0], addr->bytes[1], addr->bytes[2],
                addr->bytes[3], addr->bytes[4], addr->bytes[5]);

    return kIOReturnSuccess;
}

IOReturn USBIntersilJack::_getIdentity(WLIdentity* wli) {
    UInt32 size = sizeof(WLIdentity);
    if (_getRecord(0xFD20, (UInt16*)wli, &size) != kIOReturnSuccess) {
        NSLog(@"USBIntersilJack::getIdentity: getRecord failed\n");
        return kIOReturnError;
    }

    return kIOReturnSuccess;
}

int USBIntersilJack::_getFirmwareType() {
    UInt16 card_id;
    UInt32 size = 8;
    UInt8 d[8];
    
    if (_getRecord(0xFD0B, (UInt16*)d, &size) != kIOReturnSuccess) {
        NSLog(@"USBIntersilJack::_getFirmwareType: getRecord failed\n");
        return -1;
    }

    card_id = *(UInt16*)d;
    if (card_id & 0x8000) {
        NSLog(@"USBIntersilJack: Detected a Prism2 card\n");
        return WI_INTERSIL;
    } else {
        NSLog(@"USBIntersilJack: WARNING detected a Lucent card. This is not supported! 0x%x\n",card_id);
        return WI_LUCENT;
    }
}

IOReturn USBIntersilJack::_enable() {
    if (_doCommand(wlcEnable, 0) != kIOReturnSuccess) {
        NSLog(@"USBIntersilJack::startCapture: _doCommand(wlcEnable) failed\n");
        return kIOReturnError;
    }
    _isEnabled = true;
    
    return kIOReturnSuccess;
}

IOReturn USBIntersilJack::_disable() {
    if (_doCommand(wlcDisable, 0) != kIOReturnSuccess) {
        NSLog(@"USBIntersilJack::_disable: _doCommand(wlcDisable) failed\n");
        return kIOReturnError;
    }
    _isEnabled = false;

    return kIOReturnSuccess;
}

IOReturn USBIntersilJack::_init() {
    WLIdentity ident;
    WLHardwareAddress macAddr;
    int i; 
    
    _firmwareType = -1;
    
    for (i = 0; i< wlResetTries; i++) {
        if (_reset() == kIOReturnSuccess) break;
        if (!_devicePresent) return kIOReturnError;
    }
    
    /*
     * Get firmware vendor and version
     */
     
    if (_getIdentity(&ident) != kIOReturnSuccess) {
        NSLog(@"USBIntersilJack::_init: Couldn't read card identity\n");
        return kIOReturnError;
    }

    NSLog(@"USBIntersilJack: Firmware vendor %d, variant %d, version %d.%d\n", ident.vendor, ident.variant, ident.major, ident.minor);

    if (_getHardwareAddress(&macAddr) != kIOReturnSuccess) {
        NSLog(@"USBIntersilJack::_init: Couldn't read MAC address\n");
        return kIOReturnError;
    }
    
    _deviceInit = true;
    
    return kIOReturnSuccess;
}

IOReturn USBIntersilJack::_reset() {
    int i;
    
    if (_doCommand(wlcInit, 0) != kIOReturnSuccess) {
        NSLog(@"USBIntersilJack::_reset: _doCommand(wlcInit, 0) failed\n");
        return kIOReturnError;
    }
    
    usleep(100000); // give it a sec to reset
    
    for (i = 0; i < wlTimeout; i++) {
        _firmwareType = _getFirmwareType();
        if (_firmwareType != -1) break;
    }
    
    for (i = 0; i < wlTimeout; i++) {
        if (_setValue(0xFC00, (_firmwareType == WI_LUCENT) ? 0x3 : 0x5) == kIOReturnSuccess) break;
    }

    if (_firmwareType == WI_INTERSIL) {
        _setValue(0xFC06, 1); //syscale
        _setValue(0xFC07, 2304); //max data len
        _setValue(0xFC09, 0); //pm off!
        _setValue(0xFC84, 3); //default tx rate
        _setValue(0xFC85, 0); //promiscous mode
        _setValue(0xFC2A, 1); //auth type
        _setValue(0xFC2D, 1); //roaming by firmware
		_setValue(0xFC28, 0x90); //set wep ignore
    }
    
    if (i==wlTimeout) {
        NSLog(@"USBIntersilJack::_reset: could not set port type\n");
        return kIOReturnError;
    }
    
    /*
     * Set list of interesting events
     */
    //_interrupts = wleRx;  //| wleTx | wleTxExc | wleAlloc | wleInfo | wleInfDrop | wleCmd | wleWTErr | wleTick;

    _enable();
    _isSending = false;
    
    return kIOReturnSuccess;
}


#pragma mark -

IOReturn USBIntersilJack::_doCommand(enum WLCommandCode cmd, UInt16 param0, UInt16 param1, UInt16 param2) {
    UInt16 status;
    
    if (!_devicePresent) return kIOReturnError;
    
    if (_interface == NULL) {
        NSLog(@"USBIntersilJack::_doCommand called with NULL interface this is prohibited!\n");
        return kIOReturnError;
    }
    
    _lockDevice();
    /* Initialize the command */
    _outputBuffer.cmdreq.type = 	NSSwapHostShortToLittle(_USB_CMDREQ);
    _outputBuffer.cmdreq.cmd =          NSSwapHostShortToLittle(cmd);
    _outputBuffer.cmdreq.parm0 =	NSSwapHostShortToLittle(param0);
    _outputBuffer.cmdreq.parm1 =	NSSwapHostShortToLittle(param1);
    _outputBuffer.cmdreq.parm2 =	NSSwapHostShortToLittle(param2);
    
    if (_writeWaitForResponse(sizeof(_outputBuffer.cmdreq)) != kIOReturnSuccess) {
        NSLog(@"USBIntersilJack::unable to execute commmand (%08x)\n", cmd);
        _unlockDevice();
        return kIOReturnError;
    }
    
    status = NSSwapLittleShortToHost(_inputBuffer.cmdresp.status) >> 7;
    _unlockDevice();
    
    if (status) {
        NSLog(@"USBIntersilJack::_doCommand: Status code (0x%x) at command cmd 0x%x\n", status, cmd);
        return kIOReturnError;
    }

    return kIOReturnSuccess;
}

IOReturn USBIntersilJack::_doCommandNoWait(enum WLCommandCode cmd, UInt16 param0, UInt16 param1, UInt16 param2) {
    IOReturn kr;
    
    if (_interface == NULL) {
        NSLog(@"USBIntersilJack::_doCommandNoWait called with NULL interface this is prohibited!\n");
        return kIOReturnError;
    }
    
    _lockDevice();
    /* Initialize the command */
    _outputBuffer.cmdreq.type = 	NSSwapHostShortToLittle(_USB_CMDREQ);
    _outputBuffer.cmdreq.cmd =          NSSwapHostShortToLittle(cmd);
    _outputBuffer.cmdreq.parm0 =	NSSwapHostShortToLittle(param0);
    _outputBuffer.cmdreq.parm1 =	NSSwapHostShortToLittle(param1);
    _outputBuffer.cmdreq.parm2 =	NSSwapHostShortToLittle(param2);
    
    kr = (*_interface)->WritePipe(_interface, kOutPipe, &_outputBuffer, sizeof(_outputBuffer.cmdreq));
    _unlockDevice();

    return kr;
}

IOReturn USBIntersilJack::_getRecord(UInt16 rid, void* buf, UInt32* n, bool swapBytes) {
    UInt32  readLength = 0;
    
    if (!_devicePresent) return kIOReturnError;
    
    if (_interface == NULL) {
        NSLog(@"USBIntersilJack::_getRecord called with NULL interface this is prohibited!\n");
        return kIOReturnError;
    }
    
    _lockDevice();
    
    _outputBuffer.rridreq.type =   NSSwapHostShortToLittle(_USB_RRIDREQ);
    _outputBuffer.rridreq.frmlen = NSSwapHostShortToLittle(sizeof(_outputBuffer.rridreq.rid));
    _outputBuffer.rridreq.rid =    NSSwapHostShortToLittle(rid);
    
    if (_writeWaitForResponse(sizeof(_outputBuffer.rridreq)) != kIOReturnSuccess) {
        NSLog(@"USBIntersilJack::unable to write record offset.\n");
        _unlockDevice();
        return kIOReturnError;
    }
    
    readLength = ((NSSwapLittleShortToHost(_inputBuffer.rridresp.frmlen)-1)*2);
    if (readLength != *n) {  
        //NSLog(@"USBIntersilJack::RID len mismatch, rid=0x%04x hlen=%d fwlen=%d\n", rid, *n, readLength);
        _unlockDevice();
        return kIOReturnError;
    }

    if (swapBytes) {
        swab(_inputBuffer.rridresp.data, buf, readLength);
    } else {
        memcpy(buf, _inputBuffer.rridresp.data, readLength);
    }
    
    _unlockDevice();
    
    return kIOReturnSuccess;
}

IOReturn USBIntersilJack::_setRecord(UInt16 rid, const void* buf, UInt32 n, bool swapBytes) {
    UInt32      numBytes;
    UInt16      status;
  
    if (!_devicePresent) return kIOReturnError;
    
    if (_interface == NULL) {
        NSLog(@"USBIntersilJack::_setRecord called with NULL interface this is prohibited!\n");
        return kIOReturnError;
    }
    
    _lockDevice();

    bzero(&_outputBuffer, sizeof(_outputBuffer));
    _outputBuffer.wridreq.type =   NSSwapHostShortToLittle(_USB_WRIDREQ);
    _outputBuffer.wridreq.frmlen = NSSwapHostShortToLittle((sizeof(_outputBuffer.wridreq.rid) + n + 1) / 2);
    _outputBuffer.wridreq.rid =    NSSwapHostShortToLittle(rid);
    
    if (swapBytes) {
        swab(buf, _outputBuffer.wridreq.data, n);
    } else {
        memcpy(_outputBuffer.wridreq.data, buf, n);
    }
    
    numBytes =  align64(sizeof(_outputBuffer.wridreq.type) +
			sizeof(_outputBuffer.wridreq.frmlen) + sizeof(_outputBuffer.wridreq.rid) + n);
   
    if (_writeWaitForResponse(numBytes) != kIOReturnSuccess) {
        NSLog(@"USBIntersilJack::unable to write record offset for writing.\n");
        _unlockDevice();
        return kIOReturnError;
    }
    
    status = NSSwapLittleShortToHost(_inputBuffer.wridresp.status);
    if (status & 0x7F00) {
	NSLog(@"USBIntersilJack::_setRecord: setRecord result 0x%x\n", status & 0x7F00);
        _unlockDevice();
        return kIOReturnError;
    }
    
    _unlockDevice();
    
    return kIOReturnSuccess;
}

IOReturn USBIntersilJack::_getValue(UInt16 rid, UInt16* v) {
    UInt32 n = 2;
    return _getRecord(rid, v, &n);
}

IOReturn USBIntersilJack::_setValue(UInt16 rid, UInt16 v) {
    UInt16 value = v;
    IOReturn ret = _setRecord(rid, &value, 2);

    if (ret != kIOReturnSuccess)
        return ret;

    ret = _getValue(rid, &value);

    if (ret != kIOReturnSuccess)
        return ret;

    if (value != v) {
        //NSLog(@"USBIntersilJack::setValue: Failed to set value (0x%x != 0x%x) for register 0x%x\n", value, v, rid);
        return kIOReturnError;
    }

    return kIOReturnSuccess;
}

IOReturn USBIntersilJack::_sendFrame(UInt8* data, IOByteCount size) {
    UInt32      numBytes;
    IOReturn    kr;
  
    if (!_devicePresent) return kIOReturnError;
    
    if (_interface == NULL) {
        NSLog(@"USBIntersilJack::_sendFrame called with NULL interface this is prohibited!\n");
        return kIOReturnError;
    }
    
    _lockDevice();

    memcpy(&_outputBuffer, data, size);
    _outputBuffer.type =   NSSwapHostShortToLittle(_USB_TXFRM);
    
    numBytes =  align64(size);
   
    kr = (*_interface)->WritePipe(_interface, kOutPipe, &_outputBuffer, numBytes);

    _unlockDevice();
        
    return kr;
}

#pragma mark -

void  USBIntersilJack::_lockDevice() {
    pthread_mutex_lock(&_wait_mutex);
}

void  USBIntersilJack::_unlockDevice() {
    pthread_mutex_unlock(&_wait_mutex);
}

IOReturn USBIntersilJack::_writeWaitForResponse(UInt32 size) {
    IOReturn kr;
    struct timespec to;
    int error;
    int calls = 0;
    
    to.tv_nsec = 0;
    
    do {
        kr = (*_interface)->WritePipe(_interface, kOutPipe, &_outputBuffer, size);
        if (kr != kIOReturnSuccess) {
            if (kr==wlcDeviceGone) _devicePresent = false;
            NSLog(@"USBIntersilJack::unable to write to USB Device(%08x)\n", kr);
            return kr;
        }

        to.tv_sec  = time(0) + 5;
        error = pthread_cond_timedwait(&_wait_cond, &_wait_mutex, &to);
        if (error == ETIMEDOUT) NSLog(@"Timeout error.");
        
        if (calls++ == 5) return kIOReturnTimeout;
    } while (error == ETIMEDOUT); //wait for async response

    return kIOReturnSuccess;
}
 
void USBIntersilJack::_interruptRecieved(void *refCon, IOReturn result, int len) {
    USBIntersilJack             *me = (USBIntersilJack*) refCon;
    IOReturn                    kr;
    UInt32                      type;
    
    //NSLog(@"interruptRecieved.\n");
    if (kIOReturnSuccess != result) {
        if (result == (IOReturn)0xe00002ed) {
            me->_devicePresent = false;
            //tell the recieve function that we are gone
            pthread_mutex_lock(&me->_recv_mutex);
            me->_frameSize = 0;
            pthread_cond_signal(&me->_recv_cond);
            pthread_mutex_unlock(&me->_recv_mutex);
            return;
        } else {
            if (result == kIOReturnOverrun) {  //for some reason we overran the device buffer
                NSLog(@"USBIntersilJack::Data overrun, attempting to clear the pipe stall");
                result = (*me->_interface)->ClearPipeStallBothEnds(me->_interface, kInPipe);  //3 appears to be the interrupt pipe
            }
            else {
                 NSLog(@"USBIntersilJack::Unhandled error from async interrupt recieved, please report on http://trac.kismac.de (%08x)\n", result);
            }
            if (me->_devicePresent) goto readon;
        }
    }
    
    type = NSSwapLittleShortToHost(me->_recieveBuffer.type);
    if (_USB_ISRXFRM(type)) {
        WLFrame* frameDescriptor = (WLFrame*)&(me->_recieveBuffer.rxfrm);
        frameDescriptor->status = NSSwapLittleShortToHost(frameDescriptor->status);
        frameDescriptor->dataLen = NSSwapLittleShortToHost(frameDescriptor->dataLen);
        frameDescriptor->channel = me->_channel;
        if (me->_channel > 14) 
            return;

        /*
            * If the frame has an FCS error, is received on a MAC port other
            * than the monitor mode port, or is a message type other than
            * normal, we don't want it.
            */
        if (frameDescriptor->status & 0x1 ||
            (frameDescriptor->status & 0x700) != 0x700 ||
            frameDescriptor->status & 0xe000) {
            goto readon;
        }
        
        if (frameDescriptor->dataLen > 2304) {
            //NSLog(@"MACJackCard::_handleRx: Oversized packet (%d bytes)\n",
            //            frameDescriptor->dataLen);
            goto readon;
        }
        
        /*
            * Read in the packet data.  Read 8 extra bytes for IV + ICV if
            * applicable.
        */
        UInt16 packetLength = sizeof(WLFrame) + (frameDescriptor->dataLen) + 8;
        
        pthread_mutex_lock(&me->_recv_mutex);
        memcpy(me->_frameBuffer, frameDescriptor, packetLength);
        me->_frameSize = packetLength;
        
        pthread_cond_signal(&me->_recv_cond);
        pthread_mutex_unlock(&me->_recv_mutex);
        
    } else {
        switch (type) {
        case _USB_INFOFRM:
            /*if (action == ABORT)
                    goto exit;
            if (action == HANDLE)
                    _usbin_info(wlandev, usbin);*/
            break;
        case _USB_CMDRESP:
        case _USB_WRIDRESP:
        case _USB_RRIDRESP:
        case _USB_WMEMRESP:
        case _USB_RMEMRESP:
                pthread_mutex_lock(&me->_wait_mutex);
                memcpy(&me->_inputBuffer, &me->_recieveBuffer, len);
                pthread_cond_signal(&me->_wait_cond);
                pthread_mutex_unlock(&me->_wait_mutex);
                break;
        case _USB_BUFAVAIL:
                NSLog(@"Received BUFAVAIL packet, frmlen=%d\n", me->_recieveBuffer.bufavail.frmlen);
                break;
        case _USB_ERROR:
                NSLog(@"Received USB_ERROR packet, errortype=%d\n", me->_recieveBuffer.usberror.errortype);
                break;
    
        default:
                break;
        }
    }
    
readon:
    bzero(&me->_recieveBuffer, sizeof(me->_recieveBuffer));
    kr = (*me->_interface)->ReadPipeAsync((me->_interface), kInPipe, &me->_recieveBuffer, sizeof(me->_recieveBuffer), (IOAsyncCallback1)_interruptRecieved, refCon);
    if (kIOReturnSuccess != kr) {
        NSLog(@"USBIntersilJack::Unable to do async interrupt read (%08x). The card is stopped!\n", kr);
        if (kr == kIOReturnNoDevice) {
            NSLog(@"USBIntersilJack::There is no connection to an IOService,");
            me->_devicePresent = false;
        }
        if (kr == kIOReturnNotOpen) {
            NSLog(@"USBIntersilJack::Pipe not open for exclusive access.");
            me->_devicePresent = false;
        }
        //we should never get here because some devices don't like to be inited more than once, however this might do something good 
		// I haven't been able to reproduce the error that caused it to hit this point in the code again since adding the following lines
		// however, when it hit this point previously, the only solution was to kill and relaunch KisMAC, so at least this won't make anything worse
		NSLog(@"Attempting to re-initialise adapter...");
		if (me->_init() != kIOReturnSuccess) NSLog(@"USBIntersilJack::_interruptReceived: _init() failed\n");
    }
}

#pragma mark -

IOReturn USBIntersilJack::_configureAnchorDevice(IOUSBDeviceInterface **dev) {
    UInt8				numConf;
    IOReturn				kr;
    IOUSBConfigurationDescriptorPtr	confDesc;
    
    kr = (*dev)->GetNumberOfConfigurations(dev, &numConf);
    if (!numConf)
        return kIOReturnError;
    
    // get the configuration descriptor for index 0
    kr = (*dev)->GetConfigurationDescriptorPtr(dev, 0, &confDesc);
    if (kr) {
        NSLog(@"\tunable to get config descriptor for index %d (err = %08x)\n", 0, kr);
        return kIOReturnError;
    }
    kr = (*dev)->SetConfiguration(dev, confDesc->bConfigurationValue);
    if (kr) {
        NSLog(@"\tunable to set configuration to value %d (err=%08x)\n", 0, kr);
        return kIOReturnError;
    }
    
    return kIOReturnSuccess;
}

IOReturn USBIntersilJack::_findInterfaces(void *refCon, IOUSBDeviceInterface **dev) {
    IOReturn			kr;
    IOUSBFindInterfaceRequest	request;
    io_iterator_t		iterator;
    io_service_t		usbInterface;
    IOCFPlugInInterface 	**plugInInterface = NULL;
    IOUSBInterfaceInterface192 	**intf = NULL;
    HRESULT 			res;
    SInt32 			score;
    UInt8			intfClass;
    UInt8			intfSubClass;
    UInt8			intfNumEndpoints;
    int				pipeRef;
    CFRunLoopSourceRef		runLoopSource;
    BOOL                        error;
    USBIntersilJack             *me = (USBIntersilJack*)refCon;
    
    request.bInterfaceClass = kIOUSBFindInterfaceDontCare;
    request.bInterfaceSubClass = kIOUSBFindInterfaceDontCare;
    request.bInterfaceProtocol = kIOUSBFindInterfaceDontCare;
    request.bAlternateSetting = kIOUSBFindInterfaceDontCare;
   
    kr = (*dev)->CreateInterfaceIterator(dev, &request, &iterator);
    
    while ((usbInterface = IOIteratorNext(iterator))) {
        //NSLog(@"Interface found.\n");
       
        kr = IOCreatePlugInInterfaceForService(usbInterface, kIOUSBInterfaceUserClientTypeID, kIOCFPlugInInterfaceID, &plugInInterface, &score);
        kr = IOObjectRelease(usbInterface);				// done with the usbInterface object now that I have the plugin
        if ((kIOReturnSuccess != kr) || !plugInInterface) {
            NSLog(@"unable to create a plugin (%08x)\n", kr);
            break;
        }
            
        // I have the interface plugin. I need the interface interface
        res = (*plugInInterface)->QueryInterface(plugInInterface, CFUUIDGetUUIDBytes(kIOUSBInterfaceInterfaceID), (void**) &intf);
        (*plugInInterface)->Release(plugInInterface);			// done with this
        if (res || !intf) {
            NSLog(@"couldn't create an IOUSBInterfaceInterface (%08x)\n", (int) res);
            break;
        }
        
        kr = (*intf)->GetInterfaceClass(intf, &intfClass);
        kr = (*intf)->GetInterfaceSubClass(intf, &intfSubClass);
        
        //NSLog(@"Interface class %d, subclass %d\n", intfClass, intfSubClass);
        
        // Now open the interface. This will cause the pipes to be instantiated that are 
        // associated with the endpoints defined in the interface descriptor.
        kr = (*intf)->USBInterfaceOpen(intf);
        if (kIOReturnSuccess != kr) {
            NSLog(@"unable to open interface (%08x)\n", kr);
            (void) (*intf)->Release(intf);
            break;
        }
        
    	kr = (*intf)->GetNumEndpoints(intf, &intfNumEndpoints);
        if (kIOReturnSuccess != kr) {
            NSLog(@"unable to get number of endpoints (%08x)\n", kr);
            (void) (*intf)->USBInterfaceClose(intf);
            (void) (*intf)->Release(intf);
            break;
        }
        
        if (intfNumEndpoints != 3) {
            NSLog(@"Error: Interface has %d endpoints. Needs 3!\n", intfNumEndpoints);
            (void) (*intf)->USBInterfaceClose(intf);
            (void) (*intf)->Release(intf);
            break;
        }
        
        for (pipeRef = 1; pipeRef <= intfNumEndpoints; pipeRef++)
        {
            IOReturn	kr2;
            UInt8	direction;
            UInt8	number;
            UInt8	transferType;
            UInt16	maxPacketSize;
            UInt8	interval;
            
            kr2 = (*intf)->GetPipeProperties(intf, pipeRef, &direction, &number, &transferType, &maxPacketSize, &interval);
            if (kIOReturnSuccess != kr) {
                NSLog(@"unable to get properties of pipe %d (%08x)\n", pipeRef, kr2);
                (void) (*intf)->USBInterfaceClose(intf);
                (void) (*intf)->Release(intf);
                break;
            } else {
                error = false;
                switch (pipeRef) {
                    case 1:
                        if (direction != kUSBIn || transferType != kUSBBulk) error = true;
                        break;
                    case 2:
                        if (direction != kUSBOut || transferType != kUSBBulk) error = true;
                        break;
                    case 3:
                        if (direction != kUSBIn || transferType  != kUSBInterrupt) error = true;
                        break;
                    default:
                        error = true;
                }
                
                if (error) {
                    NSLog(@"unable to properties of pipe %d are not as expected!\n", pipeRef);
                    (void) (*intf)->USBInterfaceClose(intf);
                    (void) (*intf)->Release(intf);
                    break;
                }
            }
        }
        
        // Just like with service matching notifications, we need to create an event source and add it 
        //  to our run loop in order to receive async completion notifications.
        kr = (*intf)->CreateInterfaceAsyncEventSource(intf, &runLoopSource);
        if (kIOReturnSuccess != kr) {
            NSLog(@"unable to create async event source (%08x)\n", kr);
            (void) (*intf)->USBInterfaceClose(intf);
            (void) (*intf)->Release(intf);
            break;
        }
        CFRunLoopAddSource(_intLoop, runLoopSource, kCFRunLoopDefaultMode);
        
        _interface = intf;
        
        NSLog(@"USBIntersilJack is now ready to start working.\n");
        
        //startUp Interrupt handling
        UInt32 numBytesRead = sizeof(_recieveBuffer); // leave one byte at the end for NUL termination
        bzero(&_recieveBuffer, numBytesRead);
        kr = (*intf)->ReadPipeAsync(intf, kInPipe, &_recieveBuffer, numBytesRead, (IOAsyncCallback1)_interruptRecieved, this);
        
        if (kIOReturnSuccess != kr) {
            NSLog(@"unable to do async interrupt read (%08x)\n", kr);
            (void) (*intf)->USBInterfaceClose(intf);
            (void) (*intf)->Release(intf);
            break;
        }
        
        _devicePresent = true;
        me->_init();
        
        if (_channel) {
            startCapture(_channel);
        }
        
        break;
    }
    
    return kr;
}

void USBIntersilJack::_addDevice(void *refCon, io_iterator_t iterator) {
    USBIntersilJack             *me = (USBIntersilJack*)refCon;
    kern_return_t		kr;
    io_service_t		usbDevice;
    IOCFPlugInInterface 	**plugInInterface=NULL;
    IOUSBDeviceInterface 	**dev=NULL;
    HRESULT 			res;
    SInt32 			score;
    UInt16			vendor;
    UInt16			product;
    UInt16			release;
    int                         i;
    
    
    while ((usbDevice = IOIteratorNext(iterator))) {
        //NSLog(@"USB Device added.\n");
       
        kr = IOCreatePlugInInterfaceForService(usbDevice, kIOUSBDeviceUserClientTypeID, kIOCFPlugInInterfaceID, &plugInInterface, &score);
        if ((kIOReturnSuccess != kr) || !plugInInterface) {
            kr = IOObjectRelease(usbDevice);				// done with the device object now that I have the plugin
            NSLog(@"unable to create a plugin (%08x)\n", kr);
            continue;
        }
        kr = IOObjectRelease(usbDevice);				// done with the device object now that I have the plugin
            
        // I have the device plugin, I need the device interface
        res = (*plugInInterface)->QueryInterface(plugInInterface, CFUUIDGetUUIDBytes(kIOUSBDeviceInterfaceID), (void**)&dev);
        (*plugInInterface)->Release(plugInInterface);			// done with this
        if (res || !dev) {
            NSLog(@"couldn't create a device interface (%08x)\n", (int) res);
            continue;
        }
        // technically should check these kr values
        kr = (*dev)->GetDeviceVendor(dev, &vendor);
        kr = (*dev)->GetDeviceProduct(dev, &product);
        kr = (*dev)->GetDeviceReleaseNumber(dev, &release);
        
        //find the correct device
        for (i=0; i< dDeviceCount; i++) {
            if ((vendor == devices[i].vendor) && (product == devices[i].device)) break;
        }
        
        if (i == dDeviceCount) {
            NSLog(@"found device i didn't want (vendor = 0x%x, product = 0x%x)\n", vendor, product);
            (*dev)->Release(dev);
            continue;
        }
                
        // need to open the device in order to change its state
        kr = (*dev)->USBDeviceOpen(dev);
        if (kIOReturnSuccess != kr) {
            NSLog(@"unable to open device: %08x\n", kr);
            (*dev)->Release(dev);
            continue;
        }
        
        kr = me->_configureAnchorDevice(dev);
        if (kIOReturnSuccess != kr) {
            NSLog(@"unable to configure device: %08x\n", kr);
            (*dev)->USBDeviceClose(dev);
            (*dev)->Release(dev);
            continue;
        }

        kr = me->_findInterfaces(refCon, dev);
        if (kIOReturnSuccess != kr) {
            NSLog(@"unable to find interfaces on device: %08x\n", kr);
            (*dev)->USBDeviceClose(dev);
            (*dev)->Release(dev);
            continue;
        }
        
        kr = (*dev)->USBDeviceClose(dev);
        kr = (*dev)->Release(dev);
        
        break;
    }
}

void USBIntersilJack::_handleDeviceRemoval(void *refCon, io_iterator_t iterator) {
    kern_return_t	kr;
    io_service_t	obj;
    int                 count = 0;
    //USBIntersilJack     *me = (USBIntersilJack*)refCon;
    
    while ((obj = IOIteratorNext(iterator)) != nil) {
        count++;
        //NSLog(@"Device removed.\n");
        kr = IOObjectRelease(obj);
    }
    
    /*if (count) {
        me->_interface = NULL;
        me->stopRun();
    }*/
}

#pragma mark -

bool USBIntersilJack::stopRun() {
    if (_runLoop == NULL && _intLoop == NULL) return false;
    _stayUp = false;
    if (_notifyPort) {
        IONotificationPortDestroy(_notifyPort);
        _notifyPort = NULL;
    }
    
    if (_runLoop) CFRunLoopStop(_runLoop);
    if (_intLoop) CFRunLoopStop(_intLoop);
    
    _runLoop = NULL;
    _intLoop = NULL;
    return true;
}

void USBIntersilJack::_runCFRunLoop(USBIntersilJack* me) {
    me->_runLoop = CFRunLoopGetCurrent();
    while(me->_stayUp) {
        CFRunLoopRun();
    };
    if (me->_runLoop) {
        if (me->_runLoopSource) CFRunLoopRemoveSource(me->_runLoop, me->_runLoopSource, kCFRunLoopDefaultMode);
        CFRunLoopStop(me->_runLoop);
        me->_runLoop = NULL;
    }
} 

void USBIntersilJack::_intCFRunLoop(USBIntersilJack* me) {
    me->_intLoop = CFRunLoopGetCurrent();
    while(me->_stayUp) {
        CFRunLoopRun();
    };
    if (me->_intLoop) {
        CFRunLoopStop(me->_intLoop);
        me->_intLoop = NULL;
    }
} 

bool USBIntersilJack::run() {
    pthread_t pt;
    
    _stayUp = true;
    if (_runLoop==NULL) {
        pthread_create(&pt, NULL, (void*(*)(void*))_runCFRunLoop, this);
    }
    if (_intLoop==NULL) {
        pthread_create(&pt, NULL, (void*(*)(void*))_intCFRunLoop, this);
    }
    
    return true;
}

void USBIntersilJack::startMatching() {
    mach_port_t 		masterPort;
    CFMutableDictionaryRef 	matchingDict;
    kern_return_t		kr;
 
    if (_runLoopSource) return;
    
    // first create a master_port for my task
    kr = IOMasterPort(MACH_PORT_NULL, &masterPort);
    if (kr || !masterPort) {
        NSLog(@"ERR: Couldn't create a master IOKit Port(%08x)\n", kr);
    }

    //NSLog(@"Looking for devices matching vendor ID=%ld and product ID=%ld\n", _vendorID, _productID);

    // Set up the matching criteria for the devices we're interested in
    matchingDict = IOServiceMatching(kIOUSBDeviceClassName);	// Interested in instances of class IOUSBDevice and its subclasses
    if (!matchingDict) {
        NSLog(@"Can't create a USB matching dictionary\n");
        //mach_port_deallocate(mach_task_self(), masterPort);
        return;
    }
    
    // Create a notification port and add its run loop event source to our run loop
    // This is how async notifications get set up.
    _notifyPort = IONotificationPortCreate(masterPort);
    _runLoopSource = IONotificationPortGetRunLoopSource(_notifyPort);
    
    CFRunLoopAddSource(_runLoop, _runLoopSource, kCFRunLoopDefaultMode);
    
    // Retain additional references because we use this same dictionary with four calls to 
    // IOServiceAddMatchingNotification, each of which consumes one reference.
    matchingDict = (CFMutableDictionaryRef) CFRetain(matchingDict); 
    
    // Now set up two more notifications, one to be called when a bulk test device is first matched by I/O Kit, and the other to be
    // called when the device is terminated.
    kr = IOServiceAddMatchingNotification(  _notifyPort,
                                            kIOFirstMatchNotification,
                                            matchingDict,
                                            _addDevice,
                                            this,
                                            &_deviceAddedIter);
                                            
    _addDevice(this, _deviceAddedIter);	// Iterate once to get already-present devices and
                                                        // arm the notification

    kr = IOServiceAddMatchingNotification(  _notifyPort,
                                            kIOTerminatedNotification,
                                            matchingDict,
                                            _handleDeviceRemoval,
                                            this,
                                            &_deviceRemovedIter );
                                            
    _handleDeviceRemoval(this, _deviceRemovedIter); 	// Iterate once to arm the notification


    // Now done with the master_port
    masterPort = 0;
}

USBIntersilJack::USBIntersilJack() {
    _isEnabled = false;
    _deviceInit = false;
    _devicePresent = false;
    
    _interface = NULL;
    _runLoopSource = NULL;
    _runLoop = NULL;
    _intLoop = NULL;
    _channel = 3;
    _frameSize = 0;
    _notifyPort = NULL;
    
    pthread_mutex_init(&_wait_mutex, NULL);
    pthread_cond_init (&_wait_cond, NULL);
    pthread_mutex_init(&_recv_mutex, NULL);
    pthread_cond_init (&_recv_cond, NULL);
    
    run();
    
    while (_runLoop==NULL || _intLoop==NULL)
        usleep(100);
}

USBIntersilJack::~USBIntersilJack() {
    stopRun();
    _interface = NULL;
    _frameSize = 0;

    pthread_mutex_destroy(&_wait_mutex);
    pthread_cond_destroy(&_wait_cond);
    pthread_mutex_destroy(&_recv_mutex);
    pthread_cond_destroy(&_recv_cond);
}
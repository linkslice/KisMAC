/*
        
        File:			AtheroJack.cpp
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

#include "AtheroJack.h"

extern "C" {
#include <sys/param.h>
#include <sys/mbuf.h>
#include <string.h>
}

#define super WiFiControllerPCI
OSDefineMetaClassAndStructors(AtheroJack, WiFiControllerPCI)

const OSString* AtheroJack::newModelString() const {
    return OSString::withCString("Atheros card");
}

#define ROUNDUP(x, inc) ( (x) + (inc) - ((x)%(inc)) )

AtheroJack::opmodeSettings AtheroJack::_opmodeSettings[] = {
    { _operationModeStation, HAL_M_STA,     (HAL_RX_FILTER)(HAL_RX_FILTER_UCAST | HAL_RX_FILTER_BCAST | HAL_RX_FILTER_BEACON | HAL_RX_FILTER_PHYERR) },
    { _operationModeMonitor, HAL_M_MONITOR,	(HAL_RX_FILTER)(HAL_RX_FILTER_UCAST | HAL_RX_FILTER_BCAST | HAL_RX_FILTER_MCAST | HAL_RX_FILTER_PROM | HAL_RX_FILTER_PROBEREQ ) },
    { _operationModeInvalid, HAL_M_STA,     (HAL_RX_FILTER)(0) }
};

#define	IEEE80211_WEP_KEYLEN			5	/* 40bit */
#define	IEEE80211_WEP_IVLEN			3	/* 24bit */
#define	IEEE80211_WEP_KIDLEN			1	/* 1 octet */
#define	IEEE80211_WEP_CRCLEN			4	/* CRC-32 */
#define	IEEE80211_WEP_NKID			4	/* number of key ids */

#define	IEEE80211_CRC_LEN			4

#define	IEEE80211_MTU				1500
#define	IEEE80211_MIN_LEN \
	(sizeof(struct ieee80211_frame_min) + IEEE80211_CRC_LEN)

#pragma mark -

bool AtheroJack::startHardware() {
    int i;
    WLEnter();
  
    _activeOpMode = 1;
    _rxBufferSize = MAX_FRAGMENT_SIZE;//ROUNDUP(IEEE80211_MAX_LEN, CACHE_ALIGNMENT);
	WLLogDebug("Buffer size is: %d", _rxBufferSize);
    _rxListHead = _rxListTail = NULL;
    _headRx = _tailRx = 0;
    _activeChannelIndex = 0;
    _intrMask = (HAL_INT)0;
    _hal = new OpenHAL5212;
	
	if (!_hal) {
		WLLogCrit("Could not alloc HAL");
		return false;
	}
	
    if (!initHardware()) return false;

    WLExit();

    return true;
}

bool AtheroJack::initHardware() {
    HAL_STATUS status = HAL_OK;
    int att_cnt = 0;
    bool ret;
	WLEnter();
    
	do {
        ret = _hal->ath_hal_attach(_nub->configRead16(kIOPCIConfigDeviceID),
                                 NULL, 0, (void*)_ioBase,
                                 &status);
        if (!ret) IODelay(10000);
    } while (!ret && (att_cnt++ < 4) );
		
    if (!ret) {
        WLLogCrit("Unable to attach to _hal: status %u", status);
        WLReturn(false);
    }
	
	IOLog("AtheroJack: EEPROM version: 0x%04x\n", _hal->ah_ee_version);
	
    _setLedState(HAL_LED_INIT);
    _updateMACAddress();
    
	/*
	 * Get regulation domain either stored in the EEPROM or defined
	 * as the default value. Some devices are known to have broken
	 * regulation domain values in their EEPROM.
	 */
	_hal->ar5k_get_regdomain();
	
	/*
	 * Override regulation domain
	 * Maybe switch to DMN_ETSI1_WORLD as soon KisMAC supports 5GHz ...
	 */
	_hal->ah_regdomain = DMN_ETSI2_WORLD;

	/*
	 * Construct channel list based on the current regulation domain.
	 */
	if (!_hal->ath_hal_init_channels(_chans, IEEE80211_CHAN_MAX, &_nchans, HAL_MODE_ALL, AH_TRUE, AH_TRUE)) {
		WLLogCrit("Unable to collect channel list from _hal");
		WLReturn(false);
	}

    bool hasA = false, hasB = false, hasG = false, hasT = false;
    for (u_int i = 0; i < _nchans; ++i) {
        if ((_chans[i].channelFlags & CHANNEL_A) == CHANNEL_A) hasA = true;
        if ((_chans[i].channelFlags & CHANNEL_B) == CHANNEL_B) hasB = true;
        if ((_chans[i].channelFlags & CHANNEL_G) == CHANNEL_G) hasG = true;
        if ((_chans[i].channelFlags & CHANNEL_T) == CHANNEL_T) hasT = true;
		IOLog("channel % 3d: %u MHz, type 0x%x:",
			i + 1, _chans[i].channel, _chans[i].channelFlags);
		if (_chans[i].channelFlags & IEEE80211_CHAN_2GHZ)		IOLog(" 2GHZ");
		if (_chans[i].channelFlags & IEEE80211_CHAN_5GHZ)		IOLog(" 5GHZ");
		if (_chans[i].channelFlags & IEEE80211_CHAN_DYN)		IOLog(" DYN");
		if (_chans[i].channelFlags & IEEE80211_CHAN_CCK)		IOLog(" CCK");
		if (_chans[i].channelFlags & IEEE80211_CHAN_OFDM)		IOLog(" OFDM");
		if (_chans[i].channelFlags & IEEE80211_CHAN_TURBO)		IOLog(" TURBO");
		if (_chans[i].channelFlags & IEEE80211_CHAN_GFSK)		IOLog(" GFSK");
		if (_chans[i].channelFlags & IEEE80211_CHAN_PASSIVE)	IOLog(" PASSIVE");
		if (_chans[i].channelFlags & IEEE80211_CHAN_XR)			IOLog(" XR");
		IOLog("\n");
		IOSleep(10);
    }
	IOLog("\n");
	
    //by default prefer g mode...
    if (hasG)       _activeIEEEMode = _modulationMode80211g;
    else if (hasB)  _activeIEEEMode = _modulationMode80211b;
    else if (hasA)  _activeIEEEMode = _modulationMode80211a;
    else {
        WLLogCrit("No valid modualtion mode found!");
        WLReturn(false);
    }

	setFrequency(2412);
	//setFrequency(5240);
	
	WLLogDebug("Current radio freq %d modulation type: 0x%x", _chans[_activeChannelIndex].channel, _chans[_activeChannelIndex].channelFlags);
    WLLogDebug("Supported Modulations: 11a %s, 11b %s, 11g %s, TURBO %s", hasA ? "YES" : "NO",  hasB ? "YES" : "NO", hasG ? "YES" : "NO", hasT ? "YES" : "NO");
	
    WLExit();
	
    return true;
};

bool AtheroJack::freeHardware() {
    WLEnter();

    if (_hal) {
		if (_enabledForNetif && !_disableRx()) WLReturn(false);
		IOSleep(100);
		
		(_hal->ar5k_ar5212_detach)();
		_hal->release();
		_hal = NULL;
	}

    WLExit();
    return true;
}

bool AtheroJack::enableHardware() {
    WLEnter();
    
	_enabledForNetif = true;
    if (!_reset()) WLReturn(false);
    _setLedState(HAL_LED_RUN);
    
    WLReturn(true);
}

bool AtheroJack::disableHardware() {
    //disable the card
    WLEnter();
    	
	if (!_disableRx()) WLReturn(false);
    _setLedState(HAL_LED_INIT);
    
    WLReturn(true);
}

bool AtheroJack::getReadyForSleep() {
    WLEnter();
    
	_disableRx();
	
    WLExit();
    return true;
}

bool AtheroJack::wakeUp() {
    WLEnter();
    
	_reset();
	
    WLExit();
    return true;
}

bool AtheroJack::handleInterrupt() {
    HAL_INT ints;
    
    if (_cardGone) return false; //there are phantom interrupts upon card removal

    (_hal->ar5k_ar5212_get_isr)(&ints);
    if ((ints & HAL_INT_NOCARD) == HAL_INT_NOCARD) {
        WLLogErr("Int no card");
        return false;
    }
    
	//WLLogErr("Interrupt 0x%x", ints);
    
    if (ints & HAL_INT_FATAL) {
        WLLogCrit("Recieved Fatal interrupt! Resetting");
        _reset();
	} else if (ints & HAL_INT_RXORN) {
		WLLogCrit("Recieved FIFI overrun interrupt! Resetting");
        //_reset();
    } else if (ints & HAL_INT_RX) {
        HAL_STATUS status;
        UInt32 index;
        UInt8 packet[IEEE80211_MAX_LEN + sizeof(WLFrame)];
        WLFrame *f = (WLFrame*)packet;
            
        while(true) {
            index = _headRx % ATH_NUM_RX_DESCS;
			UInt8 *p = (UInt8*) mbuf_data(_rxData[index]);

            if ((_tailRx % ATH_NUM_RX_DESCS) == index) {
                WLLogCrit("RX overrun!!!!");
                break;
            }
            if (_controlBlock.p->d[index].ds_link == (_controlBlock.dmaAddress + (index * sizeof(ath_desc)))) {
                WLLogCrit("RX overrun 2!!!!");
                break;
            }
            
            status = (_hal->ar5k_ar5212_proc_rx_desc)((ath_desc*)&_controlBlock.p->d[index], (_controlBlock.dmaAddress + (index * sizeof(ath_desc))),
                                    (ath_desc*)&_controlBlock.p->d[(_headRx + 1) % ATH_NUM_RX_DESCS]);
            if (status != HAL_OK) {
				break;
            }
			
			//WLLogCrit("Packet!!!!");
            //WLLogCrit("Packet 0x%.2x%.2x%.2x%.2x 0x%.2x%.2x%.2x%.2x 0x%.2x%.2x%.2x%.2x 0x%.2x%.2x%.2x%.2x", p[0], p[1], p[2], p[3], p[4], p[5], p[6], p[7], p[8], p[9], p[10], p[11], p[12], p[13], p[14], p[15]);
			
			if (_rxListHead->ds_rxstat.rs_status != 0) {
                //WLLogDebug("RX Error! status:0x%x phy_error:%d datalen:%d", _rxListHead->ds_rxstat.rs_status, _rxListHead->ds_rxstat.rs_phyerr, _rxListHead->ds_rxstat.rs_datalen);
				//WLLogCrit("Phyerror!!!!");
                goto next_packet;
            }
            
			bzero(f, sizeof(WLFrame));
            f->ph.status  = _rxListHead->ds_rxstat.rs_status;
            f->ph.channel = _hal->ieee80211_mhz2ieee(_chans[_activeChannelIndex].channel, _chans[_activeChannelIndex].channelFlags);
            f->ph.signal  = _rxListHead->ds_rxstat.rs_rssi;
            f->ph.silence = 0;
            f->ph.len     = _rxListHead->ds_rxstat.rs_datalen;

            if (f->ph.len > IEEE80211_MAX_LEN) {
                WLLogCrit("Received an oversized frame! size: %u", f->ph.len);
                goto next_packet;
            }
            
            memcpy(packet + sizeof(_Prism_HEADER), p, f->ph.len);
			
            if (!_packetQueue) {
                WLLogCrit("No packet queue present!");
                break;
            }
            _packetQueue->enqueue(packet, f->ph.len + sizeof(_Prism_HEADER));
            
            //WLLogDebug("Got a frame size: %d", f->ph.len);
            
next_packet:
            _fillFragment(index);
            _headRx++;
            _rxListHead = &_controlBlock.p->d[_headRx % ATH_NUM_RX_DESCS];
        }
        
        (_hal->ar5k_ar5212_set_rx_signal)();
        (_hal->ar5k_ar5212_start_rx)();
    } else if (ints & _intrMask) {
        WLLogNotice("Unknown Interrupt 0x%X:", ints);
    }

    return true;
}

bool AtheroJack::handleTimer() {
    return _recalibration();
}

#pragma mark -

UInt32 AtheroJack::getFrequency() {
    return _chans[_activeChannelIndex].channel;
}

bool AtheroJack::setFrequency(UInt32 freq) {
    unsigned int i;
    UInt32 flags;
    HAL_STATUS status;
	
    switch(_activeIEEEMode) {
        case _modulationMode80211a:
            flags = CHANNEL_A;
            break;
        case _modulationMode80211b:
            flags = CHANNEL_B;
            break;
        case _modulationMode80211g:
            flags = CHANNEL_G;
            break;
        case _modulationModeAtherosT:
            flags = CHANNEL_T;
            break;
        default:
            WLLogAlert("Warning: Unknown IEEE mode! Defaulting to 802.11b");
            flags = CHANNEL_B;
            break;
    }
    
    for (i = 0; i < _nchans; i++) {
        if (_chans[i].channel == freq && (_chans[i].channelFlags & flags) == flags) break;
    }
    if (i == _nchans) return false;
    
    _activeChannelIndex = i;
    
	WLLogDebug("Current radio freq %d modulation type: 0x%x", _chans[_activeChannelIndex].channel, _chans[_activeChannelIndex].channelFlags);

    if (_enabledForNetif) { return _quickReset(); }
	
	return true;
}

bool AtheroJack::setFirmware(UInt32 length, UInt8* firmware)  {
	UInt16 i, crc = 0;
	UInt16 *f = (UInt16*)firmware;
	
	if (length != 0x800) {
		WLLogErr("Error EEPROM has invalid length!");
		return false;
	}
	
	if (_enabledForNetif) {
		WLLogErr("Error THE NETWORK INTERFACE IS UP!");
		return false;
	}
	
	for (i = 0xc0; i < 0x400; i++) {
		crc ^= f[i];
	}
	if (crc != 0xffff) {
		WLLogErr("Error EEPROM has invalid checksum 0xffff != 0x%x!", crc);
		return false;
	}
	
	if (f[0] != 0x0013 || f[1] != 0x168c) {
		WLLogErr("Error EEPROM has non Atheros PCI ID!");
		return false;
	}
	
	if (_hal->ar5k_ar5212_eeprom_is_busy() == AH_TRUE) {
		WLLogErr("Error EEPROM is busy!");
		return false;
	}
	
	for (i = 0xc0; i < 0x400; i++) {
		if (_hal->ar5k_ar5212_eeprom_write(i, f[i]) != HAL_OK) {
			WLLogErr("Error writing EEPROM at Position 0x%x", i);
			IOSleep(100);
		}
	}
	
	WLLogErr("Flashing succeeded\n");
	
	return true;
}

#pragma mark -

bool AtheroJack::_fillFragment(int index) {
    struct IOPhysicalSegment vector;
    UInt32 count;
    volatile struct ath_desc *d = &(_controlBlock.p->d[index]);
    
    if (!_mbufCursor) {
        WLLogEmerg("No mbuf Cursor present!");
        return false;
    }
    
    if (index < 0 || index >= ATH_NUM_RX_DESCS) {
        WLLogEmerg("Reality error: index went out of bounds %d", index);
        return false;
    }
    
	bzero(mbuf_data(_rxData[index]), _rxBufferSize);
	count = _mbufCursor->getPhysicalSegmentsWithCoalesce(_rxData[index], &vector, 1);
    if (count == 0) {
        WLLogEmerg("Could not allocated a nice mbuf!");
        return false;
    }
    
    if (vector.location & 3) {
        WLLogEmerg("Warning trying to transmit unaligned packet!");
    }
    
	bzero((void *)d, sizeof(struct ath_desc));
    d->ds_data = (vector.location);	// cursor is host-endian
    d->ds_link = (_controlBlock.dmaAddress + (index * sizeof(ath_desc))); //my own address
	
	//WLLogDebug("DMA address is 0x%x for index %d. Data is at 0x%x max size: %d. sizeof athdesc=%d", d->ds_link, index, d->ds_data, vector.length, sizeof(ath_desc));
    
    if ((_hal->ar5k_ar5212_setup_rx_desc)((ath_desc*)d, vector.length, 0) == AH_FALSE) {
        WLLogEmerg("Unable to initialize rx descriptor with HAL");
        return false;
    }
    
    _rxDataAddress[index] = d->ds_data;
    
    if (_rxListTail) {
        _rxListTail->ds_link = d->ds_link;
        _rxListTail = d;
        _tailRx++;
    } else {
        _rxListHead = _rxListTail = d;
    }
    
    return true;
}

bool AtheroJack::_allocPacketForRxDesc(int index) {
    _rxData[index] = allocatePacket(_rxBufferSize);
    if (!_rxData[index]) {
        WLLogEmerg("Could not alloc Packet for Queue!");
        return false;
    }
    
    return _fillFragment(index);
}

bool AtheroJack::_allocQueues() {
    int i;
    
    WLEnter();
    
    //_freeQueues();
    
    _rxListHead = _rxListTail = NULL;
    _headRx = _tailRx = 0;
        
    if (!allocatePageBlock(&_controlBlock.page)) {
        WLLogCrit("Could not allocate Pageblock!");
        WLReturn(false);
    }
    
    _controlBlock.p = (desc*) allocateMemoryFrom(&_controlBlock.page, sizeof(desc), CACHE_ALIGNMENT, &_controlBlock.dmaAddress);
    if (!_controlBlock.p) {
        WLLogCrit("Could not allocate rx Descs from pb");
        WLReturn(false);
    }
    
    for (int i = 0; i < ATH_NUM_RX_DESCS; i++) {
        if (!_allocPacketForRxDesc(i)) WLReturn(false);
    }
    
    WLExit();
    
    return true;
}

#pragma mark -

bool AtheroJack::_reset() {
    HAL_STATUS status;
    WLEnter();
    
    _disableInterupts(HAL_INT_COMMON);
    _disableRx();
    IODelay(3000);
    
    if (!_allocQueues()) WLReturn(false);
    
    (_hal->ar5k_ar5212_set_intr)((HAL_INT)_intrMask);
	
    if (!(_hal->ar5k_ar5212_reset)(_opmodeSettings[_activeOpMode].halmode, &_chans[_activeChannelIndex], AH_TRUE, &status)) {
        WLLogEmerg("Reset failed status %d", status);
        WLReturn(false);
    }

    _hal->ar5k_ar5212_reset_key(0);
    _hal->ar5k_ar5212_reset_key(1);
    _hal->ar5k_ar5212_reset_key(2);
    _hal->ar5k_ar5212_reset_key(3);

    if (_enabledForNetif) {
        _enableRx();
    }
    
    WLReturn(true);
}

bool AtheroJack::_quickReset() {
    HAL_STATUS status;
    WLEnter();

	_hal->ar5k_ar5212_set_intr(0);
	_hal->ar5k_ar5212_set_rx_filter(0);
	_hal->ar5k_ar5212_stop_pcu_recv();
	_hal->ar5k_ar5212_stop_rx_dma();
	IODelay(3000);    

	if (!_hal->ar5k_ar5212_reset(_opmodeSettings[_activeOpMode].halmode, &_chans[_activeChannelIndex], AH_TRUE, &status)) {
		WLLogEmerg("Reset failed status %d", status);
		WLReturn(false);
	}
	
	if (!_enableRx()) {
		WLLogEmerg("Could not reenable RX");
		WLReturn(false);
	}

    WLReturn(true);
}

bool AtheroJack::_updateMACAddress() {
    if (_hal) {
        (_hal->ar5k_ar5212_get_lladdr)(_myAddress.bytes);
        return true;
    };
    
    WLLogCrit("No _hal pointer present!");
    return false;
}

bool AtheroJack::_recalibration() {
    if (_cardGone || !_hal) return false;
    
    if ((_hal->ar5k_ar5212_get_rf_gain)() == HAL_RFGAIN_NEED_CHANGE) {
        /*
         * Rfgain is out of bounds, reset the chip
         * to load new gain values.
         */
        WLLogEmerg("Need to change rfGain");
        _quickReset();
    }
    if (!(_hal->ar5k_ar5212_calibrate)(&_chans[_activeChannelIndex])) {
        WLLogAlert("calibration of channel %u failed", _chans[_activeChannelIndex].channel);
        return false;
    }
    
    return true;
}
bool AtheroJack::_setLedState(HAL_LED_STATE state) {
    if (_hal) {
        (_hal->ar5k_ar5212_set_ledstate)(state);
        return true;
    };
    
    WLLogCrit("No _hal pointer present!");
    return false;
}

#pragma mark -

bool AtheroJack::_disableInterupts(HAL_INT ints) {
    if (!_hal) return false;
    
    _intrMask = (HAL_INT)(_intrMask & (~ints));
    (_hal->ar5k_ar5212_set_intr)(_intrMask);
    IODelay(3000);
    
    return true;
}

bool AtheroJack::_enableRx() {
    WLEnter();
    if (!_hal) return false;

    // Enable the RX interrupts
    _intrMask = (HAL_INT)(_intrMask | HAL_INT_RX | HAL_INT_RXORN | HAL_INT_FATAL | HAL_INT_GLOBAL | HAL_INT_COMMON);
    (_hal->ar5k_ar5212_set_intr)(_intrMask);
    
    // Enable the RX process
    (_hal->ar5k_ar5212_put_rx_buf)(_controlBlock.dmaAddress + ((_headRx % ATH_NUM_RX_DESCS) * sizeof(ath_desc)));
    (_hal->ar5k_ar5212_start_rx)();
    (_hal->ar5k_ar5212_set_rx_filter)(_opmodeSettings[_activeOpMode].filter);
    (_hal->ar5k_ar5212_start_rx_pcu)();
    IODelay(3000);
    
	//_hal->ar5k_ar5212_dump_state();
	
    WLExit();
    return true;
}

bool AtheroJack::_disableRx() {
    if (!_hal) return false;

    // Disable the RX interrupts
    _intrMask = (HAL_INT)(_intrMask & (~(HAL_INT_RX | HAL_INT_RXEOL | HAL_INT_RXORN)));
    (_hal->ar5k_ar5212_set_intr)(_intrMask);

    // Disable RX process
    (_hal->ar5k_ar5212_set_rx_filter)(0);
    (_hal->ar5k_ar5212_stop_pcu_recv)();
    (_hal->ar5k_ar5212_stop_rx_dma)();
    (_hal->ar5k_ar5212_put_rx_buf)(0);
    IODelay(3000);    

    return true;
}

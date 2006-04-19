/*
        
        File:			AtheroJack.h
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
#include "WiFiControllerPCI.h"
#include "OpenHAL/OpenHAL5212.h"

#define IEEE80211_CHAN_MAX      255
#define ATH_NUM_RX_DESCS 32
#define ATH_NUM_TX_DESCS 32
#define recalibrationTimeout 30000 //in milli seconds
#define scanTimeout 250 //in milli seconds
#define watchdogTimeout 1 //in milli seconds
        
typedef struct {
    volatile struct ath_desc    d[ATH_NUM_RX_DESCS];
} desc;

typedef struct {
    desc                *p;
    pageBlock_t         page;
    IOPhysicalAddress   dmaAddress;
} controlBlock;

class AtheroJack : public WiFiControllerPCI {
    OSDeclareDefaultStructors(AtheroJack)
    
    
public:
    virtual const OSString * newModelString() const;

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

    virtual UInt32 getFrequency();
    virtual bool setFrequency(UInt32 freq); 
    virtual bool setFirmware(UInt32 length, UInt8* firmware);
	
protected:
    virtual bool _reset();
    virtual bool _quickReset();
    
    virtual bool _fillFragment(int index);
    virtual bool _allocPacketForRxDesc(int index);
    virtual bool _allocQueues();

    virtual bool _recalibration();
    virtual bool _setLedState(HAL_LED_STATE state);
    virtual bool _updateMACAddress();
    virtual bool _disableInterupts(HAL_INT ints);
    virtual bool _enableRx();
    virtual bool _disableRx();
 
    //settings
    struct opmodeSettings {
        enum _operationMode mode;
        HAL_OPMODE          halmode;
        HAL_RX_FILTER       filter;
    };
    
	OpenHAL5212				*_hal;
	
    controlBlock            _controlBlock;
    HAL_CHANNEL             _chans[IEEE80211_CHAN_MAX];
    HAL_INT                 _intrMask;
    u_int                   _nchans;
    int                     _activeOpMode;
    enum _modulationMode    _activeIEEEMode;
    static opmodeSettings   _opmodeSettings[];
    mbuf_t					_rxData[ATH_NUM_RX_DESCS];
    IOPhysicalAddress       _rxDataAddress[ATH_NUM_RX_DESCS];
    volatile struct ath_desc    *_rxListHead, *_rxListTail;
    
    UInt32                  _headRx;
    UInt32                  _tailRx;
    int                     _rxBufferSize;
    int                     _activeChannelIndex;
};

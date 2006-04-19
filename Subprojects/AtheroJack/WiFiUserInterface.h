/*
        
        File:			WiFiUserInterface.h
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

#define MAX_BSS_COUNT   20
#define MAX_SSID_LENGTH 32

#define kWiFiUserClientNotify     0xdeadbeef
#define kWiFiUserClientMap        0xdeadface

typedef enum WLUCMethods {
    kWiFiUserClientOpen,                // kIOUCScalarIScalarO, 0, 0
    kWiFiUserClientClose,               // kIOUCScalarIScalarO, 0, 0
    kWiFiUserClientGetLinkSpeed,        // kIOUCScalarIScalarO, 0, 1
    kWiFiUserClientGetConnectionState,  // kIOUCScalarIScalarO, 0, 1
    kWiFiUserClientGetFrequency,        // kIOUCScalarIScalarO, 0, 1
    kWiFiUserClientSetFrequency,        // kIOUCScalarIScalarO, 0, 1
    kWiFiUserClientSetSSID,             // kIOUCScalarIStructI, 0, 1
    kWiFiUserClientSetWEPKey,           // kIOUCScalarIStructI, 0, 1
    kWiFiUserClientGetScan,             // kIOUCScalarIStructO, 0, 1
    kWiFiUserClientSetMode,             // kIOUCScalarIScalarO, 1, 0
    kWiFiUserClientSetFirmware,         // kIOUCScalarIStructI, 0, 1
    kWiFiUserClientStartCapture,		// kIOUCScalarIScalarO, 1, 0
    kWiFiUserClientStopCapture,			// kIOUCScalarIScalarO, 0, 0
    kWiFiUserClientLastMethod,
} WLUCMethod;

typedef enum {
    stateIntializing = 0,
    stateCardEjected,
    stateSleeping,
    stateDisabled,
    stateDisconnected,
    stateDeauthenticated,
    stateDisassociated,
    stateAuthenicated,
    stateAssociated,
} wirelessState;

typedef enum {
    modeNone = 0,
    modeIBSS,
    modeClient,
    modeHostAP,
    modeMonitor,
} wirelessMode;

enum _modulationMode {
    _modulationMode80211a   = 0x01,
    _modulationMode80211b   = 0x02,
    _modulationMode80211g   = 0x04,
    _modulationModeAtherosT = 0x08
};

enum _operationMode {
    _operationModeStation   = 0x01,
    _operationModeIBSS      = 0x02,
    _operationModeHostAP    = 0x04,
    _operationModeMonitor   = 0x08,
    _operationModeInvalid   = 0x00,
};

typedef struct {
    UInt8   ssidLength;
    UInt8   ssid[MAX_SSID_LENGTH];
    UInt8   address[6];
    UInt16  cap;
    UInt8   active;
} bssItem;


struct _IEEE_HEADER {
    /* 802.11 Header Info (Little Endian) */
    UInt16 frameControl;
    UInt8  duration;
    UInt8  id;
    UInt8  address1[6];
    UInt8  address2[6];
    UInt8  address3[6];
    UInt16 sequenceControl;
    UInt8  address4[6];
} __attribute__((packed));

struct _Prism_HEADER {
    UInt16 status;
    UInt16 channel;
    UInt16 len;
    UInt8  silence;
    UInt8  signal;
    UInt8  rate;
    UInt8  rx_flow;
    UInt8  tx_rtry;
    UInt8  tx_rate;
    UInt16 txControl;
} __attribute__((packed));

//for dumping of pcap frames
struct _WLFrame {
    /* Control Fields (Little Endian) */
    struct _Prism_HEADER ph;
    struct _IEEE_HEADER ieee;
    UInt16 dataLen;

    /* 802.3 Header Info (Big Endian) */
    UInt8  dstAddr[6];
    UInt8  srcAddr[6];
    UInt16 length;
} __attribute__((packed));

typedef struct _WLFrame WLFrame;

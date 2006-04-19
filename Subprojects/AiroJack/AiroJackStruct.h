/*
        
        File:			AiroJackCard.h
        Program:		AiroJack
	Author:		 	Michael Rossberg
				mick@binaervarianz.de
	Description:		KisMAC is a wireless stumbler for MacOS X.
                
        This file is part of KisMAC.

        This file is mostly taken from the aironet linux driver.

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

#define MOD_DEFAULT 0
#define MOD_CCK 1
#define MOD_MOK 2

#define TXCTL_TXOK (1<<1) /* report if tx is ok */
#define TXCTL_TXEX (1<<2) /* report if tx fails */
#define TXCTL_802_3 (0<<3) /* 802.3 packet */
#define TXCTL_802_11 (1<<3) /* 802.11 mac packet */
#define TXCTL_ETHERNET (0<<4) /* payload has ethertype */
#define TXCTL_LLC (1<<4) /* payload is llc */
#define TXCTL_RELEASE (0<<5) /* release after completion */
#define TXCTL_NORELEASE (1<<5) /* on completion returns to host */

typedef struct {
	UInt16 len; /* sizeof(ConfigRid) */
	UInt16 opmode; /* operating mode */
	UInt16 rmode; /* receive mode */
#define RXMODE_BC_MC_ADDR 0
#define RXMODE_BC_ADDR 1 /* ignore multicasts */
#define RXMODE_ADDR 2 /* ignore multicast and broadcast */
#define RXMODE_RFMON 3 /* wireless monitor mode */
#define RXMODE_RFMON_ANYBSS 4
#define RXMODE_LANMON 5 /* lan style monitor -- data packets only */
#define RXMODE_DISABLE_802_3_HEADER (1<<8) /* disables 802.3 header on rx */
#define RXMODE_NORMALIZED_RSSI (1<<9) /* return normalized RSSI */
	UInt16 fragThresh;
	UInt16 rtsThres;
	UInt8 macAddr[6];
	UInt8 rates[8];
	UInt16 shortRetryLimit;
	UInt16 longRetryLimit;
	UInt16 txLifetime; /* in kusec */
	UInt16 rxLifetime; /* in kusec */
	UInt16 stationary;
	UInt16 ordering;
	UInt16 u16deviceType; /* for overriding device type */
	UInt16 cfpRate;
	UInt16 cfpDuration;
	UInt16 _reserved1[3];
	/*---------- Scanning/Associating ----------*/
	UInt16 scanMode;
#define SCANMODE_ACTIVE 0
#define SCANMODE_PASSIVE 1
#define SCANMODE_AIROSCAN 2
	UInt16 probeDelay; /* in kusec */
	UInt16 probeEnergyTimeout; /* in kusec */
        UInt16 probeResponseTimeout;
	UInt16 beaconListenTimeout;
	UInt16 joinNetTimeout;
	UInt16 authTimeout;
	UInt16 authType;
#define AUTH_OPEN 0x1
#define AUTH_ENCRYPT 0x101
#define AUTH_SHAREDKEY 0x102
#define AUTH_ALLOW_UNENCRYPTED 0x200
	UInt16 associationTimeout;
	UInt16 specifiedApTimeout;
	UInt16 offlineScanInterval;
	UInt16 offlineScanDuration;
	UInt16 linkLossDelay;
	UInt16 maxBeaconLostTime;
	UInt16 refreshInterval;
#define DISABLE_REFRESH 0xFFFF
	UInt16 _reserved1a[1];
	/*---------- Power save operation ----------*/
	UInt16 powerSaveMode;
#define POWERSAVE_CAM 0
#define POWERSAVE_PSP 1
#define POWERSAVE_PSPCAM 2
	UInt16 sleepForDtims;
	UInt16 listenInterval;
	UInt16 fastListenInterval;
	UInt16 listenDecay;
	UInt16 fastListenDelay;
	UInt16 _reserved2[2];
	/*---------- Ap/Ibss config items ----------*/
	UInt16 beaconPeriod;
	UInt16 atimDuration;
	UInt16 hopPeriod;
	UInt16 channelSet;
	UInt16 channel;
	UInt16 dtimPeriod;
	UInt16 bridgeDistance;
	UInt16 radioID;
	/*---------- Radio configuration ----------*/
	UInt16 radioType;
#define RADIOTYPE_DEFAULT 0
#define RADIOTYPE_802_11 1
#define RADIOTYPE_LEGACY 2
	UInt8 rxDiversity;
	UInt8 txDiversity;
	UInt16 txPower;
#define TXPOWER_DEFAULT 0
	UInt16 rssiThreshold;
#define RSSI_DEFAULT 0
        UInt16 modulation;
#define PREAMBLE_AUTO 0
#define PREAMBLE_LONG 1
#define PREAMBLE_SHORT 2
	UInt16 preamble;
	UInt16 homeProduct;
	UInt16 radioSpecific;
	/*---------- Aironet Extensions ----------*/
	UInt8 nodeName[16];
	UInt16 arlThreshold;
	UInt16 arlDecay;
	UInt16 arlDelay;
	UInt16 _reserved4[1];
	/*---------- Aironet Extensions ----------*/
	UInt16 magicAction;
#define MAGIC_ACTION_STSCHG 1
#define MACIC_ACTION_RESUME 2
#define MAGIC_IGNORE_MCAST (1<<8)
#define MAGIC_IGNORE_BCAST (1<<9)
#define MAGIC_SWITCH_TO_PSP (0<<10)
#define MAGIC_STAY_IN_CAM (1<<10)
	UInt16 magicControl;
	UInt16 autoWake;
} ConfigRid;

typedef struct {
	UInt16 len;
	UInt8 mac[6];
	UInt16 mode;
	UInt16 errorCode;
	UInt16 sigQuality;
	UInt16 SSIDlen;
	char SSID[32];
	char apName[16];
	char bssid[4][6];
	UInt16 beaconPeriod;
	UInt16 dimPeriod;
	UInt16 atimDuration;
	UInt16 hopPeriod;
	UInt16 channelSet;
	UInt16 channel;
	UInt16 hopsToBackbone;
	UInt16 apTotalLoad;
	UInt16 generatedLoad;
	UInt16 accumulatedArl;
	UInt16 signalQuality;
	UInt16 currentXmitRate;
	UInt16 apDevExtensions;
	UInt16 normalizedSignalStrength;
	UInt16 shortPreamble;
	UInt8 apIP[4];
	UInt8 noisePercent; /* Noise percent in last second */
	UInt8 noisedBm; /* Noise dBm in last second */
	UInt8 noiseAvePercent; /* Noise percent in last minute */
	UInt8 noiseAvedBm; /* Noise dBm in last minute */
	UInt8 noiseMaxPercent; /* Highest noise percent in last minute */
	UInt8 noiseMaxdBm; /* Highest noise dbm in last minute */
	UInt16 load;
	UInt8 carrier[4];
	UInt16 assocStatus;
#define STAT_NOPACKETS 0
#define STAT_NOCARRIERSET 10
#define STAT_GOTCARRIERSET 11
#define STAT_WRONGSSID 20
#define STAT_BADCHANNEL 25
#define STAT_BADBITRATES 30
#define STAT_BADPRIVACY 35
#define STAT_APFOUND 40
#define STAT_APREJECTED 50
#define STAT_AUTHENTICATING 60
#define STAT_DEAUTHENTICATED 61
#define STAT_AUTHTIMEOUT 62
#define STAT_ASSOCIATING 70
#define STAT_DEASSOCIATED 71
#define STAT_ASSOCTIMEOUT 72
#define STAT_NOTAIROAP 73
#define STAT_ASSOCIATED 80
#define STAT_LEAPING 90
#define STAT_LEAPFAILED 91
#define STAT_LEAPTIMEDOUT 92
#define STAT_LEAPCOMPLETE 93
} StatusRid;

typedef struct {
	UInt16 len;
	char oui[3];
	char zero;
	UInt16 prodNum;
	char manName[32];
	char prodName[16];
	char prodVer[8];
	char factoryAddr[6];
	char aironetAddr[6];
	UInt16 radioType;
	UInt16 country;
	char callid[6];
	char supportedRates[8];
	char rxDiversity;
	char txDiversity;
	UInt16 txPowerLevels[8];
	UInt16 hardVer;
	UInt16 hardCap;
	UInt16 tempRange;
	UInt16 softVer;
	UInt16 softSubVer;
	UInt16 interfaceVer;
	UInt16 softCap;
	UInt16 bootBlockVer;
	UInt16 requiredHard;
	UInt16 extSoftCap;
} CapabilityRid;

typedef struct {
	UInt16 len;
	UInt8 ssid[32];
} Ssid;

typedef struct {
	UInt16 len;
	Ssid ssids[3];
} SsidRid;

typedef struct {
	UInt16 len;
	UInt16 spacer;
	UInt32 vals[100];
} StatsRid;


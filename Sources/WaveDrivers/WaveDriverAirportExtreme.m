/*
        
        File:			WaveDriverAirportExtreme.m
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

#import "WaveDriverAirportExtreme.h"
#import "ImportController.h"
#import "WaveHelper.h"
#import <BIGeneric/BIGeneric.h>
#import "Apple80211.h"

static bool explicitlyLoadedAirportExtremeDriver = NO;
WirelessContextPtr gWCtxt = NULL;

@implementation WaveDriverAirportExtreme

+ (enum WaveDriverType) type {
    return passiveDriver;
}

+ (bool) allowsInjection {
    return NO;
}

+ (bool) allowsChannelHopping {
    return YES;
}

+ (NSString*) description {
    return NSLocalizedString(@"Apple Airport Extreme card, passive mode", "long driver description");
}

+ (NSString*) deviceName {
    return NSLocalizedString(@"Airport Extreme Card", "short driver description");
}

#pragma mark -


+ (BOOL)deviceAvailable {
	WaveDriverAirportExtreme *w = [[WaveDriverAirportExtreme alloc] init];
	[w release];
	
	if (w) return YES;
	return NO;
}

// return 0 for success, 1 for error, 2 for self handled error
+ (int) initBackend {
	BOOL ret;
    int x;
	
    NSUserDefaults *defs;
        
        if (NSAppKitVersionNumber < 949.00) {
            NSLog(@"MacOS is not 10.5.1! AppKitVersion: %f < 949.00", NSAppKitVersionNumber);
		
            NSRunCriticalAlertPanel(
                                NSLocalizedString(@"Could not enable Monitor Mode for Airport Extreme.", "Error dialog title"),
                                NSLocalizedString(@"Incompatible MacOS version! You will need at least MacOS 10.5.1!. Please ensure that you are running Leopard, and have updated to the latest through Software Update.", "Error dialog description"),
                                OK, nil, nil);

            return 2;
        }

	if ([WaveDriverAirportExtreme deviceAvailable]) return 0;
}

+ (bool) loadBackend {
    ImportController *importController;
    int result;
    int x;
        
    do {
        importController = [[ImportController alloc] initWithWindowNibName:@"Import"];
        [importController setTitle:[NSString stringWithFormat:NSLocalizedString(@"Loading %@...", "for Backend loading"), [self description]]];
    
        [NSApp beginSheet:[importController window] modalForWindow:[WaveHelper mainWindow] modalDelegate:nil didEndSelector:nil contextInfo:nil];
        
        result = [self initBackend];
    
        [NSApp endSheet: [importController window]];        
        [[importController window] close];
        [importController stopAnimation];
        [importController release];
        importController=Nil;
            
        if (result == 1) {	//see if we actually have the driver accessed
            x = [WaveHelper showCouldNotInstaniciateDialog:[self description]];
        }
    } while (result==1 && x==1);

    return (result==0);
}

+ (bool) unloadBackend {
	BOOL ret;
	
	[NSTask launchedTaskWithLaunchPath:@"/System/Library/PrivateFrameworks/Apple80211.framework/Resources/airport"
		arguments:[NSArray arrayWithObject:@"-a"]];

	return YES;
}

#pragma mark -

- (id)init {
	NSUserDefaults *defs;
    defs = [NSUserDefaults standardUserDefaults];
    char err[PCAP_ERRBUF_SIZE];
	
	_apeType = APExtTypeBcm;
	
	[NSTask launchedTaskWithLaunchPath:@"/System/Library/PrivateFrameworks/Apple80211.framework/Resources/airport" arguments:[NSArray arrayWithObject:@"-z"]];
	
    //pcap_open_live(char *device,int snaplen, int prmisc,int to_ms,char *ebuf)
	_device = pcap_open_live([[defs objectForKey:@"scandevice"] cString], 3000, 1, 2, err);
	if (!_device) {
		if (![[BLAuthentication sharedInstance] executeCommand:@"/usr/bin/chgrp" withArgs:[NSArray arrayWithObjects:@"admin", [defs objectForKey:@"bpfloc"], nil]]) return Nil;
		if (![[BLAuthentication sharedInstance] executeCommand:@"/bin/chmod" withArgs:[NSArray arrayWithObjects:@"0660", [defs objectForKey:@"bpfloc"], nil]]) return Nil;
		[NSThread sleep:0.5];
	
		_device = pcap_open_live([[defs objectForKey:@"scandevice"] cString], 3000, 1, 2, err);
		[[BLAuthentication sharedInstance] executeCommand:@"/usr/bin/chgrp" withArgs:[NSArray arrayWithObjects:@"wheel", [defs objectForKey:@"bpfloc"], nil]];
		[[BLAuthentication sharedInstance] executeCommand:@"/bin/chmod" withArgs:[NSArray arrayWithObjects:@"0600", [defs objectForKey:@"bpfloc"], nil]];

		if (!_device) return Nil;
    }
	pcap_set_datalink(_device, DLT_IEEE802_11_RADIO_AVS);
	
	self=[super init];
    if(!self) return Nil;

    return self;
}

#pragma mark -

- (unsigned short) getChannelUnCached {
	return _currentChannel;
}

WIErr
wlc_ioctl(int command, int bufsize, void* buffer, int outsize, void* out) {
	if (!buffer) bufsize = 0;
	int* buf = malloc(bufsize+8);
	buf[0] = 3; 
	buf[1] = command;
	if (bufsize && buffer)
		memcpy(&buf[2], buffer, bufsize);
	return WirelessPrivate(gWCtxt, buf, bufsize+8, out, outsize);
}

int chanint;

- (bool) setChannel:(unsigned short)newChannel {
	chanint = newChannel;
	WirelessAttach(&gWCtxt, 0);
	wlc_ioctl(52, 0, NULL, 0, NULL); // disassociate
	wlc_ioctl(30, 8, &chanint, 0, NULL); // set channel
	WirelessDetach(gWCtxt);
	_currentChannel = newChannel;
    return YES;
}

- (bool) startCapture:(unsigned short)newChannel {
	[self setChannel:newChannel];
    return YES;
}

-(bool) stopCapture {
    return YES;
}

#pragma mark -

// wlan-ng (and hopefully others) AVS header, version one.  Fields in
// network byte order.
typedef struct __avs_80211_1_header {
	uint32_t version;
	uint32_t length;
	uint64_t mactime;
	uint64_t hosttime;
	uint32_t phytype;
	uint32_t channel;
	uint32_t datarate;
	uint32_t antenna;
	uint32_t priority;
	uint32_t ssi_type;
	int32_t ssi_signal;
	int32_t ssi_noise;
	uint32_t preamble;
	uint32_t encoding;
} __attribute__((packed)) avs_80211_1_header;


- (int)headerLenForFrameControl:(UInt16)frameControl {
	UInt16 isToDS, isFrDS, subtype, headerLength = 0;

	UInt16 type=(frameControl & IEEE80211_TYPE_MASK);
	//depending on the frame we have to figure the length of the header
	switch(type) {
		case IEEE80211_TYPE_DATA: //Data Frames
			isToDS = ((frameControl & IEEE80211_DIR_TODS) ? YES : NO);
			isFrDS = ((frameControl & IEEE80211_DIR_FROMDS) ? YES : NO);
			if (isToDS&&isFrDS) headerLength=30; //WDS Frames are longer
			else headerLength=24;
			break;
		case IEEE80211_TYPE_CTL: //Control Frames
			subtype=(frameControl & IEEE80211_SUBTYPE_MASK);
			switch(subtype) {
				case IEEE80211_SUBTYPE_PS_POLL:
				case IEEE80211_SUBTYPE_RTS:
					headerLength=16;
					break;
				case IEEE80211_SUBTYPE_CTS:
				case IEEE80211_SUBTYPE_ACK:
					headerLength=10;
					break;
				default:
					break;
			}
			break;
		case IEEE80211_TYPE_MGT: //Management Frame
			headerLength=24;
			break;
		default:
			break;
	}
	return headerLength;
}

- (WLFrame*) nextFrame {
	struct pcap_pkthdr			header;
	const u_char				*data;
	static UInt8				frame[2500];
    WLFrame						*f;
    avs_80211_1_header			*af;
	UInt16						headerLength = 0, rthP;
	UInt8						flags;
	
	f = (WLFrame*)frame;
	
	while(YES) {
		data = pcap_next(_device, &header);
		//NSLog(@"pcap_next: data:0x%x, len:%u\n", data, header.caplen);
		if (!data) continue;
		
			if ((header.caplen - sizeof(avs_80211_1_header)) < 30) continue;
			
			memcpy(frame + sizeof(WLPrismHeader), data + sizeof(avs_80211_1_header), 30);
			
			headerLength = [self headerLenForFrameControl:f->frameControl];
			if (headerLength == 0) continue;
			
			af = (avs_80211_1_header*)data;
			f->silence = af->ssi_signal + 155;
			f->signal = af->ssi_noise;
			f->channel = af->channel;
			
			f->length = f->dataLen = header.caplen - headerLength - sizeof(avs_80211_1_header) - 4; //we dont want the fcs or do we?
			//NSLog(@"Got packet!!! hLen %u signal: %d  noise: %d channel %u length: %u\n", headerLength, af->ssi_signal, af->ssi_noise, f->channel, f->dataLen );
			memcpy(frame + sizeof(WLFrame), data + sizeof(avs_80211_1_header) + headerLength, f->dataLen);
			
        _packets++;
        return f;
    }
}

#pragma mark -

-(bool) sendFrame:(UInt8*)f withLength:(int) size atInterval:(int)interval {
    return NO;
}

-(bool) stopSendingFrames {    
    return NO;
}

#pragma mark -

-(void) dealloc {
	pcap_close(_device);
    [super dealloc];
}

@end

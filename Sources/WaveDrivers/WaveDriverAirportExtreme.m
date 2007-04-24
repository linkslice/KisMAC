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
static NSString *airportExtremeBundleID = nil;
WirelessContextPtr gWCtxt = NULL;

static NSString *kAppleAirPort2Path = @"/System/Library/Extensions/AppleAirPort2.kext/Contents/Info.plist";
static NSString *kAppleAirPortBrcm4311Path = @"/System/Library/Extensions/IO80211Family.kext/Contents/PlugIns/AppleAirPortBrcm4311.kext/Contents/Info.plist";

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

+ (BOOL)monitorModeEnabled {
	NSDictionary *dict;
	NSData *fileData;
	
	fileData = [NSData dataWithContentsOfFile:kAppleAirPort2Path];
	dict = [NSPropertyListSerialization propertyListFromData:fileData mutabilityOption:kCFPropertyListImmutable format:NULL errorDescription:Nil];
	if ([[dict valueForKeyPath:@"IOKitPersonalities.Broadcom PCI.APMonitorMode"] boolValue]) return YES;
	
	fileData = [NSData dataWithContentsOfFile:kAppleAirPortBrcm4311Path];
	dict = [NSPropertyListSerialization propertyListFromData:fileData mutabilityOption:kCFPropertyListImmutable format:NULL errorDescription:Nil];
	if ([[dict valueForKeyPath:@"IOKitPersonalities.Broadcom PCI.APMonitorMode"] boolValue] ||
		[[[[dict objectForKey:@"IOKitPersonalities"] objectForKey:@"Broadcom 802.11 PCI"] objectForKey:@"APMonitorMode"] boolValue]) return YES;
	
	return NO;
}

+ (BOOL)setMonitorMode:(BOOL)enable forFile:(NSString*)fileName {
	[NSThread sleep:1.0];
	[[BLAuthentication sharedInstance] executeCommand:@"/usr/bin/chgrp" withArgs:[NSArray arrayWithObjects:@"admin", fileName, nil]];
	[[BLAuthentication sharedInstance] executeCommand:@"/bin/chmod" withArgs:[NSArray arrayWithObjects:@"0664", fileName, nil]];
	
	[NSThread sleep:1.0];
	NSDictionary *dict = [NSPropertyListSerialization propertyListFromData:[NSData dataWithContentsOfFile:fileName] mutabilityOption:kCFPropertyListMutableContainers format:NULL errorDescription:Nil];
	if ([dict valueForKeyPath:@"IOKitPersonalities.Broadcom PCI"])
		[dict setValue:[NSNumber numberWithBool:enable] forKeyPath:@"IOKitPersonalities.Broadcom PCI.APMonitorMode"];
	if ([[dict objectForKey:@"IOKitPersonalities"] objectForKey:@"Broadcom 802.11 PCI"])
		[[[dict objectForKey:@"IOKitPersonalities"] objectForKey:@"Broadcom 802.11 PCI"] setValue:[NSNumber numberWithBool:enable] forKey:@"APMonitorMode"];
	[[NSPropertyListSerialization dataFromPropertyList:dict format:kCFPropertyListXMLFormat_v1_0 errorDescription:nil] writeToFile:fileName atomically:NO];
		
	[[BLAuthentication sharedInstance] executeCommand:@"/bin/chmod" withArgs:[NSArray arrayWithObjects:@"0644", fileName, nil]];
	[[BLAuthentication sharedInstance] executeCommand:@"/usr/bin/chgrp" withArgs:[NSArray arrayWithObjects:@"wheel", fileName, nil]];
	
	return YES;
}

+ (void)setMonitorMode:(BOOL)enable {
	NSUserDefaults *defs;
    
    defs = [NSUserDefaults standardUserDefaults];
    [WaveDriverAirportExtreme setMonitorMode:enable forFile:kAppleAirPort2Path];
	[WaveDriverAirportExtreme setMonitorMode:enable forFile:kAppleAirPortBrcm4311Path];
	
	if ([[defs objectForKey:@"aeForever"] boolValue]) {
		[[BLAuthentication sharedInstance] executeCommand:@"/bin/rm" withArgs:[NSArray arrayWithObject:@"/System/Library/Extensions.kextcache"]];
		[[BLAuthentication sharedInstance] executeCommand:@"/bin/rm" withArgs:[NSArray arrayWithObject:@"/System/Library/Extensions.mkext"]];
	}
}

// return 0 for success, 1 for error, 2 for self handled error
+ (int) initBackend {
	BOOL ret;
    int x;
	
    NSUserDefaults *defs;
    
	if([WaveHelper isServiceAvailable:"AirPort_Athr5424"]) {
		NSLog(@"User has a Atheros card.");
		return 0;
		NSRunCriticalAlertPanel(
		NSLocalizedString(@"Could not enable Monitor Mode for Airport Extreme.", "Error dialog title"),
		NSLocalizedString(@"Passive mode for Airport Extreme does not work with MacBookPros and recent Mac Minis, as they have different Airport Extreme Hardware.", "Error dialog description"),
		OK, nil, nil);
		return 2;
	}
	
	if(![WaveHelper isServiceAvailable:"AirPortPCI_MM"] && ![WaveHelper isServiceAvailable:"AirPort_Brcm43xx"]) {
		NSLog(@"User has no Broadcom card.");
		NSRunCriticalAlertPanel(
		NSLocalizedString(@"Could not enable Monitor Mode for Airport Extreme.", "Error dialog title"),
		NSLocalizedString(@"Your Airport Extreme driver has not been found.", "Error dialog description"),
		OK, nil, nil);
		return 2;
	}
	
	if([[NSFileManager defaultManager] fileExistsAtPath:@"/System/Library/Extensions/IO80211Family.kext"]) {
		NSLog(@"Enabling for new Intel Mac");
		airportExtremeBundleID = @"com.apple.driver.AirPortBrcm43xx";
	} else {
		NSLog(@"Enabling for Mac of the old school");
		airportExtremeBundleID = @"com.apple.iokit.AppleAirPort2";
	}

	
	defs = [NSUserDefaults standardUserDefaults];
    if ([WaveDriverAirportExtreme deviceAvailable]) return 0;
    if (![[defs objectForKey:@"aeForever"] boolValue]){
        NSLog(@"Loading AE Passive for this session only!");
        explicitlyLoadedAirportExtremeDriver = YES;
    
        if (NSAppKitVersionNumber < 824.11) {
            NSLog(@"MacOS is not 10.4.2! AppKitVersion: %f < 824.11", NSAppKitVersionNumber);
		
            NSRunCriticalAlertPanel(
                                NSLocalizedString(@"Could not enable Monitor Mode for Airport Extreme.", "Error dialog title"),
                                NSLocalizedString(@"Incompatible MacOS version! You will need at least MacOS 10.4.2!.", "Error dialog description"),
                                OK, nil, nil);

            return 2;
        }

        ret = [[BLAuthentication sharedInstance] executeCommand:@"/sbin/kextunload" withArgs:[NSArray arrayWithObjects:@"-b", airportExtremeBundleID, nil]];
        if (!ret) {
            NSLog(@"WARNING!!! User canceled password dialog for: kextunload");
            return 2;
        }
        [WaveDriverAirportExtreme setMonitorMode:YES];
	
        for (x=0; x<5; x++) {
            [NSThread sleep:1.0];
            [[BLAuthentication sharedInstance] executeCommand:@"/sbin/kextload" withArgs:[NSArray arrayWithObjects:@"-b", airportExtremeBundleID, nil]];
		
            if ([WaveDriverAirportExtreme deviceAvailable]) return 0;
        }
        [[BLAuthentication sharedInstance] executeCommand:@"/sbin/kextunload" withArgs:[NSArray arrayWithObjects:@"-b", airportExtremeBundleID, nil]];
        for (x=0; x<5; x++) {
            [NSThread sleep:1.0];
            [[BLAuthentication sharedInstance] executeCommand:@"/sbin/kextload" withArgs:[NSArray arrayWithObjects:@"-b", airportExtremeBundleID, nil]];
    
            if ([WaveDriverAirportExtreme deviceAvailable]) return 0;
        }
	
        [WaveDriverAirportExtreme setMonitorMode:NO];
    }
	NSLog(@"Could not enable monitor mode for Airport Extreme.");
	NSRunCriticalAlertPanel(
		NSLocalizedString(@"Could not enable Monitor Mode for Airport Extreme.", "Error dialog title"),
		NSLocalizedString(@"Could not load Monitor Mode for Airport Extreme. Drivers were not found.  If you just enabled persistent passive support, you must reboot. Please note that passive mode does not work with MacBookPros and recent Mac Minis.", "Error dialog description"),
		OK, nil, nil);
	
	return 2;
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
	
    if (explicitlyLoadedAirportExtremeDriver) {
		ret = [[BLAuthentication sharedInstance] executeCommand:@"/sbin/kextunload" withArgs:[NSArray arrayWithObjects:@"-b", airportExtremeBundleID, nil]];
		if (!ret) {
			NSLog(@"WARNING!!! User canceled password dialog for: kextunload");
			return NO;
		}
		
		[WaveDriverAirportExtreme setMonitorMode:NO];
		[[BLAuthentication sharedInstance] executeCommand:@"/sbin/kextload" withArgs:[NSArray arrayWithObjects:@"-b", airportExtremeBundleID, nil]];

		[NSThread sleep:1.0];
	}
	
	[NSTask launchedTaskWithLaunchPath:@"/System/Library/PrivateFrameworks/Apple80211.framework/Resources/airport"
		arguments:[NSArray arrayWithObject:@"-a"]];

	return YES;
}

#pragma mark -

- (id)init {
	NSUserDefaults *defs;
    defs = [NSUserDefaults standardUserDefaults];
    char err[PCAP_ERRBUF_SIZE];
	
	if([WaveHelper isServiceAvailable:"AirPort_Athr5424"]) {
		_apeType = APExtTypeAth5414;
	} else if([WaveHelper isServiceAvailable:"AirPortPCI_MM"] || [WaveHelper isServiceAvailable:"AirPort_Brcm43xx"]) {
		_apeType = APExtTypeBcm;
	} else {
		_apeType = APExtTypeUnknown;
	}
	
    //pcap_open_live(char *device,int snaplen, int prmisc,int to_ms,char *ebuf)
	_device = pcap_open_live([[defs objectForKey:@"bpfdevice"] cString], 3000, 1, 2, err);
	if (!_device) {
		if (![[BLAuthentication sharedInstance] executeCommand:@"/usr/bin/chgrp" withArgs:[NSArray arrayWithObjects:@"admin", [defs objectForKey:@"bpfloc"], nil]]) return Nil;
		if (![[BLAuthentication sharedInstance] executeCommand:@"/bin/chmod" withArgs:[NSArray arrayWithObjects:@"0660", [defs objectForKey:@"bpfloc"], nil]]) return Nil;
		[NSThread sleep:0.5];
	
		_device = pcap_open_live([[defs objectForKey:@"bpfdevice"] cString], 3000, 1, 2, err);
		[[BLAuthentication sharedInstance] executeCommand:@"/usr/bin/chgrp" withArgs:[NSArray arrayWithObjects:@"wheel", [defs objectForKey:@"bpfloc"], nil]];
		[[BLAuthentication sharedInstance] executeCommand:@"/bin/chmod" withArgs:[NSArray arrayWithObjects:@"0600", [defs objectForKey:@"bpfloc"], nil]];

		if (!_device) return Nil;
    }
	
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

/* The radio capture header precedes the 802.11 header. */
typedef struct __ieee80211_radiotap_header {
	u_int8_t	it_version;	/* Version 0. Only increases
					 * for drastic changes,
					 * introduction of compatible
					 * new fields does not count.
					 */
	u_int8_t	it_pad;
	u_int16_t   it_len;         /* length of the whole
					 * header in bytes, including
					 * it_version, it_pad,
					 * it_len, and data fields.
					 */
	u_int32_t    it_present;     /* A bitmap telling which
					 * fields are present. Set bit 31
					 * (0x80000000) to extend the
					 * bitmap by another 32 bits.
					 * Additional extensions are made
					 * by setting bit 31.
					 */
} __attribute__((__packed__)) ieee80211_radiotap_header;

enum ieee80211_radiotap_type {
	IEEE80211_RADIOTAP_TSFT = 0,
	IEEE80211_RADIOTAP_FLAGS = 1,
	IEEE80211_RADIOTAP_RATE = 2,
	IEEE80211_RADIOTAP_CHANNEL = 3,
	IEEE80211_RADIOTAP_FHSS = 4,
	IEEE80211_RADIOTAP_DBM_ANTSIGNAL = 5,
	IEEE80211_RADIOTAP_DBM_ANTNOISE = 6,
	IEEE80211_RADIOTAP_LOCK_QUALITY = 7,
	IEEE80211_RADIOTAP_TX_ATTENUATION = 8,
	IEEE80211_RADIOTAP_DB_TX_ATTENUATION = 9,
	IEEE80211_RADIOTAP_DBM_TX_POWER = 10,
	IEEE80211_RADIOTAP_ANTENNA = 11,
	IEEE80211_RADIOTAP_DB_ANTSIGNAL = 12,
	IEEE80211_RADIOTAP_DB_ANTNOISE = 13,
	IEEE80211_RADIOTAP_EXT = 31
};

/* For IEEE80211_RADIOTAP_FLAGS */
#define	IEEE80211_RADIOTAP_F_CFP	0x01	/* sent/received
						 * during CFP
						 */
#define	IEEE80211_RADIOTAP_F_SHORTPRE	0x02	/* sent/received
						 * with short
						 * preamble
						 */
#define	IEEE80211_RADIOTAP_F_WEP	0x04	/* sent/received
						 * with WEP encryption
						 */
#define	IEEE80211_RADIOTAP_F_FRAG	0x08	/* sent/received
						 * with fragmentation
						 */
#define	IEEE80211_RADIOTAP_F_FCS	0x10	/* frame includes FCS */
#define	IEEE80211_RADIOTAP_F_DATAPAD	0x20	/* frame has padding between
						 * 802.11 header and payload
						 * (to 32-bit boundary)
						 */
#define	IEEE80211_RADIOTAP_F_BADFCS	0x40	/* does not pass FCS check */

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
	ieee80211_radiotap_header	*rth;
	UInt16						headerLength = 0, rthP;
	UInt8						flags;
	
	f = (WLFrame*)frame;
	
	while(YES) {
		data = pcap_next(_device, &header);
		//NSLog(@"pcap_next: data:0x%x, len:%u\n", data, header.caplen);
		if (!data) continue;
		
		if(_apeType == APExtTypeBcm) {
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
        } else {
			if((header.caplen - sizeof(ieee80211_radiotap_header)) < 30) continue;
			rth = (ieee80211_radiotap_header*)data;
			
			if(rth->it_version != 0) continue;
			if ((header.caplen - rth->it_len) < 30) continue;
			
			memcpy(frame + sizeof(WLPrismHeader), data + rth->it_len, 30);
			headerLength = [self headerLenForFrameControl:f->frameControl];
			if (headerLength == 0) continue;
			
			if((rth->it_present & (1 << IEEE80211_RADIOTAP_EXT)) == 0) { //we cannot process extended headers
				rthP = sizeof(ieee80211_radiotap_header);
				if(rth->it_present & (1 << IEEE80211_RADIOTAP_TSFT)) {
					rthP += 8;
				} 
				if(rth->it_present & (1 << IEEE80211_RADIOTAP_FLAGS)) {
					memcpy(&flags, data + rthP, 1);
					if(flags & IEEE80211_RADIOTAP_F_BADFCS) continue;
					
					rthP += 1;
				} 
				if(rth->it_present & (1 << IEEE80211_RADIOTAP_RATE)) {
					rthP += 1;
				}
				if(rth->it_present & (1 << IEEE80211_RADIOTAP_CHANNEL)) {
					memcpy(&f->channel, data + rthP , 2);
					if(f->channel == 2484) f->channel = 14;
					else if(f->channel >= 2412 && f->channel < 3000) f->channel = (f->channel - 2412) / 5 + 1;
					else f->channel = 16;
					rthP += 4;
				} 	
				if(rth->it_present & (1 << IEEE80211_RADIOTAP_FHSS)) {
					rthP += 2;
				} 
				if(rth->it_present & (1 << IEEE80211_RADIOTAP_DBM_ANTSIGNAL)) {
					memcpy(&f->silence, data + rthP, 1);
					rthP += 1;
				}				
				if(rth->it_present & (1 << IEEE80211_RADIOTAP_DBM_ANTNOISE)) {
					memcpy(&f->signal, data + rthP, 1);
					rthP += 1;
				}				
				if(rth->it_present & (1 << IEEE80211_RADIOTAP_LOCK_QUALITY)) {
					rthP += 2;
				} 
				if(rth->it_present & (1 << IEEE80211_RADIOTAP_TX_ATTENUATION)) {
					rthP += 2;
				} 
				if(rth->it_present & (1 << IEEE80211_RADIOTAP_DB_TX_ATTENUATION)) {
					rthP += 2;
				} 
				if(rth->it_present & (1 << IEEE80211_RADIOTAP_DBM_TX_POWER)) {
					rthP += 1;
				} 
				if(rth->it_present & (1 << IEEE80211_RADIOTAP_ANTENNA)) {
					rthP += 1;
				}
				if(rth->it_present & (1 << IEEE80211_RADIOTAP_DB_ANTSIGNAL)) {
					memcpy(&f->silence, data + rthP, 1);
					rthP += 1;
				}				
				if(rth->it_present & (1 << IEEE80211_RADIOTAP_DB_ANTNOISE)) {
					memcpy(&f->signal, data + rthP, 1);
					rthP += 1;
				}							
			}
			
			f->length = f->dataLen = header.caplen - headerLength - rth->it_len - (flags & IEEE80211_RADIOTAP_F_FCS ? 4 : 0); //we dont want the fcs or do we?
			//NSLog(@"Got packet!!! hLen %u signal: %d  noise: %d channel %u length: %u\n", headerLength, af->ssi_signal, af->ssi_noise, f->channel, f->dataLen );
			memcpy(frame + sizeof(WLFrame), data +  rth->it_len + headerLength, f->dataLen);
		
		}
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

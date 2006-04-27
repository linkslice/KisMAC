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

static bool explicitlyLoadedAirportExtremeDriver = NO;

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
	
	fileData = [NSData dataWithContentsOfFile:@"/System/Library/Extensions/AppleAirPort2.kext/Contents/Info.plist"];
	dict = [NSPropertyListSerialization propertyListFromData:fileData mutabilityOption:kCFPropertyListImmutable format:NULL errorDescription:Nil];
	if ([[dict valueForKeyPath:@"IOKitPersonalities.Broadcom PCI.APMonitorMode"] boolValue]) return YES;
	
	fileData = [NSData dataWithContentsOfFile:@"/System/Library/Extensions/IO80211Family.kext/Contents/PlugIns/AppleAirPortBrcm4311.kext/Contents/Info.plist"];
	dict = [NSPropertyListSerialization propertyListFromData:fileData mutabilityOption:kCFPropertyListImmutable format:NULL errorDescription:Nil];
	if ([[dict valueForKeyPath:@"IOKitPersonalities.Broadcom PCI.APMonitorMode"] boolValue]) return YES;
	
	return NO;
}

+ (BOOL)setMonitorMode:(BOOL)enable forFile:(NSString*)fileName {
	[NSThread sleep:1.0];
	[[BLAuthentication sharedInstance] executeCommand:@"/usr/bin/chgrp" withArgs:[NSArray arrayWithObjects:@"admin", fileName, nil]];
	[[BLAuthentication sharedInstance] executeCommand:@"/bin/chmod" withArgs:[NSArray arrayWithObjects:@"0664", fileName, nil]];
	
	[NSThread sleep:1.0];
	NSDictionary *dict = [NSPropertyListSerialization propertyListFromData:[NSData dataWithContentsOfFile:fileName] mutabilityOption:kCFPropertyListMutableContainers format:NULL errorDescription:Nil];
	[dict setValue:[NSNumber numberWithBool:enable] forKeyPath:@"IOKitPersonalities.Broadcom PCI.APMonitorMode"];
	[[NSPropertyListSerialization dataFromPropertyList:dict format:kCFPropertyListXMLFormat_v1_0 errorDescription:nil] writeToFile:fileName atomically:NO];
		
	[[BLAuthentication sharedInstance] executeCommand:@"/bin/chmod" withArgs:[NSArray arrayWithObjects:@"0644", fileName, nil]];
	[[BLAuthentication sharedInstance] executeCommand:@"/usr/bin/chgrp" withArgs:[NSArray arrayWithObjects:@"wheel", fileName, nil]];
	
	return YES;
}

+ (void)setMonitorMode:(BOOL)enable {
	NSUserDefaults *defs;
    
    defs = [NSUserDefaults standardUserDefaults];
    [WaveDriverAirportExtreme setMonitorMode:enable forFile:@"/System/Library/Extensions/AppleAirPort2.kext/Contents/Info.plist"];
	[WaveDriverAirportExtreme setMonitorMode:enable forFile:@"/System/Library/Extensions/IO80211Family.kext/Contents/PlugIns/AppleAirPortBrcm4311.kext/Contents/Info.plist"];
	
	if ([[defs objectForKey:@"aeForever"] boolValue]) {
		[[BLAuthentication sharedInstance] executeCommand:@"/bin/rm" withArgs:[NSArray arrayWithObject:@"/System/Library/Extensions.kextcache"]];
		[[BLAuthentication sharedInstance] executeCommand:@"/bin/rm" withArgs:[NSArray arrayWithObject:@"/System/Library/Extensions.mkext"]];
	}
}

// return 0 for success, 1 for error, 2 for self handled error
+ (int) initBackend {
	BOOL ret;
    int x;
    NSString *kextFile;
	
    NSUserDefaults *defs;
    
	if([WaveHelper isServiceAvailable:"AirPort_Athr5424"]) {
		NSLog(@"User has a Atheros card.");
		NSRunCriticalAlertPanel(
		NSLocalizedString(@"Could not enable Monitor Mode for Airport Extreme.", "Error dialog title"),
		NSLocalizedString(@"Passive mode for Airport Extreme does not work with MacBookPros and recent Mac Minis, as they have different Airport Extreme Hardware.", "Error dialog description"),
		OK, nil, nil);
		return 2;
	}
	
	if(![WaveHelper isServiceAvailable:"AirPortPCI_MM"]) {
		NSLog(@"User has no Broadcom card.");
		NSRunCriticalAlertPanel(
		NSLocalizedString(@"Could not enable Monitor Mode for Airport Extreme.", "Error dialog title"),
		NSLocalizedString(@"Your Airport Extreme driver has not been found.", "Error dialog description"),
		OK, nil, nil);
		return 2;
	}
	
	if([[NSFileManager defaultManager] fileExistsAtPath:@"/System/Library/Extensions/IO80211Family.kext"]) {
		kextFile = @"/System/Library/Extensions/IO80211Family.kext/Contents/PlugIns/AppleAirPortBrcm4311.kext";
	} else {
		kextFile = @"/System/Library/Extensions/AppleAirPort2.kext";
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

        ret = [[BLAuthentication sharedInstance] executeCommand:@"/sbin/kextunload" withArgs:[NSArray arrayWithObjects:@"-b", @"com.apple.iokit.AppleAirPort2", nil]];
        if (!ret) {
            NSLog(@"WARNING!!! User canceled password dialog for: kextunload");
            return 2;
        }
        [WaveDriverAirportExtreme setMonitorMode:YES];
	
        for (x=0; x<5; x++) {
            [NSThread sleep:1.0];
            [[BLAuthentication sharedInstance] executeCommand:@"/sbin/kextload" withArgs:[NSArray arrayWithObject:kextFile]];
		
            if ([WaveDriverAirportExtreme deviceAvailable]) return 0;
        }
        [[BLAuthentication sharedInstance] executeCommand:@"/sbin/kextunload" withArgs:[NSArray arrayWithObjects:@"-b", @"com.apple.iokit.AppleAirPort2", nil]];
        for (x=0; x<5; x++) {
            [NSThread sleep:1.0];
            [[BLAuthentication sharedInstance] executeCommand:@"/sbin/kextload" withArgs:[NSArray arrayWithObject:kextFile]];
    
            if ([WaveDriverAirportExtreme deviceAvailable]) return 0;
        }
	
        [WaveDriverAirportExtreme setMonitorMode:NO];
    }
	NSLog(@"Could not enable monitor mode for Airport Extreme.");
	NSRunCriticalAlertPanel(
		NSLocalizedString(@"Could not enable Monitor Mode for Airport Extreme.", "Error dialog title"),
		NSLocalizedString(@"Could not load Monitor Mode for Airport Extreme. Drivers were not found.  If you just enabled Monitor Mode permanently, you must reboot. Please note that passive mode does not work with MacBookPros and recent Mac Minis.", "Error dialog description"),
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
	NSString *kextFile;
	
    if (explicitlyLoadedAirportExtremeDriver) {
		ret = [[BLAuthentication sharedInstance] executeCommand:@"/sbin/kextunload" withArgs:[NSArray arrayWithObjects:@"-b", @"com.apple.iokit.AppleAirPort2", nil]];
		if (!ret) {
			NSLog(@"WARNING!!! User canceled password dialog for: kextunload");
			return NO;
		}
		if([[NSFileManager defaultManager] fileExistsAtPath:@"/System/Library/Extensions/IO80211Family.kext"]) {
			kextFile = @"/System/Library/Extensions/IO80211Family.kext/Contents/PlugIns/AppleAirPortBrcm4311.kext";
		} else {
			kextFile = @"/System/Library/Extensions/AppleAirPort2.kext";
		}
		[WaveDriverAirportExtreme setMonitorMode:NO];
		[[BLAuthentication sharedInstance] executeCommand:@"/sbin/kextload" withArgs:[NSArray arrayWithObject:kextFile]];

		[NSThread sleep:1.0];
	}
	
	[[NSTask launchedTaskWithLaunchPath:@"/System/Library/PrivateFrameworks/Apple80211.framework/Resources/airport"
		arguments:[NSArray arrayWithObject:@"-a"]] waitUntilExit];

	return YES;
}

#pragma mark -

- (id)init {
	NSUserDefaults *defs;
    defs = [NSUserDefaults standardUserDefaults];
    char err[PCAP_ERRBUF_SIZE];
	
	if (![[BLAuthentication sharedInstance] executeCommand:@"/usr/bin/chgrp" withArgs:[NSArray arrayWithObjects:@"admin", [defs objectForKey:@"bpfloc"], nil]]) return Nil;
	if (![[BLAuthentication sharedInstance] executeCommand:@"/bin/chmod" withArgs:[NSArray arrayWithObjects:@"0660", [defs objectForKey:@"bpfloc"], nil]]) return Nil;
	[NSThread sleep:0.5];
	
	_device = pcap_open_live([[defs objectForKey:@"bpfdevice"] cString], 3000, 0, 2, err);
	[[BLAuthentication sharedInstance] executeCommand:@"/usr/bin/chgrp" withArgs:[NSArray arrayWithObjects:@"admin", [defs objectForKey:@"bpfloc"], nil]];
	[[BLAuthentication sharedInstance] executeCommand:@"/bin/chmod" withArgs:[NSArray arrayWithObjects:@"0660", [defs objectForKey:@"bpfloc"], nil]];

	if (!_device) return Nil;
    
	self=[super init];
    if(!self) return Nil;

    return self;
}

#pragma mark -

- (unsigned short) getChannelUnCached {
	return _currentChannel;
}

- (bool) setChannel:(unsigned short)newChannel {
   [[NSTask launchedTaskWithLaunchPath:@"/System/Library/PrivateFrameworks/Apple80211.framework/Resources/airport"
		arguments:[NSArray arrayWithObjects:@"-z", [NSString stringWithFormat:@"--channel=%u", newChannel], nil]] waitUntilExit];
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

- (WLFrame*) nextFrame {
	struct pcap_pkthdr header;
	const u_char *data;
	static UInt8 frame[2500];
    WLFrame *f;
    avs_80211_1_header *af;
	UInt16 isToDS, isFrDS, subtype, headerLength;
 
	f = (WLFrame*)frame;
	
	while(YES) {
		data = pcap_next(_device, &header);
		//NSLog(@"pcap_next: data:0x%x, len:%u\n", data, header.caplen);
		if (!data) continue;
		if ((header.caplen - sizeof(avs_80211_1_header)) < 30) continue;
		
		memcpy(frame + sizeof(WLPrismHeader), data + sizeof(avs_80211_1_header), 30);
		
        UInt16 type=(f->frameControl & IEEE80211_TYPE_MASK);
        //depending on the frame we have to figure the length of the header
        switch(type) {
            case IEEE80211_TYPE_DATA: //Data Frames
                isToDS = ((f->frameControl & IEEE80211_DIR_TODS) ? YES : NO);
                isFrDS = ((f->frameControl & IEEE80211_DIR_FROMDS) ? YES : NO);
                if (isToDS&&isFrDS) headerLength=30; //WDS Frames are longer
                else headerLength=24;
                break;
            case IEEE80211_TYPE_CTL: //Control Frames
                subtype=(f->frameControl & IEEE80211_SUBTYPE_MASK);
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
                        continue;
                }
                break;
            case IEEE80211_TYPE_MGT: //Management Frame
                headerLength=24;
                break;
            default:
                continue;
        }
        
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

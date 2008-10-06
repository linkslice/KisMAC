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

//stolen from kismet
// Hack around some headers that don't seem to define all of these
#ifndef IEEE80211_CHAN_TURBO
#define IEEE80211_CHAN_TURBO    0x0010  /* Turbo channel */
#endif
#ifndef IEEE80211_CHAN_CCK
#define IEEE80211_CHAN_CCK      0x0020  /* CCK channel */ 
#endif
#ifndef IEEE80211_CHAN_OFDM
#define IEEE80211_CHAN_OFDM     0x0040  /* OFDM channel */
#endif
#ifndef IEEE80211_CHAN_2GHZ
#define IEEE80211_CHAN_2GHZ     0x0080  /* 2 GHz spectrum channel. */
#endif
#ifndef IEEE80211_CHAN_5GHZ
#define IEEE80211_CHAN_5GHZ     0x0100  /* 5 GHz spectrum channel */
#endif
#ifndef IEEE80211_CHAN_PASSIVE
#define IEEE80211_CHAN_PASSIVE  0x0200  /* Only passive scan allowed */
#endif
#ifndef IEEE80211_CHAN_DYN
#define IEEE80211_CHAN_DYN      0x0400  /* Dynamic CCK-OFDM channel */
#endif
#ifndef IEEE80211_CHAN_GFSK
#define IEEE80211_CHAN_GFSK     0x0800  /* GFSK channel (FHSS PHY) */
#endif

WirelessContextPtr gWCtxt = NULL;
//defines for ioctls
#define WLC_IOCTL_MAGIC         0x14e46c77
	 
#define WLC_GET_MAGIC                           0
#define WLC_GET_BSSID                           23
#define WLC_SET_BSSID                           24
#define WLC_GET_SSID                            25
#define WLC_SET_SSID                            26
#define WLC_GET_CHANNEL                         29
#define WLC_SET_CHANNEL                         30
#define WLC_GET_MONITOR                         107     /* discovered by nbd */
#define WLC_SET_MONITOR                         108     /* discovered by nbd */
#define WLC_GET_AP                              117
#define WLC_SET_AP                              118
#define WLC_GET_RSSI                            127
#define WLC_GET_ASSOCLIST                       159

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

//todo fixme!! this is really ugly, lets do this differently
+ (BOOL)deviceAvailable 
{   
	WaveDriverAirportExtreme *w = [[WaveDriverAirportExtreme alloc] init];
	[w release];
	
	if (w) return YES;
	return NO;
}

// return 0 for success, 1 for error, 2 for self handled error
+ (int) initBackend {
	int ret;
    
        if (NSAppKitVersionNumber < 949.00) 
        {
            NSLog(@"MacOS is not 10.5.1! AppKitVersion: %f < 949.00", NSAppKitVersionNumber);
		
            NSRunCriticalAlertPanel(
                                NSLocalizedString(@"Could not enable Monitor Mode for Airport Extreme.", "Error dialog title"),
                                NSLocalizedString(@"Incompatible MacOS version! You will need at least MacOS 10.5.1!. Please ensure that you are running Leopard, and have updated to the latest through Software Update.", "Error dialog description"),
                                OK, nil, nil);

            ret = 2;
        }

	if ([WaveDriverAirportExtreme deviceAvailable]) ret = 0;
    
    return ret;
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
	
	[NSTask launchedTaskWithLaunchPath:@"/System/Library/PrivateFrameworks/Apple80211.framework/Resources/airport"
		arguments:[NSArray arrayWithObject:@"-a"]];

	return YES;
}

#pragma mark -
pcap_dumper_t * dumper;
- (id)init 
{
	NSUserDefaults *defs;
    defs = [NSUserDefaults standardUserDefaults];
    char err[PCAP_ERRBUF_SIZE];
    int retErr;
    BOOL shouldPlayback;
//    int dataLinks[] = {DLT_PRISM_HEADER, DLT_IEEE802_11, DLT_IEEE802_11_RADIO_AVS, DLT_IEEE802_11_RADIO};
    int dataLinks[] = {DLT_PRISM_HEADER, DLT_IEEE802_11_RADIO_AVS, DLT_IEEE802_11_RADIO};
    int i;
	_apeType = APExtTypeBcm;
    
    shouldPlayback = [[defs objectForKey: @"playback-rawdump"] boolValue];
	
    if( [[defs objectForKey: @"disassociateOnScan"] boolValue] && !shouldPlayback )
    {
        [NSTask launchedTaskWithLaunchPath:@"/System/Library/PrivateFrameworks/Apple80211.framework/Resources/airport" arguments:[NSArray arrayWithObject:@"-z"]];
    }
	
    if(shouldPlayback) _device = pcap_open_offline([[defs objectForKey: @"rawDumpInFile"] UTF8String], err);
	else               _device = pcap_open_live([[defs objectForKey:@"scandevice"] UTF8String], 3000, 1, 2, err);
    //todo fixme!! if we are playing back, this will be weird
	if (!_device && !shouldPlayback)
    {
		if (![[BLAuthentication sharedInstance] executeCommand:@"/usr/bin/chgrp" withArgs:[NSArray arrayWithObjects:@"admin", [defs objectForKey:@"bpfloc"], nil]]) return Nil;
		if (![[BLAuthentication sharedInstance] executeCommand:@"/bin/chmod" withArgs:[NSArray arrayWithObjects:@"0777", [defs objectForKey:@"bpfloc"], nil]]) return Nil;
		[NSThread sleep:0.5];
	
		_device = pcap_open_live([[defs objectForKey:@"scandevice"] UTF8String], 3000, 1, 2, err);
        
		[[BLAuthentication sharedInstance] executeCommand:@"/usr/bin/chgrp" withArgs:[NSArray arrayWithObjects:@"wheel", [defs objectForKey:@"bpfloc"], nil]];
		[[BLAuthentication sharedInstance] executeCommand:@"/bin/chmod" withArgs:[NSArray arrayWithObjects:@"0600", [defs objectForKey:@"bpfloc"], nil]];

		if (!_device) return Nil;
    }
    
    if(shouldPlayback)
    {
        DLTType = [[defs objectForKey: @"playback-rawdump-dlt"] intValue];
        NSLog(@"err returned from pcap open: %s", err);
        retErr = 0;
    }
    else
    {
        i = 0;
        retErr = -1;
        while ((retErr != 0) && (dataLinks[i] != 0)) 
        {
            retErr = pcap_set_datalink(_device, dataLinks[i]);
            DLTType = dataLinks[i];
            i++;
        };
    } 
    
    if( [[defs objectForKey: @"rawdump"] boolValue] )
    {
        if(_device) dumper = pcap_dump_open(_device, [[defs objectForKey: @"rawDumpOutFile"] UTF8String]);
        else NSLog(@"couldn't open dumper");
    }
    
    if(retErr != 0)
    {
        NSLog(@"Error opening airpot device using pcap_set_datalink()");
        return Nil;
    }
    
    channelEnforceTimer = nil;

	self=[super init];
    if(!self) return Nil;

    return self;
}

#pragma mark -

- (unsigned short) getChannelUnCached 
{
    int channel = 0;
  
    //this is an elaborate plan to get the channel 
    //from the airport driver.  Unfortunately switching
    //in and out of monitor mode takes forever
    //instead we work around the problem with the 
    //enforce channel timer
    #if 0
        int success;
        //this only seems to work if we reset the dlt :(
        //clear the dlt
        pcap_set_datalink(_device, 1);
        WirelessAttach(&gWCtxt, 0);
        wlc_ioctl(52, 0, NULL, 0, NULL); // disassociate
        success = wlc_ioctl(WLC_GET_CHANNEL, 0, NULL, 8, &channel);
        WirelessDetach(gWCtxt);    
        //restore the dlt
        pcap_set_datalink(_device, DLTType);
    #else
        channel = _currentChannel;
    #endif
    
    //NSLog(@"Airport Extreme getChannel: %d, success %d", channel, success );
    
	return (UInt16)channel;
}

WIErr wlc_ioctl(int command, int bufsize, void* buffer, int outsize, void* out) {
	if (!buffer) bufsize = 0;
	int* buf = malloc(bufsize+8);
	buf[0] = 3; 
	buf[1] = command;
	if (bufsize && buffer)
		memcpy(&buf[2], buffer, bufsize);
	return WirelessPrivate(gWCtxt, buf, bufsize+8, out, outsize);
}



- (bool) setChannel:(unsigned short)newChannel 
{
    int chanint;
	chanint = newChannel;
    int error;
      
    WirelessAttach(&gWCtxt, 0);
    wlc_ioctl(52, 0, NULL, 0, NULL); // disassociate
    error = wlc_ioctl(WLC_SET_CHANNEL, 8, &chanint, 0, NULL); //set channel       
    WirelessDetach(gWCtxt);
    
    //only update it if the ioctl was successful
    if(!error) _currentChannel = newChannel;
    
    //unfortunately the apple driver can not correctly get the channel when in monitor mode
    //also, the channel can change behind our back for many reasons
    //therefore we must enforce the channel periodically to be sure we are on the
    //channel we want to be on.  This should only really affect single channel scans
    [channelEnforceTimer invalidate];
    channelEnforceTimer = nil;
    channelEnforceTimer = [NSTimer scheduledTimerWithTimeInterval: 1 target: self 
                                                         selector:@selector(enforceChannel) userInfo: nil repeats: NO];

    return YES;
}

-(void)enforceChannel
{
    [self setChannel: _currentChannel];
}

- (bool) startCapture:(unsigned short)newChannel
{
    bool success = FALSE;
    int monitor = 1;
    
    WirelessAttach(&gWCtxt, 0);
    wlc_ioctl(52, 0, NULL, 0, NULL); // disassociate
    success = wlc_ioctl(WLC_SET_MONITOR, 8, &monitor, 0, NULL); //set channel
    if(success)
    {
        success = wlc_ioctl(WLC_GET_MONITOR, 0, NULL, 8, &monitor);
    }
    WirelessDetach(gWCtxt);
    
    //set dlt
    pcap_set_datalink(_device, DLTType);
    
	[self setChannel: newChannel];
    return (success && monitor);
}

-(bool) stopCapture
{
    bool success = FALSE;
    int monitor = 0;
    
    //restore dlt
    pcap_set_datalink(_device, 1);
    
    WirelessAttach(&gWCtxt, 0);
    wlc_ioctl(52, 0, NULL, 0, NULL); // disassociate
    success = wlc_ioctl(WLC_SET_MONITOR, 8, &monitor, 0, NULL); //set channel
    if(success)
    {
        success = wlc_ioctl(WLC_GET_MONITOR, 0, NULL, 8, &monitor);
    }
    WirelessDetach(gWCtxt);
    
    return (success && !monitor);
}

#pragma mark -

// wlan-ng (and hopefully others) AVS header, version one.  Fields in
// network byte order.
typedef struct __avs_80211_1_header 
{
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
} __attribute__((__packed__)) avs_80211_1_header;

typedef struct __ieee80211_radiotap_header
{
    u_int8_t        it_version;     /* set to 0 */
    u_int8_t        it_pad;
    u_int16_t       it_len;         /* entire length */
    u_int32_t       it_present;     /* fields present */
} __attribute__((__packed__)) ieee80211_radiotap_header;

//radiotap field types
#define IEEE80211_RADIOTAP_TSFT_BIT      0
#define IEEE80211_RADIOTAP_TSFT_BYTES    8
   
#define IEEE80211_RADIOTAP_FLAGS_BIT     1
#define IEEE80211_RADIOTAP_FLAGS_BYTES   1

#define IEEE80211_RADIOTAP_RATE_BIT      2
#define IEEE80211_RADIOTAP_RATE_BYTES    1

#define IEEE80211_RADIOTAP_CHANNEL_BIT   3
#define IEEE80211_RADIOTAP_CHANNEL_BYTES 4

#define IEEE80211_RADIOTAP_DBM_TX_POWER_BIT   10
#define IEEE80211_RADIOTAP_DBM_TX_POWER_BYTES 1

#define IEEE80211_RADIOTAP_ANT_BIT       11
#define IEEE80211_RADIOTAP_ANT_BYTES     1

#define IEEE80211_RADIOTAP_DBANTSIG_BIT       12
#define IEEE80211_RADIOTAP_DBANTSIG_BYTES     1

//stolen from kismet
//todo fixme!!
/*
 * Convert MHz frequency to IEEE channel number.
 */
static u_int ieee80211_mhz2ieee(u_int freq, u_int flags) {
    if (flags & IEEE80211_CHAN_2GHZ) {		/* 2GHz band */
        if (freq == 2484)
            return 14;
        if (freq < 2484)
            return (freq - 2407) / 5;
        else
            return 15 + ((freq - 2512) / 20);
    } else if (flags & IEEE80211_CHAN_5GHZ) {	/* 5Ghz band */
        return (freq - 5000) / 5;
    } else {					/* either, guess */
        if (freq == 2484)
            return 14;
        if (freq < 2484)
            return (freq - 2407) / 5;
        if (freq < 5000)
            return 15 + ((freq - 2512) / 20);
        return (freq - 5000) / 5;
    }
}


- (KFrame*) nextFrame 
{
	struct pcap_pkthdr			header;
	const u_char				*data;
	static UInt8				frame[2500];
    KFrame						*f;
    avs_80211_1_header			*af;
    ieee80211_radiotap_header   *rtf;
    UInt16 rtHeaderLength = 0;
    UInt16 dataLen = 0;
    UInt32 rtFieldsPresent;
    UInt32 rtBit;
    UInt8 * rtDataPointer = nil;
    static UInt32 count = 0;
    
	f = (KFrame *)frame;
    //NSLog(@"DLT %d", DLTType);
    
	while(YES)
    {      
		data = pcap_next(_device, &header);
        
        if(data && dumper)
        {
            pcap_dump((unsigned char*)dumper, &header, (u_char*)data);
            pcap_dump_flush(dumper); 
        }

      /*  err = pcap_inject(_device, data,  header.caplen);
        if(err) 
        {
            NSLog(@"Couldn't inject frame :(");
            pcap_perror(_device, "PCAP ERROR:");
        }*/
        
		//NSLog(@"pcap_next: data:0x%x, len:%u\n", data, header.caplen);
		if (!data) continue;
        
        count++;
        //NSLog(@"COUnt: %u", count);

        switch(DLTType)
        {
            case DLT_IEEE802_11_RADIO:
                //here we get the length of the rt header
                //this includes the length of the ieee80211_radiotap_header itself 
                rtHeaderLength = ((ieee80211_radiotap_header*)data)->it_len;
                dataLen = header.caplen - rtHeaderLength;
                if (dataLen <= 0)
                    continue;
                
                rtf = (ieee80211_radiotap_header*)data;
                //get the field's present into a u32
                rtFieldsPresent = rtf->it_present;
                
                //on my c2d it is 0x180F
                //NSLog(@"Raido Tap Fields present %.8x", rtFieldsPresent);
                
                //todo make this better
                //parse radiotap data
                //start at the least significant bit, process it, then shift it off
                //once all bits are processed, rtFieldsPresent should be 0
                //exiting the loop
                //NSLog(@"==============================================================================");
                rtBit = 0;
                //rt data is right after header
                rtDataPointer = (UInt8*)(data + sizeof(ieee80211_radiotap_header));
                //don't subtract these from dataLen, they are accounted for in the header len
                while(rtFieldsPresent)
                {
                    if(rtFieldsPresent & 0x01) //this bit is set
                    {
                        //NSLog(@"RT Field found %u", rtBit);
                        //increment the data pointer if by the bytes for this field
                        switch(rtBit)
                        {
                            case IEEE80211_RADIOTAP_TSFT_BIT:
                                rtDataPointer += IEEE80211_RADIOTAP_TSFT_BYTES;
                                break;
                            case IEEE80211_RADIOTAP_FLAGS_BIT:
                                rtDataPointer += IEEE80211_RADIOTAP_FLAGS_BYTES;
                                break;
                            case IEEE80211_RADIOTAP_RATE_BIT:
                                //NSLog(@"Rate: %u", *(UInt8*)rtDataPointer * 512); 
                                rtDataPointer += IEEE80211_RADIOTAP_RATE_BYTES;
                                break;
                            case IEEE80211_RADIOTAP_CHANNEL_BIT:
                                //NSLog(@"Found radiotap channel field");
                                //NSLog(@"Frequency: %u", *(UInt16*)rtDataPointer);
                                f->ctrl.channel = ieee80211_mhz2ieee(*(UInt16*)rtDataPointer, *(UInt16*)(rtDataPointer + 2));
                                rtDataPointer += IEEE80211_RADIOTAP_CHANNEL_BYTES;
                                break;
                            case IEEE80211_RADIOTAP_DBM_TX_POWER_BIT:
                                dataLen -= IEEE80211_RADIOTAP_DBM_TX_POWER_BYTES;  
                                rtDataPointer += IEEE80211_RADIOTAP_DBM_TX_POWER_BYTES;
                                break;
                            case IEEE80211_RADIOTAP_ANT_BIT:
                                //NSLog(@"Packet received on antenna %u", *(UInt8*)rtDataPointer);
                                rtDataPointer += IEEE80211_RADIOTAP_ANT_BYTES;
                                break;   
                            case IEEE80211_RADIOTAP_DBANTSIG_BIT:
                                //NSLog(@"Signal Db: %u", *(UInt8*)rtDataPointer);
                                f->ctrl.signal =  *(UInt8*)rtDataPointer;
                                rtDataPointer += IEEE80211_RADIOTAP_DBANTSIG_BYTES;
                                break;   
                            default:
                                NSLog(@"Unknown Field %i", rtBit);
                                //this is a serious error and will break everything
                                break;
                        }//end switch
                    }//end fields present 
                    //abort if we have reached the end of the data
                    //continue would just go around agian in this while loop
                    //pointless
                    if(dataLen <=0) break;
                    rtBit++;
                    rtFieldsPresent >>= 1;
                } //end while
                //NSLog(@"==============================================================================");
                
                //this is the start of the data after the device header and after the 80211 header
                dataLen -= 4; //Skip FCS?
                //NSLog(@"Data length: %u, caplen: %u", dataLen, header.caplen);
                if (dataLen <= 0 || dataLen > header.caplen) continue;
                f->ctrl.len = dataLen;
                memcpy(f->data, rtDataPointer, dataLen);
                break;
            case DLT_IEEE802_11_RADIO_AVS:
                dataLen = header.caplen - sizeof(avs_80211_1_header);
                dataLen -= 4;       // Skip fcs?
                if (dataLen <= 0)
                    continue;
                
                memcpy(f->data, data + sizeof(avs_80211_1_header), dataLen);
                
                af = (avs_80211_1_header*)data;
                f->ctrl.signal = OSSwapBigToHostInt32(af->ssi_signal) + 155;
                
                f->ctrl.silence = 0;
                
                f->ctrl.channel = OSSwapBigToHostInt32(af->channel);
                f->ctrl.len = dataLen;
                //NSLog(@"Got packet!!! hLen %u signal: %d  noise: %d channel %u length: %u\n", headerLength, af->ssi_signal, af->ssi_noise, f->channel, f->dataLen );
                break;
            case DLT_IEEE802_11:
                f->ctrl.len = header.caplen - 4;
                if (f->ctrl.len <= 0)
                    continue;
                f->ctrl.channel = _currentChannel;
                memcpy(f->data, data, header.caplen);
                break;
            default:
                NSLog(@"AE: Unknown packet format");
                NSLog(@"DLT %d", DLTType);
                break;
        } //switch
        _packets++;
        return f;
    }
}

#pragma mark -

-(bool) sendFrame:(UInt8*)f withLength:(int) size atInterval:(int)interval {
    return NO;
}

-(bool) stopSendingFrames
{    
    return NO;
}

#pragma mark -

-(void) dealloc 
{
    NSLog(@"about to close pcap device");
	if(_device) pcap_close(_device);
    [super dealloc];
}

@end

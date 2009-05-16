/*
        
        File:			WaveDriverPCMCIA.m
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

#import <IOKit/IOKitLib.h>
#import <IOKit/IODataQueueClient.h>
#import "WaveDriverPCMCIA.h"
#import "ImportController.h"
#import "WaveHelper.h"

typedef enum WLUCMethods {
    kWLUserClientOpen,         // kIOUCScalarIScalarO, 0, 0
    kWLUserClientClose,        // kIOUCScalarIScalarO, 0, 0
    kWLUserClientGetChannel,   // kIOUCScalarIScalarO, 0, 1
    kWLUserClientSetChannel,   // kIOUCScalarIScalarO, 1, 0
    kWLUserClientStartCapture, // kIOUCScalarIScalarO, 1, 0
    kWLUserClientStopCapture,  // kIOUCScalarIScalarO, 0, 0
    kWLUserClientSendFrame,
    kWLUserClientStopSendingFrames, // kIOUCScalarIScalarO, 0, 0
    kWLUserClientGetAllowedChannels,
    kWLUserClientLastMethod,
} WLUCMethod;

@implementation WaveDriverPCMCIA

+ (enum WaveDriverType) type {
    return passiveDriver;
}

#pragma mark -

// return 0 for success, 1 for error, 2 for self handled error
+ (int) initBackend {
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

#pragma mark -

-(kern_return_t) _connect {
    kern_return_t   kernResult;
    mach_port_t     masterPort;
    io_service_t    serviceObject;
    io_iterator_t   iterator;
    CFDictionaryRef classToMatch;

    kernResult = IOMasterPort(MACH_PORT_NULL, &masterPort);
    if (kernResult != KERN_SUCCESS) {
        NSLog(@"IOMasterPort returned 0x%x\n", kernResult);
        return kernResult;
    }
    classToMatch = IOServiceMatching(_driverName);

    if (classToMatch == NULL) {
        NSLog(@"IOServiceMatching returned a NULL dictionary.\n");
        return kernResult;
    }
    kernResult = IOServiceGetMatchingServices(masterPort,
                                              classToMatch,
                                              &iterator);

    if (kernResult != KERN_SUCCESS) {
        NSLog(@"IOServiceGetMatchingServices returned %x\n", kernResult);
        return kernResult;
    }

    serviceObject = IOIteratorNext(iterator);
    IOObjectRelease(iterator);
    if (serviceObject) {
        kernResult = IOServiceOpen(serviceObject, mach_task_self(), 0,
                                   &_userClientPort);

        IOObjectRelease(serviceObject);
        if (kernResult != KERN_SUCCESS) {
            NSLog(@"IOServiceOpen 0x%x\n", kernResult);
            return kernResult;
        }
    }

    kernResult = IOConnectMapMemory(_userClientPort,
                                    _userClientMap,
                                    mach_task_self(),
                                    (vm_address_t*)&_packetQueue,
                                    &_packetQueueSize,
                                    kIOMapAnywhere);
    if (kernResult != kIOReturnSuccess) {
        NSLog(@"IOConnectMapMemory 0x%x\n", kernResult);
        return kernResult;
    }

    _packetQueuePort = IODataQueueAllocateNotificationPort();
    kernResult = IOConnectSetNotificationPort(_userClientPort,
                                              _userClientNotify,
                                              _packetQueuePort,
                                              0);
    
    if (kernResult != kIOReturnSuccess) {
        NSLog(@"IOConnectSetNotificationPort 0x%x\n", kernResult);
        return kernResult;
    }

    return kernResult;
}

-(kern_return_t) _disconnect {
    return IOServiceClose(_userClientPort);
}

#pragma mark -

- (id)init {
    self=[super init];
    if(!self) return Nil;
    
    kern_return_t kernResult;

    kernResult = [self _connect];
    if (kernResult != KERN_SUCCESS) return Nil;
    
    kernResult = IOConnectMethodScalarIScalarO(_userClientPort, kWLUserClientOpen, 0, 0);
    if (kernResult != KERN_SUCCESS) {
        NSLog(@"open: IOConnectMethodScalarIScalarO: %x (driver not loaded?)\n", kernResult);
        [self release];
        return Nil;
    }
    
    _allowedChannels = 0;
    
    return self;
}

#pragma mark -

- (unsigned short) getChannelUnCached {
    unsigned short channel = 0;
    kern_return_t kernResult;

    if (![self allowsChannelHopping]) return 0;
    
    kernResult = IOConnectMethodScalarIScalarO(_userClientPort,
                                               kWLUserClientGetChannel, 0, 1, &channel);
    if (kernResult != KERN_SUCCESS) {
        NSLog(@"getChannel: IOConnectMethodScalarIScalarO: 0x%x\n", kernResult);
        return 0;
    }

    return channel;
}

- (bool) setChannel:(unsigned short)newChannel {
    kern_return_t kernResult;
    
    if (((_allowedChannels >> (newChannel - 1)) & 0x0001) == 0) return NO;
    
    kernResult = IOConnectMethodScalarIScalarO(_userClientPort,
                                               kWLUserClientSetChannel,
                                               1, 0, newChannel);
    if (kernResult != KERN_SUCCESS) {
        //NSLog(@"setChannel: IOConnectMethodScalarIScalarO: 0x%x\n", kernResult);
        return NO;
    }
    
    return YES;
}

- (bool) startCapture:(unsigned short)newChannel {
    kern_return_t kernResult;

    if (newChannel == 0) newChannel = _firstChannel;
    kernResult = IOConnectMethodScalarIScalarO(_userClientPort, kWLUserClientStartCapture, 1, 0, newChannel);
    
    if (kernResult != KERN_SUCCESS) {
        NSLog(@"startCapture: IOConnectMethodScalarIScalarO: 0x%x\n", kernResult);
        return NO;
    }
    
    return YES;
}

-(bool) stopCapture {
    kern_return_t kernResult;

    kernResult = IOConnectMethodScalarIScalarO(_userClientPort, kWLUserClientStopCapture, 0, 0);
    if (kernResult != KERN_SUCCESS) {
        NSLog(@"IOConnectMethodScalarIScalarO: 0x%x\n", kernResult);
        return NO;
    }

    while (IODataQueueDataAvailable(_packetQueue)) IODataQueueDequeue(_packetQueue, NULL, 0);

    return YES;
}

#pragma mark -
- (int)headerLenForFrameControl:(UInt16)frameControl {
	UInt16 isToDS, isFrDS, subtype, headerLength = 0;
    
	UInt16 type=(frameControl & IEEE80211_TYPE_MASK);
	//depending on the frame we have to figure the length of the header
	switch(type) {
		case IEEE80211_TYPE_DATA: //Data Frames
			isToDS = ((frameControl & IEEE80211_DIR_TODS) ? YES : NO);
			isFrDS = ((frameControl & IEEE80211_DIR_FROMDS) ? YES : NO);
			if (isToDS&&isFrDS)
                headerLength=30; //WDS Frames are longer
			else
                headerLength=24;
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

- (KFrame*) nextFrame {
    UInt8 tmpFrame[2500];
    static UInt8  frame[2500];
    UInt32 frameSize = 2500;
    kern_return_t kernResult;
    WLFrame *head = (WLFrame *)tmpFrame;
    KFrame *f = (KFrame *)frame;
    
    UInt16 isToDS, isFrDS, subtype, headerLength = 0;
    UInt16 type;

    while (!IODataQueueDataAvailable(_packetQueue)) {
        kernResult = IODataQueueWaitForAvailableData(_packetQueue,
                                                     _packetQueuePort);
        if (kernResult != KERN_SUCCESS) {
            NSLog(@"nextFrame: IODataQueueWaitForAvailableData: 0x%x\n", kernResult);
            return NULL;
        }
    }

    kernResult = IODataQueueDequeue(_packetQueue, tmpFrame, &frameSize);
    if (kernResult != KERN_SUCCESS) {
        NSLog(@"nextFrame: IODataQueueDequeue: 0x%x\n", kernResult);
        return NULL;
    } else {
        
        type = (head->frameControl & IEEE80211_TYPE_MASK);
        //depending on the frame we have to figure the length of the header
        switch(type) {
            case IEEE80211_TYPE_DATA: //Data Frames
                isToDS = ((head->frameControl & IEEE80211_DIR_TODS) ? YES : NO);
                isFrDS = ((head->frameControl & IEEE80211_DIR_FROMDS) ? YES : NO);
                if (isToDS&&isFrDS)
                    headerLength=30; //WDS Frames are longer
                else
                    headerLength=24;
                break;
            case IEEE80211_TYPE_CTL: //Control Frames
                subtype=(head->frameControl & IEEE80211_SUBTYPE_MASK);
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
        
        memset(f, 0, sizeof(KFrame));
        memcpy(f->data, tmpFrame+sizeof(WLPrismHeader), headerLength);
        memcpy((f->data)+24, tmpFrame+sizeof(WLFrame), frameSize-sizeof(WLFrame));
        f->ctrl.len = head->dataLen + headerLength;
        f->ctrl.signal = head->signal;
        f->ctrl.channel = head->channel;
        f->ctrl.silence = head->silence;
        _packets++;
        return f;
    }
}

#pragma mark -

-(bool) sendKFrame:(KFrame *)f howMany:(int)howMany atInterval:(int)interval notifyTarget:(id)target notifySelectorString:(NSString *)selector {
}

-(bool) sendKFrame:(KFrame *)kframe howMany:(int)howMany atInterval:(int)interval {
    UInt8 *f = kframe->data;
    int size = kframe->ctrl.len;
    static UInt8  frame[2364];
    UInt32 frameSize = 2364;
    kern_return_t kernResult;
    WLFrame *wlf = (WLFrame *)frame;
    struct ieee80211_hdr *hdr = (struct ieee80211_hdr *)f;

    UInt16 isToDS, isFrDS, subtype, headerLength = 0;
    UInt16 type;

    type = (hdr->frame_ctl & IEEE80211_TYPE_MASK);
    //depending on the frame we have to figure the length of the header
    switch(type) {
        case IEEE80211_TYPE_DATA: //Data Frames
            isToDS = ((hdr->frame_ctl & IEEE80211_DIR_TODS) ? YES : NO);
            isFrDS = ((hdr->frame_ctl & IEEE80211_DIR_FROMDS) ? YES : NO);
            if (isToDS&&isFrDS)
                headerLength=30; //WDS Frames are longer
            else
                headerLength=24;
            break;
            case IEEE80211_TYPE_CTL: //Control Frames
            subtype=(hdr->frame_ctl & IEEE80211_SUBTYPE_MASK);
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
    
    memset(frame, 0, sizeof(frame));
    memcpy(frame + sizeof(WLPrismHeader), f, headerLength);
    memcpy(frame + sizeof(WLFrame), f+headerLength, size-headerLength); 
    wlf->dataLen = size-headerLength;
    
    kernResult = IOConnectMethodScalarIStructureI(_userClientPort,
                                               kWLUserClientSendFrame,
                                               1, frameSize, interval, frame);
    if (kernResult != KERN_SUCCESS) {
        NSLog(@"sendFrame: IOConnectMethodScalarIStructureI: 0x%x\n", kernResult);
        return NO;
    }
    return YES;
}

-(bool) stopSendingFrames {    
    kern_return_t kernResult;

    kernResult = IOConnectMethodScalarIScalarO(_userClientPort,
                                               kWLUserClientStopSendingFrames,
                                               0, 0);
    if (kernResult != KERN_SUCCESS) {
        NSLog(@"IOConnectMethodScalarIScalarO: 0x%x\n", kernResult);
        return NO;
    }
    
    return YES;
}

#pragma mark -

- (int) allowedChannels {
    unsigned short channels = 0;
    kern_return_t kernResult;

    if (_allowedChannels) return _allowedChannels;
    if (![self allowsChannelHopping]) return 0;
    
    kernResult = IOConnectMethodScalarIScalarO(_userClientPort,
                                               kWLUserClientGetAllowedChannels, 0, 1, &channels);
    if (kernResult != KERN_SUCCESS) {
        NSLog(@"getChannel: IOConnectMethodScalarIScalarO: 0x%x\n", kernResult);
        return 0xFFFF;
    }

    _allowedChannels = channels;
    return channels;
}


#pragma mark -

-(void) dealloc {
    kern_return_t kernResult;
    kernResult = IOConnectMethodScalarIScalarO(_userClientPort,kWLUserClientClose,0, 0);
    if (kernResult != KERN_SUCCESS) NSLog(@"close: IOConnectMethodScalarIScalarO: 0x%x\n", kernResult);
    kernResult = [self _disconnect];
    [super dealloc];
}

@end

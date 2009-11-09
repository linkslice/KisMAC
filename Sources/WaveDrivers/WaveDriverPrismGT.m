/*
        
        File:			WaveDriverPrismGT.m
        Program:		KisMAC
	Author:			Michael Rossberg
				mick@binaervarianz.de
	Description:		KisMAC is a wireless stumbler for MacOS X.
                
        This file is part of KisMAC.

    KisMAC is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License, version 2,
    as published by the Free Software Foundation;

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
#import <BIGeneric/BINSExtensions.h>
#import "WaveDriverPrismGT.h"
#import "ImportController.h"
#import "WaveHelper.h"

#define driverName "GTDriver"

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
    kWiFiUserClientLastMethod,
} WLUCMethod;

typedef enum {
    modeNone = 0,
    modeIBSS,
    modeClient,
    modeHostAP,
    modeMonitor,
} wirelessMode;

@implementation WaveDriverPrismGT

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
    return NSLocalizedString(@"PrismGT based card, passive mode", "long driver description");
}

+ (NSString*) deviceName {
    return NSLocalizedString(@"PrismGT based Card", "short driver description");
}

#pragma mark -

// return 0 for success, 1 for error, 2 for self handled error
+ (int) initBackend {
    if ([WaveHelper isServiceAvailable:driverName]) return 0;
        
    [[WaveHelper mainWindow] showAlertMessage:NSLocalizedString(@"This driver will not work with older Prism2 or USB devices.\nPlease make sure, that the network card is properly inserted and the GTDriver (gtdriver.binaervarianz.de) is installed and working.", "PrismGT Error") title:NSLocalizedString(@"Communication failure!", "PrismGT Error title") button:nil];
    return 2;
}

+ (bool) loadBackend {
    return ([WaveDriverPrismGT initBackend] == 0);
}

+ (bool) unloadBackend {
    return YES;
}

#pragma mark -

-(kern_return_t) _connect
{
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

    _userClientNotify = kWiFiUserClientNotify;
    _userClientMap = kWiFiUserClientMap;
    _driverName = driverName;
    
    kern_return_t kernResult;

    kernResult = [self _connect];
    if (kernResult != KERN_SUCCESS) return Nil;
    
    kernResult = IOConnectMethodScalarIScalarO(_userClientPort, kWiFiUserClientOpen, 0, 0);
    if (kernResult != KERN_SUCCESS) {
        NSLog(@"open: IOConnectMethodScalarIScalarO: %x (driver not loaded?)\n", kernResult);
        [self release];
        return Nil;
    }
    
    return self;
}

#pragma mark -

- (unsigned short) getChannelUnCached {
    UInt32 channel = 0;
  
    if (![self allowsChannelHopping]) return 0;
    
    channel = IOConnectMethodScalarIScalarO(_userClientPort,
                                               kWiFiUserClientGetFrequency, 0, 0);

    return [WaveHelper freq2chan:channel];
}

- (bool) setChannel:(unsigned short)newChannel {
    kern_return_t kernResult;
    
    kernResult = IOConnectMethodScalarIScalarO(_userClientPort,
                                               kWiFiUserClientSetFrequency, 1, 0, [WaveHelper chan2freq: newChannel]);
    if (kernResult != KERN_SUCCESS) {
        //NSLog(@"setChannel: IOConnectMethodScalarIScalarO: 0x%x\n", kernResult);
        return NO;
    }
    
    return YES;
}

- (bool) startCapture:(unsigned short)newChannel {
    kern_return_t kernResult;
     
    kernResult = IOConnectMethodScalarIScalarO(_userClientPort, kWiFiUserClientSetMode, 1, 0, modeMonitor);
    if (kernResult != KERN_SUCCESS) {
        NSLog(@"startCapture: IOConnectMethodScalarIScalarO: 0x%x\n", kernResult);
        return NO;
    }

    [self setChannel: newChannel];
    
    return YES;
}

-(bool) stopCapture 
{
    kern_return_t kernResult;

    kernResult = IOConnectMethodScalarIScalarO(_userClientPort, kWiFiUserClientSetMode, 1, 0, modeClient);
    if (kernResult != KERN_SUCCESS)
    {
        NSLog(@"stopCapture: IOConnectMethodScalarIScalarO: 0x%x\n", kernResult);
        return NO;
    }
    while (IODataQueueDataAvailable(_packetQueue)) IODataQueueDequeue(_packetQueue, NULL, 0);

    return YES;
}

#pragma mark -

typedef struct {
	UInt16 unk0;		/* = 0x0000 */
	UInt16 length;		/* = 0x1400 */
	UInt32 clock;		/* 1MHz clock */
	volatile UInt8 flags;
	UInt8 unk1;
	volatile UInt8 rate;
	UInt8 unk2;
	volatile UInt16 freq;
	UInt16 unk3;
	volatile UInt8 rssi;
	UInt8 padding[3];
} rfmonHeader;

- (KFrame*) nextFrame {
    static UInt8  frame[2500];
    UInt8  tempframe[2500];
    u_int32_t frameSize = 2500;
    kern_return_t kernResult;
    rfmonHeader *head;
    KFrame *f;
    
    while(YES) {
        while (!IODataQueueDataAvailable(_packetQueue)) {
            kernResult = IODataQueueWaitForAvailableData(_packetQueue,
                                                         _packetQueuePort);
            if (kernResult != KERN_SUCCESS)
            {
                NSLog(@"nextFrame: IODataQueueWaitForAvailableData: 0x%x\n", kernResult);
                return NULL;
            }
        }

        kernResult = IODataQueueDequeue(_packetQueue, tempframe, &frameSize);
        if (kernResult != KERN_SUCCESS)
        {
            NSLog(@"nextFrame: IODataQueueDequeue: 0x%x\n", kernResult);
            return NULL;
        }

        head = (rfmonHeader*)tempframe;
        f = (KFrame *)frame;
        memset(f, 0, sizeof(frame));
        UInt16 headerLength = CFSwapInt16LittleToHost(head->length);
        f->ctrl.len = frameSize - headerLength;

        memcpy(f->data, tempframe + headerLength, f->ctrl.len);
        
        f->ctrl.signal = head->rssi & 0x7F;
        f->ctrl.channel = [WaveHelper freq2chan:NSSwapLittleShortToHost(head->freq)];
        break;
    }
    
    _packets++;
    return f;
}

#pragma mark -

-(void) dealloc 
{
    kern_return_t kernResult;
    kernResult = IOConnectMethodScalarIScalarO(_userClientPort, kWiFiUserClientClose, 0, 0);
    if (kernResult != KERN_SUCCESS) NSLog(@"close: IOConnectMethodScalarIScalarO: 0x%x\n", kernResult);
    
    kernResult = [self _disconnect];
    if (kernResult != KERN_SUCCESS) NSLog(@"close: _disconnect: 0x%x\n", kernResult);

    [super dealloc];
}

@end

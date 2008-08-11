/*
        
        File:			WaveDriverAtheros.m
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
#import "WaveDriverAtheros.h"
#import "ImportController.h"
#import "WaveHelper.h"

#define driverName "AtheroJack"

static bool explicitlyLoadedAtheros = NO;

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

enum _operationMode {
    _operationModeStation   = 0x01,
    _operationModeIBSS      = 0x02,
    _operationModeHostAP    = 0x04,
    _operationModeMonitor   = 0x08,
    _operationModeInvalid   = 0x00,
};

@implementation WaveDriverAtheros

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
    return NSLocalizedString(@"Atheros based card, passive mode", "long driver description");
}

+ (NSString*) deviceName {
    return NSLocalizedString(@"Atheros based Card", "short driver description");
}

#pragma mark -

// return 0 for success, 1 for error, 2 for self handled error
+ (int) initBackend {
    int x;
    
    if ([WaveHelper isServiceAvailable:driverName]) return 0;
    explicitlyLoadedAtheros = YES;
    
    if (![WaveHelper runScript:@"atheros_load.sh"]) return 2;
    NSLog(@"Finished runnning load script");

    for(x = 0; x < 70; x++) {
        [NSThread sleepUntilDate:[NSDate dateWithTimeIntervalSinceNow:0.2]];
        NSLog(@"Checking to see if driver is alive");
        if ([WaveHelper isServiceAvailable:driverName]) return 0;
    }
    NSLog(@"Something bad happened");
    return 1;
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
    if (!explicitlyLoadedAtheros) return YES;
    return [WaveHelper runScript:@"atheros_unload.sh"];
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

    _userClientNotify = 0xdeadbeef;
    _userClientMap = 0xdeadface;
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
    
    
    int freq = (newChannel == 0 ? [WaveHelper chan2freq: 1] : [WaveHelper chan2freq: newChannel]);
    NSLog(@"freq %d", freq);
    kernResult = IOConnectMethodScalarIScalarO(_userClientPort,
                                               kWiFiUserClientSetFrequency, 1, 0, freq);
    if (kernResult != true) {
        NSLog(@"setChannel: IOConnectMethodScalarIScalarO: 0x%x\n", kernResult);
        return NO;
    }
    
    return YES;
}

- (bool) startCapture:(unsigned short)newChannel {
    kern_return_t kernResult;

    [self setChannel: newChannel];
    int freq = (newChannel == 0 ? [WaveHelper chan2freq: 1] : [WaveHelper chan2freq: newChannel]);

    kernResult = IOConnectMethodScalarIScalarO(_userClientPort, kWiFiUserClientStartCapture, 1, 0, freq);
    if (kernResult != KERN_SUCCESS) {
        NSLog(@"startCapture: IOConnectMethodScalarIScalarO: 0x%x\n", kernResult);
        return NO;
    }
    
    return YES;
}

-(bool) stopCapture {
    kern_return_t kernResult;

    kernResult = IOConnectMethodScalarIScalarO(_userClientPort, kWiFiUserClientStopCapture, 0, 0);
    if (kernResult != KERN_SUCCESS) {
        NSLog(@"IOConnectMethodScalarIScalarO: 0x%x\n", kernResult);
        return NO;
    }

    while (IODataQueueDataAvailable(_packetQueue)) IODataQueueDequeue(_packetQueue, NULL, 0);

    return YES;
}

#pragma mark -

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

- (KFrame*) nextFrame {
    static UInt8 frame[2500];
    KFrame  *f = (KFrame *)frame;

    UInt8  tempframe[2500];
    u_int32_t frameSize = 2500;
    kern_return_t kernResult;
    
    struct _Prism_HEADER *head = (struct _Prism_HEADER *)(tempframe);
    
    while(YES) {
        while (!IODataQueueDataAvailable(_packetQueue)) {
            kernResult = IODataQueueWaitForAvailableData(_packetQueue,
                                                         _packetQueuePort);
            if (kernResult != KERN_SUCCESS) {
                NSLog(@"nextFrame: IODataQueueWaitForAvailableData: 0x%x\n", kernResult);
                return NULL;
            }
        }
        kernResult = IODataQueueDequeue(_packetQueue, tempframe, &frameSize);
        NSLog(@"Packet %d", frameSize);
        
        memset(f, 0, sizeof(KFrame));
        
        f->ctrl.len = frameSize - sizeof(struct _Prism_HEADER);
        memcpy(f->data, tempframe + sizeof(struct _Prism_HEADER), f->ctrl.len);

        f->ctrl.channel = head->channel;
        f->ctrl.silence = head->silence;
        f->ctrl.signal  = head->signal;

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
    kern_return_t kernResult;
    kernResult = IOConnectMethodScalarIScalarO(_userClientPort,kWiFiUserClientClose,0, 0);
    if (kernResult != KERN_SUCCESS) NSLog(@"close: IOConnectMethodScalarIScalarO: 0x%x\n", kernResult);
    kernResult = [self _disconnect];
    [super dealloc];
}

@end

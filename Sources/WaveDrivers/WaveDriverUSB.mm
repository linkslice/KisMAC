/*
        
        File:			WaveDriverUSBIntersil.m
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
#import "WaveDriverUSBIntersil.h"
#import "WaveHelper.h"

@implementation WaveDriverUSB

- (id)init {
    self=[super init];

    if (!self)
        return Nil;
    
    _driver = nil;
    
    [self wakeDriver];
    
    return self;
}


#pragma mark -

+ (enum WaveDriverType) type {
    return passiveDriver;
}

+ (bool) allowsInjection {
    return YES;
}

+ (bool) allowsChannelHopping {
    return YES;
}

+ (bool) allowsMultipleInstances {
    return YES;  //may be later
}

+ (NSString*) description {
    return NSLocalizedString(@"USB device, passive mode", "long driver description");
}

+ (NSString*) deviceName {
    return NSLocalizedString(@"USB device", "short driver description");
}

#pragma mark -

+ (bool) loadBackend {
    return YES;
}

+ (bool) unloadBackend {
    return YES;
}

#pragma mark -

- (unsigned short) getChannelUnCached {
    UInt16 channel;
    
    if (_driver->getChannel(&channel)) return channel;
    else return 0;
}

- (bool) setChannel:(unsigned short)newChannel {
    if (((_allowedChannels >> (newChannel - 1)) & 0x0001) == 0)
        return NO;
    
    return _driver->setChannel(newChannel);
}

- (bool) startCapture:(unsigned short)newChannel {
    if (newChannel == 0) newChannel = _firstChannel;
    return _driver->startCapture(newChannel);
}

- (bool) stopCapture {
    return _driver->stopCapture();
}

- (bool) sleepDriver{
    return YES;
}

- (bool) wakeDriver{
    return YES;
}

#pragma mark -

- (KFrame *) nextFrame {
    KFrame *f;

    f = _driver->receiveFrame();
    if (f==NULL) {
		_errors++;
        if (_packets && _driver) {
			if (_errors < 3) {
				NSLog(@"USB receiveFrame failed - attempting to reload driver");
                [self wakeDriver];
			} else {
				NSLog(@"Excessive errors received - terminating driver");
                [self sleepDriver];
			}
        }
        NSRunCriticalAlertPanel(NSLocalizedString(@"USB Prism2 error", "Error box title"),
                NSLocalizedString(@"USB Prism2 error description", "LONG Error description"),
                //@"A driver error occured with your USB device, make sure it is properly connected. Scanning will "
                //"be canceled. Errors may have be printed to console.log."
                OK, Nil, Nil);

    } else {
//        NSLog(@"nextFrame %d", f->ctrl.len);
        _packets++;
		_errors=0;
	}
    return f;
}

#pragma mark -

- (void)doInjection:(NSData*)data {
    NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];

    UInt8 *f = (UInt8 *)[data bytes];
    int size = [data length];  
    
    while(_transmitting) {
        _driver->sendFrame(f, size);
        [NSThread sleepUntilDate:[NSDate dateWithTimeIntervalSinceNow:_interval]];
    }
    [data release];
    
    [pool release];
}

-(bool) sendFrame:(UInt8*)f withLength:(int) size atInterval:(int)interval {
    NSData *data;
    
    if (interval) {
        [self stopSendingFrames];
        data = [[NSData dataWithBytes:f length:size] retain];
        _transmitting = YES;
        _interval = (float)interval / 1000.0;
        [NSThread detachNewThreadSelector:@selector(doInjection:) toTarget:self withObject:data];
    } else {
        _driver->sendFrame(f, size);
    }
    
    return YES;
}

-(bool) stopSendingFrames {
    _transmitting = NO;
    [NSThread sleepUntilDate:[NSDate dateWithTimeIntervalSinceNow:_interval]];
    return YES;
}

#pragma mark -

- (int) allowedChannels {
    UInt16 channels;
    
    if (_allowedChannels)
        return _allowedChannels;
    
    if (_driver->getAllowedChannels(&channels)) {
        _allowedChannels = channels;
        return channels;
    } else return 0xFFFF;
}

#pragma mark -

-(void) dealloc {
    [self stopSendingFrames];
    
    [self sleepDriver];
    
    [super dealloc];
}

@end

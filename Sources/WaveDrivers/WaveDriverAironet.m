/*
        
        File:			WaveDriverAironet.m
        Program:		KisMAC
	Author:			Michael Rossberg
				mick@binaervarianz.de
	Description:		KisMAC is a wireless stumbler for MacOS X.
                
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

#import "WaveDriverAironet.h"
#import "WaveHelper.h"

#define driverName "AiroJackDriver"

static bool explicitlyLoadedAiroJack = false;

@implementation WaveDriverAironet

- (id)init {
    _userClientNotify = 0xbeefc0de;
    _userClientMap = 0xdeadc0de;
    _driverName = driverName;
	_hop = NO; 
		
    self = [super init];
    return self;
}

#pragma mark -

+ (enum WaveDriverType) type {
    return passiveDriver;
}

+ (bool) allowsInjection {
    return NO;
}

+ (bool) allowsChannelHopping {
    return NO;
}

+ (NSString*) description {
    return NSLocalizedString(@"Cisco Aironet card, passive mode", "long driver description");
}

+ (NSString*) deviceName {
    return NSLocalizedString(@"Aironet Card", "short driver description");
}

#pragma mark - 

- (void) hopToNextChannel {
	return;
}

#pragma mark -

+ (int) initBackend {
    int x;
    
    if ([WaveHelper isServiceAvailable:driverName]) return 0;
    explicitlyLoadedAiroJack = YES;
    
    if (![WaveHelper runScript:@"airojack_load.sh"]) return 2;

    for(x = 0; x < 70; x++) {
        [NSThread sleepUntilDate:[NSDate dateWithTimeIntervalSinceNow:0.2]];
        
        if ([WaveHelper isServiceAvailable:driverName]) return 0;
    }
    
    return 1;
}

+ (bool) unloadBackend {
    if (!explicitlyLoadedAiroJack) return YES;
    return [WaveHelper runScript:@"airojack_unload.sh"];
}

@end

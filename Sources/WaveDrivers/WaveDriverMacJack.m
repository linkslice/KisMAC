/*
        
        File:			WaveDriverMacJack.m
        Program:		KisMAC
		Author:			Michael Rossberg
						mick@binaervarianz.de
		Description:	KisMAC is a wireless stumbler for MacOS X.
                
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
#import "WaveDriverMacJack.h"
#import "WaveHelper.h"

#define driverName "MACJackDriver"

static bool explicitlyLoadedMacJack = NO;

@implementation WaveDriverMacJack

- (id)init {
    _userClientNotify = 0xdeadbeef;
    _userClientMap = 0xdeadface;
    _driverName = driverName;

    self = [super init];
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

+ (NSString*) description {
    return NSLocalizedString(@"Prism2/Orinoco/Hermes card, passive mode", "long driver description");
}

+ (NSString*) deviceName {
    return NSLocalizedString(@"Prism2/Orinoco/Hermes Card", "short driver description");
}

#pragma mark -

+ (int) initBackend {
    int x;
    
    if ([WaveHelper isServiceAvailable:driverName]) return 0;
    
    if ([WaveHelper isServiceAvailable:"org_noncontiguous_WirelessDriver"]) {
        if (![WaveHelper runScript:@"macjack_prep.sh"]) return 1;
        for(x = 0; x < 70; x++) {
            [NSThread sleepUntilDate:[NSDate dateWithTimeIntervalSinceNow:0.2]];
            
            if (![WaveHelper isServiceAvailable:"org_noncontiguous_WirelessDriver"]) break;
        }
        if ([WaveHelper isServiceAvailable:"org_noncontiguous_WirelessDriver"]) {
            if ([[NSUserDefaults standardUserDefaults] integerForKey:@"supressWirelessDriverActiveWarning"]==0) {
                x = NSRunCriticalAlertPanel(
                    NSLocalizedString(@"WARNING! Do you really want to load the MacJack driver?.", "Warning dialog title"),
                    NSLocalizedString(@"Really load MacJack? description", "LONG Warning text. Make the user think"),
                /*@"KisMAC detected, that there is an active instance of the Sourceforge Wireless Driver, which "
                "cannot be unloaded. It is most likely that you did not install the patch which is provided with "
                "KisMAC. Please reinstall KisMAC! If you still continue chances are that your system will "
                "crash completly."*/
                    OK, CANCEL, NSLocalizedString(@"Don't bother me!", "button text"));
                
                if (x == NSAlertAlternateReturn) {
                    return 2;
                } else if (x == NSAlertOtherReturn) {
                    [[NSUserDefaults standardUserDefaults] setInteger:1 forKey:@"supressWirelessDriverActiveWarning"]; 
                }
            }

        } 
    }
    
    if (![WaveHelper runScript:@"macjack_load.sh"]) return 2;
    explicitlyLoadedMacJack = YES;
    
    for(x = 0; x < 70; x++) {
        [NSThread sleepUntilDate:[NSDate dateWithTimeIntervalSinceNow:0.2]];
        
        if ([WaveHelper isServiceAvailable:driverName]) return 0;
    }
    
    return 1;
}

+ (bool) unloadBackend {
    if (!explicitlyLoadedMacJack) return YES;
    
    return [WaveHelper runScript:@"macjack_unload.sh"];
}

@end

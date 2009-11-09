/*
        
        File:			WaveDriverViha.m
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

#import "WaveDriverViha.h"
#import "WaveHelper.h"
#import "WaveDriverAirport.h"

#define driverName "WLanDriver"

static bool explicitlyLoadedViha = NO;

@implementation WaveDriverViha

- (id)init {
    _userClientNotify = 0xfeedbeef;
    _userClientMap = 0xfeedface;
    _driverName = driverName;

    self = [super init];
    return self;
}

#pragma mark-

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
    return NSLocalizedString(@"Apple Airport card, passive mode", "long driver description");
}

+ (NSString*) deviceName {
    return NSLocalizedString(@"Airport Card", "short driver description");
}

#pragma mark -

+ (bool) initBackend {
    int x;
    WirelessContextPtr airportContext;
    
    if ([WaveDriverAirport airportInstanceCount]) { //PseudoJack is up; this will cause problems...
        NSLog(@"Could not instantiate Viha Driver, because of an active PseudoJack instance.");
        NSRunCriticalAlertPanel(
            NSLocalizedString(@"Could not load custom Airport Driver.", "Error dialog title"),
            NSLocalizedString(@"Could not load custom Airport Driver. description", "LONG description"),
            //@"You have been running KisMAC with active drivers. You will need to restart"
            //" KisMAC before you can use the passive Airport driver"
            OK, nil, nil);
        return 2;
    }
    
    //see whether we can disable the airport via API
    if ([WaveHelper isServiceAvailable:"AirPortDriver"] && WirelessIsAvailable()) {
        if (WirelessAttach(&airportContext, 0) == 0) {
            WirelessSetEnabled(airportContext, 0);
            WirelessSetPower(airportContext, 0);
            WirelessDetach(airportContext);
        }
        airportContext = Nil;
    }
    
    if (![WaveHelper runScript:@"viha_prep.sh"]) {
	   if ([WaveHelper isServiceAvailable:"AirPortDriver"] && WirelessIsAvailable()) {
			if (WirelessAttach(&airportContext, 0) == 0) {
				WirelessSetEnabled(airportContext, 1);
				WirelessSetPower(airportContext, 1);
				WirelessDetach(airportContext);
			}
			airportContext = Nil;
		}
 		return 2;
    }
	
    [NSThread sleepUntilDate:[NSDate dateWithTimeIntervalSinceNow:0.2]];

    
    for(x = 0; x < 100; x++) {
        if (![WaveHelper isServiceAvailable:"AirPortDriver"]) break;
        [NSThread sleepUntilDate:[NSDate dateWithTimeIntervalSinceNow:0.1]];
    }

    if (x==100)  {
        NSLog(@"KisMAC could not load the Apple Airport driver. Most likely you will expirience problems!"); 
        
        //this warining is ok for MacOS X 10.2 but for 10.3 we have to force the driver to quit....
        if ([[NSUserDefaults standardUserDefaults] integerForKey:@"supressAirPortActiveWarning"]==0) {
            x = NSRunCriticalAlertPanel(
            NSLocalizedString(@"WARNING! Do you really want to load the Viha driver?.", "warning dialog title"),
            NSLocalizedString(@"Really want to load Viha warning", "LONG description"),
            /*@"KisMAC detected, that there is most likely still some kind of a program, which accesses your airport driver "
            "(such as a signal meter, internet connect, another stumbler or a preference pane). If you continue, your Airport "
            "Connection might not come back when you quit KisMAC.\n\n"
            "Please note: There are certain systems, which seem to access the driver all the time. You can try to kill \"configd\" or you can try "
            "to override this warning, if you want to use KisMAC on such a system.",*/
            OK, CANCEL, NSLocalizedString(@"Don't bother me!", "button text"));
            
            if (x == NSAlertAlternateReturn) {
                [WaveHelper runScript:@"viha_unprep.sh"];
                return 2;
            } else if (x == NSAlertOtherReturn) {
                [[NSUserDefaults standardUserDefaults] setInteger:1 forKey:@"supressAirPortActiveWarning"]; 
            }
        }
    }
    
    
    //under OS X 10.3 we have to make a blood bath to get rid of the driver <eg>
    /*if (floor(NSAppKitVersionNumber) == 743) {
        if (![WaveHelper runScript:@"viha_kill.sh"]) return 2;
        
        [NSThread sleepUntilDate:[NSDate dateWithTimeIntervalSinceNow:0.2]];
        
        for(x = 0; x < 100; x++) {
            [NSThread sleepUntilDate:[NSDate dateWithTimeIntervalSinceNow:0.1]];
            if (x % 10 == 9) if (![WaveHelper runScript:@"viha_kill.sh"]) return 1;
        }
    }*/

    if (![WaveHelper runScript:@"viha_load.sh"]) return 1;
    explicitlyLoadedViha = YES;
    
    for(x = 0; x < 70; x++) {
        [NSThread sleepUntilDate:[NSDate dateWithTimeIntervalSinceNow:0.2]];
        
        if ([WaveHelper isServiceAvailable:driverName]) return 0;
    }
    
    return 1;
}

+ (bool) unloadBackend {
    if (!explicitlyLoadedViha) return YES;

    return [WaveHelper runScript:@"viha_unload.sh"];
}

@end

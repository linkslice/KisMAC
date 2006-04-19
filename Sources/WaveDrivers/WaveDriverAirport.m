/*
        
        File:			WaveDriverAirport.m
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

#import "WaveDriverAirport.h"
#import "WaveHelper.h"

static int AirPortInstances = 0;

@implementation WaveDriverAirport

- (id)init {
    self = [super init];
    if (!self)  return nil;
    
    if (![WaveHelper isServiceAvailable:"AirPortDriver"] && ![WaveHelper isServiceAvailable:"AirPortPCI"] && ![WaveHelper isServiceAvailable:"AirPort_Athr5424"]) {
        NSRunCriticalAlertPanel(NSLocalizedString(@"Could not load Airport Driver.", "Error dialog title"),
            NSLocalizedString(@"Could not load Airport Driver. Apple Driver not loaded", "LONG desc with solution"),
           // @"KisMAC is not able to load the Apple Airport driver, if you killed it by loading the Viha driver. Try restarting KisMAC."
            OK, Nil, Nil);
        return nil;
    }

    if (WirelessIsAvailable() == 0) {  // check the API
        NSRunCriticalAlertPanel(
            NSLocalizedString(@"Could not load Airport Driver.", "Error dialog title"),
            NSLocalizedString(@"Could not load Airport Driver. Apple API gone mad", "LONG desc"),
         //   @"KisMAC was able to find the driver, but the Apple API tells us, it is not there. No idea what this means.", 
            OK, Nil, Nil);
        return nil;
    }
    
    if (WirelessAttach(&_context, 0)) {
        NSRunCriticalAlertPanel(
            NSLocalizedString(@"Could not load Airport Driver.", "Error dialog title"),
            NSLocalizedString(@"Could not load Airport Driver. Attachment error", "LONG desc"),
          //  @"KisMAC could not attach to the Apple Airport Driver.", 
            OK, Nil, Nil);
        return nil;
    }
    
    AirPortInstances++;
    _hop = NO;
    WirelessSetEnabled(_context, 1); //just make sure we can actually scan
    WirelessSetPower(_context, 1);
    
    return self;
}

+(int) airportInstanceCount {
    return AirPortInstances;
}

#pragma mark -

+ (enum WaveDriverType) type {
    return activeDriver;
}

+ (NSString*) description {
    return NSLocalizedString(@"Apple Airport or Airport Extreme card, active mode", "long driver description");
}

+ (NSString*) deviceName {
    return NSLocalizedString(@"Airport Card", "short driver description");
}

#pragma mark -

+ (bool) loadBackend {
    if (!([WaveHelper isServiceAvailable:"AirPortDriver"] || [WaveHelper isServiceAvailable:"AirPortPCI"] || [WaveHelper isServiceAvailable:"AirPort_Athr5424"] || WirelessIsAvailable()==1)) {
        NSLog(@"Could not find an AirPortCard for PseudoJack.");
        NSRunCriticalAlertPanel(
            NSLocalizedString(@"Could not load Airport Driver.", "Error dialog title"),
            NSLocalizedString(@"Could not load Airport Driver. Drivers not found", "LONG Error dialog description"),
            //@"KisMAC could not find your Apple Airport drivers. Please "
            //"make sure they are properly installed and not replaced it with the Viha driver.", 
            OK, nil, nil);
        return NO;
    }
   
    return YES;
}

+ (bool) unloadBackend {
    return YES;
}

#pragma mark -

- (NSArray*) networksInRange {
    CFArrayRef netsp = NULL, netsAdHocp = NULL;
    WIErr res;

    if (_context==Nil) return Nil;        //someone killed the aiport driver?!
    
	res = WirelessCreateScanResults(_context, 0, &netsp, &netsAdHocp, 0);
	if (res) {
        return Nil;
    }
    
	NSMutableArray *ret = [NSMutableArray arrayWithArray:(NSArray*)netsp];
	[ret addObjectsFromArray:(NSArray*)netsAdHocp];
	
    return ret;
}

#pragma mark -

- (void) hopToNextChannel {
	return;
}

#pragma mark -

-(void) dealloc {
    WirelessDetach(_context);
    _context = Nil;
    
    AirPortInstances--;
    
    [super dealloc];
}


@end

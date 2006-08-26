/*
 
 File:			GPSInfoController.m
 Program:		KisMAC
 Author:	    themacuser  themacuser -at- gmail.com
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

#import "GPSInfoController.h"
#import "WaveHelper.h"

@implementation GPSInfoController

- (void)awakeFromNib {
    [[self window] setDelegate:self];
}

- (void)closeWindow:(NSWindow *)sheet returnCode:(int)returnCode contextInfo:(void *)contextInfo {
	//[[self window] performClose:self];
}

- (void)updateDataNS:(double)ns EW:(double)ew ELV:(double)elv numSats:(int)sats HDOP:(double)hdop VEL:(float)vel {
	_vel = vel;
	
	[_sats_indicator setCriticalValue:3];
	if (!sats || ew > 180 || ns > 90 || vel < 0) {
		[_fix_indicator setFloatValue:0.1];
		[_fix_type setStringValue:@"NO"];
		[_hdop_indicator setIntValue:8];
		[_sats_indicator setIntValue:0];
		[_lat_field setStringValue:@""];
		[_lon_field setStringValue:@""];
		[_vel_field setStringValue:@""];
		_haveFix = 0;
	} else if (!elv) {
		[_fix_indicator setFloatValue:0.5];
		[_fix_type setStringValue:@"2D"];
		[_hdop_indicator setFloatValue:hdop];
		[_sats_indicator setIntValue:sats];
		[_lat_field setStringValue:[NSString stringWithFormat:@"%.5f",ns]];
		[_lon_field setStringValue:[NSString stringWithFormat:@"%.5f",ew]];
		[_vel_field setStringValue:[NSString stringWithFormat:@"%.5f",(_vel * _velFactor)]];
		_haveFix = 1;
	} else if (elv && sats) {
		[_fix_indicator setFloatValue:1];
		[_fix_type setStringValue:@"3D"];
		[_hdop_indicator setFloatValue:hdop];
		[_sats_indicator setIntValue:sats];
		[_lat_field setStringValue:[NSString stringWithFormat:@"%.5f",ns]];
		[_lon_field setStringValue:[NSString stringWithFormat:@"%.5f",ew]];
		[_vel_field setStringValue:[NSString stringWithFormat:@"%.5f",(_vel * _velFactor)]];
		_haveFix = 2;
	}
}

- (IBAction)updateSpeed:(id)sender {
		if ([[_speedType titleOfSelectedItem] isEqualToString:@"KT"]) {
			_velFactor = 1;
		} else if ([[_speedType titleOfSelectedItem] isEqualToString:@"KPH"]) {
			_velFactor = 1.852;
		} else if ([[_speedType titleOfSelectedItem] isEqualToString:@"MPH"]) {
			_velFactor = 1.15077945;
		}
		
	if (_haveFix) {
		[_vel_field setStringValue:[NSString stringWithFormat:@"%.5f",(_vel * _velFactor)]];
	}
}

- (BOOL)windowShouldClose:(id)sender {
    // Set up our timer to periodically call the fade: method.
    [[NSTimer scheduledTimerWithTimeInterval:0.05 target:self selector:@selector(fade:) userInfo:nil repeats:YES] retain];
    [_showMenu setState:NSOffState];
    return NO;
}

- (void)setShowMenu:(NSMenuItem *)menu
{
	_showMenu = menu;
}

- (void)fade:(NSTimer *)timer {
    if ([[self window] alphaValue] > 0.0) {
        // If window is still partially opaque, reduce its opacity.
        [[self window] setAlphaValue:[[self window] alphaValue] - 0.2];
    } else {
        // Otherwise, if window is completely transparent, destroy the timer and close the window.
        [timer invalidate];
        [timer release];
        
		[[self window] close];
		[self release];
		[WaveHelper setGPSInfoController:NULL];
    }
}


@end
/*
 
 File:			GPSInfoController.h
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

#import <Cocoa/Cocoa.h>


@interface GPSInfoController : NSWindowController {
	NSMenuItem* _showMenu;
	IBOutlet NSLevelIndicator* _sats_indicator;
	IBOutlet NSLevelIndicator* _hdop_indicator;
	IBOutlet NSLevelIndicator* _fix_indicator;
	IBOutlet NSTextField* _fix_type;
	IBOutlet NSTextField* _lat_field;
	IBOutlet NSTextField* _lon_field;
	IBOutlet NSTextField* _vel_field;
	IBOutlet NSPopUpButton* _speedType;
	
	float _vel;
	float _velFactor;
	
	int _haveFix;
}
- (void)setShowMenu:(NSMenuItem *)menu;
- (void)updateDataNS:(double)ns EW:(double)ew ELV:(double)elv numSats:(int)sats HDOP:(double)hdop VEL:(float)vel;
- (IBAction)updateSpeed:(id)sender;
@end

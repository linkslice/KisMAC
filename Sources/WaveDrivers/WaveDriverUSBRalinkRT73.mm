/*
 
 File:			WaveDriverUSBRalinkRT73.m
 Program:		KisMAC
 Author:        pr0gg3d
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

#import "WaveDriverUSBRalinkRT73.h"


@implementation WaveDriverUSBRalinkRT73

- (bool) wakeDriver{
    [self sleepDriver];
    
    _driver = new RT73Jack;
    _driver->startMatching();
    NSLog(@"Matching finished\n");
    if (!(_driver->deviceMatched()))
        return NO;
    
    if(_driver->_init() != kIOReturnSuccess)
        return NO;
    
	_errors = 0;
    
    return YES;
}

+ (NSString*) description {
    return NSLocalizedString(@"USB RT73 device", "long driver description");
}

+ (NSString*) deviceName {
    return NSLocalizedString(@"USB RT73 device", "short driver description");
}

#pragma mark -

@end

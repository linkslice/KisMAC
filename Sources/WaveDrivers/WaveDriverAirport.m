/*
        
        File:			WaveDriverAirport.m
        Program:		KisMAC
		Author:			Geoffrey Kruse
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

#import "WaveDriverAirport.h"
#import "WaveHelper.h"

static int AirPortInstances = 0;

@implementation WaveDriverAirport

- (id)init 
{
    [super init];
    NSArray * availableInterfaces;
    BOOL success = NO;
    NSError * error;
    
    NSLog(@"init");
    
    //first we must find an interface
    availableInterfaces = [CWInterface supportedInterfaces];
    
    //CFShow(availableInterfaces);
    
    //for now just grab the first one
    if([availableInterfaces count] > 0)
    {
        airportInterface = [[CWInterface interfaceWithName: 
                                         [availableInterfaces objectAtIndex: 0]] retain];
        //CFShow(airportInterface);
        if(YES == airportInterface.power)
        {
            success = YES;
        }
        else
        {
            success = [airportInterface setPower: YES error: &error];
            CFShow(error);
        }
    }
    
    if(!success)
    {
        self = nil;
    }
    
    return self;
}

+(int) airportInstanceCount 
{
    return AirPortInstances;
}

#pragma mark -

+ (enum WaveDriverType) type 
{
    return activeDriver;
}

+ (NSString*) description 
{
    return NSLocalizedString(@"Apple Airport or Airport Extreme card, active mode", 
                                                        "long driver description");
}

+ (NSString*) deviceName 
{
    return NSLocalizedString(@"Airport Card", "short driver description");
}

#pragma mark -
//apple knows best, ask api if wireless is available
+ (bool) loadBackend 
{
    NSArray * availableInterfaces;
    
    availableInterfaces = [CWInterface supportedInterfaces];
    
    return ([availableInterfaces count] > 0);
}

+ (bool) unloadBackend 
{
    return YES;
}

#pragma mark -

//this is the same as what you would see in the airport menu
//don't expect any more information than that in active mode
- (NSArray*) networksInRange 
{
    NSArray * networks;
    NSDictionary *params;
    NSError * error = nil;
        
    //don't merge duplicate ssids
    params = [NSDictionary dictionaryWithObject: [NSNumber numberWithBool: NO]
                                                       forKey:kCWScanKeyMerge];
    
    networks = [airportInterface scanForNetworksWithParameters:params error: &error]; 
    
    CFShow(error);
    //CFShow(networks);
  
    return networks;
}

#pragma mark -

//active driver does not support changing channels
- (void) hopToNextChannel 
{
	return;
}

#pragma mark -

-(void) dealloc 
{
    [airportInterface release];
    airportInterface = nil;
    [super dealloc];
}


@end

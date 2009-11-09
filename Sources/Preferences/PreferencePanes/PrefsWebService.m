/*
        
        File:			PrefsWebService.m
        Program:		KisMAC
	Author:			Michael Rossberg
				mick@binaervarianz.de
	Description:		KisMAC is a wireless stumbler for MacOS X.
                
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

#import "PrefsWebService.h"
#import "WaveHelper.h"

@implementation PrefsWebService

- (void)updateRestrictions {
    switch ([_useWebService state]) {
        case NSOnState:
            [_webServiceAccount setEnabled:YES];
            [_webServicePassword setEnabled:YES];
            [_useWebServiceAutomatically setEnabled:YES];
            break;
        case NSOffState:
        default:
            [_webServiceAccount setEnabled:NO];
            [_webServicePassword setEnabled:NO];
            [_useWebServiceAutomatically setEnabled:NO];
            break;
    }
}
- (void)updateUI {
    NSString *password;
    
    password = [WaveHelper getPasswordForAccount:[controller objectForKey:@"webServiceAccount"]];
    if (!password) password = @"";
    
    [_useWebService                 setState: [[controller objectForKey:@"useWebService"] boolValue] ? NSOnState : NSOffState];
    [_useWebServiceAutomatically    setState: [[controller objectForKey:@"useWebServiceAutomatically"] boolValue] ? NSOnState : NSOffState];
    
    [_webServiceAccount     setStringValue:[controller objectForKey:@"webServiceAccount"]];
    [_webServicePassword    setStringValue:password];
    
    [self updateRestrictions];
}

-(BOOL)updateDictionary {
    NSString *account;
    
    [_webServiceAccount validateEditing];
    [_webServicePassword validateEditing];
    
    account = [[_webServiceAccount stringValue] stringByTrimmingCharactersInSet:[NSCharacterSet whitespaceAndNewlineCharacterSet]];
    
    [WaveHelper deletePasswordForAccount:[controller objectForKey:@"webServiceAccount"]];
    [controller setObject:account forKey:@"webServiceAccount"];
    [WaveHelper storePassword:[_webServicePassword stringValue] forAccount:account];

    [controller setObject:[NSNumber numberWithBool:[_useWebService state] == NSOnState] forKey:@"useWebService"];
    [controller setObject:[NSNumber numberWithBool:[_useWebServiceAutomatically state] == NSOnState] forKey:@"useWebServiceAutomatically"];
   
    [self updateRestrictions];

    return YES;
}

-(IBAction)setValueForSender:(id)sender {
    [self updateRestrictions];
}

-(IBAction)signUp:(id)sender {
    [[NSWorkspace sharedWorkspace] openURL:[NSURL URLWithString:@"http://binaervarianz.de/register.php"]];
}

-(IBAction)openDotKisMAC:(id)sender {
    [[NSWorkspace sharedWorkspace] openURL:[NSURL URLWithString:@"http://kismac.binaervarianz.de/dotkismac.php"]];
}

@end

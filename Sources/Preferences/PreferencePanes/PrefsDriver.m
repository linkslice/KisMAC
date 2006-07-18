//
//  PrefsDriver.m
//  KisMAC
//
//  Created by Michael Thole on Mon Jan 20 2003.
//  Copyright (c) 2003 Michael Thole. All rights reserved.
//

#import "PrefsDriver.h"
#import "WaveHelper.h"
#import "WaveDriver.h"
#import "WaveDriverAirportExtreme.h"

@implementation PrefsDriver


//updates the driverlist, ignoring multiple drivers, which are allowed to be selected only once
- (void)updateDrivers {
    NSArray *drivers;
    int i = 0;
    unsigned int j;
    NSString *s;
    Class c;
    
    [_driver removeAllItems];
    drivers = [controller objectForKey:@"ActiveDrivers"];
    
    while (WaveDrivers[i][0]) {
        s = [NSString stringWithCString:WaveDrivers[i]];
        for (j = 0; j < [drivers count]; j++) {
            c = NSClassFromString(s);
            
            //check if device exists
            if ([[[drivers objectAtIndex:j] objectForKey:@"driverID"] isEqualToString:s]) break;

            //check if device is in use by some other driver, which is already loaded
            if ([[[drivers objectAtIndex:j] objectForKey:@"deviceName"] isEqualToString:[c deviceName]]) break;
        }
        c = NSClassFromString(s);
        
        if (j == [drivers count] || [c allowsMultipleInstances]) {
            [_driver addItemWithTitle:[NSClassFromString(s) description]];
            [[_driver lastItem] setTag:i];
        }
        i++;
    }
}

-(Class) getCurrentDriver {
    NSDictionary *d;
    int i = [_driverTable selectedRow];
    
    if (i<0) return Nil;
    
    d = [[controller objectForKey:@"ActiveDrivers"] objectAtIndex:i];
    return NSClassFromString([d objectForKey:@"driverID"]);
}

-(NSDictionary*) getCurrentSettings {
    int i = [_driverTable selectedRow];
    
    if (i<0) return Nil;
    
    return [[controller objectForKey:@"ActiveDrivers"] objectAtIndex:i];
}

- (void) updateSettings {
    bool enableAll = NO;
    bool enableChannel = NO;
    bool enableInjection = NO;
    bool enableDumping = NO;
    Class driverClass;
    NSDictionary *d = Nil;
    unsigned int x, y;
    int val, startCorrect = 0;
	BOOL aeEnabledForever;
    
	aeEnabledForever = [[controller objectForKey:@"aeForever"] boolValue] && [WaveDriverAirportExtreme monitorModeEnabled];
    [_frequence     setFloatValue:  [[controller objectForKey:@"frequence"   ] floatValue]];
    [_aeForever     setState:       aeEnabledForever ? NSOnState : NSOffState]; 

    if ([_driverTable numberOfSelectedRows]) {
        d = [self getCurrentSettings];
        enableAll = YES;

        driverClass = [self getCurrentDriver];
        if ([driverClass allowsChannelHopping]) enableChannel = YES;
        if ([driverClass allowsInjection]) enableInjection = YES;
        if ([driverClass type] == passiveDriver) enableDumping = YES;
        
    }
    
    [_removeDriver  setEnabled:enableAll];
    
    [_selAll        setEnabled:enableChannel];
    [_selNone       setEnabled:enableChannel];
    [_channelSel    setEnabled:enableChannel];
    [_firstChannel  setEnabled:enableChannel];
    
    [_dumpDestination       setEnabled:enableDumping];
    [_dumpFilter            setEnabled:enableDumping];
    
    [_injectionDevice       setEnabled:enableInjection];
    if (!enableInjection) {
         [_injectionDevice setTitle:@"Airport Not Supported"];
    }
    
    if (enableChannel) {
        [_firstChannel  setIntValue:    [[d objectForKey:@"firstChannel"] intValue]];

        for (x = 0; x<2; x++) 
            for (y = 0; y < 7; y++) {
                val = [[d objectForKey:[NSString stringWithFormat:@"useChannel%.2i",(x*7+y+1)]] boolValue] ? NSOnState : NSOffState;
                [[_channelSel cellAtRow:y column:x] setState:val];
                if (x*7+y+1 == [_firstChannel intValue]) startCorrect = val;
            }
        
        if (startCorrect==0) {
            for (x = 0; x<2; x++) {
                for (y = 0; y < 7; y++) {
                    val = [[d objectForKey:[NSString stringWithFormat:@"useChannel%.2i",(x*7+y+1)]] boolValue] ? NSOnState : NSOffState;
                    if (val) {  
                        [_firstChannel setIntValue:x*7+y+1];
                        break;
                    }
                }
                if (y!=7) break;
            }
        }
    } else {
        for (x = 0; x<2; x++) 
            for (y = 0; y < 7; y++)
                [[_channelSel cellAtRow:y column:x] setState:NSOffState];

        
        [_firstChannel  setIntValue:   1];
    }
    
    if (enableInjection) {
        [_injectionDevice setState: [[d objectForKey:@"injectionDevice"] intValue]];
    } else {
        [_injectionDevice setState: NSOffState];
    }
    
    if (enableDumping) {
       [_dumpDestination setStringValue:[d objectForKey:@"dumpDestination"]];
       [_dumpFilter selectCellAtRow:[[d objectForKey:@"dumpFilter"] intValue] column:0];
       [_dumpDestination setEnabled:[[d objectForKey:@"dumpFilter"] intValue] ? YES : NO];
    } else {
       [_dumpDestination setStringValue:@"~/DumpLog %y-%m-%d %H:%M"];
       [_dumpFilter selectCellAtRow:0 column:0];
       [_dumpDestination setEnabled:NO];
    }
}

-(BOOL)updateInternalSettings:(BOOL)warn {
    NSMutableDictionary *d;
    NSMutableArray *a;
    WaveDriver *wd;
    int i = [_driverTable selectedRow];
    int val = 0, startCorrect = 0;
    unsigned int x, y;
    
    [controller setObject:[NSNumber numberWithFloat: [_frequence     floatValue]]    forKey:@"frequence"];
    [controller setObject:[NSNumber numberWithBool: [_aeForever state] == NSOnState] forKey:@"aeForever"];

    if (i < 0) return YES;
    d = [[self getCurrentSettings] mutableCopy];
    if (!d) return YES;
    
    if ([[self getCurrentDriver] allowsChannelHopping]) {
        for (x = 0; x<2; x++) 
            for (y = 0; y < 7; y++) {
                val+=[[_channelSel cellAtRow:y column:x] state];
                if (x*7+y+1 == [_firstChannel intValue]) startCorrect = [[_channelSel cellAtRow:y column:x] state];
            }    
        
        if (warn && (val == 0 || startCorrect == 0)) {
            NSRunAlertPanel(NSLocalizedString(@"Invalid Option", "Invalid channel selection failure title"),
                            NSLocalizedString(@"Invalid channel selection failure title", "LONG Error description"),
                            //@"You have to select at least one channel, otherwise scanning makes no sense. Also please make sure that you have selected "
                            //"a valid start channel.",
                            OK,nil,nil);
            [d release];
            return NO;
        }
    }

    for (x = 0; x<2; x++) 
        for (y = 0; y < 7; y++) {
            val = [[_channelSel cellAtRow:y column:x] state];
            [d setObject:[NSNumber numberWithBool: val ? YES : NO] forKey:[NSString stringWithFormat:@"useChannel%.2i",(x*7+y+1)]];
        }
    
    [d setObject:[NSNumber numberWithInt:   [_firstChannel  intValue]]      forKey:@"firstChannel"];
    
    [d setObject:[NSNumber numberWithBool:  [_injectionDevice state] ? YES : NO] forKey:@"injectionDevice"];
    
    [d setObject:[_dumpDestination stringValue] forKey:@"dumpDestination"];
    [d setObject:[NSNumber numberWithInt:[_dumpFilter selectedRow]] forKey:@"dumpFilter"];
    
    a = [[controller objectForKey:@"ActiveDrivers"] mutableCopy];
    [a replaceObjectAtIndex:i withObject:d];
    [controller setObject:a forKey:@"ActiveDrivers"];
    
    wd = [WaveHelper driverWithName:[d objectForKey:@"deviceName"]];
    [wd setConfiguration:d];
    
    [d release];
    [a release];
    
    return YES;
}

#pragma mark -

- (int)numberOfRowsInTableView:(NSTableView *)aTableView {
    return [[controller objectForKey:@"ActiveDrivers"] count];
}

- (id) tableView:(NSTableView *) aTableView objectValueForTableColumn:(NSTableColumn *) aTableColumn row:(int) rowIndex {     
    return [NSClassFromString([[[controller objectForKey:@"ActiveDrivers"] objectAtIndex: rowIndex] objectForKey:@"driverID"]) description]; 
}

- (void)tableViewSelectionDidChange:(NSNotification *)notification {
    [self updateSettings];
}

- (BOOL)tableView:(NSTableView *)tableView shouldSelectRow:(int)row {
    return [self updateInternalSettings:YES];
}

#pragma mark -

-(void)updateUI {
    [self updateDrivers];
    [self updateSettings];
}

-(BOOL)updateDictionary {
    return [self updateInternalSettings:YES];
}

-(IBAction)setValueForSender:(id)sender {
    [self updateInternalSettings:NO];
    [self updateSettings];
}

#pragma mark -

- (IBAction)selAddDriver:(id)sender {
    NSMutableArray *drivers;
    NSString *driverClassName;
	int result;
    
    driverClassName = [NSString stringWithCString:WaveDrivers[[[_driver selectedItem] tag]]];
    
    drivers = [[controller objectForKey:@"ActiveDrivers"] mutableCopy];
    [drivers addObject:[NSDictionary dictionaryWithObjectsAndKeys:
        driverClassName, @"driverID",
        [NSNumber numberWithInt: 1]     , @"firstChannel",
        [NSNumber numberWithBool: YES]  , @"useChannel01",
        [NSNumber numberWithBool: YES]  , @"useChannel02",
        [NSNumber numberWithBool: YES]  , @"useChannel03",
        [NSNumber numberWithBool: YES]  , @"useChannel04",
        [NSNumber numberWithBool: YES]  , @"useChannel05",
        [NSNumber numberWithBool: YES]  , @"useChannel06",
        [NSNumber numberWithBool: YES]  , @"useChannel07",
        [NSNumber numberWithBool: YES]  , @"useChannel08",
        [NSNumber numberWithBool: YES]  , @"useChannel09",
        [NSNumber numberWithBool: YES]  , @"useChannel10",
        [NSNumber numberWithBool: YES]  , @"useChannel11",
        [NSNumber numberWithBool: NO]   , @"useChannel12",
        [NSNumber numberWithBool: NO]   , @"useChannel13",
        [NSNumber numberWithBool: NO]   , @"useChannel14",
        [NSNumber numberWithInt: 0]     , @"injectionDevice",
        [NSNumber numberWithInt: 0]     , @"dumpFilter",
        @"~/DumpLog %y-%m-%d %H:%M"    , @"dumpDestination",
        [NSClassFromString(driverClassName) deviceName], @"deviceName", //todo make this unique for ever instance
        nil]];
    [controller setObject:drivers forKey:@"ActiveDrivers"];
    
	if (([_driver indexOfSelectedItem] == 1) && ![_aeForever state] && ![WaveHelper isServiceAvailable:"AirPort_Athr5424"]) {
		// user has chosen Airport Extreme - STRONGLY suggest enabling persistent passive mode
		result = NSRunAlertPanel(NSLocalizedString(@"Please enable persistent Airport Extreme passive.", "Persistent dialog title"),
								 NSLocalizedString(@"Airport Extreme passive may not work without persistent passive support enabled.  Some users have reported errors and even system crashes when attempting to use without persistent passive support.  Enable persistent passive support now?", "Persistent dialog description"),
								 NSLocalizedString(@"Yes please!","Yes button"), NSLocalizedString(@"No, I like kernel panics.","No button"), nil);
		if (result == 1) {
			[_aeForever setState:1];
			[self enableAEForever:_aeForever];
		} else {
			NSRunAlertPanel(NSLocalizedString(@"Don't say we didn't warn you!", "Persistent dialog title"),
							NSLocalizedString(@"There's just no helping some people.", "Persistent dialog description"),
							OK,nil, nil);
		}
	}
	
    [_driverTable reloadData];
    [_driverTable selectRow:[drivers count]-1 byExtendingSelection:NO];
    [self updateUI];
    [drivers release];
}

- (IBAction)selRemoveDriver:(id)sender {
    int i;
    NSMutableArray *drivers;
    
    i = [_driverTable selectedRow];
    if (i < 0) return;
    
    drivers = [[controller objectForKey:@"ActiveDrivers"] mutableCopy];
    [drivers removeObjectAtIndex:i];
    [controller setObject:drivers forKey:@"ActiveDrivers"];    
    
    [_driverTable reloadData];
    [self updateUI];
    [drivers release];
}

- (IBAction)selAll:(id)sender {
    [_channelSel selectAll:self];
    [self setValueForSender:_channelSel];
}

- (IBAction)selNone:(id)sender {
    [_channelSel deselectAllCells];
    [self setValueForSender:_channelSel];
}

- (IBAction)enableAEForever:(id)sender {
    if (NSAppKitVersionNumber < 824.11) {
		NSLog(@"MacOS is not 10.4.2! AppKitVersion: %f < 824.11", NSAppKitVersionNumber);
		
		NSRunCriticalAlertPanel(
                                NSLocalizedString(@"Could not enable Monitor Mode for Airport Extreme.", "Error dialog title"),
                                NSLocalizedString(@"Incompatible MacOS version! You will need at least MacOS 10.4.2!.", "Error dialog description"),
                                OK, nil, nil);
        return;
	}
	if ([_aeForever state] == NSOnState && [WaveHelper isServiceAvailable:"AirPort_Athr5424"]) {
		[_aeForever setState:NSOffState];
		NSRunCriticalAlertPanel(
                                NSLocalizedString(@"Not Needed.", "Error dialog title"),
                                NSLocalizedString(@"Atheros based Airport Extreme cards keep track of monitor mode themselves.", "Error dialog description"),
                                OK, nil, nil);
        return;
	
	}
    [WaveDriverAirportExtreme setMonitorMode: [_aeForever state] == NSOnState];
    [self setValueForSender:sender];
    NSRunCriticalAlertPanel(
                            NSLocalizedString(@"You Must Reboot.", "Error dialog title"),
                            NSLocalizedString(@"You must reboot after changing this setting for it to take effect.", "Error dialog description"),
                            OK, nil, nil);
}

@end

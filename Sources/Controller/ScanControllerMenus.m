/*
        
        File:			ScanControllerMenus.m
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

#import <BIGeneric/ColoredRowTableView.h>
#import "ScanController.h"
#import "ScanControllerPrivate.h"
#import "ScanControllerScriptable.h"
#import "KisMACNotifications.h"
#import "../WindowControllers/DownloadMapController.h"
#import "DecryptController.h"
#import "GPSInfoController.h"
#import "HTTPStream.h"
#import "../Core/KismetXMLImporter.h"
#import "../Crypto/WPA.h"
#import "TrafficController.h"
#import "../WaveDrivers/WaveDriver.h"
#import "MapView.h"
#import "MapViewAreaView.h"
#import "WaveStorageController.h"
#import "WaveNet.h"
#import "FSWindow.h"

@implementation ScanController(MenuExtension)

#pragma mark -
#pragma mark KISMAC MENU
#pragma mark -

- (IBAction)showPrefs:(id)sender {
    if(!prefsWindow) {
        if(![NSBundle loadNibNamed:@"Preferences" owner:self]) {
            NSLog(@"Preferences.nib failed to load!");
            return;
        }
    } else
        [prefsController refreshUI:self];
    
    if(![[NSUserDefaults standardUserDefaults] objectForKey:@"NSWindow Frame prefsWindow"])
        [prefsWindow center];

    [prefsWindow makeKeyAndOrderFront:nil];
}

#pragma mark -
#pragma mark FILE MENU
#pragma mark -

- (IBAction)importKismetXML:(id)sender {
    KismetXMLImporter * myImporter =  [[KismetXMLImporter alloc] init];
    
    aOP=[NSOpenPanel openPanel];
    [aOP setAllowsMultipleSelection:YES];
    [aOP setCanChooseFiles:YES];
    [aOP setCanChooseDirectories:NO];
    if ([aOP runModalForTypes:[NSArray arrayWithObjects:@"txt", @"xml", nil]]==NSOKButton) {
        [self stopActiveAttacks];
        [self stopScan];
        _refreshGUI = NO;
        
        int i;
        for (i = 0; i < [[aOP filenames] count]; i++) {
            NSString *file = [[aOP filenames] objectAtIndex:i];
            [self showBusyWithText: [NSString stringWithFormat: @"Importing %@ as Kismet XML", [file lastPathComponent]]];    
            [myImporter performKismetImport: file withContainer:_container];
            [self busyDone];
        }
        _refreshGUI = YES;
        
        [self updateNetworkTable:self complete:YES];
        [self refreshScanHierarch];
        [_window setDocumentEdited:YES];
        [[NSNotificationCenter defaultCenter] postNotificationName:KisMACViewItemChanged object:self];
    }
}

- (IBAction)importMapFromServer:(id)sender {
    DownloadMapController* dmc = [[DownloadMapController alloc] initWithWindowNibName:@"DownloadMap"];
    
    [[dmc window] setFrameUsingName:@"aKisMAC_DownloadMap"];
    [[dmc window] setFrameAutosaveName:@"aKisMAC_DownloadMap"];
    
    [dmc setCoordinates:[[WaveHelper gpsController] currentPoint]];
    [dmc showWindow:self];
    [[dmc window] makeKeyAndOrderFront:self];
}

- (IBAction)importNetstumbler:(id)sender {
    aOP=[NSOpenPanel openPanel];
    [aOP setAllowsMultipleSelection:NO];
    [aOP setCanChooseFiles:YES];
    [aOP setCanChooseDirectories:NO];
    if ([aOP runModalForTypes:[NSArray arrayWithObjects:@"txt", @"ns1", nil]]==NSOKButton) {
        [self stopActiveAttacks];
        [self stopScan];

        [self showBusy:@selector(performImportNetstumbler:) withArg:[aOP filename]];
        
        [[NSNotificationCenter defaultCenter] postNotificationName:KisMACViewItemChanged object:self];
    }
}

- (void)performImportNetstumbler:(NSString*)filename {
    [_importController setTitle:[NSString stringWithFormat:NSLocalizedString(@"Importing %@...", "Status for busy dialog"), filename]];  
    
    _refreshGUI = NO;
    [WaveStorageController importFromNetstumbler:filename withContainer:_container andImportController:_importController];
    _refreshGUI = YES;

    [self updateNetworkTable:self complete:YES];
    [self refreshScanHierarch];
    [_window setDocumentEdited:YES];
}

#pragma mark -

- (IBAction)exportNS:(id)sender {
    NSSavePanel *aSP;
    aSP=[NSSavePanel savePanel];
    [aSP setRequiredFileType:@"ns1"];
    [aSP setCanSelectHiddenExtension:YES];
    [aSP setTreatsFilePackagesAsDirectories:NO];
    if ([aSP runModal]==NSFileHandlingPanelOKButton) {
        [self showBusy:@selector(performExportNS:) withArg:[aSP filename]];
        if (_asyncFailure) [self showExportFailureDialog];
    }
}
- (void)performExportNS:(id)filename {
	[[WaveHelper scanController] checkFilter:self];
	
    [_importController setTitle:[NSString stringWithFormat:NSLocalizedString(@"Exporting to %@...", "Status for busy dialog"), filename]];  

    if (![WaveStorageController exportNSToFile:filename withContainer:_container andImportController:_importController]) _asyncFailure = YES;
    else _asyncFailure = NO;
	[[WaveHelper scanController] changeSearchValue:self];
}

- (void)performExportKML:(id)filename {
    [_importController setTitle:[NSString stringWithFormat:NSLocalizedString(@"Exporting to %@...", "Status for busy dialog"), filename]];  

    if (![WaveStorageController exportKMLToFile:filename withContainer:_container andImportController:_importController]) _asyncFailure = YES;
    else _asyncFailure = NO;
}

- (IBAction)exportKMLFile:(id)sender
{
    NSSavePanel *aSP;
    aSP=[NSSavePanel savePanel];
    [aSP setRequiredFileType:@"kml"];
    [aSP setCanSelectHiddenExtension:YES];
    [aSP setTreatsFilePackagesAsDirectories:NO];
    if ([aSP runModal]==NSFileHandlingPanelOKButton)
    {
        [self showBusy:@selector(performExportKML:) withArg:[aSP filename]];
    }
}

- (IBAction)exportWarD:(id)sender {
    NSSavePanel *aSP;
    
    aSP=[NSSavePanel savePanel];
    [aSP setRequiredFileType:@"txt"];
    [aSP setCanSelectHiddenExtension:YES];
    [aSP setTreatsFilePackagesAsDirectories:NO];
    if ([aSP runModal]==NSFileHandlingPanelOKButton) {
        [self showBusy:@selector(performExportWarD:) withArg:[aSP filename]];
        if (_asyncFailure) [self showExportFailureDialog];
    }
}
- (void)performExportWarD:(id)filename {
	[[WaveHelper scanController] checkFilter:self];
    [_importController setTitle:[NSString stringWithFormat:NSLocalizedString(@"Exporting to %@...", "Status for busy dialog"), filename]];  

    _asyncFailure = ! [[WaveStorageController webServiceDataOfContainer:_container andImportController:_importController] writeToFile:[filename stringByExpandingTildeInPath] atomically:YES encoding:NSASCIIStringEncoding error:NULL];
	[[WaveHelper scanController] changeSearchValue:self];
}

- (IBAction)exportToServer:(id)sender {
    NSUserDefaults *defs;

    defs = [NSUserDefaults standardUserDefaults];
    if (![defs boolForKey:@"useWebService"]) {
        NSBeginCriticalAlertSheet(
        NSLocalizedString(@"Export failed", "Export failure dialog title"),
        OK, NULL, NULL, _window, self, NULL, NULL, NULL, 
        NSLocalizedString(@"Server Export disabled", "LONG Export failure dialog text. Enable it in preferences")
        );
        return;
    }

    if ([[defs stringForKey:@"webServiceAccount"] length] == 0) {
        NSBeginCriticalAlertSheet(
        NSLocalizedString(@"Export failed", "Export failure dialog title"),
        OK, NULL, NULL, _window, self, NULL, NULL, NULL, 
        NSLocalizedString(@"No account name given. Enter it in the preferences.", "For export Server.")
        );
        return;
    }
    
    [self showBusy:@selector(performExportToServer:) withArg:[NSNumber numberWithBool:YES]];

	if (_asyncFailure) {
        NSBeginCriticalAlertSheet(
        NSLocalizedString(@"Export failed", "Export failure dialog title"),
        OK, NULL, NULL, _window, self, NULL, NULL, NULL, 
        _lastError
        );
    
	}
}

- (void)performExportToServer:(id)reportErrors {
	[[WaveHelper scanController] checkFilter:self];

    NSUserDefaults *defs;
    NSString *account, *password, *data, *errorStr;
    NSURL *url;
    HTTPStream *stream;
    
    [_importController setTitle:NSLocalizedString(@"Exporting to .kismac Server...", "Status for busy dialog")];  
    defs = [NSUserDefaults standardUserDefaults];

    if (![defs boolForKey:@"useWebService"]) {
        NSLog(@"Webserver export has been disabled!");
        return;
    }

    account = [defs stringForKey:@"webServiceAccount"];
    if (!account) {
        NSLog(@"Something is wrong with your webService preferences!");
        return;
    }

    password = [WaveHelper getPasswordForAccount:account];
    if (!password) {
        NSLog(@"Looks like KisMAC does not have access to it's WebService password. Cancel export.");
        return;
    }

    data = [WaveStorageController webServiceDataOfContainer:_container andImportController:_importController];
    
    // Create a new url based upon the user entered string
    url = [NSURL URLWithString: @"http://kismac.binaervarianz.de/_uploadnets.php"];
    
    stream = [[HTTPStream alloc] initWithURL:url andPostVariables:[NSDictionary dictionaryWithObjectsAndKeys:
        account,    @"user",
        password,   @"pass",
        data,       @"file",
        nil]
      reportErrors:[reportErrors boolValue]];
    
    if ([reportErrors boolValue] && [stream errorCode]!=201) {
        switch ([stream errorCode]) {
        case -1:
            errorStr = NSLocalizedString(@"Could not connect to Internet Server. Please check your internet connection and see if you can connect to http://binaervarianz.de.", "For export Server.");
            break;
        case 500:
            errorStr = NSLocalizedString(@"The Internet server reported an Internal Error. Please export your data and send it to us for debugging.", "For export Server.");
            break;
        case 401:
            errorStr = NSLocalizedString(@"Access denied. Please check your account and password settings.", "For export Server.");
            break;
        case 201:
            errorStr = @"";
            break;
        default:
            errorStr = NSLocalizedString(@"The Server answerd with an unknown error code! Please check for a new KisMAC version.", "For export Server.");
        }
		
		_asyncFailure = YES;
		[_lastError autorelease];
		_lastError = [errorStr retain];
	} else {
		_asyncFailure = NO;
	}
    [stream release];
	[[WaveHelper scanController] changeSearchValue:self];
}

- (IBAction)exportMacstumbler:(id)sender {
    NSSavePanel *aSP;
    
    aSP=[NSSavePanel savePanel];
    [aSP setRequiredFileType:@"txt"];
    [aSP setCanSelectHiddenExtension:YES];
    [aSP setTreatsFilePackagesAsDirectories:NO];
    if ([aSP runModal]==NSFileHandlingPanelOKButton) {
        [self showBusy:@selector(performExportMacStumbler:) withArg:[aSP filename]];
        if (_asyncFailure) [self showExportFailureDialog];
    }
}
- (void)performExportMacStumbler:(id)filename {
	[[WaveHelper scanController] checkFilter:self];
    
    [_importController setTitle:[NSString stringWithFormat:NSLocalizedString(@"Exporting to %@...", "Status for busy dialog"), filename]];  

    if (![WaveStorageController exportMacStumblerToFile:filename withContainer:_container andImportController:_importController]) _asyncFailure = YES;
    else _asyncFailure = NO;
	[[WaveHelper scanController] changeSearchValue:self];
}

- (IBAction)exportPDF:(id)sender {
    NSSavePanel *aSP;
    
    aSP=[NSSavePanel savePanel];
    [aSP setRequiredFileType:@"pdf"];
    [aSP setCanSelectHiddenExtension:YES];
    [aSP setTreatsFilePackagesAsDirectories:NO];
    if ([aSP runModal]==NSFileHandlingPanelOKButton) {
        [self showBusy:@selector(performExportPDF:) withArg:[aSP filename]];
        if (_asyncFailure) [self showExportFailureDialog];
    }
}
- (void)performExportPDF:(id)filename {
	[[WaveHelper scanController] checkFilter:self];

    NSData *data;
    [_importController setTitle:[NSString stringWithFormat:NSLocalizedString(@"Exporting to %@...", "Status for busy dialog"), filename]];  
    
    NS_DURING
        //TODO
        data = [_mappingView pdfData];
        [data writeToFile:[filename stringByExpandingTildeInPath] atomically:NO];
        _asyncFailure = NO;
    NS_HANDLER
        _asyncFailure = YES;
    NS_ENDHANDLER
	[[WaveHelper scanController] changeSearchValue:self];
}

- (IBAction)exportJPEG:(id)sender {
    NSSavePanel *aSP;
    
    aSP=[NSSavePanel savePanel];
    [aSP setRequiredFileType:@"jpg"];
    [aSP setCanSelectHiddenExtension:YES];
    [aSP setTreatsFilePackagesAsDirectories:NO];
    if ([aSP runModal]==NSFileHandlingPanelOKButton) {
        [self showBusy:@selector(performExportJPEG:) withArg:[aSP filename]];
        if (_asyncFailure) [self showExportFailureDialog];
    }
}
- (void)performExportJPEG:(id)filename {
	[[WaveHelper scanController] checkFilter:self];
    
    NSData *data;
    NSImage *img;
    [_importController setTitle:[NSString stringWithFormat:NSLocalizedString(@"Exporting to %@...", "Status for busy dialog"), filename]];  
    
    NS_DURING
        img  = [[NSImage alloc] initWithData:[_mappingView pdfData]];
        data = [img TIFFRepresentationUsingCompression:NSTIFFCompressionNone factor:0.0];
        data = [[NSBitmapImageRep imageRepWithData:data] representationUsingType:NSJPEGFileType properties:nil];
            
        [data writeToFile:[filename stringByExpandingTildeInPath] atomically:NO];
        [img release];
        
        _asyncFailure = NO;
    NS_HANDLER
        _asyncFailure = YES;
    NS_ENDHANDLER
	[[WaveHelper scanController] changeSearchValue:self];
}
    
#pragma mark -

- (IBAction)decryptPCAPFile:(id)sender {
    DecryptController* d;
    
    d = [[DecryptController alloc] initWithWindowNibName:@"DecryptDialog"];
    [d showWindow:sender];
}

#pragma mark -
#pragma mark CHANNEL MENU
#pragma mark -

- (IBAction)selChannel:(id)sender {
    WaveDriver *wd;
    NSMutableDictionary *md;
    int y;
    int newChannel = [[[sender title] substringFromIndex:8] intValue];
    
    wd = [WaveHelper driverWithName:_whichDriver];
    if (!wd) {
        NSLog(@"Error: invalid driver selected (%@)", _whichDriver);
        return;
    }
    
    md = [[wd configuration] mutableCopy];
    for(y=1; y<15; y++)
        [md setObject:[NSNumber numberWithInt:(y==newChannel) ? 1 : 0] forKey:[NSString stringWithFormat:@"useChannel%.2i",y]];
  
    [wd setConfiguration: md];
    [md release];

    [self updateChannelMenu];
}

- (IBAction)selChannelRange:(id)sender {
    WaveDriver *wd;
    NSMutableDictionary *md;
    int y;
    
    wd = [WaveHelper driverWithName:_whichDriver];
    if (!wd) {
        NSLog(@"Error: invalid driver selected");
        return;
    }
    
    md = [[wd configuration] mutableCopy];
    if ([[sender title] isEqualToString:NSLocalizedString(@"All FCC/IC Channels (1-11)", "menu item. needs to be the same as in MainMenu.nib")]) {
        for(y=1; y<=11; y++)
            [md setObject:[NSNumber numberWithInt:1] forKey:[NSString stringWithFormat:@"useChannel%.2i", y]];

        [md setObject:[NSNumber numberWithInt:0] forKey:[NSString stringWithFormat:@"useChannel%.2i", 12]];
        [md setObject:[NSNumber numberWithInt:0] forKey:[NSString stringWithFormat:@"useChannel%.2i", 13]];
     } else {
        for(y=1; y<=13; y++)
            [md setObject:[NSNumber numberWithInt:1] forKey:[NSString stringWithFormat:@"useChannel%.2i", y]];
    }
    
    [wd setConfiguration: md];
    [md release];
    
    [self updateChannelMenu];
}

- (IBAction)selDriver:(id)sender {
    NSUserDefaults *sets;

    sets = [NSUserDefaults standardUserDefaults];
    [sets setObject:[sender title] forKey:@"whichDriver"];
    [self updateChannelMenu];
}

- (IBAction)setAutoAdjustTimer:(id)sender {
    WaveDriver *wd;
    NSMutableDictionary *md;
    
    wd = [WaveHelper driverWithName:_whichDriver];
    if (!wd) {
        NSLog(@"Error: invalid driver selected");
        return;
    }
    
    md = [[wd configuration] mutableCopy];
    [md setObject:[NSNumber numberWithBool:(([sender state]==NSOffState) ? YES : NO)] forKey:@"autoAdjustTimer"];
 
    [wd setConfiguration: md];
    [md release];
    
    [self updateChannelMenu];

}
#pragma mark -
#pragma mark NETWORK MENU
#pragma mark -

- (IBAction)clearNetwork:(id)sender {
    WaveNet* net = _curNet;
    
    if (!_curNet) {
        NSBeep();
        return;
    }
    
    if (sender!=self) {
        NSBeginAlertSheet(
            NSLocalizedString(@"Really want to delete?", "Network deletion dialog title"),
            NSLocalizedString(@"Delete", "Network deletion dialog button"),
            NSLocalizedString(@"Delete and Filter", "Network deletion dialog button"),
            CANCEL, _window, self, NULL, @selector(reallyWantToDelete:returnCode:contextInfo:), self,
            NSLocalizedString(@"Network deletion dialog text", "LONG description of what this dialog does")
            //@"Are you sure that you whish to delete the network? This action cannot be undone. You may also choose to add the network to the filter list in the preferences and prevent it from re-appearing."
            );
        return;
    }
           
    [_window setDocumentEdited:YES];
    
    [self clearAreaMap];
    [self hideDetails];
    [self showNetworks];
    [_networkTable deselectAll:self];
    
    if (net) {
        if ([[net ID] isEqualToString:_activeAttackNetID]) [self stopActiveAttacks];
		[_container clearEntry:net];
    }
    _curNet = Nil;
    
    [self refreshScanHierarch];
    [self updateNetworkTable:self complete:YES];
}

- (void)reallyWantToDelete:(NSWindow *)sheet returnCode:(int)returnCode contextInfo:(void *)contextInfo {
    NSUserDefaults *sets;
    NSMutableArray *temp;
    NSString *mac;
    
    switch (returnCode) {
    case NSAlertDefaultReturn:
        [self clearNetwork:self];
    case NSAlertOtherReturn:
        break;
    case NSAlertAlternateReturn:
    default:
        sets=[NSUserDefaults standardUserDefaults];
        temp = [NSMutableArray arrayWithArray:[sets objectForKey:@"FilterBSSIDList"]];
        mac = [_curNet ID];
        
        if (mac!=Nil && [temp indexOfObject:mac]==NSNotFound) {
            [temp addObject:mac];
            [sets setObject:temp forKey:@"FilterBSSIDList"];
        }
        [[NSNotificationCenter defaultCenter] postNotificationName:KisMACFiltersChanged object:self];

        [self clearNetwork:self];
    }
}

- (IBAction)joinNetwork:(id)sender {
    [_curNet joinNetwork];
}

#pragma mark -

- (IBAction)injectPackets:(id)sender {

	if ([_curNet type] != networkTypeManaged) {
		[_window showAlertMessage: NSLocalizedString(@"KisMAC can only attack managed networks!", "Error for packet reinjection") title: NSLocalizedString(@"Re-Injection failed", "Error for packet reinjection") button: NULL];
		return;
    }
	if ([_curNet wep] != encryptionTypeWEP && [_curNet wep] != encryptionTypeWEP40) {
		[_window showAlertMessage: NSLocalizedString(@"You can only reinject into WEP encrypted networks!", "Error for packet reinjection") title: NSLocalizedString(@"Re-Injection failed", "Error for packet reinjection") button: NULL];
		return;
    }
	
    if ([aInjPacketsMenu state]==NSOffState && [self startActiveAttack]) {
        //either we are already active or we have to load the driver
        if (!_scanning) [self startScan];
        
        _crackType = 5;
        [self startCrackDialogWithTitle:NSLocalizedString(@"Setting up packet reinjection...", "busy dialog") stopScan:NO];
        [_curNet reinjectWithImportController:_importController andScanner:scanner];
    
    } else {
        [self stopActiveAttacks];
    }
	
}
- (IBAction)deautheticateNetwork:(id)sender {
	NSUserDefaults *defs;
	defs = [NSUserDefaults standardUserDefaults];
    if ([_deauthMenu state]==NSOffState && [self startActiveAttack] && [scanner deauthenticateNetwork:_curNet atInterval:[[defs objectForKey:@"pr_interval"] intValue]]) {
        [_deauthMenu setState:NSOnState];
        [_deauthMenu setTitle:[NSLocalizedString(@"Deauthenticating ", "menu item") stringByAppendingString:[_curNet BSSID]]];
    } else {
        [self stopActiveAttacks];
    }
}
- (IBAction)deautheticateAllNetworks:(id)sender {
    if ([sender state]==NSOffState && [self startActiveAttack]) {
		if (!_scanning) [self startScan];
		[scanner setDeauthingAll:YES];
        [sender setState:NSOnState];
    } else {
        [self stopActiveAttacks];
    }
}
- (IBAction)authFloodNetwork:(id)sender {
    if ([_authFloodMenu state]==NSOffState && [self startActiveAttack] && [scanner authFloodNetwork:_curNet]) {
        [_authFloodMenu setState:NSOnState];
        [_authFloodMenu setTitle:[NSLocalizedString(@"Flooding ", "menu item") stringByAppendingString:[_curNet BSSID]]];
    } else {
        [self stopActiveAttacks];
    }
}

- (IBAction)monitorSignal:(id)sender {
	   if ([_monitorMenu state]==NSOffState) {
		   [_monitorMenu setState:NSOnState];
		   
		   [_monitorAllMenu setState:NSOffState];

		   [_monitorMenu setTitle:[NSLocalizedString(@"Monitoring ", "menu item") stringByAppendingString:[_curNet BSSID]]];
		   
		   [WaveNet setTrackString:[_curNet BSSID]];
		   [WaveNet setTrackStringClient:@"any"];
	   } else {
			[_monitorMenu setState:NSOffState];
			[_monitorMenu setTitle:NSLocalizedString(@"Monitor Signal Strength", "menu item")];

			[WaveNet setTrackString:@""];
			[WaveNet setTrackStringClient:@""];
	   }	
}

- (IBAction)monitorAllNetworks:(id)sender {
	if ([_monitorAllMenu state]==NSOffState) {
		[_monitorAllMenu setState:NSOnState];
		
		[_monitorMenu setState:NSOffState];
		[_monitorMenu setTitle:NSLocalizedString(@"Monitor Signal Strength", "menu item")];
		
		[WaveNet setTrackString:@"any"];
		[WaveNet setTrackStringClient:@"any"];
	   } else {
		   [_monitorAllMenu setState:NSOffState];
		   [_monitorAllMenu setTitle:NSLocalizedString(@"Monitor all signals", "menu item")];
		   [WaveNet setTrackString:@""];
		   [WaveNet setTrackStringClient:@""];
	   }	
}

#pragma mark -
#pragma mark MAP MENU
#pragma mark -


- (void)showAreaDone:(NSWindow *)sheet returnCode:(int)returnCode contextInfo:(void *)contextInfo {
    _importOpen--;
	NSParameterAssert(_importOpen == 0);
	[self menuSetEnabled:YES menu:[NSApp mainMenu]];

    [[_importController window] close];
    [_importController stopAnimation];
	
	if ([_importController canceled]) {
		[self clearAreaMap];
		[_showNetInMap setTitle:@"Show Net Area"];
		[_showNetInMap setState: NSOffState];
	} else {
		[_showNetInMap setTitle:[NSLocalizedString(@"Show Net Area of ", "menu item") stringByAppendingString:[_curNet BSSID]]];
		[_showNetInMap setState: NSOnState];
	}
		
	[self showMap];
	[WaveHelper secureRelease:&_importController];   
}

- (IBAction)showCurNetArea:(id)sender {
   if ([sender state] == NSOffState) {
        if (![[WaveHelper mapView] hasValidMap]) {
			[_window showAlertMessage:NSLocalizedString(@"You have to load a map in order to perform this action", "area mapping failure") title:NSLocalizedString(@"Area mapping failed", "error box title") button:nil];
			return;
		}
		
		[self stopScan];

		[self showBusyWithText:NSLocalizedString(@"Caching Map...", "Title of busy dialog") andEndSelector:@selector(showAreaDone:returnCode:contextInfo:) andDialog:@"Crack"];
        
        if ([_showAllNetsInMap state] == NSOnState) [self showAllNetArea:_showAllNetsInMap];

		[_mappingView showAreaNet:_curNet];
    } else {
        [self clearAreaMap];
    }
}

- (void)showAreaAllDone:(NSWindow *)sheet returnCode:(int)returnCode contextInfo:(void *)contextInfo {
    _importOpen--;
	NSParameterAssert(_importOpen == 0);
	[self menuSetEnabled:YES menu:[NSApp mainMenu]];

    [[_importController window] close];
    [_importController stopAnimation];
	
	if ([_importController canceled]) {
		[self clearAreaMap];
		[_showAllNetsInMap setState: NSOffState];
	} else [_showAllNetsInMap setState: NSOnState];
	
    [WaveHelper secureRelease:&_importController]; 
	[self showMap];
}


- (IBAction)showAllNetArea:(id)sender {
    NSMutableArray *a;
    unsigned int i;
    
    if ([sender state] == NSOffState) {
        if (![[WaveHelper mapView] hasValidMap]) {
			[_window showAlertMessage:NSLocalizedString(@"You have to load a map in order to perform this action", "area mapping failure") title:NSLocalizedString(@"Area mapping failed", "error box title") button:nil];
			return;
		}
        [self stopScan];
        
   		[self showBusyWithText:NSLocalizedString(@"Caching Map...", "Title of busy dialog") andEndSelector:@selector(showAreaAllDone:returnCode:contextInfo:) andDialog:@"Crack"];
     
         if ([_showNetInMap state] == NSOnState) [self showCurNetArea:_showNetInMap];
    
        a = [[[NSMutableArray alloc] init] autorelease];
        for (i=0; i<[_container count]; i++) [a addObject:[_container netAtIndex:i]];
        [_mappingView showAreaNets:[NSArray arrayWithArray:a]];
        
    } else {
        [self clearAreaMap];
    }
}

- (IBAction)restartGPS:(id)sender {
    NSString *lDevice;
    
    lDevice=[[NSUserDefaults standardUserDefaults] objectForKey:@"GPSDevice"];
    if ((lDevice!=nil)&&(![lDevice isEqualToString:@""])) {
        [[NSNotificationCenter defaultCenter] postNotificationName:KisMACGPSStatusChanged object:NSLocalizedString(@"Resetting GPS subsystem...", "gps status")];
        [WaveHelper initGPSControllerWithDevice: lDevice];
    } else {
        [[NSNotificationCenter defaultCenter] postNotificationName:KisMACGPSStatusChanged object:NSLocalizedString(@"GPS disabled", "LONG GPS status string with informations where to enable")];
    }
}

#pragma mark -
#pragma mark WINDOW MENU
#pragma mark -

- (IBAction)closeActiveWindow:(id)sender {
    [[NSApp keyWindow] performClose:sender];
}

- (IBAction)displayGPSInfo:(id)sender {
	
	if ([_showGPSDetails state]==NSOffState) {
		_g = [[GPSInfoController alloc] initWithWindowNibName:@"GPSDialog"];
		[_g setShowMenu:_showGPSDetails];
		[_showGPSDetails setState:NSOnState];
		[_g showWindow:sender];
		[WaveHelper setGPSInfoController:_g];
	   } else {
		[_showGPSDetails setState:NSOffState];
		[_g close];
		[_g release];
		[WaveHelper setGPSInfoController:NULL];
	   }
	
}

- (IBAction)goFullscreen:(id)sender {
	if ([_fullscreen state]==NSOffState) {
		borderlessWindow = [[FSWindow alloc] initWithContentRect:[[NSScreen mainScreen] frame] 
			styleMask:(NSTexturedBackgroundWindowMask) backing:NSBackingStoreBuffered defer:YES];
		[borderlessWindow setAlphaValue:0];
		[borderlessWindow setContentView:_mapView];
		[borderlessWindow makeKeyAndOrderFront:borderlessWindow];
		[borderlessWindow setLevel:kCGStatusWindowLevel + 1];	
		int i;
		for (i=0; i<10; i++) {
			[borderlessWindow setAlphaValue:[borderlessWindow alphaValue] + 0.1];
			[NSThread sleepUntilDate:[NSDate dateWithTimeIntervalSinceNow:0.05]];
		}
		[NSMenu setMenuBarVisible:NO];
		[borderlessWindow setLevel:kCGNormalWindowLevel];
		[[WaveHelper mainWindow] setIsVisible:NO];
		[borderlessWindow makeFirstResponder:_mappingView];
		[_fullscreen setState:NSOnState];
	} else {
		[borderlessWindow setLevel:kCGStatusWindowLevel + 1];
		[borderlessWindow makeKeyAndOrderFront:borderlessWindow];
		[[WaveHelper mainWindow] setIsVisible:YES];
		if (_visibleTab == tabMap) {
			[self changedViewTo:tabNetworks contentView:_networkView];
			[self changedViewTo:tabMap contentView:_mapView];
		}
		[NSMenu setMenuBarVisible:YES];
		int i;
		for (i=0; i<10; i++) {
			[borderlessWindow setAlphaValue:[borderlessWindow alphaValue] - 0.1];
			[NSThread sleepUntilDate:[NSDate dateWithTimeIntervalSinceNow:0.05]];
		}
		[borderlessWindow close];
		[[WaveHelper mainWindow] makeKeyAndOrderFront:[WaveHelper mainWindow]];
		[_fullscreen setState:NSOffState];
	}
}

#pragma mark -
#pragma mark HELP MENU
#pragma mark -

- (IBAction)openWebsiteURL:(id)sender {
    [[NSWorkspace sharedWorkspace] openURL:[NSURL URLWithString:@"http://kismac-ng.org"]];
}

- (IBAction)openDonateURL:(id)sender {
    [[NSWorkspace sharedWorkspace] openURL:[NSURL URLWithString:@"https://www.paypal.com/cgi-bin/webscr?cmd=_donations&business=donations@kismac-ng.org"]];
}

- (IBAction)openForumsURL:(id)sender{
    [[NSWorkspace sharedWorkspace] openURL:[NSURL URLWithString:@"http://forum.kismac-ng.org/"]];
}

- (IBAction)openFAQURL:(id)sender{
    [[NSWorkspace sharedWorkspace] openURL:[NSURL URLWithString:@"http://trac.kismac-ng.org/wiki/FAQ"]];
}

- (IBAction)showContextHelp:(id)sender {
    switch(_visibleTab) {
    case tabNetworks:
        [[NSHelpManager sharedHelpManager] openHelpAnchor:@"KisMAC_Main_View" inBook:@"KisMAC Help"];
        break;
    case tabTraffic:
        [[NSHelpManager sharedHelpManager] openHelpAnchor:@"KisMAC_Traffic_View" inBook:@"KisMAC Help"];
        break;
    case tabMap:
        [[NSHelpManager sharedHelpManager] openHelpAnchor:@"KisMAC_Map_View" inBook:@"KisMAC Help"];
        break;
    case tabDetails:
        [[NSHelpManager sharedHelpManager] openHelpAnchor:@"KisMAC_Details_View" inBook:@"KisMAC Help"];
        break;
    default:
        NSAssert(NO, @"invalid visible tab");
    }
}

#pragma mark -
#pragma mark DEBUG MENU
#pragma mark -

- (IBAction)debugSaveStressTest:(id)sender {
    [NSThread detachNewThreadSelector:@selector(doDebugSaveStressTest:) toTarget:self withObject:nil];
}

- (IBAction)doDebugSaveStressTest:(id)anObject {
    NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
    int i;
    
    for (i=0; i< 1500; i++) {
        if (![self save:@"~/stressTest.kismac"]) {
            NSLog(@"Stress test broken!");
            break;
        }
        [NSThread sleepUntilDate:[NSDate dateWithTimeIntervalSinceNow:1]];
    }
    [pool release];
}

- (IBAction)gpsDebugToConsole:(id)sender {
    if ([sender state] == NSOffState) {
        [[WaveHelper gpsController] writeDebugOutput:YES];
        [sender setState: NSOnState];
    } else {
        [[WaveHelper gpsController] writeDebugOutput:NO];
        [sender setState: NSOffState];
    }
}


- (IBAction)debugBeaconFlood:(id)sender {
    if ([sender state]==NSOffState) {
        [self stopActiveAttacks];
        if (![scanner beaconFlood]) {
            NSLog(@"Could not start injectiong beacons like hell. Did you choose an injection driver?\n");
            return;
        }
        [sender setState:NSOnState];
    } else {
        [self stopActiveAttacks];
        [sender setState:NSOffState];
    }
}

- (IBAction)debugTestWPAHashingFunction:(id)sender {
    UInt8 output[40];
    int i, j;
    NSMutableString *ms;
    
    if (!wpaTestPasswordHash()) NSLog(@"WPA hash test failed");
    else NSLog(@"WPA hash test succeeded");
    
    wpaPasswordHash("password", (const UInt8*)"IEEE", 4, output);
    ms = [NSMutableString string];
    for (i=0; i < WPA_PMK_LENGTH; i++) {
        j = output[i];
        [ms appendFormat:@"%.2x", j];
    }
    NSLog(@"Testvector 1 returned: %@", ms);
    
    wpaPasswordHash("ThisIsAPassword", (const UInt8*)"ThisIsASSID", 11, output);
    ms = [NSMutableString string];
    for (i=0; i < WPA_PMK_LENGTH; i++) {
        j = output[i];
        [ms appendFormat:@"%.2x", j];
    }
    NSLog(@"Testvector 2 returned: %@", ms);
    
}

- (IBAction)debugExportTrafficView:(id)sender {
    [_trafficController outputTIFFTo:@"/test.tiff"];
}


@end

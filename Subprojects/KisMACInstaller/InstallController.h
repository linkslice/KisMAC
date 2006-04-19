/*
        
        File:			InstallController.h
        Program:		KisMAC
		Author:			Michael Rossberg
						mick@binaervarianz.de
		Description:	KisMAC is a wireless stumbler for MacOS X.
                
        This file is part of the KisMAC Installer.

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
#import <AppKit/AppKit.h>

enum states {
    stateWelcome = 0,
    stateLicense = 1,
    stateRemovePrefs = 2,
    stateInstallSFPatch = 3,
    stateInstallDir = 4,
    stateDoInstall = 5,
    stateConfigure = 6,
    stateConfirmConfigure = 7,
	stateEnableMonitorMode = 8,
    stateInstallDone = 9,
    stateInstallCanceled = 10,
    
    stateCleanKisMAC = 97,
    stateRemovingKisMAC = 98,
    stateRemovalDone = 99,
};

@interface InstallController : NSWindowController {
    IBOutlet NSButton		*_next;
    IBOutlet NSButton		*_prev;
    IBOutlet NSBox			*_mainBox;
    
    IBOutlet NSView			*_welcomeView;
	IBOutlet NSButtonCell   *_installKisMAC;
	IBOutlet NSButtonCell   *_removeKisMAC;

    IBOutlet NSView			*_licenseView;
    
    IBOutlet NSView			*_removePreferencesView;
	IBOutlet NSButtonCell   *_keepPrefs;
	IBOutlet NSButtonCell   *_removePrefs;

    IBOutlet NSView			*_installSFPatchView;
	IBOutlet NSButtonCell   *_installPatch;
	IBOutlet NSButtonCell   *_skipPatch;

    IBOutlet NSView			*_installDirectoryView;
	IBOutlet NSTextField    *_targetDirectory;

    IBOutlet NSView					*_installView;
	IBOutlet NSTextField            *_installStatus;
	IBOutlet NSProgressIndicator    *_progBar;
    
    IBOutlet NSView					*_configureView;
    
    IBOutlet NSView					*_confirmConfigureView;
	IBOutlet NSPopUpButton			*_selectedDriver;
    
	IBOutlet NSView					*_enableMonitorModeView;
	IBOutlet NSButtonCell			*_enableMonitorMode;
	IBOutlet NSButtonCell			*_skipMonModeInstallation;
		    
    IBOutlet NSView					*_installDoneView;
	IBOutlet NSTextField			*_installationStatus;
	IBOutlet NSTextField			*_restartWarning;
    
    IBOutlet NSView					*_removeKisMACView;
	IBOutlet NSTextField            *_removeStatus;
	IBOutlet NSProgressIndicator    *_removeBar;
    
    IBOutlet NSView					*_removalDoneView;
	IBOutlet NSTextField			*_removeRestartWarning;
        
    enum states						_currentState;
    BOOL							_prevEnabled;
    BOOL							_nextEnabled;
	BOOL							_shallReboot;
}

-(IBAction)next:(id)sender;
-(IBAction)prev:(id)sender;
-(IBAction)choseTargetDirectory:(id)sender;

-(void)updateState;

@end

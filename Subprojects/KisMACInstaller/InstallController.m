/*
        
        File:			InstallController.m
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
#import "InstallController.h"
#import "BLAuthentication.h"
#import <Carbon/Carbon.h>

struct identStruct {
    UInt16 vendor;
    UInt16 device;
};

OSStatus SendAppleEventToSystemProcess(AEEventID EventToSend)
{
    AEAddressDesc targetDesc;
    static const ProcessSerialNumber
         kPSNOfSystemProcess = { 0, kSystemProcess };
    AppleEvent eventReply = {typeNull, NULL};
    AppleEvent appleEventToSend = {typeNull, NULL};

    OSStatus error = noErr;

    error = AECreateDesc(typeProcessSerialNumber,
        &kPSNOfSystemProcess, sizeof(kPSNOfSystemProcess),
        &targetDesc);

    if (error != noErr)
    {
        return(error);
    }

    error = AECreateAppleEvent(kCoreEventClass, EventToSend,
                     &targetDesc, kAutoGenerateReturnID,
                     kAnyTransactionID, &appleEventToSend);

    AEDisposeDesc(&targetDesc);

    if (error != noErr)
    {
        return(error);
    }

    error = AESend(&appleEventToSend, &eventReply, kAENoReply,
             kAENormalPriority, kAEDefaultTimeout,
             NULL, NULL);

    AEDisposeDesc(&appleEventToSend);

    if (error != noErr)
    {
        return(error);
    }

    AEDisposeDesc(&eventReply);

    return(error); //if this is noErr then we are successful
}

@implementation InstallController

- (id)init {
    self=[super init];
    if (self==Nil) return Nil;

    _currentState = stateWelcome;
    _nextEnabled  = YES;
    _shallReboot  = NO;
	[NSApp setDelegate:self];
	
    return self;
}

- (void)awakeFromNib {
    [self updateState];
	[_prev setEnabled:_prevEnabled];
}

#pragma mark -

- (void)reboot {
    SendAppleEventToSystemProcess(kAERestart);
}
- (BOOL)findFile:(NSString*)file {
    NSFileManager *m;
    
    m = [NSFileManager defaultManager];
    return [m fileExistsAtPath:[file stringByExpandingTildeInPath]];
}
- (BOOL)enableMonitorModeForFile:(NSString*)fileName andEnable:(BOOL)enable {
	NSDictionary *dict;
	NSData *data;
	[[BLAuthentication sharedInstance] executeCommand:@"/bin/chmod" withArgs:[NSArray arrayWithObjects:@"0666", fileName, nil]];
	
	[NSThread sleepUntilDate:[NSDate dateWithTimeIntervalSinceNow:1.0]];
	data = [NSData dataWithContentsOfFile:fileName];
	if(!data) return NO;
	dict = [NSPropertyListSerialization propertyListFromData:data mutabilityOption:kCFPropertyListMutableContainers format:NULL errorDescription:Nil];
	if(!dict) return NO;
	[dict setValue:[NSNumber numberWithBool:enable] forKeyPath:@"IOKitPersonalities.Broadcom PCI.APMonitorMode"];
	[[NSPropertyListSerialization dataFromPropertyList:dict format:kCFPropertyListXMLFormat_v1_0 errorDescription:nil] writeToFile:fileName atomically:NO];
		
	[[BLAuthentication sharedInstance] executeCommand:@"/bin/chmod" withArgs:[NSArray arrayWithObjects:@"0644", fileName, nil]];
	return YES;	
}
- (BOOL)enableMonitorMode:(BOOL)enable {
	BOOL ret;
	_shallReboot = YES;
	ret = [self enableMonitorModeForFile:@"/System/Library/Extensions/AppleAirPort2.kext/Contents/Info.plist" andEnable:enable] || 
	[self enableMonitorModeForFile:@"/System/Library/Extensions/IO80211Family.kext/Contents/PlugIns/AppleAirPortBrcm4311.kext/Contents/Info.plist" andEnable:enable];
	
	[[BLAuthentication sharedInstance] executeCommand:@"/bin/rm" withArgs:[NSArray arrayWithObject:@"/System/Library/Extensions.kextcache"]];
	//[[BLAuthentication sharedInstance] executeCommand:@"/usr/sbin/kextcache" withArgs:[NSArray arrayWithObjects:@"-k", @"/System/Library/Extensions", nil]];
	[[BLAuthentication sharedInstance] executeCommand:@"/bin/rm" withArgs:[NSArray arrayWithObject:@"/System/Library/Extensions.mkext"]];
	
	return ret;
}
- (BOOL)monitorModeEnabled {
	NSDictionary *dict;
	NSData *fileData;
	
	fileData = [NSData dataWithContentsOfFile:@"/System/Library/Extensions/AppleAirPort2.kext/Contents/Info.plist"];
	dict = [NSPropertyListSerialization propertyListFromData:fileData mutabilityOption:kCFPropertyListImmutable format:NULL errorDescription:Nil];
	if ([[dict valueForKeyPath:@"IOKitPersonalities.Broadcom PCI.APMonitorMode"] boolValue]) return YES;
	
	fileData = [NSData dataWithContentsOfFile:@"/System/Library/Extensions/IO80211Family.kext/Contents/PlugIns/AppleAirPortBrcm4311.kext/Contents/Info.plist"];
	dict = [NSPropertyListSerialization propertyListFromData:fileData mutabilityOption:kCFPropertyListImmutable format:NULL errorDescription:Nil];
	if ([[dict valueForKeyPath:@"IOKitPersonalities.Broadcom PCI.APMonitorMode"] boolValue]) return YES;
	
	return NO;
}
- (BOOL)findWirelessDriverPatch {
    return [self findFile:@"/System/Library/Extensions/WirelessDriver.kext/bipatch"];
}
- (BOOL)findWirelessDriver {
    return [self findFile:@"/System/Library/Extensions/WirelessDriver.kext"];
}
- (BOOL)findKisMACPrefs {
    return [self findFile:@"~/Library/Preferences/org.kismac-ng.kismac.plist"];
}
- (BOOL)findTAR {
	return [self findFile:@"/usr/bin/tar"];
}

- (NSString*)getPreviousInstallDir {
    NSUserDefaults *d;
    NSString *s;
    
    d = [NSUserDefaults standardUserDefaults];
    [d addSuiteNamed:@"org.kismac-ng.kismac"];
    
    s = [d objectForKey:@"KisMACHomeDir"];
    if (s) return [s stringByDeletingLastPathComponent];
    return @"/Applications";
}

- (BOOL)validDir:(NSString*)dir {
    NSFileManager *m;
    BOOL isDir;
    NSString *d;
    
    d = [dir stringByExpandingTildeInPath];
    
    m = [NSFileManager defaultManager];
    if (![m fileExistsAtPath:d isDirectory:&isDir]) return NO;
    if (!isDir) return NO;
    
    if (![m isWritableFileAtPath:d]) return NO;
    
    return YES;
}

- (BOOL)removePreferences {
    BLAuthentication *a;
    
    a = [BLAuthentication sharedInstance];
    return [a executeCommandSynced:[NSString stringWithFormat:@"%@/remove_prefs.sh", [[NSBundle mainBundle] resourcePath]] withArgs:nil];
}

- (BOOL)removeKisMACInstallation:(NSString*)targetDir {
    BLAuthentication *a;
    
    a = [BLAuthentication sharedInstance];
    return [a executeCommandSynced:[NSString stringWithFormat:@"%@/remove.sh", [[NSBundle mainBundle] resourcePath]] withArgs:[NSArray arrayWithObject:targetDir]];
}

- (BOOL)installKisMACToPath:(NSString*)targetDir {
    BLAuthentication *a;
    
    a = [BLAuthentication sharedInstance];
    return [a executeCommandSynced:[NSString stringWithFormat:@"%@/install.sh", [[NSBundle mainBundle] resourcePath]] withArgs:[NSArray arrayWithObjects:targetDir, [[NSBundle mainBundle] resourcePath], nil]];
}

- (BOOL)adjustKisMACPermissionsAtPath:(NSString*)targetDir {
    BLAuthentication *a;
    
    a = [BLAuthentication sharedInstance];
    return [a executeCommandSynced:[NSString stringWithFormat:@"%@/permissions.sh", [[NSBundle mainBundle] resourcePath]] withArgs:[NSArray arrayWithObject:targetDir]];
}

- (BOOL)removeWirelessDriverPatch {
    BLAuthentication *a;
    
	_shallReboot = YES;
    a = [BLAuthentication sharedInstance];
    return [a executeCommandSynced:[NSString stringWithFormat:@"%@/sfpatch_remove.sh", [[NSBundle mainBundle] resourcePath]] withArgs:nil];
}

- (BOOL)installWirelessPatchToPath:(NSString*)targetDir {
    BLAuthentication *a;
    
	_shallReboot = YES;
    a = [BLAuthentication sharedInstance];
    return [a executeCommandSynced:[NSString stringWithFormat:@"%@/sfpatch_install.sh", [[NSBundle mainBundle] resourcePath]] withArgs:[NSArray arrayWithObjects:targetDir, [[NSBundle mainBundle] resourcePath], nil]];
}

- (BOOL)adjustWirelessPatchPermissions {
    BLAuthentication *a;
    
    a = [BLAuthentication sharedInstance];
    return [a executeCommandSynced:[NSString stringWithFormat:@"%@/sfpatch_permissions.sh", [[NSBundle mainBundle] resourcePath]] withArgs:nil];
}

- (bool)usbPrismDeviceAvailable {
    mach_port_t masterPort;
    io_iterator_t iterator;
    io_object_t sdev;
    NSNumber *vendor, *product;
    int i;
    struct identStruct devices[] = {
        { 0x04bb, 0x0922}, //1 IOData AirPort WN-B11/USBS
        { 0x07aa, 0x0012}, //2 Corega Wireless LAN USB Stick-11
        { 0x09aa, 0x3642}, //3 Prism2.x 11Mbps WLAN USB Adapter
        { 0x1668, 0x0408}, //4 Actiontec Prism2.5 11Mbps WLAN USB Adapter
        { 0x1668, 0x0421}, //5 Actiontec Prism2.5 11Mbps WLAN USB Adapter
        { 0x066b, 0x2212}, //6 Linksys WUSB11v2.5 11Mbps WLAN USB Adapter
        { 0x066b, 0x2213}, //7 Linksys WUSB12v1.1 11Mbps WLAN USB Adapter
        { 0x067c, 0x1022}, //8 Siemens SpeedStream 1022 11Mbps WLAN USB Adapter
        { 0x049f, 0x0033}, //9 Compaq/Intel W100 PRO/Wireless 11Mbps multiport WLAN Adapter
        { 0x0411, 0x0016}, //10 Melco WLI-USB-S11 11Mbps WLAN Adapter
        { 0x08de, 0x7a01}, //11 PRISM25 IEEE 802.11 Mini USB Adapter
        { 0x8086, 0x1111}, //12 Intel PRO/Wireless 2011B LAN USB Adapter
        { 0x0d8e, 0x7a01}, //13 PRISM25 IEEE 802.11 Mini USB Adapter
        { 0x045e, 0x006e}, //14 Microsoft MN510 Wireless USB Adapter
        { 0x0967, 0x0204}, //15 Acer Warplink USB Adapter
        { 0x0cde, 0x0002}, //16 Z-Com 725/726 Prism2.5 USB/USB Integrated
        { 0x413c, 0x8100}, //17 Dell TrueMobile 1180 Wireless USB Adapter
        { 0x0b3b, 0x1601}, //18 ALLNET 0193 11Mbps WLAN USB Adapter
        { 0x0b3b, 0x1602}, //19 ZyXEL ZyAIR B200 Wireless USB Adapter
        { 0x0baf, 0x00eb}, //20 USRobotics USR1120 Wireless USB Adapter
        { 0x0411, 0x0027}, //21 Melco WLI-USB-KS11G 11Mbps WLAN Adapter
        { 0x04f1, 0x3009}, //22 JVC MP-XP7250 Builtin USB WLAN Adapter
        { 0x03f3, 0x0020}, //23 Adaptec AWN-8020 USB WLAN Adapter
        { 0x0ace, 0x1201}, //24 ZyDAS ZD1201 Wireless USB Adapter
        { 0x2821, 0x3300}, //25 ASUS-WL140 Wireless USB Adapter
        { 0x2001, 0x3700}, //26 DWL-122 Wireless USB Adapter
        { 0x0846, 0x4110}, //27 NetGear MA111
        { 0x0772, 0x5731}, //28 MacSense WUA-700
        { 0x124a, 0x4017}, //29 AirVast WN-220?
    };

    if (IOMasterPort(MACH_PORT_NULL, &masterPort) != KERN_SUCCESS) {
        return NO; // REV/FIX: throw.
    }
        
    if (IORegistryCreateIterator(masterPort, kIOServicePlane, kIORegistryIterateRecursively, &iterator) == KERN_SUCCESS) {
        while (sdev = IOIteratorNext(iterator))
            if (IOObjectConformsTo(sdev, "IOUSBDevice")) {
                product = (NSNumber*)IORegistryEntrySearchCFProperty(sdev, kIOServicePlane, CFSTR("idProduct"), NULL, 0);
                vendor  = (NSNumber*)IORegistryEntrySearchCFProperty(sdev, kIOServicePlane, CFSTR("idVendor"), NULL, 0);
                
                if ((product != Nil) && (vendor != Nil)) {
                    for (i = 0; i < 29; i++) {
                        if ([vendor intValue] == devices[i].vendor && [product intValue] == devices[i].device) break;
                    }
                    [product release];
                    [vendor  release];
                    
                    if (i!=29) {
                        IOObjectRelease (iterator);
                        return YES;
                    }
                }
            }
        IOObjectRelease(iterator);
    }
    
    return NO;
}

- (bool)isDeviceAvailable:(NSArray*)devices {
    mach_port_t masterPort;
    io_iterator_t iterator;
    io_object_t sdev;
    NSString *name;
    int i;
    
    if (IOMasterPort(MACH_PORT_NULL, &masterPort) != KERN_SUCCESS) {
        return NO; // REV/FIX: throw.
    }
        
    if (IORegistryCreateIterator(masterPort, kIOServicePlane, kIORegistryIterateRecursively, &iterator) == KERN_SUCCESS) {
        while (sdev = IOIteratorNext(iterator))
            if (IOObjectConformsTo(sdev, "IOPCCard16Device")) {
                name = (NSString*)IORegistryEntrySearchCFProperty(sdev, kIOServicePlane, CFSTR("IOName"), NULL, 0);
                
                if (name) {
                    for (i = 0; i < [devices count]; i++) {
                        if ([name isEqualToString:[devices objectAtIndex:i]]) break;
                    }
                    [name release];
                    
                    if ([devices count]) {
                        IOObjectRelease (iterator);
                        return YES;
                    }
                }
            }
        IOObjectRelease(iterator);
    }
    
    return NO;
}

- (bool)isServiceAvailable:(char*)service {
    mach_port_t masterPort;
    io_iterator_t iterator;
    io_object_t sdev;
 
    if (IOMasterPort(MACH_PORT_NULL, &masterPort) != KERN_SUCCESS) {
        return NO; // REV/FIX: throw.
    }
        
    if (IORegistryCreateIterator(masterPort, kIOServicePlane, kIORegistryIterateRecursively, &iterator) == KERN_SUCCESS) {
        while (sdev = IOIteratorNext(iterator))
            if (IOObjectConformsTo(sdev, service)) {
                IOObjectRelease (iterator);
                return YES;
            }
        IOObjectRelease(iterator);
    }
    
    return NO;
}

- (void)populateDriverBox {
    NSArray *ids;
    [_selectedDriver addItemWithTitle:@"<configure manually>"];
    [[_selectedDriver lastItem] setTag: 0];
    
    if ([self isServiceAvailable:"AirPortDriver"]) {
        [_selectedDriver addItemWithTitle:@"Apple Airport Card, passive mode"];
        [[_selectedDriver lastItem] setTag: 1];
        [_selectedDriver addItemWithTitle:@"Apple Airport Card, active mode"];
        [[_selectedDriver lastItem] setTag: 2];
    }
    if ([self isServiceAvailable:"AirPortPCI"] || [self isServiceAvailable:"AirPortPCI_MM"] || [self isServiceAvailable:"AirPort_Athr5424"] ) {
        [_selectedDriver addItemWithTitle:@"Apple Airport Extreme Card, active mode"];
        [[_selectedDriver lastItem] setTag: 2];
	}
	if ([self isServiceAvailable:"AirPortPCI"] || [self isServiceAvailable:"AirPortPCI_MM"]) {
		[_selectedDriver addItemWithTitle:@"Apple Airport Extreme Card, passive mode"];
        [[_selectedDriver lastItem] setTag: 8];
    }
    
    ids = [NSArray arrayWithObjects:@"pccard156,3", @"pccardb,7300", @"pccard156,2", @"pccard126,8000", @"pccard105,7", @"pccard89,1", @"pccard124,1110", @"pccard138,2", @"pccard268,1", @"pccard250,2", @"pccard26f,30b", @"pccard274,1612", @"pccard274,1613", @"pccard274,3301", @"pccard28a,2", @"pccard2d2,1", @"pccardd601,2", @"pccardd601,5", nil];
    if ([self isServiceAvailable:"org_noncontiguous_WirelessDriver"] || [self isServiceAvailable:"com_ioxperts_802_11_driver"] || [self isDeviceAvailable:ids]) {
        [_selectedDriver addItemWithTitle:@"Prism2 or Orinoco PCMCIA Card, passive mode"];
        [[_selectedDriver lastItem] setTag: 3];
    }
    if ([self isServiceAvailable:"com_intersil_prism2USB"] || [self isServiceAvailable:"com_macsense_driver_AeroPad"] || [self usbPrismDeviceAvailable]) {
        [_selectedDriver addItemWithTitle:@"Prism2 USB device, passive mode"];
        [[_selectedDriver lastItem] setTag: 4];
    }    
    ids = [NSArray arrayWithObjects:@"pccard15f,5", @"pccard15f,a", @"pccard15f,7", nil];
    if ([self isServiceAvailable:"com_cisco_PCCardRadio"] || [self isDeviceAvailable:ids]) {
        [_selectedDriver addItemWithTitle:@"Cisco Card, passive mode"];
        [[_selectedDriver lastItem] setTag: 5];
    }
    if ([self isServiceAvailable:"com_orangeware_iokit_OMI_80211g"]) {
        [_selectedDriver addItemWithTitle:@"Atheros Card, passive mode"];
        [[_selectedDriver lastItem] setTag: 6];
    }    
   if ([self isServiceAvailable:"GTDriver"]) {
        [_selectedDriver addItemWithTitle:@"PrismGT based card, passive mode"];
        [[_selectedDriver lastItem] setTag: 7];
    }    
}

- (void)writeDriverSetting {
    NSString *s;
    NSMutableDictionary *md;
	NSMutableDictionary *md2;
    int i;
    
    if ([[_selectedDriver selectedItem] tag] == 0) return;
    
    md = [NSMutableDictionary dictionary];
    
    switch ([[_selectedDriver selectedItem] tag]) {
    case 1:
        [md setObject:@"Airport Card" forKey:@"deviceName"];
        [md setObject:@"WaveDriverViha" forKey:@"driverID"];
        break;
    case 2:
        [md setObject:@"Airport Card" forKey:@"deviceName"];
        [md setObject:@"WaveDriverAirport" forKey:@"driverID"];
        break;
    case 3:
        [md setObject:@"Prism2/Orinoco/Hermes Card" forKey:@"deviceName"];
        [md setObject:@"WaveDriverMacJack" forKey:@"driverID"];
        break;
    case 4:
        [md setObject:@"Prism2 USB device" forKey:@"deviceName"];
        [md setObject:@"WaveDriverUSBIntersil" forKey:@"driverID"];
        break;
    case 5:
        [md setObject:@"Aironet Card" forKey:@"deviceName"];
        [md setObject:@"WaveDriverAironet" forKey:@"driverID"];
        break;
    case 6:
        [md setObject:@"Atheros based Card" forKey:@"deviceName"];
        [md setObject:@"WaveDriverAtheros" forKey:@"driverID"];
        break;
    case 7:
        [md setObject:@"PrismGT based Card" forKey:@"deviceName"];
        [md setObject:@"WaveDriverPrismGT" forKey:@"driverID"];
        break;
	case 8:
	    [md setObject:@"Airport Card" forKey:@"deviceName"];
        [md setObject:@"WaveDriverAirportExtreme" forKey:@"driverID"];
        break;
    }    
    
    [md setObject:@"~/DumpLog %y-%m-%d %H:%M" forKey:@"dumpDestination"];
    [md setObject:[NSNumber numberWithInt:0] forKey:@"dumpFilter"];
    [md setObject:[NSNumber numberWithInt:1] forKey:@"firstChannel"];
    [md setObject:[NSNumber numberWithBool:NO] forKey:@"injectionDevice"];
    for (i = 1; i <= 11; i++) {
        s = [NSString stringWithFormat:@"useChannel%.2d", i];
        [md setObject:[NSNumber numberWithBool:YES] forKey:s];
    }
    for (i = 12; i <= 14; i++) {
        s = [NSString stringWithFormat:@"useChannel%.2d", i];
        [md setObject:[NSNumber numberWithBool:NO] forKey:s];
    }
    
	md2 = [NSMutableDictionary dictionaryWithObject:[NSArray arrayWithObject:md] forKey:@"ActiveDrivers"];
	if(([_enableMonitorMode state] == NSOnState) && ([_selectedDriver selectedTag] == 8)) {
		[md2 setValue:[NSNumber numberWithBool:YES] forKey:@"aeForever"];
	}
	
    [md2 writeToFile:[@"~/Library/Preferences/org.kismac-ng.kismac.plist" stringByExpandingTildeInPath] atomically:YES];
}
#pragma mark -

- (void)reallyWantToDelete:(NSWindow *)sheet returnCode:(int)returnCode contextInfo:(void *)contextInfo {
    if (returnCode == NSAlertDefaultReturn) {
        _currentState = stateCleanKisMAC;
        [self next:nil];
    }
}

- (IBAction)next:(id)sender {
    [_prev setEnabled:NO];
    [_next setEnabled:NO];

    switch(_currentState) {
    case stateWelcome:
        if ([_installKisMAC state] == NSOnState) 
			if (![self findTAR]) {
				NSBeginInformationalAlertSheet(@"Unable to find Support Program!", @"OK", Nil, Nil, [self window], self, nil, nil, nil, 
					@"The KisMAC installer was unable to find /usr/bin/tar. Make sure it exists! The program is part of the MacOS X BSD Subsystem, which is a default part of the operating system.");
				_currentState = stateWelcome;
			} else {
				_currentState = stateLicense;
			}
        else if ([_removeKisMAC state] == NSOnState) 
            NSBeginInformationalAlertSheet(@"Erase permanently?", @"Remove", @"Cancel", Nil, [self window], self, @selector(reallyWantToDelete:returnCode:contextInfo:), nil, nil, @"Do you really want to delete KisMAC permanently? You will loose all your preferences!");
        else 
            NSAssert(0, @"Reality Error");
            
        break;
    case stateLicense:
		if ([self findKisMACPrefs])
            _currentState = stateRemovePrefs;
        else if ([self findWirelessDriver])
            _currentState = stateInstallSFPatch;
        else 
            _currentState = stateInstallDir;
        break;
    case stateRemovePrefs:
        if (([_removePrefs state] == NSOnState) && (![self removePreferences])) break;
        
        if ([self findWirelessDriver])
            _currentState = stateInstallSFPatch;
        else 
            _currentState = stateInstallDir;
        break;
    case stateInstallSFPatch:
        _currentState = stateInstallDir;
        break;
    case stateInstallDir:
        if ([self validDir:[_targetDirectory stringValue]])
            if ([self findKisMACPrefs])
				_currentState = stateDoInstall;
			else
				_currentState = stateConfigure;
        else
            NSBeginInformationalAlertSheet(@"Invalid Directory", Nil, Nil, Nil, [self window], self, nil, nil, nil, @"The path you have chosen, does either not exist or is not writeable!");
        break;
	case stateConfigure:
        _prevEnabled = YES;
        _currentState = stateConfirmConfigure;
        break;
    case stateConfirmConfigure:
        if([_selectedDriver selectedTag] == 8 && ![self monitorModeEnabled])
			_currentState = stateEnableMonitorMode;
		else
			_currentState = stateDoInstall;
        break;
	case stateEnableMonitorMode:
		_currentState = stateDoInstall;
		break;
    case stateDoInstall:
        _prevEnabled = NO; //cannot go back to config
		if ([_progBar doubleValue] == 0)
			_currentState = stateInstallCanceled;
		else
            _currentState = stateInstallDone;
        break;
    case stateInstallDone:
        if (_shallReboot) [self reboot];
        [NSApp terminate:nil];
        break;
	case stateInstallCanceled:
        [NSApp terminate:nil];
        break;
		
    case stateCleanKisMAC:
        _currentState = stateRemovingKisMAC;
        break;
    case stateRemovingKisMAC:
        _currentState = stateRemovalDone;
        break;
    case stateRemovalDone:
        if (_shallReboot) [self reboot];
        [NSApp terminate:nil];
        break;
        
    default:
        NSAssert(0, @"Illegal state in next");
    }
    [self updateState];

    [_prev setEnabled:_prevEnabled];
    [_next setEnabled:_nextEnabled];
}

-(IBAction)prev:(id)sender {
    [_prev setEnabled:NO];
    [_next setEnabled:NO];
    
    switch(_currentState) {
    case stateLicense:
		_prevEnabled  = NO;
        _currentState = stateWelcome;
        break;
    case stateRemovePrefs:
        _currentState = stateLicense;
        break;
    case stateInstallSFPatch:
        if ([self findKisMACPrefs]) 
            _currentState = stateRemovePrefs;
        else 
            _currentState = stateLicense;
        break;
    case stateInstallDir:
        if ([self findWirelessDriver]) 
            _currentState = stateInstallSFPatch;
        else if ([self findKisMACPrefs]) 
            _currentState = stateRemovePrefs;
        else 
            _currentState = stateLicense;
        
        break;
    case stateConfigure:
		_currentState = stateInstallDir;
        break;
    case stateConfirmConfigure:
        _currentState = stateConfigure;
        break;
	case stateEnableMonitorMode:
        _currentState = stateConfirmConfigure;
        break;
    case stateDoInstall:
        break;
	case stateInstallDone:
		break;
	case stateInstallCanceled:
		_prevEnabled  = YES;
		_currentState = stateWelcome;
		break;
		
    default:
        NSAssert(0, @"Illegal state in prev");
    }
    
    [self updateState];

    [_prev setEnabled:_prevEnabled];
    [_next setEnabled:_nextEnabled];
}

- (void)openPanelDidEnd:(NSOpenPanel *)sheet returnCode:(int)returnCode contextInfo:(void *)contextInfo {
    if (returnCode == NSOKButton) {
        [_targetDirectory setStringValue:[[sheet filename] stringByAbbreviatingWithTildeInPath]];
    }
}

-(IBAction)choseTargetDirectory:(id)sender {
    NSOpenPanel *o;
    
    o = [NSOpenPanel openPanel];
    [o setAllowsMultipleSelection:NO];
    [o setCanChooseFiles:NO];
    [o setCanChooseDirectories:YES];
    [o setResolvesAliases:YES];
    if (NSAppKitVersionNumber>=743)
        [o setCanCreateDirectories:YES];
    
    [o beginSheetForDirectory:[[_targetDirectory stringValue] stringByExpandingTildeInPath] file:nil types:nil modalForWindow:[self window] modalDelegate:self didEndSelector:@selector(openPanelDidEnd:returnCode:contextInfo:) contextInfo:Nil];
}

#pragma mark -

-(void)performInstall:(id)nilObject {
    NSAutoreleasePool *pool;
    
    pool = [[NSAutoreleasePool alloc] init];
    
	[self writeDriverSetting];
		
    if (![self removeKisMACInstallation:[[self getPreviousInstallDir] stringByExpandingTildeInPath]]) goto cancel;
    [_progBar incrementBy:1.0];
        
    if (![self removeKisMACInstallation:[[[_targetDirectory stringValue] stringByExpandingTildeInPath] stringByResolvingSymlinksInPath]]) goto cancel;
    [_progBar incrementBy:1.0];
    
    if ([self findWirelessDriverPatch]) {
        [_installStatus setStringValue:@"Removing Wireless driver patch..."];
        if (![self removeWirelessDriverPatch]) goto cancel;
        [_progBar incrementBy:1.0];
    }
    
    [_installStatus setStringValue:@"Installing KisMAC..."];
    if (![self installKisMACToPath:[[[_targetDirectory stringValue] stringByExpandingTildeInPath] stringByResolvingSymlinksInPath]]) goto cancel;
    [_progBar incrementBy:1.0];
    
    [_installStatus setStringValue:@"Adjusting Permissions for KisMAC..."];
    if (![self adjustKisMACPermissionsAtPath:[[[_targetDirectory stringValue] stringByExpandingTildeInPath] stringByResolvingSymlinksInPath]]) goto cancel;
    [_progBar incrementBy:1.0];
    
    if ([self findWirelessDriver] && ([_installPatch state] == NSOnState)) {
        [_installStatus setStringValue:@"Installing Wireless Patch..."];
        if (![self installWirelessPatchToPath:[[[_targetDirectory stringValue] stringByExpandingTildeInPath] stringByResolvingSymlinksInPath]]) goto cancel;
        [_progBar incrementBy:1.0];

        [_installStatus setStringValue:@"Adjusting Permissions for Wireless Patch..."];
        if (![self adjustWirelessPatchPermissions]) goto cancel;
        [_progBar incrementBy:1.0];
    }
	
	if (![self monitorModeEnabled] && ([_selectedDriver selectedTag] == 8) && ([_enableMonitorMode state] == NSOnState)) {
		[_installStatus setStringValue:@"Enabling Airport Extreme Monitor Mode..."];
		[self enableMonitorMode:YES];
		[_progBar incrementBy:1.0];
	}
    
    [_installStatus setStringValue:@"Installation complete..."];
    [_progBar setDoubleValue:[_progBar maxValue]];

    [_next setEnabled:YES];
    _nextEnabled = YES;
    
    [self performSelectorOnMainThread:@selector(next:) withObject:nil waitUntilDone:NO];
    
    [pool release];
	
	return;
cancel:
    [_installStatus setStringValue:@"Installation canceled..."];
    [_progBar setDoubleValue:0];
	        
    [_next setEnabled:YES];
    _nextEnabled = YES;
    
    [self performSelectorOnMainThread:@selector(next:) withObject:nil waitUntilDone:NO];
	
    [pool release];
}

-(void)performRemoval:(id)nilObject {
    NSAutoreleasePool *pool;
    
    pool = [[NSAutoreleasePool alloc] init];
    
    [_installStatus setStringValue:@"Removing Preferences..."];
    [self removePreferences];
    [_progBar incrementBy:1.0];

    [_installStatus setStringValue:@"Removing KisMAC..."];
    [self removeKisMACInstallation:[[self getPreviousInstallDir] stringByExpandingTildeInPath]];
    [_progBar incrementBy:1.0];
        
    if ([self findWirelessDriverPatch]) {
        [_installStatus setStringValue:@"Removing Wireless driver patch..."];
        [self removeWirelessDriverPatch];
        [_progBar incrementBy:1.0];
    }
 
	if([self monitorModeEnabled]) {
		[_installStatus setStringValue:@"Disabling Airport Extreme Monitor Mode..."];
		[self enableMonitorMode:NO];
		[_progBar incrementBy:1.0];
	}
   
    [_installStatus setStringValue:@"Removal complete..."];
    [_progBar setDoubleValue:[_progBar maxValue]];

    [_next setEnabled:YES];
    _nextEnabled = YES;
    
    [self performSelectorOnMainThread:@selector(next:) withObject:nil waitUntilDone:NO];
    
    [pool release];
}

-(void)updateState {
    int numberOfSteps;
    
    switch(_currentState) {
    case stateWelcome:
        _prevEnabled = NO;
		[_next setImage:[NSImage imageNamed:@"skip_forward_active"]];
        [_next setAlternateImage:[NSImage imageNamed:@"skip_forward_blue"]];
        [_mainBox setContentView:_welcomeView];        
        break;
    case stateLicense:
        _prevEnabled = YES;
        [_mainBox setContentView:_licenseView];
        break;
    case stateRemovePrefs:
        [_mainBox setContentView:_removePreferencesView];
        break;
    case stateInstallSFPatch:
        [_mainBox setContentView:_installSFPatchView];
        break;
    case stateInstallDir:
        [_targetDirectory setStringValue:[self getPreviousInstallDir]];
        [_mainBox setContentView:_installDirectoryView];
        break;
    case stateConfigure:
        [_mainBox setContentView:_configureView];
        break;
    case stateConfirmConfigure:
        [_next setImage:[NSImage imageNamed:@"skip_forward_active"]];
        [_next setAlternateImage:[NSImage imageNamed:@"skip_forward_blue"]];
        
        [_selectedDriver removeAllItems];
        [self populateDriverBox];
        [_mainBox setContentView:_confirmConfigureView];
        break;
	case stateEnableMonitorMode:
		[_mainBox setContentView:_enableMonitorModeView];
		break;
    case stateDoInstall:
        _prevEnabled = NO;
        _nextEnabled = NO;
        numberOfSteps = 4;
        if ([self findWirelessDriverPatch]) numberOfSteps++;
        if ([self findWirelessDriver] && ([_installPatch state] == NSOnState)) numberOfSteps+=2;
        if (![self monitorModeEnabled] && ([_selectedDriver selectedTag] == 8) && ([_enableMonitorMode state] == NSOnState)) numberOfSteps++;
		
        [_progBar setMinValue:0];
        [_progBar setMaxValue:numberOfSteps];
        [_progBar setDoubleValue:0.0];
        [_progBar setUsesThreadedAnimation:YES];
        [_installStatus setStringValue:@"Removing any old components..."];
        [_mainBox setContentView:_installView];
        
        [NSThread detachNewThreadSelector:@selector(performInstall:) toTarget:self withObject:nil];
        break;
    case stateInstallDone:
        [NSApp requestUserAttention:NSInformationalRequest];
        [_next setImage:[NSImage imageNamed:@"stop_active"]];
        [_next setAlternateImage:[NSImage imageNamed:@"stop_blue"]];
        
        if (_shallReboot)
            [_restartWarning setStringValue:@"WARNING! Your computer will be rebooted!"];
        else
            [_restartWarning setStringValue:@""];

        [_mainBox setContentView:_installDoneView];
        break;
	case stateInstallCanceled:
		_prevEnabled = YES;
	    [NSApp requestUserAttention:NSInformationalRequest];
        [_next setImage:[NSImage imageNamed:@"stop_active"]];
        [_next setAlternateImage:[NSImage imageNamed:@"stop_blue"]];
        
		[_restartWarning setStringValue:@""];
		[_installationStatus setStringValue:@"Installation canceled!"];
		
        [_mainBox setContentView:_installDoneView];
        break;
	
    case stateRemovingKisMAC:
        _prevEnabled = NO;
        _nextEnabled = NO;
        numberOfSteps = 2;
        if ([self findWirelessDriverPatch]) numberOfSteps++;
        if ([self monitorModeEnabled]) numberOfSteps++;
		
        [_progBar setMinValue:0];
        [_progBar setMaxValue:numberOfSteps];
        [_progBar setDoubleValue:0.0];
        [_progBar setUsesThreadedAnimation:YES];
        [_installStatus setStringValue:@"Removing KisMAC..."];
        [_mainBox setContentView:_removeKisMACView];

        [NSThread detachNewThreadSelector:@selector(performRemoval:) toTarget:self withObject:nil];
        break;
    case stateRemovalDone:
        _prevEnabled = NO;
        [NSApp requestUserAttention:NSInformationalRequest];
        [_next setImage:[NSImage imageNamed:@"stop_active"]];
        [_next setAlternateImage:[NSImage imageNamed:@"stop_blue"]];
        
		if (_shallReboot)
			[_removeRestartWarning setStringValue:@"WARNING! Your computer will be rebooted!"];
		else
			[_removeRestartWarning setStringValue:@""];

        [_mainBox setContentView:_removalDoneView];
        break;
    default:
        NSAssert(0, @"Illegal state");
    }
}

#pragma mark -

- (void)quitInstaller:(NSWindow *)sheet returnCode:(int)returnCode contextInfo:(void *)contextInfo {
    if (returnCode == NSAlertDefaultReturn) {
        [NSApp terminate:nil];
	} else {
		_nextEnabled = YES;
		[_next setEnabled:_nextEnabled];
	}
}
- (void)applicationDidFinishLaunching:(NSNotification *)aNotification {

	if (NSAppKitVersionNumber < 800) {
		NSLog(@"MacOS is not 10.4! AppKitVersion: %f < 800", NSAppKitVersionNumber);
		
		NSBeginInformationalAlertSheet(@"Incompatible MacOS version!", @"Quit", @"Override this Warning", Nil, [self window], self, @selector(quitInstaller:returnCode:contextInfo:), nil, nil, @"This version of KisMAC requires at least MacOS 10.4 (Tiger). You will need to either update your operating system or get an older version of KisMAC from <http://kismac.binaervarianz.de/>.");

		_nextEnabled = NO;
		[_next setEnabled:_nextEnabled];
	}
}
@end

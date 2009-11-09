/*
        
        File:			main.c
        Program:		AirPortMenu
		Author:			Michael Rossberg
						mick@binaervarianz.de
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
#include <CoreServices/CoreServices.h>
#include "../../Sources/3rd Party/Apple80211.h"

int CoreMenuExtraGetMenuExtra( CFStringRef identifier, CFTypeRef menuExtra );
int CoreMenuExtraAddMenuExtra( CFURLRef path, int position, int whoCares, int whoCares2, int whoCares3, int whoCares4 );
int CoreMenuExtraRemoveMenuExtra( CFTypeRef menuExtra, int whoCares );

int main (int argc, const char * argv[]) {
    CFTypeRef g;
    unsigned char c[]="/System/Library/CoreServices/Menu Extras/AirPort.menu";
    CFURLRef url;
    WirelessContextPtr airportContext;
   
    if (argc==1) {
		if (WirelessIsAvailable()) return 0;
		return 1;
    }
	if (strcmp(argv[1],"start")==0) {
		url = CFURLCreateFromFileSystemRepresentation(kCFAllocatorDefault,c,strlen((char*)c),true);
		CoreMenuExtraAddMenuExtra(url, 1, 0, 0, 0, 0);
		CFRelease(url);
		
		 //see whether we can disable the airport via API
		if (WirelessIsAvailable()) {
			if (WirelessAttach(&airportContext, 0) == 0) {
				WirelessSetEnabled(airportContext, 1);
				WirelessSetPower(airportContext, 1);
				WirelessDetach(airportContext);
			} else return 2;
			airportContext = NULL;
		}
		return 0;
	} else if (strcmp(argv[1],"enable")==0) {
		if (WirelessIsAvailable()) {
			if (WirelessAttach(&airportContext, 0) == 0) {
				WirelessSetEnabled(airportContext, 1);
				WirelessSetPower(airportContext, 1);
				WirelessDetach(airportContext);
			} else return 2;
			airportContext = NULL;
		}
		return 0;
    } else if (strcmp(argv[1],"stop")==0) {
        CoreMenuExtraGetMenuExtra(CFSTR("com.apple.menuextra.airport"),&g);
        CoreMenuExtraRemoveMenuExtra(g,0);
        //CFRelease(g);
		return 0;
    }
    return 1;
}

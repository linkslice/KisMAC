/*
        
        File:			WaveDriverAirportExtreme.h
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

#import <Cocoa/Cocoa.h>
#import <pcap.h>
#import "WaveDriver.h"
#import "Apple80211.h"

enum APExtType {
	APExtTypeUnknown,
	APExtTypeBcm,
	APExtTypeAth5414
};

#define DLT_PRISM_HEADER         119
#define DLT_IEEE802_11           105
#define DLT_IEEE802_11_RADIO_AVS 163
#define DLT_IEEE802_11_RADIO     127
#define DLT_TYPE_ERR             -1


@interface WaveDriverAirportExtreme : WaveDriver
{
	pcap_t *_device;
	enum APExtType _apeType;
    int DLTType;
    NSTimer * channelEnforceTimer;
}

WIErr wlc_ioctl(int command, int bufsize, void* buffer, int outsize, void* out);

@end

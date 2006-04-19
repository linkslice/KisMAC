/*
        
        File:			MACJackDriverInterface.h
        Program:		MACJack
	Author:			Dino Dai Zovi, Michael Rossberg
				mick@binaervarianz.de
	Description:		KisMAC is a wireless stumbler for MacOS X.
                
        This file is part of KisMAC.

        This file is mostly taken from the Viha driver.

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

/*
 * Interface between user space device interface framework and kernel
 * space user client.
 */

#ifndef WLDRIVERINTERFACE_H
#define WLDRIVERINTERFACE_H

#define kWLUCClassName "MACJackUserClient"

typedef enum WLUCMethods {
    kMACJackUserClientOpen,         // kIOUCScalarIScalarO, 0, 0
    kMACJackUserClientClose,        // kIOUCScalarIScalarO, 0, 0
    kMACJackUserClientGetChannel,   // kIOUCScalarIScalarO, 0, 1
    kMACJackUserClientSetChannel,   // kIOUCScalarIScalarO, 1, 0
    kMACJackUserClientStartCapture, // kIOUCScalarIScalarO, 1, 0
    kMACJackUserClientStopCapture,  // kIOUCScalarIScalarO, 0, 0
    kMACJackUserClientSendFrame,    // kIOUCScalarIStructI, 1, 2364
    kMACJackUserClientStopSendingFrames, //kIOUCScalarIScalarO 0, 0
    kMACJackUserClientGetAllowedChannels,   // kIOUCScalarIScalarO, 0, 1
    kMACJackUserClientLastMethod,
} WLUCMethod;

#define kMACJackUserClientNotify     0xdeadbeef
#define kMACJackUserClientMap        0xdeadface

#endif /* WLDRIVERINTERFACE_H */

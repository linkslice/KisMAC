/*
        
        File:			AiroJackDriverInterface.h
        Program:		AiroJack
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

#define kWLUCClassName "AiroJackUserClient"

typedef enum WLUCMethods {
    kAiroJackUserClientOpen,         // kIOUCScalarIScalarO, 0, 0
    kAiroJackUserClientClose,        // kIOUCScalarIScalarO, 0, 0
    kAiroJackUserClientGetChannel,   // kIOUCScalarIScalarO, 0, 1
    kAiroJackUserClientSetChannel,   // kIOUCScalarIScalarO, 1, 0
    kAiroJackUserClientStartCapture, // kIOUCScalarIScalarO, 1, 0
    kAiroJackUserClientStopCapture,  // kIOUCScalarIScalarO, 0, 0
    kAiroJackUserClientSendFrame,    // kIOUCScalarIStructI, 1, 2364
    kAiroJackUserClientStopSendingFrames, //kIOUCScalarIScalarO 0, 0
    kAiroJackUserClientLastMethod,
} WLUCMethod;

#define kAiroJackUserClientNotify     0xbeefc0de
#define kAiroJackUserClientMap        0xdeadc0de

#endif /* WLDRIVERINTERFACE_H */

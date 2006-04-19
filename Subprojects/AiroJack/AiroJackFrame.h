/*
        
        File:			AiroJackFrame.h
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

#ifndef AiroJackFRAME_H
#define AiroJackFRAME_H

#include <IOKit/IOTypes.h>

/*
 * Frame descriptor
 */
struct _AiroFrame{
    UInt16 tmp1[2];
    UInt16 status;
    UInt16 len;
    UInt8  silence;
    UInt8  signal;
    UInt8  tmp4;
    UInt8  channel;
    UInt8  tmp3;
    UInt8  tmp5;
    UInt8  tmp6;
    UInt8  tmp8;
    UInt16 tmp[2];
};

struct _WLFrame {
    /* Control Fields (Little Endian) */
    UInt16 status;
    UInt16 channel;
    UInt16 reserved1;
    UInt8  signal;
    UInt8  silence;
    UInt8  rate;
    UInt8  rx_flow;
    UInt8  tx_rtry;
    UInt8  tx_rate;
    UInt16 txControl;

    /* 802.11 Header Info (Little Endian) */
    UInt16 frameControl;
    UInt8  duration;
    UInt8  id;
    UInt8  address1[6];
    UInt8  address2[6];
    UInt8  address3[6];
    UInt16 sequenceControl;
    UInt8  address4[6];
    UInt16 dataLen;

    /* 802.3 Header Info (Big Endian) */
    UInt8  dstAddr[6];
    UInt8  srcAddr[6];
    UInt16 length;
} __attribute__((packed));

typedef struct _WLFrame WLFrame;
typedef struct _AiroFrame AiroFrame;
#endif /* AiroJackFRAME_H */

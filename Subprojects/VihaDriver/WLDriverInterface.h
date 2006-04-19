/* $Id: WLDriverInterface.h,v 1.3 2004/05/05 00:02:09 kismac Exp $ */

/* Copyright (C) 2003 Dino Dai Zovi <ddz@theta44.org>
 *
 * This file is part of Viha.
 *
 * Viha is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * Viha is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public
 * License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Viha; if not, write to the Free Software Foundation,
 * Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 */

/*
 * Interface between user space device interface framework and kernel
 * space user client.
 */

#ifndef WLDRIVERINTERFACE_H
#define WLDRIVERINTERFACE_H

#define kWLUCClassName "WLUserClient"

typedef enum WLUCMethods {
    kWLUserClientOpen,         // kIOUCScalarIScalarO, 0, 0
    kWLUserClientClose,        // kIOUCScalarIScalarO, 0, 0
    kWLUserClientGetChannel,   // kIOUCScalarIScalarO, 0, 1
    kWLUserClientSetChannel,   // kIOUCScalarIScalarO, 1, 0
    kWLUserClientStartCapture, // kIOUCScalarIScalarO, 1, 0
    kWLUserClientStopCapture,  // kIOUCScalarIScalarO, 0, 0
    kWLUserClientSendFrame,    // kIOUCScalarIStructI, 1, 2364
    kWLUserClientStopSendingFrames, //kIOUCScalarIScalarO 0, 0
    kWLUserClientGetAllowedChannels,   // kIOUCScalarIScalarO, 0, 1
    kWLUserClientLastMethod,
} WLUCMethod;

#define kWLUserClientNotify     0xfeedbeef
#define kWLUserClientMap        0xfeedface

#ifndef KERNEL

#include <IOKit/IODataQueueClient.h>
#include "WLFrame.h"

class WLDriverInterface {
public:

    /*
     * Open a connection to in-kernel User Client
     */
    bool open();

    /*
     * Close User Client connection
     */
    bool close();
    
    /*
     * Get the current channel
     */
    unsigned short getChannel();

    /*
     * Set current channel
     */
    void           setChannel(unsigned short);

    /*
     * Start capturing frames on the specified channel
     */
    void           startCapture(unsigned short);

    /*
     * Stop capturing frames
     */
    void           stopCapture();

    /*
     * Return the next captured frame
     */
    WLFrame*       next();
    
private:
    kern_return_t _connect();
    kern_return_t _disconnect();
    
    io_connect_t       _userClientPort;
    IODataQueueMemory* _packetQueue;
    vm_size_t          _packetQueueSize;
    mach_port_t        _packetQueuePort;
};

#endif /* KERNEL */

#endif /* WLDRIVERINTERFACE_H */

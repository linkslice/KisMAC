/*
        
        File:			Insomnia.cpp
        Program:		KisMAC
	Author:			Michael Roßberg
				mick@binaervarianz.de
	Description:		KisMAC is a wireless stumbler for MacOS X.
                
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

#include "Insomnia.h"
#include <IOKit/IOLib.h>
#include <IOKit/pwr_mgt/RootDomain.h>

#define super IOService

OSDefineMetaClassAndStructors(Insomnia, IOService);

bool Insomnia::init(OSDictionary* properties) {
    IOPMrootDomain *root = NULL;

    if (super::init(properties) == false) {
	IOLog("Insomnia::init: super::init failed\n");
	return false;
    }
    
    root = getPMRootDomain();

    if (!root) {
        IOLog("Insomnia: Fatal error could not get RootDomain.\n");
        return false;
    }
    
    root->receivePowerNotification(kIOPMDisableClamshell);
    IOLog("Insomnia: Lid close is now ignored.");

    return true;
}

bool Insomnia::start(IOService* provider) {
    if (!super::start(provider)) {
	IOLog("Insomnia::start: super::start failed\n");
	return false;
    }

    return true;
}

void Insomnia::stop(IOService* provider) {
    if (_workLoop) {
        _workLoop->release();
        _workLoop = 0;
    }
    
    super::stop(provider);
}

void Insomnia::free() {
    IOPMrootDomain *root = NULL;
    
    root = getPMRootDomain();

    if (!root) {
        IOLog("Insomnia: Fatal error could not get RootDomain.\n");
        return;
    }
    
    root->receivePowerNotification(kIOPMEnableClamshell);
    IOLog("Insomnia: Lid close is now processed again.\n");

    super::free();
    return;
}

IOWorkLoop* Insomnia::getWorkLoop() {
    if (!_workLoop)
        _workLoop = IOWorkLoop::workLoop();
        
    return _workLoop;
}
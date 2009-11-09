/*
        
        File:			Insomnia.h
        Program:		KisMAC
	Author:			Michael RoÃƒÅ¸berg
				mick@binaervarianz.de
	Description:		KisMAC is a wireless stumbler for MacOS X.
                
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


#include <IOKit/IOService.h>
#include <IOKit/IOWorkLoop.h>

class Insomnia : public IOService {
    OSDeclareDefaultStructors(Insomnia);

public:
    // driver startup and shutdown
    virtual bool init(OSDictionary * = 0);
    virtual bool start(IOService* provider);
    virtual void stop(IOService* provider);
    virtual void free();
    
    virtual IOWorkLoop* getWorkLoop();

private:

    IOWorkLoop*             _workLoop;
};

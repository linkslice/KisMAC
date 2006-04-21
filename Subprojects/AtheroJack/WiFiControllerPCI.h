/*
        
        File:			WiFiControllerPCI.h
        Program:		AtheroJack
		Author:			Michael Rossberg
						mick@binaervarianz.de
		Description:	AtheroJack is a free driver monitor mode driver for Atheros cards.
                
        This file is part of AtheroJack.

    AtheroJack is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    AtheroJack is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with AtheroJack; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#include "WiFiController.h"
#include <IOKit/pci/IOPCIDevice.h>

#define CACHE_ALIGNMENT				32
#define MAX_FRAGMENT_SIZE	        1600

/*
 * Type: pageBlock_t
 *
 * Purpose:
 *   Track a page sized memory block.
 */
typedef struct {
    IOBufferMemoryDescriptor    *memory;
    void *                      freeStart;
    UInt32                      freeBytes;
} pageBlock_t;


class WiFiControllerPCI : public WiFiController {
    OSDeclareDefaultStructors(WiFiControllerPCI)

public:
    virtual bool startProvider(IOService *provider);
    virtual bool openProvider();
    virtual bool closeProvider();
    virtual bool freeProvider();

    //memory functions for convinience
    bool allocatePageBlock(pageBlock_t * p);
    void freePageBlock(pageBlock_t * p);
    bool getPhysAddress(pageBlock_t * p, void * va, IOPhysicalAddress * pa);
    void *allocateMemoryFrom(pageBlock_t * p, UInt32 size, UInt32 align,
                              IOPhysicalAddress * paddr = 0);
protected:
    virtual bool pciConfigInit(IOPCIDevice * provider);
    
    IOPCIDevice         *_nub;
    UInt8               _pmPCICapPtr;
    IOMemoryMap         *_map;
    IOVirtualAddress    _ioBase;
    
    IOMbufNaturalMemoryCursor *_mbufCursor;
};

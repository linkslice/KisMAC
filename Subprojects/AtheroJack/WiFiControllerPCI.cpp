/*
        
        File:			WiFiControllerPCI.cpp
        Program:		AtheroJack
	Author:			Michael Rossberg
				mick@binaervarianz.de
	Description:		AtheroJack is a free driver monitor mode driver for Atheros cards.
                
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

#include "WiFiControllerPCI.h"

#define super WiFiController
OSDefineMetaClassAndStructors(WiFiControllerPCI, WiFiController)

//---------------------------------------------------------------------------
// Function: pciConfigInit
//
// Update PCI command register to enable the memory-mapped range,
// and bus-master interface.

bool WiFiControllerPCI::pciConfigInit(IOPCIDevice * provider) {
    #define kPCIPMCSR                   (_pmPCICapPtr + 4)
    UInt16 reg16;

    reg16 = provider->configRead16(kIOPCIConfigCommand);

    reg16 |= (kIOPCICommandBusMaster      |
              kIOPCICommandMemorySpace    |
              kIOPCICommandMemWrInvalidate);

    reg16 &= ~kIOPCICommandIOSpace;  // disable I/O space

    //provider->configWrite16(kIOPCIConfigCommand, reg16);

    // To allow the device to use the PCI Memory Write and Invalidate
    // command, it must know the correct cache line size. The only
    // supported cache line sizes are 8 and 16 Dwords.
    //
    // Do not modify the cache line size register. Leave this up
    // to the platform's firmware.
    // provider->configWrite8( kIOPCIConfigCacheLineSize, 8 );

    // Locate the PM register block of this device in its PCI config space.

    /*provider->findPCICapability(kIOPCIPowerManagementCapability,
                                 &_pmPCICapPtr);
    if (_pmPCICapPtr) {
        // Clear PME# and set power state to D0.
        provider->configWrite16(kPCIPMCSR, 0x8000);
        IOSleep(10);
    }*/

    return true;
}

#pragma mark provicer specific functions

bool WiFiControllerPCI::startProvider(IOService *provider) {
    bool ret = false;
    
    do {
        // Cache our provider to an instance variable.
        _nub = OSDynamicCast(IOPCIDevice, provider);
        if (_nub == 0) break;

        // Retain provider, released in free().
        _nub->retain();

        // Open our provider.
        if (_nub->open(this) == false) break;

        // Request domain power.
        // Without this, the PCIDevice may be in state 0, and the
        // PCI config space may be invalid if the machine has been
        // sleeping.
        if (_nub->requestPowerDomainState(
        /* power flags */ kIOPMPowerOn,
        /* connection  */ (IOPowerConnection *) getParentEntry(gIOPowerPlane),
        /* spec        */ IOPMLowestState ) != IOPMNoErr ) {
            break;
        }
        
        // Get the virtual address mapping of CSR registers located at
        // Base Address Range 0 (0x10). The size of this range is 4K.
        // This was changed to 32 bytes in 82558 A-Step, though only
        // the first 32 bytes should be accessed, and the other bytes
        // are considered reserved.
        _map = _nub->mapDeviceMemoryWithRegister(kIOPCIConfigBaseAddress0);
        if (_map == 0) break;
        
        _ioBase = _map->getVirtualAddress();

        // Setup our PCI config space.
        if (pciConfigInit(_nub) == false) break;

    	_mbufCursor = IOMbufNaturalMemoryCursor::withSpecification(MAX_FRAGMENT_SIZE, 1);
        if (!_mbufCursor) break;
        
        ret = true;
    } while(false);
    
    return ret;
}

bool WiFiControllerPCI::openProvider() {
    if ((_nub == NULL) || (_nub->open(this) == false)) {
        return false;
    }
    
    return true;
}

bool WiFiControllerPCI::closeProvider() {
    if (_nub) _nub->close(this);
    
    return true;
}

bool WiFiControllerPCI::freeProvider() {
    #define RELEASE(x) do { if(x) { (x)->release(); (x) = 0; } } while(0)
    
    RELEASE(_mbufCursor);
    RELEASE(_map);
    RELEASE(_nub);
    
    return true;
}

#pragma mark memory allocation

//---------------------------------------------------------------------------
// Function: allocatePageBlock
//
// Purpose:
//   Allocate a page of memory.

bool WiFiControllerPCI::allocatePageBlock(pageBlock_t * p) {
    UInt32 size = PAGE_SIZE;

    p->memory = IOBufferMemoryDescriptor::withOptions(kIOMemoryUnshared,
                                                      size, PAGE_SIZE);

    if (p->memory == 0) return false;

    if (p->memory->prepare() != kIOReturnSuccess)
    {
        p->memory->release();
        p->memory = 0;
        return false;
    }

    p->freeStart = p->memory->getBytesNoCopy();
    p->freeBytes = p->memory->getCapacity();
    bzero(p->freeStart, p->freeBytes);
    
    return true;
}

//---------------------------------------------------------------------------
// Function: freePageBlock
//
// Purpose:
//   Deallocate a page of memory.
//
void WiFiControllerPCI::freePageBlock(pageBlock_t * p) {
    if (p->memory) {
        p->memory->complete();
        p->memory->release();
        p->memory = 0;
    }
}

//---------------------------------------------------------------------------
// Function: allocateMemoryFrom
//
// Purpose:
//   Allocate the next aligned chunk of memory in a page block.

void * WiFiControllerPCI::allocateMemoryFrom( pageBlock_t *       p,
                                       UInt32              size,
                                       UInt32              align,
                                       IOPhysicalAddress * paddr ) {
    void *      allocPtr;
    IOByteCount segLength;

    if (align == 0)
            return 0;

    // Locate next alignment boundary.
    allocPtr = (void *)(((UInt32)p->freeStart + (align - 1)) & ~(align - 1));

    // Add alignment padding to the allocation size.
    size += (UInt32) allocPtr - (UInt32) p->freeStart;

    if (size > p->freeBytes)
		return 0;

    p->freeStart  = (void *)((UInt32) p->freeStart + size);
    p->freeBytes -= size;

    if (paddr) {
        *paddr = p->memory->getPhysicalSegment(
                 (UInt32) allocPtr - (UInt32) p->memory->getBytesNoCopy(),
                 &segLength);
    }

    return allocPtr;
}

//---------------------------------------------------------------------------
// Function: getPhysAddress
//
// Purpose:
//   Get the physical address for a location described by a pageBlock.
//
bool WiFiControllerPCI::getPhysAddress( pageBlock_t *       pageBlock,
                                        void *              virtAddress,
                                        IOPhysicalAddress * physAddress ) {
    IOPhysicalAddress paddr = 0;
    IOByteCount       segLength;

    if (pageBlock && pageBlock->memory) {
        paddr = pageBlock->memory->getPhysicalSegment(
                    (UInt32) virtAddress -
                    (UInt32) pageBlock->memory->getBytesNoCopy(),
                    &segLength);
    }

    *physAddress = paddr;

    return (paddr != 0);
}

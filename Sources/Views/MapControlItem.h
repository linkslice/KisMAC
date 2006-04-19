/*
        
        File:			MapControlItem.h
        Program:		KisMAC
		Author:			Michael Roßberg
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

#import <AppKit/AppKit.h>
#import "BIImageView.h"

typedef struct {
    float red;
    float green;
    float blue;
    float alpha;
} col;

typedef struct {
    col border;
    col fill;
} colState;

@interface MapControlItem : BIImageView {
    colState    _current;
    colState    _delta;
    colState    _target;
	float		_slideScale;
    NSLock      *_zoomLock;
    NSLock      *_slideLock;
    NSTimer     *_timeout;
	int			_index;
	NSPoint		_parentLocation;
}

- (id)initForID:(int)i;
- (void)slide:(BOOL)visible forParentLocation:(NSPoint)parentLocation;
- (void)mouseEntered:(NSPoint)parentLocation;
- (void)mouseClicked:(NSPoint)parentLocation;

@end

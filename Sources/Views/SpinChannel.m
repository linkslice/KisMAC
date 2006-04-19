/*
        
        File:			SpinChannel.mm
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

#import "SpinChannel.h"

//shall we use fast bitmats or draw just normally
//bitmaps need to be present for first way
#define USE_FAST_BITMAP_METHOD

@implementation SpinChannel

- (id)initWithFrame:(NSRect)frame {
    int i;
    self = [super initWithFrame:frame];
    if (self) {
      _state=0;
      _channel=0;
      _shallAnimate = NO;
      _animLock = [[NSLock alloc] init];
      
      for (i = 0; i < 12; i++) {
        _stateImg[i] = [[NSImage imageNamed:[NSString stringWithFormat:@"ChanState%d.png", i]] retain];
      }
    }
    return self;
}

- (void)animThread:(id)object {
    NSAutoreleasePool* pool = [[NSAutoreleasePool alloc] init];
    NSDate *date;
    [NSThread setThreadPriority:0];
    
    if([_animLock tryLock]) {
        while(_shallAnimate) {
			@try {
				[self setNeedsDisplay:YES];
				date = [[NSDate alloc] initWithTimeIntervalSinceNow: 1.0/12.0];
				[NSThread sleepUntilDate:date];
				[date release];
			}
			@finally {
				[pool release];
				pool = [[NSAutoreleasePool alloc] init];
			}
        }
        [_animLock unlock];
    }
        
    [pool release];
}

- (IBAction)startAnimation:(id)sender {
    _shallAnimate = YES;
    [NSThread detachNewThreadSelector:@selector(animThread:) toTarget:self withObject:nil];
}

- (IBAction)stopAnimation:(id)sender {
    _shallAnimate = NO;
    _channel=0;
    _state=0;
    [self setNeedsDisplay:YES];
}

- (void)setChannel:(int)channel {
    _channel = channel;
    [self setNeedsDisplay:YES];
}

- (void)drawRect:(NSRect)rect {
    NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
    NSSize size = _frame.size;
    NSSize txtsize;
#ifndef USE_FAST_BITMAP_METHOD
    NSColor *c = [NSColor grayColor];
    NSBezierPath *b;
    NSAffineTransform *t, *z;
    NSRect r;
    float x,y,i;
    int f;
#endif
    NSBezierPath *a;
    NSString *chan;
    NSFont* textFont;
    NSMutableDictionary* attrs = [[NSMutableDictionary alloc] init];
    
    float ratio = 4.0 / 30.0;
    
    if (_shallAnimate) _state=(_state+1)%12;
    
#ifdef USE_FAST_BITMAP_METHOD
    [_stateImg[_state] dissolveToPoint:NSMakePoint(0,0) fraction:1.0];
#else
    for (i=0;i<12;i++) {
        f = ((int)(i+_state) % 12);
        
        [[[NSColor blackColor] blendedColorWithFraction:(f < 10? f/10.0: 1) ofColor:c] set];
        b = [[NSBezierPath alloc] init];
        r = NSMakeRect(0, -0.06*size.height, (size.width)*ratio, 0.12*size.height);
        [b appendBezierPathWithRect:r];
        
        t = [NSAffineTransform transform];
        [t rotateByDegrees:i*30];
        
        x = (size.width*0.5) +(cos(i/6*pi)*(0.5-ratio)*size.width);
        y = (size.height*0.5)+(sin(i/6*pi)*(0.5-ratio)*size.height);
        z = [NSAffineTransform transform];
        [z translateXBy:x yBy:y];
        [t appendTransform:z];
        
        a = [t transformBezierPath:b];
        [b release];
        [a fill];
    }
#endif
    
    [[NSColor darkGrayColor] set];
    a = [NSBezierPath bezierPathWithOvalInRect:NSMakeRect(ratio*size.width, ratio*size.height, (1-2*ratio)*size.width, (1-2*ratio)*size.height)];
    [a fill];
    
    textFont =  [NSFont fontWithName:@"Monaco" size:(size.height>size.width ? size.width*0.8 : size.height*0.8)];
    if ((_channel!=0)&&(_shallAnimate)) chan = [NSString stringWithFormat:@"%X", _channel];
    else chan = @"/";
    [attrs setObject:textFont forKey:NSFontAttributeName];
    [attrs setObject:[NSColor whiteColor] forKey:NSForegroundColorAttributeName];
    
    txtsize = [chan sizeWithAttributes:attrs];

    [chan drawAtPoint:NSMakePoint((size.width-txtsize.width)/2, (size.height-txtsize.height)/2) withAttributes:attrs];
    
    [attrs release];
    [pool release];
}

- (void)dealloc {
    int i;
    
    for(i = 0; i < 12; i++) {
        [_stateImg[i] release];
    }
	[super dealloc];
}
@end

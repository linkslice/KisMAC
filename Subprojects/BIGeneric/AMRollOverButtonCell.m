//
//  AMRollOverButtonCell.m
//  PlateControl
//
//  Created by Andreas on Sat Jan 24 2004.
//  Copyright (c) 2004 Andreas Mayer. All rights reserved.
//
//	2004-02-01	- cleaned up layout code
//				- open menu instantly when user clicks on arrow
//	2004-03-15	- Don McConnel sent me a better formula for calculating
//				  the textInset which doesn't need trigonometric functions,
//				  so we don't need to include the libmx.dylib - thanks Don!


#import "AMRollOverButtonCell.h"
#import "NSBezierPath_AMAdditons.h"
#import </usr/include/math.h>

float am_backgroundInset = 1.5;
float am_textInsetFactor = 1.3;


@interface NSShadow (AMRollOverButton)
+ (NSShadow *)amRollOverButtonDefaultControlShadow;
+ (NSShadow *)amRollOverButtonDefaultTextShadow;
@end

@implementation NSShadow (AMRollOverButton)

+ (NSShadow *)amRollOverButtonDefaultControlShadow
{
	NSShadow *result = [[[NSShadow alloc] init] autorelease];
	[result setShadowOffset:NSMakeSize(0.0, 1.0)];
	[result setShadowBlurRadius:1.0];
	[result setShadowColor:[NSColor controlDarkShadowColor]];
	return result;
}

+ (NSShadow *)amRollOverButtonDefaultTextShadow
{
	NSShadow *result = [[[NSShadow alloc] init] autorelease];
	[result setShadowOffset:NSMakeSize(0.0, -1.0)];
	[result setShadowBlurRadius:1.0];
	[result setShadowColor:[NSColor colorWithCalibratedWhite:0.3 alpha:1.0]];
	return result;
}

@end


@interface AMRollOverButtonCell (Private)
- (NSBezierPath *)platePath;
- (void)setPlatePath:(NSBezierPath *)newPlatePath;
- (NSSize)lastFrameSize;
- (void)setLastFrameSize:(NSSize)newLastFrameSize;
- (float)calculateTextInsetForRadius:(float)radius font:(NSFont *)font;
- (void)finishInit;
@end


@implementation AMRollOverButtonCell

- (id)initTextCell:(NSString *)aString
{
	if (self = [super initTextCell:aString]) {
		[self finishInit];
	}
	return self;
}

- (void)finishInit
{
	[self setControlColor:[NSColor clearColor]];
	[self setFrameColor:[NSColor clearColor]];
	[self setTextColor:[NSColor colorWithCalibratedWhite:0.25 alpha:1.0]];
	[self setArrowColor:[NSColor colorWithCalibratedWhite:0.25 alpha:1.0]];
	[self setMouseoverControlColor:[NSColor colorWithCalibratedWhite:0.6 alpha:1.0]];
	[self setMouseoverFrameColor:[NSColor clearColor]];
	[self setMouseoverTextColor:[NSColor whiteColor]];
	[self setMouseoverArrowColor:[NSColor whiteColor]];
	[self setHighlightedControlColor:[NSColor colorWithCalibratedWhite:0.5 alpha:1.0]];
	[self setHighlightedFrameColor:[NSColor clearColor]];
	[self setHighlightedTextColor:[NSColor whiteColor]];
	[self setHighlightedArrowColor:[NSColor whiteColor]];
	[self setTextShadow:nil];
	[self setMouseoverTextShadow:[NSShadow amRollOverButtonDefaultTextShadow]];
	[self setHighlightedTextShadow:[self mouseoverTextShadow]];
	[self setHighlightedControlShadow:[NSShadow amRollOverButtonDefaultControlShadow]];
	[self setPopUpMenuDelay:0.6];
}

- (id)initWithCoder:(NSCoder *)decoder
{
	self = [super initWithCoder:decoder];
	[self setControlColor:[decoder decodeObject]];
	[self setFrameColor:[decoder decodeObject]];
	[self setTextColor:[decoder decodeObject]];
	[self setArrowColor:[decoder decodeObject]];
	[self setMouseoverControlColor:[decoder decodeObject]];
	[self setMouseoverFrameColor:[decoder decodeObject]];
	[self setMouseoverTextColor:[decoder decodeObject]];
	[self setMouseoverArrowColor:[decoder decodeObject]];
	[self setHighlightedControlColor:[decoder decodeObject]];
	[self setHighlightedFrameColor:[decoder decodeObject]];
	[self setHighlightedTextColor:[decoder decodeObject]];
	[self setHighlightedArrowColor:[decoder decodeObject]];
	[self setTextShadow:[decoder decodeObject]];
	[self setMouseoverTextShadow:[decoder decodeObject]];
	[self setHighlightedTextShadow:[decoder decodeObject]];
	[self setHighlightedControlShadow:[decoder decodeObject]];
	[decoder decodeValueOfObjCType:@encode(double) at:&am_popUpMenuDelay];
	return self;
}

- (void)encodeWithCoder:(NSCoder *)coder
{
	[super encodeWithCoder:coder];
	[coder encodeObject:am_controlColor];
	[coder encodeObject:am_frameColor];
	[coder encodeObject:am_textColor];
	[coder encodeObject:am_arrowColor];
	[coder encodeObject:am_mouseoverControlColor];
	[coder encodeObject:am_mouseoverFrameColor];
	[coder encodeObject:am_mouseoverTextColor];
	[coder encodeObject:am_mouseoverArrowColor];
	[coder encodeObject:am_highlightedControlColor];
	[coder encodeObject:am_highlightedFrameColor];
	[coder encodeObject:am_highlightedTextColor];
	[coder encodeObject:am_highlightedArrowColor];
	[coder encodeObject:am_textShadow];
	[coder encodeObject:am_mouseoverTextShadow];
	[coder encodeObject:am_highlightedTextShadow];
	[coder encodeObject:am_highlightedControlShadow];
	[coder encodeValueOfObjCType:@encode(double) at:&am_popUpMenuDelay];
}

- (void)dealloc
{
	[am_arrowPath release];
	[am_controlColor release];
	[am_frameColor release];
	[am_textColor release];
	[am_arrowColor release];
	[am_mouseoverControlColor release];
	[am_mouseoverFrameColor release];
	[am_mouseoverTextColor release];
	[am_mouseoverArrowColor release];
	[am_highlightedControlColor release];
	[am_highlightedFrameColor release];
	[am_highlightedTextColor release];
	[am_highlightedArrowColor release];
	[am_textShadow release];
	[am_mouseoverTextShadow release];
	[am_highlightedTextShadow release];
	[am_highlightedControlShadow release];
	[am_backgroundPath release];
	[am_highlightedBackgroundPath release];
	[super dealloc];
}

- (NSColor *)controlColor
{
    return am_controlColor;
}

- (void)setControlColor:(NSColor *)newControlColor
{
    id old = nil;

    if (newControlColor != am_controlColor) {
        old = am_controlColor;
        am_controlColor = [newControlColor retain];
        [old release];
    }
}

- (NSColor *)frameColor
{
    return am_frameColor;
}

- (void)setFrameColor:(NSColor *)newFrameColor
{
    id old = nil;

    if (newFrameColor != am_frameColor) {
        old = am_frameColor;
        am_frameColor = [newFrameColor retain];
        [old release];
    }
}

- (NSColor *)textColor
{
    return am_textColor;
}

- (void)setTextColor:(NSColor *)newTextColor
{
    id old = nil;

    if (newTextColor != am_textColor) {
        old = am_textColor;
        am_textColor = [newTextColor retain];
        [old release];
    }
}

- (NSColor *)arrowColor
{
    return am_arrowColor;
}

- (void)setArrowColor:(NSColor *)newArrowColor
{
    id old = nil;

    if (newArrowColor != am_arrowColor) {
        old = am_arrowColor;
        am_arrowColor = [newArrowColor retain];
        [old release];
    }
}

- (NSColor *)mouseoverControlColor
{
    return am_mouseoverControlColor;
}

- (void)setMouseoverControlColor:(NSColor *)newMouseoverControlColor
{
    id old = nil;

    if (newMouseoverControlColor != am_mouseoverControlColor) {
        old = am_mouseoverControlColor;
        am_mouseoverControlColor = [newMouseoverControlColor retain];
        [old release];
    }
}

- (NSColor *)mouseoverFrameColor
{
    return am_mouseoverFrameColor;
}

- (void)setMouseoverFrameColor:(NSColor *)newMouseoverFrameColor
{
    id old = nil;

    if (newMouseoverFrameColor != am_mouseoverFrameColor) {
        old = am_mouseoverFrameColor;
        am_mouseoverFrameColor = [newMouseoverFrameColor retain];
        [old release];
    }
}

- (NSColor *)mouseoverTextColor
{
    return am_mouseoverTextColor;
}

- (void)setMouseoverTextColor:(NSColor *)newMouseoverTextColor
{
    id old = nil;

    if (newMouseoverTextColor != am_mouseoverTextColor) {
        old = am_mouseoverTextColor;
        am_mouseoverTextColor = [newMouseoverTextColor retain];
        [old release];
    }
}

- (NSColor *)mouseoverArrowColor
{
    return am_mouseoverArrowColor;
}

- (void)setMouseoverArrowColor:(NSColor *)newMouseoverArrowColor
{
    id old = nil;

    if (newMouseoverArrowColor != am_mouseoverArrowColor) {
        old = am_mouseoverArrowColor;
        am_mouseoverArrowColor = [newMouseoverArrowColor retain];
        [old release];
    }
}

- (NSColor *)highlightedControlColor
{
    return am_highlightedControlColor;
}

- (void)setHighlightedControlColor:(NSColor *)newHighlightedControlColor
{
    id old = nil;

    if (newHighlightedControlColor != am_highlightedControlColor) {
        old = am_highlightedControlColor;
        am_highlightedControlColor = [newHighlightedControlColor retain];
        [old release];
    }
}

- (NSColor *)highlightedFrameColor
{
    return am_highlightedFrameColor;
}

- (void)setHighlightedFrameColor:(NSColor *)newHighlightedFrameColor
{
    id old = nil;

    if (newHighlightedFrameColor != am_highlightedFrameColor) {
        old = am_highlightedFrameColor;
        am_highlightedFrameColor = [newHighlightedFrameColor retain];
        [old release];
    }
}

- (NSColor *)highlightedTextColor
{
    return am_highlightedTextColor;
}

- (void)setHighlightedTextColor:(NSColor *)newHighlightedTextColor
{
    id old = nil;

    if (newHighlightedTextColor != am_highlightedTextColor) {
        old = am_highlightedTextColor;
        am_highlightedTextColor = [newHighlightedTextColor retain];
        [old release];
    }
}

- (NSColor *)highlightedArrowColor
{
    return am_highlightedArrowColor;
}

- (void)setHighlightedArrowColor:(NSColor *)newHighlightedArrowColor
{
    id old = nil;

    if (newHighlightedArrowColor != am_highlightedArrowColor) {
        old = am_highlightedArrowColor;
        am_highlightedArrowColor = [newHighlightedArrowColor retain];
        [old release];
    }
}

- (NSShadow *)textShadow
{
    return am_textShadow;
}

- (void)setTextShadow:(NSShadow *)newTextShadow
{
    id old = nil;

    if (newTextShadow != am_textShadow) {
        old = am_textShadow;
        am_textShadow = [newTextShadow retain];
        [old release];
    }
}

- (NSShadow *)mouseoverTextShadow
{
    return am_mouseoverTextShadow;
}

- (void)setMouseoverTextShadow:(NSShadow *)newMouseoverTextShadow
{
    id old = nil;

    if (newMouseoverTextShadow != am_mouseoverTextShadow) {
        old = am_mouseoverTextShadow;
        am_mouseoverTextShadow = [newMouseoverTextShadow retain];
        [old release];
    }
}

- (NSShadow *)highlightedTextShadow
{
    return am_highlightedTextShadow;
}

- (void)setHighlightedTextShadow:(NSShadow *)newHighlightedTextShadow
{
    id old = nil;

    if (newHighlightedTextShadow != am_highlightedTextShadow) {
        old = am_highlightedTextShadow;
        am_highlightedTextShadow = [newHighlightedTextShadow retain];
        [old release];
    }
}

- (NSShadow *)highlightedControlShadow
{
    return am_highlightedControlShadow;
}

- (void)setHighlightedControlShadow:(NSShadow *)newHighlightedControlShadow
{
    id old = nil;

    if (newHighlightedControlShadow != am_highlightedControlShadow) {
        old = am_highlightedControlShadow;
        am_highlightedControlShadow = [newHighlightedControlShadow retain];
        [old release];
    }
}

- (double)popUpMenuDelay
{
    return am_popUpMenuDelay;
}

- (void)setPopUpMenuDelay:(double)newPopUpMenuDelay
{
    am_popUpMenuDelay = newPopUpMenuDelay;
}


- (NSBezierPath *)backgroundPath
{
    return am_backgroundPath;
}

- (void)setBackgroundPath:(NSBezierPath *)newBackgroundPath
{
    id old = nil;

    if (newBackgroundPath != am_backgroundPath) {
        old = am_backgroundPath;
        am_backgroundPath = [newBackgroundPath retain];
        [old release];
    }
}

- (NSBezierPath *)highlightedBackgroundPath
{
    return am_highlightedBackgroundPath;
}

- (void)setHighlightedBackgroundPath:(NSBezierPath *)newHighlightedBackgroundPath
{
    id old = nil;

    if (newHighlightedBackgroundPath != am_highlightedBackgroundPath) {
        old = am_highlightedBackgroundPath;
        am_highlightedBackgroundPath = [newHighlightedBackgroundPath retain];
        [old release];
    }
}

- (NSBezierPath *)arrowPath
{
	return am_arrowPath;
}

- (void)setArrowPath:(NSBezierPath *)newArrowPath
{
	id old = nil;
	
	if (newArrowPath != am_arrowPath) {
		old = am_arrowPath;
		am_arrowPath = [newArrowPath retain];
		[old release];
	}
}

- (NSSize)lastFrameSize
{
	return am_lastFrameSize;
}

- (void)setLastFrameSize:(NSSize)newLastFrameSize
{
	am_lastFrameSize = newLastFrameSize;
}

- (BOOL)showArrow
{
	return am_showArrow;
}

- (void)setShowArrow:(BOOL)newShowArrow
{
	am_showArrow = newShowArrow;
}

- (BOOL)mouseOver
{
    return am_mouseOver;
}

- (void)setMouseOver:(BOOL)newMouseOver
{
    am_mouseOver = newMouseOver;
}


- (void)calculateLayoutForFrame:(NSRect)cellFrame inView:(NSView *)controlView
{
	// bezier path for plate background
	[self setLastFrameSize:cellFrame.size];
	NSRect innerRect = NSInsetRect(cellFrame, am_backgroundInset, am_backgroundInset);
	innerRect.origin.x = 0;
	innerRect.origin.y = 0;
	[self setBackgroundPath:[NSBezierPath bezierPathWithPlateInRect:innerRect]];
	[am_backgroundPath setCachesBezierPath:YES];
	innerRect.size.height--;
	if ([controlView isFlipped]) {
		innerRect.origin.y--;
	}
	[self setHighlightedBackgroundPath:[NSBezierPath bezierPathWithPlateInRect:innerRect]];
	[am_highlightedBackgroundPath setCachesBezierPath:YES];
	// text rect
	NSFont *font = [NSFont boldSystemFontOfSize:[NSFont systemFontSizeForControlSize:[self controlSize]]];
	NSDictionary *stringAttributes = [NSDictionary dictionaryWithObjectsAndKeys:font, NSFontAttributeName, nil];
	NSAttributedString *string = [[[NSAttributedString alloc] initWithString:[self title] attributes:stringAttributes] autorelease];
	NSSize size = [string size];
	float radius = (am_lastFrameSize.height/2.0)-am_backgroundInset;
	// calculate minimum text inset
	float textInset = ([font ascender]-([font xHeight]/2.0));
	//textInset = (1-cosf(asinf(textInset)));
	textInset = sqrt(radius*radius - textInset*textInset);
	// more looks better
	textInset *= am_textInsetFactor;
	am_textRect = NSInsetRect(cellFrame, textInset+am_backgroundInset, am_backgroundInset);
	float textHeight;
	float xHeight;
	textHeight = size.height;
	xHeight = [font xHeight];
	float diffY;
	if ([controlView isFlipped]) {
		diffY = ((am_textRect.size.height+xHeight)/2.0)-[font descender]-am_textRect.size.height;
	} else {
		diffY = (am_textRect.size.height-xHeight)/2.0+[font descender];
	}
	am_textRect.origin.y += diffY;
	am_textRect.size.height = textHeight;
	
	// arrow path
	if ([self menu]) {
		float arrowWidth = [NSFont systemFontSizeForControlSize:[self controlSize]]*0.6;
		float arrowHeight = [NSFont systemFontSizeForControlSize:[self controlSize]]*0.5;
		// clip text rect
		am_textRect.size.width -= (radius*0.2)+(arrowWidth/2.0);
		float x = am_lastFrameSize.width-am_backgroundInset-am_textRect.origin.x-(arrowWidth/2.0);
		float y = am_backgroundInset+radius-arrowHeight/2.0;

		NSBezierPath *path = [NSBezierPath bezierPath];
		NSPoint point1;
		NSPoint point2;
		NSPoint point3;
		if ([controlView isFlipped]) {
			y += radius*0.05;
			point1 = NSMakePoint(x, y);
			point2 = NSMakePoint(x+arrowWidth, y);
			point3 = NSMakePoint(x+arrowWidth/2.0, y+arrowHeight);
		} else {
			y -= radius*0.05;
			point1 = NSMakePoint(x, y+arrowHeight);
			point2 = NSMakePoint(x+arrowWidth, y+arrowHeight);
			point3 = NSMakePoint(x+arrowWidth/2.0, y);
		}
		[path moveToPoint:point1];
		[path lineToPoint:point3];
		[path lineToPoint:point2];
		[path lineToPoint:point1];
		[path closePath];
		[self setArrowPath:path];
				
	}
}

- (void)drawWithFrame:(NSRect)cellFrame inView:(NSView *)controlView
{
	if ((am_lastFrameSize.width != cellFrame.size.width) || (am_lastFrameSize.height != cellFrame.size.height)) {
		[self calculateLayoutForFrame:cellFrame inView:controlView];
	}
	if ([self isBordered]) {
		if ([self isHighlighted]) {
			[am_highlightedFrameColor set];
		} else if (am_mouseOver) {
			[am_mouseoverFrameColor set];
		} else {
			[am_frameColor set];
		}
		// translate to current origin
		NSBezierPath *path = [[am_backgroundPath copy] autorelease];
		NSAffineTransform *transformation = [NSAffineTransform transform];
		[transformation translateXBy:cellFrame.origin.x+am_backgroundInset yBy:cellFrame.origin.y+am_backgroundInset];
		[path transformUsingAffineTransform:transformation];
		[path setLineWidth:am_backgroundInset];
		[path stroke];
	}
	[self drawInteriorWithFrame:cellFrame inView:controlView];
}

- (void)drawInteriorWithFrame:(NSRect)cellFrame inView:(NSView *)controlView
{
	NSBezierPath *path;
	NSColor *textColor;
	NSColor *arrowColor;
	NSShadow *textShadow;
	NSAffineTransform *transformation = [NSAffineTransform transform];
	[transformation translateXBy:cellFrame.origin.x+am_backgroundInset yBy:cellFrame.origin.y+am_backgroundInset];
	if ([self isHighlighted]) {
		// set shadow
		[NSGraphicsContext saveGraphicsState];
		[am_highlightedControlShadow set];
		[am_highlightedControlColor set];
		path = [[am_highlightedBackgroundPath copy] autorelease];
		textColor = am_highlightedTextColor;
		arrowColor = am_highlightedArrowColor;
		textShadow = am_highlightedTextShadow;
	} else if (am_mouseOver) {
		[am_mouseoverControlColor set];
		path = [[am_backgroundPath copy] autorelease];
		textColor = am_mouseoverTextColor;
		arrowColor = am_mouseoverArrowColor;
		textShadow = am_mouseoverTextShadow;
	} else {
		[am_controlColor set];
		path = [[am_backgroundPath copy] autorelease];
		textColor = am_textColor;
		arrowColor = am_arrowColor;
		textShadow = am_textShadow;
	}
	[path transformUsingAffineTransform:transformation];
	[path setLineWidth:0.0];
	[path fill];

	if ([self isHighlighted]) {
		[NSGraphicsContext restoreGraphicsState];
	}
	[textShadow set];
	// menu arrow
	if (([self menu] && am_showArrow) || ([self menu] && am_mouseOver)) {
		// draw menu arrow
		[arrowColor set];
		[am_arrowPath fill];
	}
	NSDictionary *stringAttributes;
	NSFont *font;
	font = [NSFont boldSystemFontOfSize:[NSFont systemFontSizeForControlSize:[self controlSize]]];
	stringAttributes = [NSDictionary dictionaryWithObjectsAndKeys:font, NSFontAttributeName, textColor, NSForegroundColorAttributeName, nil];
	[[self title] drawInRect:am_textRect withAttributes:stringAttributes];
}

- (float)widthForFrame:(NSRect)frameRect
{
	float result;
	NSFont *font;
	font = [NSFont boldSystemFontOfSize:[NSFont systemFontSizeForControlSize:[self controlSize]]];
	result = [font widthOfString:[self title]];
	float radius = (frameRect.size.height/2.0)-am_backgroundInset;
	float textInset = ([font ascender]-([font xHeight]/2.0));
	//textInset = (1-cosf(asinf(textInset)));
	textInset = sqrt(radius*radius - textInset*textInset);
	textInset *= am_textInsetFactor;
	result += 2.0*(textInset+am_backgroundInset);
	if ([self menu] != nil) {
		float arrowSize = [NSFont systemFontSizeForControlSize:[self controlSize]]*0.3;
		result += (radius*0.5)+arrowSize;
	}
	return result;
}

- (NSPoint)menuPositionForFrame:(NSRect)cellFrame inView:(NSView *)controlView
{
	NSPoint result = [controlView convertPoint:cellFrame.origin toView:nil];
	result.x += 1.0;
	result.y -= am_backgroundInset+4.0;
	return result;
}

void showMenu(AMRollOverButtonCell *bc, NSRect cellFrame, NSView * controlView, NSEvent *theEvent)
{
	NSPoint menuPosition = [bc menuPositionForFrame:cellFrame inView:controlView];
	// create event for pop up menu with adjusted mouse position
	NSEvent *menuEvent = [NSEvent mouseEventWithType:[theEvent type] location:menuPosition modifierFlags:[theEvent modifierFlags] timestamp:[theEvent timestamp] windowNumber:[theEvent windowNumber] context:[theEvent context] eventNumber:[theEvent eventNumber] clickCount:[theEvent clickCount] pressure:[theEvent pressure]];
	[NSMenu popUpContextMenu:[bc menu] withEvent:menuEvent forView:controlView];
}

- (BOOL)trackMouse:(NSEvent *)theEvent inRect:(NSRect)cellFrame ofView:(NSView *)controlView untilMouseUp:(BOOL)untilMouseUp
{
	showMenu(self, cellFrame, controlView, theEvent);
	
	BOOL result = NO;
	//NSLog(@"trackMouse:inRect:ofView:untilMouseUp:");
	NSDate *endDate;
	NSPoint currentPoint = [theEvent locationInWindow];
	BOOL done = NO;
	if ([self menu]) {
		// check if mouse is over menu arrow
		NSPoint localPoint = [controlView convertPoint:currentPoint fromView:nil];
		if (localPoint.x >= (am_textRect.origin.x+am_textRect.size.width)) {
			done = YES;
			result = YES;
			showMenu(self, cellFrame, controlView, theEvent);
		}
	}
	BOOL trackContinously = [self startTrackingAt:currentPoint inView:controlView];
	// catch next mouse-dragged or mouse-up event until timeout
	BOOL mouseIsUp = NO;
	NSEvent *event;
	while (!done) { // loop ...
		NSPoint lastPoint = currentPoint;
		if ([self menu]) { // timeout for menu
			endDate = [NSDate dateWithTimeIntervalSinceNow:am_popUpMenuDelay];
		} else { // no timeout
			endDate = [NSDate distantFuture];
		}
		event = [NSApp nextEventMatchingMask:(NSLeftMouseUpMask|NSLeftMouseDraggedMask) untilDate:endDate inMode:NSEventTrackingRunLoopMode dequeue:YES];
		if (event) { // mouse event
			currentPoint = [event locationInWindow];
			if (trackContinously) { // send continueTracking.../stopTracking...
				if (![self continueTracking:lastPoint at:currentPoint inView:controlView]) {
					done = YES;
					[self stopTracking:lastPoint at:currentPoint inView:controlView mouseIsUp:mouseIsUp];
				}
				if ([self isContinuous]) {
					[NSApp sendAction:[self action] to:[self target] from:self];
				}
			}
			mouseIsUp = ([event type] == NSLeftMouseUp);
			done = done || mouseIsUp;
			if (untilMouseUp) {
				result = mouseIsUp;
			} else {
				// check, if the mouse left our cell rect
				result = NSPointInRect([controlView convertPoint:currentPoint fromView:nil], cellFrame);
				if (!result) {
					done = YES;
					[self setMouseOver:NO];
				} else {
					[self setMouseOver:YES];
				}
			}
			if (done && result && ![self isContinuous]) {
				[NSApp sendAction:[self action] to:[self target] from:self];
			}
		} else { // show menu
			done = YES;
			result = YES;
			showMenu(self, cellFrame, controlView, theEvent);
		}
	} // ... while (!done)
	return result;
}

- (int)state {
    return _state;
}
- (void)setState:(int)value {
    _state = value;
}

- (BOOL)isHighlighted {
    if (_state == NSOnState) return YES;
    return [super isHighlighted];
}

@end



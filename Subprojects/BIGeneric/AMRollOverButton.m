//
//  AMRollOverButton.m
//  PlateControl
//
//  Created by Andreas on Sat Jan 24 2004.
//  Copyright (c) 2004 Andreas Mayer. All rights reserved.
//
//	2004-03-17	- register forNSViewFrameDidChangeNotification to adjust
//				  the tracking rect; thanks go to Ville Virkkunen for reporting it
//				  (doesn't work in IB though - any help appreciated)
//	2004-05-09	- overwrite removeFromSuperview and
//				  removeFromSuperviewWithoutNeedingDisplay
//				  to remove the tracking rectangle.
//				  Thanks to Jeremy Dronfield for reporting the issue.


#import "AMRollOverButton.h"
#import "AMRollOverButtonCell.h"

float am_insetCell = 0.0;

@interface AMRollOverButtonCell (AMRollOverButton)
- (void)finishInit;
@end


@interface AMRollOverButton (Private)
- (void)am_setDefaultValues;
- (void)am_resetTrackingRect;
@end


@implementation AMRollOverButton

- (id)initWithFrame:(NSRect)frameRect andTitle:(NSString *)theTitle
{
	if (self = [super initWithFrame:frameRect]) {
		[self am_setDefaultValues];
		[self setTitle:theTitle];
	}
	return self;
}

/* doesn't work in IB anyway ... :-(
- (id)initWithCoder:(NSCoder *)decoder
{
	self = [super initWithCoder:decoder];
	[[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(frameDidChange:) name:NSViewFrameDidChangeNotification object:self];
	[self setPostsFrameChangedNotifications:YES];
	return self;
}
*/

- (void)awakeFromNib
{
	[self am_setDefaultValues];
}

- (void)am_setDefaultValues
{
	if (![[self cell] isKindOfClass:[AMRollOverButtonCell class]]) {
		NSString *title = [self title];
		if (title == nil) title = @"";
		[self setCell:[[[AMRollOverButtonCell alloc] initTextCell:title] autorelease]];
		[[self cell] setControlSize:NSRegularControlSize];
	}
	[[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(frameDidChange:) name:NSViewFrameDidChangeNotification object:self];
	[self setPostsFrameChangedNotifications:YES];
	[self am_resetTrackingRect];
}

- (void)am_resetTrackingRect
{
	if (am_trackingRect) {
		[self removeTrackingRect:am_trackingRect];
	}
	NSRect trackingRect = [self frame];
	trackingRect.origin = NSZeroPoint;
	am_trackingRect = [self addTrackingRect:trackingRect owner:self userData:nil assumeInside:NO];
}

- (void)dealloc
{
	[self removeTrackingRect:am_trackingRect];
	[super dealloc];
}


- (NSString *)title
{
    return [[self cell] title];
}

- (void)setTitle:(NSString *)newTitle
{
	[[self cell] setTitle:newTitle];
}

- (NSControlSize)controlSize
{
	return [[self cell] controlSize];
}

- (void)setControlSize:(NSControlSize)newControlSize
{
	[[self cell] setControlSize:newControlSize];
}

- (NSColor *)controlColor
{
	return [[self cell] controlColor];
}

- (void)setControlColor:(NSColor *)newControlColor
{
	[[self cell] setControlColor:newControlColor];
}

- (NSColor *)frameColor
{
	return [[self cell] frameColor];
}

- (void)setFrameColor:(NSColor *)newFrameColor
{
	[[self cell] setFrameColor:newFrameColor];
}

- (NSColor *)textColor
{
	return [[self cell] textColor];
}

- (void)setTextColor:(NSColor *)newTextColor
{
	[[self cell] setTextColor:newTextColor];
}

- (NSColor *)arrowColor
{
	return [[self cell] arrowColor];
}

- (void)setArrowColor:(NSColor *)newArrowColor
{
	[[self cell] setArrowColor:newArrowColor];
}

- (NSColor *)mouseoverControlColor
{
	return [[self cell] mouseoverControlColor];
}

- (void)setMouseoverControlColor:(NSColor *)newMouseoverControlColor
{
	[[self cell] setMouseoverControlColor:newMouseoverControlColor];
}

- (NSColor *)mouseoverFrameColor
{
	return [[self cell] mouseoverFrameColor];
}

- (void)setMouseoverFrameColor:(NSColor *)newMouseoverFrameColor
{
	[[self cell] setMouseoverFrameColor:newMouseoverFrameColor];
}

- (NSColor *)mouseoverTextColor
{
	return [[self cell] mouseoverTextColor];
}

- (void)setMouseoverTextColor:(NSColor *)newMouseoverTextColor
{
	[[self cell] setMouseoverTextColor:newMouseoverTextColor];
}

- (NSColor *)mouseoverArrowColor
{
	return [[self cell] mouseoverArrowColor];
}

- (void)setMouseoverArrowColor:(NSColor *)newMouseoverArrowColor
{
	[[self cell] setMouseoverArrowColor:newMouseoverArrowColor];
}

- (NSColor *)highlightedControlColor
{
	return [[self cell] highlightedControlColor];
}

- (void)setHighlightedControlColor:(NSColor *)newHighlightedControlColor
{
	[[self cell] setHighlightedControlColor:newHighlightedControlColor];
}

- (NSColor *)highlightedFrameColor
{
	return [[self cell] highlightedFrameColor];
}

- (void)setHighlightedFrameColor:(NSColor *)newHighlightedFrameColor
{
	[[self cell] setHighlightedFrameColor:newHighlightedFrameColor];
}

- (NSColor *)highlightedTextColor
{
	return [[self cell] highlightedTextColor];
}

- (void)setHighlightedTextColor:(NSColor *)newHighlightedTextColor
{
	[[self cell] setHighlightedTextColor:newHighlightedTextColor];
}

- (NSColor *)highlightedArrowColor
{
	return [[self cell] highlightedArrowColor];
}

- (void)setHighlightedArrowColor:(NSColor *)newHighlightedArrowColor
{
	[[self cell] setHighlightedArrowColor:newHighlightedArrowColor];
}

- (NSShadow *)textShadow
{
	return [[self cell] textShadow];
}

- (void)setTextShadow:(NSShadow *)newTextShadow
{
	[[self cell] setTextShadow:newTextShadow];
}

- (NSShadow *)mouseoverTextShadow
{
	return [[self cell] mouseoverTextShadow];
}

- (void)setMouseoverTextShadow:(NSShadow *)newMouseoverTextShadow
{
	[[self cell] setMouseoverTextShadow:newMouseoverTextShadow];
}

- (NSShadow *)highlightedTextShadow
{
	return [[self cell] highlightedTextShadow];
}

- (void)setHighlightedTextShadow:(NSShadow *)newHighlightedTextShadow
{
	[[self cell] setHighlightedTextShadow:newHighlightedTextShadow];
}

- (NSShadow *)highlightedControlShadow
{
	return [[self cell] highlightedControlShadow];
}

- (void)setHighlightedControlShadow:(NSShadow *)newHighlightedControlShadow
{
	[[self cell] setHighlightedControlShadow:newHighlightedControlShadow];
}

- (double)popUpMenuDelay
{
    return [[self cell] popUpMenuDelay];
}

- (void)setPopUpMenuDelay:(double)newPopUpMenuDelay
{
	[[self cell] setPopUpMenuDelay:newPopUpMenuDelay];
}


- (BOOL)isOpaque
{
	return NO;
}

- (void)mouseEntered:(NSEvent *)theEvent
{
	[[self cell] setMouseOver:YES];
	[self setNeedsDisplay:YES];
}

- (void)mouseExited:(NSEvent *)theEvent
{
	[[self cell] setMouseOver:NO];
	[self setNeedsDisplay:YES];
}

- (void)drawRect:(NSRect)aRect
{
	NSRect cellRect = [self frame];
	cellRect.origin = NSZeroPoint;
	cellRect = NSInsetRect(cellRect, am_insetCell, am_insetCell);
	[[self cell] drawWithFrame:cellRect inView:self];
}

- (void)sizeToFit
{
	NSRect cellRect = [self frame];
	cellRect.origin = NSZeroPoint;
	cellRect = NSInsetRect(cellRect, am_insetCell, am_insetCell);
	float width = [[self cell] widthForFrame:cellRect];
	NSRect newFrame = [self frame];
	newFrame.size.width = width;
	[self setFrameSize:newFrame.size];
	[self am_resetTrackingRect];
}

- (void)frameDidChange:(NSNotification *)aNotification
{
	[self am_resetTrackingRect];
}

- (void)removeFromSuperview
{
	if (am_trackingRect) {
		[self removeTrackingRect:am_trackingRect];
	}
	[super removeFromSuperview];
}

- (void)removeFromSuperviewWithoutNeedingDisplay
{
	if (am_trackingRect) {
		[self removeTrackingRect:am_trackingRect];
	}
	[super removeFromSuperviewWithoutNeedingDisplay];
}

#pragma mark micks extensions

- (int)state {
    return [[self cell] state];
}

- (void)setState:(int)value {
    [[self cell] setState:value];
    [self setNeedsDisplay:YES];
}

@end

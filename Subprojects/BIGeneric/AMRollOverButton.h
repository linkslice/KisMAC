//
//  AMRollOverButton.h
//  PlateControl
//
//  Created by Andreas on Sat Jan 24 2004.
//  Copyright (c) 2004 Andreas Mayer. All rights reserved.
//

#import <AppKit/AppKit.h>


@interface AMRollOverButton : NSControl {
	NSTrackingRectTag am_trackingRect;
}

- (id)initWithFrame:(NSRect)frameRect andTitle:(NSString *)title;

// these are convienience methods that invoke the equally named cell methods

- (NSString *)title;
- (IBAction)setTitle:(NSString *)newTitle;

- (NSControlSize)controlSize;
- (void)setControlSize:(NSControlSize)newControlSize;

- (NSColor *)controlColor;
- (void)setControlColor:(NSColor *)newControlColor;

- (NSColor *)frameColor;
- (void)setFrameColor:(NSColor *)newFrameColor;

- (NSColor *)textColor;
- (void)setTextColor:(NSColor *)newTextColor;

- (NSColor *)arrowColor;
- (void)setArrowColor:(NSColor *)newArrowColor;

- (NSColor *)mouseoverControlColor;
- (void)setMouseoverControlColor:(NSColor *)newMouseoverControlColor;

- (NSColor *)mouseoverFrameColor;
- (void)setMouseoverFrameColor:(NSColor *)newMouseoverFrameColor;

- (NSColor *)mouseoverTextColor;
- (void)setMouseoverTextColor:(NSColor *)newMouseoverTextColor;

- (NSColor *)mouseoverArrowColor;
- (void)setMouseoverArrowColor:(NSColor *)newMouseoverArrowColor;

- (NSColor *)highlightedControlColor;
- (void)setHighlightedControlColor:(NSColor *)newHighlightedControlColor;

- (NSColor *)highlightedFrameColor;
- (void)setHighlightedFrameColor:(NSColor *)newHighlightedFrameColor;

- (NSColor *)highlightedTextColor;
- (void)setHighlightedTextColor:(NSColor *)newHighlightedTextColor;

- (NSColor *)highlightedArrowColor;
- (void)setHighlightedArrowColor:(NSColor *)newHighlightedArrowColor;

- (NSShadow *)textShadow;
- (void)setTextShadow:(NSShadow *)newTextShadow;

- (NSShadow *)mouseoverTextShadow;
- (void)setMouseoverTextShadow:(NSShadow *)newMouseoverTextShadow;

- (NSShadow *)highlightedTextShadow;
- (void)setHighlightedTextShadow:(NSShadow *)newHighlightedTextShadow;

- (NSShadow *)highlightedControlShadow;
- (void)setHighlightedControlShadow:(NSShadow *)newHighlightedControlShadow;

- (double)popUpMenuDelay;
- (void)setPopUpMenuDelay:(double)newPopUpMenuDelay;

#pragma mark micks extensions

- (int)state;
- (void)setState:(int)value;

@end

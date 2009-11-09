/*
        
        File:			ImportController.m
        Program:		KisMAC
	Author:			Michael Ro§berg
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

#import "ImportController.h"

@implementation ImportController

-(void)awakeFromNib
{
    _canceled = NO;
    _animate = YES;
    _isFullyInititialized = NO;
    [[self window] setDelegate:self];
    [aProgressBar setUsesThreadedAnimation:YES];
    [aProgressBar startAnimation:self];
    //[NSThread detachNewThreadSelector:@selector(animationThread:) toTarget:self withObject:nil];
    //[[self window] makeKeyAndOrderFront:self];
}

-(void)setMax:(float)max {
    [aProgressBar setIndeterminate:NO];
    [aProgressBar setMinValue:0.0];
    [aProgressBar setMaxValue:(double)max];
    [aProgressBar setDoubleValue:0.0];
    [aProgressBar animate:nil];
    [aProgressBar displayIfNeeded];
}
-(void)increment {
    [aProgressBar incrementBy:1.0];
    //[aProgressBar animate:nil];
    [aProgressBar displayIfNeeded];
}

-(void)animate {
    if (_isFullyInititialized) {
        NS_DURING
            [aProgressBar animate:nil];
            [aProgressBar displayIfNeeded];
        NS_HANDLER
        NS_ENDHANDLER
    }
}

-(void)stopAnimation {
     _animate = NO;
    [NSThread sleepUntilDate:[NSDate dateWithTimeIntervalSinceNow:0.2]];
}

- (bool)canceled {
    return _canceled;
}

- (void)setTitle:(NSString*)title {
    [[self window] setTitle:title];
    [_title setStringValue:title];
}

- (void)setStatusField:(NSString*)status {
    [_statusField setStringValue:status];
}

- (IBAction)cancelAction:(id)sender {
    _canceled = YES;
    [aCancel setEnabled:NO];
}

- (void)animationThread:(id)anObject {
    NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
    [NSThread setThreadPriority:0.0];	//we are not important
    NSDate *d;
    
    while(_animate) {
        [self animate];
        d = [[NSDate alloc] initWithTimeIntervalSinceNow:0.05];
        [NSThread sleepUntilDate:d];
        [d release];
    }
    
    [pool release];
}

- (void)terminateWithCode:(int)code {
    [NSApp endSheet:[self window] returnCode:code];
}

- (void)windowDidBecomeKey:(NSNotification *)aNotification {
    _isFullyInititialized = YES;
}

- (void)closeWindow:(NSWindow *)sheet returnCode:(int)returnCode contextInfo:(void *)contextInfo {
    _animate = NO;
    [NSThread sleepUntilDate:[NSDate dateWithTimeIntervalSinceNow:0.3]];
}


- (void)dealloc {
    if (_animate) {
        _animate = NO;
        [NSThread sleepUntilDate:[NSDate dateWithTimeIntervalSinceNow:0.3]];
    }
    [super dealloc];
}

@end

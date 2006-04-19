//
//  PrefsAdvanced.h
//  KisMAC
//
//  Created by themacuser on Mon Apr 3 2006.
//

#import <AppKit/AppKit.h>

#import "PrefsClient.h"

@interface PrefsAdvanced : PrefsClient
{
	IBOutlet id ac_ff; // aircrack fudge factor
	IBOutlet id bf_interval; // beacon flood interval
	IBOutlet id bpfdevice; // capture device
	IBOutlet id bpfloc;  // bpf location
	IBOutlet id pr_interval; //deauthenticate interval
}

-(IBAction)setDefaults:(id)sender;
@end

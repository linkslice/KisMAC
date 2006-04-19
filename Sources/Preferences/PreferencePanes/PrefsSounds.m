//
//  PrefsSounds.m
//  KisMAC
//
//  Created by Michael Thole on Mon Jan 20 2003.
//  Copyright (c) 2003 Michael Thole. All rights reserved.
//

#import "PrefsSounds.h"


@implementation PrefsSounds


-(void)updateUI {
    short numOfVoices;
    long voiceIndex;
    VoiceDescription theVoiceDesc;
    NSString *voiceName;
    VoiceSpec theVoiceSpec;

    NSEnumerator* sounds;
    id object;

    CountVoices(&numOfVoices);
    for (voiceIndex = 1; voiceIndex <= numOfVoices; voiceIndex++) {
        GetIndVoice(voiceIndex, &theVoiceSpec);
        GetVoiceDescription(&theVoiceSpec, &theVoiceDesc, sizeof(theVoiceDesc));
        voiceName = [NSString stringWithCString:(char*)&(theVoiceDesc.name[1]) length:theVoiceDesc.name[0]];
        [aVoices addItemWithTitle:voiceName];
    }

    [aGeigerSensity setIntValue:[[controller objectForKey:@"GeigerSensity"] intValue]];
    if ([aGeigerSensity intValue]<1) [aGeigerSensity setIntValue:1];

    [aGeigerSounds removeAllItems];
    [aWEPSounds removeAllItems];
    [aNOWEPSounds removeAllItems];

    sounds = [[[NSFileManager defaultManager]
            directoryContentsAtPath:@"/System/Library/Sounds"] objectEnumerator];

    [aGeigerSounds addItemWithTitle:[NSString stringWithString:@"None"]];
    [aWEPSounds addItemWithTitle:[NSString stringWithString:@"None"]];
    [aNOWEPSounds addItemWithTitle:[NSString stringWithString:@"None"]];

    [[aGeigerSounds menu] addItem:[NSMenuItem separatorItem]];
    [[aWEPSounds menu] addItem:[NSMenuItem separatorItem]];
    [[aNOWEPSounds menu] addItem:[NSMenuItem separatorItem]];

    while ((object = [sounds nextObject]) != nil) {
        [aGeigerSounds addItemWithTitle:[object stringByDeletingPathExtension]];
        [aWEPSounds addItemWithTitle:[object stringByDeletingPathExtension]];
        [aNOWEPSounds addItemWithTitle:[object stringByDeletingPathExtension]];
    }

    if([controller objectForKey:@"GeigerSound"] == nil || [controller objectForKey:@"WEPSound"] == nil ||
       [controller objectForKey:@"noWEPSound"] == nil) {
        [controller setObject:@"None" forKey:@"WEPSound"];
        [controller setObject:@"None" forKey:@"noWEPSound"];
        [controller setObject:@"None" forKey:@"GeigerSound"];
    }
    [aGeigerSounds selectItemWithTitle:[controller objectForKey:@"GeigerSound"]];
    [aWEPSounds selectItemWithTitle:[controller objectForKey:@"WEPSound"]];
    [aNOWEPSounds selectItemWithTitle:[controller objectForKey:@"noWEPSound"]];

    [aVoices selectItemAtIndex:[[controller objectForKey:@"Voice"] intValue]];
}

-(BOOL)updateDictionary {
    
    [aGeigerSensity validateEditing];
    [controller setObject:[NSNumber numberWithInt:[aGeigerSensity intValue]] forKey:@"GeigerSensity"];

    return YES;
}

-(IBAction)setValueForSender:(id)sender {
    if(sender == aVoices) {
        [self playVoice:sender];
        [controller setObject:[NSNumber numberWithInt:[sender indexOfSelectedItem]] forKey:@"Voice"];
    }
    else if(sender == aWEPSounds) {
        [self playSound:sender];
        [controller setObject:[sender titleOfSelectedItem] forKey:@"WEPSound"];
    }
    else if(sender == aNOWEPSounds) {
        [self playSound:sender];
        [controller setObject:[sender titleOfSelectedItem] forKey:@"noWEPSound"];
    }
    else if(sender == aGeigerSounds) {
        [self playSound:sender];
        [controller setObject:[sender titleOfSelectedItem] forKey:@"GeigerSound"];
    }
    else if (sender == aGeigerSensity) {
        [controller setObject:[NSNumber numberWithInt:[sender intValue]] forKey:@"GeigerSensity"];
    }
    else {
        NSLog(@"Error: Invalid sender(%@) in setValueForSender:",sender);
    }
}

#pragma mark -

- (IBAction)playSound:(id)sender {
    [[NSSound soundNamed:[sender titleOfSelectedItem]] play];
}

- (IBAction) playVoice:(id)sender
{
    char cSentence[]= "Found network. SSID is TEST.";
    if ([sender indexOfSelectedItem]>0)
        [WaveHelper speakSentence:cSentence withVoice:[sender indexOfSelectedItem]];
}

@end

//
//  #PROJNAMEAppDelegate.m
//  #PROJNAMEios
//
//  Created by James McClellan on 8/26/12.
//  Copyright (c) 2012 __MyCompanyName__. All rights reserved.
//

#import "#PROJNAMEAppDelegate.h"
#import "#PROJNAMEViewController.h"
#import "jsaubase.h"
#include "CAStreamBasicDescription.h"

@implementation #PROJNAMEAppDelegate

@synthesize window = _window;
@synthesize viewController = _viewController;

- (void)dealloc
{
    [_window release];
    [_viewController release];
    [super dealloc];
}

#define kInputBus 1
#define kOutputBus 0

- (BOOL)application:(UIApplication *)application didFinishLaunchingWithOptions:(NSDictionary *)launchOptions
{
    self.window = [[[UIWindow alloc] initWithFrame:[[UIScreen mainScreen] bounds]] autorelease];
    // Override point for customization after application launch.
    if ([[UIDevice currentDevice] userInterfaceIdiom] == UIUserInterfaceIdiomPhone) {
        self.viewController = [[[#PROJNAMEViewController alloc] initWithNibName:@"#PROJNAMEViewController_iPhone" bundle:nil] autorelease];
    } else {
        self.viewController = [[[#PROJNAMEViewController alloc] initWithNibName:@"#PROJNAMEViewController_iPad" bundle:nil] autorelease];
    }
    self.window.rootViewController = self.viewController;
    [self.window makeKeyAndVisible];
    
    // first, register the AU
    DoRegister('#AUTYPE', '#TYPECODE', '#MANUCODE', CFSTR("#NAME"), 0x00000001);
    
    // set up the AUGraph
    NewAUGraph(&mGraph);
    // open it
    AUGraphOpen(mGraph);
    
    // So we want two units - the "IO" unit, and our custom unit.
    AudioComponentDescription ioDesc = {0};
    ioDesc.componentType = kAudioUnitType_Output;
    ioDesc.componentSubType = kAudioUnitSubType_RemoteIO;
    ioDesc.componentManufacturer = kAudioUnitManufacturer_Apple;
    
    AudioComponentDescription effectDesc = {0};
    effectDesc.componentType = '#AUTYPE';
	effectDesc.componentSubType = '#TYPECODE';
	effectDesc.componentManufacturer = '#MANUCODE';
    
    AUNode effectNode, ioNode;
    AUGraphAddNode(mGraph, &ioDesc, &ioNode);
    AUGraphAddNode(mGraph, &effectDesc, &effectNode);
    
    AudioUnit ioComponent;
    AUGraphNodeInfo(mGraph, ioNode, NULL, &ioComponent);
    AUGraphNodeInfo(mGraph, effectNode, NULL, &mEffect);
    
    // Enable input
    UInt32 one = 1;
    AudioUnitSetProperty(ioComponent,
                         kAudioOutputUnitProperty_EnableIO,
                         kAudioUnitScope_Input, 
                         kInputBus, 
                         &one, 
                         sizeof(one));
                         
    // need to deal with the stream formats.  What a bore.
    CAStreamBasicDescription format;
    UInt32 size = sizeof(AudioStreamBasicDescription);
    format.SetAUCanonical(2, false);
    
    AudioUnitSetProperty(   ioComponent,
                            kAudioUnitProperty_StreamFormat,
							kAudioUnitScope_Output,
							kInputBus,
							&format,
							size );
                                     
    // connect the graph!
    AUGraphConnectNodeInput(mGraph, ioNode, kInputBus, effectNode, 0);
	AUGraphConnectNodeInput(mGraph, effectNode, 0, ioNode, kOutputBus);
	
    // initialize.
	AUGraphInitialize(mGraph);
    
    // run!
    AUGraphStart(mGraph);
    
    // initialize GUI
    [_viewController setAU: mEffect];
    
    return YES;
}

- (void)applicationWillResignActive:(UIApplication *)application
{
    /*
     Sent when the application is about to move from active to inactive state. This can occur for certain types of temporary interruptions (such as an incoming phone call or SMS message) or when the user quits the application and it begins the transition to the background state.
     Use this method to pause ongoing tasks, disable timers, and throttle down OpenGL ES frame rates. Games should use this method to pause the game.
     */
}

- (void)applicationDidEnterBackground:(UIApplication *)application
{
    /*
     Use this method to release shared resources, save user data, invalidate timers, and store enough application state information to restore your application to its current state in case it is terminated later. 
     If your application supports background execution, this method is called instead of applicationWillTerminate: when the user quits.
     */
}

- (void)applicationWillEnterForeground:(UIApplication *)application
{
    /*
     Called as part of the transition from the background to the inactive state; here you can undo many of the changes made on entering the background.
     */
}

- (void)applicationDidBecomeActive:(UIApplication *)application
{
    /*
     Restart any tasks that were paused (or not yet started) while the application was inactive. If the application was previously in the background, optionally refresh the user interface.
     */
}

- (void)applicationWillTerminate:(UIApplication *)application
{
    /*
     Called when the application is about to terminate.
     Save data if appropriate.
     See also applicationDidEnterBackground:.
     */
    AUGraphClose(mGraph);
    DisposeAUGraph(mGraph);
}

@end

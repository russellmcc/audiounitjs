//
//  #PROJNAMEViewController.m
//  #PROJNAMEios
//
//  Created by James McClellan on 8/26/12.
//  Copyright (c) 2012 __MyCompanyName__. All rights reserved.
//

#import "#PROJNAMEViewController.h"
#include "AUWrapper.h"
#include "obsfu.h"

@implementation #PROJNAMEViewController

- (void)didReceiveMemoryWarning
{
    [super didReceiveMemoryWarning];
    // Release any cached data, images, etc that aren't in use.
}

#pragma mark - View lifecycle

- (void)viewDidLoad
{
    [super viewDidLoad];
    
	// Do any additional setup after loading the view, typically from a nib.
}

- (void)viewDidUnload
{
    [super viewDidUnload];
    // Release any retained subviews of the main view.
    // e.g. self.myOutlet = nil;
}

- (void)viewWillAppear:(BOOL)animated
{
    [super viewWillAppear:animated];
}

- (void)viewDidAppear:(BOOL)animated
{
    [super viewDidAppear:animated];
}

- (void)viewWillDisappear:(BOOL)animated
{
	[super viewWillDisappear:animated];
}

- (void)viewDidDisappear:(BOOL)animated
{
	[super viewDidDisappear:animated];
    if(mJSObj)
        [mJSObj release];
}

- (void)webView:(id)sender didClearWindowObject:(id)window forFrame:(WebFrame *)frame
{
    [window setValue:mJSObj forKey:@"AudioUnit"];
}

- (BOOL)shouldAutorotateToInterfaceOrientation:(UIInterfaceOrientation)interfaceOrientation
{
    // Return YES for supported orientations
    if ([[UIDevice currentDevice] userInterfaceIdiom] == UIUserInterfaceIdiomPhone) {
        return (interfaceOrientation != UIInterfaceOrientationPortraitUpsideDown);
    } else {
        return YES;
    }
}

-(void)setAU:(AudioUnit)au
{
    mAU = au;
    mJSObj = CreateAudioUnitObject(mAU);
    obsfuSet(mWebview, self);
    [mWebview loadRequest:[NSURLRequest requestWithURL:[[NSBundle mainBundle] URLForResource:@"index" withExtension:@"html" subdirectory:@"ui"]]];
}
@end

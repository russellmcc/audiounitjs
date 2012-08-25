#import "#PROJNAME_ViewFactory.h"

@implementation #PROJNAME_ViewFactory

-(NSView *)uiViewForAudioUnit:(AudioUnit)inAudioUnit withSize:(NSSize)inPreferredSize {
    if(![NSBundle loadNibNamed:@"webview" owner:self]) {
        NSLog(@"Unable to load nib from view");
        return nil;
    }
    
    [uiViewInstance setAU:inAudioUnit];
    NSView *returnView = [uiViewInstance webView];
    uiViewInstance = nil; // zero out pointer.  This is a view factory.  Once a view's been created
    
    // and handed off, the factory keeps no record of it.
    return [returnView autorelease];
}

- (NSString *) description {
    return [NSString stringWithString: @"Cocoa View"];
}

- (unsigned)interfaceVersion {
    return 0;
}

@end

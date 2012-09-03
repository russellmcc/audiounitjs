#import "#PROJNAME_WebView.h"
#include <vector>
#include "AUWrapper.h"

@implementation #PROJNAME_WebController

- (void)webView:(WebView *)sender didClearWindowObject:(WebScriptObject *)window forFrame:(WebFrame *)frame
{
    // set up the "AudioUnit" javascript object.
    mJSObj = CreateAudioUnitObject(mAU);
    
    [window setValue:mJSObj forKey:@"AudioUnit"];
}

- (void)setAU:(AudioUnit)inAU {
	// remove previous listeners
    mAU = inAU;
    
    // get the path to a local HTML file.
    NSURL* index = [mBundle URLForResource:@"index" withExtension:@"html" subdirectory:@"ui"];
    
    // fail silently now if there's no index.
    if(not index)
        return;
    
    [mWebview setFrameLoadDelegate:self];
    [mWebview setMainFrameURL:[index absoluteString]];
}

#include <dlfcn.h>

-(void) awakeFromNib
{
    mJSObj = 0;
#ifdef STANDALONE
    mBundle = [NSBundle mainBundle];
#else
    mBundle = [NSBundle bundleWithIdentifier:@"com.#COMPANY_UNDERSCORED.#PROJNAME.CocoaUI"];
#endif
}

-(void) dealloc
{
    if(mJSObj)
        [mJSObj release];
    [super dealloc];
}

- (WebView*) webView
{
    return mWebview;
}

@end

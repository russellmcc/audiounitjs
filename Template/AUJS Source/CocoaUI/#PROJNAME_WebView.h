#import <AudioUnit/AudioUnit.h>
#import <WebKit/WebKit.h>
#include "audioprops.h"

@interface #PROJNAME_WebController : NSObject
{
    NSBundle*               mBundle;
    IBOutlet WebView*       mWebview;
    
    // event listener for the AU.
    AudioUnit mAU;
    
    // the audio unit javascript object
    NSObject* mJSObj;
}

- (void)setAU:(AudioUnit)inAU;
- (void)awakeFromNib;
- (void)dealloc;
- (WebView*) webView;

@end

@interface #PROJNAME_WebView : WebView
- (void)setFrameOrigin:(NSPoint)newOrigin;
- (void)setBoundsOrigin:(NSPoint)newOrigin;
- (void)setFrame:(NSRect)frameRect;
- (void)setBounds:(NSRect)aRect;
@end

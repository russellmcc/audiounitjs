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

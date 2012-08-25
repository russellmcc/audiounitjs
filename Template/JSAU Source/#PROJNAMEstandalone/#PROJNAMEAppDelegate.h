#import <Cocoa/Cocoa.h>
#import "#PROJNAME_WebView.h"
#import <AudioToolbox/AudioToolbox.h>

class CAPlayThroughHost;

@interface #PROJNAMEAppDelegate : NSObject <NSApplicationDelegate> {
    IBOutlet #PROJNAME_WebController* uiViewInstance;
    @private NSWindow* _window;
    @private CAPlayThroughHost* host;
}
@property (assign) IBOutlet NSWindow *window;


@end

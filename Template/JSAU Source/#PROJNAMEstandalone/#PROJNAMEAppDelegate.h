#import <Cocoa/Cocoa.h>
#import "#PROJNAME_WebView.h"
#import <AudioToolbox/AudioToolbox.h>
#include "MIDIReceiver.h"

class CAPlayThroughHost;

@interface #PROJNAMEAppDelegate : NSObject <NSApplicationDelegate> {
    IBOutlet #PROJNAME_WebController* uiViewInstance;
    @private NSWindow* _window;
    @private CAPlayThroughHost* host;
    @private MIDIReceiver* receiver;
}
@property (assign) IBOutlet NSWindow *window;


@end

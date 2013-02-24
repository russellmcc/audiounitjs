#import "#PROJNAME_WebView.h"
#import <AudioUnit/AUCocoaUIView.h>

@interface #PROJNAME_ViewFactory : NSObject<AUCocoaUIBase>
{
    IBOutlet #PROJNAME_WebController * uiViewInstance;
}

- (NSString *) description;
@end

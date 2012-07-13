#import "#PROJNAME_CefView.h"
#import <AudioUnit/AUCocoaUIView.h>

@interface #PROJNAME_ViewFactory : NSObject<AUCocoaUIBase>
{
    IBOutlet #PROJNAME_CefView * uiViewInstance;
}

- (NSString *) description;
@end

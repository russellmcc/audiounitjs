#import <AudioUnit/AudioUnit.h>

@interface #PROJNAME_CefView : NSView
{
    AudioUnit 				mAU;
    NSBundle*               mBundle;
}

- (void)setAU:(AudioUnit)inAU;
- (void)awakFromNib;

@end

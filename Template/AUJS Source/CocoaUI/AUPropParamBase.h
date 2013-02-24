//
//  AUPropParamBase.h
//
//  Created by James McClellan on 8/25/12.
//

#import <Foundation/Foundation.h>
#import <AudioToolbox/AudioToolbox.h>

#if IPHONE_VER
    #include "iosAUEvents.h"
#endif

@interface #PROJNAME_AUPropParamBase : NSObject
{
    AudioUnit mAU;
    UInt32 mID;
    AUEventListenerRef mListener;
    id OnChange;
}

-(id)initWithAU:(AudioUnit)au withID:(UInt32)id;
-(void)dealloc;
-(void)initListenerType:(AudioUnitEventType)type;
+(BOOL)isSelectorExcludedFromWebScript:(SEL)s;
+(BOOL)isKeyExcludedFromWebScript:(const char*)c;

@property (retain) id OnChange;
@end

//
//  AUPropParamBase.h
//  #PROJNAME
//
//  Created by James McClellan on 8/25/12.
//

#import <Foundation/Foundation.h>
#import <WebKit/WebKit.h>
#include <AudioToolbox/AudioToolbox.h>

@interface #PROJNAME_AUPropParamBase : NSObject
{
    AudioUnit mAU;
    UInt32 mID;
    AUEventListenerRef mListener;
    JSGlobalContextRef mContext;
    id OnChange;
}

-(id)initWithAU:(AudioUnit)au withID:(UInt32)id withContext:(JSGlobalContextRef)context;
-(void)dealloc;
-(void)initListenerType:(AudioUnitEventType)type;
+(BOOL)isSelectorExcludedFromWebScript:(SEL)s;
+(BOOL)isKeyExcludedFromWebScript:(const char*)c;

@property (retain) id OnChange;
@property (readonly) JSGlobalContextRef context;
@end

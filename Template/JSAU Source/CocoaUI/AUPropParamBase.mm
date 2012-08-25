//
//  AUPropParamBase.m
//  #PROJNAME
//
//  Created by James McClellan on 8/25/12.
//

#import "AUPropParamBase.h"
#import <JavaScriptCore/JavaScriptCore.h>
#include "audioprops.h"
#include "LowLevelCocoaUtils.h"

using namespace LowLevelCocoaUtils;

namespace
{
    void DispatchChangeEvent(void *refCon, void *object, const AudioUnitEvent *event, UInt64 hostTime, Float32 value);
    bool IsEventTypeForProperties(AudioUnitEventType t)
    {
        return t == kAudioUnitEvent_PropertyChange;
    }
}

@implementation #PROJNAME_AUPropParamBase
@synthesize OnChange, context=mContext;
-(id)initWithAU:(AudioUnit)au withID:(UInt32)id withContext:(JSGlobalContextRef)context
{
    self = [super init];
    
    mAU = au;
    mID = id;
    mContext = context;
    JSGlobalContextRetain(mContext);
    
    OnChange = 0;
    
    AUEventListenerRef tRef;
    AUEventListenerCreate(DispatchChangeEvent, self,
                    CFRunLoopGetCurrent(), kCFRunLoopDefaultMode, 0.1, 0.1, 
                    &tRef);
    
    mListener = tRef;
    return self;
}

-(void)dealloc
{
    AUListenerDispose(mListener);
    JSGlobalContextRelease(mContext);
    [super dealloc];
}

-(void)initListenerType:(AudioUnitEventType)type
{
    AudioUnitEvent auEvent;
    if(IsEventTypeForProperties(type))
    {
        AudioUnitProperty auProp = {mAU, mID + kFirstAudioProp, kAudioUnitScope_Global, 0};
        auEvent.mArgument.mProperty = auProp;
    }
    else 
    {
        AudioUnitParameter parameter = {mAU, mID, kAudioUnitScope_Global, 0 };
        auEvent.mArgument.mParameter = parameter;
    }
    auEvent.mEventType = type;
    
    // now, hook the listener up to the event type.
    AUEventListenerAddEventType(mListener, 
                                self,
                                &auEvent);
}

+(BOOL)isSelectorExcludedFromWebScript:(SEL)s
{
    return NO;
}

+(BOOL)isKeyExcludedFromWebScript:(const char*)key
{
    // anything beginning with a non-uppercase letter is allowed.
    return not isupper(*key);
}

@end

namespace
{
    struct EventInfo
    {
        const char* mJSName;
        AudioUnitEventType mAUType;
    };
    
    const EventInfo kEventInfo[] = {  
                                {"OnChange", kAudioUnitEvent_ParameterValueChange},
                                {"OnBeginGesture", kAudioUnitEvent_BeginParameterChangeGesture},
                                {"OnEndGesture", kAudioUnitEvent_EndParameterChangeGesture},
                                {"OnChange", kAudioUnitEvent_PropertyChange}
                             };
    
    const char* GetEventNameForType(AudioUnitEventType t)
    {
        // for now, we take advantage of the fact that the enums match up with arry indices.
        // if the enum becomes more complicated we can't do this.
        return kEventInfo[t].mJSName;
    }
    
    void DispatchChangeEvent(void *refCon, void *object, const AudioUnitEvent *event, UInt64 hostTime, Float32 value)
    {
        id self = (id)refCon;
        const char* ivarName = GetEventNameForType(event->mEventType);

        // now, get the relevant ivar.
        try {
            id jsObj = doGet<id>(self, ivarName);
            
            // check if it can be called
            if(jsObj)
            {
                // okay, so if we have a jsObj it was set via javascript and is a webscriptobject.
                NSMutableArray* args = [[NSMutableArray alloc] initWithCapacity:2];
                [args addObject:[NSNull null]];
                [args addObject:[NSNumber numberWithDouble: value]];
                [jsObj callWebScriptMethod:@"call" withArguments:args];
                [args release];
            }
        }
        catch(...)
        {
            // no big deal, just don't do anything.
        }
    }    
}



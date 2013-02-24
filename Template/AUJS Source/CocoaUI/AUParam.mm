//
//  AUParam.m
//  #PROJNAME
//
//  Created by James McClellan on 8/25/12.
//

#import "AUParam.h"

@implementation #PROJNAME_AUParam

@synthesize OnBeginGesture, OnEndGesture;

-(id)initWithAU:(AudioUnit)au withID:(UInt32)id
{
    self = [super initWithAU:au withID:id];
    [self initListenerType:kAudioUnitEvent_ParameterValueChange];
    [self initListenerType:kAudioUnitEvent_BeginParameterChangeGesture];
    [self initListenerType:kAudioUnitEvent_EndParameterChangeGesture];
    
    OnBeginGesture = OnEndGesture = 0;
    
    return self;
}

-(id)Get {
    AudioUnitParameterValue val = 0;
    AudioUnitGetParameter(mAU, mID, kAudioUnitScope_Global, 0, &val);
    return [NSNumber numberWithDouble:val];
}

-(void)Set:(id)val {
    if(not [val respondsToSelector:@selector(doubleValue)])
        return;
    
    double v = (double)[val doubleValue];
    AudioUnitParameter param = {mAU, mID, kAudioUnitScope_Global, 0 };
	AUParameterSet(mListener, 0, &param, (Float32)v, 0);
}

-(void)BeginGesture {
    AudioUnitEvent event;
    AudioUnitParameter param = {mAU, mID, kAudioUnitScope_Global, 0};
    event.mArgument.mParameter = param;
    event.mEventType = kAudioUnitEvent_BeginParameterChangeGesture;
            
    AUEventListenerNotify(mListener, 0, &event);
}

-(void)EndGesture {
    AudioUnitEvent event;
    AudioUnitParameter param = {mAU, mID, kAudioUnitScope_Global, 0};
    event.mArgument.mParameter = param;
    event.mEventType = kAudioUnitEvent_EndParameterChangeGesture;
            
    AUEventListenerNotify(mListener, 0, &event);
}

+(NSString *)webScriptNameForSelector:(SEL)sel {
    if(sel == @selector(Set:))
        return @"Set";
        
    return nil;
}

@end

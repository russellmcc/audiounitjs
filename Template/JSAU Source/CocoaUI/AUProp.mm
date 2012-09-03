//
//  AUProp.m
//  #PROJNAME
//
//  Created by James McClellan on 8/25/12.
//

#import "AUProp.h"


@implementation #PROJNAME_AUProp
-(id)initWithAU:(AudioUnit)au withID:(UInt32)id withType:(JSPropDesc::JSType)type
{
    [super initWithAU:au withID:id];
    [self initListenerType:kAudioUnitEvent_PropertyChange];
    mType = type;
    return self;
}


//////////////////////////////
// private helpers for Get
//////////////////////////////

-(UInt32)getAUID
{
    return mID + kFirstAudioProp;
}

-(UInt32)getPropertySize
{
    UInt32 propSize;
    OSStatus status = AudioUnitGetPropertyInfo(mAU, [self getAUID], kAudioUnitScope_Global, 0, &propSize, 0);
    if(status != noErr) throw status;
    return propSize;
}

-(void)getPropertyWithRetval:(void*)retval withSize:(UInt32)size
{
    AudioUnitGetProperty(mAU, [self getAUID], kAudioUnitScope_Global, 0, retval, &size);
}

-(NSNumber*)getNumberProperty
{
    // if the size is wrong, something's messed up!
    if([self getPropertySize] != sizeof(double)) return [NSNumber numberWithDouble:0];
    
    double retval;
    [self getPropertyWithRetval:&retval withSize:sizeof(retval)];
    return [NSNumber numberWithDouble:retval];
}

-(NSArray*) getArrayProperty
{
    UInt32 size = [self getPropertySize];
    UInt32 count = size / sizeof(double);
    double arr[count];
    [self getPropertyWithRetval:arr withSize:size];
    NSMutableArray* nsArr = [[NSMutableArray alloc] initWithCapacity:count];
    for(int i = 0; i < count; ++i)
        [nsArr addObject:[NSNumber numberWithDouble:arr[i]]];
    return nsArr;
}

-(NSString*) getStringProperty
{
    UInt32 size = [self getPropertySize];
    char cStr[size];
    [self getPropertyWithRetval:cStr withSize:size];
    return [NSString stringWithUTF8String:cStr];
}

// switch based on my type.
-(id)Get
{
    switch(mType)
    {
        case JSPropDesc::kJSNumber:
            return [self getNumberProperty];
            break;
        case JSPropDesc::kJSString:
            return [self getStringProperty];
            break;
        case JSPropDesc::kJSNumberArray:
            return [self getArrayProperty];
            break;
    }
    return nil;
}

@end

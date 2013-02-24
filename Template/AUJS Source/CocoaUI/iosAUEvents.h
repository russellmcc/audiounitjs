//
//  iosAUEvents.h
//
//  Created by James Russell McClellan on 9/3/12.
//
//

#ifndef iosAUEvents_h
#define iosAUEvents_h

class OpaqueAUEventListener;
typedef OpaqueAUEventListener* AUEventListenerRef;

enum {
    kAudioUnitEvent_ParameterValueChange        = 0,
    kAudioUnitEvent_BeginParameterChangeGesture = 1,
    kAudioUnitEvent_EndParameterChangeGesture   = 2,
    kAudioUnitEvent_PropertyChange              = 3
};
typedef UInt32 AudioUnitEventType;

typedef struct AudioUnitEvent {
    AudioUnitEventType                  mEventType;
    union {
        AudioUnitParameter  mParameter; // for parameter value change, begin and end gesture
        AudioUnitProperty   mProperty;  // for kAudioUnitEvent_PropertyChange
    }                                   mArgument;
} AudioUnitEvent;

typedef void (*AUEventListenerProc)(void *                      inUserData,
                                    void *                      inObject,
                                    const AudioUnitEvent *      inEvent,
                                    UInt64                      inEventHostTime,
                                    AudioUnitParameterValue     inParameterValue);

extern OSStatus
AUEventListenerCreate(              AUEventListenerProc         inProc,
                                    void *                      inUserData,
                                    CFRunLoopRef                inRunLoop,
                                    CFStringRef                 inRunLoopMode,
                                    Float32                     inNotificationInterval,     // seconds
                                    Float32                     inValueChangeGranularity,   // seconds
                                    AUEventListenerRef *        outListener);

extern OSStatus
AUListenerDispose(                  AUEventListenerRef          inListener);

extern OSStatus
AUEventListenerAddEventType(        AUEventListenerRef          inListener,
                                    void *                      inObject,
                                    const AudioUnitEvent *      inEvent);
                                    
extern OSStatus
AUParameterSet(                     AUEventListenerRef          inSendingListener, 
                                    void *                          inSendingObject,
                                    const AudioUnitParameter *      inParameter,
                                    AudioUnitParameterValue         inValue,
                                    UInt32                          inBufferOffsetInFrames);
                                    
extern OSStatus
AUEventListenerNotify(              AUEventListenerRef          inSendingListener,
                                    void *                      inSendingObject,
                                    const AudioUnitEvent *      inEvent);
#endif

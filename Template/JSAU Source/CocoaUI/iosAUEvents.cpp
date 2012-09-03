//
//  iosAUEvents.cpp
//
//  Created by James Russell McClellan on 9/3/12.
//
//

#include "iosAUEvents.h"
#include <vector>
#include <mach/mach_time.h>

using namespace std;

namespace
{
    struct ListeningProp
    {
        ListeningProp(AudioUnitProperty prop, void* user) :
            mProp(prop), mUser(user)
        {}
        
        AudioUnitProperty mProp;
        void*             mUser;
        vector<UInt64>    mPendingHostTimes;
    };
}

class OpaqueAUEventListener
{
    public:
        OpaqueAUEventListener(AUEventListenerProc proc,
                              void*               user,
                              CFRunLoopRef        runLoop,
                              CFStringRef         mode,
                              Float32             interval,
                              Float32             granularity);
        
        ~OpaqueAUEventListener();
        void PushPropToRunLoop(const AudioUnitProperty& prop);
        void StartListeningToProp(void* user, const AudioUnitProperty& prop);
        void HandlePendingEvents();
        
    private:
        AUEventListenerProc mProc;
        void*               mUser;
        CFRunLoopRef        mRunLoop;
        CFStringRef         mMode;
        UInt64              mGranularity;
        UInt64              mLastNotification;
        vector<ListeningProp> mListeningProps;
        CFRunLoopTimerRef   mTimer;
};

namespace
{
    void TimerCallback(CFRunLoopTimerRef timer, void *info)
    {
        AUEventListenerRef eventListener = reinterpret_cast<AUEventListenerRef>(info);
        eventListener->HandlePendingEvents();
    }
}

OSStatus
AUEventListenerCreate(              AUEventListenerProc         inProc,
                                    void *                      inUserData,
                                    CFRunLoopRef                inRunLoop,
                                    CFStringRef                 inRunLoopMode,
                                    Float32                     inNotificationInterval,     // seconds
                                    Float32                     inValueChangeGranularity,   // seconds
                                    AUEventListenerRef *        outListener)
{
    *outListener = new OpaqueAUEventListener(inProc, inUserData, inRunLoop, inRunLoopMode, inNotificationInterval, inValueChangeGranularity);
    return noErr;
}

OSStatus
AUListenerDispose(                  AUEventListenerRef          inListener)
{
    delete inListener;
    return noErr;
}

namespace
{
    void ListenProc (	void *				inRefCon,
                        AudioUnit			inUnit,
                        AudioUnitPropertyID	inID,
                        AudioUnitScope		inScope,
                        AudioUnitElement	inElement)
    {
        AUEventListenerRef listener = reinterpret_cast<AUEventListenerRef>(inRefCon);
        AudioUnitProperty prop = {inUnit, inID, inScope, inElement};
        listener->PushPropToRunLoop(prop);
    }
}

OpaqueAUEventListener::OpaqueAUEventListener(AUEventListenerProc proc,
                                             void*               user,
                                             CFRunLoopRef        runLoop,
                                             CFStringRef         mode,
                                             Float32             interval,
                                             Float32             granularity) :
            mProc(proc),
            mUser(user),
            mRunLoop(runLoop),
            mMode(mode),
            mLastNotification(0)
{
    // we need to convert granularity from seconds to host time ticks.
    struct mach_timebase_info timeBaseInfo;
    mach_timebase_info(&timeBaseInfo);
    Float64 freq = static_cast<Float64>(timeBaseInfo.denom) / static_cast<Float64>(timeBaseInfo.numer);
    mGranularity = freq * granularity;
    
    // now, set up the run loop at the interval frequency.
    CFRunLoopTimerContext context = {0, this, 0, 0, 0};
    mTimer = CFRunLoopTimerCreate (kCFAllocatorDefault, 0, interval, 0, 0, TimerCallback, &context);
    
    CFRunLoopAddTimer(mRunLoop, mTimer, mMode);
}

OpaqueAUEventListener::~OpaqueAUEventListener()
{
    CFRunLoopRemoveTimer(mRunLoop, mTimer, mMode);
    
    // remove all the listeners.
    for(vector<ListeningProp>::iterator i = mListeningProps.begin(); i != mListeningProps.end(); ++i)
    {
        AudioUnitRemovePropertyListenerWithUserData(
									i->mProp.mAudioUnit,
									i->mProp.mPropertyID,
									ListenProc,
									this);
    }
}

void
OpaqueAUEventListener::StartListeningToProp(void* user, const AudioUnitProperty& prop)
{
    AudioUnitAddPropertyListener(prop.mAudioUnit,
                                 prop.mPropertyID,
								 ListenProc,
                                 this);
    mListeningProps.push_back(ListeningProp(prop, user));
}

void
OpaqueAUEventListener::HandlePendingEvents()
{
    for(vector<ListeningProp>::iterator i = mListeningProps.begin(); i != mListeningProps.end(); ++i)
    {
        if(i->mPendingHostTimes.size())
        {
            // cull with granularity.
            UInt64 firstTime = i->mPendingHostTimes[0];
            UInt64 notifyTime = firstTime;
            vector<UInt64>::iterator j = i->mPendingHostTimes.begin() + 1;
            for(; j != i->mPendingHostTimes.end(); ++j)
            {
                if(*j >= firstTime + mGranularity)
                    break;
                notifyTime = *j;
            }
            i->mPendingHostTimes.erase(i->mPendingHostTimes.begin(), j);
            
            CFRunLoopPerformBlock(mRunLoop, mMode, ^{
                AudioUnitEvent ev;
                ev.mEventType = kAudioUnitEvent_PropertyChange;
                ev.mArgument.mProperty = i->mProp;
                
                mProc(mUser,
                      i->mUser,
                      &ev,
                      notifyTime,
                      0);
            });
        }
    }
}

void
OpaqueAUEventListener::PushPropToRunLoop(const AudioUnitProperty& prop)
{
    UInt64 time = mach_absolute_time();
    CFRunLoopPerformBlock(mRunLoop, mMode, ^{
        // check to see if this event matches any that we are listening to.
        for(vector<ListeningProp>::iterator i = mListeningProps.begin(); i != mListeningProps.end(); ++i)
        {
            if(memcmp(&prop, &i->mProp, sizeof(AudioUnitProperty)) == 0)
                i->mPendingHostTimes.push_back(time);
        }
    });
    CFRunLoopWakeUp(mRunLoop);
}

OSStatus
AUEventListenerAddEventType(        AUEventListenerRef          inListener,
                                    void *                      inObject,
                                    const AudioUnitEvent *      inEvent)
{
    // we only care about property listeners.
    if(inEvent->mEventType == kAudioUnitEvent_PropertyChange)
    {
        inListener->StartListeningToProp(inObject, inEvent->mArgument.mProperty);
    }
    
    return noErr;
}

OSStatus
AUParameterSet(                     AUEventListenerRef          inSendingListener, 
                                    void *                          inSendingObject,
                                    const AudioUnitParameter *      inParameter,
                                    AudioUnitParameterValue         inValue,
                                    UInt32                          inBufferOffsetInFrames)
{
    // we don't really need to notify on iOS because only the UI can ever change the parameters.
    AudioUnitSetParameter(inParameter->mAudioUnit,
                          inParameter->mParameterID,
                          inParameter->mScope,
                          inParameter->mElement,
                          inValue,
                          inBufferOffsetInFrames
                          );
    return noErr;
}

OSStatus
AUEventListenerNotify(              AUEventListenerRef          inSendingListener,
                                    void *                      inSendingObject,
                                    const AudioUnitEvent *      inEvent)
{
    // again, we don't really have a host, so we don't need to do anything.
    return noErr;
}

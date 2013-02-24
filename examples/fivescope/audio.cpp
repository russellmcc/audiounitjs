#include "jsaubase.h"

// PARAMETERS GO HERE
enum
{
	kParam_TriggerLevel
};

// PROPERTIES GO HERE
enum
{
    kProp_ScopeData = kFirstAudioProp
};


class Audio;

// This is the main plug in class.
class Audio : public JSAudioUnitBase {
public:
	Audio(AudioUnit component);
	virtual OSStatus Version() { return 0xFFFFFF; }
    
    virtual OSStatus GetProperty(AudioUnitPropertyID id, AudioUnitScope scope, 
                                 AudioUnitElement elem, void* data);
    virtual OSStatus GetPropertyInfo (AudioUnitPropertyID	id,
                                      AudioUnitScope		scope,
                                      AudioUnitElement	elem,
                                      UInt32 &		size,
                                      Boolean &    writable);
	virtual OSStatus GetParameterInfo(	AudioUnitScope			inScope,
                                        AudioUnitParameterID	inParameterID,
                                        AudioUnitParameterInfo	&outParameterInfo );
    
    virtual UInt32   SupportedNumChannels (const AUChannelInfo** outInfo);
    
    virtual OSStatus	Reset(		AudioUnitScope 				inScope,
                                    AudioUnitElement 			inElement);
    
    virtual OSStatus    ProcessBufferLists (AudioUnitRenderActionFlags& ioActionFlags,
                           const AudioBufferList& inBuffer,
                           AudioBufferList& outBuffer,
                           UInt32 numSamples);
    
    virtual OSStatus	HandleNoteOn(UInt8 	inChannel,
                                     UInt8 	inNoteNumber,
                                     UInt8 	inVelocity,
                                     UInt32 	inStartFrame);
    
    virtual OSStatus	HandleNoteOff(UInt8 	inChannel,
                                     UInt8 	inNoteNumber,
                                     UInt8 	inVelocity,
                                     UInt32 	inStartFrame);
    
    virtual std::vector<JSPropDesc> GetPropertyDescriptionList();
    
private:
    double mScopeData[400];
    int mCurrScopeInd;
    bool mTriggered;
};

// this boilerplate has to be here so that the system can know about the 
// class we just made.
AUDIOCOMPONENT_ENTRY(AUMIDIEffectFactory, Audio)
void DoRegister(OSType Type, OSType Subtype, OSType Manufacturer, CFStringRef name, UInt32 vers)
{
    AUMIDIEffectFactory<Audio>::Register(Type, Subtype, Manufacturer, name, vers, 0);
}

Audio::Audio(AudioUnit component) : JSAudioUnitBase(component)
{
    memset(mScopeData, 0, sizeof(mScopeData));
    SetParameter(kParam_TriggerLevel, 0.0);
    mTriggered = false;
    mCurrScopeInd = 0;
}


// Processing stuff.

// this lets the host know how many channels are allowed.
// return 0 to indicate that this is an N-to-N effect.
UInt32 Audio::SupportedNumChannels (const AUChannelInfo** outInfo)
{
/* //this is how you would indicate that this is a stereo-in/stereo-out effect:
    static const AUChannelInfo channelConfigs[] = {{2,2}};
    if(outInfo) *outInfo = channelConfigs;
    return sizeof(channelConfigs)/sizeof(AUChannelInfo); */
    
    // this indicates we are an N-to-N effect.
    return 0;
}

// Reset the state of any reverb tails, etc.
OSStatus Audio::Reset(		AudioUnitScope 				inScope,
                            AudioUnitElement 			inElement)
{
    mTriggered = false;
    mCurrScopeInd = 0;
    return noErr;
};

OSStatus
Audio::ProcessBufferLists (AudioUnitRenderActionFlags& ioActionFlags,
                           const AudioBufferList& inBuffer,
                           AudioBufferList& outBuffer,
                           UInt32 numSamples)
{
    double trigger = GetParameter(kParam_TriggerLevel);
    // bind trigger to 1.0
    trigger = fmin(fmax(trigger, 0.0), 1.0);
    
    // do processing here. we can assume non-interleaved buffers.
    for(int b = 0; b < inBuffer.mNumberBuffers; ++b) {
        for(int s = 0; s < numSamples; ++s) {
            Float32& inSamp = reinterpret_cast<Float32*>(inBuffer.mBuffers[b].mData)[s];
            Float32& outSamp = reinterpret_cast<Float32*>(outBuffer.mBuffers[b].mData)[s];
            // Per-sample processing here
            if(b == 0)
            {
                // scope only works on the first channel.
                if(mTriggered)
                {
                    // write to scope data.
                    mScopeData[mCurrScopeInd++] = inSamp;
                    
                    if(mCurrScopeInd >= 400)
                    {
                        mTriggered = false;
                        PropertyChanged(kProp_ScopeData, kAudioUnitScope_Global, 0);
                    }
                }
                else
                {
                    if(inSamp > trigger)
                    {
                        mTriggered = true;
                        mScopeData[0] = inSamp;
                        mCurrScopeInd = 1;
                    }
                }
            }
            
            outSamp = inSamp;
            //end per-sample processing
        }
    }
    
    // this is a scope so the output is always silent for standalone versions.
    // if we're a plug-in, we pass the audio through unchanged.
#ifdef STANDALONE
    ioActionFlags |= kAudioUnitRenderAction_OutputIsSilence;
#endif
    
    return noErr;
}

OSStatus
Audio::HandleNoteOn(UInt8 	inChannel,
                    UInt8 	inNoteNumber,
                    UInt8 	inVelocity,
                    UInt32  inStartFrame)
{
    // This is where you handle note-on events.
    // it's best not to change parameters directly here - 
    // you should process this only after handling inStartFrame
    // events.
    return noErr;
}

OSStatus
Audio::HandleNoteOff(UInt8 	inChannel,
                     UInt8 	inNoteNumber,
                     UInt8 	inVelocity,
                     UInt32 inStartFrame)
{
    // This is where you handle note-off events.
    // it's best not to change parameters directly here - 
    // you should process this only after handling inStartFrame
    // events.
    return noErr;
}

// Parameter stuff

OSStatus Audio::GetParameterInfo(	AudioUnitScope			inScope,
                                 AudioUnitParameterID	inParameterID,
                                 AudioUnitParameterInfo	&outParameterInfo )
{
 	OSStatus result = noErr;
    
    
	outParameterInfo.flags = 	kAudioUnitParameterFlag_IsWritable
    +		kAudioUnitParameterFlag_IsReadable;
    
	if (inScope == kAudioUnitScope_Global)
    {
		
		switch(inParameterID)
		{
			case kParam_TriggerLevel:
				AUBase::FillInParameterName (outParameterInfo, CFSTR("Trigger"), false);
				outParameterInfo.unit = kAudioUnitParameterUnit_LinearGain;
				outParameterInfo.minValue = 0.0;
				outParameterInfo.maxValue = 1.0;
				outParameterInfo.defaultValue = 0;
				outParameterInfo.flags += kAudioUnitParameterFlag_IsHighResolution;
				break;
                
			default:
				result = kAudioUnitErr_InvalidParameter;
				break;
		}
	} else
    {
		result = kAudioUnitErr_InvalidParameter;
	}
	
	return result;
}

// Property stuff

// this lets javascript know which properties are available.
std::vector<JSPropDesc>
Audio::GetPropertyDescriptionList()
{
    std::vector<JSPropDesc> propList;
    JSPropDesc prop = {JSPropDesc::kJSNumberArray, "ScopeData"};
    propList.push_back(prop);
    return propList;
}

// this gets the size and writability of individual properties

OSStatus Audio::GetPropertyInfo (AudioUnitPropertyID	id,
                                 AudioUnitScope		scope,
                                 AudioUnitElement	elem,
                                 UInt32 &		size,
                                 Boolean &    writable)
{
    if(scope == kAudioUnitScope_Global)
    {
        switch(id)
        {
            case kProp_ScopeData:
                writable = false;
                size = sizeof(mScopeData);
                return noErr;
        }
    }
    return JSAudioUnitBase::GetPropertyInfo(id, scope, elem, size, writable);
    
}

// this actually gets the properties
OSStatus Audio::GetProperty(AudioUnitPropertyID id, AudioUnitScope scope,
                            AudioUnitElement elem, void* data)
{
    if (scope == kAudioUnitScope_Global)
    {
        switch (id)
        {
            case kProp_ScopeData:
                memcpy(data, mScopeData, sizeof(mScopeData));
                return noErr;
                break;
        }
    }
    return JSAudioUnitBase::GetProperty(id, scope, elem, data);
}

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

// This is the class that does the actually processing.
class AudioKernel : public AUKernelBase {
public:
	AudioKernel(Audio* inAudioUnit);
	virtual void Process(Float32 const* inSourceP,
                         Float32 * inDestP,
                         UInt32 inFramesToProcess,
                         UInt32 inNumChannels,
                         bool & ioSilence);
private:
    Audio* mAudio;

};

// This takes care of all parameters and properties
class Audio : public JSAudioUnitBase {
public:
	Audio(AudioUnit component);
	virtual OSStatus Version() { return 0xFFFFFF; }
	virtual AUKernelBase * NewKernel() { return new AudioKernel(this); }
    
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
    
    virtual std::vector<JSPropDesc> GetPropertyDescriptionList();
private:
    double scopeData[400];
    int currScopeInd;
    bool triggered;
    friend class AudioKernel;
};

COMPONENT_ENTRY(Audio)
void DoRegister(OSType Type, OSType Subtype, OSType Manufacturer)
{
    ComponentEntryPoint<Audio>::Register(Type, Subtype, Manufacturer);
}


AudioKernel::AudioKernel(Audio * inAudioUnit ) : AUKernelBase(inAudioUnit), mAudio(inAudioUnit) {}

Audio::Audio(AudioUnit component) : JSAudioUnitBase(component),
    currScopeInd(0),
    triggered(0)
{
    memset(scopeData, 0, sizeof(scopeData));
    SetParameter(kParam_TriggerLevel, 0.0);
}


// Processing stuff.
void AudioKernel::Process(Float32 const* sourceP, Float32 * destP, UInt32 framesToProcess, UInt32 numChannels, bool & ioSilence){
    // This is usually where you want to process data to produce an effect.
    // currently, we're just passing through audio and recording it in the scope.
	
    double trigger = GetParameter(kParam_TriggerLevel);
    
    // bind trigger to 1.0
    trigger = fmin(fmax(trigger, 0.0), 1.0);
    
    for(int i = 0; i < framesToProcess; ++i)
    {
        if(mAudio->triggered)
        {
            // write to scope data.
            mAudio->scopeData[mAudio->currScopeInd++] = *sourceP;
            
            if(mAudio->currScopeInd >= 400)
            {
                mAudio->triggered = false;
                mAudio->PropertyChanged(kProp_ScopeData, kAudioUnitScope_Global, 0);
            }
        }
        else
        {
            if(*sourceP > trigger)
            {
                mAudio->triggered = true;
                mAudio->scopeData[0] = *sourceP;
                mAudio->currScopeInd = 1;
            }
        }
        
        #ifdef STANDALONE
            // don't feed back in standalone.
            *destP = 0;
        #else
            *destP = *sourceP;
        #endif
        destP++;
        sourceP++;
    }
}

// Parameter stuff

// TODO - setup parameter info.
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
                size = sizeof(scopeData);
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
                memcpy(data, scopeData, sizeof(scopeData));
                return noErr;
                break;
        }
    }
    return JSAudioUnitBase::GetProperty(id, scope, elem, data);
}

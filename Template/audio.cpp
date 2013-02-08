#include "jsaubase.h"

// PARAMETERS GO HERE
enum
{
	kParam_VolumeLevel
};

// PROPERTIES GO HERE
// see oscilloscope example for how to use these - this
// simple volume example has none.
enum
{
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
    friend class AudioKernel;
};

// this boilerplate has to be here so that the system can know about the 
// class we just made.
AUDIOCOMPONENT_ENTRY(AUBaseFactory, Audio)
void DoRegister(OSType Type, OSType Subtype, OSType Manufacturer, CFStringRef name, UInt32 vers)
{
    AUBaseFactory<Audio>::Register(Type, Subtype, Manufacturer, name, vers, 0);
}


AudioKernel::AudioKernel(Audio * inAudioUnit ) : AUKernelBase(inAudioUnit), mAudio(inAudioUnit) {}

Audio::Audio(AudioUnit component) : JSAudioUnitBase(component)
{
    SetParameter(kParam_VolumeLevel, 0.5);
}


// Processing stuff.
void AudioKernel::Process(Float32 const* sourceP, Float32 * destP, UInt32 framesToProcess, UInt32 numChannels, bool & ioSilence){
    // This is usually where you want to process data to produce an effect.
    
    // Get the current value of "volume"
    double volume = GetParameter(kParam_VolumeLevel);
    
    // bind volume from 0.0 to 1.0
    volume = fmin(fmax(volume, 0.0), 1.0);
    
    for(int i = 0; i < framesToProcess; ++i)
    {
        *destP = volume * (*sourceP);
        destP++;
        sourceP++;
    }
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
			case kParam_VolumeLevel:
				AUBase::FillInParameterName (outParameterInfo, CFSTR("Volume"), false);
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
    // currently no properties
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
        // you'd fill in property information here.
    }
    return JSAudioUnitBase::GetPropertyInfo(id, scope, elem, size, writable);
}

// this actually gets the properties
OSStatus Audio::GetProperty(AudioUnitPropertyID id, AudioUnitScope scope, 
                              AudioUnitElement elem, void* data)
{
    if (scope == kAudioUnitScope_Global)
    {
        // you'd fill in the property here - see the oscilloscope example 
        // for information on how.
    }
    return JSAudioUnitBase::GetProperty(id, scope, elem, data);
}

#include "audioprops.h"

// PARAMETERS GO HERE
enum
{
	kParam_Gain
};

// PROPERTIES GO HERE
enum
{
    kProp_ScopeData = kFirstAudioProp
};

class AudioKernel : public AUKernelBase {
public:
	AudioKernel(AUEffectBase * inAudioUnit);
	virtual void Process(Float32 const* inSourceP,
                         Float32 * inDestP,
                         UInt32 inFramesToProcess,
                         UInt32 inNumChannels,
                         bool & ioSilence);

};

class Audio : public AUEffectBase {
public:
	Audio(AudioUnit component);
	virtual OSStatus Version() { return 0xFFFFFF; }
	virtual OSStatus Initialize();
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
private:
    double scopeData[400];
    int currScopeInd;
};

COMPONENT_ENTRY(Audio)

AudioKernel::AudioKernel(AUEffectBase * inAudioUnit ) : AUKernelBase(inAudioUnit) {}

Audio::Audio(AudioUnit component) : AUEffectBase(component),
    currScopeInd(0)
{
    memset(scopeData, 0, sizeof(scopeData));
    SetParameter(kParam_Gain, 0.0);
}

OSStatus Audio::Initialize() {
    OSStatus result = AUEffectBase::Initialize();
    if(result == noErr ) {
        // do something
    }
    return result;
}

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
			case kAudioUnitProperty_CocoaUI:
                writable = false;
                size = sizeof(AudioUnitCocoaViewInfo);
                return noErr;
            case kAudioProp_JSPropList:
                writable = false;
                size = sizeof(JSPropDesc);
                return noErr;
            case kProp_ScopeData:
                writable = false;
                size = sizeof(scopeData);
                return noErr;
        }
    }
    return AUEffectBase::GetPropertyInfo(id, scope, elem, size, writable);
    
}


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
			case kParam_Gain:
				AUBase::FillInParameterName (outParameterInfo, CFSTR("Gain"), false);
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

// TODO : write processing code.
void AudioKernel::Process(Float32 const* sourceP, Float32 * destP, UInt32 framesToProcess, UInt32 numChannels, bool & ioSilence){
    //This code will pass-thru the audio data.
    //This is usually where you want to process data to produce an effect.
	double gain = GetParameter(kParam_Gain);
    
    // bind gain to 1.0
    gain = fmin(fmax(gain, 0.0), 1.0);
    
    if(gain == 0)
        ioSilence = true;
    
    for(int i = 0; i < framesToProcess; ++i)
    {
        *destP = gain * (*sourceP);
        destP++;
        sourceP++;
    }
}

OSStatus Audio::GetProperty(AudioUnitPropertyID id, AudioUnitScope scope, 
                              AudioUnitElement elem, void* data)
{
    if (scope == kAudioUnitScope_Global)
    {
        switch (id)
        {
            case kAudioUnitProperty_CocoaUI:
            {

                CFBundleRef bundle= CFBundleGetBundleWithIdentifier(CFSTR("com.#COMPANY.#PROJNAME"));
                
                if(!bundle) return fnfErr;
                
                CFURLRef bundleUrl = CFBundleCopyResourceURL(bundle, CFSTR("CocoaUI"), CFSTR("bundle"), NULL);
                
                if(!bundleUrl) return fnfErr;
                
                AudioUnitCocoaViewInfo info;
                info.mCocoaAUViewBundleLocation = bundleUrl;
                info.mCocoaAUViewClass[0] = CFStringCreateWithCString(0, "#PROJNAME_ViewFactory", 
                                                                     kCFStringEncodingUTF8); 
                
                *(reinterpret_cast<AudioUnitCocoaViewInfo*>(data)) = info;
                return noErr;
            }
            break;
            case kAudioProp_JSPropList:
            {
                JSPropDesc desc;
                desc.type = JSPropDesc::kJSNumberArray;
                memcpy(desc.name, "ScopeData", sizeof(desc.name));
                *(reinterpret_cast<JSPropDesc*>(data)) = desc;
                return noErr;
            }
            break;
            case kProp_ScopeData:
                memcpy(data, scopeData, sizeof(scopeData));
                return noErr;
                break;
        }
    }
    return AUEffectBase::GetProperty(id, scope, elem, data);
}

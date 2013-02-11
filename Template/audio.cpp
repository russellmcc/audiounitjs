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
    
    virtual OSStatus	Reset(		AudioUnitScope 				inScope,
                                    AudioUnitElement 			inElement);
    
    virtual OSStatus    ProcessBufferLists (AudioUnitRenderActionFlags& ioActionFlags,
                           const AudioBufferList& inBuffer,
                           AudioBufferList& outBuffer,
                           UInt32 numSamples);
    
    virtual std::vector<JSPropDesc> GetPropertyDescriptionList();
};

// this boilerplate has to be here so that the system can know about the 
// class we just made.
AUDIOCOMPONENT_ENTRY(AUBaseFactory, Audio)
void DoRegister(OSType Type, OSType Subtype, OSType Manufacturer, CFStringRef name, UInt32 vers)
{
    AUBaseFactory<Audio>::Register(Type, Subtype, Manufacturer, name, vers, 0);
}

Audio::Audio(AudioUnit component) : JSAudioUnitBase(component)
{
    SetParameter(kParam_VolumeLevel, 0.5);
}


// Processing stuff.

// Reset the state of any reverb tails, etc.
OSStatus Audio::Reset(		AudioUnitScope 				inScope,
                            AudioUnitElement 			inElement)
{
    // this #PROJNAME-changer doesn't store any state, so there's nothing
    // to reset.
    return noErr;
};

OSStatus
Audio::ProcessBufferLists (AudioUnitRenderActionFlags& ioActionFlags,
                           const AudioBufferList& inBuffer,
                           AudioBufferList& outBuffer,
                           UInt32 numSamples)
{
    // watch out!  buffers could be interleaved or not interleaved!
	SInt16 channels = GetOutput(0)->GetStreamFormat().mChannelsPerFrame;
    
    // check for silence
    bool silentInput = IsInputSilent (ioActionFlags, numSamples);
    if(silentInput) {
        ioActionFlags |= kAudioUnitRenderAction_OutputIsSilence;
        return noErr;
    }
    
    // Gather per-buffer parameters here
    double v = GetParameter(kParam_VolumeLevel);
    // bind v to 1.0
    v = fmin(fmax(v, 0.0), 1.0);
    
    // do processing here.
    for(int b = 0; b < inBuffer.mNumberBuffers; ++b) {
        UInt32 numChans = inBuffer.mBuffers[b].mNumberChannels;
        for(int c = 0; c < numChans; ++c) {
            UInt32 chan = (b*numChans) + c;
            for(int s = 0; s < numSamples; ++s) {
                // Per-sample processing here
                Float32& inSamp = reinterpret_cast<Float32*>(inBuffer.mBuffers[b].mData)[s*numChans + c];
                Float32& outSamp = reinterpret_cast<Float32*>(outBuffer.mBuffers[b].mData)[s*numChans + c];
                
                outSamp = inSamp * v;
            
                outSamp = inSamp;
                //end per-sample processing
            }
        }
    }
    
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

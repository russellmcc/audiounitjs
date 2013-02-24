#include "jsaubase.h"
#include <cmath>
#include <set>

using namespace std;

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
    
    virtual vector<JSPropDesc> GetPropertyDescriptionList();
private:
    set<uint8_t> mActiveKeys;
    vector<pair<uint32_t, pair<bool, uint8_t> > > mNoteEventsToHandle;
    double mPhase;
    double mPhaseIncr;
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
    SetParameter(kParam_VolumeLevel, 0.5);
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
    mActiveKeys.clear();
    mPhase = 0;
    mPhaseIncr = 0;
    mNoteEventsToHandle.clear();
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
    
    auto noteIt = mNoteEventsToHandle.begin();
    
    // do processing here.
    for(int s = 0; s < numSamples; ++s) {
        // handle any note messages!
        while(noteIt != mNoteEventsToHandle.end() and
           noteIt->first <= s) {
            if(noteIt->second.first)
                mActiveKeys.insert(noteIt->second.second);
            else
                mActiveKeys.erase(noteIt->second.second);

            if(mActiveKeys.size()){
                auto topNote = mActiveKeys.end(); --topNote;
                // equal temperament equation.
                double freq = pow(2, (static_cast<double>(*topNote) - 69) / 12) * 440;
                mPhaseIncr = freq / GetSampleRate();
            } else {
                mPhaseIncr = 0;
            }
            ++noteIt;
        }
        
        mPhase += mPhaseIncr;
        if(mPhase > 1)
            mPhase -= 1;
        for(int b = 0; b < inBuffer.mNumberBuffers; ++b) {
            UInt32 numChans = inBuffer.mBuffers[b].mNumberChannels;
            for(int c = 0; c < numChans; ++c) {
                UInt32 chan = (b*numChans) + c;
                
                // Per-sample processing here
                Float32& inSamp = reinterpret_cast<Float32*>(inBuffer.mBuffers[b].mData)[s*numChans + c];
                Float32& outSamp = reinterpret_cast<Float32*>(outBuffer.mBuffers[b].mData)[s*numChans + c];
                
                if(mPhaseIncr > 0)
                    outSamp = sin(mPhase * 2 * 3.145159) * v;
                else
                    outSamp = 0;
                //end per-sample processing
            }
        }
    }
    mNoteEventsToHandle.clear();
    return noErr;
}

OSStatus
Audio::HandleNoteOn(UInt8 	inChannel,
                    UInt8 	inNoteNumber,
                    UInt8 	inVelocity,
                    UInt32  inStartFrame)
{
    mNoteEventsToHandle.push_back(make_pair(inStartFrame, make_pair(inVelocity, inNoteNumber)));
    return noErr;
}

OSStatus
Audio::HandleNoteOff(UInt8 	inChannel,
                     UInt8 	inNoteNumber,
                     UInt8 	inVelocity,
                     UInt32 inStartFrame)
{
    mNoteEventsToHandle.push_back(make_pair(inStartFrame, make_pair(false, inNoteNumber)));
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
vector<JSPropDesc> 
Audio::GetPropertyDescriptionList()
{
    vector<JSPropDesc> propList;
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

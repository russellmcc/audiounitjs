
#ifndef example_jsaubase_h
#define example_jsaubase_h

#include "audioprops.h"

// base class that eliminates some boilerplate for javascript-based AUs
class JSAudioUnitBase : public AUEffectBase
{
    public:
        JSAudioUnitBase(AudioUnit unit) : AUEffectBase(unit) {}
        ~JSAudioUnitBase() {}
        
        virtual OSStatus GetProperty(AudioUnitPropertyID id, AudioUnitScope scope, 
                                     AudioUnitElement elem, void* data);
        virtual OSStatus GetPropertyInfo (AudioUnitPropertyID	id,
                                          AudioUnitScope		scope,
                                          AudioUnitElement	elem,
                                          UInt32 &		size,
                                          Boolean &    writable);
    protected:
        // override this provide properties accessible in javascript.
        virtual std::vector<JSPropDesc> GetPropertyDescriptionList() { return std::vector<JSPropDesc>(); }
};

void DoRegister(OSType Type, OSType Subtype, OSType Manufacturer, CFStringRef name, UInt32 vers);

#endif

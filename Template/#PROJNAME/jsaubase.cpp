#include "jsaubase.h"

OSStatus JSAudioUnitBase::GetPropertyInfo (AudioUnitPropertyID	id,
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
                size = sizeof(JSPropDesc) * GetPropertyDescriptionList().size();
                return noErr;
        }
    }
    return AUEffectBase::GetPropertyInfo(id, scope, elem, size, writable);
}



OSStatus JSAudioUnitBase::GetProperty(AudioUnitPropertyID id, AudioUnitScope scope, 
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
                std::vector<JSPropDesc> descs = GetPropertyDescriptionList();
                memcpy(data, &descs[0], sizeof(JSPropDesc) * descs.size());
                return noErr;
            }
            break;
        }
    }
    return AUEffectBase::GetProperty(id, scope, elem, data);
}

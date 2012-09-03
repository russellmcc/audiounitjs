//
//  AUWrapper.mm
//  #PROJNAME
//
//  Created by James McClellan on 8/23/12.
//
/*
    Why is this file so complex?
    
    We're using safari's WebScriptObject API to add javacript
    interaction with our audiounits.  This is actually more difficult than it sounds,
    because we're querying the audio unit for all of the information about its
    parameters and properties to generate the javascript hooks.  
    The safari WebScriptObject API clearly wasn't designed for this, as it uses
    objective-C classes exposed as javascript classes.  You can't add data members or 
    methods to indivdual objects.
    
    So, we use runtime functions to create a new class description for our AudioUnit
    item, and add getters for each sub-object.
 */
#include <vector>
#include <string>
#include <utility>
#include "AUWrapper.h"
#include "audioprops.h"
#include "AUPropParamBase.h"
#include "AUProp.h"
#include "AUParam.h"

#include <cmath>
#include "LowLevelCocoaUtils.h"

using namespace LowLevelCocoaUtils;

#define COUNT_OF(x) ((sizeof(x)/sizeof(0[x])) / ((size_t)(!(sizeof(x) % sizeof(0[x])))))

using namespace std;

namespace
{
    Class AllocateAndInitWebScriptClass(const char* name)
    {
        // qualify the name with the prefix.
        string qualName = string("#PROJNAME_") + string(name);
        
        // create the basic class pair.
        Class c = objc_allocateClassPair([NSObject class], qualName.c_str(), 0);
        if(not c) throwErr();
        
        CopyMethodFromBase(c, [#PROJNAME_AUPropParamBase class], @selector(isSelectorExcludedFromWebScript:), "c@::");
        CopyMethodFromBase(c, [#PROJNAME_AUPropParamBase class], @selector(isKeyExcludedFromWebScript:), "c@:*");
        
        return c;
    }
    
    // follows the "create rule" - that id, the objective-C objects returned are owned by the caller.
    typedef vector<pair<string, id> > NamedObjectList;
    NamedObjectList
    CreatePropertyObjectsForAU(AudioUnit au)
    {
        NamedObjectList properties;
        UInt32 dataSize = 0;
        OSStatus status = AudioUnitGetPropertyInfo(au, kAudioProp_JSPropList, kAudioUnitScope_Global, 0, &dataSize, 0);
        if(status != noErr) return properties;
        
        UInt32 numElems = dataSize / sizeof(JSPropDesc);
        
        JSPropDesc propInfoData[numElems];
        status = AudioUnitGetProperty(au, kAudioProp_JSPropList, kAudioUnitScope_Global, 0, propInfoData, &dataSize);
        if(status != noErr) return properties;
        
        for(int i = 0; i < numElems; ++i)
        {
            JSPropDesc& prop = propInfoData[i];
            
            // set-up class.
            properties.push_back(make_pair<string, id>(prop.name, [[#PROJNAME_AUProp alloc] initWithAU:au withID:i withType:prop.type]));
        }
        
        return properties;
    }
    
    NamedObjectList
    CreateParamObjectsForAU(AudioUnit au)
    {
        NamedObjectList params;
        
        // okay, we need to get a list of all the parameters of the AU.
        UInt32 dataSize = 0;
        OSStatus status = AudioUnitGetPropertyInfo(au, kAudioUnitProperty_ParameterList, kAudioUnitScope_Global, 0, &dataSize, 0);
        if(status != noErr) return params;
            
        unsigned int numElems = dataSize / sizeof(AudioUnitParameterID);
        AudioUnitParameterID data[numElems];
        status = AudioUnitGetProperty(au, kAudioUnitProperty_ParameterList, kAudioUnitScope_Global, 0, reinterpret_cast<void*>(data), &dataSize);
        
        for(int i = 0; i < numElems; ++i)
        {
            AudioUnitParameterID paramID = data[i];
            
            // now, we want to create an info structure.
            AudioUnitParameterInfo info;
            UInt32 infoSize = sizeof(info);
            status = AudioUnitGetProperty(au, kAudioUnitProperty_ParameterInfo, kAudioUnitScope_Global, paramID, reinterpret_cast<void*>(&info), &infoSize);
            if(status != noErr) continue;
            params.push_back(make_pair<string, id>(info.name, [[#PROJNAME_AUParam alloc] initWithAU:au withID:paramID]));
        }
        return params;
    }
    
    void AddObjectsToClass(Class c, const NamedObjectList& list)
    {
        for(NamedObjectList::const_iterator i = list.begin(); i != list.end(); ++i)
        {
            AddIvar<id>(c, i->first.c_str());
            AddGetter<id>(c, i->first.c_str());
        }
    }
    
    void AssignObjects(id o, const NamedObjectList& list)
    {
        for(NamedObjectList::const_iterator i = list.begin(); i != list.end(); ++i)
            doSet<id>(o, i->first.c_str(), i->second);
    }
    
    NamedObjectList GetAllPropsAndParams(AudioUnit au)
    {
        NamedObjectList props = CreatePropertyObjectsForAU(au);
        NamedObjectList params = CreateParamObjectsForAU(au);
        props.insert(props.end(), params.begin(), params.end());
        return props;
    }
}

// follows the "Create rule"
id CreateAudioUnitObject(AudioUnit au)
{
    NamedObjectList propsAndParams = GetAllPropsAndParams(au);
    Class auClass = AllocateAndInitWebScriptClass("AudioUnit");
    AddObjectsToClass(auClass, propsAndParams);
    objc_registerClassPair(auClass);
    id auObj = [[auClass alloc] init];
    AssignObjects(auObj, propsAndParams);
    return auObj;
}

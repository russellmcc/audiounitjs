// Copyright (c) 2011 The Chromium Embedded Framework Authors. All rights
// reserved. Use of this source code is governed by a BSD-style license that
// can be found in the LICENSE file.

#include "cef.h"
#include "client_handler.h"
#include "CppV8Handler.h"
#include "audioprops.h"

ClientHandler::ClientHandler(AudioUnit au) :
    mAU(au)
{
    printf("CREATED!");
}

ClientHandler::~ClientHandler()
{
}

#include <assert.h>

#ifndef NDEBUG
#define ASSERT(condition) if(!(condition)) { assert(false); }
#else
#define ASSERT(condition) ((void)0)
#endif

#define REQUIRE_UI_THREAD()   ASSERT(CefCurrentlyOn(TID_UI));

void ClientHandler::OnAfterCreated(CefRefPtr<CefBrowser> browser)
{
    REQUIRE_UI_THREAD();
    
    printf("AFTER CREATED!");
    AutoLock lock_scope(this);
    if(!mBrowser.get())
    {
        // We need to keep the main child window, but not popup windows
        mBrowser = browser;
    }
}

bool ClientHandler::DoClose(CefRefPtr<CefBrowser> browser)
{
    REQUIRE_UI_THREAD();
    
    // A popup browser window is not contained in another window, so we can let
    // these windows close by themselves.
    return false;
}

void ClientHandler::OnBeforeClose(CefRefPtr<CefBrowser> browser)
{
    REQUIRE_UI_THREAD();
}

void ClientHandler::OnLoadStart(CefRefPtr<CefBrowser> browser,
                                CefRefPtr<CefFrame> frame)
{
    REQUIRE_UI_THREAD();
}

void ClientHandler::OnLoadEnd(CefRefPtr<CefBrowser> browser,
                              CefRefPtr<CefFrame> frame,
                              int httpStatusCode)
{
    REQUIRE_UI_THREAD();

}

bool ClientHandler::OnLoadError(CefRefPtr<CefBrowser> browser,
                                CefRefPtr<CefFrame> frame,
                                ErrorCode errorCode,
                                const CefString& failedUrl,
                                CefString& errorText)
{
    REQUIRE_UI_THREAD();

    
    return false;
}

void ClientHandler::OnNavStateChange(CefRefPtr<CefBrowser> browser,
                                     bool canGoBack,
                                     bool canGoForward)
{
    REQUIRE_UI_THREAD();
}

bool ClientHandler::OnConsoleMessage(CefRefPtr<CefBrowser> browser,
                                     const CefString& message,
                                     const CefString& source,
                                     int line)
{
    REQUIRE_UI_THREAD();
    
    return false;
}

void ClientHandler::OnFocusedNodeChanged(CefRefPtr<CefBrowser> browser,
                                         CefRefPtr<CefFrame> frame,
                                         CefRefPtr<CefDOMNode> node)
{
    REQUIRE_UI_THREAD();
    
}

bool ClientHandler::OnKeyEvent(CefRefPtr<CefBrowser> browser,
                               KeyEventType type,
                               int code,
                               int modifiers,
                               bool isSystemKey,
                               bool isAfterJavaScript)
{
    REQUIRE_UI_THREAD();
    
    return false;
}

bool ClientHandler::GetPrintHeaderFooter(CefRefPtr<CefBrowser> browser,
                                         CefRefPtr<CefFrame> frame,
                                         const CefPrintInfo& printInfo,
                                         const CefString& url,
                                         const CefString& title,
                                         int currentPage,
                                         int maxPages,
                                         CefString& topLeft,
                                         CefString& topCenter,
                                         CefString& topRight,
                                         CefString& bottomLeft,
                                         CefString& bottomCenter,
                                         CefString& bottomRight)
{
    REQUIRE_UI_THREAD();

    return false;
}

namespace
{
    void DispatchChangeEvent(void *refCon, void *object, const AudioUnitEvent *event, UInt64 hostTime, Float32 value)
    {
        ClientHandler* self = reinterpret_cast<ClientHandler*>(refCon);
        self->ReceiveAUEvent(object, event, hostTime, value);
    }
    
    void addParamListener (AUEventListenerRef listener, void* refCon, AudioUnitEvent *inEvent)
    {
        inEvent->mEventType = kAudioUnitEvent_BeginParameterChangeGesture;
        AUEventListenerAddEventType(	listener, refCon, inEvent);
        
        inEvent->mEventType = kAudioUnitEvent_EndParameterChangeGesture;
        AUEventListenerAddEventType(	listener, refCon, inEvent);
        
        inEvent->mEventType = kAudioUnitEvent_ParameterValueChange;
        AUEventListenerAddEventType(	listener, refCon, inEvent);	
    } 
}

void ClientHandler::ReceiveAUEvent(void *object, const AudioUnitEvent *event, UInt64 hostTime, Float32 value)
{
    // don't do anything if we're in a bad state.
    if(!mV8Context.get())
        return;

    WithV8Context withContext(mV8Context);
    
    CefRefPtr<CefV8Exception> exception;
    CefRefPtr<CefV8Value> retVal;
    CefV8ValueList args; 
    
    // set-up arguments.
    CefString callbackStr;
    
    bool paramEvent = false;
    
    AudioUnitParameterID currId = event->mArgument.mParameter.mParameterID;
    switch (event->mEventType)
    {
        case kAudioUnitEvent_ParameterValueChange:
            callbackStr = "OnParameterChange";
            paramEvent = true;
            args.push_back(CefV8Value::CreateInt(currId));
            args.push_back(CefV8Value::CreateDouble(value));
            break;
        case kAudioUnitEvent_BeginParameterChangeGesture:
            callbackStr = "OnBeginParameterGesture";
            paramEvent = true;
            args.push_back(CefV8Value::CreateInt(currId));
            args.push_back(CefV8Value::CreateDouble(value));
            break;
        case kAudioUnitEvent_EndParameterChangeGesture:
            callbackStr = "OnEndParameterGesture";
            paramEvent = true;
            args.push_back(CefV8Value::CreateInt(currId));
            args.push_back(CefV8Value::CreateDouble(value));
            break;
        case kAudioUnitEvent_PropertyChange:
            callbackStr = "OnPropertyChange";
            paramEvent = false;
            args.push_back(CefV8Value::CreateInt(currId));
            args.push_back(CefV8Value::CreateDouble(value));
            break;
        default:
            return;
    }
    
    if(mV8Value->HasValue(callbackStr))
        mV8Value->GetValue(callbackStr)->ExecuteFunction(mV8Value, args, retVal, exception, true);
        
    if(paramEvent)
    {
        // now, check if any parameter has a parameter-specific callback.
        CefRefPtr<CefV8Value> params = mV8Value->GetValue("Params");
        if(not params)
            return;
            
        std::vector<CefString> keys;
        if(not params->GetKeys(keys))
            return;
        
        for(std::vector<CefString>::iterator i = keys.begin(); i != keys.end(); ++i)
        {
            CefRefPtr<CefV8Value> curr = params->GetValue(*i);
            if(not curr)
                return;
                
            if(curr->GetValue("id")->GetIntValue() != currId)
                continue;
                
            if(curr->HasValue(callbackStr))
            {
                CefV8ValueList args;
                args.push_back(CefV8Value::CreateDouble(value));
                curr->GetValue(callbackStr)->ExecuteFunction(curr, args, retVal, exception, true);
                return;
            }
        }
    }
    else
    {
        // ditto for the properties
        CefRefPtr<CefV8Value> props = mV8Value->GetValue("Properties");
        if(not props)
            return;
            
        std::vector<CefString> keys;
        if(not props->GetKeys(keys))
            return;
        
        for(std::vector<CefString>::iterator i = keys.begin(); i != keys.end(); ++i)
        {
            CefRefPtr<CefV8Value> curr = props->GetValue(*i);
            if(not curr)
                return;
                
            if(curr->GetValue("id")->GetIntValue() + kFirstAudioProp != currId )
                continue;
                
            if(curr->HasValue(callbackStr))
            {
                CefV8ValueList args;
                args.push_back(CefV8Value::CreateDouble(value));
                curr->GetValue(callbackStr)->ExecuteFunction(curr, args, retVal, exception, true);
                return;
            }
        }
    }
}    

// This will create the au param info list and setup the listeners for each param
// mV8Value and mAUEventListener must be initialized, mV8Context must be open.
void ClientHandler::SetupAUParams()
{
    // okay, we need to get a list of all the parameters of the AU.
    UInt32 dataSize = 0;
    OSStatus status = AudioUnitGetPropertyInfo(mAU, kAudioUnitProperty_ParameterList, kAudioUnitScope_Global, 0, &dataSize, 0);
    if(status != noErr) return;
        
    unsigned int numElems = dataSize / sizeof(AudioUnitParameterID);
    AudioUnitParameterID data[numElems];
    status = AudioUnitGetProperty(mAU, kAudioUnitProperty_ParameterList, kAudioUnitScope_Global, 0, reinterpret_cast<void*>(data), &dataSize);
    
    if(status !=noErr) return;
    
    numElems = dataSize / sizeof(AudioUnitParameterID);
    
    // paramInfos is a name-indexed map of info for the parameters.
    CefRefPtr<CefV8Value> paramInfos = CefV8Value::CreateObject(NULL);
    mV8Value->SetValue("Params", paramInfos, V8_PROPERTY_ATTRIBUTE_NONE);
    
    for(int i = 0; i < numElems; ++i)
    {
        AudioUnitParameterID paramID = data[i];
    
        // listen to all events from this parameter..
        AudioUnitEvent auEvent;
        AudioUnitParameter parameter = {mAU, paramID, kAudioUnitScope_Global, 0 };
        auEvent.mArgument.mParameter = parameter;		

        addParamListener (mAUEventListener, this, &auEvent);
        
        // now, we want to create an info structure.
        AudioUnitParameterInfo info;
        UInt32 infoSize = sizeof(info);
        status = AudioUnitGetProperty(mAU, kAudioUnitProperty_ParameterInfo, kAudioUnitScope_Global, paramID, reinterpret_cast<void*>(&info), &infoSize);
        if(status != noErr) continue;
        
        CefRefPtr<CefV8Value> paramInfo = CefV8Value::CreateObject(NULL);
        paramInfos->SetValue(info.name, paramInfo, V8_PROPERTY_ATTRIBUTE_NONE);
        paramInfo->SetValue("id", CefV8Value::CreateInt(paramID), V8_PROPERTY_ATTRIBUTE_READONLY);
        
        // okay, make the get and set functions
        void (^MakePassThru)(const char*, const char*) = ^(const char* myName, const char* otherName){ 
            paramInfo->SetValue(myName, CefV8Value::CreateFunction(myName, new CppV8Handler(myName, ^
                (
                CefRefPtr<CefV8Value> obj,
                const CefV8ValueList& args,
                CefRefPtr<CefV8Value>& ret,
                CefString& except) {
                  CefV8ValueList allArgs;
                  // first argument is my id.
                  allArgs.push_back(CefV8Value::CreateInt(paramID));
                  
                  // copy all args to this after.
                  allArgs.insert(allArgs.end(), args.begin(), args.end());
                  
                  CefRefPtr<CefV8Exception> exception;
                  
                  // just call the "SetParameter" function on mV8Value.
                  mV8Value->GetValue(otherName)->ExecuteFunction(mV8Value, allArgs, ret, exception, true);
                  return true;
              }
            )), V8_PROPERTY_ATTRIBUTE_NONE);            
        };
       
        MakePassThru("Set", "SetParameter");
        MakePassThru("Get", "GetParameter");
        MakePassThru("BeginGesture", "BeginParameterChangeGesture");
        MakePassThru("EndGesture", "EndParameterChangeGesture");
        
        // for convenience, put this param right on the AudioUnit object.
        mV8Value->SetValue(info.name, paramInfo, V8_PROPERTY_ATTRIBUTE_NONE);
    }
    
    // now, do similar stuff for the JS properties.
    status = AudioUnitGetPropertyInfo(mAU, kAudioProp_JSPropList, kAudioUnitScope_Global, 0, &dataSize, 0);
    if(status != noErr) return;
    
    numElems = dataSize / sizeof(JSPropDesc);
    JSPropDesc propInfoData[numElems];
    status = AudioUnitGetProperty(mAU, kAudioProp_JSPropList, kAudioUnitScope_Global, 0, propInfoData, &dataSize);
    if(status != noErr) return;
        
    // propInfos is a name-indexed map of info for the parameters.
    CefRefPtr<CefV8Value> propInfos = CefV8Value::CreateObject(NULL);
    mV8Value->SetValue("Properties", propInfos, V8_PROPERTY_ATTRIBUTE_NONE);
    
        
    for(int i = 0; i < numElems; ++i)
    {
        // calculate the au-side id.
        int auPropID = i + kFirstAudioProp;
        
        // register for "property changed" callbacks.
        AudioUnitEvent auEvent;
        AudioUnitProperty auProp = {mAU, auPropID, kAudioUnitScope_Global, 0};
        auEvent.mArgument.mProperty = auProp;
        auEvent.mEventType = kAudioUnitEvent_PropertyChange;
        AUEventListenerAddEventType(mAUEventListener, this, &auEvent);
        
        // remember the type of this ID.
        mJSPropTypes.push_back(propInfoData[i].type);
        
        // create an info structure
        CefRefPtr<CefV8Value> propInfo = CefV8Value::CreateObject(NULL);
        propInfos->SetValue(propInfoData[i].name, propInfo, V8_PROPERTY_ATTRIBUTE_NONE);
        propInfo->SetValue("id", CefV8Value::CreateInt(i), V8_PROPERTY_ATTRIBUTE_READONLY);
        
        // make a "get" pass-through
        propInfo->SetValue("Get", CefV8Value::CreateFunction("Get", new CppV8Handler("Get", ^
                (
                CefRefPtr<CefV8Value> obj,
                const CefV8ValueList& args,
                CefRefPtr<CefV8Value>& ret,
                CefString& except) {
                  CefV8ValueList allArgs;
                  // first argument is my id.
                  allArgs.push_back(CefV8Value::CreateInt(i));
                  
                  CefRefPtr<CefV8Exception> exception;
                  
                  mV8Value->GetValue("GetProperty")->ExecuteFunction(mV8Value, allArgs, ret, exception, true);
                  return true;
              }
            )), V8_PROPERTY_ATTRIBUTE_NONE);   
        
        // for convenience, put this param right on the AudioUnit object.
        mV8Value->SetValue(propInfoData[i].name, propInfo, V8_PROPERTY_ATTRIBUTE_NONE);
    }
}

void ClientHandler::OnContextCreated(CefRefPtr<CefBrowser> browser,
                                     CefRefPtr<CefFrame> frame,
                                     CefRefPtr<CefV8Context> context)
{
    REQUIRE_UI_THREAD();
    
    // mark the context, create the audiounit base for the API.
    mV8Context = context;
    
    // This is the "AudioUnit" value.
    mV8Value = CefV8Value::CreateObject(NULL);    
    
    AUEventListenerCreate(DispatchChangeEvent, this,
                          CFRunLoopGetCurrent(), kCFRunLoopDefaultMode, 0.05, 0.05, 
                          &mAUEventListener);
    
    SetupAUParams();

    void (^BindSendNotification)(const char*, AudioUnitEventType) = ^(const char* func, AudioUnitEventType type) {
        mV8Value->SetValue(func, CefV8Value::CreateFunction(func, new CppV8Handler(func, ^
        (
        CefRefPtr<CefV8Value> obj,
        const CefV8ValueList& args,
        CefRefPtr<CefV8Value>& ret,
        CefString& except) {
            if(args.size() < 1)
            {
                except = "Incorrect number of arguments: 1 expected.";
                return true;
            }
            int paramID = args[0]->GetIntValue();
            
            AudioUnitEvent event;
            AudioUnitParameter param = {mAU, paramID, kAudioUnitScope_Global, 0};
            event.mArgument.mParameter = param;
            event.mEventType = type;
            
            AUEventListenerNotify(mAUEventListener, 0, &event);
            return true;
        })), V8_PROPERTY_ATTRIBUTE_NONE);
    };
    
    BindSendNotification("BeginParameterChangeGesture", kAudioUnitEvent_BeginParameterChangeGesture);
    BindSendNotification("EndParameterChangeGesture", kAudioUnitEvent_EndParameterChangeGesture);
    
    
    mV8Value->SetValue("SetParameter", CefV8Value::CreateFunction("SetParameter", new CppV8Handler("SetParameter", ^
        (
        CefRefPtr<CefV8Value> obj,
        const CefV8ValueList& args,
        CefRefPtr<CefV8Value>& ret,
        CefString& except) {
        
        // first argument is the parameter to set, second argument is the value.
        if(args.size() < 2)
        {
            except = "Incorrect number of arguments to SetParameter: 2 expected.";
            return true;
        }
        
        int paramID = args[0]->GetIntValue();
        double paramVal = args[1]->GetDoubleValue();
        // change the param to .5
        AudioUnitParameter param = {mAU, paramID, kAudioUnitScope_Global, 0 };
	
        AUParameterSet(mAUEventListener, 0, &param, (Float32)paramVal, 0);
        return true; 
    }
    )), V8_PROPERTY_ATTRIBUTE_NONE);
    mV8Value->SetValue("GetParameter", CefV8Value::CreateFunction("GetParameter", new CppV8Handler("GetParameter", ^
         (
        CefRefPtr<CefV8Value> obj,
        const CefV8ValueList& args,
        CefRefPtr<CefV8Value>& ret,
        CefString& except) {
        // first argument is the parameter to set
        if(args.size() < 1)
        {
            except = "Incorrect number of arguments to GetParameter: 1 expected.";
            return true;
        }
        int paramID = args[0]->GetIntValue();
        AudioUnitParameterValue val = 0;
        AudioUnitGetParameter(mAU, paramID, kAudioUnitScope_Global, 0, &val);
        ret = CefV8Value::CreateDouble(val);
        return true;
    }
    )), V8_PROPERTY_ATTRIBUTE_NONE);                                                                                                                                            
    mV8Value->SetValue("GetProperty", CefV8Value::CreateFunction("GetProperty", new CppV8Handler("GetProperty", ^
         (
        CefRefPtr<CefV8Value> obj,
        const CefV8ValueList& args,
        CefRefPtr<CefV8Value>& ret,
        CefString& except) {
        // first argument is the parameter to set
        if(args.size() < 1)
        {
            except = "Incorrect number of arguments to GetParameter: 1 expected.";
            return true;
        }
        
        int propID = args[0]->GetIntValue();
        
        // if it
        if((propID < 0) or (propID > mJSPropTypes.size()))
        {
            except = "Unrecognized property ID";
            return true;
        }
        
        int auPropID = propID + kFirstAudioProp; // the AU has the ids shifted by kFirstAudioProp

        UInt32 propSize;
        OSStatus status = AudioUnitGetPropertyInfo(mAU, auPropID, kAudioUnitScope_Global, 0, &propSize, 0);
        if(status != noErr)
        {
            except = "Property not found";
            return true;
        }
        
        // what we return depends on what type of value it is.
        switch(mJSPropTypes[propID])
        {
            case JSPropDesc::kJSNumber: {
                if(propSize != sizeof(double))
                {
                    except = "property size mismatch";
                    return true;
                }
                double retAsDouble;
                AudioUnitGetProperty(mAU, auPropID, kAudioUnitScope_Global, 0, &retAsDouble, &propSize);
                ret = CefV8Value::CreateDouble(retAsDouble);
            } break;
            case JSPropDesc::kJSString: {
                char propString[propSize];
                AudioUnitGetProperty(mAU, auPropID, kAudioUnitScope_Global, 0, propString, &propSize);
                ret = CefV8Value::CreateString(propString);
            } break;
            case JSPropDesc::kJSNumberArray: {
                int numDoubles = propSize / sizeof(double);
                double retAsDoubles[numDoubles + 1];
                AudioUnitGetProperty(mAU, auPropID, kAudioUnitScope_Global, 0, retAsDoubles, &propSize);
                ret = CefV8Value::CreateArray();
                for(int i = 0; i < numDoubles; ++i)
                    ret->SetValue(i, CefV8Value::CreateDouble(retAsDoubles[i]));
            } break;
        }

        return true;
    }
    )), V8_PROPERTY_ATTRIBUTE_NONE);
    
    context->GetGlobal()->SetValue("AudioUnit", mV8Value, V8_PROPERTY_ATTRIBUTE_NONE);
    
}

    
void ClientHandler::OnContextReleased(CefRefPtr<CefBrowser> browser,
                                 CefRefPtr<CefFrame> frame,
                                 CefRefPtr<CefV8Context> context)
{
    CefRefPtr<CefV8Context> foo;
    mV8Context.swap(foo);
}
    

bool ClientHandler::OnDragStart(CefRefPtr<CefBrowser> browser,
                                CefRefPtr<CefDragData> dragData,
                                DragOperationsMask mask)
{
    REQUIRE_UI_THREAD();
    
    // Forbid dragging of image files.
    if (dragData->IsFile()) {
        std::string fileExt = dragData->GetFileExtension();
        if (fileExt == ".png" || fileExt == ".jpg" || fileExt == ".gif")
            return true;
    }
    
    return false;
}

bool ClientHandler::OnDragEnter(CefRefPtr<CefBrowser> browser,
                                CefRefPtr<CefDragData> dragData,
                                DragOperationsMask mask)
{
    REQUIRE_UI_THREAD();
    
    // Forbid dragging of link URLs.
    if (dragData->IsLink())
        return true;
    
    return false;
}

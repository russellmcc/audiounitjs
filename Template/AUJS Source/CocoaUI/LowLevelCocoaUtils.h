//
//  LowLevelCocoaUtils.h
//  #PROJNAME
//
//  Created by James McClellan on 8/25/12.
//

#ifndef #PROJNAME_LowLevelCocoaUtils_h
#define #PROJNAME_LowLevelCocoaUtils_h

#import <objc/runtime.h>
#import <objc/message.h>
#include <string>

namespace LowLevelCocoaUtils
{
    using namespace std;
    
    inline void throwErr() { throw static_cast<OSStatus>(-9); }
    
    template<class T>
    void doSet (id self, const char* name, const T& t)
    {
        Ivar ivar = object_getInstanceVariable(self, name, 0);
        if(not ivar) throwErr();
        *reinterpret_cast<T*>(reinterpret_cast<char*>(self) + ivar_getOffset(ivar)) = t;    
    }
    
    template<class T>
    void setImp(id self, SEL _cmd, T toSet)
    {
        // we take advantage of the fact that the selector should be named
        // "set$:", where "$" is the name of the variable.
        // there's no regex support in 10.6 in c++ so we have to do this the hard way :'(.
        NSString* s = NSStringFromSelector(_cmd);
        string selector([s UTF8String]);
        if(selector.compare(0, 3, "set")) throwErr();
        if(selector.compare(selector.length() - 1, 1, ":")) throwErr();
        string ivarName = selector.substr(3, selector.length() - 4);
        
        doSet<T>(self, ivarName.c_str(), toSet);
    }
    
    template<class T>
    T doGet (id self, const char* name)
    {
        Ivar ivar = object_getInstanceVariable(self, name, 0);
        if(not ivar) throwErr();
        return *reinterpret_cast<T*>(reinterpret_cast<char*>(self) + ivar_getOffset(ivar));
    }
    
    template<class T>
    T getImp(id self, SEL _cmd)
    {
        // just get the ivar with the same name as this selector.
        NSString* s = NSStringFromSelector(_cmd);
        return doGet<T>(self, [s UTF8String]);
    }
    
    template <class T>
    void AddIvar(Class& c, const char* name)
    {
        BOOL success = class_addIvar(c, name, sizeof(T), log2(sizeof(T)), @encode(T));
        if(not success) throwErr();
    }
    
    template <class T>
    void AddGetter(Class& c, const char* name)
    {
        string getType = string(@encode(T)) + string("@:");
        BOOL success = class_addMethod(c, NSSelectorFromString([NSString stringWithUTF8String:name]), (IMP)static_cast<T(*)(id, SEL)>(getImp<T>), getType.c_str());
        if(not success) throwErr();
    }
    
    template <class T>
    void AddSetter(Class& c, const char* name)
    {
        string setType = string("v@:") + string(@encode(T));
        string setName = string("set") + string(name) + string(":");
        BOOL success = class_addMethod(c, NSSelectorFromString([NSString stringWithUTF8String:setName.c_str()]), (IMP)static_cast<void(*)(id, SEL, T)>(setImp<T>), setType.c_str());
        if(not success) throwErr();
    }
    
    inline void CopyMethodFromBase(Class c, Class base, SEL s, const char* t)
    {
        Class meta = object_getClass(c);
        if(not meta) throwErr();
        
        Class baseMeta = object_getClass(base);
        if(not baseMeta) throwErr();
        
        BOOL success = class_addMethod(meta, s, class_getMethodImplementation(baseMeta, s), t);        
        if(not success) throwErr();
    }
}    


#endif

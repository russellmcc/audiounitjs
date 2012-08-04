#ifndef #PROJNAME_CppV8Handler_h
#define #PROJNAME_CppV8Handler_h

#include "cef.h"
#include <functional>

// Note this COPIES the passed-in block.
class CppV8Handler : public CefV8Handler
{
public:
    typedef bool (^Function)(
                            CefRefPtr<CefV8Value>,
                            const CefV8ValueList&,
                            CefRefPtr<CefV8Value>&,
                            CefString&);
    
    CppV8Handler(CefString name, Function func);
    
    CppV8Handler(const CppV8Handler&);
    const CppV8Handler& operator= (const CppV8Handler&);
    virtual ~CppV8Handler();
    
    // Execute with the specified argument list and return value.  Return true if
    // the method was handled.
    virtual bool Execute(const CefString& name,
                         CefRefPtr<CefV8Value> object,
                         const CefV8ValueList& arguments,
                         CefRefPtr<CefV8Value>& retval,
                         CefString& exception) OVERRIDE;
    
  IMPLEMENT_REFCOUNTING(CppV8Handler);
private:
    Function mFunction;
    CefString mName;
};

class WithV8Context
{
public:
    WithV8Context(CefRefPtr<CefV8Context>& context)
        : mContext(context)
    {
        mContext->Enter();
    }
    
    ~WithV8Context()
    {
        mContext->Exit();
    }
    
private:
    // don't copy me
    WithV8Context(const WithV8Context&);
    const WithV8Context& operator=(const WithV8Context&);
    
    CefRefPtr<CefV8Context> mContext;
};

#endif

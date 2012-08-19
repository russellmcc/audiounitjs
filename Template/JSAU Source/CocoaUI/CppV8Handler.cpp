#include "CppV8Handler.h"
#include <Block.h>

CppV8Handler::CppV8Handler(CefString name, Function func)
    : mName(name), mFunction(Block_copy(func)) 
{
}

// Execute with the specified argument list and return value.  Return true if
// the method was handled.
bool CppV8Handler::Execute(const CefString& name,
                     CefRefPtr<CefV8Value> object,
                     const CefV8ValueList& arguments,
                     CefRefPtr<CefV8Value>& retval,
                     CefString& exception)
{
    if(name == mName)
    {
        return mFunction(object, arguments, retval, exception);
    }
    return false;
}

CppV8Handler::CppV8Handler(const CppV8Handler& other)
 : mName(other.mName), mFunction(Block_copy(other.mFunction))
{
}
    
const CppV8Handler& CppV8Handler::operator= (const CppV8Handler& rhs)
{
    mName = rhs.mName;
    mFunction = Block_copy(rhs.mFunction);
}

CppV8Handler::~CppV8Handler()
{
    Block_release(mFunction);
}

//
//  obsfu.cpp
// don't mind me.
//

#include "obsfu.h"
#include <dlfcn.h>

namespace
{
    char caesarChar(char in)
    {
        if((in >= 'a') and (in <= 'z'))
            return (((in - 'a') + 1) % ('z' - 'a' + 1)) + 'a';
        return in;
    }
    
    void caesar(const char* in, char* out)
    {
        for(; *in; ++in)
        {
            *out = caesarChar(*in);
            out++;
        }
        *out = 0;
    }
}

    typedef void (*vfunc)();
#define CALL_BY_NAME(fname) vfunc fname() \
    { \
        static vfunc f = 0; \
        if(not f) { \
            static size_t fs = strlen(#fname); \
            char ff[fs + 1]; \
            caesar(#fname, ff);\
            f = reinterpret_cast<vfunc>(dlsym(RTLD_DEFAULT, ff)); \
        }\
        return f; \
    }

namespace{
CALL_BY_NAME(WdaTgqdzcLnbj)
CALL_BY_NAME(WdaTgqdzcUmknbj)
}

void obsfuLock()
{
#if IPHONE_VER
    WdaTgqdzcLnbj()();
#endif
}

void obsfuUnlock()
{
#if IPHONE_VER
    WdaTgqdzcUmknbj()();
#endif
}

#define SEL_BY_NAME(name, sname)     static SEL sname = 0; \
    if(not sname) {\
        static const char* o = name; \
        char c[strlen(o) + 1]; \
        caesar(o, c); \
        sname = NSSelectorFromString([NSString stringWithUTF8String:c]); \
    }

void obsfuCall(id a, id b)
{
    SEL_BY_NAME("bzkkWdaSbqhosMdsgnc:vhsgAqftldmsr:", sel)
    [a performSelector:sel withObject:@"call" withObject:b];
}

void obsfuSet(id a, id b)
{
    SEL_BY_NAME("_cnbtldmsVhdv", one)
    SEL_BY_NAME("vdaVhdv", two)
    SEL_BY_NAME("rdsFqzldLnzcDdkdfzsd:", three)
    [[[a performSelector:one] performSelector:two] performSelector:three withObject:b];
}

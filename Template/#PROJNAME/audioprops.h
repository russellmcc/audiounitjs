#ifndef audioprops_h
#define audioprops_h

enum
{
    // an array of JSPropDesc (see below).  compare the size of 
    // JSPropDesc to the length of this property to get the count.
    kAudioProp_JSPropList = 0x10000,
    kFirstAudioProp
};

struct JSPropDesc
{

    // allowable types
    enum JSType
    {
        kJSNumber, // passed as a double
        kJSString, // passed as a char array
        kJSNumberArray // passed as an array of doubles.
    };

    JSType type;
    const char* name;
};


#endif

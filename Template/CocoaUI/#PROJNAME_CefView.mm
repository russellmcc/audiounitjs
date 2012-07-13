#import "#PROJNAME_CefView.h"
#include "cef.h"
#include "client_handler.h"
#include "cef_application_mac.h"

#include <objc/runtime.h>


namespace
{
    BOOL gHandling = false;
}

@interface ConformingApp : NSObject
- (void)setHandlingSendEvent:(BOOL)handlingSendEvent;
- (BOOL)isHandlingSendEvent;
- (void)sendEventHacked:(NSEvent *)anEvent;
- (void)doCefLoop;
@end

@implementation ConformingApp 

- (void)setHandlingSendEvent:(BOOL)handlingSendEvent
{
    gHandling = handlingSendEvent;
}

- (BOOL)isHandlingSendEvent
{
    return gHandling;
}

- (void)sendEventHacked:(NSEvent *)event
{
    CefScopedSendingEvent sendingEventScoper;
    // since we're going to switch impls with sendEvent,
    // calling sendEventHacked will end up calling the old 
    // sendEvent.
    [self sendEventHacked:event];
}
-(void) doCefLoop
{
    CefRunMessageLoop();
}
@end

namespace
{
    void AddConformingSelectorToClassIfAbsent(SEL sel, Class target)
    {
        if(class_getInstanceMethod(target, sel))
        {
            return;
        }
                
        Class conformingClass = [ConformingApp class];

        IMP imp = class_getMethodImplementation(conformingClass, sel);
        Method method = class_getInstanceMethod(conformingClass, sel);
        class_addMethod(target, sel, imp, method_getTypeEncoding(method));
    }
}

@implementation #PROJNAME_CefView

- (void)setAU:(AudioUnit)inAU {
	// remove previous listeners

	mAU = inAU;
    
    // Create the browser view.
    CefWindowInfo window_info;
    CefBrowserSettings bsettings;
    CefRefPtr<CefClient> client(new ClientHandler(mAU));
    
    // get the path to a local HTML file.
    NSURL* index = [mBundle URLForResource:@"index" withExtension:@"html" subdirectory:@"ui"];
    
    // fail silently now if there's no index.
    if(not index)
        return;
    
    NSString* path = [index absoluteString];

    window_info.SetAsChild(self, 0, 0, [self bounds].size.width, [self bounds].size.height);
    CefBrowser::CreateBrowser(window_info, client,
                              [path UTF8String], bsettings);	// initial setup
}


// ClientApp implementation.
class ClientApp : public CefApp,
public CefProxyHandler
{
public:
    ClientApp()
    {
    }
    
    // CefApp methods
    virtual CefRefPtr<CefProxyHandler> GetProxyHandler() OVERRIDE { return this; }
    
    // CefProxyHandler methods
    virtual void GetProxyForUrl(const CefString& url,
                                CefProxyInfo& proxy_info) OVERRIDE
    {
    }
    
protected:
    
    IMPLEMENT_REFCOUNTING(ClientApp);
};

#include <dlfcn.h>

-(void) awakeFromNib
{
    CefSettings settings;
    CefRefPtr<CefApp> app(new ClientApp());

    
    mBundle = [NSBundle bundleWithIdentifier:@"ghostfact.CocoaUI0"];
    CefOverrideNSBundle(mBundle);
    
    // now, perform sketchy objc-runtime magic to ensure
    // the current app is CEF-compatible.
    Protocol* cefAppProtocol = objc_getProtocol("CefAppProtocol");
    NSApplication* share = [NSApplication sharedApplication];
    Class appClass = [share class];
    BOOL conform = class_conformsToProtocol(appClass, cefAppProtocol);
    if(not conform)
    {
        AddConformingSelectorToClassIfAbsent(@selector(setHandlingSendEvent:), appClass);
        AddConformingSelectorToClassIfAbsent(@selector(isHandlingSendEvent), appClass);
        AddConformingSelectorToClassIfAbsent(@selector(doCefLoop), appClass);
        if(not class_getInstanceMethod(appClass, @selector(sendEventHacked:)))
        {
            AddConformingSelectorToClassIfAbsent(@selector(sendEventHacked:), appClass);
            
            // swap hacked and real send event hacked.
            Method real = class_getInstanceMethod(appClass, @selector(sendEvent:));
            if(not real) printf("uh oh, we don't have a sendEvent method!");
            Method hacked = class_getInstanceMethod(appClass, @selector(sendEventHacked:));
            if(not hacked) printf("uh oh, we don't have a sendEventHacked method!");
            method_exchangeImplementations(real, hacked);
        }
        
        // okay, now we note that it now conforms to the protocol.
        class_addProtocol(appClass, cefAppProtocol);
        
        [share performSelectorOnMainThread:@selector(doCefLoop) withObject:nil
                             waitUntilDone:NO];
    }
    
    // Initialize CEF.
    CefInitialize(settings, app);    
}
@end

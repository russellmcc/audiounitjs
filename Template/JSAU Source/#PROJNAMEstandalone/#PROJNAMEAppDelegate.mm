#import "#PROJNAMEAppDelegate.h"
#include "jsaubase.h"
#include <cstdio>
#include "CAPlayThrough.h"

@implementation #PROJNAMEAppDelegate 

@synthesize window = _window;

- (void)dealloc
{
    delete receiver;
    [super dealloc];
}

- (void)applicationDidFinishLaunching:(NSNotification *)aNotification
{
    DoRegister('#AUTYPE', '#TYPECODE', '#MANUCODE', CFSTR("#NAME"), 0x00000001);
    host = new CAPlayThroughHost;
    host->Start();
    [uiViewInstance setAU:host->GetEffectAU()];
    receiver = new MIDIReceiver(host->GetEffectAU());
}

- (void)applicationWillTerminate:(NSNotification *)notification
{
    host->Stop();
    delete host;
}

- (BOOL)applicationShouldTerminateAfterLastWindowClosed:(NSApplication*)theApplication {
    return YES;
}

@end

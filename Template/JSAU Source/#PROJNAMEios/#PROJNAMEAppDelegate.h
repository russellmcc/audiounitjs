//
//  #PROJNAMEAppDelegate.h
//  #PROJNAMEios
//
//  Created by James McClellan on 8/26/12.
//

#import <UIKit/UIKit.h>
#import <AudioToolbox/AudioToolbox.h>

@class #PROJNAMEViewController;

@interface #PROJNAMEAppDelegate : UIResponder <UIApplicationDelegate>
{
    AUGraph mGraph;
    AudioUnit mEffect;
}

@property (strong, nonatomic) UIWindow *window;

@property (strong, nonatomic) #PROJNAMEViewController *viewController;

@end

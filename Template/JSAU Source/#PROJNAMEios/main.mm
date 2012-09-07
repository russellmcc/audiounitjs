//
//  main.m
//  #PROJNAMEios
//
//  Created by James McClellan on 8/26/12.
//

#import <UIKit/UIKit.h>
#import <objc/runtime.h>
#import <objc/message.h>
#import "#PROJNAMEAppDelegate.h"

int main(int argc, char *argv[])
{
    @autoreleasepool {
        return UIApplicationMain(argc, argv, nil, NSStringFromClass([#PROJNAMEAppDelegate class]));
    }
}

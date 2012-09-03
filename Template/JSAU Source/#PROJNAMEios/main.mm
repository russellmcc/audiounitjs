//
//  main.m
//  #PROJNAMEios
//
//  Created by James McClellan on 8/26/12.
//  Copyright (c) 2012 __MyCompanyName__. All rights reserved.
//

#import <UIKit/UIKit.h>
#import <objc/runtime.h>
#import <objc/objc-runtime.h>
#import "#PROJNAMEAppDelegate.h"

int main(int argc, char *argv[])
{
    @autoreleasepool {
        return UIApplicationMain(argc, argv, nil, NSStringFromClass([#PROJNAMEAppDelegate class]));
    }
}

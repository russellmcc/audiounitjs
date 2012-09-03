//
//  #PROJNAMEViewController.h
//  #PROJNAMEios
//
//  Created by James McClellan on 8/26/12.
//  Copyright (c) 2012 __MyCompanyName__. All rights reserved.
//

#import <UIKit/UIKit.h>

@interface #PROJNAMEViewController : UIViewController<UIWebViewDelegate>
{
    IBOutlet UIWebView* mWebview;
    id mJSObj;
    AudioUnit mAU;
}

-(void)setAU:(AudioUnit)au;
@end

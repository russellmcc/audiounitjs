//
//  AUParam.h
//  #PROJNAME
//
//  Created by James McClellan on 8/25/12.
//

#import "AUPropParamBase.h"

@interface #PROJNAME_AUParam : #PROJNAME_AUPropParamBase
{
    id OnBeginGesture;
    id OnEndGesture;
}

@property (retain) id OnBeginGesture;
@property (retain) id OnEndGesture;


-(id)initWithAU:(AudioUnit)au withID:(UInt32)id;

-(id)Get;
-(void)Set:(id)val;
-(void)BeginGesture;
-(void)EndGesture;
+(NSString *)webScriptNameForSelector:(SEL)sel;
@end

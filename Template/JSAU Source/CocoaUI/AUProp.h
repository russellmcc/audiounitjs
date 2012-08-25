//
//  AUProp.h
//  #PROJNAME
//
//  Created by James McClellan on 8/25/12.
//

#import "AUPropParamBase.h"
#include "audioprops.h"

@interface #PROJNAME_AUProp : #PROJNAME_AUPropParamBase
{
    JSPropDesc::JSType mType;
}
-(id)initWithAU:(AudioUnit)au withID:(UInt32)id withContext:(JSGlobalContextRef)context withType:(JSPropDesc::JSType)type;
-(id)Get;
@end

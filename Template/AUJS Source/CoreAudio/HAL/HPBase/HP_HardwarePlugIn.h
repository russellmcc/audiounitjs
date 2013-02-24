/*	Copyright © 2007 Apple Inc. All Rights Reserved.
	
	Disclaimer: IMPORTANT:  This Apple software is supplied to you by 
			Apple Inc. ("Apple") in consideration of your agreement to the
			following terms, and your use, installation, modification or
			redistribution of this Apple software constitutes acceptance of these
			terms.  If you do not agree with these terms, please do not use,
			install, modify or redistribute this Apple software.
			
			In consideration of your agreement to abide by the following terms, and
			subject to these terms, Apple grants you a personal, non-exclusive
			license, under Apple's copyrights in this original Apple software (the
			"Apple Software"), to use, reproduce, modify and redistribute the Apple
			Software, with or without modifications, in source and/or binary forms;
			provided that if you redistribute the Apple Software in its entirety and
			without modifications, you must retain this notice and the following
			text and disclaimers in all such redistributions of the Apple Software. 
			Neither the name, trademarks, service marks or logos of Apple Inc. 
			may be used to endorse or promote products derived from the Apple
			Software without specific prior written permission from Apple.  Except
			as expressly stated in this notice, no other rights or licenses, express
			or implied, are granted by Apple herein, including but not limited to
			any patent rights that may be infringed by your derivative works or by
			other works in which the Apple Software may be incorporated.
			
			The Apple Software is provided by Apple on an "AS IS" basis.  APPLE
			MAKES NO WARRANTIES, EXPRESS OR IMPLIED, INCLUDING WITHOUT LIMITATION
			THE IMPLIED WARRANTIES OF NON-INFRINGEMENT, MERCHANTABILITY AND FITNESS
			FOR A PARTICULAR PURPOSE, REGARDING THE APPLE SOFTWARE OR ITS USE AND
			OPERATION ALONE OR IN COMBINATION WITH YOUR PRODUCTS.
			
			IN NO EVENT SHALL APPLE BE LIABLE FOR ANY SPECIAL, INDIRECT, INCIDENTAL
			OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
			SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
			INTERRUPTION) ARISING IN ANY WAY OUT OF THE USE, REPRODUCTION,
			MODIFICATION AND/OR DISTRIBUTION OF THE APPLE SOFTWARE, HOWEVER CAUSED
			AND WHETHER UNDER THEORY OF CONTRACT, TORT (INCLUDING NEGLIGENCE),
			STRICT LIABILITY OR OTHERWISE, EVEN IF APPLE HAS BEEN ADVISED OF THE
			POSSIBILITY OF SUCH DAMAGE.
*/
#if !defined(__HP_HardwarePlugIn_h__)
#define __HP_HardwarePlugIn_h__

//==================================================================================================
//	Includes
//==================================================================================================

//	Super Class Includes
#include "HP_Object.h"

//	PublicUtility Includes
#include "CACFObject.h"

//	System Includes
#include <CoreAudio/AudioHardwarePlugIn.h>
#include <CoreFoundation/CoreFoundation.h>

//==================================================================================================
//	HP_HardwarePlugIn
//==================================================================================================

class HP_HardwarePlugIn
:
	public HP_Object
{

//	Construction/Destruction
public:
									HP_HardwarePlugIn(CFUUIDRef inFactoryUUID);
	virtual							~HP_HardwarePlugIn();
	
	virtual void					Initialize();
	virtual void					InitializeWithObjectID(AudioObjectID inObjectID);
	virtual void					Teardown();

//	Reference Counting
public:
	virtual UInt32					Retain();
	virtual UInt32					Release();

//	Implementation
public:
	AudioHardwarePlugInRef			GetInterface()									{ return &mInterface; }
	bool							SameFactoryUUID(CFUUIDRef theUUID)				{ return mFactoryUUID.IsEqual(theUUID); }

	static HP_HardwarePlugIn*		GetObject(AudioHardwarePlugInRef inRef)			{ HP_HardwarePlugIn* p = (HP_HardwarePlugIn*)inRef; return (HP_HardwarePlugIn*)((Byte*)p - ((Byte*)&p->mInterface - (Byte*)p)); }
	
private:
	AudioHardwarePlugInInterface*	mInterface;
	CACFUUID						mFactoryUUID;
	UInt32							mRefCount;

	static AudioHardwarePlugInInterface sInterface;

};

#endif

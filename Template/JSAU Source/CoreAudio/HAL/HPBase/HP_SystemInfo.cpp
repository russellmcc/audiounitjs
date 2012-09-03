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
//==================================================================================================
//	Includes
//==================================================================================================

//	Self Include
#include "HP_SystemInfo.h"

//	PublicUtility Includes
#include "CAPropertyAddress.h"

//==================================================================================================
//	HP_SystemInfo
//==================================================================================================

void	HP_SystemInfo::Initialize()
{
	if(!sIsInitialized)
	{
		//	set up the user session status including initializing it
		CAPropertyAddress theAddress('user');	//	kAudioHardwarePropertyUserSessionIsActiveOrHeadless
		AudioObjectAddPropertyListener(kAudioObjectSystemObject, &theAddress, SystemListener, NULL);
		SystemListener(kAudioObjectSystemObject, 1, &theAddress, NULL);
		
		//	set up the audibility status
		theAddress.mSelector = 'pmut';	//	kAudioHardwarePropertyProcessIsAudible
		AudioObjectAddPropertyListener(kAudioObjectSystemObject, &theAddress, SystemListener, NULL);
		SystemListener(kAudioObjectSystemObject, 1, &theAddress, NULL);
		
		//	set up the mixing mono to stereo status
		theAddress.mSelector = 'stmo';	//	kAudioHardwarePropertyMixStereoToMono
		AudioObjectAddPropertyListener(kAudioObjectSystemObject, &theAddress, SystemListener, NULL);
		SystemListener(kAudioObjectSystemObject, 1, &theAddress, NULL);
		
		sIsInitialized = true;
	}
}

void	HP_SystemInfo::Teardown()
{
	if(sIsInitialized)
	{
		CAPropertyAddress theAddress('user');	//	kAudioHardwarePropertyUserSessionIsActiveOrHeadless
		AudioObjectRemovePropertyListener(kAudioObjectSystemObject, &theAddress, SystemListener, NULL);
		
		theAddress.mSelector = 'pmut';	//	kAudioHardwarePropertyProcessIsAudible
		AudioObjectRemovePropertyListener(kAudioObjectSystemObject, &theAddress, SystemListener, NULL);
		
		sIsInitialized = false;
	}
}
	
bool	HP_SystemInfo::IsCurrentProcessTheMaster()
{
	UInt32 isMaster = 0;
	UInt32 theSize = SizeOf32(UInt32);
	AudioObjectPropertyAddress theAddress = { kAudioHardwarePropertyProcessIsMaster, kAudioObjectPropertyScopeGlobal, kAudioObjectPropertyElementMaster };
	AudioObjectGetPropertyData(kAudioObjectSystemObject, &theAddress, 0, NULL, &theSize, &isMaster);
	return isMaster != 0;
}

bool	HP_SystemInfo::IsCurrentProcessInitingOrExiting()
{
	UInt32 isInitingOrExiting = 0;
	UInt32 theSize = SizeOf32(UInt32);
	AudioObjectPropertyAddress theAddress = { kAudioHardwarePropertyIsInitingOrExiting, kAudioObjectPropertyScopeGlobal, kAudioObjectPropertyElementMaster };
	AudioObjectGetPropertyData(kAudioObjectSystemObject, &theAddress, 0, NULL, &theSize, &isInitingOrExiting);
	return isInitingOrExiting != 0;
}

OSStatus	HP_SystemInfo::SystemListener(AudioObjectID inObjectID, UInt32 inNumberAddresses, const AudioObjectPropertyAddress inAddresses[], void* /*inClientData*/)
{
	if(inObjectID == kAudioObjectSystemObject)
	{
		for(UInt32 theAddressIndex = 0; theAddressIndex < inNumberAddresses; ++theAddressIndex)
		{
			switch(inAddresses[theAddressIndex].mSelector)
			{
				case 'user':	//	kAudioHardwarePropertyUserSessionIsActiveOrHeadless
					{
						CAPropertyAddress theAddress('user');	//	kAudioHardwarePropertyUserSessionIsActiveOrHeadless
						UInt32 theUserSessionIsActiveOrHeadless = 1;
						UInt32 theSize = SizeOf32(UInt32);
						AudioObjectGetPropertyData(kAudioObjectSystemObject, &theAddress, 0, NULL, &theSize, &theUserSessionIsActiveOrHeadless);
						sCurrentUserSessionIsActiveOrHeadless = theUserSessionIsActiveOrHeadless != 0;
					}
					break;
				
				case 'pmut':	//	kAudioHardwarePropertyProcessIsAudible
					{
						CAPropertyAddress theAddress('pmut');	//	kAudioHardwarePropertyProcessIsAudible
						UInt32 theProcessIsAudible = 1;
						UInt32 theSize = SizeOf32(UInt32);
						AudioObjectGetPropertyData(kAudioObjectSystemObject, &theAddress, 0, NULL, &theSize, &theProcessIsAudible);
						sCurrentProcessIsAudible = theProcessIsAudible != 0;
					}
					break;
				
				case 'stmo':	//	kAudioHardwarePropertyMixStereoToMono
					{
						CAPropertyAddress theAddress('stmo');	//	kAudioHardwarePropertyMixStereoToMono
						UInt32 theIsMixingStereoToMono = 1;
						UInt32 theSize = SizeOf32(UInt32);
						OSStatus theError = AudioObjectGetPropertyData(kAudioObjectSystemObject, &theAddress, 0, NULL, &theSize, &theIsMixingStereoToMono);
						if(theError == 0)
						{
							sIsMixingStereoToMono = theIsMixingStereoToMono != 0;
						}
					}
					break;
			};
		}
	}

	return 0;
}

bool	HP_SystemInfo::sIsInitialized = false;
bool	HP_SystemInfo::sCurrentUserSessionIsActiveOrHeadless = true;
bool	HP_SystemInfo::sCurrentProcessIsAudible = true;
bool	HP_SystemInfo::sIsMixingStereoToMono = false;

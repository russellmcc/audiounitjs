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
#include "HP_HogMode.h"

//	Internal Includes
#include "HP_Device.h"
#include "HP_SystemInfo.h"

//	PublicUtility Includes
#include "CACFDistributedNotification.h"
#include "CACFNumber.h"
#include "CACFString.h"
#include "CADebugMacros.h"
#include "CAException.h"

#if HogMode_UseCFPrefs
	#include "CACFPreferences.h"
#else
	#include "CASettingsStorage.h"
#endif

//	System Includes
#include <CoreFoundation/CFNotificationCenter.h>

//==================================================================================================
//	HP_HogMode
//==================================================================================================

HP_HogMode::HP_HogMode(HP_Device* inDevice)
:
	mToken(0),
	mDevice(inDevice),
	mPrefName(NULL),
	mOwner(-2)
{
	pthread_once(&sStaticInitializer, StaticInitializer);
	
	//	get our token
	mToken = sTokenMap->MapObject(this);
	
	//	construct the name of the preference
	mPrefName = CFStringCreateMutable(NULL, 0);
	ThrowIfNULL(mPrefName, CAException(kAudioHardwareUnspecifiedError), "HP_HogMode::HP_HogMode: couldn't allocate the pref name string");
	CFStringAppendCString((CFMutableStringRef)mPrefName, "com.apple.audio.CoreAudio.HogMode.Owner-", kCFStringEncodingASCII);
	CACFString theUID(inDevice->CopyDeviceUID());
	CFStringAppend((CFMutableStringRef)mPrefName, theUID.GetCFString());
	
	//	sign up for notifications
	CACFDistributedNotification::AddObserver((const void*)mToken, (CFNotificationCallback)ChangeNotification, mPrefName, 0);
}

HP_HogMode::~HP_HogMode()
{
	CACFDistributedNotification::RemoveObserver((const void*)mToken, mPrefName);
	CFRelease(mPrefName);
	sTokenMap->UnmapObject(this);
}

void	HP_HogMode::Initialize()
{
	//  figure out if this is the master process
	bool isMaster = HP_SystemInfo::IsCurrentProcessTheMaster();
	
	//	get the current hog mode value from the prefs
	pid_t theHogModeOwner = GetOwnerFromPreference(true);
	
	//	if this process is the master, blow away the hog mode setting as nobody should have hog mode at this point
	//	also blow the setting away if it is pointing at this process, as this process can't have hog mode at this point either
	if((theHogModeOwner != -1) && (isMaster || (theHogModeOwner == CAProcess::GetPID())))
	{
		//	set the value on the disk
		SetOwnerInPreference(-1);
		
		//	signal that hog mode changed to the world in case somebody thinks they have it
		SendHogModeChangedNotification();
	}
}

void	HP_HogMode::StaticInitializer()
{
#if	!HogMode_UseCFPrefs
	sSettingsStorage = new CASettingsStorage("/tmp/com.apple.audio.hogmode.plist", S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH);
#endif	
	sTokenMap = new CATokenMap<HP_HogMode>();
}

pthread_once_t	HP_HogMode::sStaticInitializer = PTHREAD_ONCE_INIT;

pid_t	HP_HogMode::GetOwner() const
{
	if(mOwner == -2)
	{
		const_cast<HP_HogMode*>(this)->mOwner = GetOwnerFromPreference(true);
	}
	return mOwner;
}

bool	HP_HogMode::IsFree() const
{
	if(mOwner == -2)
	{
		const_cast<HP_HogMode*>(this)->mOwner = GetOwnerFromPreference(true);
	}
	return (mOwner == -1);
}

bool	HP_HogMode::CurrentProcessIsOwner() const
{
	if(mOwner == -2)
	{
		const_cast<HP_HogMode*>(this)->mOwner = GetOwnerFromPreference(true);
	}
	return (mOwner == CAProcess::GetPID());
}

bool	HP_HogMode::CurrentProcessIsOwnerOrIsFree() const
{
	if(mOwner == -2)
	{
		const_cast<HP_HogMode*>(this)->mOwner = GetOwnerFromPreference(true);
	}
	return (CurrentProcessIsOwner() || IsFree());
}

void	HP_HogMode::Take()
{
	if(IsFree())
	{
		//	set the new owner
		mOwner = CAProcess::GetPID();
		
		//	write the new owner's pid to the preferences
		SetOwnerInPreference(mOwner);
		
		//	tell the device that the hog mode state has changed
		mDevice->HogModeStateChanged();
		
		//	signal that hog mode changed to the device
		CAPropertyAddress theAddress(kAudioDevicePropertyHogMode);
		mDevice->PropertiesChanged(1, &theAddress);
		
		//	signal that hog mode changed to the world
		SendHogModeChangedNotification();
	}
}

void	HP_HogMode::Release()
{
	if(CurrentProcessIsOwner())
	{
		//	set the owner to free
		mOwner = -1;
		
		//	delete the pref
		SetOwnerInPreference(-1);
		
		//	tell the device that the hog mode state has changed
		mDevice->HogModeStateChanged();
		
		//	signal that hog mode changed to the device
		CAPropertyAddress theAddress(kAudioDevicePropertyHogMode);
		mDevice->PropertiesChanged(1, &theAddress);
		
		//	signal that hog mode changed to the world
		SendHogModeChangedNotification();
	}
}

pid_t	HP_HogMode::GetOwnerFromPreference(bool inSendNotifications) const
{
	pid_t theAnswer = -1;
	
#if HogMode_UseCFPrefs
	//	get the preference
	CFNumberRef theCFNumber = CACFPreferences::CopyNumberValue(mPrefName, false, true);
	if(theCFNumber != NULL)
	{
		//	get the number
		pid_t theOwner = -1;
		CFNumberGetValue(theCFNumber, kCFNumberSInt32Type, &theOwner);
		
		//	make sure the process exists
		if(theOwner == -1)
		{
			//	hog mode is free
			theAnswer = -1;
		}
		else if(CAProcess::ProcessExists(theOwner))
		{
			//	it does, so set the return value
			theAnswer = theOwner;
		}
		else
		{
			//	it doesn't, so delete the pref
			SetOwnerInPreference((pid_t)-1);
			
			if(inSendNotifications)
			{
				//	signal that hog mode changed
				SendHogModeChangedNotification();
			}
		}
		CFRelease(theCFNumber);
	}
#else
	//	get the owner from the preference
	SInt32 theOwner = -1;
	sSettingsStorage->CopySInt32Value(mPrefName, theOwner, SInt32(-1));
		
	//	make sure the process exists
	if(theOwner == -1)
	{
		//	hog mode is free
		theAnswer = -1;
	}
	else if(CAProcess::ProcessExists(theOwner))
	{
		//	the process that owns hog mode exists
		theAnswer = theOwner;
	}
	else
	{
		//	the process that owns hog mode doesn't exist, so delete the pref
		theAnswer = -1;
		SetOwnerInPreference((pid_t)-1);
		
		if(inSendNotifications)
		{
			//	signal that hog mode changed
			SendHogModeChangedNotification();
		}
	}
#endif
	
	return theAnswer;
}

void	HP_HogMode::SetOwnerInPreference(pid_t inOwner) const
{
#if HogMode_UseCFPrefs
	if(inOwner != -1)
	{
		CACFNumber theNumber(static_cast<SInt32>(inOwner));
		CACFPreferences::SetValue(mPrefName, theNumber.GetCFNumber(), false, true, true);
	}
	else
	{
		CACFPreferences::DeleteValue(mPrefName, false, true, true);
	}
	CACFPreferences::Synchronize(false, true, true);
#else
	if(inOwner != -1)
	{
		sSettingsStorage->SetSInt32Value(mPrefName, inOwner);
	}
	else
	{
		sSettingsStorage->RemoveValue(mPrefName);
	}
#endif
}

void	HP_HogMode::MarkPreferencesDirty()
{
#if HogMode_UseCFPrefs
	CACFPreferences::MarkPrefsOutOfDate(false, true);
#else
	sSettingsStorage->ForceRefresh();
#endif
}

void	HP_HogMode::SendHogModeChangedNotification() const
{
	CACFDistributedNotification::PostNotification(mPrefName, NULL, true);
}

void	HP_HogMode::ChangeNotification(CFNotificationCenterRef /*inCenter*/, const void* inHogMode, CFStringRef /*inNotificationName*/, const void* /*inObject*/, CFDictionaryRef /*inUserInfo*/)
{
	try
	{
		if(sTokenMap != NULL)
		{
			HP_HogMode* theHogModeObject = sTokenMap->GetObject(inHogMode);
			if(theHogModeObject != NULL)
			{
				//	mark the prefs as dirty
				theHogModeObject->MarkPreferencesDirty();
				
				//	get the new owner
				pid_t theNewOwner = theHogModeObject->GetOwnerFromPreference(false);
				pid_t theOldOwner = theHogModeObject->GetOwner();
				
				if(theNewOwner != theOldOwner)
				{
					//	it's different for this process
					theHogModeObject->SetOwner(theNewOwner);
					theHogModeObject->GetDevice()->HogModeStateChanged();
					
					//	send the notification
					CAPropertyAddress theAddress(kAudioDevicePropertyHogMode);
					theHogModeObject->GetDevice()->PropertiesChanged(1, &theAddress);
				}
			}
		}
	}
	catch(...)
	{
	}
}

#if	!HogMode_UseCFPrefs
	CASettingsStorage*	HP_HogMode::sSettingsStorage = NULL;
#endif	
CATokenMap<HP_HogMode>*	HP_HogMode::sTokenMap = NULL;

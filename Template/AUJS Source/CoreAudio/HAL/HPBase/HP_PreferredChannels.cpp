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
#include "HP_PreferredChannels.h"

//	Local Includes
#include "HP_Device.h"

//	PublicUtility Includes
#include "CAAudioChannelLayout.h"
#include "CACFArray.h"
#include "CACFDictionary.h"
#include "CACFDistributedNotification.h"
#include "CACFPreferences.h"
#include "CACFString.h"
#include "CADebugMacros.h"
#include "CAException.h"

//==================================================================================================
//	HP_PreferredChannels
//==================================================================================================

static void	HP_PreferredChannels_ConstructDictionaryFromLayout(const AudioChannelLayout& inLayout, CACFDictionary& outLayoutDictionary)
{
	//	stick in the tag
	outLayoutDictionary.AddUInt32(CFSTR("channel layout tag"), inLayout.mChannelLayoutTag);
	
	//	stick in the bitmap
	outLayoutDictionary.AddUInt32(CFSTR("channel bitmap"), inLayout.mChannelBitmap);
	
	//	stick in the number channels
	outLayoutDictionary.AddUInt32(CFSTR("number channels"), inLayout.mNumberChannelDescriptions);
	
	//	add the channel descriptions, if necessary
	if(inLayout.mChannelLayoutTag == kAudioChannelLayoutTag_UseChannelDescriptions)
	{
		//	create an array to hold the channel descriptions
		CACFArray theChannelDescriptions(true);
		if(theChannelDescriptions.IsValid())
		{
			//	iterate through the descriptions and stick them in the array
			for(UInt32 theChannelIndex = 0; theChannelIndex < inLayout.mNumberChannelDescriptions; ++theChannelIndex)
			{
				//	create a dictionary to hold the description
				CACFDictionary theDescription(true);
				if(theDescription.IsValid())
				{
					//	stick in the easy values
					theDescription.AddUInt32(CFSTR("channel label"), inLayout.mChannelDescriptions[theChannelIndex].mChannelLabel);
					theDescription.AddUInt32(CFSTR("channel flags"), inLayout.mChannelDescriptions[theChannelIndex].mChannelFlags);
				
					//	create an array to hold the coordinates
					CACFArray theCoordinates(true);
					if(theCoordinates.IsValid())
					{
						//	add the coordinates to the array
						for(UInt32 theCoordinateIndex = 0; theCoordinateIndex < 3; ++theCoordinateIndex)
						{
							theCoordinates.AppendFloat32(inLayout.mChannelDescriptions[theChannelIndex].mCoordinates[theCoordinateIndex]);
						}
						
						//	add the array of coordinates to the description
						theDescription.AddArray(CFSTR("coordinates"), theCoordinates.GetCFArray());
					}
					
					//	add the description to the array of descriptions
					theChannelDescriptions.AppendDictionary(theDescription.GetCFDictionary());
				}
			}
			
			//	add the array of descriptions to the layout dictionary
			outLayoutDictionary.AddArray(CFSTR("channel descriptions"), theChannelDescriptions.GetCFArray());
		}
	}
}

static void	HP_PreferredChannels_ConstructLayoutFromDictionary(const CACFDictionary& inLayoutDictionary, AudioChannelLayout& outLayout)
{
	//	get the tag, bitmap
	inLayoutDictionary.GetUInt32(CFSTR("channel layout tag"), outLayout.mChannelLayoutTag);
	inLayoutDictionary.GetUInt32(CFSTR("channel bitmap"), outLayout.mChannelBitmap);
	
	//	get the number of channels specified in the dictionary
	UInt32 theNumberChannelsInDictionary = 0;
	inLayoutDictionary.GetUInt32(CFSTR("number channels"), theNumberChannelsInDictionary);
	
	//	get the descriptions if they are present and required
	CFArrayRef __theDescriptions;
	if((outLayout.mChannelLayoutTag == kAudioChannelLayoutTag_UseChannelDescriptions) && inLayoutDictionary.GetArray(CFSTR("channel descriptions"), __theDescriptions))
	{
		//	don't release this array because it came straight out of the dictionary
		CACFArray theDescriptions(__theDescriptions, false);
		
		//	get the number of items in the array
		UInt32 theNumberItems = theDescriptions.GetNumberItems();
		
		//	iterate through the array and fill out the struct
		for(UInt32 theItemIndex = 0; (theItemIndex < theNumberItems) && (theItemIndex < outLayout.mNumberChannelDescriptions); ++theItemIndex)
		{
			//	get the description
			CFDictionaryRef __theDescription;
			if(theDescriptions.GetDictionary(theItemIndex, __theDescription))
			{
				//	don't release this dictionary because it came straight out of the array
				CACFDictionary theDescription(__theDescription, false);
				
				//	get the channel label and flags
				theDescription.GetUInt32(CFSTR("channel label"), outLayout.mChannelDescriptions[theItemIndex].mChannelLabel);
				theDescription.GetUInt32(CFSTR("channel flags"), outLayout.mChannelDescriptions[theItemIndex].mChannelFlags);
				
				//	get the coordinates
				CFArrayRef __theCoordinates;
				if(theDescription.GetArray(CFSTR("coordinates"), __theCoordinates))
				{
					//	don't release this array because it came straight out of the dictionary
					CACFArray theCoordinates(__theCoordinates, false);
					
					//	iterate through the array and get the coordinates
					UInt32 theNumberCoordinates = theCoordinates.GetNumberItems();
					for(UInt32 theCoordinateIndex = 0; (theCoordinateIndex < 3) && (theCoordinateIndex < theNumberCoordinates); ++theCoordinateIndex)
					{
						theCoordinates.GetFloat32(theCoordinateIndex, outLayout.mChannelDescriptions[theItemIndex].mCoordinates[theCoordinateIndex]);
					}
				}
			}
		}
	}
}

HP_PreferredChannels::HP_PreferredChannels(HP_Device* inDevice)
:
	HP_Property(),
	mDevice(inDevice),
	mToken(0),
	mInputStereoPrefsKey(NULL),
	mOutputStereoPrefsKey(NULL),
	mInputChannelLayoutPrefsKey(NULL),
	mOutputChannelLayoutPrefsKey(NULL),
	mPreferredInputStereoChannels(NULL),
	mPreferredOutputStereoChannels(NULL),
	mOutputStereoPair(),
	mPreferredInputChannelLayout(NULL),
	mPreferredOutputChannelLayout(NULL)
{
	//	get our token
	if(sTokenMap == NULL)
	{
		sTokenMap = new CATokenMap<HP_PreferredChannels>();
	}
	mToken = sTokenMap->MapObject(this);
}

HP_PreferredChannels::~HP_PreferredChannels()
{
	sTokenMap->RemoveMapping(mToken, this);
}

void	HP_PreferredChannels::Initialize()
{
	//	construct the name of the preferences
	CACFString theUID(mDevice->CopyDeviceUID());
	
	mInputStereoPrefsKey = CFStringCreateWithFormat(NULL, NULL, CFSTR("com.apple.audio.CoreAudio.PreferredStereoChannels.%s.%@"), "Input", theUID.GetCFString());
	ThrowIfNULL(mInputStereoPrefsKey, CAException(kAudioHardwareIllegalOperationError), "HP_PreferredChannels::Initialize: couldn't create the input stereo prefs key");
	
	mOutputStereoPrefsKey = CFStringCreateWithFormat(NULL, NULL, CFSTR("com.apple.audio.CoreAudio.PreferredStereoChannels.%s.%@"), "Output", theUID.GetCFString());
	ThrowIfNULL(mOutputStereoPrefsKey, CAException(kAudioHardwareIllegalOperationError), "HP_PreferredChannels::Initialize: couldn't create the output stereo prefs key");
	
	mInputChannelLayoutPrefsKey = CFStringCreateWithFormat(NULL, NULL, CFSTR("com.apple.audio.CoreAudio.PreferredChannelLayout.%s.%@"), "Input", theUID.GetCFString());
	ThrowIfNULL(mInputChannelLayoutPrefsKey, CAException(kAudioHardwareIllegalOperationError), "HP_PreferredChannels::Initialize: couldn't create the input channel layout prefs key");
	
	mOutputChannelLayoutPrefsKey = CFStringCreateWithFormat(NULL, NULL, CFSTR("com.apple.audio.CoreAudio.PreferredChannelLayout.%s.%@"), "Output", theUID.GetCFString());
	ThrowIfNULL(mOutputChannelLayoutPrefsKey, CAException(kAudioHardwareIllegalOperationError), "HP_PreferredChannels::Initialize: couldn't create the output channel layout prefs key");
	
	//	cache the prefs
	mPreferredInputStereoChannels = CACFPreferences::CopyArrayValue(mInputStereoPrefsKey, false, true);
	mPreferredOutputStereoChannels = CACFPreferences::CopyArrayValue(mOutputStereoPrefsKey, false, true);
	mPreferredInputChannelLayout = CACFPreferences::CopyDictionaryValue(mInputChannelLayoutPrefsKey, false, true);
	mPreferredOutputChannelLayout = CACFPreferences::CopyDictionaryValue(mOutputChannelLayoutPrefsKey, false, true);
	
	//	cache the actual CFArray values for the output preferred stereo pair
	CACFArray thePrefStereoChannels(mPreferredOutputStereoChannels, false);
	if(thePrefStereoChannels.IsValid())
	{
		thePrefStereoChannels.GetUInt32(0, mOutputStereoPair[0]);
		thePrefStereoChannels.GetUInt32(1, mOutputStereoPair[1]);
	}
	else
	{
		mOutputStereoPair[0] = 1;
		mOutputStereoPair[1] = 2;
	}
	
	//	sign up for notifications
	CACFDistributedNotification::AddObserver((const void*)mToken, (CFNotificationCallback)ChangeNotification, mInputStereoPrefsKey);
	CACFDistributedNotification::AddObserver((const void*)mToken, (CFNotificationCallback)ChangeNotification, mOutputStereoPrefsKey);
	CACFDistributedNotification::AddObserver((const void*)mToken, (CFNotificationCallback)ChangeNotification, mInputChannelLayoutPrefsKey);
	CACFDistributedNotification::AddObserver((const void*)mToken, (CFNotificationCallback)ChangeNotification, mOutputChannelLayoutPrefsKey);
}

void	HP_PreferredChannels::Teardown()
{
	CACFDistributedNotification::RemoveObserver((const void*)mToken, mInputStereoPrefsKey);
	CACFDistributedNotification::RemoveObserver((const void*)mToken, mOutputStereoPrefsKey);
	CACFDistributedNotification::RemoveObserver((const void*)mToken, mInputChannelLayoutPrefsKey);
	CACFDistributedNotification::RemoveObserver((const void*)mToken, mOutputChannelLayoutPrefsKey);
	if(mPreferredInputStereoChannels != NULL)
	{
		CFRelease(mPreferredInputStereoChannels);
	}
	if(mPreferredOutputStereoChannels != NULL)
	{
		CFRelease(mPreferredOutputStereoChannels);
	}
	if(mPreferredInputChannelLayout != NULL)
	{
		CFRelease(mPreferredInputChannelLayout);
	}
	if(mPreferredOutputChannelLayout != NULL)
	{
		CFRelease(mPreferredOutputChannelLayout);
	}
	CFRelease(mOutputChannelLayoutPrefsKey);
	CFRelease(mInputChannelLayoutPrefsKey);
	CFRelease(mOutputStereoPrefsKey);
	CFRelease(mInputStereoPrefsKey);
}

bool	HP_PreferredChannels::IsActive(const AudioObjectPropertyAddress& inAddress) const
{
	bool theAnswer = false;
	
	switch(inAddress.mSelector)
	{
		case kAudioDevicePropertyPreferredChannelsForStereo:
			theAnswer = ((inAddress.mScope == kAudioDevicePropertyScopeInput) && (mDevice->GetTotalNumberChannels(true) > 1)) || ((inAddress.mScope == kAudioDevicePropertyScopeOutput) && (mDevice->GetTotalNumberChannels(false) > 1));
			break;
		
		case kAudioDevicePropertyPreferredChannelLayout:
			theAnswer = ((inAddress.mScope == kAudioDevicePropertyScopeInput) && mDevice->HasInputStreams()) || ((inAddress.mScope == kAudioDevicePropertyScopeOutput) && mDevice->HasOutputStreams());
			break;
		
		case 'srdd':
			theAnswer = ((inAddress.mScope == kAudioDevicePropertyScopeInput) && mDevice->HasInputStreams()) || ((inAddress.mScope == kAudioDevicePropertyScopeOutput) && mDevice->HasOutputStreams());
			break;
	};
	
	return theAnswer;
}

bool	HP_PreferredChannels::IsPropertySettable(const AudioObjectPropertyAddress& inAddress) const
{
	bool theAnswer = false;
	
	switch(inAddress.mSelector)
	{
		case kAudioDevicePropertyPreferredChannelsForStereo:
			theAnswer = true;
			break;
		
		case kAudioDevicePropertyPreferredChannelLayout:
			theAnswer = true;
			break;
		
		case 'srdd':
			theAnswer = false;
			break;
	};
	
	return theAnswer;
}

UInt32	HP_PreferredChannels::GetPropertyDataSize(const AudioObjectPropertyAddress& inAddress, UInt32 /*inQualifierDataSize*/, const void* /*inQualifierData*/) const
{
	UInt32 theAnswer = 0;
	
	switch(inAddress.mSelector)
	{
		case kAudioDevicePropertyPreferredChannelsForStereo:
			theAnswer = 2 * SizeOf32(UInt32);
			break;
			
		case kAudioDevicePropertyPreferredChannelLayout:
			theAnswer = CAAudioChannelLayout::CalculateByteSize(mDevice->GetTotalNumberChannels(inAddress.mScope == kAudioDevicePropertyScopeInput));
			break;
			
		case 'srdd':
			theAnswer = CAAudioChannelLayout::CalculateByteSize(mDevice->GetTotalNumberChannels(inAddress.mScope == kAudioDevicePropertyScopeInput));
			break;
	};
	
	return theAnswer;
}

void	HP_PreferredChannels::GetPropertyData(const AudioObjectPropertyAddress& inAddress, UInt32 inQualifierDataSize, const void* inQualifierData, UInt32& ioDataSize, void* outData) const
{
	bool isInput = inAddress.mScope == kAudioDevicePropertyScopeInput;
	switch(inAddress.mSelector)
	{
		case kAudioDevicePropertyPreferredChannelsForStereo:
			{
				ThrowIf(ioDataSize != GetPropertyDataSize(inAddress, inQualifierDataSize, inQualifierData), CAException(kAudioHardwareBadPropertySizeError), "HP_PreferredChannels::GetPropertyData: wrong data size for kAudioDevicePropertyPreferredChannelsForStereo");
				UInt32* theStereoChannels = static_cast<UInt32*>(outData);
				
				//	initialize the output
				theStereoChannels[0] = 1;
				theStereoChannels[1] = 2;
				
				//	get the preference
				CACFArray thePrefStereoChannels(isInput ? mPreferredInputStereoChannels : mPreferredOutputStereoChannels, false);
				if(thePrefStereoChannels.IsValid())
				{
					//	get the values from the array
					thePrefStereoChannels.GetUInt32(0, theStereoChannels[0]);
					thePrefStereoChannels.GetUInt32(1, theStereoChannels[1]);
				}
				
				if (!isInput) 
				{
					// update the cached value if necessary
					memcpy(const_cast<HP_PreferredChannels*>(this)->mOutputStereoPair, theStereoChannels, sizeof(UInt32)*2);
				}
			}
			break;
			
		case kAudioDevicePropertyPreferredChannelLayout:
			{
				ThrowIf(ioDataSize != GetPropertyDataSize(inAddress, inQualifierDataSize, inQualifierData), CAException(kAudioHardwareBadPropertySizeError), "HP_PreferredChannels::GetPropertyData: wrong data size for kAudioDevicePropertyPreferredChannelLayout");
				AudioChannelLayout* theChannelLayout = static_cast<AudioChannelLayout*>(outData);
				
				//	fetch the default layout from the device
				mDevice->GetDefaultChannelLayout(isInput, *theChannelLayout);
				
				//	get the pref
				CACFDictionary thePrefChannelLayout(isInput ? mPreferredInputChannelLayout : mPreferredOutputChannelLayout, false);
				if(thePrefChannelLayout.IsValid())
				{
					HP_PreferredChannels_ConstructLayoutFromDictionary(thePrefChannelLayout, *theChannelLayout);
				}
			}
			break;
			
		case 'srdd':
			{
				ThrowIf(ioDataSize != GetPropertyDataSize(inAddress, inQualifierDataSize, inQualifierData), CAException(kAudioHardwareBadPropertySizeError), "HP_PreferredChannels::GetPropertyData: wrong data size for 'srdd'");
				AudioChannelLayout* theChannelLayout = static_cast<AudioChannelLayout*>(outData);
				
				//	fetch the default layout from the device
				mDevice->GetDefaultChannelLayout(isInput, *theChannelLayout);
			}
			break;
	};
}

void	HP_PreferredChannels::SetPropertyData(const AudioObjectPropertyAddress& inAddress, UInt32 inQualifierDataSize, const void* inQualifierData, UInt32 inDataSize, const void* inData, const AudioTimeStamp* /*inWhen*/)
{
	CFStringRef thePrefsKey = NULL;
	bool isInput = inAddress.mScope == kAudioDevicePropertyScopeInput;
	UInt32 theTotalNumberChannels = mDevice->GetTotalNumberChannels(isInput);
	switch(inAddress.mSelector)
	{
		case kAudioDevicePropertyPreferredChannelsForStereo:
			{
				ThrowIf(inDataSize != GetPropertyDataSize(inAddress, inQualifierDataSize, inQualifierData), CAException(kAudioHardwareBadPropertySizeError), "HP_PreferredChannels::SetPropertyData: wrong data size for kAudioDevicePropertyPreferredChannelsForStereo");
				const UInt32* theStereoChannels = static_cast<const UInt32*>(inData);
				
				//	create an array to hold the prefs value
				CACFArray thePrefStereoChannels(true);
				
				//	put in the left channel
				thePrefStereoChannels.AppendUInt32(std::min(std::max(theStereoChannels[0], (UInt32)1), theTotalNumberChannels));
				
				//	put in the right channel
				thePrefStereoChannels.AppendUInt32(std::min(std::max(theStereoChannels[1], (UInt32)1), theTotalNumberChannels));
				
				//	set the value in the prefs
				thePrefsKey = (isInput ? mInputStereoPrefsKey : mOutputStereoPrefsKey);
				CACFPreferences::SetValue(thePrefsKey, thePrefStereoChannels.GetCFArray(), false, true, true);
				
				CFArrayRef theStereoChannelsArray = isInput ? mPreferredInputStereoChannels : mPreferredOutputStereoChannels;

				//	if the value changed re-cache it and send notifications
				CFArrayRef array = CACFPreferences::CopyArrayValue(thePrefsKey, false, true);
				//	release the old array
				if (theStereoChannelsArray != NULL)
					CFRelease(theStereoChannelsArray);

				//	save the new one
				if (isInput)
					mPreferredInputStereoChannels = array;
				else
					mPreferredOutputStereoChannels = array;

				//	send property changed
				mDevice->PropertiesChanged(1, &inAddress);

				//	send the notification
				SendChangeNotification(thePrefsKey);
			}
			break;
			
		case kAudioDevicePropertyPreferredChannelLayout:
			{
				ThrowIf(inDataSize != GetPropertyDataSize(inAddress, inQualifierDataSize, inQualifierData), CAException(kAudioHardwareBadPropertySizeError), "HP_PreferredChannels::SetPropertyData: wrong data size for kAudioDevicePropertyPreferredChannelLayout");
				const AudioChannelLayout* theChannelLayout = static_cast<const AudioChannelLayout*>(inData);
				
				//	create a dictionary to hold the prefs value
				CACFDictionary thePrefChannelLayout(true);
				
				//	fill out the dictionary
				HP_PreferredChannels_ConstructDictionaryFromLayout(*theChannelLayout, thePrefChannelLayout);
				
				//	set the value in the prefs
				thePrefsKey = (isInput ? mInputChannelLayoutPrefsKey : mOutputChannelLayoutPrefsKey);
				CACFPreferences::SetValue(thePrefsKey, thePrefChannelLayout.GetDict(), false, true, true);
				
				CFDictionaryRef theChannelLayoutDict = isInput ? mPreferredInputChannelLayout : mPreferredOutputChannelLayout;

				//	if the value changed re-cache it and send notifications
				CFDictionaryRef dictionary = CACFPreferences::CopyDictionaryValue(thePrefsKey, false, true);

				//	release the old dict
				if (theChannelLayoutDict != NULL)
					CFRelease(theChannelLayoutDict);

				//	save the new one
				if (isInput)
					mPreferredInputChannelLayout = dictionary;
				else
					mPreferredOutputChannelLayout = dictionary;

				//	send property changed
				mDevice->PropertiesChanged(1, &inAddress);

				//	send the notification
				SendChangeNotification(thePrefsKey);
			}
			break;
	};
}

void	HP_PreferredChannels::ClearPrefs()
{
	CACFPreferences::DeleteValue(mInputStereoPrefsKey, false, true, true);
	CACFPreferences::DeleteValue(mOutputStereoPrefsKey, false, true, true);
	CACFPreferences::DeleteValue(mInputChannelLayoutPrefsKey, false, true, true);
	CACFPreferences::DeleteValue(mOutputChannelLayoutPrefsKey, false, true, true);
	CACFPreferences::Synchronize(false, true, true);
}

UInt32	HP_PreferredChannels::GetNumberAddressesImplemented() const
{
	return 3;
}

void	HP_PreferredChannels::GetImplementedAddressByIndex(UInt32 inIndex, AudioObjectPropertyAddress& outAddress) const
{
	switch(inIndex)
	{
		case 0:
			outAddress.mSelector = kAudioDevicePropertyPreferredChannelsForStereo;
			break;
			
		case 1:
			outAddress.mSelector = kAudioDevicePropertyPreferredChannelLayout;
			break;
			
		case 2:
			outAddress.mSelector = 'srdd';
			break;
	};
	outAddress.mScope = kAudioObjectPropertyScopeWildcard;
	outAddress.mElement = kAudioObjectPropertyElementWildcard;
}

void	HP_PreferredChannels::SendChangeNotification(CFStringRef inNotificationName) const
{
	CACFPreferences::SendNotification(inNotificationName);
}

void	HP_PreferredChannels::ChangeNotification(CFNotificationCenterRef /*inCenter*/, const void* inToken, CFStringRef inNotificationName, const void* /*inObject*/, CFDictionaryRef /*inUserInfo*/)
{
	try
	{
		if(sTokenMap != NULL)
		{
			HP_PreferredChannels* thePreferredChannelProperty = sTokenMap->GetObject(inToken);
			if(thePreferredChannelProperty != NULL)
			{
				AudioObjectPropertyAddress theAddress;
				theAddress.mElement = kAudioObjectPropertyElementMaster;
				
				//	mark the prefs as dirty
				CACFPreferences::MarkPrefsOutOfDate(false, true);
				
				//	figure out what changed
				if(CFStringCompare(inNotificationName, thePreferredChannelProperty->mInputStereoPrefsKey, 0) == kCFCompareEqualTo)
				{
					theAddress.mSelector = kAudioDevicePropertyPreferredChannelsForStereo;
					theAddress.mScope = kAudioDevicePropertyScopeInput;
					
					CFArrayRef array = CACFPreferences::CopyArrayValue(thePreferredChannelProperty->mInputStereoPrefsKey, false, true);
					if (array != NULL)
					{
						if (thePreferredChannelProperty->mPreferredInputStereoChannels != NULL && CFEqual(array, thePreferredChannelProperty->mPreferredInputStereoChannels))
						{
							//	nothing to do, except release array
							CFRelease(array);
						}
						else
						{
							//	re-cache the value
							if (thePreferredChannelProperty->mPreferredInputStereoChannels != NULL)
								CFRelease(thePreferredChannelProperty->mPreferredInputStereoChannels);
							thePreferredChannelProperty->mPreferredInputStereoChannels = array;

							thePreferredChannelProperty->mDevice->PropertiesChanged(1, &theAddress);
							
						}
					}
					else if (thePreferredChannelProperty->mPreferredInputStereoChannels != NULL)
					{
						CFRelease(thePreferredChannelProperty->mPreferredInputStereoChannels);
						thePreferredChannelProperty->mPreferredInputStereoChannels = NULL;

						thePreferredChannelProperty->mDevice->PropertiesChanged(1, &theAddress);
					}
				}
				else if(CFStringCompare(inNotificationName, thePreferredChannelProperty->mOutputStereoPrefsKey, 0) == kCFCompareEqualTo)
				{
					theAddress.mSelector = kAudioDevicePropertyPreferredChannelsForStereo;
					theAddress.mScope = kAudioDevicePropertyScopeOutput;
					
					CFArrayRef array = CACFPreferences::CopyArrayValue(thePreferredChannelProperty->mOutputStereoPrefsKey, false, true);
					if (array != NULL)
					{
						if (thePreferredChannelProperty->mPreferredOutputStereoChannels != NULL && CFEqual(array, thePreferredChannelProperty->mPreferredOutputStereoChannels))
						{
							//	nothing to do, except release array
							CFRelease(array);
						}
						else
						{
							//	re-cache the value
							if (thePreferredChannelProperty->mPreferredOutputStereoChannels != NULL)
								CFRelease(thePreferredChannelProperty->mPreferredOutputStereoChannels);
							thePreferredChannelProperty->mPreferredOutputStereoChannels = array;

							thePreferredChannelProperty->mDevice->PropertiesChanged(1, &theAddress);
							
						}
					}
					else if (thePreferredChannelProperty->mPreferredOutputStereoChannels != NULL)
					{
						CFRelease(thePreferredChannelProperty->mPreferredOutputStereoChannels);
						thePreferredChannelProperty->mPreferredOutputStereoChannels = NULL;

						thePreferredChannelProperty->mDevice->PropertiesChanged(1, &theAddress);
					}
				}
				else if(CFStringCompare(inNotificationName, thePreferredChannelProperty->mInputChannelLayoutPrefsKey, 0) == kCFCompareEqualTo)
				{
					theAddress.mSelector = kAudioDevicePropertyPreferredChannelLayout;
					theAddress.mScope = kAudioDevicePropertyScopeInput;
					
					CFDictionaryRef dictionary = CACFPreferences::CopyDictionaryValue(thePreferredChannelProperty->mInputChannelLayoutPrefsKey, false, true);
					if (dictionary != NULL)
					{
						if (thePreferredChannelProperty->mPreferredInputChannelLayout != NULL && CFEqual(dictionary, thePreferredChannelProperty->mPreferredInputChannelLayout))
						{
							//	nothing to do, except release dictionary
							CFRelease(dictionary);
						}
						else
						{
							//	re-cache the value
							if (thePreferredChannelProperty->mPreferredInputChannelLayout != NULL)
								CFRelease(thePreferredChannelProperty->mPreferredInputChannelLayout);
							thePreferredChannelProperty->mPreferredInputChannelLayout = dictionary;

							thePreferredChannelProperty->mDevice->PropertiesChanged(1, &theAddress);
							
						}
					}
					else if (thePreferredChannelProperty->mPreferredInputChannelLayout != NULL)
					{
						CFRelease(thePreferredChannelProperty->mPreferredInputChannelLayout);
						thePreferredChannelProperty->mPreferredInputChannelLayout = NULL;

						thePreferredChannelProperty->mDevice->PropertiesChanged(1, &theAddress);
					}
				}
				else if(CFStringCompare(inNotificationName, thePreferredChannelProperty->mOutputChannelLayoutPrefsKey, 0) == kCFCompareEqualTo)
				{
					theAddress.mSelector = kAudioDevicePropertyPreferredChannelLayout;
					theAddress.mScope = kAudioDevicePropertyScopeOutput;

					CFDictionaryRef dictionary = CACFPreferences::CopyDictionaryValue(thePreferredChannelProperty->mOutputChannelLayoutPrefsKey, false, true);
					if (dictionary != NULL)
					{
						if (thePreferredChannelProperty->mPreferredOutputChannelLayout != NULL && CFEqual(dictionary, thePreferredChannelProperty->mPreferredOutputChannelLayout))
						{
							//	nothing to do, except release dictionary
							CFRelease(dictionary);
						}
						else
						{
							//	re-cache the value
							if (thePreferredChannelProperty->mPreferredOutputChannelLayout != NULL)
								CFRelease(thePreferredChannelProperty->mPreferredOutputChannelLayout);
							thePreferredChannelProperty->mPreferredOutputChannelLayout = dictionary;

							thePreferredChannelProperty->mDevice->PropertiesChanged(1, &theAddress);
							
						}
					}
					else if (thePreferredChannelProperty->mPreferredOutputChannelLayout != NULL)
					{
						CFRelease(thePreferredChannelProperty->mPreferredOutputChannelLayout);
						thePreferredChannelProperty->mPreferredOutputChannelLayout = NULL;

						thePreferredChannelProperty->mDevice->PropertiesChanged(1, &theAddress);
					}
				}
			}
		}
	}
	catch(...)
	{
	}
}

CATokenMap<HP_PreferredChannels>*	HP_PreferredChannels::sTokenMap = NULL;

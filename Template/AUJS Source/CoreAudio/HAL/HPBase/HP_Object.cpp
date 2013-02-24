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
#include "HP_Object.h"

//	Internal Includes
#include "HP_Device.h"
#include "HP_HardwarePlugIn.h"
#include "HP_Property.h"
#include "HP_Stream.h"

//	PublicUtility Includes
#include "CADebugMacros.h"
#include "CAException.h"
#include "CAMutex.h"
#include "CATokenMap.h"

//	Standard Library Includes
#include <map>

//==================================================================================================
//	HP_Object
//==================================================================================================

static CATokenMap<HP_Object>*	sHP_ObjectIDMap = NULL;

static inline UInt32	HP_Object_MapObject(UInt32 inID, HP_Object* inObject)
{
	if(sHP_ObjectIDMap == NULL)
	{
		sHP_ObjectIDMap = new CATokenMap<HP_Object>();
	}
	
	sHP_ObjectIDMap->AddMapping(inID, inObject);
	
	return inID;
}

static inline void  HP_Object_UnmapObject(HP_Object* inObject)
{
	if(sHP_ObjectIDMap != NULL)
	{
		UInt32 theID = inObject->GetObjectID();
		sHP_ObjectIDMap->RemoveMapping(theID, inObject);
	}
}

static inline HP_Object* HP_Object_GetObjectForID(UInt32 inID)
{
	HP_Object* theAnswer = NULL;
	
	if(sHP_ObjectIDMap != NULL)
	{
		theAnswer = sHP_ObjectIDMap->GetObject(inID);
	}
	
	return theAnswer;
}

HP_Object::HP_Object(AudioObjectID inObjectID, AudioClassID inClassID, HP_HardwarePlugIn* inPlugIn)
:
	mObjectID(inObjectID),
	mClassID(inClassID),
	mPlugIn(inPlugIn),
	mPropertyMap()
{
	if(inObjectID != kAudioObjectUnknown)
	{
		HP_Object_MapObject(inObjectID, this);
	}
}

HP_Object::~HP_Object()
{
	HP_Object_UnmapObject(this);
}

void	HP_Object::Initialize()
{
}

void	HP_Object::Teardown()
{
}

void	HP_Object::SetObjectID(AudioObjectID inObjectID)
{
	HP_Object_UnmapObject(this);
	mObjectID = inObjectID;
	if(inObjectID != kAudioObjectUnknown)
	{
		HP_Object_MapObject(inObjectID, this);
	}
}

bool	HP_Object::IsSubClass(AudioClassID inClassID, AudioClassID inBaseClassID)
{
	bool theAnswer = false;
	
	switch(inBaseClassID)
	{
		case kAudioObjectClassID:
		{
			//  all classes are subclasses of AudioObject
			theAnswer = true;
		}
		break;
		
		case kAudioControlClassID:
		{
			switch(inClassID)
			{
				case kAudioControlClassID:
				case kAudioLevelControlClassID:
				case kAudioBooleanControlClassID:
				case kAudioSelectorControlClassID:
				case kAudioStereoPanControlClassID:
				case kAudioVolumeControlClassID:
				case kAudioLFEVolumeControlClassID:
				case kAudioBootChimeVolumeControlClassID:
				case kAudioMuteControlClassID:
				case kAudioSoloControlClassID:
				case kAudioJackControlClassID:
				case kAudioLFEMuteControlClassID:
				case kAudioISubOwnerControlClassID:
				case kAudioDataSourceControlClassID:
				case kAudioDataDestinationControlClassID:
				case kAudioClockSourceControlClassID:
				case kAudioLineLevelControlClassID:
				{
					theAnswer = true;
				}
				break;
			};
		}
		break;
		
		case kAudioLevelControlClassID:
		{
			switch(inClassID)
			{
				case kAudioLevelControlClassID:
				case kAudioVolumeControlClassID:
				case kAudioLFEVolumeControlClassID:
				case kAudioBootChimeVolumeControlClassID:
				{
					theAnswer = true;
				}
				break;
			};
		}
		break;
		
		case kAudioBooleanControlClassID:
		{
			switch(inClassID)
			{
				case kAudioBooleanControlClassID:
				case kAudioMuteControlClassID:
				case kAudioSoloControlClassID:
				case kAudioJackControlClassID:
				case kAudioLFEMuteControlClassID:
				case kAudioISubOwnerControlClassID:
				{
					theAnswer = true;
				}
				break;
			};
		}
		break;
		
		case kAudioSelectorControlClassID:
		{
			switch(inClassID)
			{
				case kAudioSelectorControlClassID:
				case kAudioDataSourceControlClassID:
				case kAudioDataDestinationControlClassID:
				case kAudioClockSourceControlClassID:
				case kAudioLineLevelControlClassID:
				{
					theAnswer = true;
				}
				break;
			};
		}
		break;
		
		case kAudioDeviceClassID:
		{
			switch(inClassID)
			{
				case kAudioDeviceClassID:
				case kAudioAggregateDeviceClassID:
				{
					theAnswer = true;
				}
				break;
			};
		}
		break;
		
		//  leaf classes
		case kAudioStereoPanControlClassID:
		case kAudioVolumeControlClassID:
		case kAudioLFEVolumeControlClassID:
		case kAudioBootChimeVolumeControlClassID:
		case kAudioMuteControlClassID:
		case kAudioSoloControlClassID:
		case kAudioJackControlClassID:
		case kAudioLFEMuteControlClassID:
		case kAudioISubOwnerControlClassID:
		case kAudioDataSourceControlClassID:
		case kAudioDataDestinationControlClassID:
		case kAudioClockSourceControlClassID:
		case kAudioLineLevelControlClassID:
		case kAudioSystemObjectClassID:
		case kAudioPlugInClassID:
		case kAudioStreamClassID:
		case kAudioAggregateDeviceClassID:
		case kAudioSubDeviceClassID:
		{
			theAnswer = inClassID == inBaseClassID;
		}
		break;
		
		default:
		{
			#if CoreAudio_Debug
				char theClassIDString[5] = CA4CCToCString(inBaseClassID);
				DebugMessageN1("HP_Object::IsSubClass: unknown base class '%s'", theClassIDString);
			#endif
			theAnswer = inClassID == inBaseClassID;
		}
		break;
		
	};
	
	return theAnswer;
}

CAMutex*	HP_Object::GetObjectStateMutex()
{
	//	most object don't have a state mutex
	return NULL;
}

void	HP_Object::Show() const
{
	//  the default implementation is to print the object's ID, class ID and name (if it has one)
	
	//  make a string for the class ID
	char theClassID[] = CA4CCToCString(mClassID);
	
	//  get the object's name
	CAPropertyAddress theAddress(kAudioObjectPropertyName, kAudioObjectPropertyScopeGlobal, kAudioObjectPropertyElementMaster);
	CFStringRef theCFName = NULL;
	UInt32 theSize = SizeOf32(CFStringRef);
	try
	{
		GetPropertyData(theAddress, 0, NULL, theSize, &theCFName);
	}
	catch(...)
	{
		theCFName = NULL;
	}
	
	//  make a C string out of the name
	char theName[256];
	theName[0] = 0;
	if(theCFName != NULL)
	{
		CFIndex theLength = 0;
		CFRange theRange = { 0, CFStringGetLength(theCFName) };
		CFStringGetBytes(theCFName, theRange, kCFStringEncodingUTF8, 0, false, (UInt8*)theName, 255, &theLength);
		theName[theLength] = 0;
		CFRelease(theCFName);
	}
	
	//  print the information to the standard output
	printf("AudioObjectID:\t\t0x%lX\n\tAudioClassID:\t'%s'\n\tName:\t\t\t%s\n", (long unsigned int)mObjectID, theClassID, theName);
}

HP_Object*	HP_Object::GetObjectByID(AudioObjectID inObjectID)
{
	return HP_Object_GetObjectForID(inObjectID);
}

HP_Device*	HP_Object::GetDeviceByID(AudioObjectID inObjectID)
{
	HP_Device* theAnswer = NULL;
	HP_Object* theObject = HP_Object_GetObjectForID(inObjectID);
	if((theObject != NULL) && IsSubClass(theObject->GetClassID(), kAudioDeviceClassID))
	{
		theAnswer = static_cast<HP_Device*>(theObject);
	}
	return theAnswer;
}

HP_Stream*	HP_Object::GetStreamByID(AudioObjectID inObjectID)
{
	HP_Stream* theAnswer = NULL;
	HP_Object* theObject = HP_Object_GetObjectForID(inObjectID);
	if((theObject != NULL) && IsSubClass(theObject->GetClassID(), kAudioStreamClassID))
	{
		theAnswer = static_cast<HP_Stream*>(theObject);
	}
	return theAnswer;
}

typedef std::map<AudioObjectID, CAMutex*>	ObjectStateMutexMap;
static ObjectStateMutexMap*	sHP_ObjectStateMutexMap = NULL;

CAMutex*	HP_Object::GetObjectStateMutexByID(AudioObjectID inObjectID)
{
	CAMutex* theAnswer = NULL;
	
	if(sHP_ObjectStateMutexMap != NULL)
	{
		ObjectStateMutexMap::iterator theIterator = sHP_ObjectStateMutexMap->find(inObjectID);
		if(theIterator != sHP_ObjectStateMutexMap->end())
		{
			theAnswer = theIterator->second;
		}
	}
	
	return theAnswer;
}

void	HP_Object::SetObjectStateMutexForID(AudioObjectID inObjectID, CAMutex* inMutex)
{
	if(sHP_ObjectStateMutexMap == NULL)
	{
		sHP_ObjectStateMutexMap = new ObjectStateMutexMap;
	}
	
	if(sHP_ObjectStateMutexMap != NULL)
	{
		ObjectStateMutexMap::iterator theIterator = sHP_ObjectStateMutexMap->find(inObjectID);
		if(theIterator != sHP_ObjectStateMutexMap->end())
		{
			if(inMutex != NULL)
			{
				theIterator->second = inMutex;
			}
			else
			{
				sHP_ObjectStateMutexMap->erase(theIterator);
			}
		}
		else
		{
			if(inMutex != NULL)
			{
				sHP_ObjectStateMutexMap->insert(ObjectStateMutexMap::value_type(inObjectID, inMutex));
			}
		}
	}
}

bool	HP_Object::HasProperty(const AudioObjectPropertyAddress& inAddress) const
{
	bool theAnswer = false;
	
	switch(inAddress.mSelector)
	{
		case kAudioObjectPropertyListenerAdded:
		case kAudioObjectPropertyListenerRemoved:
			theAnswer = true;
			break;
		
		default:
		{
			HP_Property* theProperty = FindActivePropertyByAddress(inAddress);
			if(theProperty != NULL)
			{
				theAnswer = true;
			}
		}
		break;
	};
	
	return theAnswer;
}

bool	HP_Object::IsPropertySettable(const AudioObjectPropertyAddress& inAddress) const
{
	bool theAnswer = false;
	
	switch(inAddress.mSelector)
	{
		case kAudioObjectPropertyListenerAdded:
		case kAudioObjectPropertyListenerRemoved:
			theAnswer = true;
			break;
			
		default:
		{
			HP_Property* theProperty = FindActivePropertyByAddress(inAddress);
			if(theProperty != NULL)
			{
				theAnswer = theProperty->IsPropertySettable(inAddress);
			}
			else
			{
				DebugMessage("HP_Object::IsPropertySettable: unknown property");
				Throw(CAException(kAudioHardwareUnknownPropertyError));
			}
		}
		break;
	};
	
	return theAnswer;
}

UInt32	HP_Object::GetPropertyDataSize(const AudioObjectPropertyAddress& inAddress, UInt32 inQualifierDataSize, const void* inQualifierData) const
{
	UInt32 theAnswer = 0;
	
	switch(inAddress.mSelector)
	{
		case kAudioObjectPropertyListenerAdded:
		case kAudioObjectPropertyListenerRemoved:
			theAnswer = SizeOf32(AudioObjectPropertyAddress);
			break;
			
		default:
		{
			HP_Property* theProperty = FindActivePropertyByAddress(inAddress);
			if(theProperty != NULL)
			{
				theAnswer = theProperty->GetPropertyDataSize(inAddress, inQualifierDataSize, inQualifierData);
			}
			else
			{
				DebugMessage("HP_Object::GetPropertyDataSize: unknown property");
				Throw(CAException(kAudioHardwareUnknownPropertyError));
			}
		}
		break;
	};
	
	return theAnswer;
}

void	HP_Object::GetPropertyData(const AudioObjectPropertyAddress& inAddress, UInt32 inQualifierDataSize, const void* inQualifierData, UInt32& ioDataSize, void* outData) const
{
	switch(inAddress.mSelector)
	{
		case kAudioObjectPropertyListenerAdded:
		case kAudioObjectPropertyListenerRemoved:
			ThrowIf(ioDataSize != sizeof(AudioObjectPropertyAddress), CAException(kAudioHardwareBadPropertySizeError), "IOA_Device::GetPropertyData: wrong data size for kAudioObjectPropertyListenerAdded/kAudioObjectPropertyListenerRemoved");
			memset(outData, 0, ioDataSize);
			break;
			
		default:
		{
			HP_Property* theProperty = FindActivePropertyByAddress(inAddress);
			if(theProperty != NULL)
			{
				theProperty->GetPropertyData(inAddress, inQualifierDataSize, inQualifierData, ioDataSize, outData);
			}
			else
			{
#if	CoreAudio_Debug
				char theSelectorString[5] = CA4CCToCString(inAddress.mSelector);
				char theScopeString[5] = CA4CCToCString(inAddress.mScope);
				DebugMessageN3("HP_Object::GetPropertyData: unknown property ('%s', '%s', %lu)", theSelectorString, theScopeString, (long unsigned int)inAddress.mElement);
#endif
				Throw(CAException(kAudioHardwareUnknownPropertyError));
			}
		}
		break;
	};
}

void	HP_Object::SetPropertyData(const AudioObjectPropertyAddress& inAddress, UInt32 inQualifierDataSize, const void* inQualifierData, UInt32 inDataSize, const void* inData, const AudioTimeStamp* inWhen)
{
	ThrowIf(!IsPropertySettable(inAddress), CAException(kAudioHardwareIllegalOperationError), "HP_Object::SetPropertyData: address isn't settable");
	switch(inAddress.mSelector)
	{
		case kAudioObjectPropertyListenerAdded:
			ThrowIf(inDataSize != sizeof(AudioObjectPropertyAddress), CAException(kAudioHardwareBadPropertySizeError), "IOA_Device::SetPropertyData: wrong data size for kAudioObjectPropertyListenerAdded");
			PropertyListenerAdded(*(static_cast<const AudioObjectPropertyAddress*>(inData)));
			break;
		
		case kAudioObjectPropertyListenerRemoved:
			ThrowIf(inDataSize != sizeof(AudioObjectPropertyAddress), CAException(kAudioHardwareBadPropertySizeError), "IOA_Device::SetPropertyData: wrong data size for kAudioObjectPropertyListenerRemoved");
			PropertyListenerRemoved(*(static_cast<const AudioObjectPropertyAddress*>(inData)));
			break;
			
		default:
		{
			HP_Property* theProperty = FindActivePropertyByAddress(inAddress);
			if(theProperty != NULL)
			{
				theProperty->SetPropertyData(inAddress, inQualifierDataSize, inQualifierData, inDataSize, inData, inWhen);
			}
			else
			{
				DebugMessage("HP_Object::SetPropertyData: unknown property");
				Throw(CAException(kAudioHardwareUnknownPropertyError));
			}
		}
		break;
	};
}

#if	(MAC_OS_X_VERSION_MAX_ALLOWED < MAC_OS_X_VERSION_10_4)
void	HP_Object::PropertiesChanged(UInt32 /*inNumberAddresses*/, const AudioObjectPropertyAddress /*inAddresses*/[]) const
{
}
#else
void	HP_Object::PropertiesChanged(UInt32 inNumberAddresses, const AudioObjectPropertyAddress inAddresses[]) const
{
	//	note that we need to be sure that the object state mutex is not held while we call the listeners
	bool ownsStateMutex = false;
	CAMutex* theObjectStateMutex = const_cast<HP_Object*>(this)->GetObjectStateMutex();
	if(theObjectStateMutex != NULL)
	{
		ownsStateMutex = theObjectStateMutex->IsOwnedByCurrentThread();
		if(ownsStateMutex)
		{
			theObjectStateMutex->Unlock();
		}
	}
		
	OSStatus theError = AudioObjectPropertiesChanged(mPlugIn->GetInterface(), mObjectID, inNumberAddresses, inAddresses);
	AssertNoError(theError, "HP_Object::PropertiesChanged: got an error calling the listeners");
		
	//	re-lock the mutex
	if((theObjectStateMutex != NULL) && ownsStateMutex)
	{
		theObjectStateMutex->Lock();
	}
}
#endif

void	HP_Object::PropertyListenerAdded(const AudioObjectPropertyAddress& /*inAddress*/)
{
}

void	HP_Object::PropertyListenerRemoved(const AudioObjectPropertyAddress& /*inAddress*/)
{
}

void	HP_Object::AddProperty(HP_Property* inProperty)
{
	//  get the number of addresses implemented by this property object
	UInt32 theNumberAddresses = inProperty->GetNumberAddressesImplemented();
	
	//  iterate across the addresses
	for(UInt32 theIndex = 0; theIndex < theNumberAddresses; ++theIndex)
	{
		//  get the address
		CAPropertyAddress theAddress;
		inProperty->GetImplementedAddressByIndex(theIndex, theAddress);
		
		//  look to see if it has already been spoken for
		PropertyMap::iterator thePropertyMapIterator = FindPropertyByAddress(theAddress);
		ThrowIf(thePropertyMapIterator != mPropertyMap.end(), CAException(kAudioHardwareIllegalOperationError), "HP_Object::AddProperty: redefined address");
		
		//  it isn't, so add it
		PropertyMapItem theItem(theAddress, inProperty);
		mPropertyMap.push_back(theItem);
	}
}

void	HP_Object::RemoveProperty(HP_Property* inProperty)
{
	//  get the number of addresses implemented by this property object
	UInt32 theNumberAddresses = inProperty->GetNumberAddressesImplemented();
	
	//  iterate across the addresses
	for(UInt32 theIndex = 0; theIndex < theNumberAddresses; ++theIndex)
	{
		//  get the address
		CAPropertyAddress theAddress;
		inProperty->GetImplementedAddressByIndex(theIndex, theAddress);
		
		//  look for it in the property map
		PropertyMap::iterator thePropertyMapIterator = FindPropertyByAddress(theAddress);
		if(thePropertyMapIterator != mPropertyMap.end())
		{
			//  we found it, so get rid of it
			mPropertyMap.erase(thePropertyMapIterator);
		}
	}
}

HP_Object::PropertyMap::iterator	HP_Object::FindPropertyByAddress(const AudioObjectPropertyAddress& inAddress)
{
	PropertyMap::iterator theAnswer = mPropertyMap.end();
	
	//  iterate through the property map
	PropertyMap::iterator thePropertyMapIterator = mPropertyMap.begin();
	while((theAnswer == mPropertyMap.end()) && (thePropertyMapIterator != mPropertyMap.end()))
	{
		//  check to see if the addresses match
		if(CAPropertyAddress::IsCongruentAddress(thePropertyMapIterator->first, inAddress))
		{
			//  they do
			theAnswer = thePropertyMapIterator;
		}
		
		std::advance(thePropertyMapIterator, 1);
	}
	
	return theAnswer;
}

HP_Property*	HP_Object::FindActivePropertyByAddress(const AudioObjectPropertyAddress& inAddress) const
{
	HP_Property* theAnswer = NULL;
	
	//  iterate through the property map
	PropertyMap::const_iterator thePropertyMapIterator = mPropertyMap.begin();
	while((theAnswer == NULL) && (thePropertyMapIterator != mPropertyMap.end()))
	{
		//  check to see if the addresses match
		if(CAPropertyAddress::IsCongruentAddress(thePropertyMapIterator->first, inAddress))
		{
			//  they do, so make sure that what we found is active
			if(thePropertyMapIterator->second->IsActive(inAddress))
			{
				theAnswer = thePropertyMapIterator->second;
			}
		}
		
		std::advance(thePropertyMapIterator, 1);
	}
	
	return theAnswer;
}

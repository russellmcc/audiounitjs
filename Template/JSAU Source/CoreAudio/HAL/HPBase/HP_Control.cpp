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
#include "HP_Control.h"

//	Local Includes
#include "HP_Device.h"
#include "HP_Stream.h"

//	PublicUtility Includes
#include "CACFString.h"
#include "CADebugMacros.h"
#include "CAException.h"

//==================================================================================================
//	HP_Control
//==================================================================================================

HP_Control::HP_Control(AudioObjectID inObjectID, AudioClassID inClassID, HP_HardwarePlugIn* inPlugIn, HP_Device* inOwningDevice)
:
	HP_Object(inObjectID, inClassID, inPlugIn),
	mOwningDevice(inOwningDevice),
	mMark(false)
{
}

HP_Control::~HP_Control()
{
}

AudioClassID	HP_Control::GetBaseClassID() const
{
	return kAudioControlClassID;
}

CAMutex*	HP_Control::GetObjectStateMutex()
{
	return mOwningDevice->GetObjectStateMutex();
}

void	HP_Control::Show() const
{
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
	
	//	get a string for the scope
	char* theScope = NULL;
	switch(GetPropertyScope())
	{
		case kAudioDevicePropertyScopeInput:
			theScope = (char*)"Input";
			break;
		
		case kAudioDevicePropertyScopeOutput:
			theScope = (char*)"Output";
			break;
		
		case kAudioDevicePropertyScopePlayThrough:
			theScope = (char*)"Play Through";
			break;
		
		case kAudioObjectPropertyScopeGlobal:
		default:
			theScope = (char*)"Global";
			break;
	};
	
	//  print the information to the standard output
	printf("AudioObjectID:\t\t0x%lX\n\tAudioClassID:\t'%s'\n\tName:\t\t\t%s\n\tScope:\t\t\t%s\n\tChannel:\t\t%lu\n", (long unsigned int)mObjectID, theClassID, theName, theScope, (long unsigned int)GetPropertyElement());
}

CFStringRef	HP_Control::CopyName() const
{
	return NULL;
}

CFStringRef	HP_Control::CopyManufacturerName() const
{
	return mOwningDevice->CopyDeviceManufacturerName();
}

void*	HP_Control::GetImplementationObject() const
{
	return NULL;
}

UInt32	HP_Control::GetVariant() const
{
	return mClassID;
}

bool	HP_Control::IsReadOnly() const
{
	return false;
}

bool	HP_Control::HasProperty(const AudioObjectPropertyAddress& inAddress) const
{
	//	initialize the return value
	bool theAnswer = false;
	
	//	initialize some commonly used variables
	CFStringRef theCFString = NULL;
	
	switch(inAddress.mSelector)
	{
		case kAudioObjectPropertyName:
			theCFString = CopyName();
			if(theCFString != NULL)
			{
				theAnswer = true;
				CFRelease(theCFString);
			}
			break;
			
		case kAudioObjectPropertyManufacturer:
			theCFString = CopyManufacturerName();
			if(theCFString != NULL)
			{
				theAnswer = true;
				CFRelease(theCFString);
			}
			break;
		
		case kAudioControlPropertyScope:
			theAnswer = true;
			break;
			
		case kAudioControlPropertyElement:
			theAnswer = true;
			break;
			
		case kAudioControlPropertyVariant:
			theAnswer = true;
			break;
			
		default:
			theAnswer = HP_Object::HasProperty(inAddress);
			break;
	};
	
	return theAnswer;
}

bool	HP_Control::IsPropertySettable(const AudioObjectPropertyAddress& inAddress) const
{
	bool theAnswer = false;
	
	switch(inAddress.mSelector)
	{
		case kAudioObjectPropertyName:
			theAnswer = false;
			break;
			
		case kAudioObjectPropertyManufacturer:
			theAnswer = false;
			break;
			
		case kAudioControlPropertyScope:
			theAnswer = false;
			break;
			
		case kAudioControlPropertyElement:
			theAnswer = false;
			break;
			
		case kAudioControlPropertyVariant:
			theAnswer = false;
			break;
			
		default:
			theAnswer = HP_Object::IsPropertySettable(inAddress);
			break;
	};
	
	return theAnswer;
}

UInt32	HP_Control::GetPropertyDataSize(const AudioObjectPropertyAddress& inAddress, UInt32 inQualifierDataSize, const void* inQualifierData) const
{
	UInt32 theAnswer = 0;
	
	switch(inAddress.mSelector)
	{
		case kAudioObjectPropertyName:
			theAnswer = SizeOf32(CFStringRef);
			break;
			
		case kAudioObjectPropertyManufacturer:
			theAnswer = SizeOf32(CFStringRef);
			break;
			
		case kAudioControlPropertyScope:
			theAnswer = SizeOf32(AudioObjectPropertyScope);
			break;
			
		case kAudioControlPropertyElement:
			theAnswer = SizeOf32(AudioObjectPropertyElement);
			break;
			
		case kAudioControlPropertyVariant:
			theAnswer = SizeOf32(UInt32);
			break;
			
		default:
			theAnswer = HP_Object::GetPropertyDataSize(inAddress, inQualifierDataSize, inQualifierData);
			break;
	};
	
	return theAnswer;
}

void	HP_Control::GetPropertyData(const AudioObjectPropertyAddress& inAddress, UInt32 inQualifierDataSize, const void* inQualifierData, UInt32& ioDataSize, void* outData) const
{
	switch(inAddress.mSelector)
	{
		case kAudioObjectPropertyName:
			ThrowIf(ioDataSize != GetPropertyDataSize(inAddress, inQualifierDataSize, inQualifierData), CAException(kAudioHardwareBadPropertySizeError), "HP_Control::GetPropertyData: wrong data size for kAudioObjectPropertyName");
			*static_cast<CFStringRef*>(outData) = CopyName();
			break;
			
		case kAudioObjectPropertyManufacturer:
			ThrowIf(ioDataSize != GetPropertyDataSize(inAddress, inQualifierDataSize, inQualifierData), CAException(kAudioHardwareBadPropertySizeError), "HP_Control::GetPropertyData: wrong data size for kAudioObjectPropertyManufacturer");
			*static_cast<CFStringRef*>(outData) = CopyManufacturerName();
			break;
			
		case kAudioControlPropertyScope:
			ThrowIf(ioDataSize != GetPropertyDataSize(inAddress, inQualifierDataSize, inQualifierData), CAException(kAudioHardwareBadPropertySizeError), "HP_Control::GetPropertyData: wrong data size for kAudioControlPropertyScope");
			*static_cast<AudioObjectPropertyScope*>(outData) = GetPropertyScope();
			break;
			
		case kAudioControlPropertyElement:
			ThrowIf(ioDataSize != GetPropertyDataSize(inAddress, inQualifierDataSize, inQualifierData), CAException(kAudioHardwareBadPropertySizeError), "HP_Control::GetPropertyData: wrong data size for kAudioControlPropertyElement");
			*static_cast<AudioObjectPropertyElement*>(outData) = GetPropertyElement();
			break;
			
		case kAudioControlPropertyVariant:
			ThrowIf(ioDataSize != GetPropertyDataSize(inAddress, inQualifierDataSize, inQualifierData), CAException(kAudioHardwareBadPropertySizeError), "HP_Control::GetPropertyData: wrong data size for kAudioControlPropertyVariant");
			*static_cast<UInt32*>(outData) = GetVariant();
			break;
			
		default:
			HP_Object::GetPropertyData(inAddress, inQualifierDataSize, inQualifierData, ioDataSize, outData);
			break;
	};
}

void	HP_Control::SetPropertyData(const AudioObjectPropertyAddress& inAddress, UInt32 inQualifierDataSize, const void* inQualifierData, UInt32 inDataSize, const void* inData, const AudioTimeStamp* inWhen)
{
	switch(inAddress.mSelector)
	{
		default:
			HP_Object::SetPropertyData(inAddress, inQualifierDataSize, inQualifierData, inDataSize, inData, inWhen);
			break;
	};
}

void	HP_Control::ValueChanged()
{
	//	get the properties that have changed
	CAPropertyAddressList theDeviceNotifications;
	GetValueChangedDeviceNotifications(theDeviceNotifications);
	
	//	send the notifications
	SendNotifications(theDeviceNotifications);
}

void	HP_Control::RangeChanged()
{
	//	get the properties that have changed
	CAPropertyAddressList theDeviceNotifications;
	GetRangeChangedDeviceNotifications(theDeviceNotifications);
	
	//	send the notifications
	SendNotifications(theDeviceNotifications);
}

void	HP_Control::GetValueChangedDeviceNotifications(CAPropertyAddressList& outDeviceNotifications)
{
	//	clear out the list
	outDeviceNotifications.EraseAllItems();
	
	//	get the primary value changed address and add it to the list
	CAPropertyAddress theChangedAddress(mOwningDevice->GetPrimaryValueChangedPropertySelectorForControl(this), mOwningDevice->GetPrimaryValueChangedPropertyScopeForControl(this), GetPropertyElement());
	if(theChangedAddress.mSelector != 0)
	{
		outDeviceNotifications.AppendUniqueItem(theChangedAddress);
	}
	
	//	get the secondary value changed address and add it to the list
	theChangedAddress.mSelector = mOwningDevice->GetSecondaryValueChangedPropertySelectorForControl(this);
	theChangedAddress.mScope = mOwningDevice->GetSecondaryValueChangedPropertyScopeForControl(this);
	if(theChangedAddress.mSelector != 0)
	{
		outDeviceNotifications.AppendUniqueItem(theChangedAddress);
	}
	
	//	get the third value changed address and add it to the list
	theChangedAddress.mSelector = mOwningDevice->GetThirdValueChangedPropertySelectorForControl(this);
	theChangedAddress.mScope = mOwningDevice->GetThirdValueChangedPropertyScopeForControl(this);
	if(theChangedAddress.mSelector != 0)
	{
		outDeviceNotifications.AppendUniqueItem(theChangedAddress);
	}
	
	//	get the fourth value changed address and add it to the list
	theChangedAddress.mSelector = mOwningDevice->GetFourthValueChangedPropertySelectorForControl(this);
	theChangedAddress.mScope = mOwningDevice->GetFourthValueChangedPropertyScopeForControl(this);
	if(theChangedAddress.mSelector != 0)
	{
		outDeviceNotifications.AppendUniqueItem(theChangedAddress);
	}
}

void	HP_Control::GetRangeChangedDeviceNotifications(CAPropertyAddressList& outDeviceNotifications)
{
	//	clear out the list
	outDeviceNotifications.EraseAllItems();
	
	//	get the primary range changed address and add it to the list
	CAPropertyAddress theChangedAddress(mOwningDevice->GetPrimaryRangeChangedPropertySelectorForControl(this), mOwningDevice->GetPrimaryRangeChangedPropertyScopeForControl(this), GetPropertyElement());
	if(theChangedAddress.mSelector != 0)
	{
		outDeviceNotifications.AppendUniqueItem(theChangedAddress);
	}
	
	//	get the secondary range changed address and add it to the list
	theChangedAddress.mSelector = mOwningDevice->GetSecondaryRangeChangedPropertySelectorForControl(this);
	theChangedAddress.mScope = mOwningDevice->GetSecondaryRangeChangedPropertyScopeForControl(this);
	if(theChangedAddress.mSelector != 0)
	{
		outDeviceNotifications.AppendUniqueItem(theChangedAddress);
	}
}

void	HP_Control::SendNotifications(const CAPropertyAddressList& inDeviceNotifications)
{
	//	It is presumed that inDeviceNotifications only contains notifications relating to this
	//	control. If it is otherwise, it will yield unpredictable results.

	UInt32 theNumberAddresses = inDeviceNotifications.GetNumberItems();
	if(theNumberAddresses > 0)
	{
		//	send the device notifications
		mOwningDevice->PropertiesChanged(theNumberAddresses, inDeviceNotifications.GetItems());
		
		//	duplicate the address list so it can be modified
		CAPropertyAddressList theStreamNotifications(inDeviceNotifications);
		AudioObjectPropertyAddress* theStreamNotificationAddresses = theStreamNotifications.GetItems();
			
		//	get the stream associated with this control
		HP_Stream* theStream = mOwningDevice->GetStreamByPropertyAddress(theStreamNotificationAddresses[0], false);
		if(theStream != NULL)
		{
			//	change the device notifications into stream notifications
			for(UInt32 theIndex = 0; theIndex < theNumberAddresses; ++theIndex)
			{
				theStream->ChangeDevicePropertyAddressIntoStreamPropertyAddress(theStreamNotificationAddresses[theIndex]);
			}
			
			//	send the stream notifications
			theStream->PropertiesChanged(theNumberAddresses, theStreamNotificationAddresses);
		}
	}
}

//==================================================================================================
//	HP_LevelControl
//==================================================================================================

HP_LevelControl::HP_LevelControl(AudioObjectID inObjectID, AudioClassID inClassID, HP_HardwarePlugIn* inPlugIn, HP_Device* inOwningDevice)
:
	HP_Control(inObjectID, inClassID, inPlugIn, inOwningDevice)
{
}

HP_LevelControl::~HP_LevelControl()
{
}

AudioClassID	HP_LevelControl::GetBaseClassID() const
{
	return kAudioLevelControlClassID;
}

bool	HP_LevelControl::HasProperty(const AudioObjectPropertyAddress& inAddress) const
{
	//	initialize the return value
	bool theAnswer = false;
	
	switch(inAddress.mSelector)
	{
		case kAudioLevelControlPropertyScalarValue:
			theAnswer = true;
			break;
		
		case kAudioLevelControlPropertyDecibelValue:
			theAnswer = true;
			break;
		
		case kAudioLevelControlPropertyDecibelRange:
			theAnswer = true;
			break;
		
		case kAudioLevelControlPropertyConvertScalarToDecibels:
			theAnswer = true;
			break;
		
		case kAudioLevelControlPropertyConvertDecibelsToScalar:
			theAnswer = true;
			break;
		
		default:
			theAnswer = HP_Control::HasProperty(inAddress);
			break;
	};
	
	return theAnswer;
}

bool	HP_LevelControl::IsPropertySettable(const AudioObjectPropertyAddress& inAddress) const
{
	//	initialize the return value
	bool theAnswer = false;
	
	switch(inAddress.mSelector)
	{
		case kAudioLevelControlPropertyScalarValue:
			theAnswer = !IsReadOnly();
			break;
		
		case kAudioLevelControlPropertyDecibelValue:
			theAnswer = !IsReadOnly();
			break;
		
		case kAudioLevelControlPropertyDecibelRange:
			theAnswer = false;
			break;
		
		case kAudioLevelControlPropertyConvertScalarToDecibels:
			theAnswer = false;
		
		case kAudioLevelControlPropertyConvertDecibelsToScalar:
			theAnswer = false;
			break;
		
		default:
			theAnswer = HP_Control::IsPropertySettable(inAddress);
			break;
	};
	
	return theAnswer;
}

UInt32	HP_LevelControl::GetPropertyDataSize(const AudioObjectPropertyAddress& inAddress, UInt32 inQualifierDataSize, const void* inQualifierData) const
{
	//	initialize the return value
	UInt32 theAnswer = 0;
	
	switch(inAddress.mSelector)
	{
		case kAudioLevelControlPropertyScalarValue:
			theAnswer = SizeOf32(Float32);
			break;
		
		case kAudioLevelControlPropertyDecibelValue:
			theAnswer = SizeOf32(Float32);
			break;
		
		case kAudioLevelControlPropertyDecibelRange:
			theAnswer = SizeOf32(AudioValueRange);
			break;
		
		case kAudioLevelControlPropertyConvertScalarToDecibels:
			theAnswer = SizeOf32(Float32);
			break;
		
		case kAudioLevelControlPropertyConvertDecibelsToScalar:
			theAnswer = SizeOf32(Float32);
			break;
		
		default:
			theAnswer = HP_Control::GetPropertyDataSize(inAddress, inQualifierDataSize, inQualifierData);
			break;
	};
	
	return theAnswer;
}

void	HP_LevelControl::GetPropertyData(const AudioObjectPropertyAddress& inAddress, UInt32 inQualifierDataSize, const void* inQualifierData, UInt32& ioDataSize, void* outData) const
{
	switch(inAddress.mSelector)
	{
		case kAudioLevelControlPropertyScalarValue:
			ThrowIf(ioDataSize != GetPropertyDataSize(inAddress, inQualifierDataSize, inQualifierData), CAException(kAudioHardwareBadPropertySizeError), "HP_LevelControl::GetPropertyData: wrong data size for kAudioLevelControlPropertyScalarValue");
			*static_cast<Float32*>(outData) = GetScalarValue();
			break;
		
		case kAudioLevelControlPropertyDecibelValue:
			ThrowIf(ioDataSize != GetPropertyDataSize(inAddress, inQualifierDataSize, inQualifierData), CAException(kAudioHardwareBadPropertySizeError), "HP_LevelControl::GetPropertyData: wrong data size for kAudioLevelControlPropertyDecibelValue");
			*static_cast<Float32*>(outData) = GetDBValue();
			break;
		
		case kAudioLevelControlPropertyDecibelRange:
			ThrowIf(ioDataSize != GetPropertyDataSize(inAddress, inQualifierDataSize, inQualifierData), CAException(kAudioHardwareBadPropertySizeError), "HP_LevelControl::GetPropertyData: wrong data size for kAudioLevelControlPropertyDecibelRange");
			static_cast<AudioValueRange*>(outData)->mMinimum = GetMinimumDBValue();
			static_cast<AudioValueRange*>(outData)->mMaximum = GetMaximumDBValue();
			break;
		
		case kAudioLevelControlPropertyConvertScalarToDecibels:
			ThrowIf(ioDataSize != GetPropertyDataSize(inAddress, inQualifierDataSize, inQualifierData), CAException(kAudioHardwareBadPropertySizeError), "HP_LevelControl::GetPropertyData: wrong data size for kAudioLevelControlPropertyConvertScalarToDecibels");
			*static_cast<Float32*>(outData) = ConverScalarValueToDBValue(*static_cast<Float32*>(outData));
			break;
		
		case kAudioLevelControlPropertyConvertDecibelsToScalar:
			ThrowIf(ioDataSize != GetPropertyDataSize(inAddress, inQualifierDataSize, inQualifierData), CAException(kAudioHardwareBadPropertySizeError), "HP_LevelControl::GetPropertyData: wrong data size for kAudioLevelControlPropertyConvertDecibelsToScalar");
			*static_cast<Float32*>(outData) = ConverDBValueToScalarValue(*static_cast<Float32*>(outData));
			break;
		
		default:
			HP_Control::GetPropertyData(inAddress, inQualifierDataSize, inQualifierData, ioDataSize, outData);
			break;
	};
}

void	HP_LevelControl::SetPropertyData(const AudioObjectPropertyAddress& inAddress, UInt32 inQualifierDataSize, const void* inQualifierData, UInt32 inDataSize, const void* inData, const AudioTimeStamp* inWhen)
{
	switch(inAddress.mSelector)
	{
		case kAudioLevelControlPropertyScalarValue:
			ThrowIf(inDataSize != GetPropertyDataSize(inAddress, inQualifierDataSize, inQualifierData), CAException(kAudioHardwareBadPropertySizeError), "HP_LevelControl::GetPropertyData: wrong data size for kAudioLevelControlPropertyScalarValue");
			SetScalarValue(*static_cast<const Float32*>(inData));
			break;
		
		case kAudioLevelControlPropertyDecibelValue:
			ThrowIf(inDataSize != GetPropertyDataSize(inAddress, inQualifierDataSize, inQualifierData), CAException(kAudioHardwareBadPropertySizeError), "HP_LevelControl::GetPropertyData: wrong data size for kAudioLevelControlPropertyDecibelValue");
			SetDBValue(*static_cast<const Float32*>(inData));
			break;
		
		default:
			HP_Control::SetPropertyData(inAddress, inQualifierDataSize, inQualifierData, inDataSize, inData, inWhen);
			break;
	};
}

//==================================================================================================
//	HP_BooleanControl
//==================================================================================================

HP_BooleanControl::HP_BooleanControl(AudioObjectID inObjectID, AudioClassID inClassID, HP_HardwarePlugIn* inPlugIn, HP_Device* inOwningDevice)
:
	HP_Control(inObjectID, inClassID, inPlugIn, inOwningDevice)
{
}

HP_BooleanControl::~HP_BooleanControl()
{
}

AudioClassID	HP_BooleanControl::GetBaseClassID() const
{
	return kAudioBooleanControlClassID;
}

bool	HP_BooleanControl::HasProperty(const AudioObjectPropertyAddress& inAddress) const
{
	//	initialize the return value
	bool theAnswer = false;
	
	switch(inAddress.mSelector)
	{
		case kAudioBooleanControlPropertyValue:
			theAnswer = true;
			break;
			
		default:
			theAnswer = HP_Control::HasProperty(inAddress);
			break;
	};
	
	return theAnswer;
}

bool	HP_BooleanControl::IsPropertySettable(const AudioObjectPropertyAddress& inAddress) const
{
	//	initialize the return value
	bool theAnswer = false;
	
	switch(inAddress.mSelector)
	{
		case kAudioBooleanControlPropertyValue:
			theAnswer = !IsReadOnly();
			break;
		
		default:
			theAnswer = HP_Control::IsPropertySettable(inAddress);
			break;
	};
	
	return theAnswer;
}

UInt32	HP_BooleanControl::GetPropertyDataSize(const AudioObjectPropertyAddress& inAddress, UInt32 inQualifierDataSize, const void* inQualifierData) const
{
	//	initialize the return value
	UInt32 theAnswer = 0;
	
	switch(inAddress.mSelector)
	{
		case kAudioBooleanControlPropertyValue:
			theAnswer = SizeOf32(UInt32);
			break;
		
		default:
			theAnswer = HP_Control::GetPropertyDataSize(inAddress, inQualifierDataSize, inQualifierData);
			break;
	};
	
	return theAnswer;
}

void	HP_BooleanControl::GetPropertyData(const AudioObjectPropertyAddress& inAddress, UInt32 inQualifierDataSize, const void* inQualifierData, UInt32& ioDataSize, void* outData) const
{
	switch(inAddress.mSelector)
	{
		case kAudioBooleanControlPropertyValue:
			ThrowIf(ioDataSize != GetPropertyDataSize(inAddress, inQualifierDataSize, inQualifierData), CAException(kAudioHardwareBadPropertySizeError), "HP_BooleanControl::GetPropertyData: wrong data size for kAudioBooleanControlPropertyValue");
			*static_cast<UInt32*>(outData) = GetValue() ? 1 : 0;
			break;
		
		default:
			HP_Control::GetPropertyData(inAddress, inQualifierDataSize, inQualifierData, ioDataSize, outData);
			break;
	};
}

void	HP_BooleanControl::SetPropertyData(const AudioObjectPropertyAddress& inAddress, UInt32 inQualifierDataSize, const void* inQualifierData, UInt32 inDataSize, const void* inData, const AudioTimeStamp* inWhen)
{
	switch(inAddress.mSelector)
	{
		case kAudioBooleanControlPropertyValue:
			ThrowIf(inDataSize != GetPropertyDataSize(inAddress, inQualifierDataSize, inQualifierData), CAException(kAudioHardwareBadPropertySizeError), "HP_BooleanControl::GetPropertyData: wrong data size for kAudioBooleanControlPropertyValue");
			SetValue(*static_cast<const UInt32*>(inData) != 0);
			break;
		
		default:
			HP_Control::SetPropertyData(inAddress, inQualifierDataSize, inQualifierData, inDataSize, inData, inWhen);
			break;
	};
}

//==================================================================================================
//	HP_SelectorControl
//==================================================================================================

HP_SelectorControl::HP_SelectorControl(AudioObjectID inObjectID, AudioClassID inClassID, HP_HardwarePlugIn* inPlugIn, HP_Device* inOwningDevice)
:
	HP_Control(inObjectID, inClassID, inPlugIn, inOwningDevice)
{
}

HP_SelectorControl::~HP_SelectorControl()
{
}

AudioClassID	HP_SelectorControl::GetBaseClassID() const
{
	return kAudioSelectorControlClassID;
}

bool	HP_SelectorControl::HasProperty(const AudioObjectPropertyAddress& inAddress) const
{
	//	initialize the return value
	bool theAnswer = false;
	
	switch(inAddress.mSelector)
	{
		case kAudioSelectorControlPropertyCurrentItem:
			theAnswer = true;
			break;
			
		case kAudioSelectorControlPropertyAvailableItems:
			theAnswer = true;
			break;
			
		case kAudioSelectorControlPropertyItemName:
			theAnswer = true;
			break;
		
		case kAudioClockSourceControlPropertyItemKind:
			theAnswer = IsSubClass(GetClassID(), kAudioClockSourceControlClassID);
			break;
			
		default:
			theAnswer = HP_Control::HasProperty(inAddress);
			break;
	};
	
	return theAnswer;
}

bool	HP_SelectorControl::IsPropertySettable(const AudioObjectPropertyAddress& inAddress) const
{
	//	initialize the return value
	bool theAnswer = false;
	
	switch(inAddress.mSelector)
	{
		case kAudioSelectorControlPropertyCurrentItem:
			theAnswer = !IsReadOnly();
			break;
			
		case kAudioSelectorControlPropertyAvailableItems:
			theAnswer = false;
			break;
			
		case kAudioSelectorControlPropertyItemName:
			theAnswer = false;
			break;
		
		case kAudioClockSourceControlPropertyItemKind:
			theAnswer = false;
			break;
			
		default:
			theAnswer = HP_Control::IsPropertySettable(inAddress);
			break;
	};
	
	return theAnswer;
}

UInt32	HP_SelectorControl::GetPropertyDataSize(const AudioObjectPropertyAddress& inAddress, UInt32 inQualifierDataSize, const void* inQualifierData) const
{
	//	initialize the return value
	UInt32 theAnswer = 0;
	
	switch(inAddress.mSelector)
	{
		case kAudioSelectorControlPropertyCurrentItem:
			theAnswer = SizeOf32(UInt32);
			break;
			
		case kAudioSelectorControlPropertyAvailableItems:
			theAnswer = GetNumberItems() * SizeOf32(UInt32);
			break;
			
		case kAudioSelectorControlPropertyItemName:
			theAnswer = SizeOf32(CFStringRef);
			break;
		
		case kAudioClockSourceControlPropertyItemKind:
			theAnswer = SizeOf32(UInt32);
			break;
			
		default:
			theAnswer = HP_Control::GetPropertyDataSize(inAddress, inQualifierDataSize, inQualifierData);
			break;
	};
	
	return theAnswer;
}

void	HP_SelectorControl::GetPropertyData(const AudioObjectPropertyAddress& inAddress, UInt32 inQualifierDataSize, const void* inQualifierData, UInt32& ioDataSize, void* outData) const
{
	switch(inAddress.mSelector)
	{
		case kAudioSelectorControlPropertyCurrentItem:
			ThrowIf(ioDataSize != GetPropertyDataSize(inAddress, inQualifierDataSize, inQualifierData), CAException(kAudioHardwareBadPropertySizeError), "HP_SelectorControl::GetPropertyData: wrong data size for kAudioSelectorControlPropertyCurrentItem");
			*static_cast<UInt32*>(outData) = GetCurrentItemID();
			break;
		
		case kAudioSelectorControlPropertyAvailableItems:
			{
				UInt32 theNumberItemsToGet = std::min((UInt32)(ioDataSize / SizeOf32(UInt32)), GetNumberItems());
				UInt32* theItemIDs = static_cast<UInt32*>(outData);
				for(UInt32 theIndex = 0; theIndex < theNumberItemsToGet; ++theIndex)
				{
					theItemIDs[theIndex] = GetItemIDForIndex(theIndex);
				}
				ioDataSize = theNumberItemsToGet * SizeOf32(UInt32);
			}
			break;
		
		case kAudioSelectorControlPropertyItemName:
			ThrowIf(ioDataSize != GetPropertyDataSize(inAddress, inQualifierDataSize, inQualifierData), CAException(kAudioHardwareBadPropertySizeError), "HP_SelectorControl::GetPropertyData: wrong data size for kAudioSelectorControlPropertyItemName");
			ThrowIf(inQualifierDataSize != sizeof(UInt32), CAException(kAudioHardwareBadPropertySizeError), "HP_SelectorControl::GetPropertyData: wrong qualifier size for kAudioSelectorControlPropertyItemName");
			*static_cast<CFStringRef*>(outData) = CopyItemNameByID(*static_cast<const UInt32*>(inQualifierData));
			break;
		
		case kAudioClockSourceControlPropertyItemKind:
			ThrowIf(ioDataSize != GetPropertyDataSize(inAddress, inQualifierDataSize, inQualifierData), CAException(kAudioHardwareBadPropertySizeError), "HP_SelectorControl::GetPropertyData: wrong data size for kAudioClockSourceControlPropertyItemKind");
			ThrowIf(inQualifierDataSize != sizeof(UInt32), CAException(kAudioHardwareBadPropertySizeError), "HP_SelectorControl::GetPropertyData: wrong qualifier size for kAudioClockSourceControlPropertyItemKind");
			*static_cast<UInt32*>(outData) = GetItemKindByID(*static_cast<const UInt32*>(inQualifierData));
			break;
			
		default:
			HP_Control::GetPropertyData(inAddress, inQualifierDataSize, inQualifierData, ioDataSize, outData);
			break;
	};
}

void	HP_SelectorControl::SetPropertyData(const AudioObjectPropertyAddress& inAddress, UInt32 inQualifierDataSize, const void* inQualifierData, UInt32 inDataSize, const void* inData, const AudioTimeStamp* inWhen)
{
	switch(inAddress.mSelector)
	{
		case kAudioSelectorControlPropertyCurrentItem:
			ThrowIf(inDataSize != GetPropertyDataSize(inAddress, inQualifierDataSize, inQualifierData), CAException(kAudioHardwareBadPropertySizeError), "HP_SelectorControl::GetPropertyData: wrong data size for kAudioSelectorControlPropertyCurrentItem");
			SetCurrentItemByID(*static_cast<const UInt32*>(inData));
			break;
		
		default:
			HP_Control::SetPropertyData(inAddress, inQualifierDataSize, inQualifierData, inDataSize, inData, inWhen);
			break;
	};
}

//==================================================================================================
//	HP_StereoPanControl
//==================================================================================================

HP_StereoPanControl::HP_StereoPanControl(AudioObjectID inObjectID, AudioClassID inClassID, HP_HardwarePlugIn* inPlugIn, HP_Device* inOwningDevice)
:
	HP_Control(inObjectID, inClassID, inPlugIn, inOwningDevice)
{
}

HP_StereoPanControl::~HP_StereoPanControl()
{
}

AudioClassID	HP_StereoPanControl::GetBaseClassID() const
{
	return kAudioStereoPanControlClassID;
}

bool	HP_StereoPanControl::HasProperty(const AudioObjectPropertyAddress& inAddress) const
{
	//	initialize the return value
	bool theAnswer = false;
	
	switch(inAddress.mSelector)
	{
		case kAudioStereoPanControlPropertyValue:
			theAnswer = true;
			break;
			
		case kAudioStereoPanControlPropertyPanningChannels:
			theAnswer = true;
			break;
			
		default:
			theAnswer = HP_Control::HasProperty(inAddress);
			break;
	};
	
	return theAnswer;
}

bool	HP_StereoPanControl::IsPropertySettable(const AudioObjectPropertyAddress& inAddress) const
{
	//	initialize the return value
	bool theAnswer = false;
	
	switch(inAddress.mSelector)
	{
		case kAudioStereoPanControlPropertyValue:
			theAnswer = !IsReadOnly();
			break;
			
		case kAudioStereoPanControlPropertyPanningChannels:
			theAnswer = false;
			break;
			
		default:
			theAnswer = HP_Control::IsPropertySettable(inAddress);
			break;
	};
	
	return theAnswer;
}

UInt32	HP_StereoPanControl::GetPropertyDataSize(const AudioObjectPropertyAddress& inAddress, UInt32 inQualifierDataSize, const void* inQualifierData) const
{
	//	initialize the return value
	UInt32 theAnswer = 0;
	
	switch(inAddress.mSelector)
	{
		case kAudioStereoPanControlPropertyValue:
			theAnswer = SizeOf32(Float32);
			break;
			
		case kAudioStereoPanControlPropertyPanningChannels:
			theAnswer = 2 * SizeOf32(UInt32);
			break;
			
		default:
			theAnswer = HP_Control::GetPropertyDataSize(inAddress, inQualifierDataSize, inQualifierData);
			break;
	};
	
	return theAnswer;
}

void	HP_StereoPanControl::GetPropertyData(const AudioObjectPropertyAddress& inAddress, UInt32 inQualifierDataSize, const void* inQualifierData, UInt32& ioDataSize, void* outData) const
{
	switch(inAddress.mSelector)
	{
		case kAudioStereoPanControlPropertyValue:
			ThrowIf(ioDataSize != GetPropertyDataSize(inAddress, inQualifierDataSize, inQualifierData), CAException(kAudioHardwareBadPropertySizeError), "HP_StereoPanControl::GetPropertyData: wrong data size for kAudioStereoPanControlPropertyValue");
			*static_cast<Float32*>(outData) = GetValue();
			break;
		
		case kAudioStereoPanControlPropertyPanningChannels:
			ThrowIf(ioDataSize != GetPropertyDataSize(inAddress, inQualifierDataSize, inQualifierData), CAException(kAudioHardwareBadPropertySizeError), "HP_StereoPanControl::GetPropertyData: wrong data size for kAudioStereoPanControlPropertyPanningChannels");
			GetChannels(static_cast<UInt32*>(outData)[0], static_cast<UInt32*>(outData)[1]);
			break;
		
		default:
			HP_Control::GetPropertyData(inAddress, inQualifierDataSize, inQualifierData, ioDataSize, outData);
			break;
	};
}

void	HP_StereoPanControl::SetPropertyData(const AudioObjectPropertyAddress& inAddress, UInt32 inQualifierDataSize, const void* inQualifierData, UInt32 inDataSize, const void* inData, const AudioTimeStamp* inWhen)
{
	switch(inAddress.mSelector)
	{
		case kAudioStereoPanControlPropertyValue:
			ThrowIf(inDataSize != GetPropertyDataSize(inAddress, inQualifierDataSize, inQualifierData), CAException(kAudioHardwareBadPropertySizeError), "HP_StereoPanControl::SetPropertyData: wrong data size for kAudioStereoPanControlPropertyValue");
			SetValue(*static_cast<const Float32*>(inData));
			break;
		
		default:
			HP_Control::SetPropertyData(inAddress, inQualifierDataSize, inQualifierData, inDataSize, inData, inWhen);
			break;
	};
}

//==================================================================================================
//	HP_DeviceControlProperty
//==================================================================================================

HP_DeviceControlProperty::HP_DeviceControlProperty(HP_Device* inDevice)
:
	HP_Property(),
	mDevice(inDevice)
{
}

HP_DeviceControlProperty::~HP_DeviceControlProperty()
{
}

bool	HP_DeviceControlProperty::IsActive(const AudioObjectPropertyAddress& inAddress) const
{
	//	find the control for the address
	HP_Control* theControl = mDevice->GetControlByAddress(inAddress);
	
	//	the property is active if there is a control
	return theControl != NULL;
}

bool	HP_DeviceControlProperty::IsPropertySettable(const AudioObjectPropertyAddress& inAddress) const
{
	bool theAnswer = false;
	
	//	find the control for the address
	HP_Control* theControl = mDevice->GetControlByAddress(inAddress);
	if(theControl != NULL)
	{
		switch(inAddress.mSelector)
		{
			case kAudioDevicePropertyDataSourceNameForIDCFString:
			case kAudioDevicePropertyDataSourceNameForID:
			case kAudioDevicePropertyClockSourceNameForIDCFString:
			case kAudioDevicePropertyClockSourceNameForID:
			case kAudioDevicePropertyClockSourceKindForID:
			case kAudioDevicePropertyPlayThruDestinationNameForIDCFString:
			case kAudioDevicePropertyPlayThruDestinationNameForID:
			case kAudioDevicePropertyChannelNominalLineLevelNameForIDCFString:
			case kAudioDevicePropertyChannelNominalLineLevelNameForID:
				theAnswer = false;
				break;
			
			default:
				{
					CAPropertyAddress theControlAddress(mDevice->ConvertDeviceSelectorToControlSelector(inAddress.mSelector));
					theAnswer = theControl->IsPropertySettable(theControlAddress);
				}
				break;
		};
	}
	
	return theAnswer;
}

UInt32	HP_DeviceControlProperty::GetPropertyDataSize(const AudioObjectPropertyAddress& inAddress, UInt32 inQualifierDataSize, const void* inQualifierData) const
{
	UInt32 theAnswer = 0;
	
	//	find the control for the address
	HP_Control* theControl = mDevice->GetControlByAddress(inAddress);
	if(theControl != NULL)
	{
		switch(inAddress.mSelector)
		{
			case kAudioDevicePropertyDataSourceNameForIDCFString:
			case kAudioDevicePropertyDataSourceNameForID:
			case kAudioDevicePropertyClockSourceNameForIDCFString:
			case kAudioDevicePropertyClockSourceNameForID:
			case kAudioDevicePropertyClockSourceKindForID:
			case kAudioDevicePropertyPlayThruDestinationNameForIDCFString:
			case kAudioDevicePropertyPlayThruDestinationNameForID:
			case kAudioDevicePropertyChannelNominalLineLevelNameForIDCFString:
			case kAudioDevicePropertyChannelNominalLineLevelNameForID:
				theAnswer = SizeOf32(AudioValueTranslation);
				break;
			
			default:
				{
					CAPropertyAddress theControlAddress(mDevice->ConvertDeviceSelectorToControlSelector(inAddress.mSelector));
					theAnswer = theControl->GetPropertyDataSize(theControlAddress, inQualifierDataSize, inQualifierData);
				}
				break;
		};
	}
	
	return theAnswer;
}

void	HP_DeviceControlProperty::GetPropertyData(const AudioObjectPropertyAddress& inAddress, UInt32 inQualifierDataSize, const void* inQualifierData, UInt32& ioDataSize, void* outData) const
{
	//	find the control for the address
	HP_Control* theControl = mDevice->GetControlByAddress(inAddress);
	if(theControl != NULL)
	{
		CAPropertyAddress theControlAddress(mDevice->ConvertDeviceSelectorToControlSelector(inAddress.mSelector));
		AudioValueTranslation* theTranslationData = static_cast<AudioValueTranslation*>(outData);
		UInt32* theItemIDPtr;
		UInt32 theItemNameSize;
		CFStringRef theItemName;
		CFStringRef* theItemNamePtr = NULL;
		UInt32* theItemKindPtr;
		UInt32 theItemKindSize;
		switch(inAddress.mSelector)
		{
			case kAudioDevicePropertyDataSourceNameForIDCFString:
				{
					//	check the data size
					ThrowIf(ioDataSize != GetPropertyDataSize(inAddress, inQualifierDataSize, inQualifierData), CAException(kAudioHardwareBadPropertySizeError), "HP_DeviceControlProperty::GetPropertyData: wrong data size for kAudioDevicePropertyDataSourceNameForIDCFString");
					theTranslationData = static_cast<AudioValueTranslation*>(outData);
					
					//	check the translation data size
					ThrowIf(theTranslationData->mInputDataSize != sizeof(UInt32), CAException(kAudioHardwareBadPropertySizeError), "HP_DeviceControlProperty::GetPropertyData: wrong input data size for kAudioDevicePropertyDataSourceNameForIDCFString");
					ThrowIfNULL(theTranslationData->mInputData, CAException(kAudioHardwareBadPropertySizeError), "HP_DeviceControlProperty::GetPropertyData: wrong input data buffer for kAudioDevicePropertyDataSourceNameForIDCFString");
					ThrowIf(theTranslationData->mOutputDataSize != sizeof(CFStringRef), CAException(kAudioHardwareBadPropertySizeError), "HP_DeviceControlProperty::GetPropertyData: wrong output data size for kAudioDevicePropertyDataSourceNameForIDCFString");
					ThrowIfNULL(theTranslationData->mOutputData, CAException(kAudioHardwareBadPropertySizeError), "HP_DeviceControlProperty::GetPropertyData: wrong output data buffer for kAudioDevicePropertyDataSourceNameForIDCFString");
					
					//	cast the values
					theItemIDPtr = static_cast<UInt32*>(theTranslationData->mInputData);
					theItemNamePtr = static_cast<CFStringRef*>(theTranslationData->mOutputData);
					
					//	get the info from the control
					theItemNameSize = SizeOf32(CFStringRef);
					theControl->GetPropertyData(theControlAddress, SizeOf32(UInt32), theItemIDPtr, theItemNameSize, theItemNamePtr);
				}
				break;
				
			case kAudioDevicePropertyDataSourceNameForID:
				{
					//	check the data size
					ThrowIf(ioDataSize != GetPropertyDataSize(inAddress, inQualifierDataSize, inQualifierData), CAException(kAudioHardwareBadPropertySizeError), "HP_DeviceControlProperty::GetPropertyData: wrong data size for kAudioDevicePropertyDataSourceNameForID");
					theTranslationData = static_cast<AudioValueTranslation*>(outData);
					
					//	check the translation data size
					ThrowIf(theTranslationData->mInputDataSize != sizeof(UInt32), CAException(kAudioHardwareBadPropertySizeError), "HP_DeviceControlProperty::GetPropertyData: wrong input data size for kAudioDevicePropertyDataSourceNameForID");
					ThrowIfNULL(theTranslationData->mInputData, CAException(kAudioHardwareBadPropertySizeError), "HP_DeviceControlProperty::GetPropertyData: wrong input data buffer for kAudioDevicePropertyDataSourceNameForID");
					ThrowIfNULL(theTranslationData->mOutputData, CAException(kAudioHardwareBadPropertySizeError), "HP_DeviceControlProperty::GetPropertyData: wrong output data buffer for kAudioDevicePropertyDataSourceNameForID");
					
					//	cast the values
					theItemIDPtr = static_cast<UInt32*>(theTranslationData->mInputData);
					
					//	get the info from the control
					theItemNameSize = SizeOf32(CFStringRef);
					theControl->GetPropertyData(theControlAddress, SizeOf32(UInt32), theItemIDPtr, theItemNameSize, &theItemName);
					
					//	get the return value
					if(theItemName != NULL)
					{
						CACFString::GetCString(theItemName, static_cast<char*>(theTranslationData->mOutputData), theTranslationData->mOutputDataSize);
					}
				}
				break;
			
			case kAudioDevicePropertyClockSourceNameForIDCFString:
				{
					//	check the data size
					ThrowIf(ioDataSize != GetPropertyDataSize(inAddress, inQualifierDataSize, inQualifierData), CAException(kAudioHardwareBadPropertySizeError), "HP_DeviceControlProperty::GetPropertyData: wrong data size for kAudioDevicePropertyClockSourceNameForIDCFString");
					theTranslationData = static_cast<AudioValueTranslation*>(outData);
					
					//	check the translation data size
					ThrowIf(theTranslationData->mInputDataSize != sizeof(UInt32), CAException(kAudioHardwareBadPropertySizeError), "HP_DeviceControlProperty::GetPropertyData: wrong input data size for kAudioDevicePropertyClockSourceNameForIDCFString");
					ThrowIfNULL(theTranslationData->mInputData, CAException(kAudioHardwareBadPropertySizeError), "HP_DeviceControlProperty::GetPropertyData: wrong input data buffer for kAudioDevicePropertyClockSourceNameForIDCFString");
					ThrowIf(theTranslationData->mOutputDataSize != sizeof(CFStringRef), CAException(kAudioHardwareBadPropertySizeError), "HP_DeviceControlProperty::GetPropertyData: wrong output data size for kAudioDevicePropertyClockSourceNameForIDCFString");
					ThrowIfNULL(theTranslationData->mOutputData, CAException(kAudioHardwareBadPropertySizeError), "HP_DeviceControlProperty::GetPropertyData: wrong output data buffer for kAudioDevicePropertyClockSourceNameForIDCFString");
					
					//	cast the values
					theItemIDPtr = static_cast<UInt32*>(theTranslationData->mInputData);
					theItemNamePtr = static_cast<CFStringRef*>(theTranslationData->mOutputData);
					
					//	get the info from the control
					theItemNameSize = SizeOf32(CFStringRef);
					theControl->GetPropertyData(theControlAddress, SizeOf32(UInt32), theItemIDPtr, theItemNameSize, theItemNamePtr);
				}
				break;
				
			case kAudioDevicePropertyClockSourceNameForID:
				{
					//	check the data size
					ThrowIf(ioDataSize != GetPropertyDataSize(inAddress, inQualifierDataSize, inQualifierData), CAException(kAudioHardwareBadPropertySizeError), "HP_DeviceControlProperty::GetPropertyData: wrong data size for kAudioDevicePropertyClockSourceNameForID");
					theTranslationData = static_cast<AudioValueTranslation*>(outData);
					
					//	check the translation data size
					ThrowIf(theTranslationData->mInputDataSize != sizeof(UInt32), CAException(kAudioHardwareBadPropertySizeError), "HP_DeviceControlProperty::GetPropertyData: wrong input data size for kAudioDevicePropertyClockSourceNameForID");
					ThrowIfNULL(theTranslationData->mInputData, CAException(kAudioHardwareBadPropertySizeError), "HP_DeviceControlProperty::GetPropertyData: wrong input data buffer for kAudioDevicePropertyClockSourceNameForID");
					ThrowIfNULL(theTranslationData->mOutputData, CAException(kAudioHardwareBadPropertySizeError), "HP_DeviceControlProperty::GetPropertyData: wrong output data buffer for kAudioDevicePropertyClockSourceNameForID");
					
					//	cast the values
					theItemIDPtr = static_cast<UInt32*>(theTranslationData->mInputData);
					
					//	get the info from the control
					theItemNameSize = SizeOf32(CFStringRef);
					theControl->GetPropertyData(theControlAddress, SizeOf32(UInt32), theItemIDPtr, theItemNameSize, &theItemName);
					
					//	get the return value
					if(theItemName != NULL)
					{
						CACFString::GetCString(theItemName, static_cast<char*>(theTranslationData->mOutputData), theTranslationData->mOutputDataSize);
					}
				}
				break;
			
			case kAudioDevicePropertyClockSourceKindForID:
				{
					//	check the data size
					ThrowIf(ioDataSize != GetPropertyDataSize(inAddress, inQualifierDataSize, inQualifierData), CAException(kAudioHardwareBadPropertySizeError), "HP_DeviceControlProperty::GetPropertyData: wrong data size for kAudioDevicePropertyClockSourceKindForID");
					theTranslationData = static_cast<AudioValueTranslation*>(outData);
					
					//	check the translation data size
					ThrowIf(theTranslationData->mInputDataSize != sizeof(UInt32), CAException(kAudioHardwareBadPropertySizeError), "HP_DeviceControlProperty::GetPropertyData: wrong input data size for kAudioDevicePropertyClockSourceKindForID");
					ThrowIfNULL(theTranslationData->mInputData, CAException(kAudioHardwareBadPropertySizeError), "HP_DeviceControlProperty::GetPropertyData: wrong input data buffer for kAudioDevicePropertyClockSourceKindForID");
					ThrowIf(theTranslationData->mOutputDataSize != sizeof(UInt32), CAException(kAudioHardwareBadPropertySizeError), "HP_DeviceControlProperty::GetPropertyData: wrong output data size for kAudioDevicePropertyClockSourceKindForID");
					ThrowIfNULL(theTranslationData->mOutputData, CAException(kAudioHardwareBadPropertySizeError), "HP_DeviceControlProperty::GetPropertyData: wrong output data buffer for kAudioDevicePropertyClockSourceKindForID");
					
					//	cast the values
					theItemIDPtr = static_cast<UInt32*>(theTranslationData->mInputData);
					theItemKindPtr = static_cast<UInt32*>(theTranslationData->mOutputData);
					
					//	get the info from the control
					theItemKindSize = SizeOf32(UInt32);
					theControl->GetPropertyData(theControlAddress, SizeOf32(UInt32), theItemIDPtr, theItemKindSize, theItemNamePtr);
				}
				break;
				
			case kAudioDevicePropertyPlayThruDestinationNameForIDCFString:
				{
					//	check the data size
					ThrowIf(ioDataSize != GetPropertyDataSize(inAddress, inQualifierDataSize, inQualifierData), CAException(kAudioHardwareBadPropertySizeError), "HP_DeviceControlProperty::GetPropertyData: wrong data size for kAudioDevicePropertyPlayThruDestinationNameForIDCFString");
					theTranslationData = static_cast<AudioValueTranslation*>(outData);
					
					//	check the translation data size
					ThrowIf(theTranslationData->mInputDataSize != sizeof(UInt32), CAException(kAudioHardwareBadPropertySizeError), "HP_DeviceControlProperty::GetPropertyData: wrong input data size for kAudioDevicePropertyPlayThruDestinationNameForIDCFString");
					ThrowIfNULL(theTranslationData->mInputData, CAException(kAudioHardwareBadPropertySizeError), "HP_DeviceControlProperty::GetPropertyData: wrong input data buffer for kAudioDevicePropertyPlayThruDestinationNameForIDCFString");
					ThrowIf(theTranslationData->mOutputDataSize != sizeof(CFStringRef), CAException(kAudioHardwareBadPropertySizeError), "HP_DeviceControlProperty::GetPropertyData: wrong output data size for kAudioDevicePropertyPlayThruDestinationNameForIDCFString");
					ThrowIfNULL(theTranslationData->mOutputData, CAException(kAudioHardwareBadPropertySizeError), "HP_DeviceControlProperty::GetPropertyData: wrong output data buffer for kAudioDevicePropertyPlayThruDestinationNameForIDCFString");
					
					//	cast the values
					theItemIDPtr = static_cast<UInt32*>(theTranslationData->mInputData);
					theItemNamePtr = static_cast<CFStringRef*>(theTranslationData->mOutputData);
					
					//	get the info from the control
					theItemNameSize = SizeOf32(CFStringRef);
					theControl->GetPropertyData(theControlAddress, SizeOf32(UInt32), theItemIDPtr, theItemNameSize, theItemNamePtr);
				}
				break;
				
			case kAudioDevicePropertyPlayThruDestinationNameForID:
				{
					//	check the data size
					ThrowIf(ioDataSize != GetPropertyDataSize(inAddress, inQualifierDataSize, inQualifierData), CAException(kAudioHardwareBadPropertySizeError), "HP_DeviceControlProperty::GetPropertyData: wrong data size for kAudioDevicePropertyPlayThruDestinationNameForID");
					theTranslationData = static_cast<AudioValueTranslation*>(outData);
					
					//	check the translation data size
					ThrowIf(theTranslationData->mInputDataSize != sizeof(UInt32), CAException(kAudioHardwareBadPropertySizeError), "HP_DeviceControlProperty::GetPropertyData: wrong input data size for kAudioDevicePropertyPlayThruDestinationNameForID");
					ThrowIfNULL(theTranslationData->mInputData, CAException(kAudioHardwareBadPropertySizeError), "HP_DeviceControlProperty::GetPropertyData: wrong input data buffer for kAudioDevicePropertyPlayThruDestinationNameForID");
					ThrowIfNULL(theTranslationData->mOutputData, CAException(kAudioHardwareBadPropertySizeError), "HP_DeviceControlProperty::GetPropertyData: wrong output data buffer for kAudioDevicePropertyPlayThruDestinationNameForID");
					
					//	cast the values
					theItemIDPtr = static_cast<UInt32*>(theTranslationData->mInputData);
					
					//	get the info from the control
					theItemNameSize = SizeOf32(CFStringRef);
					theControl->GetPropertyData(theControlAddress, SizeOf32(UInt32), theItemIDPtr, theItemNameSize, &theItemName);
					
					//	get the return value
					if(theItemName != NULL)
					{
						CACFString::GetCString(theItemName, static_cast<char*>(theTranslationData->mOutputData), theTranslationData->mOutputDataSize);
					}
				}
				break;
			
			case kAudioDevicePropertyChannelNominalLineLevelNameForIDCFString:
				{
					//	check the data size
					ThrowIf(ioDataSize != GetPropertyDataSize(inAddress, inQualifierDataSize, inQualifierData), CAException(kAudioHardwareBadPropertySizeError), "HP_DeviceControlProperty::GetPropertyData: wrong data size for kAudioDevicePropertyChannelNominalLineLevelNameForIDCFString");
					theTranslationData = static_cast<AudioValueTranslation*>(outData);
					
					//	check the translation data size
					ThrowIf(theTranslationData->mInputDataSize != sizeof(UInt32), CAException(kAudioHardwareBadPropertySizeError), "HP_DeviceControlProperty::GetPropertyData: wrong input data size for kAudioDevicePropertyChannelNominalLineLevelNameForIDCFString");
					ThrowIfNULL(theTranslationData->mInputData, CAException(kAudioHardwareBadPropertySizeError), "HP_DeviceControlProperty::GetPropertyData: wrong input data buffer for kAudioDevicePropertyChannelNominalLineLevelNameForIDCFString");
					ThrowIf(theTranslationData->mOutputDataSize != sizeof(CFStringRef), CAException(kAudioHardwareBadPropertySizeError), "HP_DeviceControlProperty::GetPropertyData: wrong output data size for kAudioDevicePropertyChannelNominalLineLevelNameForIDCFString");
					ThrowIfNULL(theTranslationData->mOutputData, CAException(kAudioHardwareBadPropertySizeError), "HP_DeviceControlProperty::GetPropertyData: wrong output data buffer for kAudioDevicePropertyChannelNominalLineLevelNameForIDCFString");
					
					//	cast the values
					theItemIDPtr = static_cast<UInt32*>(theTranslationData->mInputData);
					theItemNamePtr = static_cast<CFStringRef*>(theTranslationData->mOutputData);
					
					//	get the info from the control
					theItemNameSize = SizeOf32(CFStringRef);
					theControl->GetPropertyData(theControlAddress, SizeOf32(UInt32), theItemIDPtr, theItemNameSize, theItemNamePtr);
				}
				break;
				
			case kAudioDevicePropertyChannelNominalLineLevelNameForID:
				{
					//	check the data size
					ThrowIf(ioDataSize != GetPropertyDataSize(inAddress, inQualifierDataSize, inQualifierData), CAException(kAudioHardwareBadPropertySizeError), "HP_DeviceControlProperty::GetPropertyData: wrong data size for kAudioDevicePropertyChannelNominalLineLevelNameForID");
					theTranslationData = static_cast<AudioValueTranslation*>(outData);
					
					//	check the translation data size
					ThrowIf(theTranslationData->mInputDataSize != sizeof(UInt32), CAException(kAudioHardwareBadPropertySizeError), "HP_DeviceControlProperty::GetPropertyData: wrong input data size for kAudioDevicePropertyChannelNominalLineLevelNameForID");
					ThrowIfNULL(theTranslationData->mInputData, CAException(kAudioHardwareBadPropertySizeError), "HP_DeviceControlProperty::GetPropertyData: wrong input data buffer for kAudioDevicePropertyChannelNominalLineLevelNameForID");
					ThrowIfNULL(theTranslationData->mOutputData, CAException(kAudioHardwareBadPropertySizeError), "HP_DeviceControlProperty::GetPropertyData: wrong output data buffer for kAudioDevicePropertyChannelNominalLineLevelNameForID");
					
					//	cast the values
					theItemIDPtr = static_cast<UInt32*>(theTranslationData->mInputData);
					
					//	get the info from the control
					theItemNameSize = SizeOf32(CFStringRef);
					theControl->GetPropertyData(theControlAddress, SizeOf32(UInt32), theItemIDPtr, theItemNameSize, &theItemName);
					
					//	get the return value
					if(theItemName != NULL)
					{
						CACFString::GetCString(theItemName, static_cast<char*>(theTranslationData->mOutputData), theTranslationData->mOutputDataSize);
					}
				}
				break;
			
			default:
				theControl->GetPropertyData(theControlAddress, inQualifierDataSize, inQualifierData, ioDataSize, outData);
				break;
		};
	}
}

void	HP_DeviceControlProperty::SetPropertyData(const AudioObjectPropertyAddress& inAddress, UInt32 inQualifierDataSize, const void* inQualifierData, UInt32 inDataSize, const void* inData, const AudioTimeStamp* inWhen)
{
	//	find the control for the address
	HP_Control* theControl = mDevice->GetControlByAddress(inAddress);
	if(theControl != NULL)
	{
		switch(inAddress.mSelector)
		{
			default:
				{
					CAPropertyAddress theControlAddress(mDevice->ConvertDeviceSelectorToControlSelector(inAddress.mSelector));
					theControl->SetPropertyData(theControlAddress, inQualifierDataSize, inQualifierData, inDataSize, inData, inWhen);
				}
				break;
		};
	}
}

UInt32	HP_DeviceControlProperty::GetNumberAddressesImplemented() const
{
	return 52;
}

void	HP_DeviceControlProperty::GetImplementedAddressByIndex(UInt32 inIndex, AudioObjectPropertyAddress& outAddress) const
{
	switch(inIndex)
	{
		case 0:
			outAddress.mSelector = kAudioDevicePropertyJackIsConnected;
			break;
			
		case 1:
			outAddress.mSelector = kAudioDevicePropertyVolumeScalar;
			break;
		
		case 2:
			outAddress.mSelector = kAudioDevicePropertyVolumeDecibels;
			break;
			
		case 3:
			outAddress.mSelector = kAudioDevicePropertyVolumeRangeDecibels;
			break;
			
		case 4:
			outAddress.mSelector = kAudioDevicePropertyVolumeScalarToDecibels;
			break;
			
		case 5:
			outAddress.mSelector = kAudioDevicePropertyVolumeDecibelsToScalar;
			break;
			
		case 6:
			outAddress.mSelector = 'vctf';	//	kAudioDevicePropertyVolumeDecibelsToScalarTransferFunction
			break;
		
		case 7:
			outAddress.mSelector = kAudioDevicePropertyStereoPan;
			break;
			
		case 8:
			outAddress.mSelector = kAudioDevicePropertyStereoPanChannels;
			break;
			
		case 9:
			outAddress.mSelector = kAudioDevicePropertyMute;
			break;
			
		case 10:
			outAddress.mSelector = kAudioDevicePropertySolo;
			break;
			
		case 11:
			outAddress.mSelector = kAudioDevicePropertyDataSource;
			break;
			
		case 12:
			outAddress.mSelector = kAudioDevicePropertyDataSources;
			break;
			
		case 13:
			outAddress.mSelector = kAudioDevicePropertyDataSourceNameForIDCFString;
			break;
		
		case 14:
			outAddress.mSelector = kAudioDevicePropertyDataSourceNameForID;
			break;
		
		case 15:
			outAddress.mSelector = kAudioDevicePropertyClockSource;
			break;
		
		case 16:
			outAddress.mSelector = kAudioDevicePropertyClockSources;
			break;
		
		case 17:
			outAddress.mSelector = kAudioDevicePropertyClockSourceNameForIDCFString;
			break;
		
		case 18:
			outAddress.mSelector = kAudioDevicePropertyClockSourceNameForID;
			break;
		
		case 19:
			outAddress.mSelector = kAudioDevicePropertyClockSourceKindForID;
			break;
		
		case 20:
			outAddress.mSelector = kAudioDevicePropertyPlayThru;
			break;
		
		case 21:
			outAddress.mSelector = kAudioDevicePropertyPlayThruSolo;
			break;
		
		case 22:
			outAddress.mSelector = kAudioDevicePropertyPlayThruVolumeScalar;
			break;
		
		case 23:
			outAddress.mSelector = kAudioDevicePropertyPlayThruVolumeDecibels;
			break;
		
		case 24:
			outAddress.mSelector = kAudioDevicePropertyPlayThruVolumeRangeDecibels;
			break;
		
		case 25:
			outAddress.mSelector = kAudioDevicePropertyPlayThruVolumeScalarToDecibels;
			break;
		
		case 26:
			outAddress.mSelector = kAudioDevicePropertyPlayThruVolumeDecibelsToScalar;
			break;
		
		case 27:
			outAddress.mSelector = 'mvtf';	//	kAudioDevicePropertyPlayThruVolumeDecibelsToScalarTransferFunction
			break;
		
		case 28:
			outAddress.mSelector = kAudioDevicePropertyPlayThruStereoPan;
			break;
		
		case 29:
			outAddress.mSelector = kAudioDevicePropertyPlayThruStereoPanChannels;
			break;
		
		case 30:
			outAddress.mSelector = kAudioDevicePropertyPlayThruDestination;
			break;
		
		case 31:
			outAddress.mSelector = kAudioDevicePropertyPlayThruDestinations;
			break;
		
		case 32:
			outAddress.mSelector = kAudioDevicePropertyPlayThruDestinationNameForIDCFString;
			break;
		
		case 33:
			outAddress.mSelector = kAudioDevicePropertyPlayThruDestinationNameForID;
			break;
		
		case 34:
			outAddress.mSelector = kAudioDevicePropertyChannelNominalLineLevel;
			break;
		
		case 35:
			outAddress.mSelector = kAudioDevicePropertyChannelNominalLineLevels;
			break;
		
		case 36:
			outAddress.mSelector = kAudioDevicePropertyChannelNominalLineLevelNameForIDCFString;
			break;
		
		case 37:
			outAddress.mSelector = kAudioDevicePropertyChannelNominalLineLevelNameForID;
			break;
		
		case 38:
			outAddress.mSelector = kAudioDevicePropertyDriverShouldOwniSub;
			break;
		
		case 39:
			outAddress.mSelector = kAudioDevicePropertySubVolumeScalar;
			break;
		
		case 40:
			outAddress.mSelector = kAudioDevicePropertySubVolumeDecibels;
			break;
		
		case 41:
			outAddress.mSelector = kAudioDevicePropertySubVolumeRangeDecibels;
			break;
		
		case 42:
			outAddress.mSelector = kAudioDevicePropertySubVolumeScalarToDecibels;
			break;
		
		case 43:
			outAddress.mSelector = kAudioDevicePropertySubVolumeDecibelsToScalar;
			break;
		
		case 44:
			outAddress.mSelector = 'svtf';	//	kAudioDevicePropertySubVolumeDecibelsToScalarTransferFunction
			break;
		
		case 45:
			outAddress.mSelector = kAudioDevicePropertySubMute;
			break;
		
		case 46:
			outAddress.mSelector = kAudioHardwarePropertyBootChimeVolumeScalar;
			break;
		
		case 47:
			outAddress.mSelector = kAudioHardwarePropertyBootChimeVolumeDecibels;
			break;
		
		case 48:
			outAddress.mSelector = kAudioHardwarePropertyBootChimeVolumeRangeDecibels;
			break;
		
		case 49:
			outAddress.mSelector = kAudioHardwarePropertyBootChimeVolumeScalarToDecibels;
			break;
		
		case 50:
			outAddress.mSelector = kAudioHardwarePropertyBootChimeVolumeDecibelsToScalar;
			break;
		
		case 51:
			outAddress.mSelector = 'bvtf';	//	kAudioHardwarePropertyBootChimeVolumeDecibelsToScalarTransferFunction
			break;
	};
	outAddress.mScope = kAudioObjectPropertyScopeWildcard;
	outAddress.mElement = kAudioObjectPropertyElementWildcard;
}

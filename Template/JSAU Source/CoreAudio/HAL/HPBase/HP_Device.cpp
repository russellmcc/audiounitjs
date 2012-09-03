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
#include "HP_Device.h"
#include "HP_DeviceCommands.h"

//  Local Includes
#include "HP_Control.h"
#include "HP_FormatList.h"
#if Use_HAL_Telemetry
	#include "HP_IOCycleTelemetry.h"
#else
	#include "HALdtrace.h"
#endif
#include "HP_IOProcList.h"
#include "HP_HardwarePlugIn.h"
#include "HP_PreferredChannels.h"
#include "HP_Stream.h"

//  PublicUtility Includes
#include "CAAudioBufferList.h"
#include "CAAudioChannelLayout.h"
#include "CAAutoDisposer.h"
#include "CACFString.h"
#include "CADebugMacros.h"
#include "CAException.h"
#if !Use_HAL_Telemetry
	#include "CAGuard.h"
#endif
#include "CAHostTimeBase.h"

//  System Includes
#include <sys/types.h>

//#define Log_BufferSizeChanges	1

//==================================================================================================
//	HP_Device
//==================================================================================================

HP_Device::HP_Device(AudioDeviceID inAudioDeviceID, AudioClassID inClassID, HP_HardwarePlugIn* inPlugIn, UInt32 inIOBufferSetID, bool inUseIOBuffers)
:
	HP_Object(inAudioDeviceID, inClassID, inPlugIn),
	mDeviceStateMutex(NULL),
	mPreferredChannels(NULL),
	mDeviceFormatList(NULL),
	mIOProcList(NULL),
	mIOBufferSetID(inIOBufferSetID),
	mUseIOBuffers(inUseIOBuffers),
	mIOBufferFrameSize(512),
	mIOEngineIsRunning(false),
#if Use_HAL_Telemetry
	mIOCycleTelemetry(NULL),
#endif
	mInputStreamList(),
	mOutputStreamList()
{
}

HP_Device::~HP_Device()
{
}

void	HP_Device::Initialize()
{
	CreateDeviceStateMutex();
	
	HP_Object::Initialize();
	
	CreateIOProcList();
	
#if Use_HAL_Telemetry
	mIOCycleTelemetry = new HP_IOCycleTelemetry(this);
	mIOCycleTelemetry->Initialize(NULL);
#endif	
	mPreferredChannels = new HP_PreferredChannels(this);
	mPreferredChannels->Initialize();
	AddProperty(mPreferredChannels);
	
	mDeviceFormatList = new HP_DeviceFormatList(this);
	mDeviceFormatList->Initialize();
	AddProperty(mDeviceFormatList);
	
	//	grab the name of the device for debugging purposes
	CACFString theDeviceName(CopyDeviceName());
	if(theDeviceName.IsValid())
	{
		UInt32 theStringSize = 255;
		theDeviceName.GetCString(mDebugDeviceName, theStringSize);
	}
	else
	{
		strlcpy(mDebugDeviceName, "Unknown", 256);
	}
}

void	HP_Device::Teardown()
{
#if Use_HAL_Telemetry
	mIOCycleTelemetry->SetIsCapturing(false);
#endif	
	RemoveProperty(mDeviceFormatList);
	mDeviceFormatList->Teardown();
	delete mDeviceFormatList;
	mDeviceFormatList = NULL;
	
	RemoveProperty(mPreferredChannels);
	mPreferredChannels->Teardown();
	delete mPreferredChannels;
	mPreferredChannels = NULL;
	
#if Use_HAL_Telemetry
	mIOCycleTelemetry->Teardown();
	delete mIOCycleTelemetry;
	mIOCycleTelemetry = NULL;
#endif
	DestroyIOProcList();
	
	HP_Object::Teardown();
	
	DestroyDeviceStateMutex();
}

bool	HP_Device::IsAlive() const
{
	return true;
}

bool	HP_Device::IsHidden() const
{
	return false;
}

CFStringRef	HP_Device::CopyDeviceName() const
{
	//	This routine should return a CFStringRef that contains a string
	//	that is the human readable and localized name of the device.
	//	Note that the caller will CFRelease the returned object, so be
	//	sure that the object's ref count is correct.
	return NULL;
}

CFStringRef	HP_Device::CopyDeviceManufacturerName() const
{
	//	This routine should return a CFStringRef that contains a string
	//	that is the human readable and localized name of the device's manufacturer.
	//	Note that the caller will CFRelease the returned object, so be
	//	sure that the object's ref count is correct.
	return NULL;
}

CFURLRef	HP_Device::CopyDeviceIcon() const
{
	//	This routine should return a CFURLRef that contains a URL
	//	that is the location of the device's icon.
	//	Note that the caller will CFRelease the returned object, so be
	//	sure that the object's ref count is correct.
	return NULL;
}
 
CFStringRef	HP_Device::CopyElementFullName(const AudioObjectPropertyAddress& /*inAddress*/) const
{
	//	This routine returns a human readable, localized name of the given element
	//	this routine shouldn't throw an exception. Just return NULL if the value doesn't exist
	return NULL;
}

CFStringRef	HP_Device::CopyElementCategoryName(const AudioObjectPropertyAddress& /*inAddress*/) const
{
	//	This routine returns a human readable, localized name of the category of the given element
	//	this routine shouldn't throw an exception. Just return NULL if the value doesn't exist
	return NULL;
}

CFStringRef	HP_Device::CopyElementNumberName(const AudioObjectPropertyAddress& /*inAddress*/) const
{
	//	This routine returns a human readable, localized name of the number of the given element
	//	this routine shouldn't throw an exception. Just return NULL if the value doesn't exist
	return NULL;
}

CFStringRef	HP_Device::CopyConfigurationApplicationBundleID() const
{
	//	This routine should return a CFStringRef that contains a string
	//	that is the full path. By default, this app is Audio MIDI Setup.
	CFStringRef theAnswer = CFSTR("com.apple.audio.AudioMIDISetup");
	return static_cast<CFStringRef>(CFRetain(theAnswer));
}

CFStringRef	HP_Device::CopyDeviceUID() const
{
	//	This routine should return a CFStringRef that contains a string
	//	that is unique to this instance of the device. This string must
	//	be persistant so that it is the same between process launches
	//	and boots. Note that the caller will CFRelease the returned
	//	object, so be sure that the object's ref count is correct.
	return NULL;
}

CFStringRef	HP_Device::CopyModelUID() const
{
	//	This routine should return a CFString that contains a persistent
	//  identifier for the model of an AudioDevice. The identifier is
	//  unique such that the identifier from two AudioDevices are equal
	//  if and only if the two AudioDevices are the exact same model from
	//  the same manufacturer. Further, the identifier has to be the same
	//  no matter on what machine the AudioDevice appears.
	return NULL;
}

UInt32	HP_Device::GetTransportType() const
{
	//	This routine returns how the device is connected to the system.
	return 0;
}

bool	HP_Device::IsConstantRateClock() const
{
	//  by default, use the transport to determine this
	UInt32 theTransportType = GetTransportType();
	return theTransportType == kAudioDeviceTransportTypeUSB;
}

bool	HP_Device::CanBeDefaultDevice(bool /*inIsInput*/, bool /*inIsSystem*/) const
{
	//	This routine returns whether or not the device is allowed to be
	//	selected as the default device. Since plug-in devices generally
	//	do not support mult-process mixing, they should generally always
	//	return false for this routine.
	return false;
}

bool	HP_Device::HogModeIsOwnedBySelf() const
{
	return false;
}

bool	HP_Device::HogModeIsOwnedBySelfOrIsFree() const
{
	return true;
}

void	HP_Device::HogModeStateChanged()
{
	Do_StopAllIOProcs();
}

void	HP_Device::GetDefaultChannelLayout(bool inIsInput, AudioChannelLayout& outLayout) const
{
	CAAudioChannelLayout::SetAllToUnknown(outLayout, GetTotalNumberChannels(inIsInput));
}

CAMutex*	HP_Device::GetObjectStateMutex()
{
	return mDeviceStateMutex;
}

void	HP_Device::CreateDeviceStateMutex()
{
	if(mDeviceStateMutex == NULL)
	{
		mDeviceStateMutex = new CAMutex("DeviceStateMutex");
	}
}

void	HP_Device::DestroyDeviceStateMutex()
{
	delete mDeviceStateMutex;
	mDeviceStateMutex = NULL;
}

void	HP_Device::Show() const
{
	//  make a string for the class name
	char* theClassName = NULL;
	
	switch(mClassID)
	{
		case kAudioAggregateDeviceClassID:
			theClassName = (char*)"Aggregate Device";
			break;
		
		case kAudioDeviceClassID:
		default:
			theClassName = (char*)"Audio Device";
			break;
	}
	
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
	printf("AudioObjectID:\t\t\t0x%lX\n\tClass:\t\t\t\t%s\n\tName:\t\t\t\t%s\n\tInput Channels:\t\t%lu\n\tOutput Channels:\t%lu\n", (long unsigned int)mObjectID, theClassName, theName, (long unsigned int)GetTotalNumberChannels(true), (long unsigned int)GetTotalNumberChannels(false));
}

bool	HP_Device::HasProperty(const AudioObjectPropertyAddress& inAddress) const
{
	//	initialize the return value
	bool theAnswer = false;
	
	//	initialize some commonly used variables
	CFStringRef theCFString = NULL;
	CFURLRef theCFURL = NULL;
	
	//	take and hold the state mutex
	CAMutex::Locker theStateMutex(const_cast<HP_Device*>(this)->GetDeviceStateMutex());
	
	switch(inAddress.mSelector)
	{
		case kAudioObjectPropertyName:
			theCFString = CopyDeviceName();
			if(theCFString != NULL)
			{
				theAnswer = true;
				CFRelease(theCFString);
			}
			break;
			
		case kAudioObjectPropertyManufacturer:
			theCFString = CopyDeviceManufacturerName();
			if(theCFString != NULL)
			{
				theAnswer = true;
				CFRelease(theCFString);
			}
			break;
			
		case kAudioObjectPropertyElementName:
			theCFString = CopyElementFullName(inAddress);
			if(theCFString != NULL)
			{
				theAnswer = true;
				CFRelease(theCFString);
			}
			break;
			
		case kAudioObjectPropertyElementCategoryName:
			theCFString = CopyElementCategoryName(inAddress);
			if(theCFString != NULL)
			{
				theAnswer = true;
				CFRelease(theCFString);
			}
			break;
			
		case kAudioObjectPropertyElementNumberName:
			theCFString = CopyElementNumberName(inAddress);
			if(theCFString != NULL)
			{
				theAnswer = true;
				CFRelease(theCFString);
			}
			break;
			
		case kAudioDevicePropertyConfigurationApplication:
			theCFString = CopyConfigurationApplicationBundleID();
			if(theCFString != NULL)
			{
				theAnswer = true;
				CFRelease(theCFString);
			}
			break;
			
		case kAudioDevicePropertyDeviceUID:
			theCFString = CopyDeviceUID();
			if(theCFString != NULL)
			{
				theAnswer = true;
				CFRelease(theCFString);
			}
			break;
			
		case kAudioDevicePropertyModelUID:
			theCFString = CopyModelUID();
			if(theCFString != NULL)
			{
				theAnswer = true;
				CFRelease(theCFString);
			}
			break;
			
		case kAudioDevicePropertyTransportType:
			theAnswer = true;
			break;
			
		case kAudioDevicePropertyRelatedDevices:
			theAnswer = true;
			break;
		
		case kAudioDevicePropertyClockDomain:
			theAnswer = true;
			break;
			
		case kAudioDevicePropertyDeviceIsAlive:
			theAnswer = true;
			break;
			
		case kAudioDevicePropertyDeviceHasChanged:
			theAnswer = true;
			break;
			
		case kAudioDevicePropertyDeviceIsRunning:
			theAnswer = true;
			break;
			
		case kAudioDevicePropertyDeviceIsRunningSomewhere:
			theAnswer = true;
			break;
			
		case kAudioDevicePropertyDeviceCanBeDefaultDevice:
			theAnswer = (inAddress.mScope == kAudioDevicePropertyScopeInput) || (inAddress.mScope == kAudioDevicePropertyScopeOutput);
			break;
			
		case kAudioDevicePropertyDeviceCanBeDefaultSystemDevice:
			theAnswer = inAddress.mScope == kAudioDevicePropertyScopeOutput;
			break;
			
		case kAudioDeviceProcessorOverload:
			theAnswer = true;
			break;
		
		case kAudioDevicePropertyHogMode:
			theAnswer = true;
			break;
			
		case kAudioDevicePropertyLatency:
			theAnswer = ((inAddress.mScope == kAudioDevicePropertyScopeInput) && HasInputStreams()) || ((inAddress.mScope == kAudioDevicePropertyScopeOutput) && HasOutputStreams());
			break;
			
		case kAudioDevicePropertyBufferFrameSize:
			theAnswer = true;
			break;
			
		case kAudioDevicePropertyBufferFrameSizeRange:
			theAnswer = true;
			break;
			
		case kAudioDevicePropertyStreams:
			theAnswer = (inAddress.mScope == kAudioDevicePropertyScopeInput) || (inAddress.mScope == kAudioDevicePropertyScopeOutput);
			break;
			
		case kAudioDevicePropertySafetyOffset:
			theAnswer = ((inAddress.mScope == kAudioDevicePropertyScopeInput) && HasInputStreams()) || ((inAddress.mScope == kAudioDevicePropertyScopeOutput) && HasOutputStreams());
			break;
			
		case kAudioDevicePropertyStreamConfiguration:
			theAnswer = (inAddress.mScope == kAudioDevicePropertyScopeInput) || (inAddress.mScope == kAudioDevicePropertyScopeOutput);
			break;
			
		case kAudioDevicePropertyIOProcStreamUsage:
			theAnswer = ((inAddress.mScope == kAudioDevicePropertyScopeInput) && HasInputStreams()) || ((inAddress.mScope == kAudioDevicePropertyScopeOutput) && HasOutputStreams());
			break;
			
		case kAudioDevicePropertyActualSampleRate:
			theAnswer = true;
			break;
			
		case kAudioDevicePropertyDeviceName:
			theCFString = CopyDeviceName();
			if(theCFString != NULL)
			{
				theAnswer = true;
				CFRelease(theCFString);
			}
			break;
			
		case kAudioDevicePropertyDeviceManufacturer:
			theCFString = CopyDeviceManufacturerName();
			if(theCFString != NULL)
			{
				theAnswer = true;
				CFRelease(theCFString);
			}
			break;
			
		case kAudioDevicePropertyChannelName:
			theCFString = CopyElementFullName(inAddress);
			if(theCFString != NULL)
			{
				theAnswer = true;
				CFRelease(theCFString);
			}
			break;
			
		case kAudioDevicePropertyChannelCategoryName:
			theCFString = CopyElementCategoryName(inAddress);
			if(theCFString != NULL)
			{
				theAnswer = true;
				CFRelease(theCFString);
			}
			break;
			
		case kAudioDevicePropertyChannelNumberName:
			theCFString = CopyElementNumberName(inAddress);
			if(theCFString != NULL)
			{
				theAnswer = true;
				CFRelease(theCFString);
			}
			break;
			
		case kAudioDevicePropertyBufferSize:
			theAnswer = true;
			break;
			
		case kAudioDevicePropertyBufferSizeRange:
			theAnswer = true;
			break;
		
		case kAudioDevicePropertyIcon:
			theCFURL = CopyDeviceIcon();
			if(theCFURL != NULL)
			{
				theAnswer = true;
				CFRelease(theCFURL);
			}
			break;
		
		case kAudioDevicePropertyIsHidden:
			theAnswer = true;
			break;
			
		default:
			theAnswer = HP_Object::HasProperty(inAddress);
			break;
	};
	
	return theAnswer;
}

bool	HP_Device::IsPropertySettable(const AudioObjectPropertyAddress& inAddress) const
{
	bool theAnswer = false;
	
	//	take and hold the state mutex
	CAMutex::Locker theStateMutex(const_cast<HP_Device*>(this)->GetDeviceStateMutex());
	
	switch(inAddress.mSelector)
	{
		case kAudioObjectPropertyName:
			theAnswer = false;
			break;
			
		case kAudioObjectPropertyManufacturer:
			theAnswer = false;
			break;
			
		case kAudioObjectPropertyElementName:
			theAnswer = false;
			break;
			
		case kAudioObjectPropertyElementCategoryName:
			theAnswer = false;
			break;
			
		case kAudioObjectPropertyElementNumberName:
			theAnswer = false;
			break;
			
		case kAudioDevicePropertyConfigurationApplication:
			theAnswer = false;
			break;
			
		case kAudioDevicePropertyDeviceUID:
			theAnswer = false;
			break;
			
		case kAudioDevicePropertyModelUID:
			theAnswer = false;
			break;
			
		case kAudioDevicePropertyTransportType:
			theAnswer = false;
			break;
			
		case kAudioDevicePropertyRelatedDevices:
			theAnswer = false;
			break;
			
		case kAudioDevicePropertyClockDomain:
			theAnswer = false;
			break;
			
		case kAudioDevicePropertyDeviceIsAlive:
			theAnswer = false;
			break;
			
		case kAudioDevicePropertyDeviceHasChanged:
			theAnswer = false;
			break;
			
		case kAudioDevicePropertyDeviceIsRunning:
			theAnswer = true;
			break;
			
		case kAudioDevicePropertyDeviceIsRunningSomewhere:
			theAnswer = false;
			break;
			
		case kAudioDevicePropertyDeviceCanBeDefaultDevice:
			theAnswer = false;
			break;
			
		case kAudioDevicePropertyDeviceCanBeDefaultSystemDevice:
			theAnswer = false;
			break;
			
		case kAudioDeviceProcessorOverload:
			theAnswer = false;
			break;
		
		case kAudioDevicePropertyHogMode:
			theAnswer = false;
			break;
			
		case kAudioDevicePropertyLatency:
			theAnswer = false;
			break;
			
		case kAudioDevicePropertyBufferFrameSize:
			theAnswer = true;
			break;
			
		case kAudioDevicePropertyBufferFrameSizeRange:
			theAnswer = false;
			break;
			
		case kAudioDevicePropertyStreams:
			theAnswer = false;
			break;
			
		case kAudioDevicePropertySafetyOffset:
			theAnswer = false;
			break;
			
		case kAudioDevicePropertyStreamConfiguration:
			theAnswer = false;
			break;
			
		case kAudioDevicePropertyIOProcStreamUsage:
			theAnswer = true;
			break;
			
		case kAudioDevicePropertyActualSampleRate:
			theAnswer = false;
			break;
			
		case kAudioDevicePropertyDeviceName:
			theAnswer = false;
			break;
			
		case kAudioDevicePropertyDeviceManufacturer:
			theAnswer = false;
			break;
			
		case kAudioDevicePropertyChannelName:
			theAnswer = false;
			break;
			
		case kAudioDevicePropertyChannelCategoryName:
			theAnswer = false;
			break;
			
		case kAudioDevicePropertyChannelNumberName:
			theAnswer = false;
			break;
			
		case kAudioDevicePropertyBufferSize:
			theAnswer = true;
			break;
			
		case kAudioDevicePropertyBufferSizeRange:
			theAnswer = false;
			break;
		
		case kAudioDevicePropertyIcon:
			theAnswer = false;
			break;
			
		case kAudioDevicePropertyIsHidden:
			theAnswer = false;
			break;
			
		default:
			theAnswer = HP_Object::IsPropertySettable(inAddress);
			break;
	};
	
	return theAnswer;
}

UInt32	HP_Device::GetPropertyDataSize(const AudioObjectPropertyAddress& inAddress, UInt32 inQualifierDataSize, const void* inQualifierData) const
{
	UInt32 theAnswer = 0;
	CFStringRef theCFString = NULL;
	
	//	Figure out what section is involved. Note that the HAL's API calls HasProperty before calling
	//	GetPropertyDataSize. This means that it can be assumed that inAddress is valid for the property involved.
	bool isInput = inAddress.mScope == kAudioDevicePropertyScopeInput;
				
	//	take and hold the state mutex
	CAMutex::Locker theStateMutex(const_cast<HP_Device*>(this)->GetDeviceStateMutex());
	
	switch(inAddress.mSelector)
	{
		case kAudioObjectPropertyName:
			theAnswer = SizeOf32(CFStringRef);
			break;
			
		case kAudioObjectPropertyManufacturer:
			theAnswer = SizeOf32(CFStringRef);
			break;
			
		case kAudioObjectPropertyElementName:
			theAnswer = SizeOf32(CFStringRef);
			break;
			
		case kAudioObjectPropertyElementCategoryName:
			theAnswer = SizeOf32(CFStringRef);
			break;
			
		case kAudioObjectPropertyElementNumberName:
			theAnswer = SizeOf32(CFStringRef);
			break;
			
		case kAudioDevicePropertyConfigurationApplication:
			theAnswer = SizeOf32(CFStringRef);
			break;
			
		case kAudioDevicePropertyDeviceUID:
			theAnswer = SizeOf32(CFStringRef);
			break;
			
		case kAudioDevicePropertyModelUID:
			theAnswer = SizeOf32(CFStringRef);
			break;
			
		case kAudioDevicePropertyTransportType:
			theAnswer = SizeOf32(UInt32);
			break;
			
		case kAudioDevicePropertyRelatedDevices:
			theAnswer = SizeOf32(AudioObjectID);
			break;
			
		case kAudioDevicePropertyClockDomain:
			theAnswer = SizeOf32(UInt32);
			break;
			
		case kAudioDevicePropertyDeviceIsAlive:
			theAnswer = SizeOf32(UInt32);
			break;
			
		case kAudioDevicePropertyDeviceHasChanged:
			theAnswer = SizeOf32(UInt32);
			break;
			
		case kAudioDevicePropertyDeviceIsRunning:
			theAnswer = SizeOf32(UInt32);
			break;
			
		case kAudioDevicePropertyDeviceIsRunningSomewhere:
			theAnswer = SizeOf32(UInt32);
			break;
			
		case kAudioDevicePropertyDeviceCanBeDefaultDevice:
			theAnswer = SizeOf32(UInt32);
			break;
			
		case kAudioDevicePropertyDeviceCanBeDefaultSystemDevice:
			theAnswer = SizeOf32(UInt32);
			break;
			
		case kAudioDeviceProcessorOverload:
			theAnswer = SizeOf32(UInt32);
			break;
			
		case kAudioDevicePropertyHogMode:
			theAnswer = SizeOf32(pid_t);
			break;
			
		case kAudioDevicePropertyLatency:
			theAnswer = SizeOf32(UInt32);
			break;
			
		case kAudioDevicePropertyBufferFrameSize:
			theAnswer = SizeOf32(UInt32);
			break;
			
		case kAudioDevicePropertyBufferFrameSizeRange:
			theAnswer = SizeOf32(AudioValueRange);
			break;
			
		case kAudioDevicePropertyStreams:
			theAnswer = SizeOf32(AudioStreamID) * GetNumberStreams(isInput);
			break;
			
		case kAudioDevicePropertySafetyOffset:
			theAnswer = SizeOf32(UInt32);
			break;
			
		case kAudioDevicePropertyStreamConfiguration:
			theAnswer = CAAudioBufferList::CalculateByteSize(GetNumberStreams(isInput));
			break;
			
		case kAudioDevicePropertyIOProcStreamUsage:
			theAnswer = SizeOf32(void*) + SizeOf32(UInt32) + (GetNumberStreams(isInput) * SizeOf32(UInt32));
			break;
			
		case kAudioDevicePropertyActualSampleRate:
			theAnswer = SizeOf32(Float64);
			break;
			
		case kAudioDevicePropertyDeviceName:
			theCFString = CopyDeviceName();
			if(theCFString != NULL)
			{
				theAnswer = ToUInt32(CFStringGetLength(theCFString)) + 1;
				CFRelease(theCFString);
			}
			break;
			
		case kAudioDevicePropertyDeviceManufacturer:
			theCFString = CopyDeviceManufacturerName();
			if(theCFString != NULL)
			{
				theAnswer = ToUInt32(CFStringGetLength(theCFString)) + 1;
				CFRelease(theCFString);
			}
			break;
			
		case kAudioDevicePropertyChannelName:
			theCFString = CopyElementFullName(inAddress);
			if(theCFString != NULL)
			{
				theAnswer = ToUInt32(CFStringGetLength(theCFString)) + 1;
				CFRelease(theCFString);
			}
			break;
			
		case kAudioDevicePropertyChannelCategoryName:
			theCFString = CopyElementCategoryName(inAddress);
			if(theCFString != NULL)
			{
				theAnswer = ToUInt32(CFStringGetLength(theCFString)) + 1;
				CFRelease(theCFString);
			}
			break;
			
		case kAudioDevicePropertyChannelNumberName:
			theCFString = CopyElementNumberName(inAddress);
			if(theCFString != NULL)
			{
				theAnswer = ToUInt32(CFStringGetLength(theCFString)) + 1;
				CFRelease(theCFString);
			}
			break;
			
		case kAudioDevicePropertyBufferSize:
			theAnswer = SizeOf32(UInt32);
			break;
			
		case kAudioDevicePropertyBufferSizeRange:
			theAnswer = SizeOf32(AudioValueRange);
			break;
			
		case kAudioDevicePropertyIcon:
			theAnswer = SizeOf32(CFURLRef);
			break;
			
		case kAudioDevicePropertyIsHidden:
			theAnswer = SizeOf32(UInt32);
			break;
			
		default:
			theAnswer = HP_Object::GetPropertyDataSize(inAddress, inQualifierDataSize, inQualifierData);
			break;
	};
	
	return theAnswer;
}

void	HP_Device::GetPropertyData(const AudioObjectPropertyAddress& inAddress, UInt32 inQualifierDataSize, const void* inQualifierData, UInt32& ioDataSize, void* outData) const
{
	HP_Stream* theStream = NULL;
	UInt32 theIndex = 0;
	
	//	Figure out what section is involved. Note that the HAL's API calls HasProperty before calling
	//	GetPropertyData. This means that it can be assumed that inAddress is valid for the property involved.
	bool isInput = inAddress.mScope == kAudioDevicePropertyScopeInput;
				
	//	take and hold the state mutex
	CAMutex::Locker theStateMutex(const_cast<HP_Device*>(this)->GetDeviceStateMutex());
	
	switch(inAddress.mSelector)
	{
		case kAudioObjectPropertyName:
			ThrowIf(ioDataSize != GetPropertyDataSize(inAddress, inQualifierDataSize, inQualifierData), CAException(kAudioHardwareBadPropertySizeError), "HP_Device::GetPropertyData: wrong data size for kAudioObjectPropertyName");
			*static_cast<CFStringRef*>(outData) = CopyDeviceName();
			break;
			
		case kAudioObjectPropertyManufacturer:
			ThrowIf(ioDataSize != GetPropertyDataSize(inAddress, inQualifierDataSize, inQualifierData), CAException(kAudioHardwareBadPropertySizeError), "HP_Device::GetPropertyData: wrong data size for kAudioObjectPropertyManufacturer");
			*static_cast<CFStringRef*>(outData) = CopyDeviceManufacturerName();
			break;
			
		case kAudioObjectPropertyElementName:
			ThrowIf(ioDataSize != GetPropertyDataSize(inAddress, inQualifierDataSize, inQualifierData), CAException(kAudioHardwareBadPropertySizeError), "HP_Device::GetPropertyData: wrong data size for kAudioObjectPropertyElementName");
			*static_cast<CFStringRef*>(outData) = CopyElementFullName(inAddress);
			break;
			
		case kAudioObjectPropertyElementCategoryName:
			ThrowIf(ioDataSize != GetPropertyDataSize(inAddress, inQualifierDataSize, inQualifierData), CAException(kAudioHardwareBadPropertySizeError), "HP_Device::GetPropertyData: wrong data size for kAudioObjectPropertyElementCategoryName");
			*static_cast<CFStringRef*>(outData) = CopyElementCategoryName(inAddress);
			break;
			
		case kAudioObjectPropertyElementNumberName:
			ThrowIf(ioDataSize != GetPropertyDataSize(inAddress, inQualifierDataSize, inQualifierData), CAException(kAudioHardwareBadPropertySizeError), "HP_Device::GetPropertyData: wrong data size for kAudioObjectPropertyElementNumberName");
			*static_cast<CFStringRef*>(outData) = CopyElementNumberName(inAddress);
			break;
			
		case kAudioDevicePropertyConfigurationApplication:
			ThrowIf(ioDataSize != GetPropertyDataSize(inAddress, inQualifierDataSize, inQualifierData), CAException(kAudioHardwareBadPropertySizeError), "HP_Device::GetPropertyData: wrong data size for kAudioDevicePropertyConfigurationApplication");
			*static_cast<CFStringRef*>(outData) = CopyConfigurationApplicationBundleID();
			break;
			
		case kAudioDevicePropertyDeviceUID:
			ThrowIf(ioDataSize != GetPropertyDataSize(inAddress, inQualifierDataSize, inQualifierData), CAException(kAudioHardwareBadPropertySizeError), "HP_Device::GetPropertyData: wrong data size for kAudioDevicePropertyDeviceUID");
			*static_cast<CFStringRef*>(outData) = CopyDeviceUID();
			break;
			
		case kAudioDevicePropertyModelUID:
			ThrowIf(ioDataSize != GetPropertyDataSize(inAddress, inQualifierDataSize, inQualifierData), CAException(kAudioHardwareBadPropertySizeError), "HP_Device::GetPropertyData: wrong data size for kAudioDevicePropertyModelUID");
			*static_cast<CFStringRef*>(outData) = CopyModelUID();
			break;
			
		case kAudioDevicePropertyTransportType:
			ThrowIf(ioDataSize != GetPropertyDataSize(inAddress, inQualifierDataSize, inQualifierData), CAException(kAudioHardwareBadPropertySizeError), "HP_Device::GetPropertyData: wrong data size for kAudioDevicePropertyTransportType");
			*static_cast<UInt32*>(outData) = GetTransportType();
			break;
			
		case kAudioDevicePropertyRelatedDevices:
			if(ioDataSize >= sizeof(AudioObjectID))
			{
				*static_cast<AudioDeviceID*>(outData) = GetObjectID();
				ioDataSize = SizeOf32(AudioObjectID);
			}
			else
			{
				memset(outData, ioDataSize, 0);
				ioDataSize = 0;
			}
			break;
			
		case kAudioDevicePropertyClockDomain:
			ThrowIf(ioDataSize != GetPropertyDataSize(inAddress, inQualifierDataSize, inQualifierData), CAException(kAudioHardwareBadPropertySizeError), "HP_Device::GetPropertyData: wrong data size for kAudioDevicePropertyClockDomain");
			*static_cast<UInt32*>(outData) = 0;
			break;
			
		case kAudioDevicePropertyDeviceIsAlive:
			ThrowIf(ioDataSize != GetPropertyDataSize(inAddress, inQualifierDataSize, inQualifierData), CAException(kAudioHardwareBadPropertySizeError), "HP_Device::GetPropertyData: wrong data size for kAudioDevicePropertyDeviceIsAlive");
			*static_cast<UInt32*>(outData) = IsAlive();
			break;
			
		case kAudioDevicePropertyDeviceHasChanged:
			ThrowIf(ioDataSize != GetPropertyDataSize(inAddress, inQualifierDataSize, inQualifierData), CAException(kAudioHardwareBadPropertySizeError), "HP_Device::GetPropertyData: wrong data size for kAudioDevicePropertyDeviceHasChanged");
			*static_cast<UInt32*>(outData) = 0;
			break;
			
		case kAudioDevicePropertyDeviceIsRunning:
			ThrowIf(ioDataSize != GetPropertyDataSize(inAddress, inQualifierDataSize, inQualifierData), CAException(kAudioHardwareBadPropertySizeError), "HP_Device::GetPropertyData: wrong data size for kAudioDevicePropertyDeviceIsRunning");
			*static_cast<UInt32*>(outData) = IsIOEngineRunning() ? 1 : 0;
			break;
			
		case kAudioDevicePropertyDeviceIsRunningSomewhere:
			ThrowIf(ioDataSize != GetPropertyDataSize(inAddress, inQualifierDataSize, inQualifierData), CAException(kAudioHardwareBadPropertySizeError), "HP_Device::GetPropertyData: wrong data size for kAudioDevicePropertyDeviceIsRunningSomewhere");
			*static_cast<UInt32*>(outData) = IsIOEngineRunningSomewhere() ? 1 : 0;
			break;
			
		case kAudioDevicePropertyDeviceCanBeDefaultDevice:
			ThrowIf(ioDataSize != GetPropertyDataSize(inAddress, inQualifierDataSize, inQualifierData), CAException(kAudioHardwareBadPropertySizeError), "HP_Device::GetPropertyData: wrong data size for kAudioDevicePropertyDeviceCanBeDefaultDevice");
			*static_cast<UInt32*>(outData) = CanBeDefaultDevice(isInput, false) ? 1 : 0;
			break;
			
		case kAudioDevicePropertyDeviceCanBeDefaultSystemDevice:
			ThrowIf(ioDataSize != GetPropertyDataSize(inAddress, inQualifierDataSize, inQualifierData), CAException(kAudioHardwareBadPropertySizeError), "HP_Device::GetPropertyData: wrong data size for kAudioDevicePropertyDeviceCanBeDefaultSystemDevice");
			*static_cast<UInt32*>(outData) = CanBeDefaultDevice(isInput, true) ? 1 : 0;
			break;
			
		case kAudioDeviceProcessorOverload:
			ThrowIf(ioDataSize != GetPropertyDataSize(inAddress, inQualifierDataSize, inQualifierData), CAException(kAudioHardwareBadPropertySizeError), "HP_Device::GetPropertyData: wrong data size for kAudioDeviceProcessorOverload");
			*static_cast<UInt32*>(outData) = 0;
			break;
			
		case kAudioDevicePropertyHogMode:
			ThrowIf(ioDataSize != GetPropertyDataSize(inAddress, inQualifierDataSize, inQualifierData), CAException(kAudioHardwareBadPropertySizeError), "HP_Device::GetPropertyData: wrong data size for kAudioDevicePropertyHogMode");
			*static_cast<pid_t*>(outData) = -1;
			break;
			
		case kAudioDevicePropertyLatency:
			ThrowIf(ioDataSize != GetPropertyDataSize(inAddress, inQualifierDataSize, inQualifierData), CAException(kAudioHardwareBadPropertySizeError), "HP_Device::GetPropertyData: wrong data size for kAudioDevicePropertyLatency");
			*static_cast<UInt32*>(outData) = GetLatency(isInput);
			break;
			
		case kAudioDevicePropertyBufferFrameSize:
			ThrowIf(ioDataSize != GetPropertyDataSize(inAddress, inQualifierDataSize, inQualifierData), CAException(kAudioHardwareBadPropertySizeError), "HP_Device::GetPropertyData: wrong data size for kAudioDevicePropertyLatency");
			*static_cast<UInt32*>(outData) = GetIOBufferFrameSize();
			break;
			
		case kAudioDevicePropertyBufferFrameSizeRange:
			ThrowIf(ioDataSize != GetPropertyDataSize(inAddress, inQualifierDataSize, inQualifierData), CAException(kAudioHardwareBadPropertySizeError), "HP_Device::GetPropertyData: wrong data size for kAudioDevicePropertyBufferFrameSizeRange");
			static_cast<AudioValueRange*>(outData)->mMinimum = GetMinimumIOBufferFrameSize();
			static_cast<AudioValueRange*>(outData)->mMaximum = GetMaximumIOBufferFrameSize();
			break;
			
		case kAudioDevicePropertyStreams:
			{
				AudioStreamID* theStreamList = static_cast<AudioStreamID*>(outData);
				UInt32 theNumberStreams = std::min((UInt32)(ioDataSize / SizeOf32(AudioStreamID)), GetNumberStreams(isInput));
				for(theIndex = 0; theIndex < theNumberStreams ; ++theIndex)
				{
					theStream = GetStreamByIndex(isInput, theIndex);
					theStreamList[theIndex] = theStream->GetObjectID();
				}
				ioDataSize = theNumberStreams * SizeOf32(AudioStreamID);
			}
			break;
			
		case kAudioDevicePropertySafetyOffset:
			ThrowIf(ioDataSize != GetPropertyDataSize(inAddress, inQualifierDataSize, inQualifierData), CAException(kAudioHardwareBadPropertySizeError), "HP_Device::GetPropertyData: wrong data size for kAudioDevicePropertySafetyOffset");
			*static_cast<UInt32*>(outData) = GetSafetyOffset(isInput);
			break;
			
		case kAudioDevicePropertyStreamConfiguration:
			{
				UInt32 theExpectedSize = GetPropertyDataSize(inAddress, inQualifierDataSize, inQualifierData);
				ThrowIf(ioDataSize < theExpectedSize, CAException(kAudioHardwareBadPropertySizeError), "HP_Device::GetPropertyData: wrong data size for kAudioDevicePropertyStreamConfiguration");
				ioDataSize = theExpectedSize;
				AudioBufferList* theBufferList = static_cast<AudioBufferList*>(outData);
				theBufferList->mNumberBuffers = GetNumberStreams(isInput);
				for(theIndex = 0; theIndex < theBufferList->mNumberBuffers; ++theIndex)
				{
					theStream = GetStreamByIndex(isInput, theIndex);
					theBufferList->mBuffers[theIndex].mNumberChannels = theStream->GetCurrentNumberChannels();
					theBufferList->mBuffers[theIndex].mDataByteSize = theStream->CalculateIOBufferByteSize(GetIOBufferFrameSize());
					theBufferList->mBuffers[theIndex].mData = NULL;
				}
			}
			break;
			
		case kAudioDevicePropertyIOProcStreamUsage:
			{
				//	make sure the size is correct
				ThrowIf(ioDataSize != GetPropertyDataSize(inAddress, inQualifierDataSize, inQualifierData), CAException(kAudioHardwareBadPropertySizeError), "HP_Device::GetPropertyData: wrong data size for kAudioDevicePropertyIOProcStreamUsage");
				
				//	get the current stream usage
				AudioHardwareIOProcStreamUsage* theStreamUsageStruct = static_cast<AudioHardwareIOProcStreamUsage*>(outData);
				UInt32 theNumberStreams = GetNumberStreams(isInput);
				CAAutoArrayDelete<bool> theStreamUsage(theNumberStreams);
				memset(theStreamUsage.get(), 0, theNumberStreams * sizeof(bool));
				if(mIOProcList != NULL)
				{
					mIOProcList->GetIOProcStreamUsage((AudioDeviceIOProc)theStreamUsageStruct->mIOProc, isInput, theNumberStreams, theStreamUsage);
				}
				
				//	fill out the return value
				theStreamUsageStruct->mNumberStreams = theNumberStreams;
				for(theIndex = 0; theIndex < theNumberStreams; ++theIndex)
				{
					theStreamUsageStruct->mStreamIsOn[theIndex] = (theStreamUsage[theIndex] ? 1 : 0);
				}
			}
			break;
			
		case kAudioDevicePropertyActualSampleRate:
			ThrowIf(ioDataSize != GetPropertyDataSize(inAddress, inQualifierDataSize, inQualifierData), CAException(kAudioHardwareBadPropertySizeError), "HP_Device::GetPropertyData: wrong data size for kAudioDevicePropertyActualSampleRate");
			*static_cast<Float64*>(outData) = GetCurrentActualSampleRate();
			break;
			
		case kAudioDevicePropertyDeviceName:
			{
				CACFString	theDeviceName(CopyDeviceName());
				theDeviceName.GetCString(static_cast<char*>(outData), ioDataSize);
			}
			break;
			
		case kAudioDevicePropertyDeviceManufacturer:
			{
				CACFString	theDeviceManufacturerName(CopyDeviceManufacturerName());
				theDeviceManufacturerName.GetCString(static_cast<char*>(outData), ioDataSize);
			}
			break;
			
		case kAudioDevicePropertyChannelName:
			{
				CACFString	theElementFullName(CopyElementFullName(inAddress));
				theElementFullName.GetCString(static_cast<char*>(outData), ioDataSize);
			}
			break;
			
		case kAudioDevicePropertyChannelCategoryName:
			{
				CACFString	theElementCategoryName(CopyElementCategoryName(inAddress));
				theElementCategoryName.GetCString(static_cast<char*>(outData), ioDataSize);
			}
			break;
			
		case kAudioDevicePropertyChannelNumberName:
			{
				CACFString	theElementNumberName(CopyElementNumberName(inAddress));
				theElementNumberName.GetCString(static_cast<char*>(outData), ioDataSize);
			}
			break;
			
		case kAudioDevicePropertyBufferSize:
			{
				ThrowIf(ioDataSize != GetPropertyDataSize(inAddress, inQualifierDataSize, inQualifierData), CAException(kAudioHardwareBadPropertySizeError), "HP_Device::GetPropertyData: wrong data size for kAudioDevicePropertyBufferSize");
				
				//	To convert a buffer size in frames to a size in bytes, we need to find the stream that goes with the address
				theStream = GetStreamByPropertyAddress(inAddress, true);
				ThrowIfNULL(theStream, CAException(kAudioHardwareIllegalOperationError), "HP_Device::GetPropertyData: kAudioDevicePropertyBufferSize: no streams");
				
				//	do the work
				*static_cast<UInt32*>(outData) = theStream->CalculateIOBufferByteSize(GetIOBufferFrameSize());
			}
			break;
			
		case kAudioDevicePropertyBufferSizeRange:
			{
				ThrowIf(ioDataSize != GetPropertyDataSize(inAddress, inQualifierDataSize, inQualifierData), CAException(kAudioHardwareBadPropertySizeError), "HP_Device::GetPropertyData: wrong data size for kAudioDevicePropertyBufferSizeRange");
				
				//	To convert a buffer size in frames to a size in bytes, we need to find the stream that goes with the address
				theStream = GetStreamByPropertyAddress(inAddress, true);
				ThrowIfNULL(theStream, CAException(kAudioHardwareIllegalOperationError), "HP_Device::GetPropertyData: kAudioDevicePropertyBufferSize: no streams");
				
				//	do the work
				static_cast<AudioValueRange*>(outData)->mMinimum = theStream->CalculateIOBufferByteSize(GetMinimumIOBufferFrameSize());
				static_cast<AudioValueRange*>(outData)->mMaximum = theStream->CalculateIOBufferByteSize(GetMaximumIOBufferFrameSize());
			}
			break;
		
		case kAudioDevicePropertyIcon:
			{
				ThrowIf(ioDataSize != GetPropertyDataSize(inAddress, inQualifierDataSize, inQualifierData), CAException(kAudioHardwareBadPropertySizeError), "HP_Device::GetPropertyData: wrong data size for kAudioDevicePropertyIcon");
				
				//	do the work
				*static_cast<CFURLRef*>(outData) = CopyDeviceIcon();
			}
			break;
			
		case kAudioDevicePropertyIsHidden:
			{
				ThrowIf(ioDataSize != GetPropertyDataSize(inAddress, inQualifierDataSize, inQualifierData), CAException(kAudioHardwareBadPropertySizeError), "HP_Device::GetPropertyData: wrong data size for kAudioDevicePropertyIsHidden");
				
				//	do the work
				*static_cast<UInt32*>(outData) = IsHidden() ? 1 : 0;
			}
			break;
			
		default:
			HP_Object::GetPropertyData(inAddress, inQualifierDataSize, inQualifierData, ioDataSize, outData);
			break;
	};
}

void	HP_Device::SetPropertyData(const AudioObjectPropertyAddress& inAddress, UInt32 inQualifierDataSize, const void* inQualifierData, UInt32 inDataSize, const void* inData, const AudioTimeStamp* inWhen)
{
	//	Figure out what section is involved. Note that the HAL's API calls HasProperty and
	//	IsPropertySettable before calling SetPropertyData. This means that it can be assumed that
	//	inAddress is valid for the property involved.
	bool isInput = inAddress.mScope == kAudioDevicePropertyScopeInput;
				
	//	take and hold the state mutex
	CAMutex::Locker theStateMutex(GetDeviceStateMutex());
	
	switch(inAddress.mSelector)
	{
		case kAudioDevicePropertyDeviceIsRunning:
			ThrowIf(inDataSize != GetPropertyDataSize(inAddress, inQualifierDataSize, inQualifierData), CAException(kAudioHardwareBadPropertySizeError), "HP_Device::GetPropertyData: wrong data size for kAudioDevicePropertyDeviceIsRunning");
			if(*static_cast<const UInt32*>(inData) != 0)
			{
				Do_StartIOProc(NULL);
			}
			else
			{
				Do_StopIOProc(NULL);
			}
			break;
			
		case kAudioDevicePropertyBufferFrameSize:
			ThrowIf(inDataSize != GetPropertyDataSize(inAddress, inQualifierDataSize, inQualifierData), CAException(kAudioHardwareBadPropertySizeError), "HP_Device::SetPropertyData: wrong data size for kAudioDevicePropertyBufferFrameSize");
			Do_SetIOBufferFrameSize(*static_cast<const UInt32*>(inData));
			break;
			
		case kAudioDevicePropertyIOProcStreamUsage:
			{
				ThrowIf(inDataSize != GetPropertyDataSize(inAddress, inQualifierDataSize, inQualifierData), CAException(kAudioHardwareBadPropertySizeError), "HP_Device::SetPropertyData: wrong data size for kAudioDevicePropertyIOProcStreamUsage");
				const AudioHardwareIOProcStreamUsage* theStreamUsageStruct = static_cast<const AudioHardwareIOProcStreamUsage*>(inData);
				Do_SetIOProcStreamUsage((AudioDeviceIOProc)theStreamUsageStruct->mIOProc, isInput, theStreamUsageStruct->mNumberStreams, theStreamUsageStruct->mStreamIsOn);
			}
			break;
			
		case kAudioDevicePropertyBufferSize:
			{
				ThrowIf(inDataSize != GetPropertyDataSize(inAddress, inQualifierDataSize, inQualifierData), CAException(kAudioHardwareBadPropertySizeError), "HP_Device::SetPropertyData: wrong data size for kAudioDevicePropertyBufferFrameSize");
				
				//	To convert a buffer size in bytes to a size in frames, we need to find the stream that goes with the address
				HP_Stream* theStream = GetStreamByPropertyAddress(inAddress, true);
				ThrowIfNULL(theStream, CAException(kAudioHardwareIllegalOperationError), "HP_Device::SetPropertyData: kAudioDevicePropertyBufferSize: no streams");
				
				//	do the work
				Do_SetIOBufferFrameSize(theStream->CalculateIOBufferFrameSize(*static_cast<const UInt32*>(inData)));
			}
			break;
			
		default:
			HP_Object::SetPropertyData(inAddress, inQualifierDataSize, inQualifierData, inDataSize, inData, inWhen);
			break;
	};
}

#if	(MAC_OS_X_VERSION_MAX_ALLOWED < MAC_OS_X_VERSION_10_4)
void	HP_Device::PropertiesChanged(UInt32 inNumberAddresses, const AudioObjectPropertyAddress inAddresses[]) const
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
		
	for(UInt32 theIndex = 0; theIndex < inNumberAddresses; ++theIndex)
	{
		OSStatus theError = 0;
		
		if((inAddresses[theIndex].mScope == kAudioDevicePropertyScopeInput) || ((inAddresses[theIndex].mScope == kAudioObjectPropertyScopeGlobal) && HasInputStreams()))
		{
			theError = AudioHardwareDevicePropertyChanged(mPlugIn->GetInterface(), mObjectID, inAddresses[theIndex].mElement, true, inAddresses[theIndex].mSelector);
			AssertNoError(theError, "HP_Device::PropertiesChanged: got an error calling the input listeners");
		}
		
		if((inAddresses[theIndex].mScope == kAudioDevicePropertyScopeOutput) || ((inAddresses[theIndex].mScope == kAudioObjectPropertyScopeGlobal) && HasOutputStreams()))
		{
			theError = AudioHardwareDevicePropertyChanged(mPlugIn->GetInterface(), mObjectID, inAddresses[theIndex].mElement, false, inAddresses[theIndex].mSelector);
			AssertNoError(theError, "HP_Device::PropertiesChanged: got an error calling the output listeners");
		}
	}
		
	//	re-lock the mutex
	if((theObjectStateMutex != NULL) && ownsStateMutex)
	{
		theObjectStateMutex->Lock();
	}
}
#endif

void	HP_Device::ClearPrefs()
{
	mPreferredChannels->ClearPrefs();
}

void	HP_Device::ExecuteCommand(HP_Command* inCommand)
{
	//	see if we can execute the command immediately
	if(IsSafeToExecuteCommand() && IsPermittedToExecuteCommand(inCommand))
	{
		OSStatus theError = 0;
		
		//	we can, so do so and toss the object
		void* theSavedCommandState;
		if(StartCommandExecution(&theSavedCommandState))
		{
			try
			{
				inCommand->Execute(this);
			}
			catch(const CAException inException)
			{
				theError = inException.GetError();
			}
			catch(...)
			{
				theError = kAudioHardwareUnspecifiedError;
			}
			FinishCommandExecution(theSavedCommandState);
		}
		delete inCommand;
		if(theError != 0)
		{
			throw CAException(theError);
		}
	}
	else
	{
		//	we can't, so put it in the list for later execution
		mCommandList.insert(mCommandList.begin(), inCommand);
		
		//	make sure the shadow list has at least as much capacity as what's in the list
		mShadowCommandList.reserve(std::max(mShadowCommandList.capacity(), mCommandList.size()));
	}
}

void	HP_Device::ExecuteAllCommands()
{
	//	This routine must always be called at a time when
	//	it is safe to execute commands.
	
	//	clear out the shadow list to hold any commands that don't have permission to execute
	mShadowCommandList.clear();
	
	while(mCommandList.size() > 0)
	{
		HP_Command* theCommand = mCommandList.back();
		mCommandList.pop_back();
		
		try
		{
			if(IsPermittedToExecuteCommand(theCommand))
			{
				theCommand->Execute(this);
				delete theCommand;
			}
			else
			{
				mShadowCommandList.push_back(theCommand);
			}
		}
		catch(...)
		{
		}
	}
	
	//	if there is anything in the shadow list, put them back into the real list
	if(!mShadowCommandList.empty())
	{
		mCommandList.insert(mCommandList.begin(), mShadowCommandList.begin(), mShadowCommandList.end());
		mShadowCommandList.clear();
	}
}

void	HP_Device::ClearAllCommands()
{
	CommandList::iterator theIterator = mCommandList.begin();
	while(theIterator != mCommandList.end())
	{
		HP_Command* theCommand = *theIterator;
		
		delete theCommand;
		
		std::advance(theIterator, 1);
	}
	
	mCommandList.clear();
}

bool	HP_Device::IsSafeToExecuteCommand()
{
	//	Override this routine to determine whether or not a command can be executed immediately in
	//	the current context.
	return true;
}

bool	HP_Device::IsPermittedToExecuteCommand(HP_Command* /*inCommand*/)
{
	//	Override this method to determine whether or not the current command can
	//	executed immediately in the current context.  Note that this method assumes the context
	//	is safe as determined by IsSafeToExecuteCommand(). If a command isn't permitted to execute,
	//	it will be queued to be executed at the next available safe opportunity. Denying permission
	//	from off of the IO thread is a good way to force a command to be executing on the IO thread.
	return true;
}

bool	HP_Device::StartCommandExecution(void** /*outSavedCommandState*/)
{
	//	called prior to exectuting each command
	//	subclasses should override in order to lock
	//	any required locks and save any state in the provided pointer
	//	returns whether or not commands should be executed
	return true;
}

void	HP_Device::FinishCommandExecution(void* /*inSavedCommandState*/)
{
	//	called prior to exectuting each command
	//	subclasses should override in order to unlock
	//	any required locks using the state provided
}

AudioDeviceIOProcID	HP_Device::Do_CreateIOProcID(AudioDeviceIOProc inProc, void* inClientData)
{
	//	make sure the IOProc hasn't already been registered
	ThrowIf(mIOProcList->HasIOProc(inProc, inClientData, true), CAException(kAudioHardwareIllegalOperationError), "HP_Device::Do_CreateIOProcID: an IOProc ID has already been created for the given IOProc");
	
	//	We are bypassing the command machinery here because we have to return something
	//	sadly, this effectively means that we cannot defer this command
	AudioDeviceIOProcID theAnswer = NULL;
	
	//	see if we can execute the command immediately
	if(IsSafeToExecuteCommand())
	{
		OSStatus theError = 0;
		
		//	we can, so do so
		void* theSavedCommandState;
		if(StartCommandExecution(&theSavedCommandState))
		{
			try
			{
				theAnswer = mIOProcList->AddIOProc(inProc, inClientData, true);
			}
			catch(const CAException inException)
			{
				theError = inException.GetError();
			}
			catch(...)
			{
				theError = kAudioHardwareUnspecifiedError;
			}
			FinishCommandExecution(theSavedCommandState);
		}
		if(theError != 0)
		{
			throw CAException(theError);
		}
	}
	else
	{
		DebugMessage("HP_Device::Do_CreateIOProcID: can't create the IOProc ID in the current thread context");
		throw CAException(kAudioHardwareIllegalOperationError);
	}
	
	return theAnswer;
}

#if	defined(kAudioHardwarePlugInInterface5ID)

AudioDeviceIOProcID	HP_Device::Do_CreateIOProcIDWithBlock(dispatch_queue_t inDispatchQueue, AudioDeviceIOBlock inBlock)
{
	//	make sure the IOProc hasn't already been registered
	ThrowIf(mIOProcList->HasIOBlock(inDispatchQueue, inBlock), CAException(kAudioHardwareIllegalOperationError), "HP_Device::Do_CreateIOProcIDWithBlock: an IOProc ID has already been created for the given IOBlock");
	
	//	We are bypassing the command machinery here because we have to return something
	//	sadly, this effectively means that we cannot defer this command
	AudioDeviceIOProcID theAnswer = NULL;
	
	//	see if we can execute the command immediately
	if(IsSafeToExecuteCommand())
	{
		OSStatus theError = 0;
		
		//	we can, so do so
		void* theSavedCommandState;
		if(StartCommandExecution(&theSavedCommandState))
		{
			try
			{
				theAnswer = mIOProcList->AddIOBlock(inDispatchQueue, inBlock);
			}
			catch(const CAException inException)
			{
				theError = inException.GetError();
			}
			catch(...)
			{
				theError = kAudioHardwareUnspecifiedError;
			}
			FinishCommandExecution(theSavedCommandState);
		}
		if(theError != 0)
		{
			throw CAException(theError);
		}
	}
	else
	{
		DebugMessage("HP_Device::Do_CreateIOProcIDWithBlock: can't create the IOProc ID in the current thread context");
		throw CAException(kAudioHardwareIllegalOperationError);
	}
	
	return theAnswer;
}

#endif

void	HP_Device::Do_DestroyIOProcID(AudioDeviceIOProcID inIOProcID)
{
	if(mIOProcList->HasIOProcID(inIOProcID))
	{
		HP_Command* theCommand = new HP_RemoveIOProcCommand(inIOProcID);
		ExecuteCommand(theCommand);
	}
}

void	HP_Device::Do_AddIOProc(AudioDeviceIOProc inProc, void* inClientData)
{
	ThrowIf(mIOProcList->HasIOProc(inProc, inClientData, false), CAException(kAudioHardwareIllegalOperationError), "HP_Device::Do_AddIOProc: IOProc was already added");
	HP_Command* theCommand = new HP_AddIOProcCommand(inProc, inClientData);
	ExecuteCommand(theCommand);
}

void	HP_Device::AddIOProc(AudioDeviceIOProc inProc, void* inClientData)
{
	mIOProcList->AddIOProc(inProc, inClientData, false);
}

void	HP_Device::Do_RemoveIOProc(AudioDeviceIOProc inProc)
{
	if(mIOProcList->HasIOProcID(inProc))
	{
		HP_Command* theCommand = new HP_RemoveIOProcCommand(inProc);
		ExecuteCommand(theCommand);
	}
}

void	HP_Device::RemoveIOProc(AudioDeviceIOProc inProc)
{
	StopIOProc(inProc);
	mIOProcList->RemoveIOProc(inProc);
}

void	HP_Device::Do_StartIOProc(AudioDeviceIOProcID inProcID)
{
	if(inProcID != NULL)
	{
		ThrowIf(!mIOProcList->HasIOProcID(inProcID), CAException(kAudioHardwareIllegalOperationError), "HP_Device::Do_StartIOProc: IOProc wasn't previously added");
	}
	HP_Command* theCommand = new HP_StartIOProcCommand(inProcID);
	ExecuteCommand(theCommand);
}

void	HP_Device::StartIOProc(AudioDeviceIOProcID inProcID)
{
	mIOProcList->StartIOProc(inProcID);
	
	if(mIOProcList->IsOnlyOneThingEnabled())
	{
		//  inProcID was the first IOProc started, so the IO engine needs to be started now
		StartIOEngine();
	}
}

void	HP_Device::Do_StartIOProcAtTime(AudioDeviceIOProcID inProcID, AudioTimeStamp& ioStartTime, UInt32 inStartTimeFlags)
{
	if(inProcID == NULL)
	{
		Do_StartIOProc(inProcID);
	}
	else
	{
		ThrowIf(!mIOProcList->HasIOProcID(inProcID), CAException(kAudioHardwareIllegalOperationError), "HP_Device::Do_StartIOProcAtTime: IOProc wasn't previously added");
		ThrowIf(!mIOProcList->IsAnythingEnabled(), CAException(kAudioHardwareNotRunningError), "HP_Device::Do_StartIOProcAtTime: can't start because there isn't anything running yet");
		ThrowIf(!HasAnyStreams((inStartTimeFlags & kAudioDeviceStartTimeIsInputFlag) != 0), CAException(kAudioHardwareIllegalOperationError), "HP_Device::Do_StartIOProcAtTime: device doesn't have the given section");
		
		//	fix up the requested time so we have everything we need
		AudioTimeStamp theStartTime;
		theStartTime.mFlags = ioStartTime.mFlags | kAudioTimeStampSampleTimeValid | kAudioTimeStampHostTimeValid;
		TranslateTime(ioStartTime, theStartTime);
	
		//  need to ask about the start time
		GetNearestStartTime(theStartTime, inStartTimeFlags);
		ioStartTime = theStartTime;
		
		//	make a command and execute it
		HP_Command* theCommand = new HP_StartIOProcCommand(inProcID, ioStartTime, inStartTimeFlags);
		ExecuteCommand(theCommand);
	}
}

void	HP_Device::StartIOProcAtTime(AudioDeviceIOProcID inProcID, const AudioTimeStamp& inStartTime, UInt32 inStartTimeFlags)
{
	mIOProcList->StartIOProcAtTime(inProcID, inStartTime, inStartTimeFlags);
	
	if(mIOProcList->IsOnlyOneIOProcEnabled())
	{
		//  inProcID was the first IOProc started, so the IO engine needs to be started now
		StartIOEngineAtTime(inStartTime, inStartTimeFlags);
	}
}

void	HP_Device::Do_StopIOProc(AudioDeviceIOProcID inProcID)
{
	if(inProcID != NULL)
	{
		ThrowIf(!mIOProcList->HasIOProcID(inProcID), CAException(kAudioHardwareIllegalOperationError), "HP_Device::Do_StopIOProc: IOProc wasn't previously added");
	}
	HP_Command* theCommand = new HP_StopIOProcCommand(inProcID);
	ExecuteCommand(theCommand);
}

void	HP_Device::StopIOProc(AudioDeviceIOProcID inProcID)
{
	mIOProcList->StopIOProc(inProcID);
	
	if(mIOProcList->IsNothingEnabled())
	{
		//  inProcID was the first IOProc started, so the IO engine needs to be stopped now
		StopIOEngine();
	}
}

void	HP_Device::Do_StopAllIOProcs()
{
	HP_Command* theCommand = new HP_StopAllIOProcsCommand();
	ExecuteCommand(theCommand);
}

void	HP_Device::StopAllIOProcs()
{
	if(mIOProcList->IsAnythingEnabled())
	{
		mIOProcList->StopAllIOProcs();
		StopIOEngine();
	}
}

void	HP_Device::Do_SetIOProcStreamUsage(AudioDeviceIOProcID inProcID, bool inIsInput, UInt32 inNumberStreams, const UInt32 inStreamUsage[])
{
	ThrowIf(!mIOProcList->HasIOProcID(inProcID), CAException(kAudioHardwareIllegalOperationError), "HP_Device::Do_SetIOProcStreamUsage: IOProc wasn't previously added");
	HP_Command* theCommand = new HP_ChangeIOProcStreamUsageCommand(inProcID, inIsInput, inNumberStreams, inStreamUsage);
	ExecuteCommand(theCommand);
}

void	HP_Device::SetIOProcStreamUsage(AudioDeviceIOProcID inProcID, bool inIsInput, UInt32 inNumberStreams, const bool inStreamUsage[])
{
	mIOProcList->SetIOProcStreamUsage(inProcID, inIsInput, inNumberStreams, inStreamUsage);
}

void	HP_Device::CreateIOProcList()
{
	if(mIOProcList == NULL)
	{
		mIOProcList = new HP_IOProcList(this, mIOBufferSetID, mUseIOBuffers);
	}
}

void	HP_Device::DestroyIOProcList()
{
	delete mIOProcList;
	mIOProcList = NULL;
}

UInt32	HP_Device::GetLatency(bool /*inIsInput*/) const
{
	return 0;
}

UInt32	HP_Device::GetSafetyOffset(bool /*inIsInput*/) const
{
	return 0;
}

bool	HP_Device::IsValidIOBufferFrameSize(UInt32 inIOBufferFrameSize) const
{
	return (inIOBufferFrameSize >= GetMinimumIOBufferFrameSize()) && (inIOBufferFrameSize <= GetMaximumIOBufferFrameSize());
}

UInt32	HP_Device::GetMinimumIOBufferFrameSize() const
{
	UInt32 theAnswer = 2;
	
	if(!HasAnyNonLinearPCMStreams())
	{
		//	no non-mixable streams
		theAnswer = static_cast<UInt32>(ceil(GetCurrentNominalSampleRate() * 0.000300));
	}
	else
	{
		//	at least one non-mixable stream, so assume the current buffer size is the only available size
		//	and has been set properly elsewhere
		theAnswer = mIOBufferFrameSize;
	}
	return theAnswer;
}

UInt32	HP_Device::GetMaximumIOBufferFrameSize() const
{
	UInt32 theAnswer = 4096;
	if(!HasAnyNonLinearPCMStreams())
	{
		//	no non-mixable streams
		theAnswer = static_cast<UInt32>(ceil(GetCurrentNominalSampleRate() * 0.250000));
	}
	else
	{
		//	at least one non-mixable stream, so assume the current buffer size is the only available size
		//	and has been set properly elsewhere
		theAnswer = mIOBufferFrameSize;
	}
	return theAnswer;
}

UInt32	HP_Device::GetIOBufferFrameSizePadding() const
{
	//	this method returns the amount of extra frames of data that should be allocated for each stream
	//	over and above the IO buffer frame size. This is useful for the aggregate devices' need to
	//	have available more space for synch operations.
	return 0;
}

UInt32	HP_Device::DetermineIOBufferFrameSize() const
{
	//	This method is called to figure out if any streams have any restrictions on the IO buffer size.
	
	//	start with the current size
	UInt32 theAnswer = mIOBufferFrameSize;
	
	//	range it against the min and max buffer size
	theAnswer = std::min(theAnswer, GetMaximumIOBufferFrameSize());
	theAnswer = std::max(theAnswer, GetMinimumIOBufferFrameSize());
	
	//	iterate through the streams to see if any of them require a specific IO buffer size
	bool isDone = false;
	for(UInt32 theSection = 0; !isDone && (theSection < 2); ++theSection)
	{
		//	calculate the section
		bool isInput = theSection != 0;
		
		//	iterate through the streams
		UInt32 theNumberStreams = GetNumberStreams(isInput);
		for(UInt32 theStreamIndex = 0; !isDone && (theStreamIndex < theNumberStreams); ++theStreamIndex)
		{
			//	get the stream
			HP_Stream* theStream = GetStreamByIndex(isInput, theStreamIndex);
			
			//	get the stream format
			AudioStreamBasicDescription theFormat;
			theStream->GetCurrentPhysicalFormat(theFormat);
			
			//	All non-linear PCM formats are restricted to one packet of IO at a time.
			if(theFormat.mFormatID != kAudioFormatLinearPCM)
			{
				isDone = true;
				theAnswer = theFormat.mFramesPerPacket;
			}
		}
	}
	
	return theAnswer;
}

void	HP_Device::Do_SetIOBufferFrameSize(UInt32 inIOBufferFrameSize)
{
	ThrowIf(!IsValidIOBufferFrameSize(inIOBufferFrameSize), CAException(kAudioHardwareIllegalOperationError), "HP_Device::Do_SetIOBufferFrameSize: buffer size isn't valid");
	if(inIOBufferFrameSize != mIOBufferFrameSize)
	{
		HP_Command* theCommand = new HP_ChangeBufferSizeCommand(inIOBufferFrameSize, true);
		ExecuteCommand(theCommand);
	}
}

void	HP_Device::Do_SetQuietIOBufferFrameSize(UInt32 inIOBufferFrameSize)
{
	ThrowIf(!IsValidIOBufferFrameSize(inIOBufferFrameSize), CAException(kAudioHardwareIllegalOperationError), "HP_Device::Do_SetQuietIOBufferFrameSize: buffer size isn't valid");
	if(inIOBufferFrameSize != mIOBufferFrameSize)
	{
		HP_Command* theCommand = new HP_ChangeBufferSizeCommand(inIOBufferFrameSize, false);
		ExecuteCommand(theCommand);
	}
}

void	HP_Device::SetIOBufferFrameSize(UInt32 inIOBufferFrameSize, bool inSendNotifications)
{
	#if	Log_BufferSizeChanges
		DebugMessageN1("HP_Device::SetIOBufferFrameSize: requested buffer size is %lu", inIOBufferFrameSize);
	#endif

	if(mIOBufferFrameSize != inIOBufferFrameSize)
	{
		#if	Log_BufferSizeChanges
			DebugMessageN1("HP_Device::Do_SetIOBufferFrameSize: changing the buffer size to %lu", inIOBufferFrameSize);
		#endif

		//	mark the telemetry
#if Use_HAL_Telemetry
		mIOCycleTelemetry->IOBufferSizeChangeBegin(GetIOCycleNumber(), inIOBufferFrameSize);
#else
		HAL_IOBUFFERSIZECHANGEBEGIN(GetIOCycleNumber(), inIOBufferFrameSize);
#endif
		//  do the work
		mIOBufferFrameSize = inIOBufferFrameSize;
		if(mIOProcList != NULL)
		{
			mIOProcList->ReallocateAllIOProcBufferLists();
		}
		
		//	there's less work to do 
		if(inSendNotifications)
		{
			//  make a list to hold the notifications
			CAPropertyAddressList theChangedProperties;
			
			//  add the global scoped notifications, since they always apply
			CAPropertyAddress theAddress(kAudioDevicePropertyBufferSize, kAudioObjectPropertyScopeGlobal, 0);
			theChangedProperties.AppendItem(theAddress);
			theAddress.mSelector = kAudioDevicePropertyBufferFrameSize;
			theChangedProperties.AppendItem(theAddress);
			
			//  allow subclasses to do work
			IOBufferFrameSizeChanged(true, &theChangedProperties);
			
			//	note that we need to be sure that the device state lock is not held while we call the listeners
			bool ownsIOGuard = false;
			CAGuard* theIOGuard = GetIOGuard();
			if(theIOGuard != NULL)
			{
				ownsIOGuard = theIOGuard->IsOwnedByCurrentThread();
				if(ownsIOGuard)
				{
					theIOGuard->Unlock();
				}
			}
			
			//  send the notifications
			PropertiesChanged(theChangedProperties.GetNumberItems(), theChangedProperties.GetItems());
			
			//	re-lock the mutex
			if((theIOGuard != NULL) && ownsIOGuard)
			{
				theIOGuard->Lock();
			}
		}
		else
		{
			//  allow subclasses to do work
			IOBufferFrameSizeChanged(false, NULL);
		}
		
		//	mark the telemetry
#if Use_HAL_Telemetry
		mIOCycleTelemetry->IOBufferSizeChangeEnd(GetIOCycleNumber());
#else
		HAL_IOBUFFERSIZECHANGEEND(GetIOCycleNumber());
#endif
	}
}

void	HP_Device::CalculateIOThreadTimeConstraints(UInt64& outPeriod, UInt32& outQuanta)
{
	static const UInt64	kLowLatencyThreshhold = 1500ULL * 1000ULL;
	static const UInt64	kMedLatencyThreshhold = 4444ULL * 1000ULL;
	static UInt32		kLowLatencyComputeQuantum = 0;
	static UInt32		kMedLatencyComputeQuantum = 0;
	static UInt32		kHighLatencyComputeQuantum = 0;
	
	if(kLowLatencyComputeQuantum == 0)
	{
		kLowLatencyComputeQuantum = static_cast<UInt32>(CAHostTimeBase::ConvertFromNanos(500 * 1000));
		kMedLatencyComputeQuantum = static_cast<UInt32>(CAHostTimeBase::ConvertFromNanos(300 * 1000));
		kHighLatencyComputeQuantum = static_cast<UInt32>(CAHostTimeBase::ConvertFromNanos(100 * 1000));
	}
	
	outPeriod = static_cast<UInt64>((static_cast<Float64>(GetIOBufferFrameSize()) / GetCurrentActualSampleRate()) * CAHostTimeBase::GetFrequency());
	UInt64 thePeriodNanos = CAHostTimeBase::ConvertToNanos(outPeriod);
	outQuanta = kHighLatencyComputeQuantum;
	
	if(thePeriodNanos < kLowLatencyThreshhold)
	{
		outQuanta = kLowLatencyComputeQuantum;
	}
	else if(thePeriodNanos < kMedLatencyThreshhold)
	{
		outQuanta = kMedLatencyComputeQuantum;
	}
	
	if(outQuanta > thePeriodNanos)
	{
		outQuanta = static_cast<UInt32>(outPeriod);
	}
}

bool	HP_Device::IsIOEngineRunningSomewhere() const
{
	return IsIOEngineRunning();
}

CAGuard*	HP_Device::GetIOGuard()
{
	//	this method returns the CAGuard that is to be used to synchronize with the IO cycle
	//	by default, there is no CAGuard to synchronize with
	return NULL;
}

void	HP_Device::Read(const AudioTimeStamp& /*inStartTime*/, AudioBufferList* /*outData*/)
{
	DebugMessage("HP_Device::Read: AudioDeviceRead is not supported by this device");
	throw CAException(kAudioHardwareUnsupportedOperationError);
}

bool	HP_Device::CallIOProcs(const AudioTimeStamp& /*inCurrentTime*/, const AudioTimeStamp& /*inInputTime*/, const AudioTimeStamp& /*inOutputTime*/)
{
	//	This routine is called to perform IO using the provided times.
	//	It returns true if IO succeeded and false if it fails
	return true;
}

void	HP_Device::IOBufferFrameSizeChanged(bool /*inSendNotifications*/, CAPropertyAddressList* /*outChangedProperties*/)
{
}

void	HP_Device::StartIOEngine()
{
	mIOEngineIsRunning = true;
}

void	HP_Device::StartIOEngineAtTime(const AudioTimeStamp& /*inStartTime*/, UInt32 /*inStartTimeFlags*/)
{
	mIOEngineIsRunning = true;
}

void	HP_Device::StopIOEngine()
{
	mIOEngineIsRunning = false;
}

UInt32  HP_Device::GetDefaultIOBufferSizeForSampleRate(Float64 inSampleRate)
{
	UInt32 theAnswer = 4096;
	
	if(inSampleRate < 32000.0)
	{
		theAnswer = 256;
	}
	else if(inSampleRate < 64000.0)
	{
		theAnswer = 512;
	}
	else if(inSampleRate < 128000.0)
	{
		theAnswer = 1024;
	}
	else if(inSampleRate < 256000.0)
	{
		theAnswer = 2048;
	}

	return theAnswer;
}

UInt32	HP_Device::GetIOCycleNumber() const
{
	return 0;
}

void	HP_Device::GetCurrentTime(AudioTimeStamp& /*outTime*/)
{
}

void	HP_Device::SafeGetCurrentTime(AudioTimeStamp& /*outTime*/)
{
}

void	HP_Device::TranslateTime(const AudioTimeStamp& /*inTime*/, AudioTimeStamp& /*outTime*/)
{
}

void	HP_Device::GetNearestStartTime(AudioTimeStamp& /*ioStartTime*/, UInt32 /*inStartTimeFlags*/)
{
}

Float64	HP_Device::GetCurrentNominalSampleRate() const
{
	//	all streams have to have the same sample rate, so ask the first stream
	Float64 theAnswer = 0.0;
	HP_Stream* theStream = NULL;
	
	if(HasInputStreams())
	{
		theStream = GetStreamByIndex(true, 0);
	}
	else if(HasOutputStreams())
	{
		theStream = GetStreamByIndex(false, 0);
	}
	
	if(theStream != NULL)
	{
		theAnswer = theStream->GetCurrentNominalSampleRate();
	}
	
	return theAnswer;
}

Float64	HP_Device::GetCurrentActualSampleRate() const
{
	//	This routine should return the measured sample rate of the device when IO is running.
	Float64 theAnswer = 0.0;
	if(IsIOEngineRunning())
	{
		theAnswer = GetCurrentNominalSampleRate();
	}
	return theAnswer;
}

void	HP_Device::StartIOCycleTimingServices()
{
	//	Note that the IOGuard is _not_ held during this call!
	
	//	This method is called when an IO cycle is in it's initialization phase
	//	prior to it requiring any timing services. The device's timing services
	//	should be initialized when this method returns.
}

bool	HP_Device::EstablishIOCycleAnchorTime(AudioTimeStamp& outAnchorTime)
{
	//	Note that the IOGuard is held during this call!
	
	//	This method is called when an IO cycle needs to anchor itself.
	//	If outAnchorTime is set to zero, it means start as soon as possible.
	//	If it isn't, then it has been preset so that the first IO cycle occurs starting
	//	on a particular sample edge.
	Assert((outAnchorTime.mFlags & kAudioTimeStampSampleHostTimeValid) == kAudioTimeStampSampleHostTimeValid, "HP_Device::EstablishIOCycleAnchorTime: both the sample time and the host time need to be valid for return value");
	if(outAnchorTime.mSampleTime == 0)
	{
		GetCurrentTime(outAnchorTime);
	}
	
	//	return whether or not the anchor time could be established
	return true;
}

bool	HP_Device::UpdateIOCycleTimingServices()
{
	//	Note that the IOGuard is held during this call!
	
	//	This method is called by an IO cycle when it's cycle starts.
	//	The return value should indicate whether or not the cycle needs to be resynchronized.
	return false;
}

void	HP_Device::StopIOCycleTimingServices()
{
	//	Note that the IOGuard is held during this call!
	
	//	This method is called when an IO cycle has completed it's run and
	//	is tearing down.
}

bool	HP_Device::HasAnyNonLinearPCMStreams(bool inIsInput) const
{
	bool theAnswer = false;
	
	UInt32 theNumberStreams = GetNumberStreams(inIsInput);
	for(UInt32 theStreamIndex = 0; !theAnswer && (theStreamIndex < theNumberStreams); ++theStreamIndex)
	{
		HP_Stream* theStream = GetStreamByIndex(inIsInput, theStreamIndex);
		theAnswer = !theStream->IsLinearPCM();
	}
	
	return theAnswer;
}

bool	HP_Device::HasAnyNonMixableStreams(bool inIsInput) const
{
	bool theAnswer = false;
	
	UInt32 theNumberStreams = GetNumberStreams(inIsInput);
	for(UInt32 theStreamIndex = 0; !theAnswer && (theStreamIndex < theNumberStreams); ++theStreamIndex)
	{
		HP_Stream* theStream = GetStreamByIndex(inIsInput, theStreamIndex);
		theAnswer = !theStream->IsMixable();
	}
	
	return theAnswer;
}

bool	HP_Device::CanSetIsMixable(bool inIsInput) const
{
	bool theAnswer = false;
	
	UInt32 theNumberStreams = GetNumberStreams(inIsInput);
	for(UInt32 theStreamIndex = 0; !theAnswer && (theStreamIndex < theNumberStreams); ++theStreamIndex)
	{
		HP_Stream* theStream = GetStreamByIndex(inIsInput, theStreamIndex);
		theAnswer = theStream->CanSetIsMixable();
	}
	
	return theAnswer;
}

HP_Stream*	HP_Device::GetStreamByIndex(bool inIsInput, UInt32 inIndex) const
{
	//  initialize the return value
	HP_Stream* theAnswer = NULL;
	
	//  figure which stream list is involved
	const StreamList& theStreamList = inIsInput ? mInputStreamList : mOutputStreamList;
	
	//  make sure the index is in the proper range
	if(inIndex < theStreamList.size())
	{
		//  get the stream
		theAnswer = theStreamList.at(inIndex);
	}
	
	return theAnswer;
}

HP_Stream*	HP_Device::GetStreamByDeviceChannel(bool inIsInput, UInt32 inDeviceChannel) const
{
	//  initialize the return value
	HP_Stream* theAnswer = NULL;
	
	//  figure which stream list is involved
	const StreamList& theStreamList = inIsInput ? mInputStreamList : mOutputStreamList;
	
	//  iterate through the streams
	StreamList::const_iterator theIterator = theStreamList.begin();
	while((theIterator != theStreamList.end()) && (theAnswer == NULL))
	{
		//  get the stream
		HP_Stream* theStream = *theIterator;
		
		//  figure out if the device channel falls in it's range
		if(inDeviceChannel < (theStream->GetStartingDeviceChannelNumber() + theStream->GetCurrentNumberChannels()))
		{
			//  it does, set the return value
			theAnswer = theStream;
		}
		else
		{
			//  it doesn't, get the next item
			std::advance(theIterator, 1);
		}
	}
	
	return theAnswer;
}

HP_Stream*	HP_Device::GetStreamByPropertyAddress(const AudioObjectPropertyAddress& inAddress, bool inTryRealHard) const
{
	//  initialize the return value
	HP_Stream* theAnswer = NULL;
	
	//  figure which stream list is involved
	const StreamList* theStreamList = NULL;
	switch(inAddress.mScope)
	{
		case kAudioDevicePropertyScopeInput:
			theStreamList = &mInputStreamList;
			break;
			
		case kAudioDevicePropertyScopeOutput:
			theStreamList = &mOutputStreamList;
			break;
		
		case kAudioObjectPropertyScopeGlobal:
			if(mOutputStreamList.size() > 0)
			{
				theStreamList = &mOutputStreamList;
			}
			else if(mInputStreamList.size() > 0)
			{
				theStreamList = &mInputStreamList;
			}
			break;
	};
	
	//  iterate through the streams
	if(theStreamList != NULL)
	{
		StreamList::const_iterator theIterator = theStreamList->begin();
		while((theIterator != theStreamList->end()) && (theAnswer == NULL))
		{
			//  get the stream
			HP_Stream* theStream = *theIterator;
			
			//  figure out if the device channel falls in it's range
			if(inAddress.mElement < (theStream->GetStartingDeviceChannelNumber() + theStream->GetCurrentNumberChannels()))
			{
				//  it does, set the return value
				theAnswer = theStream;
			}
			else
			{
				//  it doesn't, get the next item
				std::advance(theIterator, 1);
			}
		}
	}
	
	//	if we still haven't found a stream and we've been told to try hard, just pick one.
	if(inTryRealHard && (theAnswer == NULL))
	{
		if(HasOutputStreams())
		{
			theAnswer = GetStreamByIndex(false, 0);
		}
		else if(HasInputStreams())
		{
			theAnswer = GetStreamByIndex(true, 0);
		}
	}
	
	return theAnswer;
}

UInt32	HP_Device::GetTotalNumberChannels(bool inIsInput) const
{
	//  initialize the return value
	UInt32 theAnswer = 0;
	
	//  figure which stream list is involved
	const StreamList& theStreamList = inIsInput ? mInputStreamList : mOutputStreamList;
	
	//  iterate through the streams
	StreamList::const_iterator theIterator = theStreamList.begin();
	while(theIterator != theStreamList.end())
	{
		//  get the stream
		HP_Stream* theStream = *theIterator;
		
		//  add it's number of channels
		theAnswer += theStream->GetCurrentNumberChannels();
		
		//  get the next one
		std::advance(theIterator, 1);
	}
	
	return theAnswer;
}

void	HP_Device::AddStream(HP_Stream* inStream)
{
	//  figure which stream list is involved
	StreamList& theStreamList = inStream->IsInput() ? mInputStreamList : mOutputStreamList;
	
	//	find the first stream with a starting channel number greater than the one being added
	UInt32 theStartingChannelNumber = inStream->GetStartingDeviceChannelNumber();
	StreamList::iterator theIterator = theStreamList.begin();
	while((theIterator != theStreamList.end()) && ((*theIterator)->GetStartingDeviceChannelNumber() < theStartingChannelNumber))
	{
		std::advance(theIterator, 1);
	}
	
	//	shove the new stream in front of it
	theStreamList.insert(theIterator, inStream);
}

void	HP_Device::RemoveStream(HP_Stream* inStream)
{
	//  figure which stream list is involved
	StreamList& theStreamList = inStream->IsInput() ? mInputStreamList : mOutputStreamList;
	
	//	find the stream in the list
	StreamList::iterator theIterator = std::find(theStreamList.begin(), theStreamList.end(), inStream);
	if(theIterator != theStreamList.end())
	{
		//	and remove it
		theStreamList.erase(theIterator);
	}
}

HP_Control*	HP_Device::GetControlByAddress(const AudioObjectPropertyAddress& inDeviceAddress) const
{
	HP_Control* theAnswer = NULL;
	
	//	figure out which control to look for
	AudioClassID theControlClassID = 0;
	AudioObjectPropertyScope theControlScope = 0;
	AudioObjectPropertyElement theControlElement = 0;
	ConvertDeviceAddressToControlAddress(inDeviceAddress, theControlClassID, theControlScope, theControlElement);
	
	//	look for it in the list
	ControlList::const_iterator theControlIterator = mControlList.begin();
	while((theAnswer == NULL) && (theControlIterator != mControlList.end()))
	{
		//	get the control
		HP_Control* theCurrentControl = *theControlIterator;
		
		//	get the info about it
		AudioClassID theCurrentControlClassID = theCurrentControl->GetClassID();
		AudioObjectPropertyScope theCurrentControlScope = theCurrentControl->GetPropertyScope();
		AudioObjectPropertyElement theCurrentControlElement = theCurrentControl->GetPropertyElement();
		
		//	see if it's the one we want
		if((theControlClassID != kAudioClockSourceControlClassID) || (theControlElement != 0))
		{
			if((theCurrentControlClassID == theControlClassID) && (theCurrentControlScope == theControlScope) && (theCurrentControlElement == theControlElement))
			{
				//	it is
				theAnswer = theCurrentControl;
			}
			else
			{
				//	go to the next one
				std::advance(theControlIterator, 1);
			}
		}
		else
		{
			//	master clock control are always global regardless of their scope
			if((theCurrentControlClassID == theControlClassID) && (theCurrentControlElement == theControlElement))
			{
				//	it is
				theAnswer = theCurrentControl;
			}
			else
			{
				//	go to the next one
				std::advance(theControlIterator, 1);
			}
		}
	}
	
	return theAnswer;
}

void	HP_Device::ConvertDeviceAddressToControlAddress(const AudioObjectPropertyAddress& inDeviceAddress, AudioClassID& outControlClassID, AudioObjectPropertyScope& outControlScope, AudioObjectPropertyElement& outControlElement) const
{
	outControlScope = inDeviceAddress.mScope;
	outControlElement = inDeviceAddress.mElement;
	
	switch(inDeviceAddress.mSelector)
	{
		case kAudioDevicePropertyJackIsConnected:
			outControlClassID = kAudioJackControlClassID;
			break;
		
		case kAudioDevicePropertyVolumeScalar:
		case kAudioDevicePropertyVolumeDecibels:
		case kAudioDevicePropertyVolumeRangeDecibels:
		case kAudioDevicePropertyVolumeScalarToDecibels:
		case kAudioDevicePropertyVolumeDecibelsToScalar:
		case 'vctf':	//	kAudioDevicePropertyVolumeDecibelsToScalarTransferFunction
			outControlClassID = kAudioVolumeControlClassID;
			break;
		
		case kAudioDevicePropertyStereoPan:
		case kAudioDevicePropertyStereoPanChannels:
			outControlClassID = kAudioStereoPanControlClassID;
			break;
		
		case kAudioDevicePropertyMute:
			outControlClassID = kAudioMuteControlClassID;
			break;
		
		case kAudioDevicePropertySolo:
			outControlClassID = kAudioSoloControlClassID;
			break;
		
		case kAudioDevicePropertyDataSource:
		case kAudioDevicePropertyDataSources:
		case kAudioDevicePropertyDataSourceNameForIDCFString:
		case kAudioDevicePropertyDataSourceNameForID:
			outControlClassID = kAudioDataSourceControlClassID;
			break;
		
		case kAudioDevicePropertyClockSource:
		case kAudioDevicePropertyClockSources:
		case kAudioDevicePropertyClockSourceNameForIDCFString:
		case kAudioDevicePropertyClockSourceNameForID:
		case kAudioDevicePropertyClockSourceKindForID:
			outControlClassID = kAudioClockSourceControlClassID;
			break;
		
		case kAudioDevicePropertyPlayThru:
			outControlScope = kAudioDevicePropertyScopePlayThrough;
			outControlClassID = kAudioMuteControlClassID;
			break;
		
		case kAudioDevicePropertyPlayThruSolo:
			outControlScope = kAudioDevicePropertyScopePlayThrough;
			outControlClassID = kAudioSoloControlClassID;
			break;
		
		case kAudioDevicePropertyPlayThruVolumeScalar:
		case kAudioDevicePropertyPlayThruVolumeDecibels:
		case kAudioDevicePropertyPlayThruVolumeRangeDecibels:
		case kAudioDevicePropertyPlayThruVolumeScalarToDecibels:
		case kAudioDevicePropertyPlayThruVolumeDecibelsToScalar:
		case 'mvtf':	//	kAudioDevicePropertyPlayThruVolumeDecibelsToScalarTransferFunction
			outControlScope = kAudioDevicePropertyScopePlayThrough;
			outControlClassID = kAudioVolumeControlClassID;
			break;
		
		case kAudioDevicePropertyPlayThruStereoPan:
		case kAudioDevicePropertyPlayThruStereoPanChannels:
			outControlScope = kAudioDevicePropertyScopePlayThrough;
			outControlClassID = kAudioStereoPanControlClassID;
			break;
		
		case kAudioDevicePropertyPlayThruDestination:
		case kAudioDevicePropertyPlayThruDestinations:
		case kAudioDevicePropertyPlayThruDestinationNameForIDCFString:
		case kAudioDevicePropertyPlayThruDestinationNameForID:
			outControlScope = kAudioDevicePropertyScopePlayThrough;
			outControlClassID = kAudioDataDestinationControlClassID;
			break;
		
		case kAudioDevicePropertyChannelNominalLineLevel:
		case kAudioDevicePropertyChannelNominalLineLevels:
		case kAudioDevicePropertyChannelNominalLineLevelNameForIDCFString:
		case kAudioDevicePropertyChannelNominalLineLevelNameForID:
			outControlClassID = kAudioLineLevelControlClassID;
			break;
		
		case kAudioDevicePropertyDriverShouldOwniSub:
			outControlClassID = kAudioISubOwnerControlClassID;
			break;
		
		case kAudioDevicePropertySubVolumeScalar:
		case kAudioDevicePropertySubVolumeDecibels:
		case kAudioDevicePropertySubVolumeRangeDecibels:
		case kAudioDevicePropertySubVolumeScalarToDecibels:
		case kAudioDevicePropertySubVolumeDecibelsToScalar:
		case 'svtf':	//	kAudioDevicePropertySubVolumeDecibelsToScalarTransferFunction
			outControlClassID = kAudioLFEVolumeControlClassID;
			break;
		
		case kAudioDevicePropertySubMute:
			outControlClassID = kAudioLFEMuteControlClassID;
			break;
		
		case kAudioHardwarePropertyBootChimeVolumeScalar:
		case kAudioHardwarePropertyBootChimeVolumeDecibels:
		case kAudioHardwarePropertyBootChimeVolumeRangeDecibels:
		case kAudioHardwarePropertyBootChimeVolumeScalarToDecibels:
		case kAudioHardwarePropertyBootChimeVolumeDecibelsToScalar:
		case 'bvtf':	//	kAudioHardwarePropertyBootChimeVolumeDecibelsToScalarTransferFunction
			outControlClassID = kAudioBootChimeVolumeControlClassID;
			break;
	};
}

AudioObjectPropertySelector	HP_Device::ConvertDeviceSelectorToControlSelector(AudioObjectPropertySelector inDeviceSelector) const
{
	AudioObjectPropertySelector theAnswer = 0;
	
	switch(inDeviceSelector)
	{
		case kAudioDevicePropertyJackIsConnected:
			theAnswer = kAudioBooleanControlPropertyValue;
			break;
		
		case kAudioDevicePropertyVolumeScalar:
			theAnswer = kAudioLevelControlPropertyScalarValue;
			break;
		
		case kAudioDevicePropertyVolumeDecibels:
			theAnswer = kAudioLevelControlPropertyDecibelValue;
			break;
		
		case kAudioDevicePropertyVolumeRangeDecibels:
			theAnswer = kAudioLevelControlPropertyDecibelRange;
			break;
		
		case kAudioDevicePropertyVolumeScalarToDecibels:
			theAnswer = kAudioLevelControlPropertyConvertScalarToDecibels;
			break;
		
		case kAudioDevicePropertyVolumeDecibelsToScalar:
			theAnswer = kAudioLevelControlPropertyConvertDecibelsToScalar;
			break;
		
		case 'vctf':	//	kAudioDevicePropertyVolumeDecibelsToScalarTransferFunction
			theAnswer = 'lctf';	//	kAudioLevelControlPropertyDecibelsToScalarTransferFunction
			break;
		
		case kAudioDevicePropertyStereoPan:
			theAnswer = kAudioStereoPanControlPropertyValue;
			break;
		
		case kAudioDevicePropertyStereoPanChannels:
			theAnswer = kAudioStereoPanControlPropertyPanningChannels;
			break;
		
		case kAudioDevicePropertyMute:
			theAnswer = kAudioBooleanControlPropertyValue;
			break;
		
		case kAudioDevicePropertySolo:
			theAnswer = kAudioBooleanControlPropertyValue;
			break;
		
		case kAudioDevicePropertyDataSource:
			theAnswer = kAudioSelectorControlPropertyCurrentItem;
			break;
		
		case kAudioDevicePropertyDataSources:
			theAnswer = kAudioSelectorControlPropertyAvailableItems;
			break;
		
		case kAudioDevicePropertyDataSourceNameForIDCFString:
		case kAudioDevicePropertyDataSourceNameForID:
			theAnswer = kAudioSelectorControlPropertyItemName;
			break;
		
		case kAudioDevicePropertyClockSource:
			theAnswer = kAudioSelectorControlPropertyCurrentItem;
			break;
		
		case kAudioDevicePropertyClockSources:
			theAnswer = kAudioSelectorControlPropertyAvailableItems;
			break;
		
		case kAudioDevicePropertyClockSourceNameForIDCFString:
		case kAudioDevicePropertyClockSourceNameForID:
			theAnswer = kAudioSelectorControlPropertyItemName;
			break;
		
		case kAudioDevicePropertyClockSourceKindForID:
			theAnswer = kAudioClockSourceControlPropertyItemKind;
		
		case kAudioDevicePropertyPlayThru:
			theAnswer = kAudioBooleanControlPropertyValue;
			break;
		
		case kAudioDevicePropertyPlayThruSolo:
			theAnswer = kAudioBooleanControlPropertyValue;
			break;
		
		case kAudioDevicePropertyPlayThruVolumeScalar:
			theAnswer = kAudioLevelControlPropertyScalarValue;
			break;
		
		case kAudioDevicePropertyPlayThruVolumeDecibels:
			theAnswer = kAudioLevelControlPropertyDecibelValue;
			break;
		
		case kAudioDevicePropertyPlayThruVolumeRangeDecibels:
			theAnswer = kAudioLevelControlPropertyDecibelRange;
			break;
		
		case kAudioDevicePropertyPlayThruVolumeScalarToDecibels:
			theAnswer = kAudioLevelControlPropertyConvertScalarToDecibels;
			break;
		
		case kAudioDevicePropertyPlayThruVolumeDecibelsToScalar:
			theAnswer = kAudioLevelControlPropertyConvertDecibelsToScalar;
			break;
		
		case 'mvtf':	//	kAudioDevicePropertyPlayThruVolumeDecibelsToScalarTransferFunction
			theAnswer = 'lctf';	//	kAudioLevelControlPropertyDecibelsToScalarTransferFunction
			break;
		
		case kAudioDevicePropertyPlayThruStereoPan:
			theAnswer = kAudioStereoPanControlPropertyValue;
			break;
		
		case kAudioDevicePropertyPlayThruStereoPanChannels:
			theAnswer = kAudioStereoPanControlPropertyPanningChannels;
			break;
		
		case kAudioDevicePropertyPlayThruDestination:
			theAnswer = kAudioSelectorControlPropertyCurrentItem;
			break;
		
		case kAudioDevicePropertyPlayThruDestinations:
			theAnswer = kAudioSelectorControlPropertyAvailableItems;
			break;
		
		case kAudioDevicePropertyPlayThruDestinationNameForIDCFString:
		case kAudioDevicePropertyPlayThruDestinationNameForID:
			theAnswer = kAudioSelectorControlPropertyItemName;
			break;
		
		case kAudioDevicePropertyChannelNominalLineLevel:
			theAnswer = kAudioSelectorControlPropertyCurrentItem;
			break;
		
		case kAudioDevicePropertyChannelNominalLineLevels:
			theAnswer = kAudioSelectorControlPropertyAvailableItems;
			break;
		
		case kAudioDevicePropertyChannelNominalLineLevelNameForIDCFString:
		case kAudioDevicePropertyChannelNominalLineLevelNameForID:
			theAnswer = kAudioSelectorControlPropertyItemName;
			break;
		
		case kAudioDevicePropertyDriverShouldOwniSub:
			theAnswer = kAudioBooleanControlPropertyValue;
			break;
		
		case kAudioDevicePropertySubVolumeScalar:
			theAnswer = kAudioLevelControlPropertyScalarValue;
			break;
		
		case kAudioDevicePropertySubVolumeDecibels:
			theAnswer = kAudioLevelControlPropertyDecibelValue;
			break;
		
		case kAudioDevicePropertySubVolumeRangeDecibels:
			theAnswer = kAudioLevelControlPropertyDecibelRange;
			break;
		
		case kAudioDevicePropertySubVolumeScalarToDecibels:
			theAnswer = kAudioLevelControlPropertyConvertScalarToDecibels;
			break;
		
		case kAudioDevicePropertySubVolumeDecibelsToScalar:
			theAnswer = kAudioLevelControlPropertyConvertDecibelsToScalar;
			break;
		
		case 'svtf':	//	kAudioDevicePropertySubVolumeDecibelsToScalarTransferFunction
			theAnswer = 'lctf';	//	kAudioLevelControlPropertyDecibelsToScalarTransferFunction
			break;
		
		case kAudioDevicePropertySubMute:
			theAnswer = kAudioBooleanControlPropertyValue;
			break;
			
		case kAudioHardwarePropertyBootChimeVolumeScalar:
			theAnswer = kAudioLevelControlPropertyScalarValue;
			break;
		
		case kAudioHardwarePropertyBootChimeVolumeDecibels:
			theAnswer = kAudioLevelControlPropertyDecibelValue;
			break;
		
		case kAudioHardwarePropertyBootChimeVolumeRangeDecibels:
			theAnswer = kAudioLevelControlPropertyDecibelRange;
			break;
		
		case kAudioHardwarePropertyBootChimeVolumeScalarToDecibels:
			theAnswer = kAudioLevelControlPropertyConvertScalarToDecibels;
			break;
		
		case kAudioHardwarePropertyBootChimeVolumeDecibelsToScalar:
			theAnswer = kAudioLevelControlPropertyConvertDecibelsToScalar;
			break;
		
		case 'bvtf':	//	kAudioHardwarePropertyBootChimeVolumeDecibelsToScalarTransferFunction
			theAnswer = 'lctf';	//	kAudioLevelControlPropertyDecibelsToScalarTransferFunction
			break;
	};
	
	return theAnswer;
}

AudioObjectPropertySelector	HP_Device::GetPrimaryValueChangedPropertySelectorForControl(HP_Control* inControl) const
{
	AudioObjectPropertySelector theAnswer = 0;
	switch(inControl->GetClassID())
	{
		case kAudioStereoPanControlClassID:
			theAnswer = kAudioDevicePropertyStereoPan;
			break;
		
		case kAudioVolumeControlClassID:
			theAnswer = kAudioDevicePropertyVolumeScalar;
			break;
		
		case kAudioLFEVolumeControlClassID:
			theAnswer = kAudioDevicePropertySubVolumeScalar;
			break;
		
		case kAudioBootChimeVolumeControlClassID:
			theAnswer = kAudioHardwarePropertyBootChimeVolumeScalar;
			break;
		
		case kAudioMuteControlClassID:
			theAnswer = kAudioDevicePropertyMute;
			break;
		
		case kAudioSoloControlClassID:
			theAnswer = kAudioDevicePropertySolo;
			break;
		
		case kAudioJackControlClassID:
			theAnswer = kAudioDevicePropertyJackIsConnected;
			break;
		
		case kAudioLFEMuteControlClassID:
			theAnswer = kAudioDevicePropertySubMute;
			break;
		
		case kAudioISubOwnerControlClassID:
			theAnswer = kAudioDevicePropertyDriverShouldOwniSub;
			break;
		
		case kAudioDataSourceControlClassID:
			theAnswer = kAudioDevicePropertyDataSource;
			break;
		
		case kAudioDataDestinationControlClassID:
			theAnswer = kAudioDevicePropertyPlayThruDestination;
			break;
		
		case kAudioClockSourceControlClassID:
			theAnswer = kAudioDevicePropertyClockSource;
			break;
		
		case kAudioLineLevelControlClassID:
			theAnswer = kAudioDevicePropertyChannelNominalLineLevel;
			break;
	};
	return theAnswer;
}

AudioObjectPropertySelector	HP_Device::GetSecondaryValueChangedPropertySelectorForControl(HP_Control* inControl) const
{
	AudioObjectPropertySelector theAnswer = 0;
	switch(inControl->GetClassID())
	{
		case kAudioStereoPanControlClassID:
			if(inControl->GetPropertyScope() == kAudioDevicePropertyScopePlayThrough)
			{
				theAnswer = kAudioDevicePropertyPlayThruStereoPan;
			}
			break;
		case kAudioVolumeControlClassID:
			theAnswer = kAudioDevicePropertyVolumeDecibels;
			break;
		
		case kAudioLFEVolumeControlClassID:
			theAnswer = kAudioDevicePropertySubVolumeDecibels;
			break;
		
		case kAudioBootChimeVolumeControlClassID:
			theAnswer = kAudioHardwarePropertyBootChimeVolumeDecibels;
			break;
			
		case kAudioMuteControlClassID:
			if(inControl->GetPropertyScope() == kAudioDevicePropertyScopePlayThrough)
			{
				theAnswer = kAudioDevicePropertyPlayThru;
			}
			break;
		
		case kAudioSoloControlClassID:
			if(inControl->GetPropertyScope() == kAudioDevicePropertyScopePlayThrough)
			{
				theAnswer = kAudioDevicePropertyPlayThruSolo;
			}
			break;
			
	};
	return theAnswer;
}

AudioObjectPropertySelector	HP_Device::GetThirdValueChangedPropertySelectorForControl(HP_Control* inControl) const
{
	AudioObjectPropertySelector theAnswer = 0;
	switch(inControl->GetClassID())
	{
		case kAudioVolumeControlClassID:
			if(inControl->GetPropertyScope() == kAudioDevicePropertyScopePlayThrough)
			{
				theAnswer = kAudioDevicePropertyPlayThruVolumeScalar;
			}
			break;
	};
	return theAnswer;
}

AudioObjectPropertySelector	HP_Device::GetFourthValueChangedPropertySelectorForControl(HP_Control* inControl) const
{
	AudioObjectPropertySelector theAnswer = 0;
	switch(inControl->GetClassID())
	{
		case kAudioVolumeControlClassID:
			if(inControl->GetPropertyScope() == kAudioDevicePropertyScopePlayThrough)
			{
				theAnswer = kAudioDevicePropertyPlayThruVolumeDecibels;
			}
			break;
	};
	return theAnswer;
}

AudioObjectPropertyScope	HP_Device::GetPrimaryValueChangedPropertyScopeForControl(HP_Control* inControl) const
{
	AudioObjectPropertyScope theAnswer = 0;
	switch(inControl->GetClassID())
	{
		case kAudioStereoPanControlClassID:
			theAnswer = inControl->GetPropertyScope();
			break;
		
		case kAudioVolumeControlClassID:
			theAnswer = inControl->GetPropertyScope();
			break;
		
		case kAudioLFEVolumeControlClassID:
			theAnswer = inControl->GetPropertyScope();
			break;
		
		case kAudioBootChimeVolumeControlClassID:
			theAnswer = inControl->GetPropertyScope();
			break;
		
		case kAudioMuteControlClassID:
			theAnswer = inControl->GetPropertyScope();
			break;
		
		case kAudioSoloControlClassID:
			theAnswer = inControl->GetPropertyScope();
			break;
		
		case kAudioJackControlClassID:
			theAnswer = inControl->GetPropertyScope();
			break;
		
		case kAudioLFEMuteControlClassID:
			theAnswer = inControl->GetPropertyScope();
			break;
		
		case kAudioISubOwnerControlClassID:
			theAnswer = inControl->GetPropertyScope();
			break;
		
		case kAudioDataSourceControlClassID:
			theAnswer = inControl->GetPropertyScope();
			break;
		
		case kAudioDataDestinationControlClassID:
			theAnswer = inControl->GetPropertyScope();
			break;
		
		case kAudioClockSourceControlClassID:
			theAnswer = inControl->GetPropertyScope();
			break;
		
		case kAudioLineLevelControlClassID:
			theAnswer = inControl->GetPropertyScope();
			break;
	};
	return theAnswer;
}

AudioObjectPropertyScope	HP_Device::GetSecondaryValueChangedPropertyScopeForControl(HP_Control* inControl) const
{
	AudioObjectPropertyScope theAnswer = 0;
	switch(inControl->GetClassID())
	{
		case kAudioStereoPanControlClassID:
			if(inControl->GetPropertyScope() == kAudioDevicePropertyScopePlayThrough)
			{
				theAnswer = kAudioDevicePropertyScopeInput;
			}
			break;
		case kAudioVolumeControlClassID:
			theAnswer = inControl->GetPropertyScope();
			break;
		
		case kAudioLFEVolumeControlClassID:
			theAnswer = inControl->GetPropertyScope();
			break;
		
		case kAudioBootChimeVolumeControlClassID:
			theAnswer = inControl->GetPropertyScope();
			break;
			
		case kAudioMuteControlClassID:
			if(inControl->GetPropertyScope() == kAudioDevicePropertyScopePlayThrough)
			{
				theAnswer = kAudioDevicePropertyScopeInput;
			}
			break;
		
		case kAudioSoloControlClassID:
			if(inControl->GetPropertyScope() == kAudioDevicePropertyScopePlayThrough)
			{
				theAnswer = kAudioDevicePropertyScopeInput;
			}
			break;
			
	};
	return theAnswer;
}

AudioObjectPropertyScope	HP_Device::GetThirdValueChangedPropertyScopeForControl(HP_Control* inControl) const
{
	AudioObjectPropertyScope theAnswer = 0;
	switch(inControl->GetClassID())
	{
		case kAudioVolumeControlClassID:
			if(inControl->GetPropertyScope() == kAudioDevicePropertyScopePlayThrough)
			{
				theAnswer = kAudioDevicePropertyScopeInput;
			}
			break;
	};
	return theAnswer;
}

AudioObjectPropertyScope	HP_Device::GetFourthValueChangedPropertyScopeForControl(HP_Control* inControl) const
{
	AudioObjectPropertyScope theAnswer = 0;
	switch(inControl->GetClassID())
	{
		case kAudioVolumeControlClassID:
			if(inControl->GetPropertyScope() == kAudioDevicePropertyScopePlayThrough)
			{
				theAnswer = kAudioDevicePropertyScopeInput;
			}
			break;
	};
	return theAnswer;
}

AudioObjectPropertySelector	HP_Device::GetPrimaryRangeChangedPropertySelectorForControl(HP_Control* inControl) const
{
	AudioObjectPropertySelector theAnswer = 0;
	switch(inControl->GetClassID())
	{
		case kAudioStereoPanControlClassID:
			theAnswer = kAudioDevicePropertyStereoPanChannels;
			break;
		
		case kAudioVolumeControlClassID:
			theAnswer = kAudioDevicePropertyVolumeRangeDecibels;
			break;
		
		case kAudioLFEVolumeControlClassID:
			theAnswer = kAudioDevicePropertySubVolumeRangeDecibels;
			break;
		
		case kAudioBootChimeVolumeControlClassID:
			theAnswer = kAudioHardwarePropertyBootChimeVolumeRangeDecibels;
			break;
		
		case kAudioDataSourceControlClassID:
			theAnswer = kAudioDevicePropertyDataSources;
			break;
		
		case kAudioDataDestinationControlClassID:
			theAnswer = kAudioDevicePropertyPlayThruDestinations;
			break;
		
		case kAudioClockSourceControlClassID:
			theAnswer = kAudioDevicePropertyClockSources;
			break;
		
		case kAudioLineLevelControlClassID:
			theAnswer = kAudioDevicePropertyChannelNominalLineLevels;
			break;
	};
	return theAnswer;
}

AudioObjectPropertySelector	HP_Device::GetSecondaryRangeChangedPropertySelectorForControl(HP_Control* inControl) const
{
	AudioObjectPropertySelector theAnswer = 0;
	switch(inControl->GetClassID())
	{
		case kAudioStereoPanControlClassID:
			if(inControl->GetPropertyScope() == kAudioDevicePropertyScopePlayThrough)
			{
				theAnswer = kAudioDevicePropertyPlayThruStereoPanChannels;
			}
			break;
		
		case kAudioVolumeControlClassID:
			if(inControl->GetPropertyScope() == kAudioDevicePropertyScopePlayThrough)
			{
				theAnswer = kAudioDevicePropertyPlayThruVolumeRangeDecibels;
			}
			break;
	};
	return theAnswer;
}

AudioObjectPropertyScope	HP_Device::GetPrimaryRangeChangedPropertyScopeForControl(HP_Control* inControl) const
{
	AudioObjectPropertyScope theAnswer = 0;
	switch(inControl->GetClassID())
	{
		case kAudioStereoPanControlClassID:
			theAnswer = inControl->GetPropertyScope();
			break;
		
		case kAudioVolumeControlClassID:
			theAnswer = inControl->GetPropertyScope();
			break;
		
		case kAudioLFEVolumeControlClassID:
			theAnswer = inControl->GetPropertyScope();
			break;
		
		case kAudioBootChimeVolumeControlClassID:
			theAnswer = inControl->GetPropertyScope();
			break;
		
		case kAudioDataSourceControlClassID:
			theAnswer = inControl->GetPropertyScope();
			break;
		
		case kAudioDataDestinationControlClassID:
			theAnswer = inControl->GetPropertyScope();
			break;
		
		case kAudioClockSourceControlClassID:
			theAnswer = inControl->GetPropertyScope();
			break;
		
		case kAudioLineLevelControlClassID:
			theAnswer = inControl->GetPropertyScope();
			break;
	};
	return theAnswer;
}

AudioObjectPropertyScope	HP_Device::GetSecondaryRangeChangedPropertyScopeForControl(HP_Control* inControl) const
{
	AudioObjectPropertyScope theAnswer = 0;
	switch(inControl->GetClassID())
	{
		case kAudioStereoPanControlClassID:
			if(inControl->GetPropertyScope() == kAudioDevicePropertyScopePlayThrough)
			{
				theAnswer = kAudioDevicePropertyScopeInput;
			}
			break;
		
		case kAudioVolumeControlClassID:
			if(inControl->GetPropertyScope() == kAudioDevicePropertyScopePlayThrough)
			{
				theAnswer = kAudioDevicePropertyScopeInput;
			}
			break;
	};
	return theAnswer;
}

void	HP_Device::AddControl(HP_Control* inControl)
{
	mControlList.push_back(inControl);
}

void	HP_Device::RemoveControl(HP_Control* inControl)
{
	ControlList::iterator theIterator = std::find(mControlList.begin(), mControlList.end(), inControl);
	if(theIterator != mControlList.end())
	{
		mControlList.erase(theIterator);
	}
}

void	HP_Device::ClearControlMarks()
{
	ControlList::iterator theIterator = mControlList.begin();
	while(theIterator != mControlList.end())
	{
		(*theIterator)->SetMark(false);
		std::advance(theIterator, 1);
	}
}

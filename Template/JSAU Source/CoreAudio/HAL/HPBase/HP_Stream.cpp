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
#include "HP_Stream.h"

//	Local Includes
#include "HP_Device.h"
#include "HP_HardwarePlugIn.h"

//	PublicUtility Includes
#include "CACFString.h"
#include "CADebugMacros.h"
#include "CAException.h"
#include "CAMutex.h"

//==================================================================================================
//	HP_Stream
//==================================================================================================

HP_Stream::HP_Stream(AudioStreamID inAudioStreamID, HP_HardwarePlugIn* inPlugIn, HP_Device* inOwningDevice, bool inIsInput, UInt32 inStartingDeviceChannelNumber)
:
	HP_Object(inAudioStreamID, kAudioStreamClassID, inPlugIn),
	mOwningDevice(inOwningDevice),
	mIsInput(inIsInput),
	mStartingDeviceChannelNumber(inStartingDeviceChannelNumber),
	mStartingDeviceChannelNumberOffset(0),
	mFormatList(NULL)
{
}

HP_Stream::HP_Stream(AudioStreamID inAudioStreamID, HP_HardwarePlugIn* inPlugIn, HP_Device* inOwningDevice, bool inIsInput, UInt32 inStartingDeviceChannelNumber, UInt32 inStartingDeviceChannelNumberOffset)
:
	HP_Object(inAudioStreamID, kAudioStreamClassID, inPlugIn),
	mOwningDevice(inOwningDevice),
	mIsInput(inIsInput),
	mStartingDeviceChannelNumber(inStartingDeviceChannelNumber),
	mStartingDeviceChannelNumberOffset(inStartingDeviceChannelNumberOffset),
	mFormatList(NULL)
{
}

HP_Stream::~HP_Stream()
{
}

void	HP_Stream::Initialize()
{
	mFormatList = new HP_FormatList(this);
	AddProperty(mFormatList);
}

void	HP_Stream::Teardown()
{
	RemoveProperty(mFormatList);
	delete mFormatList;
	mFormatList = NULL;
}

CAMutex*	HP_Stream::GetObjectStateMutex()
{
	return mOwningDevice->GetObjectStateMutex();
}

CFStringRef	HP_Stream::CopyStreamName() const
{
	//	This routine should return a CFStringRef that contains a string
	//	that is the human readable and localized name of the stream.
	//	Note that the caller will CFRelease the returned object, so be
	//	sure that the object's ref count is correct.
	return NULL;
}

CFStringRef	HP_Stream::CopyStreamManufacturerName() const
{
	//	This routine should return a CFStringRef that contains a string
	//	that is the human readable and localized name of the stream's manufacturer.
	//	Note that the caller will CFRelease the returned object, so be
	//	sure that the object's ref count is correct.
	return GetOwningDevice()->CopyDeviceManufacturerName();
}

CFStringRef	HP_Stream::CopyElementFullName(const AudioObjectPropertyAddress& inAddress) const
{
	//	This routine returns a human readable, localized name of the given element
	//	this routine shouldn't throw an exception. Just return NULL if the value doesn't exist
	AudioObjectPropertyAddress theAddress = inAddress;
	ChangeStreamPropertyAddressIntoDevicePropertyAddress(theAddress);
	return GetOwningDevice()->CopyElementFullName(theAddress);
}

CFStringRef	HP_Stream::CopyElementCategoryName(const AudioObjectPropertyAddress& inAddress) const
{
	//	This routine returns a human readable, localized name of the category of the given element
	//	this routine shouldn't throw an exception. Just return NULL if the value doesn't exist
	AudioObjectPropertyAddress theAddress = inAddress;
	ChangeStreamPropertyAddressIntoDevicePropertyAddress(theAddress);
	return GetOwningDevice()->CopyElementCategoryName(theAddress);
}

CFStringRef	HP_Stream::CopyElementNumberName(const AudioObjectPropertyAddress& inAddress) const
{
	//	This routine returns a human readable, localized name of the number of the given element
	//	this routine shouldn't throw an exception. Just return NULL if the value doesn't exist
	AudioObjectPropertyAddress theAddress = inAddress;
	ChangeStreamPropertyAddressIntoDevicePropertyAddress(theAddress);
	return GetOwningDevice()->CopyElementNumberName(theAddress);
}

UInt32	HP_Stream::GetTerminalType() const
{
	return 0;
}

UInt32	HP_Stream::GetLatency() const
{
	return 0;
}

void	HP_Stream::ChangeDevicePropertyAddressIntoStreamPropertyAddress(AudioObjectPropertyAddress& ioAddress) const
{
	ioAddress.mScope = kAudioObjectPropertyScopeGlobal;
	ioAddress.mElement = (ioAddress.mElement == 0) ? 0 : ioAddress.mElement - mStartingDeviceChannelNumber + 1;
}

void	HP_Stream::ChangeStreamPropertyAddressIntoDevicePropertyAddress(AudioObjectPropertyAddress& ioAddress) const
{
	ioAddress.mScope = GetDevicePropertyScope();
	ioAddress.mElement = (ioAddress.mElement == 0) ? 0 : ioAddress.mElement + mStartingDeviceChannelNumber - 1;
}

bool	HP_Stream::HasProperty(const AudioObjectPropertyAddress& inAddress) const
{
	//	initialize the return value
	bool theAnswer = false;
	
	//	initialize some commonly used variables
	CFStringRef theCFString = NULL;
	
	//	take and hold the state mutex
	CAMutex::Locker theStateMutex(const_cast<HP_Device*>(mOwningDevice)->GetDeviceStateMutex());
	
	switch(inAddress.mSelector)
	{
		case kAudioObjectPropertyName:
			theCFString = CopyStreamName();
			if(theCFString != NULL)
			{
				theAnswer = true;
				CFRelease(theCFString);
			}
			break;
			
		case kAudioObjectPropertyManufacturer:
			theCFString = CopyStreamManufacturerName();
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
			
		case kAudioStreamPropertyDirection:
			theAnswer = true;
			break;
			
		case kAudioStreamPropertyTerminalType:
			theAnswer = true;
			break;
			
		case kAudioStreamPropertyStartingChannel:
			theAnswer = true;
			break;
			
		case kAudioStreamPropertyLatency:
			theAnswer = true;
			break;
			
		default:
			theAnswer = HP_Object::HasProperty(inAddress);
			break;
	};
	
	return theAnswer;
}

bool	HP_Stream::IsPropertySettable(const AudioObjectPropertyAddress& inAddress) const
{
	bool theAnswer = false;
	
	//	take and hold the state mutex
	CAMutex::Locker theStateMutex(const_cast<HP_Device*>(mOwningDevice)->GetDeviceStateMutex());
	
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
			
		case kAudioStreamPropertyDirection:
			theAnswer = false;
			break;
			
		case kAudioStreamPropertyTerminalType:
			theAnswer = false;
			break;
			
		case kAudioStreamPropertyStartingChannel:
			theAnswer = false;
			break;
			
		case kAudioStreamPropertyLatency:
			theAnswer = false;
			break;
			
		default:
			theAnswer = HP_Object::IsPropertySettable(inAddress);
			break;
	};
	
	return theAnswer;
}

UInt32	HP_Stream::GetPropertyDataSize(const AudioObjectPropertyAddress& inAddress, UInt32 inQualifierDataSize, const void* inQualifierData) const
{
	UInt32 theAnswer = 0;
	
	//	take and hold the state mutex
	CAMutex::Locker theStateMutex(const_cast<HP_Device*>(mOwningDevice)->GetDeviceStateMutex());
	
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
			
		case kAudioStreamPropertyDirection:
			theAnswer = SizeOf32(UInt32);
			break;
			
		case kAudioStreamPropertyTerminalType:
			theAnswer = SizeOf32(UInt32);
			break;
			
		case kAudioStreamPropertyStartingChannel:
			theAnswer = SizeOf32(UInt32);
			break;
			
		case kAudioStreamPropertyLatency:
			theAnswer = SizeOf32(UInt32);
			break;
			
		default:
			theAnswer = HP_Object::GetPropertyDataSize(inAddress, inQualifierDataSize, inQualifierData);
			break;
	};
	
	return theAnswer;
}

void	HP_Stream::GetPropertyData(const AudioObjectPropertyAddress& inAddress, UInt32 inQualifierDataSize, const void* inQualifierData, UInt32& ioDataSize, void* outData) const
{
	//	take and hold the state mutex
	CAMutex::Locker theStateMutex(const_cast<HP_Device*>(mOwningDevice)->GetDeviceStateMutex());
	
	switch(inAddress.mSelector)
	{
		case kAudioObjectPropertyName:
			ThrowIf(ioDataSize != GetPropertyDataSize(inAddress, inQualifierDataSize, inQualifierData), CAException(kAudioHardwareBadPropertySizeError), "HP_Stream::GetPropertyData: wrong data size for kAudioObjectPropertyName");
			*static_cast<CFStringRef*>(outData) = CopyStreamName();
			break;
			
		case kAudioObjectPropertyManufacturer:
			ThrowIf(ioDataSize != GetPropertyDataSize(inAddress, inQualifierDataSize, inQualifierData), CAException(kAudioHardwareBadPropertySizeError), "HP_Stream::GetPropertyData: wrong data size for kAudioObjectPropertyName");
			*static_cast<CFStringRef*>(outData) = CopyStreamManufacturerName();
			break;
			
		case kAudioObjectPropertyElementName:
			ThrowIf(ioDataSize != GetPropertyDataSize(inAddress, inQualifierDataSize, inQualifierData), CAException(kAudioHardwareBadPropertySizeError), "HP_Stream::GetPropertyData: wrong data size for kAudioObjectPropertyElementName");
			*static_cast<CFStringRef*>(outData) = CopyElementFullName(inAddress);
			break;
			
		case kAudioObjectPropertyElementCategoryName:
			ThrowIf(ioDataSize != GetPropertyDataSize(inAddress, inQualifierDataSize, inQualifierData), CAException(kAudioHardwareBadPropertySizeError), "HP_Stream::GetPropertyData: wrong data size for kAudioObjectPropertyElementCategoryName");
			*static_cast<CFStringRef*>(outData) = CopyElementCategoryName(inAddress);
			break;
			
		case kAudioObjectPropertyElementNumberName:
			ThrowIf(ioDataSize != GetPropertyDataSize(inAddress, inQualifierDataSize, inQualifierData), CAException(kAudioHardwareBadPropertySizeError), "HP_Stream::GetPropertyData: wrong data size for kAudioObjectPropertyElementNumberName");
			*static_cast<CFStringRef*>(outData) = CopyElementNumberName(inAddress);
			break;
			
		case kAudioStreamPropertyDirection:
			ThrowIf(ioDataSize != GetPropertyDataSize(inAddress, inQualifierDataSize, inQualifierData), CAException(kAudioHardwareBadPropertySizeError), "HP_Stream::GetPropertyData: wrong data size for kAudioStreamPropertyDirection");
			*static_cast<UInt32*>(outData) = IsInput() ? 1 : 0;
			break;
			
		case kAudioStreamPropertyTerminalType:
			ThrowIf(ioDataSize != GetPropertyDataSize(inAddress, inQualifierDataSize, inQualifierData), CAException(kAudioHardwareBadPropertySizeError), "HP_Stream::GetPropertyData: wrong data size for kAudioStreamPropertyTerminalType");
			*static_cast<UInt32*>(outData) = GetTerminalType();
			break;
			
		case kAudioStreamPropertyStartingChannel:
			ThrowIf(ioDataSize != GetPropertyDataSize(inAddress, inQualifierDataSize, inQualifierData), CAException(kAudioHardwareBadPropertySizeError), "HP_Stream::GetPropertyData: wrong data size for kAudioStreamPropertyStartingChannel");
			*static_cast<UInt32*>(outData) = GetStartingDeviceChannelNumber();
			break;
			
		case kAudioStreamPropertyLatency:
			ThrowIf(ioDataSize != GetPropertyDataSize(inAddress, inQualifierDataSize, inQualifierData), CAException(kAudioHardwareBadPropertySizeError), "HP_Stream::GetPropertyData: wrong data size for kAudioStreamPropertyLatency");
			*static_cast<UInt32*>(outData) = GetLatency();
			break;
			
		default:
			HP_Object::GetPropertyData(inAddress, inQualifierDataSize, inQualifierData, ioDataSize, outData);
			break;
	};
}

void	HP_Stream::SetPropertyData(const AudioObjectPropertyAddress& inAddress, UInt32 inQualifierDataSize, const void* inQualifierData, UInt32 inDataSize, const void* inData, const AudioTimeStamp* inWhen)
{
	//	take and hold the state mutex
	CAMutex::Locker theStateMutex(mOwningDevice->GetDeviceStateMutex());
	
	switch(inAddress.mSelector)
	{
		default:
			HP_Object::SetPropertyData(inAddress, inQualifierDataSize, inQualifierData, inDataSize, inData, inWhen);
			break;
	};
}

#if	(MAC_OS_X_VERSION_MAX_ALLOWED < MAC_OS_X_VERSION_10_4)
void	HP_Stream::PropertiesChanged(UInt32 inNumberAddresses, const AudioObjectPropertyAddress inAddresses[]) const
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
		OSStatus theError = AudioHardwareStreamPropertyChanged(mPlugIn->GetInterface(), mOwningDevice->GetObjectID(), mObjectID, inAddresses[theIndex].mElement, inAddresses[theIndex].mSelector);
		AssertNoError(theError, "HP_Stream::PropertiesChanged: got an error calling the input listeners");
	}
		
	//	re-lock the mutex
	if((theObjectStateMutex != NULL) && ownsStateMutex)
	{
		theObjectStateMutex->Lock();
	}
}
#endif

void	HP_Stream::Show() const
{
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
	printf("AudioObjectID:\t0x%lX\n\tClass:\t\t%s\n\tName:\t\t%s\n\tDirection:\t%s\n\tChannels:\t%lu\n", (long unsigned int)mObjectID, "Audio Stream", theName, (IsInput() ? "Input" : "Output"), (long unsigned int)GetCurrentNumberChannels());
}

void	HP_Stream::RegisterIOBuffer(UInt32 /*inBufferSetID*/, UInt32 /*inBufferByteSize*/, void* /*inBuffer*/)
{
}

void	HP_Stream::UnregisterIOBuffer(UInt32 /*inBufferSetID*/, UInt32 /*inBufferByteSize*/, void* /*inBuffer*/)
{
}

bool	HP_Stream::TellHardwareToSetPhysicalFormat(const AudioStreamBasicDescription& /*inFormat*/)
{
	//	This method should do what it's name says and return true if the change took effect immediately.
	//	Otherwise if the change will take effect asynchronously, this method should return fals.
	return true;
}

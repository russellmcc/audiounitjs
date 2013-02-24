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
#include "HP_DriverPlugIn.h"

//	PublicUtility Includes
#include "CADebugMacros.h"
#include "CAException.h"

//==================================================================================================
//	HP_DriverPlugIn
//==================================================================================================

HP_DriverPlugIn::HP_DriverPlugIn(const AudioDriverPlugInHostInfo& inHostInfo)
:
	mHostInfo(inHostInfo)
{
}

HP_DriverPlugIn::~HP_DriverPlugIn()
{
}

HP_DriverPlugIn*	HP_DriverPlugIn::GetPlugIn(AudioDeviceID inDeviceID)
{
	HP_DriverPlugIn* theAnswer = NULL;
	
	if(sPlugInList != NULL)
	{
		PlugInList::iterator theIterator = sPlugInList->begin();
		while((theAnswer == NULL) && (theIterator != sPlugInList->end()))
		{
			HP_DriverPlugIn* thePlugIn = *theIterator;
			if(thePlugIn->mHostInfo.mDeviceID == inDeviceID)
			{
				theAnswer = thePlugIn;
			}
			else
			{
				std::advance(theIterator, 1);
			}
		}
	}
	
	return theAnswer;
}

HP_DriverPlugIn*	HP_DriverPlugIn::OpenPlugIn(const AudioDriverPlugInHostInfo& inHostInfo)
{
	//	check to see if we already have one
	HP_DriverPlugIn* theAnswer = GetPlugIn(inHostInfo.mDeviceID);
	
	if(theAnswer == NULL)
	{
		//	we don't, so make a new one
		theAnswer = HP_DriverPlugIn::CreatePlugIn(inHostInfo);
		ThrowIfNULL(theAnswer, CAException(kAudioHardwareIllegalOperationError), "HP_DriverPlugIn::OpenPlugIn: failed to create the plug-in instance");
		
		//	create the plug-in list if necessary
		if(sPlugInList == NULL)
		{
			sPlugInList = new PlugInList;
		}
		
		//	add the new plug-in to the list
		sPlugInList->push_back(theAnswer);
	}
	
	return theAnswer;
}

void	HP_DriverPlugIn::ClosePlugIn(AudioDeviceID inDeviceID)
{
	if(sPlugInList != NULL)
	{
		bool isDone = false;
		PlugInList::iterator theIterator = sPlugInList->begin();
		while(!isDone && (theIterator != sPlugInList->end()))
		{
			HP_DriverPlugIn* thePlugIn = *theIterator;
			if(thePlugIn->mHostInfo.mDeviceID == inDeviceID)
			{
				isDone = true;
				sPlugInList->erase(theIterator);
				DestroyPlugIn(thePlugIn);
			}
			else
			{
				std::advance(theIterator, 1);
			}
		}
	}
}

bool	HP_DriverPlugIn::DeviceHasProperty(UInt32 /*inChannel*/, Boolean /*isInput*/, AudioDevicePropertyID /*inPropertyID*/) const
{
	return false;
}

UInt32	HP_DriverPlugIn::DeviceGetPropertyDataSize(UInt32 /*inChannel*/, Boolean /*isInput*/, AudioDevicePropertyID /*inPropertyID*/) const
{
	return 0;
}

bool	HP_DriverPlugIn::DeviceIsPropertyWritable(UInt32 /*inChannel*/, Boolean /*isInput*/, AudioDevicePropertyID /*inPropertyID*/) const
{
	return false;
}

void	HP_DriverPlugIn::DeviceGetPropertyData(UInt32 /*inChannel*/, Boolean /*isInput*/, AudioDevicePropertyID /*inPropertyID*/, UInt32& /*ioPropertyDataSize*/, void* /*outPropertyData*/) const
{
}

void	HP_DriverPlugIn::DeviceSetPropertyData(UInt32 /*inChannel*/, Boolean /*isInput*/, AudioDevicePropertyID /*inPropertyID*/, UInt32 /*inPropertyDataSize*/, const void* /*inPropertyData*/)
{
}

bool	HP_DriverPlugIn::StreamHasProperty(io_object_t /*inIOAudioStream*/, UInt32 /*inChannel*/, AudioDevicePropertyID /*inPropertyID*/) const
{
	return false;
}

UInt32	HP_DriverPlugIn::StreamGetPropertyDataSize(io_object_t /*inIOAudioStream*/, UInt32 /*inChannel*/, AudioDevicePropertyID /*inPropertyID*/) const
{
	return 0;
}

bool	HP_DriverPlugIn::StreamIsPropertyWritable(io_object_t /*inIOAudioStream*/, UInt32 /*inChannel*/, AudioDevicePropertyID /*inPropertyID*/) const
{
	return false;
}

void	HP_DriverPlugIn::StreamGetPropertyData(io_object_t /*inIOAudioStream*/, UInt32 /*inChannel*/, AudioDevicePropertyID /*inPropertyID*/, UInt32&  /*ioPropertyDataSize*/, void*  /*outPropertyData*/) const
{
}

void	HP_DriverPlugIn::StreamSetPropertyData(io_object_t /*inIOAudioStream*/, UInt32 /*inChannel*/, AudioDevicePropertyID /*inPropertyID*/, UInt32  /*inPropertyDataSize*/, const void*  /*inPropertyData*/)
{
}

HP_DriverPlugIn::PlugInList*	HP_DriverPlugIn::sPlugInList = NULL;

//==================================================================================================
//	AudioDriverPlugIn Implementation
//==================================================================================================

extern "C" OSStatus	AudioDriverPlugInOpen(AudioDriverPlugInHostInfo* inHostInfo)
{
	OSStatus theError = kAudioHardwareNoError;
	
	try
	{
		HP_DriverPlugIn::OpenPlugIn(*inHostInfo);
	}
	catch(const CAException& inException)
	{
		theError = inException.GetError();
	}
	catch(...)
	{
		theError = kAudioHardwareUnspecifiedError;
	}
	
	return theError;
}

extern "C" OSStatus	AudioDriverPlugInClose(AudioDeviceID inDevice)
{
	OSStatus theError = kAudioHardwareNoError;
	
	try
	{
		HP_DriverPlugIn::ClosePlugIn(inDevice);
	}
	catch(const CAException& inException)
	{
		theError = inException.GetError();
	}
	catch(...)
	{
		theError = kAudioHardwareUnspecifiedError;
	}
	
	return theError;
}

extern "C" OSStatus	AudioDriverPlugInDeviceGetPropertyInfo(AudioDeviceID inDevice, UInt32 inChannel, Boolean isInput, AudioDevicePropertyID inPropertyID, UInt32* outSize, Boolean* outWritable)
{
	OSStatus theError = kAudioHardwareNoError;
	
	try
	{
		//	find the plug-in object for the device
		HP_DriverPlugIn* thePlugIn = HP_DriverPlugIn::GetPlugIn(inDevice);
		ThrowIfNULL(thePlugIn, CAException(kAudioHardwareBadDeviceError), "AudioDriverPlugInDeviceGetPropertyInfo: couldn't find the plug-in for the device");

		//	figure out if the device has the given property
		if(thePlugIn->DeviceHasProperty(inChannel, isInput, inPropertyID))
		{
			//	let the plug-in do the work
			if(outSize != NULL)
			{
				*outSize = thePlugIn->DeviceGetPropertyDataSize(inChannel, isInput, inPropertyID);
			}
			
			if(outWritable != NULL)
			{
				*outWritable = thePlugIn->DeviceIsPropertyWritable(inChannel, isInput, inPropertyID);
			}
		}
		else
		{
			theError = kAudioHardwareUnknownPropertyError;
		}
	}
	catch(const CAException& inException)
	{
		theError = inException.GetError();
	}
	catch(...)
	{
		theError = kAudioHardwareUnspecifiedError;
	}
	
	return theError;
}

extern "C" OSStatus	AudioDriverPlugInDeviceGetProperty(AudioDeviceID inDevice, UInt32 inChannel, Boolean isInput, AudioDevicePropertyID inPropertyID, UInt32* ioPropertyDataSize, void* outPropertyData)
{
	OSStatus theError = kAudioHardwareNoError;
	
	try
	{
		//	sanity check the arguments
		ThrowIfNULL(ioPropertyDataSize, CAException(kAudioHardwareIllegalOperationError), "AudioDriverPlugInDeviceGetProperty: ioPropertyDataSize is NULL");
		
		//	find the plug-in object for the device
		HP_DriverPlugIn* thePlugIn = HP_DriverPlugIn::GetPlugIn(inDevice);
		ThrowIfNULL(thePlugIn, CAException(kAudioHardwareBadDeviceError), "AudioDriverPlugInDeviceGetProperty: couldn't find the plug-in for the device");

		//	figure out if the device has the given property
		if(thePlugIn->DeviceHasProperty(inChannel, isInput, inPropertyID))
		{
			//	let the plug-in do the work
			thePlugIn->DeviceGetPropertyData(inChannel, isInput, inPropertyID, *ioPropertyDataSize, outPropertyData);
		}
		else
		{
			theError = kAudioHardwareUnknownPropertyError;
		}
	}
	catch(const CAException& inException)
	{
		theError = inException.GetError();
	}
	catch(...)
	{
		theError = kAudioHardwareUnspecifiedError;
	}
	
	return theError;
}

extern "C" OSStatus	AudioDriverPlugInDeviceSetProperty(AudioDeviceID inDevice, const AudioTimeStamp* /*inWhen*/, UInt32 inChannel, Boolean isInput, AudioDevicePropertyID inPropertyID, UInt32 inPropertyDataSize, const void* inPropertyData)
{
	OSStatus theError = kAudioHardwareNoError;
	
	try
	{
		//	find the plug-in object for the device
		HP_DriverPlugIn* thePlugIn = HP_DriverPlugIn::GetPlugIn(inDevice);
		ThrowIfNULL(thePlugIn, CAException(kAudioHardwareBadDeviceError), "AudioDriverPlugInDeviceSetProperty: couldn't find the plug-in for the device");

		//	figure out if the device has the given property
		if(thePlugIn->DeviceHasProperty(inChannel, isInput, inPropertyID))
		{
			//	make sure the property is writable
			if(thePlugIn->DeviceIsPropertyWritable(inChannel, isInput, inPropertyID))
			{
				//	let the plug-in do the work
				thePlugIn->DeviceSetPropertyData(inChannel, isInput, inPropertyID, inPropertyDataSize, inPropertyData);
			}
			else
			{
				theError = kAudioHardwareIllegalOperationError;
			}
		}
		else
		{
			theError = kAudioHardwareUnknownPropertyError;
		}
	}
	catch(const CAException& inException)
	{
		theError = inException.GetError();
	}
	catch(...)
	{
		theError = kAudioHardwareUnspecifiedError;
	}
	
	return theError;
}

extern "C" OSStatus	AudioDriverPlugInStreamGetPropertyInfo(AudioDeviceID inDevice, io_object_t inIOAudioStream, UInt32 inChannel, AudioDevicePropertyID inPropertyID, UInt32* outSize, Boolean* outWritable)
{
	OSStatus theError = kAudioHardwareNoError;
	
	try
	{
		//	find the plug-in object for the device
		HP_DriverPlugIn* thePlugIn = HP_DriverPlugIn::GetPlugIn(inDevice);
		ThrowIfNULL(thePlugIn, CAException(kAudioHardwareBadDeviceError), "AudioDriverPlugInDeviceGetPropertyInfo: couldn't find the plug-in for the device");

		//	figure out if the stream has the given property
		if(thePlugIn->StreamHasProperty(inIOAudioStream, inChannel, inPropertyID))
		{
			//	let the plug-in do the work
			if(outSize != NULL)
			{
				*outSize = thePlugIn->StreamGetPropertyDataSize(inIOAudioStream, inChannel, inPropertyID);
			}
			
			if(outWritable != NULL)
			{
				*outWritable = thePlugIn->StreamIsPropertyWritable(inIOAudioStream, inChannel, inPropertyID);
			}
		}
		else
		{
			theError = kAudioHardwareUnknownPropertyError;
		}
	}
	catch(const CAException& inException)
	{
		theError = inException.GetError();
	}
	catch(...)
	{
		theError = kAudioHardwareUnspecifiedError;
	}
	
	return theError;
}

extern "C" OSStatus	AudioDriverPlugInStreamGetProperty(AudioDeviceID inDevice, io_object_t inIOAudioStream, UInt32 inChannel, AudioDevicePropertyID inPropertyID, UInt32* ioPropertyDataSize, void* outPropertyData)
{
	OSStatus theError = kAudioHardwareNoError;
	
	try
	{
		//	sanity check the arguments
		ThrowIfNULL(ioPropertyDataSize, CAException(kAudioHardwareIllegalOperationError), "AudioDriverPlugInDeviceGetProperty: ioPropertyDataSize is NULL");
		
		//	find the plug-in object for the device
		HP_DriverPlugIn* thePlugIn = HP_DriverPlugIn::GetPlugIn(inDevice);
		ThrowIfNULL(thePlugIn, CAException(kAudioHardwareBadDeviceError), "AudioDriverPlugInDeviceGetProperty: couldn't find the plug-in for the device");

		//	figure out if the stream has the given property
		if(thePlugIn->StreamHasProperty(inIOAudioStream, inChannel, inPropertyID))
		{
			//	let the plug-in do the work
			thePlugIn->StreamGetPropertyData(inIOAudioStream, inChannel, inPropertyID, *ioPropertyDataSize, outPropertyData);
		}
		else
		{
			theError = kAudioHardwareUnknownPropertyError;
		}
	}
	catch(const CAException& inException)
	{
		theError = inException.GetError();
	}
	catch(...)
	{
		theError = kAudioHardwareUnspecifiedError;
	}
	
	return theError;
}

extern "C" OSStatus	AudioDriverPlugInStreamSetProperty(AudioDeviceID inDevice, io_object_t inIOAudioStream, const AudioTimeStamp* /*inWhen*/, UInt32 inChannel, AudioDevicePropertyID inPropertyID, UInt32 inPropertyDataSize, const void* inPropertyData)
{
	OSStatus theError = kAudioHardwareNoError;
	
	try
	{
		//	find the plug-in object for the device
		HP_DriverPlugIn* thePlugIn = HP_DriverPlugIn::GetPlugIn(inDevice);
		ThrowIfNULL(thePlugIn, CAException(kAudioHardwareBadDeviceError), "AudioDriverPlugInDeviceSetProperty: couldn't find the plug-in for the device");

		//	figure out if the stream has the given property
		if(thePlugIn->StreamHasProperty(inIOAudioStream, inChannel, inPropertyID))
		{
			//	make sure the property is writable
			if(thePlugIn->StreamIsPropertyWritable(inIOAudioStream, inChannel, inPropertyID))
			{
				//	let the plug-in do the work
				thePlugIn->StreamSetPropertyData(inIOAudioStream, inChannel, inPropertyID, inPropertyDataSize, inPropertyData);
			}
			else
			{
				theError = kAudioHardwareIllegalOperationError;
			}
		}
		else
		{
			theError = kAudioHardwareUnknownPropertyError;
		}
	}
	catch(const CAException& inException)
	{
		theError = inException.GetError();
	}
	catch(...)
	{
		theError = kAudioHardwareUnspecifiedError;
	}
	
	return theError;
}

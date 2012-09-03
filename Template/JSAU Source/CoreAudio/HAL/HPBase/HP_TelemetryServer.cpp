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
#include "HP_TelemetryServer.h"

//	PublicUtility Includes
#include "CACFDictionary.h"
#include "CACFMessagePort.h"
#include "CADebugMacros.h"
#include "CAException.h"
#include "CAHALTelemetry.h"

//	System Includes
#include <CoreAudio/AudioHardware.h>
#include <sys/stat.h>

//==================================================================================================
//	HP_TelemetryServer
//==================================================================================================

HP_TelemetryServer::HP_TelemetryServer()
:
	mName(NULL),
	mPort(NULL),
	mDispatchQueue(NULL),
	mIsEnabled(false),
	mIsCapturing(false)
{
	//	make the dispatch queue
	mDispatchQueue = dispatch_queue_create("com.apple.audio.HP_TelemetryServer", NULL);
	ThrowIfNULL(mDispatchQueue, CAException(kAudioHardwareIllegalOperationError), "HP_TelemetryServer::HP_TelemetryServer: couldn't create the dispatch queue");

	//	look for the environment variable that enables telemetry
	mIsEnabled = getenv("CoreAudio_HAL_Enable_Telemetry") != NULL;
	
	//	look for the file whose existence enables telemetry
	if(!mIsEnabled)
	{
		struct stat theFileInfo;
		int theStatError = stat("/var/db/.com.apple.audio.telemetryenabled", &theFileInfo);
		mIsEnabled = theStatError == 0;
	}
}

HP_TelemetryServer::~HP_TelemetryServer()
{
	if(mDispatchQueue != NULL)
	{
		dispatch_release(mDispatchQueue);
	}
}

void	HP_TelemetryServer::Initialize(CFStringRef inName)
{
	mName = inName;
		
	if(mIsEnabled)
	{
		//	allocate the port
		mPort = new CACFLocalMessagePort(inName, (CFMessagePortCallBack)MessageHandlerEntry, NULL, this);
		
		//	make sure we really created the port
		Assert(mPort != NULL, "HP_TelemetryServer::Initialize: allocating the port failed");
		if((mPort != NULL) && (!mPort->IsValid()))
		{
			DebugMessage("HP_TelemetryServer::Initialize: the allocated port was invalid");
			delete mPort;
			mPort = NULL;
		}
		
		//	tell the port to do it's thing on the dispatch queue
		if(mPort != NULL)
		{
			mPort->SetDispatchQueue(mDispatchQueue);
		}
	}
}

void	HP_TelemetryServer::Teardown()
{
	if(mPort != NULL)
	{
		//	delete the port
		delete mPort;
		mPort = NULL;
	}
	
	//	release the name
	if(mName != NULL)
	{
		CFRelease(mName);
		mName = NULL;
	}
}

CFDataRef	HP_TelemetryServer::HandleMessage(SInt32 inMessageID, const CACFDictionary& inMessageData)
{
	CFDataRef theAnswer = NULL;
	switch(inMessageID)
	{
		case kHALTelemetryMessageGetProperty:
			theAnswer = GetTelemetryProperty(inMessageData);
			break;
			
		case kHALTelemetryMessageSetProperty:
			theAnswer = SetTelemetryProperty(inMessageData);
			break;
			
		case kHALTelemetryMessageGetTelemetry:
			theAnswer = GetTelemetry(inMessageData);
			break;
		
		case kHALTelemetryMessageClearTelemetry:
			theAnswer = ClearTelemetry(inMessageData);
			break;
		
		default:
			DebugMessage("HP_TelemetryServer::HandleMessage: unknown message");
			throw CAException(kAudioHardwareIllegalOperationError);
			break;
			
	};
	return theAnswer;
}

CFDataRef	HP_TelemetryServer::GetTelemetryProperty(const CACFDictionary& inMessageData) const
{
	CFDataRef		theAnswer = NULL;
	CACFDictionary	theReturnValue(true);
	
	try
	{
		//	get the property ID
		UInt32	thePropertyID;
		bool hasValue = inMessageData.GetUInt32(CFSTR(kHALIOCycleTelemetryPropertyIDKey), thePropertyID);
		ThrowIf(!hasValue, CAException(kAudioHardwareIllegalOperationError), "HP_TelemetryServer::GetTelemetryProperty: requires a property ID");
	
		//	figure out what to do with it
		switch(thePropertyID)
		{
			case kHALTelemetryPropertyIsCapturing:
				theReturnValue.AddUInt32(CFSTR(kHALIOCycleTelemetryPropertyValueKey), (mIsCapturing ? 1 : 0));
				break;
			
			case kHALTelemetryPropertyEndianness:
				#if	TARGET_RT_BIG_ENDIAN
					theReturnValue.AddUInt32(CFSTR(kHALIOCycleTelemetryPropertyValueKey), 1);
				#else
					theReturnValue.AddUInt32(CFSTR(kHALIOCycleTelemetryPropertyValueKey), 0);
				#endif
				break;
			
			default:
				DebugMessage("HP_TelemetryServer::GetTelemetryProperty: unknown proprety");
				throw CAException(kAudioHardwareUnknownPropertyError);
				break;
		};
		
		//	add the return error to the return value
		theReturnValue.AddUInt32(CFSTR(kHALIOCycleTelemetryReturnErrorKey), 0);
	}
	catch(const CAException& inException)
	{
		DebugMessageN1("HP_TelemetryServer::GetTelemetryProperty: Uncaught exception: %ld", (long int)inException.GetError());
		if(theReturnValue.IsValid())
		{
			theReturnValue.AddUInt32(CFSTR(kHALIOCycleTelemetryReturnErrorKey), inException.GetError());
		}
	}
	catch(...)
	{
		DebugMessage("HP_TelemetryServer::GetTelemetryProperty: Uncaught exception");
		if(theReturnValue.IsValid())
		{
			theReturnValue.AddUInt32(CFSTR(kHALIOCycleTelemetryReturnErrorKey), static_cast<UInt32>(kAudioHardwareUnspecifiedError));
		}
	}
	
	//	if there is a return value, make it into a property list in XML format
	theAnswer = CFPropertyListCreateXMLData(NULL, theReturnValue.GetCFDictionary());
	
	return theAnswer;

}

CFDataRef	HP_TelemetryServer::SetTelemetryProperty(const CACFDictionary& inMessageData)
{
	CFDataRef		theAnswer = NULL;
	CACFDictionary	theReturnValue(true);
	
	try
	{
		UInt32	thePropertyUInt32Value;
	
		//	get the property ID
		UInt32	thePropertyID;
		bool hasValue = inMessageData.GetUInt32(CFSTR(kHALIOCycleTelemetryPropertyIDKey), thePropertyID);
		ThrowIf(!hasValue, CAException(kAudioHardwareIllegalOperationError), "HP_TelemetryServer::SetTelemetryProperty: requires a property ID");
		
		//	figure out what to do with it
		switch(thePropertyID)
		{
			case kHALTelemetryPropertyIsCapturing:
				if(inMessageData.GetUInt32(CFSTR(kHALIOCycleTelemetryPropertyValueKey), thePropertyUInt32Value))
				{
					if((thePropertyUInt32Value != 0) && !mIsCapturing && mIsEnabled)
					{
						StartCapturing();
						mIsCapturing = true;
					}
					else if((thePropertyUInt32Value == 0) && mIsCapturing)
					{
						StopCapturing();
						mIsCapturing = false;
					}
				}
				break;
			
			default:
				DebugMessage("HP_TelemetryServer::SetTelemetryProperty: unknown proprety");
				throw CAException(kAudioHardwareUnknownPropertyError);
				break;
		};
		
		//	add the return error to the return value
		theReturnValue.AddUInt32(CFSTR(kHALIOCycleTelemetryReturnErrorKey), 0);
	}
	catch(const CAException& inException)
	{
		DebugMessageN1("HP_TelemetryServer::SetTelemetryProperty: Uncaught exception: %ld", (long int)inException.GetError());
		if(theReturnValue.IsValid())
		{
			theReturnValue.AddUInt32(CFSTR(kHALIOCycleTelemetryReturnErrorKey), inException.GetError());
		}
	}
	catch(...)
	{
		DebugMessage("HP_TelemetryServer::SetTelemetryProperty: Uncaught exception");
		if(theReturnValue.IsValid())
		{
			theReturnValue.AddUInt32(CFSTR(kHALIOCycleTelemetryReturnErrorKey), static_cast<UInt32>(kAudioHardwareUnspecifiedError));
		}
	}
	
	//	if there is a return value, make it into a property list in XML format
	theAnswer = CFPropertyListCreateXMLData(NULL, theReturnValue.GetCFDictionary());
	
	return theAnswer;
}

CFDataRef	HP_TelemetryServer::GetTelemetry(const CACFDictionary& /*inMessageData*/)
{
	return NULL;
}

CFDataRef	HP_TelemetryServer::ClearTelemetry(const CACFDictionary& /*inMessageData*/)
{
	return NULL;
}

void	HP_TelemetryServer::StartCapturing()
{
}

void	HP_TelemetryServer::StopCapturing()
{
}

CFDataRef	HP_TelemetryServer::MessageHandlerEntry(CFMessagePortRef inMessagePort, SInt32 inMessageID, CFDataRef inMessageData, HP_TelemetryServer* inTelemetryServer)
{
	CFDataRef		theAnswer = NULL;
	
	try
	{
		if(inTelemetryServer->mPort != NULL)
		{
			ThrowIf(!CFEqual(inMessagePort, inTelemetryServer->mPort->GetMessagePortRef()), CAException(kAudioHardwareIllegalOperationError), "HP_TelemetryServer::MessageHandlerEntry: bad message port");
			
			//	the incoming CFData contains a property list in XML format, so make a CFDictionary out of it
			CACFDictionary theData(static_cast<CFDictionaryRef>(CFPropertyListCreateFromXMLData(NULL, inMessageData, kCFPropertyListImmutable, NULL)), true);
			
			//	handle the message
			theAnswer = inTelemetryServer->HandleMessage(inMessageID, theData);
		}
	}
	catch(const CAException& inException)
	{
		DebugMessageN1("HP_TelemetryServer::MessageHandlerEntry: Uncaught exception: %ld", (long int)inException.GetError());
	}
	catch(...)
	{
		DebugMessage("HP_TelemetryServer::MessageHandlerEntry: Uncaught exception");
	}
	
	return theAnswer;
}

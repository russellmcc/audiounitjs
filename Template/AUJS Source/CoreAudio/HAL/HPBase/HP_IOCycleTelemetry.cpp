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
#include "HP_IOCycleTelemetry.h"

//	Local Includes
#include "HP_Device.h"

//	PublicUtility Includes
#include "CACFDictionary.h"
#include "CACFString.h"
#include "CADebugMacros.h"
#include "CAException.h"
#include "CAProcess.h"
#include "CAHostTimeBase.h"

//==================================================================================================
//	HP_IOCycleTelemetry
//==================================================================================================

HP_IOCycleTelemetry::HP_IOCycleTelemetry(HP_Device* inDevice)
:
	HP_TelemetryServer(),
	mDevice(inDevice),
	mItemGuard("IOCycleTelemetryGuard"),
	mItemBuffers(),
	mCurrentWriteBuffer(0),
	mCurrentWriteItem(0),
	mCurrentReadBuffer(0)
{
}

HP_IOCycleTelemetry::~HP_IOCycleTelemetry()
{
	StopCapturing();
}

void	HP_IOCycleTelemetry::Initialize(CFStringRef /*inName*/)
{
	//	get the device's UID
	CACFString theUID(mDevice->CopyDeviceUID());
	
	//	get the CFHash of the UID
	UInt32 theHashCode = ToUInt32(CFHash(theUID.GetCFString()));
	
	//	sum all the characters in the UID
	UInt64 theSum = 0;
	UInt32 theNumberCharacters = ToUInt32(CFStringGetLength(theUID.GetCFString()));
	for(UInt32 theCharacterIndex = 0; theCharacterIndex < theNumberCharacters; ++theCharacterIndex)
	{
		UniChar theCharacter = CFStringGetCharacterAtIndex(theUID.GetCFString(), theCharacterIndex);
		theSum += theCharacter;
	}
	
	//	build a string out of the hash code and character sum
	CFStringRef thePortName = CFStringCreateWithFormat(NULL, NULL, CFSTR(kHALIOCycleTelemetryServerPortNameFormat), CAProcess::GetPID(), theHashCode, theSum);
	
	//	initialize the super class (who releases the port name too)
	HP_TelemetryServer::Initialize(thePortName);
}

void	HP_IOCycleTelemetry::Teardown()
{
	HP_TelemetryServer::Teardown();
}

void	HP_IOCycleTelemetry::SaveTelemetryItem(UInt32 inEventKind, UInt32 inIOCycleNumber, Float64 inRateScalar1, Float64 inRateScalar2, Float64 inSampleTime1, Float64 inSampleTime2, UInt64 inHostTime1, UInt64 inHostTime2, UInt32 inError, UInt32 inFlags)
{
	if(mIsEnabled)
	{
		CAGuard::Locker theItemGuard(mItemGuard);
		
		//	get the current buffer being written to
		CAHALIOCycleRawTelemetryEvent* theItems = mItemBuffers[mCurrentWriteBuffer];
		
		//	write the data
		if(theItems != NULL)
		{
			theItems[mCurrentWriteItem].mEventTime = CAHostTimeBase::GetTheCurrentTime();
			theItems[mCurrentWriteItem].mEventKind = inEventKind;
			theItems[mCurrentWriteItem].mIOCycleNumber = inIOCycleNumber;
			theItems[mCurrentWriteItem].mRateScalar1 = inRateScalar1;
			theItems[mCurrentWriteItem].mRateScalar2 = inRateScalar2;
			theItems[mCurrentWriteItem].mSampleTime1 = inSampleTime1;
			theItems[mCurrentWriteItem].mSampleTime2 = inSampleTime2;
			theItems[mCurrentWriteItem].mHostTime1 = inHostTime1;
			theItems[mCurrentWriteItem].mHostTime2 = inHostTime2;
			theItems[mCurrentWriteItem].mError = inError;
			theItems[mCurrentWriteItem].mFlags = inFlags;
			
			//	increment the counters
			++mCurrentWriteItem;
			if(mCurrentWriteItem >= kNumberItemsPerBuffer)
			{
				//	the current buffer is full, so go to the next
				mCurrentWriteItem = 0;
				++mCurrentWriteBuffer;
				if(mCurrentWriteBuffer >= kNumberBuffers)
				{
					mCurrentWriteBuffer = 0;
				}
			}
		}
	}
}
	
CFDataRef	HP_IOCycleTelemetry::GetTelemetry(const CACFDictionary& /*inMessageData*/)
{
	CFDataRef theAnswer = NULL;
	
	if(mIsEnabled)
	{
		CAGuard::Locker theItemGuard(mItemGuard);
		
		CAHALIOCycleRawTelemetryEvent* theItemBuffer = NULL;
		UInt32 theNumberItems = kNumberItemsPerBuffer;
		if(mDevice->IsIOEngineRunning())
		{
			if(mCurrentReadBuffer != mCurrentWriteBuffer)
			{
				theItemBuffer = mItemBuffers[mCurrentReadBuffer];
				++mCurrentReadBuffer;
				if(mCurrentReadBuffer >= kNumberBuffers)
				{
					mCurrentReadBuffer = 0;
				}
			}
		}
		else
		{
			if((mCurrentReadBuffer == mCurrentWriteBuffer) && (mCurrentWriteItem > 0))
			{
				theItemBuffer = mItemBuffers[mCurrentReadBuffer];
				theNumberItems = mCurrentWriteItem;
				mCurrentWriteItem = 0;
			}
			else if(mCurrentReadBuffer != mCurrentWriteBuffer)
			{
				theItemBuffer = mItemBuffers[mCurrentReadBuffer];
				++mCurrentReadBuffer;
				if(mCurrentReadBuffer > kNumberBuffers)
				{
					mCurrentReadBuffer = 0;
				}
			}
		}
		
		//	make a CFData out of the buffer
		if((theItemBuffer != NULL) && (theNumberItems > 0))
		{
			theAnswer = CFDataCreate(NULL, reinterpret_cast<const UInt8*>(theItemBuffer), theNumberItems * sizeof(CAHALIOCycleRawTelemetryEvent));
		}
	}
	
	return theAnswer;
}

CFDataRef	HP_IOCycleTelemetry::ClearTelemetry(const CACFDictionary& /*inMessageData*/)
{
	CAGuard::Locker theItemGuard(mItemGuard);
	
	mCurrentReadBuffer = mCurrentWriteBuffer;
	mCurrentWriteItem = 0;
	return NULL;
}
	
void	HP_IOCycleTelemetry::StartCapturing()
{
	if(!IsCapturing())
	{
		CAGuard::Locker theItemGuard(mItemGuard);
	
		//	allocate the buffers
		for(UInt32 theBufferIndex = 0; theBufferIndex < kNumberBuffers; ++theBufferIndex)
		{
			mItemBuffers[theBufferIndex] = new CAHALIOCycleRawTelemetryEvent[kNumberItemsPerBuffer];
			memset(mItemBuffers[theBufferIndex], 0, kNumberItemsPerBuffer * sizeof(CAHALIOCycleRawTelemetryEvent));
		}
		
		//	set up the counters
		mCurrentWriteBuffer = 0;
		mCurrentWriteItem = 0;
		mCurrentReadBuffer = 0;
	}
}

void	HP_IOCycleTelemetry::StopCapturing()
{
	if(IsCapturing())
	{
		CAGuard::Locker theItemGuard(mItemGuard);
	
		//	toss the buffers
		for(UInt32 theBufferIndex = 0; theBufferIndex < kNumberBuffers; ++theBufferIndex)
		{
			delete mItemBuffers[theBufferIndex];
			mItemBuffers[theBufferIndex] = NULL;
		}
		
		//	set up the counters
		mCurrentWriteBuffer = 0;
		mCurrentWriteItem = 0;
		mCurrentReadBuffer = 0;
	}
}

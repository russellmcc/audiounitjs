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
#if !defined(__HP_IOCycleTelemetry_h__)
#define __HP_IOCycleTelemetry_h__

//==================================================================================================
//	Includes
//==================================================================================================

//	Super Class Includes
#include "HP_TelemetryServer.h"

//	PublicUtility Includes
#include "CAGuard.h"
#include "CAHALTelemetry.h"

//	System Includes
#include <CoreAudio/AudioHardware.h>
#include <CoreFoundation/CoreFoundation.h>

//==================================================================================================
//	Types
//==================================================================================================

class	HP_Device;
struct	CAHALIOCycleRawTelemetryEvent;

//==================================================================================================
//	HP_IOCycleTelemetry
//==================================================================================================

class HP_IOCycleTelemetry
:
	public	HP_TelemetryServer
{

//	Constants
public:
	enum
	{
							kNumberBuffers			= 512,
							kNumberItemsPerBuffer	= 256
	};

//	Construction/Destruction
public:
							HP_IOCycleTelemetry(HP_Device* inDevice);
	virtual					~HP_IOCycleTelemetry();
	
	virtual void			Initialize(CFStringRef inName);
	virtual void			Teardown();

//	Operations
public:
	void					IOCycleInitializeBegin(UInt32 inIOCycleNumber) { if(mIsCapturing){ SaveTelemetryItem(kHALIOCycleTelemetryEventInitializeBegin, inIOCycleNumber, 0, 0, 0, 0, 0, 0, 0, kHALIOCycleTelemetryFlagNoneIsValid); } }
	void					IOCycleInitializeEnd(UInt32 inIOCycleNumber, const AudioTimeStamp& inAnchorTime) { if(mIsCapturing){ SaveTelemetryItem(kHALIOCycleTelemetryEventInitializeEnd, inIOCycleNumber, inAnchorTime.mRateScalar, 0, inAnchorTime.mSampleTime, 0, inAnchorTime.mHostTime, 0, 0, kHALIOCycleTelemetryFlagTimeStamp1IsValid); } }

	void					IOCycleWorkLoopBegin(UInt32 inIOCycleNumber, const AudioTimeStamp& inCurrentTime) { if(mIsCapturing){ SaveTelemetryItem(kHALIOCycleTelemetryEventWorkLoopBegin, inIOCycleNumber, inCurrentTime.mRateScalar, 0, inCurrentTime.mSampleTime, 0, inCurrentTime.mHostTime, 0, 0, kHALIOCycleTelemetryFlagTimeStamp1IsValid); } }

	void					IOCycleInputReadBegin(UInt32 inIOCycleNumber, const AudioTimeStamp& inInputTime) { if(mIsCapturing){ SaveTelemetryItem(kHALIOCycleTelemetryEventInputReadBegin, inIOCycleNumber, inInputTime.mRateScalar, 0, inInputTime.mSampleTime, 0, inInputTime.mHostTime, 0, 0, kHALIOCycleTelemetryFlagTimeStamp1IsValid); } }
	void					IOCycleInputReadEnd(UInt32 inIOCycleNumber, UInt32 inKernelError) { if(mIsCapturing){ SaveTelemetryItem(kHALIOCycleTelemetryEventInputReadEnd, inIOCycleNumber, 0, 0, 0, 0, 0, 0, inKernelError, kHALIOCycleTelemetryFlagErrorIsValid); } }

	void					IOCycleIOProcsBegin(UInt32 inIOCycleNumber) { if(mIsCapturing){ SaveTelemetryItem(kHALIOCycleTelemetryEventIOProcsBegin, inIOCycleNumber, 0, 0, 0, 0, 0, 0, 0, kHALIOCycleTelemetryFlagNoneIsValid); } }
	void					IOCycleIOProcsEnd(UInt32 inIOCycleNumber) { if(mIsCapturing){ SaveTelemetryItem(kHALIOCycleTelemetryEventIOProcsEnd, inIOCycleNumber, 0, 0, 0, 0, 0, 0, 0, kHALIOCycleTelemetryFlagNoneIsValid); } }
	
	void					IOCycleIOProcCallBegin(UInt32 inIOCycleNumber, AudioDeviceIOProc inIOProc) { if(mIsCapturing){ SaveTelemetryItem(kHALIOCycleTelemetryEventIOProcCallBegin, inIOCycleNumber, 0, 0, 0, 0, (UInt64)inIOProc, 0, 0, kHALIOCycleTelemetryFlagHostTime1IsValid); } }
	void					IOCycleIOProcCallEnd(UInt32 inIOCycleNumber) { if(mIsCapturing){ SaveTelemetryItem(kHALIOCycleTelemetryEventIOProcCallEnd, inIOCycleNumber, 0, 0, 0, 0, 0, 0, 0, kHALIOCycleTelemetryFlagNoneIsValid); } }

	void					IOCycleOutputWriteBegin(UInt32 inIOCycleNumber, const AudioTimeStamp& inOutputTime) { if(mIsCapturing){ SaveTelemetryItem(kHALIOCycleTelemetryEventOutputWriteBegin, inIOCycleNumber, inOutputTime.mRateScalar, 0, inOutputTime.mSampleTime, 0, inOutputTime.mHostTime, 0, 0, kHALIOCycleTelemetryFlagTimeStamp1IsValid); } }
	void					IOCycleOutputWriteEnd(UInt32 inIOCycleNumber, UInt32 inKernelError) { if(mIsCapturing){ SaveTelemetryItem(kHALIOCycleTelemetryEventOutputWriteEnd, inIOCycleNumber, 0, 0, 0, 0, 0, 0, inKernelError, kHALIOCycleTelemetryFlagErrorIsValid); } }

	void					IOCycleWorkLoopOverloadBegin(UInt32 inIOCycleNumber, const AudioTimeStamp& inCurrentTime, const AudioTimeStamp& inNextWakeUpTime) { if(mIsCapturing){ SaveTelemetryItem(kHALIOCycleTelemetryEventWorkLoopOverloadBegin, inIOCycleNumber, inCurrentTime.mRateScalar, inNextWakeUpTime.mRateScalar, inCurrentTime.mSampleTime, inNextWakeUpTime.mSampleTime, inCurrentTime.mHostTime, inNextWakeUpTime.mHostTime, 0, kHALIOCycleTelemetryFlagBothTimeStampsAreValid); } }
	void					IOCycleWorkLoopOverloadEnd(UInt32 inIOCycleNumber, const AudioTimeStamp& inAnchorTime) { if(mIsCapturing){ SaveTelemetryItem(kHALIOCycleTelemetryEventWorkLoopOverloadEnd, inIOCycleNumber, inAnchorTime.mRateScalar, 0, inAnchorTime.mSampleTime, 0, inAnchorTime.mHostTime, 0, 0, kHALIOCycleTelemetryFlagTimeStamp1IsValid); } }

	void					IOCycleWorkLoopEnd(UInt32 inIOCycleNumber, const AudioTimeStamp& inCurrentTime, const AudioTimeStamp& inNextWakeUpTime) { if(mIsCapturing){ SaveTelemetryItem(kHALIOCycleTelemetryEventWorkLoopEnd, inIOCycleNumber, inCurrentTime.mRateScalar, inNextWakeUpTime.mRateScalar, inCurrentTime.mSampleTime, inNextWakeUpTime.mSampleTime, inCurrentTime.mHostTime, inNextWakeUpTime.mHostTime, 0, kHALIOCycleTelemetryFlagBothTimeStampsAreValid); } }

	void					IOCycleTeardownBegin(UInt32 inIOCycleNumber) { if(mIsCapturing){ SaveTelemetryItem(kHALIOCycleTelemetryEventTeardownBegin, inIOCycleNumber, 0, 0, 0, 0, 0, 0, 0, kHALIOCycleTelemetryFlagNoneIsValid); } }
	void					IOCycleTeardownEnd(UInt32 inIOCycleNumber) { if(mIsCapturing){ SaveTelemetryItem(kHALIOCycleTelemetryEventTeardownEnd, inIOCycleNumber, 0, 0, 0, 0, 0, 0, 0, kHALIOCycleTelemetryFlagNoneIsValid); } }
	
	void					ZeroTimeStampRecevied(UInt32 inIOCycleNumber, const AudioTimeStamp& inZeroTimeStamp) { if(mIsCapturing){ SaveTelemetryItem(kHALIOCycleTelemetryEventZeroTimeStampRecieved, inIOCycleNumber, inZeroTimeStamp.mRateScalar, 0, inZeroTimeStamp.mSampleTime, 0, inZeroTimeStamp.mHostTime, 0, 0, kHALIOCycleTelemetryFlagTimeStamp1IsValid); } }
	void					ZeroTimeStampRecevied(UInt32 inIOCycleNumber, Float64 inRateScalar, Float64 inSampleTime, UInt64 inHostTime) { if(mIsCapturing){ SaveTelemetryItem(kHALIOCycleTelemetryEventZeroTimeStampRecieved, inIOCycleNumber, inRateScalar, 0, inSampleTime, 0, inHostTime, 0, 0, kHALIOCycleTelemetryFlagTimeStamp1IsValid); } }
	void					ZeroTimeStampApplied(UInt32 inIOCycleNumber, const AudioTimeStamp& inZeroTimeStamp) { if(mIsCapturing){ SaveTelemetryItem(kHALIOCycleTelemetryEventZeroTimeStampApplied, inIOCycleNumber, inZeroTimeStamp.mRateScalar, 0, inZeroTimeStamp.mSampleTime, 0, inZeroTimeStamp.mHostTime, 0, 0, kHALIOCycleTelemetryFlagTimeStamp1IsValid); } }
	void					ZeroTimeStampApplied(UInt32 inIOCycleNumber, Float64 inRateScalar, Float64 inSampleTime, UInt64 inHostTime) { if(mIsCapturing){ SaveTelemetryItem(kHALIOCycleTelemetryEventZeroTimeStampApplied, inIOCycleNumber, inRateScalar, 0, inSampleTime, 0, inHostTime, 0, 0, kHALIOCycleTelemetryFlagTimeStamp1IsValid); } }
	void					ZeroTimeStampEarly(UInt32 inIOCycleNumber, const AudioTimeStamp& inZeroTimeStamp) { if(mIsCapturing){ SaveTelemetryItem(kHALIOCycleTelemetryEventEarlyZeroTimeStamp, inIOCycleNumber, inZeroTimeStamp.mRateScalar, 0, inZeroTimeStamp.mSampleTime, 0, inZeroTimeStamp.mHostTime, 0, 0, kHALIOCycleTelemetryFlagTimeStamp1IsValid); } }
	void					ZeroTimeStampEarly(UInt32 inIOCycleNumber, Float64 inRateScalar, Float64 inSampleTime, UInt64 inHostTime) { if(mIsCapturing){ SaveTelemetryItem(kHALIOCycleTelemetryEventEarlyZeroTimeStamp, inIOCycleNumber, inRateScalar, 0, inSampleTime, 0, inHostTime, 0, 0, kHALIOCycleTelemetryFlagTimeStamp1IsValid); } }
	void					ZeroTimeStampOutOfBounds(UInt32 inIOCycleNumber, const AudioTimeStamp& inZeroTimeStamp) { if(mIsCapturing){ SaveTelemetryItem(kHALIOCycleTelemetryEventOutOfBoundsZeroTimeStamp, inIOCycleNumber, inZeroTimeStamp.mRateScalar, 0, inZeroTimeStamp.mSampleTime, 0, inZeroTimeStamp.mHostTime, 0, 0, kHALIOCycleTelemetryFlagTimeStamp1IsValid); } }
	void					ZeroTimeStampOutOfBounds(UInt32 inIOCycleNumber, Float64 inRateScalar, Float64 inSampleTime, UInt64 inHostTime) { if(mIsCapturing){ SaveTelemetryItem(kHALIOCycleTelemetryEventOutOfBoundsZeroTimeStamp, inIOCycleNumber, inRateScalar, 0, inSampleTime, 0, inHostTime, 0, 0, kHALIOCycleTelemetryFlagTimeStamp1IsValid); } }
	void					TimelineReset(UInt32 inIOCycleNumber) { if(mIsCapturing){ SaveTelemetryItem(kHALIOCycleTelemetryEventTimelineReset, inIOCycleNumber, 0, 0, 0, 0, 0, 0, 0, kHALIOCycleTelemetryFlagNoneIsValid); } }
	void					Resynch(UInt32 inIOCycleNumber, const AudioTimeStamp& inAnchorTime) { if(mIsCapturing){ SaveTelemetryItem(kHALIOCycleTelemetryEventResynch, inIOCycleNumber, inAnchorTime.mRateScalar, 0, inAnchorTime.mSampleTime, 0, inAnchorTime.mHostTime, 0, 0, kHALIOCycleTelemetryFlagTimeStamp1IsValid); } }
	
	void					InputDataPresent(UInt32 inIOCycleNumber) { if(mIsCapturing){ SaveTelemetryItem(kHALIOCycleTelemetryEventInputDataPresent, inIOCycleNumber, 0, 0, 0, 0, 0, 0, 0, kHALIOCycleTelemetryFlagNoneIsValid); } }
	void					OutputDataPresent(UInt32 inIOCycleNumber) { if(mIsCapturing){ SaveTelemetryItem(kHALIOCycleTelemetryEventOutputDataPresent, inIOCycleNumber, 0, 0, 0, 0, 0, 0, 0, kHALIOCycleTelemetryFlagNoneIsValid); } }

	void					StartHardware(UInt32 inIOCycleNumber, UInt32 inKernelError) { if(mIsCapturing){ SaveTelemetryItem(kHALIOCycleTelemetryEventStartHardware, inIOCycleNumber, 0, 0, 0, 0, 0, 0, inKernelError, kHALIOCycleTelemetryFlagErrorIsValid); } }
	void					StopHardware(UInt32 inIOCycleNumber, UInt32 inKernelError) { if(mIsCapturing){ SaveTelemetryItem(kHALIOCycleTelemetryEventStopHardware, inIOCycleNumber, 0, 0, 0, 0, 0, 0, inKernelError, kHALIOCycleTelemetryFlagErrorIsValid); } }

	void					HardwareStarted(UInt32 inIOCycleNumber) { if(mIsCapturing){ SaveTelemetryItem(kHALIOCycleTelemetryEventHardwareStarted, inIOCycleNumber, 0, 0, 0, 0, 0, 0, 0, kHALIOCycleTelemetryFlagNoneIsValid); } }
	void					HardwareStopped(UInt32 inIOCycleNumber) { if(mIsCapturing){ SaveTelemetryItem(kHALIOCycleTelemetryEventHardwareStopped, inIOCycleNumber, 0, 0, 0, 0, 0, 0, 0, kHALIOCycleTelemetryFlagNoneIsValid); } }
	void					HardwarePaused(UInt32 inIOCycleNumber) { if(mIsCapturing){ SaveTelemetryItem(kHALIOCycleTelemetryEventHardwarePaused, inIOCycleNumber, 0, 0, 0, 0, 0, 0, 0, kHALIOCycleTelemetryFlagNoneIsValid); } }
	void					HardwareResumed(UInt32 inIOCycleNumber) { if(mIsCapturing){ SaveTelemetryItem(kHALIOCycleTelemetryEventHardwareResumed, inIOCycleNumber, 0, 0, 0, 0, 0, 0, 0, kHALIOCycleTelemetryFlagNoneIsValid); } }
	void					FormatChangeBegin(UInt32 inIOCycleNumber) { if(mIsCapturing){ SaveTelemetryItem(kHALIOCycleTelemetryEventFormatChangeBegin, inIOCycleNumber, 0, 0, 0, 0, 0, 0, 0, kHALIOCycleTelemetryFlagNoneIsValid); } }
	void					FormatChangeEnd(UInt32 inIOCycleNumber) { if(mIsCapturing){ SaveTelemetryItem(kHALIOCycleTelemetryEventFormatChangeEnd, inIOCycleNumber, 0, 0, 0, 0, 0, 0, 0, kHALIOCycleTelemetryFlagNoneIsValid); } }
	void					MajorEngineChangeBegin(UInt32 inIOCycleNumber) { if(mIsCapturing){ SaveTelemetryItem(kHALIOCycleTelemetryEventMajorEngineChangeBegin, inIOCycleNumber, 0, 0, 0, 0, 0, 0, 0, kHALIOCycleTelemetryFlagNoneIsValid); } }
	void					MajorEngineChangeEnd(UInt32 inIOCycleNumber) { if(mIsCapturing){ SaveTelemetryItem(kHALIOCycleTelemetryEventMajorEngineChangeEnd, inIOCycleNumber, 0, 0, 0, 0, 0, 0, 0, kHALIOCycleTelemetryFlagNoneIsValid); } }
	void					IOBufferSizeChangeBegin(UInt32 inIOCycleNumber, Float64 inNewBufferSize) { if(mIsCapturing){ SaveTelemetryItem(kHALIOCycleTelemetryEventBufferSizeChangeBegin, inIOCycleNumber, 0, 0, inNewBufferSize, 0, 0, 0, 0, kHALIOCycleTelemetryFlagSampleTime1IsValid); } }
	void					IOBufferSizeChangeEnd(UInt32 inIOCycleNumber) { if(mIsCapturing){ SaveTelemetryItem(kHALIOCycleTelemetryEventBufferSizeChangeEnd, inIOCycleNumber, 0, 0, 0, 0, 0, 0, 0, kHALIOCycleTelemetryFlagNoneIsValid); } }

//	Implementation
protected:
	void					SaveTelemetryItem(UInt32 inEventKind, UInt32 inIOCycleNumber, Float64 inRateScalar1, Float64 inRateScalar2, Float64 inSampleTime1, Float64 inSampleTime2, UInt64 inHostTime1, UInt64 inHostTime2, UInt32 inError, UInt32 inFlags);
	
	virtual CFDataRef		GetTelemetry(const CACFDictionary& inMessageData);
	virtual CFDataRef		ClearTelemetry(const CACFDictionary& inMessageData);
	virtual void			StartCapturing();
	virtual void			StopCapturing();
	
	HP_Device*						mDevice;
	CAGuard							mItemGuard;
	CAHALIOCycleRawTelemetryEvent*	mItemBuffers[kNumberBuffers];
	UInt32							mCurrentWriteBuffer;
	UInt32							mCurrentWriteItem;
	UInt32							mCurrentReadBuffer;

};

#endif

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
#include "HP_IOThread.h"

//	Local Includes
#include "HP_Device.h"
#if Use_HAL_Telemetry
	#include "HP_IOCycleTelemetry.h"
#else
	#include "HALdtrace.h"
#endif

//	PublicUtility Includes
#include "CAAudioTimeStamp.h"
#include "CADebugger.h"
#include "CADebugMacros.h"
#include "CAException.h"
#include "CAHostTimeBase.h"

//#define	Offset_For_Input	1

#if CoreAudio_Debug
//	#define	Log_Resynchs	1
#endif

//==================================================================================================
//	HP_IOThread
//==================================================================================================

HP_IOThread::HP_IOThread(HP_Device* inDevice)
:
	mDevice(inDevice),
	mIOThread(reinterpret_cast<CAPThread::ThreadRoutine>(ThreadEntry), this, CAPThread::kMaxThreadPriority),
	mIOGuard("IOGuard"),
	mIOCycleUsage(1.0f),
	mAnchorTime(CAAudioTimeStamp::kZero),
	mFrameCounter(0),
	mIOCycleCounter(0),
	mOverloadCounter(0),
	mWorkLoopPhase(kNotRunningPhase),
	mStopWorkLoop(false)
{
	//	set the IO thread name
	char theThreadName[CAPThread::kMaxThreadNameLength];
	snprintf(theThreadName, CAPThread::kMaxThreadNameLength, "com.apple.audio.hal.io.%d-%s", (int)mDevice->GetObjectID(), mDevice->GetDebugDeviceName());
	theThreadName[CAPThread::kMaxThreadNameLength - 1] = 0;
	mIOThread.SetName(theThreadName);
}

HP_IOThread::~HP_IOThread()
{
}

Float32	HP_IOThread::GetIOCycleUsage() const
{
	return mIOCycleUsage;
}

void	HP_IOThread::SetIOCycleUsage(Float32 inIOCycleUsage)
{
	mIOCycleUsage = inIOCycleUsage;
	mIOCycleUsage = std::min(1.0f, std::max(0.0f, mIOCycleUsage));
}

UInt32	HP_IOThread::GetWorkLoopPhase() const
{
	return mWorkLoopPhase;
}

bool	HP_IOThread::HasBeenStopped() const
{
	return mStopWorkLoop;
}

bool	HP_IOThread::IsCurrentThread() const
{
	return mIOThread.IsRunning() && mIOThread.IsCurrentThread();
}

void	HP_IOThread::Start()
{
	//	the calling thread must have already locked the Guard prior to calling this method

	//	nothing to do if the IO thread is initializing or already running
	if((mWorkLoopPhase != kInitializingPhase) && (mWorkLoopPhase != kRunningPhase))
	{
		if(mWorkLoopPhase == kTeardownPhase)
		{
			//	there's nothing that can be done if this is happenning on the IO thread
			ThrowIf(mIOThread.IsCurrentThread(), CAException(kAudioHardwareIllegalOperationError), "HP_IOThread::Start: can't restart the IO thread from inside the IO thread");
			
			//	otherwise we wait on the Guard since it is already held by this thread and this isn't the IO thread
			mIOGuard.Wait();
		}
	
		//	set the anchor time to zero so that it gets taken during the IO thread's initialization
		mAnchorTime = CAAudioTimeStamp::kZero;
		mAnchorTime.mFlags = kAudioTimeStampSampleTimeValid | kAudioTimeStampHostTimeValid | kAudioTimeStampRateScalarValid;
		
		//	clear the sentinel value for stopping the IO thread in case it was set previuosly
		mStopWorkLoop = false;
		
		//	spawn a new IO thread
		//	Note that because the IOGuard is held by this thread the newly spawed IO thread
		//	won't be able to do anything since locking the IOGuard is the first thing it does.
		//	Note that we do this in a loop to be sure that the thread is actually spawned
		bool theWaitTimedOut = false;
		const UInt32 kNumberTimesToTry = 4;
		UInt32 theNumberTimesAttempted = 0;
		while((theNumberTimesAttempted < kNumberTimesToTry) && (mWorkLoopPhase == kNotRunningPhase))
		{
			//	increment the counter
			++theNumberTimesAttempted;
			
			//	spawn thre IO thread
			mIOThread.Start();
			
			//	wait for the IO thread to tell us it has started, but don't wait forever
			theWaitTimedOut = mIOGuard.WaitFor(100 * 1000 * 1000);
			if(theWaitTimedOut)
			{
				DebugMessage("HP_IOThread::Start: waited 100ms for the IO thread to spawn");
			}
		}
		
		//	throw an exception if we totally failed to spawn an IO thread
		if(theWaitTimedOut && (theNumberTimesAttempted >= kNumberTimesToTry))
		{
			DebugMessage("HP_IOThread::Start: totally failed to start the IO thread");
			throw CAException(kAudioHardwareIllegalOperationError);
		}
	}
}

void	HP_IOThread::Stop()
{
	//	the calling thread must have already locked the Guard prior to calling this method
	if((mWorkLoopPhase == kInitializingPhase) || (mWorkLoopPhase == kRunningPhase))
	{
		//	set the sentinel value to stop the work loop in the IO thread
		mStopWorkLoop = true;
		
		if(!mIOThread.IsCurrentThread())
		{
			//	the current thread isn't the IO thread so this thread has to wait for
			//	the IO thread to stop before continuing
			
			//	prod the IO thread to wake up
			mIOGuard.NotifyAll();
			
			//	and wait for it to signal that it has stopped
			mIOGuard.Wait();
		}
	}
}

void	HP_IOThread::Resynch(AudioTimeStamp* ioAnchorTime, bool inSignalIOThread)
{
	//	the calling thread must have already locked the Guard prior to calling this method
	if(mWorkLoopPhase == kRunningPhase)
	{
		//	get the current time
		CAAudioTimeStamp theCurrentTime;
		theCurrentTime.mFlags = kAudioTimeStampSampleTimeValid | kAudioTimeStampHostTimeValid | kAudioTimeStampRateScalarValid;
		mDevice->GetCurrentTime(theCurrentTime);
		
		//	re-anchor at the given time
		if(ioAnchorTime != NULL)
		{
			Float64 theIOBufferFrameSize = mDevice->GetIOBufferFrameSize();
			if(ioAnchorTime->mSampleTime <= (theCurrentTime.mSampleTime + (2 * theIOBufferFrameSize)))
			{
				//	the new anchor time is soon, so just take it
				mAnchorTime = *ioAnchorTime;
			}
			else
			{
				//	the new anchor time is way off in the future, so calculate
				//	a different anchor time that leads to an integer number of
				//	IO cycles until the given new anchor time so that we don't
				//	accidentally miss anything important.
				AudioTimeStamp theNewAnchorTime= *ioAnchorTime;
				
				//	the sample time is easy to calculate
				Float64 theSampleTime = ioAnchorTime->mSampleTime - theCurrentTime.mSampleTime;
				theSampleTime /= theIOBufferFrameSize;
				theSampleTime = floor(theSampleTime);
				theNewAnchorTime.mSampleTime -= theIOBufferFrameSize * (theSampleTime - 1);
				
				//	use that to calculate the rest of the time stamp
				theNewAnchorTime.mFlags = kAudioTimeStampSampleTimeValid;
				mAnchorTime = CAAudioTimeStamp::kZero;
				mAnchorTime.mFlags = kAudioTimeStampSampleTimeValid | kAudioTimeStampHostTimeValid | kAudioTimeStampRateScalarValid;
				mDevice->TranslateTime(theNewAnchorTime, mAnchorTime);
			}
			mFrameCounter = 0;
			
			//	set the return value
			*ioAnchorTime = mAnchorTime;
		}
		else
		{
			mAnchorTime = theCurrentTime;
			mFrameCounter = 0;
#if	Offset_For_Input
			if(mDevice->HasInputStreams())
			{
				//	the first sleep cycle as to be at least the input safety offset and a buffer's
				//	worth of time to be sure that the input data is all there
				mFrameCounter += mDevice->GetSafetyOffset(true);
			}
#endif
		}
		
		//	signal the IO thread if necessary
		if(inSignalIOThread)
		{
			mIOGuard.NotifyAll();
		}
	}
}

void	HP_IOThread::GetCurrentPosition(AudioTimeStamp& outTime) const
{
	outTime = mAnchorTime;
	outTime.mFlags = kAudioTimeStampSampleTimeValid;
	outTime.mSampleTime += mFrameCounter;
}

void	HP_IOThread::WorkLoop()
{
	//	grab the IO guard
	bool wasLocked = mIOGuard.Lock();
	
	//	initialize some stuff
	mWorkLoopPhase = kInitializingPhase;
	mIOCycleCounter = 0;
	mOverloadCounter = 0;
	CAPropertyAddress theIsRunningAddress(kAudioDevicePropertyDeviceIsRunning);
#if Use_HAL_Telemetry
	mDevice->GetIOCycleTelemetry().IOCycleInitializeBegin(mIOCycleCounter);
#else
	HAL_IOCYCLEINITIALIZEBEGIN(mIOCycleCounter);
#endif
	try
	{
		//	and signal that the IO thread is running
		mIOGuard.NotifyAll();
		
		//	initialize the work loop stopping conditions
		mStopWorkLoop = false;
		
		//	Tell the device that the IO thread has initialized. Note that we unlock around this call
		//	due to the fact that IOCycleInitialize might not return for a while because it might
		//	have to wait for the hardware to start.
		if(wasLocked)
		{
			mIOGuard.Unlock();
		}
		
		//	tell the device that the IO cycle is initializing to start the timing services
		mDevice->StartIOCycleTimingServices();
		
		//	set the device state to know the engine is running
		mDevice->IOEngineStarted();
		
		//	notify clients that the engine is running
		mDevice->PropertiesChanged(1, &theIsRunningAddress);
		
		//	re-lock the guard
		wasLocked = mIOGuard.Lock();

		//	make sure the thread is still running before moving on
		if(!mStopWorkLoop)
		{
			//	set the time constraints for the IOThread
			SetTimeConstraints();
			
			//	initialize the clock
			mDevice->EstablishIOCycleAnchorTime(mAnchorTime);
			mFrameCounter = 0;
			
#if	Offset_For_Input
			if(mDevice->HasInputStreams())
			{
				//	the first sleep cycle as to be at least the input safety offset and a buffer's
				//	worth of time to be sure that the input data is all there
				mFrameCounter += mDevice->GetSafetyOffset(true);
			}
#endif
			
			//	get the current time
			AudioTimeStamp theCurrentTime;
			mDevice->GetCurrentTime(theCurrentTime);
				
			//	enter the work loop
			mWorkLoopPhase = kRunningPhase;
			bool isInNeedOfResynch = false;
			bool isCheckingForOverloads = false;
			Float64 theIOBufferFrameSize = mDevice->GetIOBufferFrameSize();
#if Use_HAL_Telemetry
			mDevice->GetIOCycleTelemetry().IOCycleInitializeEnd(mIOCycleCounter, mAnchorTime);
			mDevice->GetIOCycleTelemetry().IOCycleWorkLoopBegin(mIOCycleCounter, theCurrentTime);
#else
			HAL_IOCYCLEINITIALIZEEND(mIOCycleCounter, mAnchorTime);
			HAL_IOCYCLEWORKLOOPBEGIN(mIOCycleCounter, theCurrentTime);
#endif
			while(!mStopWorkLoop)
			{
				//	get the new IO buffer frame size
				Float64 theNewIOBufferFrameSize = mDevice->GetIOBufferFrameSize();
				
				//	initialize the next wake up time
				AudioTimeStamp theNextWakeUpTime = CAAudioTimeStamp::kZero;
				theNextWakeUpTime.mFlags = kAudioTimeStampSampleTimeValid + kAudioTimeStampHostTimeValid + kAudioTimeStampRateScalarValid;
				
				//	get the current time
				mDevice->GetCurrentTime(theCurrentTime);
				
				//	we have to run a special, untimed IO cycle if the IO buffer size changed
				if((theNewIOBufferFrameSize != theIOBufferFrameSize) && (theCurrentTime.mSampleTime >= (mAnchorTime.mSampleTime + mFrameCounter)))
				{
					//	mark the end of the previous cycle
#if Use_HAL_Telemetry
					mDevice->GetIOCycleTelemetry().IOCycleWorkLoopEnd(mIOCycleCounter, theCurrentTime, theNextWakeUpTime);
#else
					HAL_IOCYCLEWORKLOOPEND(mIOCycleCounter, theCurrentTime, theNextWakeUpTime);
#endif
					//	increment the cycle counter
					++mIOCycleCounter;
					
					//	increment the frame counter
					mFrameCounter += theIOBufferFrameSize;
				
					//	the new cycle is starting
#if Use_HAL_Telemetry
					mDevice->GetIOCycleTelemetry().IOCycleWorkLoopBegin(mIOCycleCounter, theCurrentTime);
#else
					HAL_IOCYCLEWORKLOOPBEGIN(mIOCycleCounter, theCurrentTime);
#endif

					//	do the IO, note that we don't need to update the timing services for this special cycle
					isInNeedOfResynch = PerformIO(theCurrentTime, theIOBufferFrameSize);
					
					//	turn off overload checking for the next cycle to be nice to clients
					isCheckingForOverloads = false;
				}
				
				//	calculate the next wake up time
				if(CalculateNextWakeUpTime(theCurrentTime, theIOBufferFrameSize, theNextWakeUpTime, isCheckingForOverloads, isInNeedOfResynch, wasLocked))
				{
					//	sleep until the  next wake up time
#if Use_HAL_Telemetry
					mDevice->GetIOCycleTelemetry().IOCycleWorkLoopEnd(mIOCycleCounter, theCurrentTime, theNextWakeUpTime);
#else
					HAL_IOCYCLEWORKLOOPEND(mIOCycleCounter, theCurrentTime, theNextWakeUpTime);
#endif

					mIOGuard.WaitUntil(CAHostTimeBase::ConvertToNanos(theNextWakeUpTime.mHostTime));
					
					//	increment the cycle counter
					++mIOCycleCounter;
					
					//	make sure overload checking is enabled
					isCheckingForOverloads = true;
					
					//	do IO if the thread wasn't stopped
					if(!mStopWorkLoop)
					{
						//	get the current time
						mDevice->GetCurrentTime(theCurrentTime);
						
						if(theCurrentTime.mSampleTime >= (mAnchorTime.mSampleTime + mFrameCounter))
						{
							//	increment the frame counter
							mFrameCounter += theIOBufferFrameSize;
						
							//	refresh the current buffer size
							theIOBufferFrameSize = theNewIOBufferFrameSize;
							
							//	the new cycle is starting
#if Use_HAL_Telemetry
							mDevice->GetIOCycleTelemetry().IOCycleWorkLoopBegin(mIOCycleCounter, theCurrentTime);
#else
							HAL_IOCYCLEWORKLOOPBEGIN(mIOCycleCounter, theCurrentTime);
#endif

							if(mDevice->UpdateIOCycleTimingServices())
							{
								//	something unexpected happenned with the time stamp, so resynch prior to doing IO
								AudioTimeStamp theNewAnchor = CAAudioTimeStamp::kZero;
								theNewAnchor.mSampleTime = 0;
								theNewAnchor.mHostTime = 0;
								theNewAnchor.mFlags = kAudioTimeStampSampleTimeValid + kAudioTimeStampHostTimeValid + kAudioTimeStampRateScalarValid;
								if(mDevice->EstablishIOCycleAnchorTime(theNewAnchor))
								{
									Resynch(&theNewAnchor, false);
								}
								else
								{
									Resynch(NULL, false);
								}
								
								//	re-get the current time too
								mDevice->GetCurrentTime(theCurrentTime);
								
								//	mark the telemetry
#if Use_HAL_Telemetry
								mDevice->GetIOCycleTelemetry().Resynch(GetIOCycleNumber(), mAnchorTime);
#else
								HAL_RESYNCH(GetIOCycleNumber(), mAnchorTime);
#endif
							}
						
							//	do the IO
							isInNeedOfResynch = PerformIO(theCurrentTime, theIOBufferFrameSize);
						}
					}
				}
				else
				{
					//	calculating the next wake up time failed, so we just stop everything (which
					//	will get picked up when the commands are executed
					mDevice->ClearAllCommands();
					mDevice->Do_StopAllIOProcs();
				}
				
				//	execute any deferred commands
				mDevice->ExecuteAllCommands();
			}
		}
	
		mWorkLoopPhase = kTeardownPhase;
#if Use_HAL_Telemetry
		mDevice->GetIOCycleTelemetry().IOCycleTeardownBegin(mIOCycleCounter);
#else
		HAL_IOCYCLETEARDOWNBEGIN(mIOCycleCounter);
#endif

		//	the work loop has finished, clear the time constraints
		ClearTimeConstraints();
		
		//	tell the device that the IO thread is torn down
		mDevice->StopIOCycleTimingServices();
	}
	catch(const CAException& inException)
	{
		DebugMessageN1("HP_IOThread::WorkLoop: Caught a CAException, code == %ld", (long int)inException.GetError());
	}
	catch(...)
	{
		DebugMessage("HP_IOThread::WorkLoop: Caught an unknown exception.");
	}
	
	//	set the device state to know the engine has stopped
	mDevice->IOEngineStopped();
		
	//	Notify clients that the IO thread is stopping. Note that we unlock around this call
	//	due to the fact that clients might want to call back into the HAL.
	if(wasLocked)
	{
		mIOGuard.Unlock();
	}

	//	Notify clients that the IO thread is stopping
	mDevice->PropertiesChanged(1, &theIsRunningAddress);
		
	//	re-lock the guard
	wasLocked = mIOGuard.Lock();

#if Use_HAL_Telemetry
	mDevice->GetIOCycleTelemetry().IOCycleTeardownEnd(mIOCycleCounter);
#else
	HAL_IOCYCLETEARDOWNEND(mIOCycleCounter);
#endif

	mWorkLoopPhase = kNotRunningPhase;
	mIOGuard.NotifyAll();
	mIOCycleCounter = 0;
	
	if(wasLocked)
	{
		mIOGuard.Unlock();
	}
}

void	HP_IOThread::SetTimeConstraints()
{
	UInt64 thePeriod = 0;
	UInt32 theQuanta = 0;
	mDevice->CalculateIOThreadTimeConstraints(thePeriod, theQuanta);
	mIOThread.SetTimeConstraints(static_cast<UInt32>(thePeriod), theQuanta, static_cast<UInt32>(thePeriod), true);
}

void	HP_IOThread::ClearTimeConstraints()
{
	mIOThread.ClearTimeConstraints();
}

bool	HP_IOThread::CalculateNextWakeUpTime(const AudioTimeStamp& inCurrentTime, Float64 inIOBufferFrameSize, AudioTimeStamp& outNextWakeUpTime, bool inCheckForOverloads, bool inMustResynch, bool& inIOGuardWasLocked)
{
	bool theAnswer = true;
	
//	static const Float64 kOverloadThreshold = 0.050;
	static const Float64 kOverloadThreshold = 0.000;
	bool isDone = false;
	AudioTimeStamp theCurrentTime = inCurrentTime;
	
	int theNumberIterations = 0;
	while(!isDone)
	{
		++theNumberIterations;
		
		//	set up the outNextWakeUpTime
		outNextWakeUpTime = CAAudioTimeStamp::kZero;
		outNextWakeUpTime.mFlags = kAudioTimeStampSampleTimeValid + kAudioTimeStampHostTimeValid + kAudioTimeStampRateScalarValid;
		
		//	calculate the sample time for the next wake up time
		AudioTimeStamp theSampleTime = mAnchorTime;
		theSampleTime.mFlags = kAudioTimeStampSampleTimeValid;
		theSampleTime.mSampleTime += mFrameCounter;
		theSampleTime.mSampleTime += inIOBufferFrameSize;
		
		//	translate that to a host time
		mDevice->TranslateTime(theSampleTime, outNextWakeUpTime);
		
		if(inCheckForOverloads || inMustResynch)
		{
			//	set up the overload time
			AudioTimeStamp theOverloadTime = CAAudioTimeStamp::kZero;
			theOverloadTime.mFlags = kAudioTimeStampSampleTimeValid + kAudioTimeStampHostTimeValid + kAudioTimeStampRateScalarValid;
			
			//	calculate the overload time
			Float64 theReservedAmount = std::max(0.0, mIOCycleUsage - kOverloadThreshold);
			theSampleTime = mAnchorTime;
			theSampleTime.mFlags = kAudioTimeStampSampleTimeValid;
			theSampleTime.mSampleTime += mFrameCounter;
			theSampleTime.mSampleTime += theReservedAmount * inIOBufferFrameSize;
			
			//	translate that to a host time
			mDevice->TranslateTime(theSampleTime, theOverloadTime);
			
			if(inMustResynch || (theCurrentTime.mHostTime >= theOverloadTime.mHostTime))
			{
				//	tell the device what happenned
#if Use_HAL_Telemetry
				mDevice->GetIOCycleTelemetry().IOCycleWorkLoopOverloadBegin(mIOCycleCounter, theCurrentTime, theOverloadTime);
#else
				HAL_IOCYCLEWORKLOOPOVERLOADBEGIN(mIOCycleCounter, theCurrentTime, theOverloadTime);
#endif

				//	the current time is beyond the overload time, have to resynchronize
				#if Log_Resynchs
					if(inMustResynch)
					{
						DebugMessageN1("HP_IOThread::CalculateNextWakeUpTime: resynch was forced %d", theNumberIterations);
					}
					else
					{
						DebugMessageN1("HP_IOThread::CalculateNextWakeUpTime: wake up time is in the past... resynching %d", theNumberIterations);
						DebugMessageN3("           Now: %qd Overload: %qd Difference: %qd", CAHostTimeBase::ConvertToNanos(theCurrentTime.mHostTime), CAHostTimeBase::ConvertToNanos(theOverloadTime.mHostTime), CAHostTimeBase::ConvertToNanos(theCurrentTime.mHostTime - theOverloadTime.mHostTime));
					}
				#endif
				
				//	notify clients that the overload has taken place
				if(inIOGuardWasLocked)
				{
					mIOGuard.Unlock();
				}
				CAPropertyAddress theOverloadAddress(kAudioDeviceProcessorOverload);
				mDevice->PropertiesChanged(1, &theOverloadAddress);
				inIOGuardWasLocked = mIOGuard.Lock();

				//	re-anchor at the current time
				theCurrentTime.mSampleTime = 0;
				theCurrentTime.mHostTime = 0;
				if(mDevice->EstablishIOCycleAnchorTime(theCurrentTime))
				{
					Resynch(&theCurrentTime, false);
				}
				else
				{
					theAnswer = false;
					isDone = true;
				}
				
				//	reset the forced resynch flag
				inMustResynch = false;
#if Use_HAL_Telemetry
				mDevice->GetIOCycleTelemetry().IOCycleWorkLoopOverloadEnd(mIOCycleCounter, mAnchorTime);
#else
				HAL_IOCYCLEWORKLOOPOVERLOADEND(mIOCycleCounter, mAnchorTime);
#endif
			}
			else
			{
				//	still within the limits
				isDone = true;
			}
		}
		else
		{
			//	since we're not checking for overloads, we're also done
			isDone = true;
		}
	}
	
	//  adjust the counter depending on what happenned
	if(theNumberIterations > 1)
	{
		//  we went through the calculation more than once, which means an overload happenned
		++mOverloadCounter;
	}
	else
	{
		//  only did the calculation once, so no overload occurred
		mOverloadCounter = 0;
	}
	
	return theAnswer;
}

bool	HP_IOThread::PerformIO(const AudioTimeStamp& inCurrentTime, Float64 inIOBufferFrameSize)
{
	//	The input head is at the anchor plus the sample counter minus one
	//	buffer's worth of frames minus the safety offset.
	AudioTimeStamp theInputTime = CAAudioTimeStamp::kZero;
	if(mDevice->HasInputStreams())
	{
		AudioTimeStamp theInputFrameTime;
		theInputFrameTime = mAnchorTime;
		theInputFrameTime.mFlags = kAudioTimeStampSampleTimeValid;
		theInputFrameTime.mSampleTime += mFrameCounter;
		theInputFrameTime.mSampleTime -= mDevice->GetSafetyOffset(true);
		theInputFrameTime.mSampleTime -= inIOBufferFrameSize;
		
		//	use that to figure the corresponding host time
		theInputTime.mFlags = kAudioTimeStampSampleTimeValid + kAudioTimeStampHostTimeValid + kAudioTimeStampRateScalarValid;
		mDevice->TranslateTime(theInputFrameTime, theInputTime);
	}

	//	The output head is at the anchor plus the sample counter
	//	plus one buffer's worth of frames plus the safety offset
	AudioTimeStamp theOutputTime = CAAudioTimeStamp::kZero;
	if(mDevice->HasOutputStreams())
	{
		//	calculate the head position in frames
		AudioTimeStamp theOutputFrameTime;
		theOutputFrameTime = mAnchorTime;
		theOutputFrameTime.mFlags = kAudioTimeStampSampleTimeValid;
		theOutputFrameTime.mSampleTime += mFrameCounter;
		theOutputFrameTime.mSampleTime += mDevice->GetSafetyOffset(false);
		theOutputFrameTime.mSampleTime += (mIOCycleUsage * inIOBufferFrameSize);
		
		//	use that to figure the corresponding host time
		theOutputTime.mFlags = kAudioTimeStampSampleTimeValid + kAudioTimeStampHostTimeValid + kAudioTimeStampRateScalarValid;
		mDevice->TranslateTime(theOutputFrameTime, theOutputTime);
	}
	
	//	unlike CallIOProcs, this routine returns whether or not the caller needs to resynch.
	return !mDevice->CallIOProcs(inCurrentTime, theInputTime, theOutputTime);
}

void*	HP_IOThread::ThreadEntry(HP_IOThread* inIOThread)
{
	inIOThread->WorkLoop();
	return NULL;
}

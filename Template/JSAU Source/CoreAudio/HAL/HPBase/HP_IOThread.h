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
#if !defined(__HP_IOThread_h__)
#define __HP_IOThread_h__

//==================================================================================================
//	Includes
//==================================================================================================

//	PublicUtility Includes
#include "CAGuard.h"
#include "CAPThread.h"

//=============================================================================
//	Types
//=============================================================================

class	HP_Device;

#if Log_SchedulingLatency
	class	CALatencyLog;
#endif

//==================================================================================================
//	HP_IOThread
//==================================================================================================

class HP_IOThread
{

//	Constants
public:
	enum
	{
						kNotRunningPhase	= 0,
						kInitializingPhase	= 1,
						kRunningPhase		= 2,
						kTeardownPhase		= 3
	};

//	Construction/Destruction
public:
						HP_IOThread(HP_Device* inDevice);
	virtual				~HP_IOThread();
	
//	Operations
public:
	CAGuard&			GetIOGuard() { return mIOGuard; }
	CAGuard*			GetIOGuardPtr() { return &mIOGuard; }
	UInt32				GetIOCycleNumber() const { return mIOCycleCounter; }
	UInt64				GetOverloadCounter() const { return mOverloadCounter; }
	Float32				GetIOCycleUsage() const;
	void				SetIOCycleUsage(Float32 inIOCycleUsage);
	UInt32				GetWorkLoopPhase() const;
	bool				IsWorkLoopRunning() const { return (mWorkLoopPhase == kInitializingPhase) || (mWorkLoopPhase == kRunningPhase); }
	bool				HasBeenStopped() const;
	bool				IsCurrentThread() const;
	void				Start();
	void				Stop();
	void				Resynch(AudioTimeStamp* ioCurrentTime, bool inSignalIOThread);
	void				GetCurrentPosition(AudioTimeStamp& outTime) const;
	Float64				GetAnchorSampleTime() const { return mAnchorTime.mSampleTime; }

//	Implementation
protected:
	void				WorkLoop();
	void				SetTimeConstraints();
	void				ClearTimeConstraints();
	bool				CalculateNextWakeUpTime(const AudioTimeStamp& inCurrentTime, Float64 inIOBufferFrameSize, AudioTimeStamp& outNextWakeUpTime, bool inCheckForOverloads, bool inMustResynch, bool& inIOGuardWasLocked);
	bool				PerformIO(const AudioTimeStamp& inCurrentTime, Float64 inIOBufferFrameSize);

	static void*		ThreadEntry(HP_IOThread* inIOThread);
	
	HP_Device*			mDevice;
	CAPThread			mIOThread;
	CAGuard				mIOGuard;
	Float32				mIOCycleUsage;
	AudioTimeStamp		mAnchorTime;
	Float64				mFrameCounter;
	UInt32				mIOCycleCounter;
	UInt32				mOverloadCounter;
	UInt32				mWorkLoopPhase;
	volatile bool		mStopWorkLoop;
	
};

#endif

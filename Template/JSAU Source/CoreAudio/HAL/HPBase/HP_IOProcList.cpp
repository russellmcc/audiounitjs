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
#include "HP_IOProcList.h"

//  Internal Includes
#include "HP_Device.h"
#if Use_HAL_Telemetry
	#include "HP_IOCycleTelemetry.h"
#else
	#include "HALdtrace.h"
#endif
#include "HP_Stream.h"
#include "HP_SystemInfo.h"

//  PublicUtility Includes
#include "CAAudioBufferList.h"
#include "CAAudioTimeStamp.h"
#include "CADebugMacros.h"
#include "CAException.h"

//  System Includes
#include <Block.h>
#include <mach/mach.h>

#if	CoreAudio_Debug
//	#define	Log_IOProcActions	1
#endif

//==================================================================================================
//	HP_IOProc
//==================================================================================================

HP_IOProc::HP_IOProc(HP_Device* inDevice, AudioDeviceIOProc inIOProc, void* inClientData, bool inClientDataIsRelevant, UInt32 inIOBufferSetID, bool inAllocateBuffers, bool inUseIOBuffers)
:
	mDevice(inDevice),
	mIOProc(inIOProc),
	mClientData(inClientData),
	mClientDataIsRelevant(inClientDataIsRelevant),
#if	defined(kAudioHardwarePlugInInterface5ID)
	mDispatchQueue(NULL),
	mIOBlock(0),
#endif
	mIOBufferSetID(inIOBufferSetID),
	mAllocateBuffers(inAllocateBuffers),
	mUseIOBuffers(inUseIOBuffers),
	mIsEnabled(false),
	mStartTime(CAAudioTimeStamp::kZero),
	mStartTimeFlags(0),
	mInputStreamUsage(),
	mOutputStreamUsage(),
	mInputIOBufferList(),
	mInputAudioBufferList(NULL),
	mOutputIOBufferList(),
	mOutputAudioBufferList(NULL)
{
}

#if	defined(kAudioHardwarePlugInInterface5ID)
HP_IOProc::HP_IOProc(HP_Device* inDevice, dispatch_queue_t inDispatchQueue, AudioDeviceIOBlock inIOBlock, UInt32 inIOBufferSetID, bool inAllocateBuffers, bool inUseIOBuffers)
:
	mDevice(inDevice),
	mIOProc(NULL),
	mClientData(NULL),
	mClientDataIsRelevant(false),
	mDispatchQueue(inDispatchQueue),
	mIOBlock(inIOBlock),
	mIOBufferSetID(inIOBufferSetID),
	mAllocateBuffers(inAllocateBuffers),
	mUseIOBuffers(inUseIOBuffers),
	mIsEnabled(false),
	mStartTime(CAAudioTimeStamp::kZero),
	mStartTimeFlags(0),
	mInputStreamUsage(),
	mOutputStreamUsage(),
	mInputIOBufferList(),
	mInputAudioBufferList(NULL),
	mOutputIOBufferList(),
	mOutputAudioBufferList(NULL)
{
	if(mDispatchQueue != NULL)
	{
		dispatch_retain(mDispatchQueue);
	}
	if(mIOBlock != 0)
	{
		mIOBlock = Block_copy(mIOBlock);
	}
}
#endif

HP_IOProc::HP_IOProc(const HP_IOProc& inIOProc)
:
	mDevice(inIOProc.mDevice),
	mIOProc(inIOProc.mIOProc),
	mClientData(inIOProc.mClientData),
	mClientDataIsRelevant(inIOProc.mClientDataIsRelevant),
#if	defined(kAudioHardwarePlugInInterface5ID)
	mDispatchQueue(inIOProc.mDispatchQueue),
	mIOBlock(inIOProc.mIOBlock),
#endif
	mIOBufferSetID(inIOProc.mIOBufferSetID),
	mAllocateBuffers(inIOProc.mAllocateBuffers),
	mUseIOBuffers(inIOProc.mUseIOBuffers),
	mIsEnabled(inIOProc.mIsEnabled),
	mStartTime(inIOProc.mStartTime),
	mStartTimeFlags(inIOProc.mStartTimeFlags),
	mInputStreamUsage(inIOProc.mInputStreamUsage),
	mOutputStreamUsage(inIOProc.mOutputStreamUsage),
	mInputIOBufferList(inIOProc.mInputIOBufferList),
	mInputAudioBufferList(inIOProc.mInputAudioBufferList),
	mOutputIOBufferList(inIOProc.mOutputIOBufferList),
	mOutputAudioBufferList(inIOProc.mOutputAudioBufferList)
{
#if	defined(kAudioHardwarePlugInInterface5ID)
	if(mDispatchQueue != NULL)
	{
		dispatch_retain(mDispatchQueue);
	}
	if(mIOBlock != 0)
	{
		mIOBlock = Block_copy(mIOBlock);
	}
#endif
}

HP_IOProc::~HP_IOProc()
{
	//	free the buffer lists
	DeallocateBufferLists();
	
#if	defined(kAudioHardwarePlugInInterface5ID)
	if(mDispatchQueue != NULL)
	{
		dispatch_release(mDispatchQueue);
	}
	if(mIOBlock != 0)
	{
		Block_release(mIOBlock);
	}
#endif
}

HP_IOProc&  HP_IOProc::operator=(const HP_IOProc& inIOProc)
{
	mDevice = inIOProc.mDevice;
	mIOProc = inIOProc.mIOProc;
	mClientData = inIOProc.mClientData;
	mClientDataIsRelevant = inIOProc.mClientDataIsRelevant;
	
#if	defined(kAudioHardwarePlugInInterface5ID)
	if(mDispatchQueue != NULL)
	{
		dispatch_release(mDispatchQueue);
	}
	if(mIOBlock != 0)
	{
		Block_release(mIOBlock);
	}
	mDispatchQueue = inIOProc.mDispatchQueue;
	mIOBlock = inIOProc.mIOBlock;
	if(mDispatchQueue != NULL)
	{
		dispatch_retain(mDispatchQueue);
	}
	if(mIOBlock != 0)
	{
		mIOBlock = Block_copy(mIOBlock);
	}
#endif
	
	mIOBufferSetID = inIOProc.mIOBufferSetID;
	mAllocateBuffers = inIOProc.mAllocateBuffers;
	mUseIOBuffers = inIOProc.mUseIOBuffers;
	mIsEnabled = inIOProc.mIsEnabled;
	mStartTime = inIOProc.mStartTime;
	mStartTimeFlags = inIOProc.mStartTimeFlags;
	mInputStreamUsage = inIOProc.mInputStreamUsage;
	mOutputStreamUsage = inIOProc.mOutputStreamUsage;
	mInputIOBufferList = inIOProc.mInputIOBufferList;
	mInputAudioBufferList = inIOProc.mInputAudioBufferList;
	mOutputIOBufferList = inIOProc.mOutputIOBufferList;
	mOutputAudioBufferList = inIOProc.mOutputAudioBufferList;
	return *this;
}

bool	HP_IOProc::IsCallable(const AudioTimeStamp& inInputTime, const AudioTimeStamp& inOutputTime) const
{
	bool theAnswer = mIsEnabled;
	
	if(theAnswer && (mStartTimeFlags != 0))
	{
		if((mStartTimeFlags & kAudioDeviceStartTimeIsInputFlag) != 0)
		{
			theAnswer = inInputTime.mSampleTime >= mStartTime.mSampleTime;
		}
		else
		{
			theAnswer = inOutputTime.mSampleTime >= mStartTime.mSampleTime;
		}
	}
	
	return theAnswer;
}

void	HP_IOProc::GetIOBufferActualDataSize(bool inIsInput, UInt32 inStreamIndex, UInt32& outActualDataByteSize, UInt32& outActualDataFrameSize) const
{
	//	initialize the return values
	outActualDataByteSize = 0;
	outActualDataFrameSize = 0;
	
	//	figure out where we need to go to find the info
	const IOBufferList* theIOBufferList = GetIOBufferList(inIsInput);

	//	find the buffer for this stream
	HP_Stream* theStream = mDevice->GetStreamByIndex(inIsInput, inStreamIndex);
	if((theIOBufferList != NULL) && (theStream != NULL) && (inStreamIndex < theIOBufferList->size()))
	{
		IOBuffer* theIOBuffer = theIOBufferList->at(inStreamIndex);
		if(theIOBuffer != NULL)
		{
			//	use it to fill out the return values
			outActualDataByteSize = theIOBuffer->mActualDataByteSize;
			outActualDataFrameSize = theIOBuffer->mActualDataFrameSize;
		}
		else
		{
			//	there is no IO buffer, so return the nominal values
			outActualDataByteSize = theStream->CalculateIOBufferByteSize(mDevice->GetIOBufferFrameSize());
			outActualDataFrameSize = mDevice->GetIOBufferFrameSize();
		}
	}
	else if(theStream != NULL)
	{
		//	there is no IO buffer, so return the nominal values
		outActualDataByteSize = theStream->CalculateIOBufferByteSize(mDevice->GetIOBufferFrameSize());
		outActualDataFrameSize = mDevice->GetIOBufferFrameSize();
	}
}

void	HP_IOProc::SetIOBufferActualDataSize(bool inIsInput, UInt32 inStreamIndex, UInt32 inActualDataByteSize, UInt32 inActualDataFrameSize)
{
	//	get the IOBuffer list
	IOBufferList* theIOBufferList = GetIOBufferList(inIsInput);
	
	//	get the IOBuffer for the stream
	if(inStreamIndex < theIOBufferList->size())
	{
		IOBuffer* theIOBuffer = theIOBufferList->at(inStreamIndex);
		if(theIOBuffer != NULL)
		{
			//	set the actual size fields in the IOBuffer
			theIOBuffer->mActualDataByteSize = inActualDataByteSize;
			theIOBuffer->mActualDataFrameSize = inActualDataFrameSize;
		}
	}
		
	//	set the size in the AudioBufferList too
	AudioBufferList* theAudioBufferList = GetAudioBufferList(inIsInput);
	if((theAudioBufferList != NULL) && (inStreamIndex < theAudioBufferList->mNumberBuffers))
	{
		theAudioBufferList->mBuffers[inStreamIndex].mDataByteSize = inActualDataByteSize;
	}
}

bool	HP_IOProc::IsAnyStreamEnabled(bool inIsInput) const
{
	bool theAnswer = false;
	
	//  figure out which section's stream usage is involved
	const HP_StreamUsage& theStreamUsage = GetStreamUsage(inIsInput);
	
	//	get the number of streams
	UInt32 theNumberStreams = mDevice->GetNumberStreams(inIsInput);
	
	//  find out if the stream is enabled
	if(theNumberStreams <= theStreamUsage.size())
	{
		UInt32 theStreamIndex = 0;
		while(!theAnswer && (theStreamIndex < theStreamUsage.size()))
		{
			theAnswer = theStreamUsage.at(theStreamIndex);
			++theStreamIndex;
		}
	}
	else
	{
		//	there are more streams than this IOProc has info about
		//	by definition the streams the IOProc doesn't know about are enabled
		theAnswer = true;
	}
	
	return theAnswer;
}

bool	HP_IOProc::IsStreamEnabled(bool inIsInput, UInt32 inStreamIndex) const
{
	bool theAnswer = true;
	
	//  figure out which section's stream usage is involved
	const HP_StreamUsage& theStreamUsage = GetStreamUsage(inIsInput);
	
	//  find out if the stream is enabled
	if(inStreamIndex < theStreamUsage.size())
	{
		theAnswer = theStreamUsage.at(inStreamIndex);
	}
	
	return theAnswer;
}

void	HP_IOProc::GetStreamUsage(bool inIsInput, UInt32 inNumberStreams, bool outStreamUsage[]) const
{
	//  figure out which section's stream usage is involved
	const HP_StreamUsage& theStreamUsage = GetStreamUsage(inIsInput);

	//  get the number of streams
	UInt32 theNumberStreams = mDevice->GetNumberStreams(inIsInput);
	
	//  iterate through all the streams and figure out if each is enabled or disabled
	UInt32 theStreamIndex = 0;
	for(; (theStreamIndex < theNumberStreams) && (theStreamIndex < inNumberStreams); ++theStreamIndex)
	{
		if(theStreamIndex < theStreamUsage.size())
		{
			//  this stream's usage has been specified
			outStreamUsage[theStreamIndex] = theStreamUsage.at(theStreamIndex);
		}
		else
		{
			//  this stream's usage is unspecified, which means that it is enabled
			outStreamUsage[theStreamIndex] = true;
		}
	}
	
	//  clean up the left overs (if any) in the return value
	for(; theStreamIndex < inNumberStreams; ++theStreamIndex)
	{
		//  this stream doesn't exist
		outStreamUsage[theStreamIndex] = false;
	}
}

void	HP_IOProc::SetStreamUsage(bool inIsInput, UInt32 inNumberStreams, const bool inStreamUsage[])
{
	//  figure out which section's stream usage is involved
	HP_StreamUsage& theStreamUsage = inIsInput ? mInputStreamUsage : mOutputStreamUsage;
	
	//  clear what's there
	theStreamUsage.clear();
	
	//  get the number of streams
	UInt32 theNumberStreams = mDevice->GetNumberStreams(inIsInput);
	
	//  go through the new usage and fill in the usage
	for(UInt32 theStreamIndex = 0; (theStreamIndex < theNumberStreams) && (theStreamIndex < inNumberStreams); ++theStreamIndex)
	{
		theStreamUsage.push_back(inStreamUsage[theStreamIndex]);
	}
}

void	HP_IOProc::AllocateBufferLists()
{
	AllocateBufferList(true);
	AllocateBufferList(false);
}

void	HP_IOProc::DeallocateBufferLists()
{
	FreeBufferList(true);
	FreeBufferList(false);
}

void	HP_IOProc::ReallocateBufferLists()
{
	FreeBufferList(true);
	AllocateBufferList(true);
	FreeBufferList(false);
	AllocateBufferList(false);
}

void	HP_IOProc::AllocateBufferList(bool inIsInput)
{
	if(mIsEnabled)
	{
		if(inIsInput)
		{
			mInputAudioBufferList = AllocateBufferList(*mDevice, inIsInput, mInputStreamUsage, mIOBufferSetID, mInputIOBufferList, mUseIOBuffers, mAllocateBuffers);
		}
		else
		{
			mOutputAudioBufferList = AllocateBufferList(*mDevice, inIsInput, mOutputStreamUsage, mIOBufferSetID, mOutputIOBufferList, mUseIOBuffers, mAllocateBuffers);
		}
	}
}

void	HP_IOProc::RefreshBufferList(bool inIsInput)
{
	if(inIsInput)
	{
		if(mInputAudioBufferList != NULL)
		{
			RefreshBufferList(*mDevice, inIsInput, mInputAudioBufferList, mInputIOBufferList);
		}
	}
	else
	{
		if(mOutputAudioBufferList != NULL)
		{
			RefreshBufferList(*mDevice, inIsInput, mOutputAudioBufferList, mOutputIOBufferList);
		}
	}
}

void	HP_IOProc::FreeBufferList(bool inIsInput)
{
	if(inIsInput)
	{
		FreeBufferList(*mDevice, inIsInput, mIOBufferSetID, mInputAudioBufferList, mInputIOBufferList, mUseIOBuffers);
		mInputAudioBufferList = NULL;
	}
	else
	{
		FreeBufferList(*mDevice, inIsInput, mIOBufferSetID, mOutputAudioBufferList, mOutputIOBufferList, mUseIOBuffers);
		mOutputAudioBufferList = NULL;
	}
}

bool	HP_IOProc::BufferListHasData(bool inIsInput)
{
	bool theAnswer = false;
	if(inIsInput)
	{
		if(mInputAudioBufferList != NULL)
		{
			theAnswer = CAAudioBufferList::HasData(*mInputAudioBufferList);
		}
	}
	else
	{
		if(mOutputAudioBufferList != NULL)
		{
			theAnswer = CAAudioBufferList::HasData(*mOutputAudioBufferList);
		}
	}
	return theAnswer;
}

void	HP_IOProc::Start()
{
	mIsEnabled = true;
	mStartTime = CAAudioTimeStamp::kZero;
	mStartTimeFlags = 0;
}

void	HP_IOProc::StartAtTime(const AudioTimeStamp& inStartTime, UInt32 inStartTimeFlags)
{
	mIsEnabled = true;
	mStartTime = inStartTime;
	mStartTimeFlags = inStartTimeFlags;
}

void	HP_IOProc::Stop()
{
	mIsEnabled = false;
}

// Disabling shadow variable warnings for this routine only because we're using Blocks here.
// See <rdar://problem/6551585> -Wshadow with blocks produces spurious warning
#pragma GCC diagnostic ignored "-Wshadow"
void	HP_IOProc::Call(const AudioTimeStamp& inCurrentTime, const AudioTimeStamp& inInputTime, const AudioBufferList* inInputData, const AudioTimeStamp& inOutputTime, AudioBufferList* outOutputData)
{
	//  figure out which input buffer list to use, by default use the empty buffer list
	const AudioBufferList* theInputBufferList = &CAAudioBufferList::sEmptyBufferList;
	if(inInputData != NULL)
	{
		//  if a buffer list was passed in, then use it
		theInputBufferList = inInputData;
	}
	else if(mInputAudioBufferList != NULL)
	{
		//  otherwise, use the one we have if it has been allocated
		RefreshBufferList(*mDevice, true, mInputAudioBufferList, mInputIOBufferList);
		theInputBufferList = mInputAudioBufferList;
	}
	
	//  figure out which output buffer list to use, by default use the empty buffer list
	AudioBufferList* theOutputBufferList = &CAAudioBufferList::sEmptyBufferList;
	if(outOutputData != NULL)
	{
		//  if a buffer list was passed in, then use it
		theOutputBufferList = outOutputData;
	}
	else if(mOutputAudioBufferList != NULL)
	{
		//  otherwise, use the one we have if it has been allocated
		RefreshBufferList(*mDevice, false, mOutputAudioBufferList, mOutputIOBufferList);
		theOutputBufferList = mOutputAudioBufferList;
	}
		
	//  only call this IOProc if it is callable
	if(IsCallable(inInputTime, inOutputTime))
	{
		//	mark the telemetry
#if Use_HAL_Telemetry
		(mDevice->GetIOCycleTelemetry()).IOCycleIOProcCallBegin(mDevice->GetIOCycleNumber(), mIOProc);
#else
		HAL_IOCYCLEIOPROCCALLBEGIN(mDevice->GetIOCycleNumber(), (uint64_t)mIOProc);
#endif
		//  call the IOProc
		if(mIOProc != NULL)
		{
			mIOProc(mDevice->GetObjectID(), &inCurrentTime, theInputBufferList, &inInputTime, theOutputBufferList, &inOutputTime, mClientData);
		}
#if	defined(kAudioHardwarePlugInInterface5ID)
		else if(mIOBlock != 0)
		{
			if(mDispatchQueue != NULL)
			{
				dispatch_sync(mDispatchQueue, ^{ mIOBlock(&inCurrentTime, theInputBufferList, &inInputTime, theOutputBufferList, &inOutputTime); });
			}
			else
			{
				mIOBlock(&inCurrentTime, theInputBufferList, &inInputTime, theOutputBufferList, &inOutputTime);
			}
		}
#endif
		
		//	mark the telemetry
#if Use_HAL_Telemetry
		(mDevice->GetIOCycleTelemetry()).IOCycleIOProcCallEnd(mDevice->GetIOCycleNumber());
#else
		HAL_IOCYCLEIOPROCCALLEND(mDevice->GetIOCycleNumber());
#endif
	}
}
#pragma GCC diagnostic warning "-Wshadow"

AudioBufferList*	HP_IOProc::AllocateBufferList(const HP_Device& inDevice, bool inIsInput, const HP_StreamUsage& inStreamUsage, UInt32 inIOBufferSetID, IOBufferList& outIOBufferList, bool inUseIOBuffers, bool inAllocateBuffers)
{
	AudioBufferList* theAnswer = NULL;
	
	//	initialize the returned vector of IOBuffers
	outIOBufferList.clear();
	
	//	get the nominal IO buffer frame size
	UInt32 theIOBufferFrameSize = inDevice.GetIOBufferFrameSize();
	
	//	get the amount of padding to add to the buffers
	UInt32 theIOBufferFrameSizePad = inDevice.GetIOBufferFrameSizePadding();
	
	//	get the number of streams
	UInt32 theNumberStreams = inDevice.GetNumberStreams(inIsInput);
	
	//	allocate the AudioBufferList
	theAnswer = CAAudioBufferList::Create(theNumberStreams);
	
	//	iterate through the streams and allocate a buffer for each enabled stream
	for(UInt32 theStreamIndex = 0; theStreamIndex < theNumberStreams; ++theStreamIndex)
	{
		//  get the stream
		HP_Stream* theStream = inDevice.GetStreamByIndex(inIsInput, theStreamIndex);
		
		//	figure out the size of the buffer to allocate
		UInt32 theIOBufferByteSize = theStream->CalculateIOBufferByteSize(theIOBufferFrameSize);
		UInt32 theIOBufferByteSizePad = theStream->CalculateIOBufferByteSize(theIOBufferFrameSizePad);
		
		//  fill out the AudioBuffer fields
	
		//	figure out whether or not a buffer should be allocated
		bool shouldAllocateBuffer = true;
		if(theStreamIndex < inStreamUsage.size())
		{
			shouldAllocateBuffer = inStreamUsage.at(theStreamIndex);
		}
		
		//  allocate a buffer if needed
		if(shouldAllocateBuffer && inAllocateBuffers)
		{
			//	figure out how much space to allocate, including the IOBuffer header
			UInt32 theAllocationSize = OffsetOf32(IOBuffer, mData) + theIOBufferByteSize + theIOBufferByteSizePad;
			IOBuffer* theIOBuffer = NULL;
			
			//  allocate using vm_allocate to ensure the memory is page aligned
			kern_return_t theKernelError = vm_allocate(mach_task_self(), (vm_address_t*)&theIOBuffer, theAllocationSize, VM_FLAGS_ANYWHERE);
			AssertNoKernelError(theKernelError, "HP_IOProc::AllocateIOBufferList: vm_allocate failed");
			
			if(theIOBuffer != NULL)
			{
				//  zero out the buffer now to force it to get paged in
				memset(theIOBuffer, 0, theAllocationSize);
				
				//	fill out the IOBuffer stuff
				theIOBuffer->mActualDataByteSize = theIOBufferByteSize;
				theIOBuffer->mActualDataFrameSize = theIOBufferFrameSize;
				theIOBuffer->mTotalDataByteSize = theIOBufferByteSize + theIOBufferByteSizePad;
				theIOBuffer->mNominalDataByteSize = theIOBufferByteSize;
				
				//	register the buffer with the stream
				if(inUseIOBuffers)
				{
					theStream->RegisterIOBuffer(inIOBufferSetID, theAllocationSize, theIOBuffer);
				}
				else
				{
					theStream->RegisterIOBuffer(inIOBufferSetID, theIOBuffer->mTotalDataByteSize, &(theIOBuffer->mData[0]));
				}
				
				//	add it to the IOBuffer list
				outIOBufferList.push_back(theIOBuffer);
				
				//	fill out the AudioBufferList
				theAnswer->mBuffers[theStreamIndex].mNumberChannels = theStream->GetCurrentNumberChannels();
				theAnswer->mBuffers[theStreamIndex].mDataByteSize = theIOBuffer->mActualDataByteSize;
				theAnswer->mBuffers[theStreamIndex].mData = &(theIOBuffer->mData[0]);
			}
		}
		else
		{
			//	stick a NULL into the IOBufferList
			outIOBufferList.push_back(NULL);
				
			//	fill out the AudioBufferList
			theAnswer->mBuffers[theStreamIndex].mNumberChannels = theStream->GetCurrentNumberChannels();
			theAnswer->mBuffers[theStreamIndex].mDataByteSize = theIOBufferByteSize;
			theAnswer->mBuffers[theStreamIndex].mData = NULL;
		}
	}
	
	return theAnswer;
}

void	HP_IOProc::RefreshBufferList(const HP_Device& inDevice, bool inIsInput, AudioBufferList* ioBufferList, const IOBufferList& inIOBufferList)
{
	//	get the number of streams
	UInt32 theNumberStreams = inDevice.GetNumberStreams(inIsInput);
	
	//	iterate through the streams and allocate a buffer for each enabled stream
	for(UInt32 theStreamIndex = 0; theStreamIndex < theNumberStreams; ++theStreamIndex)
	{
		//  get the stream
		HP_Stream* theStream = inDevice.GetStreamByIndex(inIsInput, theStreamIndex);
		
		//	get the IO buffer
		IOBuffer* theIOBuffer = NULL;
		if(theStreamIndex < inIOBufferList.size())
		{
			theIOBuffer = inIOBufferList.at(theStreamIndex);
		}
		
		//	fill out the AudioBuffer fields for this stream
		if(theIOBuffer != NULL)
		{
			//	clear the data if this is for output or if this isn't an active user session
			if(!inIsInput || !HP_SystemInfo::IsCurrentProcessDoingIO())
			{
				memset(&(theIOBuffer->mData[0]), 0, theIOBuffer->mTotalDataByteSize);
			}
			
			ioBufferList->mBuffers[theStreamIndex].mNumberChannels = theStream->GetCurrentNumberChannels();
			ioBufferList->mBuffers[theStreamIndex].mDataByteSize = theIOBuffer->mNominalDataByteSize;
			ioBufferList->mBuffers[theStreamIndex].mData = &(theIOBuffer->mData[0]);
		}
		else
		{
			ioBufferList->mBuffers[theStreamIndex].mNumberChannels = theStream->GetCurrentNumberChannels();
			ioBufferList->mBuffers[theStreamIndex].mDataByteSize = theStream->CalculateIOBufferByteSize(inDevice.GetIOBufferFrameSize());
			ioBufferList->mBuffers[theStreamIndex].mData = NULL;
		}
	}
}

void	HP_IOProc::FreeBufferList(const HP_Device& inDevice, bool inIsInput, UInt32 inIOBufferSetID, AudioBufferList* inBufferList, IOBufferList& ioIOBufferList, bool inUseIOBuffers)
{
	//	toss the AudioBufferList
	if(inBufferList != NULL)
	{
		CAAudioBufferList::Destroy(inBufferList);
	}

	//	iterate through the IOBuffer list and toss all the buffers
	UInt32 theStreamIndex = 0;
	IOBufferList::iterator theIterator = ioIOBufferList.begin();
	while(theIterator != ioIOBufferList.end())
	{
		//	get the IO buffer
		IOBuffer* theIOBuffer = *theIterator;
		
		if(theIOBuffer != NULL)
		{
			//  get the stream
			HP_Stream* theStream = inDevice.GetStreamByIndex(inIsInput, theStreamIndex);
			if(theStream != NULL)
			{
				//  unregister the buffer with the stream
				if(inUseIOBuffers)
				{
					theStream->UnregisterIOBuffer(inIOBufferSetID, OffsetOf32(IOBuffer, mData) + theIOBuffer->mTotalDataByteSize, theIOBuffer);
				}
				else
				{
					theStream->UnregisterIOBuffer(inIOBufferSetID, theIOBuffer->mTotalDataByteSize, &(theIOBuffer->mData[0]));
				}
			}
			
			//  free the buffer
			vm_deallocate(mach_task_self(), (vm_address_t)theIOBuffer, offsetof(IOBuffer, mData) + theIOBuffer->mTotalDataByteSize);
		}
		
		//	go to the next one
		++theStreamIndex;
		std::advance(theIterator, 1);
	}
	
	//	clear the IOBuffer list
	ioIOBufferList.clear();
}

//==================================================================================================
//	HP_IOProcList
//==================================================================================================

HP_IOProcList::HP_IOProcList(HP_Device* inDevice, UInt32 inIOBufferSetID, bool inUseIOBuffers)
:
	mDevice(inDevice),
	mIOProcList(),
	mIOBufferSetID(inIOBufferSetID),
	mUseIOBuffers(inUseIOBuffers),
	mInputBufferListAllocationBehavior(kBufferListAllocationBehaviorShared),
	mOutputBufferListAllocationBehavior(kBufferListAllocationBehaviorAll),
	mSharedInputIOBufferList(),
	mSharedInputAudioBufferList(NULL),
	mSharedOutputIOBufferList(),
	mSharedOutputAudioBufferList(NULL),
	mNumberEnabledIOProcs(0),
	mNULLStarts(0)
{
}

HP_IOProcList::~HP_IOProcList()
{
	RemoveAllIOProcs();
	DeallocateSharedBufferLists();
}

UInt32	HP_IOProcList::GetNumberIOProcs() const
{
	return ToUInt32(mIOProcList.size());
}

HP_IOProc*	HP_IOProcList::GetIOProcByIndex(UInt32 inIndex) const
{
	HP_IOProc* theIOProc = NULL;
	if(inIndex < mIOProcList.size())
	{
		theIOProc = const_cast<HP_IOProc*>(mIOProcList.at(inIndex));
	}
	return theIOProc;
}

HP_IOProc*	HP_IOProcList::GetIOProcByIOProcID(AudioDeviceIOProcID inIOProcID) const
{
	HP_IOProc* theAnswer = NULL;
	
	//  iterate through the IOProc list
	bool wasFound = false;
	IOProcList::const_iterator theIterator = mIOProcList.begin();
	while(!wasFound && (theIterator != mIOProcList.end()))
	{
		//	get the IOProc object
		HP_IOProc* theIOProc = *theIterator;
		AudioDeviceIOProcID theIOProcID = (AudioDeviceIOProcID)theIOProc;
		
		//	For IOProc objects where the user data is relevant, the AudioDeviceIOProcID is the address
		//	of the IOProc object. Otherwise it is the IOProc itself.
		if((inIOProcID == theIOProcID) || ((inIOProcID != theIOProcID) && (theIOProc->GetIOProc() == inIOProcID)))
		{
			//  found it
			theAnswer = theIOProc;
			wasFound = true;
		}
		else
		{
			//  go to the next one
			std::advance(theIterator, 1);
		}
	}
	
	return theAnswer;
}

HP_IOProc*	HP_IOProcList::GetEnabledIOProcByIndex(UInt32 inIndex) const
{
	HP_IOProc* theAnswer = NULL;
	
	//  iterate through the IOProc list
	if(inIndex < mIOProcList.size())
	{
		bool wasFound = false;
		IOProcList::const_iterator theIterator = mIOProcList.begin();
		while(!wasFound && (theIterator != mIOProcList.end()))
		{
			//	get the IOProc object
			HP_IOProc* theIOProc = *theIterator;
			
			if(theIOProc->IsEnabled())
			{
				if(inIndex == 0)
				{
					//  found it
					theAnswer = theIOProc;
					wasFound = true;
				}
				else
				{
					//  not it, so decrement the index
					--inIndex;
				}
			}
			
			//  go to the next one
			std::advance(theIterator, 1);
		}
	}
	
	return theAnswer;
}

bool	HP_IOProcList::HasIOProcID(AudioDeviceIOProcID inIOProcID) const
{
	HP_IOProc* theIOProc = GetIOProcByIOProcID(inIOProcID);
	return theIOProc != NULL;
}

bool	HP_IOProcList::HasIOProc(AudioDeviceIOProc inIOProc, void* inClientData, bool inClientDataIsRelevant) const
{
	//	iterate through the IOProc list
	bool wasFound = false;
	IOProcList::const_iterator theIterator = mIOProcList.begin();
	while(!wasFound && (theIterator != mIOProcList.end()))
	{
		//	get the IOProc object
		HP_IOProc* theIOProc = *theIterator;
		
		//	the client data has to be relevant in both places for it to be taken into account
		if(theIOProc->IsClientDataRelevant() && inClientDataIsRelevant)
		{
			//	the client data is relevant in both, so they are the same iff the IOProc addresses
			//	are the same and the client data addresses are the same
			wasFound = (theIOProc->GetIOProc() == inIOProc) && (theIOProc->GetClientData() == inClientData);
		}
		else
		{
			//  the client data is irrelevant for one or both of the IOProcs, so they are the
			//	same iff the IOProc address is the same
			wasFound = theIOProc->GetIOProc() == inIOProc;
		}

		//	go to the next one if we're not done
		if(!wasFound)
		{
			//  go to the next one
			std::advance(theIterator, 1);
		}
	}
	
	return wasFound;
}

#if	defined(kAudioHardwarePlugInInterface5ID)

bool	HP_IOProcList::HasIOBlock(dispatch_queue_t inDispatchQueue, AudioDeviceIOBlock inIOBlock) const
{
	//	iterate through the IOProc list
	bool wasFound = false;
	IOProcList::const_iterator theIterator = mIOProcList.begin();
	while(!wasFound && (theIterator != mIOProcList.end()))
	{
		//	get the IOProc object
		HP_IOProc* theIOProc = *theIterator;
		
		//	they are the same iff the IOBlock addresses are the same and the dispatch queue addresses are the same
		wasFound = (theIOProc->GetIOBlock() == inIOBlock) && (theIOProc->GetDispatchQueue() == inDispatchQueue);

		//	go to the next one if we're not done
		if(!wasFound)
		{
			//  go to the next one
			std::advance(theIterator, 1);
		}
	}
	
	return wasFound;
}

#endif

AudioDeviceIOProcID	HP_IOProcList::AddIOProc(AudioDeviceIOProc inIOProc, void* inClientData, bool inClientDataIsRelevant)
{
	//  make a new IOProc object
	HP_IOProc* theIOProc = new HP_IOProc(mDevice, inIOProc, inClientData, inClientDataIsRelevant, mIOBufferSetID, true, mUseIOBuffers);
	
	//  add it to the list
	mIOProcList.push_back(theIOProc);
	
	AudioDeviceIOProcID theAnswer = inClientDataIsRelevant ? (AudioDeviceIOProcID)theIOProc : inIOProc;
	
	#if	Log_IOProcActions
		DebugMessageN6("HP_IOProcList::AddIOProc: (%p, %p, %p) (P: %lu N: %lu) on device %s", inIOProc, inClientData, theAnswer, mNumberEnabledIOProcs, mNULLStarts, mDevice->GetDebugDeviceName());
	#endif
	
	return theAnswer;
}

#if	defined(kAudioHardwarePlugInInterface5ID)

AudioDeviceIOProcID	HP_IOProcList::AddIOBlock(dispatch_queue_t inDispatchQueue, AudioDeviceIOBlock inIOBlock)
{
	//  make a new IOProc object
	HP_IOProc* theIOProc = new HP_IOProc(mDevice, inDispatchQueue, inIOBlock, mIOBufferSetID, true, mUseIOBuffers);
	
	//  add it to the list
	mIOProcList.push_back(theIOProc);
	
	AudioDeviceIOProcID theAnswer = (AudioDeviceIOProcID)theIOProc;
	
	#if	Log_IOProcActions
		DebugMessageN6("HP_IOProcList::AddIOBlock: (%p, %p, %p) (P: %lu N: %lu) on device %s", inDispatchQueue, inIOBlock, theAnswer, mNumberEnabledIOProcs, mNULLStarts, mDevice->GetDebugDeviceName());
	#endif
	
	return theAnswer;
}

#endif

void	HP_IOProcList::RemoveIOProc(AudioDeviceIOProcID inIOProcID)
{
	#if	Log_IOProcActions
		DebugMessageN4("HP_IOProcList::RemoveIOProc: %p (P: %lu N: %lu) on device %s", inIOProcID, mNumberEnabledIOProcs, mNULLStarts, mDevice->GetDebugDeviceName());
	#endif
	
	//  iterate through the IOProc list
	bool wasFound = false;
	IOProcList::iterator theIterator = mIOProcList.begin();
	while(!wasFound && (theIterator != mIOProcList.end()))
	{
		//	get the IOProc object
		HP_IOProc* theIOProc = *theIterator;
		AudioDeviceIOProcID theIOProcID = (AudioDeviceIOProcID)theIOProc;
		
		//	For IOProc objects where the user data is relevant, the AudioDeviceIOProcID is the address
		//	of the IOProc object. Otherwise it is the IOProc itself.
		if((inIOProcID == theIOProcID) || ((inIOProcID != theIOProcID) && (theIOProc->GetIOProc() == inIOProcID)))
		{
			//	found it
			wasFound = true;
			
			//	stop it
			StopIOProc(theIOProc);
			
			//	remove it from the list
			mIOProcList.erase(theIterator);
			
			//	and thow the object away
			delete theIOProc;
		}
		else
		{
			//  go to the next one
			std::advance(theIterator, 1);
		}
	}
}

void	HP_IOProcList::RemoveAllIOProcs()
{
	//  iterate through the list and remove all the IOProcs
	IOProcList::iterator theIterator = mIOProcList.begin();
	while(theIterator != mIOProcList.end())
	{
		//	get the IOProc object
		HP_IOProc* theIOProc = *theIterator;
		
		//	stop the IOProc
		StopIOProc(theIOProc);
		
		//	and thow the object away
		delete theIOProc;
		
		//  get the next one
		std::advance(theIterator, 1);
	}
	
	//	empty out the list
	mIOProcList.clear();
}

void	HP_IOProcList::GetIOProcStreamUsage(AudioDeviceIOProcID inIOProcID, bool inIsInput, UInt32 inNumberStreams, bool outStreamUsage[]) const
{
	//  get the IOProc
	HP_IOProc* theIOProc = GetIOProcByIOProcID(inIOProcID);
	if(theIOProc != NULL)
	{
		//  get the stream usage
		theIOProc->GetStreamUsage(inIsInput, inNumberStreams, outStreamUsage);
	}
}

void	HP_IOProcList::SetIOProcStreamUsage(AudioDeviceIOProcID inIOProcID, bool inIsInput, UInt32 inNumberStreams, const bool inStreamUsage[])
{
	//  get the IOProc
	HP_IOProc* theIOProc = GetIOProcByIOProcID(inIOProcID);
	if(theIOProc != NULL)
	{
		//  set the stream usage
		theIOProc->SetStreamUsage(inIsInput, inNumberStreams, inStreamUsage);
		
		//  reallocate the buffer lists
		if(theIOProc->IsEnabled())
		{
			ReallocateIOProcBufferLists(theIOProc);
			ReallocateSharedBufferLists();
		}
	}
}

void	HP_IOProcList::GetIOProcStreamUsageUnion(bool inIsInput, HP_StreamUsage& outStreamUsage) const
{
	//  get the number of streams
	UInt32 theNumberStreams = mDevice->GetNumberStreams(inIsInput);
	
	//  initialize the return value
	outStreamUsage.clear();
	
	//  iterate through all the streams
	for(UInt32 theStreamIndex = 0; theStreamIndex < theNumberStreams; ++theStreamIndex)
	{
		//  iterate through all the IOProcs and see if any have the stream enabled
		bool wasEnabled = false;
		IOProcList::const_iterator theIterator = mIOProcList.begin();
		while(!wasEnabled && (theIterator != mIOProcList.end()))
		{
			//	get the IOProc object
			HP_IOProc* theIOProc = *theIterator;
			
			if(theIOProc->IsEnabled())
			{
				wasEnabled = theIOProc->IsStreamEnabled(inIsInput, theStreamIndex);
			}
			std::advance(theIterator, 1);
		}
		
		//  set the return value
		outStreamUsage.push_back(wasEnabled);
	}
}

void	HP_IOProcList::AllocateAllIOProcBufferLists()
{
	//  iterate through the list
	IOProcList::iterator theIterator = mIOProcList.begin();
	while(theIterator != mIOProcList.end())
	{
		//  reallocate the buffer lists for the IOProc
		AllocateIOProcBufferLists(*theIterator);
		
		//  get the next one
		std::advance(theIterator, 1);
	}
	
	//  reallocate the shared buffer lists
	AllocateSharedBufferLists();
}

void	HP_IOProcList::DeallocateAllIOProcBufferLists()
{
	//  iterate through the list
	IOProcList::iterator theIterator = mIOProcList.begin();
	while(theIterator != mIOProcList.end())
	{
		//  reallocate the buffer lists for the IOProc
		DeallocateIOProcBufferLists(*theIterator);
		
		//  get the next one
		std::advance(theIterator, 1);
	}
	
	//  reallocate the shared buffer lists
	DeallocateSharedBufferLists();
}

void	HP_IOProcList::ReallocateAllIOProcBufferLists()
{
	//  iterate through the list
	IOProcList::iterator theIterator = mIOProcList.begin();
	while(theIterator != mIOProcList.end())
	{
		//  reallocate the buffer lists for the IOProc
		ReallocateIOProcBufferLists(*theIterator);
		
		//  get the next one
		std::advance(theIterator, 1);
	}
	
	//  reallocate the shared buffer lists
	ReallocateSharedBufferLists();
}

void	HP_IOProcList::RefreshIOProcBufferLists(bool inIsInput)
{
	//	refresh the shared buffer list
	IOBufferList* theSharedIOBufferList = GetSharedIOBufferList(inIsInput);
	AudioBufferList* theSharedAudioBufferList = GetSharedAudioBufferList(inIsInput);
	
	if(theSharedAudioBufferList != NULL)
	{
		HP_IOProc::RefreshBufferList(*mDevice, inIsInput, theSharedAudioBufferList, *theSharedIOBufferList);
	}
	
	//  iterate through the list
	IOProcList::iterator theIterator = mIOProcList.begin();
	while(theIterator != mIOProcList.end())
	{
		//	get the IOProc object
		HP_IOProc* theIOProc = *theIterator;
		
		//  refresh the buffer lists for the IOProc
		theIOProc->RefreshBufferList(inIsInput);
		
		//  get the next one
		std::advance(theIterator, 1);
	}
}

void	HP_IOProcList::RefreshAllIOProcBufferLists()
{
	//	refresh the shared input stream
	if(mSharedInputAudioBufferList != NULL)
	{
		HP_IOProc::RefreshBufferList(*mDevice, true, mSharedInputAudioBufferList, mSharedInputIOBufferList);
	}
	
	//	refresh the shared output stream
	if(mSharedOutputAudioBufferList != NULL)
	{
		HP_IOProc::RefreshBufferList(*mDevice, false, mSharedOutputAudioBufferList, mSharedOutputIOBufferList);
	}
	
	//  iterate through the list
	IOProcList::iterator theIterator = mIOProcList.begin();
	while(theIterator != mIOProcList.end())
	{
		//	get the IOProc object
		HP_IOProc* theIOProc = *theIterator;
		
		//  refresh the buffer lists for the IOProc
		theIOProc->RefreshBufferList(true);
		theIOProc->RefreshBufferList(false);
		
		//  get the next one
		std::advance(theIterator, 1);
	}
}

void	HP_IOProcList::GetIOBufferActualDataSize(bool inIsInput, UInt32 inStreamIndex, UInt32& outActualDataByteSize, UInt32& outActualDataFrameSize) const
{
	//	initialize the return values
	outActualDataByteSize = 0;
	outActualDataFrameSize = 0;
	
	//	figure out where we need to go to find the info
	const IOBufferList* theIOBufferList = NULL;
	if(GetBufferListAllocationBehavior(inIsInput) == kBufferListAllocationBehaviorShared)
	{
		//	use the shared buffers
		theIOBufferList = GetSharedIOBufferList(inIsInput);
		
	}
	else if(GetBufferListAllocationBehavior(inIsInput) == kBufferListAllocationBehaviorAll)
	{
		//	each IOProc has it's own buffers, but the size has to be the same for all of them
		HP_IOProc* theIOProc = GetEnabledIOProcByIndex(0);
		if(theIOProc != NULL)
		{
			theIOBufferList = theIOProc->GetIOBufferList(inIsInput);
		}
	}

	//	find the buffer for this stream
	HP_Stream* theStream = mDevice->GetStreamByIndex(inIsInput, inStreamIndex);
	if((theIOBufferList != NULL) && (theStream != NULL) && (inStreamIndex < theIOBufferList->size()))
	{
		IOBuffer* theIOBuffer = theIOBufferList->at(inStreamIndex);
		if(theIOBuffer != NULL)
		{
			//	use it to fill out the return values
			outActualDataByteSize = theIOBuffer->mActualDataByteSize;
			outActualDataFrameSize = theIOBuffer->mActualDataFrameSize;
		}
		else
		{
			//	there is no IO buffer, so return the nominal values
			outActualDataByteSize = theStream->CalculateIOBufferByteSize(mDevice->GetIOBufferFrameSize());
			outActualDataFrameSize = mDevice->GetIOBufferFrameSize();
		}
	}
	else if(theStream != NULL)
	{
		//	there is no IO buffer, so return the nominal values
		outActualDataByteSize = theStream->CalculateIOBufferByteSize(mDevice->GetIOBufferFrameSize());
		outActualDataFrameSize = mDevice->GetIOBufferFrameSize();
	}
}

void	HP_IOProcList::SetIOBufferActualDataSize(bool inIsInput, UInt32 inStreamIndex, UInt32 inActualDataByteSize, UInt32 inActualDataFrameSize)
{
	//	common variables
	IOBufferList* theIOBufferList;
	IOBuffer* theIOBuffer;
	AudioBufferList* theAudioBufferList;

	//	figure out where we need to go to do the work
	if(GetBufferListAllocationBehavior(inIsInput) == kBufferListAllocationBehaviorShared)
	{
		//	there is only one shared buffer
		theIOBufferList = GetSharedIOBufferList(inIsInput);
		
		//	get the IOBuffer for the stream
		if(inStreamIndex < theIOBufferList->size())
		{
			theIOBuffer = theIOBufferList->at(inStreamIndex);
			if(theIOBuffer != NULL)
			{
				//	set the actual size fields in the IOBuffer
				theIOBuffer->mActualDataByteSize = inActualDataByteSize;
				theIOBuffer->mActualDataFrameSize = inActualDataFrameSize;
			}
		}
		
		//	set the size in the AudioBufferList too
		theAudioBufferList = GetSharedAudioBufferList(inIsInput);
		if((theAudioBufferList != NULL) && (inStreamIndex < theAudioBufferList->mNumberBuffers))
		{
			theAudioBufferList->mBuffers[inStreamIndex].mDataByteSize = inActualDataByteSize;
		}
	}
	else if(GetBufferListAllocationBehavior(inIsInput) == kBufferListAllocationBehaviorAll)
	{
		//	iterate through all the IOProcs
		UInt32 theNumberIOProcs = GetNumberIOProcs();
		for(UInt32 theIOProcIndex = 0; theIOProcIndex < theNumberIOProcs; ++theIOProcIndex)
		{
			//	get the IOProc
			HP_IOProc* theIOProc = GetIOProcByIndex(theIOProcIndex);
			
			//	get it's IOBuffer list
			theIOBufferList = theIOProc->GetIOBufferList(inIsInput);
			if(theIOBufferList != NULL)
			{
				//	get the IOBuffer for the stream
				if(inStreamIndex < theIOBufferList->size())
				{
					theIOBuffer = theIOBufferList->at(inStreamIndex);
					if(theIOBuffer != NULL)
					{
						//	set the actual size fields in the IOBuffer
						theIOBuffer->mActualDataByteSize = inActualDataByteSize;
						theIOBuffer->mActualDataFrameSize = inActualDataFrameSize;
					}
				}
			}
		
			//	set the size in the AudioBufferList too
			theAudioBufferList = theIOProc->GetAudioBufferList(inIsInput);
			if((theAudioBufferList != NULL) && (inStreamIndex < theAudioBufferList->mNumberBuffers))
			{
				theAudioBufferList->mBuffers[inStreamIndex].mDataByteSize = inActualDataByteSize;
			}
		}
	}
}

void	HP_IOProcList::AllocateIOProcBufferLists(HP_IOProc* ioIOProc)
{
	//  allocate the new input buffer list
	if(mInputBufferListAllocationBehavior == kBufferListAllocationBehaviorAll)
	{
		ioIOProc->AllocateBufferList(true);
	}
	
	//  allocate the new buffer list
	if(mOutputBufferListAllocationBehavior == kBufferListAllocationBehaviorAll)
	{
		//  allocate the new buffer list
		ioIOProc->AllocateBufferList(false);
	}
}

void	HP_IOProcList::DeallocateIOProcBufferLists(HP_IOProc* ioIOProc)
{
	//  toss the old input buffer list
	ioIOProc->FreeBufferList(true);
		
	//  toss the old output buffer list
	ioIOProc->FreeBufferList(false);
}

void	HP_IOProcList::ReallocateIOProcBufferLists(HP_IOProc* ioIOProc)
{
	//  toss the old input buffer list
	ioIOProc->FreeBufferList(true);
		
	//  allocate the new input buffer list
	if(mInputBufferListAllocationBehavior == kBufferListAllocationBehaviorAll)
	{
		ioIOProc->AllocateBufferList(true);
	}
	
	//  toss the old output buffer list
	ioIOProc->FreeBufferList(false);
		
	//  allocate the new buffer list
	if(mOutputBufferListAllocationBehavior == kBufferListAllocationBehaviorAll)
	{
		//  allocate the new buffer list
		ioIOProc->AllocateBufferList(false);
	}
}

void	HP_IOProcList::AllocateSharedBufferLists()
{
	//  allocate the new shared input buffer list
	if((mInputBufferListAllocationBehavior == kBufferListAllocationBehaviorShared) || (mInputBufferListAllocationBehavior == kBufferListAllocationBehaviorNone))
	{
		//  get the union of all the IOProcs' stream usages
		HP_StreamUsage theInputStreamUsageUnion;
		GetIOProcStreamUsageUnion(true, theInputStreamUsageUnion);
		
		//	figure out if we should allocate the actual buffers
		bool shouldAllocate = mInputBufferListAllocationBehavior == kBufferListAllocationBehaviorShared;
		
		//  allocate the buffer list
		mSharedInputAudioBufferList = HP_IOProc::AllocateBufferList(*mDevice, true, theInputStreamUsageUnion, mIOBufferSetID, mSharedInputIOBufferList, mUseIOBuffers, shouldAllocate);
	}
	
	//  allocate the new shared output buffer list
	if((mOutputBufferListAllocationBehavior == kBufferListAllocationBehaviorShared) || (mOutputBufferListAllocationBehavior == kBufferListAllocationBehaviorNone))
	{
		//  get the union of all the IOProcs' stream usages
		HP_StreamUsage theOutputStreamUsageUnion;
		GetIOProcStreamUsageUnion(false, theOutputStreamUsageUnion);
		
		//	figure out if we should allocate the actual buffers
		bool shouldAllocate = mOutputBufferListAllocationBehavior == kBufferListAllocationBehaviorShared;
		
		//  allocate the buffer list
		mSharedOutputAudioBufferList = HP_IOProc::AllocateBufferList(*mDevice, false, theOutputStreamUsageUnion, mIOBufferSetID, mSharedOutputIOBufferList, mUseIOBuffers, shouldAllocate);
	}
}

void	HP_IOProcList::DeallocateSharedBufferLists()
{
	//  toss the old shared input buffer list
	if(mSharedInputAudioBufferList != NULL)
	{
		HP_IOProc::FreeBufferList(*mDevice, true, mIOBufferSetID, mSharedInputAudioBufferList, mSharedInputIOBufferList, mUseIOBuffers);
		mSharedInputAudioBufferList = NULL;
	}
	
	//  toss the old shared output buffer list
	if(mSharedOutputAudioBufferList != NULL)
	{
		HP_IOProc::FreeBufferList(*mDevice, false, mIOBufferSetID, mSharedOutputAudioBufferList, mSharedOutputIOBufferList, mUseIOBuffers);
		mSharedOutputAudioBufferList = NULL;
	}
}

void	HP_IOProcList::ReallocateSharedBufferLists()
{
	//  toss the old shared input buffer list
	if(mSharedInputAudioBufferList != NULL)
	{
		HP_IOProc::FreeBufferList(*mDevice, true, mIOBufferSetID, mSharedInputAudioBufferList, mSharedInputIOBufferList, mUseIOBuffers);
		mSharedInputAudioBufferList = NULL;
	}
	
	//  allocate the new shared input buffer list
	if((mInputBufferListAllocationBehavior == kBufferListAllocationBehaviorShared) || (mInputBufferListAllocationBehavior == kBufferListAllocationBehaviorNone))
	{
		//  get the union of all the IOProcs' stream usages
		HP_StreamUsage theInputStreamUsageUnion;
		GetIOProcStreamUsageUnion(true, theInputStreamUsageUnion);
		
		//	figure out if we should allocate the actual buffers
		bool shouldAllocate = mInputBufferListAllocationBehavior == kBufferListAllocationBehaviorShared;
		
		//  allocate the buffer list
		mSharedInputAudioBufferList = HP_IOProc::AllocateBufferList(*mDevice, true, theInputStreamUsageUnion, mIOBufferSetID, mSharedInputIOBufferList, mUseIOBuffers, shouldAllocate);
	}
	
	//  toss the old shared output buffer list
	if(mSharedOutputAudioBufferList != NULL)
	{
		HP_IOProc::FreeBufferList(*mDevice, false, mIOBufferSetID, mSharedOutputAudioBufferList, mSharedOutputIOBufferList, mUseIOBuffers);
		mSharedOutputAudioBufferList = NULL;
	}
	
	//  allocate the new shared output buffer list
	if((mOutputBufferListAllocationBehavior == kBufferListAllocationBehaviorShared) || (mOutputBufferListAllocationBehavior == kBufferListAllocationBehaviorNone))
	{
		//  get the union of all the IOProcs' stream usages
		HP_StreamUsage theOutputStreamUsageUnion;
		GetIOProcStreamUsageUnion(false, theOutputStreamUsageUnion);
		
		//	figure out if we should allocate the actual buffers
		bool shouldAllocate = mOutputBufferListAllocationBehavior == kBufferListAllocationBehaviorShared;
		
		//  allocate the buffer list
		mSharedOutputAudioBufferList = HP_IOProc::AllocateBufferList(*mDevice, false, theOutputStreamUsageUnion, mIOBufferSetID, mSharedOutputIOBufferList, mUseIOBuffers, shouldAllocate);
	}
}

void	HP_IOProcList::StartIOProc(AudioDeviceIOProcID inIOProcID)
{
	if(inIOProcID != NULL)
	{
		//  get the IOProc
		HP_IOProc* theIOProc = GetIOProcByIOProcID(inIOProcID);
		if(theIOProc != NULL)
		{
			if(!theIOProc->IsEnabled())
			{
				//  start the IOProc
				theIOProc->Start();
				++mNumberEnabledIOProcs;
				
				//  reallocate the buffer lists
				ReallocateIOProcBufferLists(theIOProc);
				ReallocateSharedBufferLists();
			}
		}
	}
	else
	{
		++mNULLStarts;
	}
	
	#if	Log_IOProcActions
		DebugMessageN4("HP_IOProcList::StartIOProc: %p (P: %lu N: %lu) on device %s", inIOProcID, mNumberEnabledIOProcs, mNULLStarts, mDevice->GetDebugDeviceName());
	#endif
}

void	HP_IOProcList::StartIOProcAtTime(AudioDeviceIOProcID inIOProcID, const AudioTimeStamp& inStartTime, UInt32 inStartTimeFlags)
{
	if(inIOProcID != NULL)
	{
		//  get the IOProc
		HP_IOProc* theIOProc = GetIOProcByIOProcID(inIOProcID);
		if(theIOProc != NULL)
		{
			if(!theIOProc->IsEnabled())
			{
				//  start the IOProc
				theIOProc->StartAtTime(inStartTime, inStartTimeFlags);
				++mNumberEnabledIOProcs;
				
				//  reallocate the buffer lists
				ReallocateIOProcBufferLists(theIOProc);
				ReallocateSharedBufferLists();
			}
		}
	}
	else
	{
		++mNULLStarts;
	}
	
	#if	Log_IOProcActions
		DebugMessageN4("HP_IOProcList::StartIOProcAtTime: %p (P: %lu N: %lu) on device %s", inIOProcID, mNumberEnabledIOProcs, mNULLStarts, mDevice->GetDebugDeviceName());
	#endif
}

void	HP_IOProcList::StopIOProc(AudioDeviceIOProcID inIOProcID)
{
	if(inIOProcID != NULL)
	{
		//  get the IOProc
		HP_IOProc* theIOProc = GetIOProcByIOProcID(inIOProcID);
		if(theIOProc != NULL)
		{
			StopIOProc(theIOProc);
		}
	}
	else
	{
		if(mNULLStarts > 0)
		{
			--mNULLStarts;
		}
	}
	
	#if	Log_IOProcActions
		DebugMessageN4("HP_IOProcList::StopIOProc: %p (P: %lu N: %lu) on device %s", inIOProcID, mNumberEnabledIOProcs, mNULLStarts, mDevice->GetDebugDeviceName());
	#endif
}

void	HP_IOProcList::StopIOProc(HP_IOProc* inIOProc)
{
	if(inIOProc->IsEnabled())
	{
		//  stop the IOProc
		inIOProc->Stop();
		--mNumberEnabledIOProcs;
		
		//  reallocate the buffer lists
		ReallocateIOProcBufferLists(inIOProc);
		ReallocateSharedBufferLists();
	}
}

void	HP_IOProcList::StopAllIOProcs()
{
	//  iterate through the list and stop all the IOProcs
	IOProcList::iterator theIterator = mIOProcList.begin();
	while(theIterator != mIOProcList.end())
	{
		//  get the IOProc
		HP_IOProc* theIOProc = *theIterator;
		
		//	stop it if it is enabled
		if(theIOProc->IsEnabled())
		{
			//  stop the IOProc
			theIOProc->Stop();
			--mNumberEnabledIOProcs;
			
			//  reallocate the IOProc's buffer lists
			ReallocateIOProcBufferLists(theIOProc);
		}
		
		//  get the next one
		std::advance(theIterator, 1);
	}
	
	//  reallocate the shared buffer lists
	ReallocateSharedBufferLists();
	
	//  clear the NULL starts
	mNULLStarts = 0;
}

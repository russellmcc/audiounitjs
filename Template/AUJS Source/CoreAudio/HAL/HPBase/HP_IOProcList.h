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
#if !defined(__HP_IOProcList_h__)
#define __HP_IOProcList_h__

//==================================================================================================
//	Includes
//==================================================================================================

//	System Includes
#include <CoreAudio/AudioHardware.h>
#include <CoreAudio/AudioHardwarePlugIn.h>

//  Standard Library Includes
#include <vector>

//==================================================================================================
//	Types
//==================================================================================================

//	This struct mirrors one that will eventually be in <IOKit/audio/IOAudioTypes.h>.
//	It is defined here to make life a bit easier on the HAL.
struct IOBuffer
{
	UInt32	mActualDataByteSize;
	UInt32	mActualDataFrameSize;
	UInt32	mTotalDataByteSize;
	UInt32	mNominalDataByteSize;
	Byte	mData[1];
};
typedef std::vector<IOBuffer*>	IOBufferList;

class						HP_Device;
typedef std::vector<bool>   HP_StreamUsage;

//==================================================================================================
//	HP_IOProc
//==================================================================================================

class HP_IOProc
{

//	Construction/Destruction
public:
							HP_IOProc(HP_Device* inDevice, AudioDeviceIOProc inIOProc, void* inClientData, bool inClientDataIsRelevant, UInt32 inIOBufferSetID, bool inAllocateBuffers, bool inUseIOBuffers);
#if	defined(kAudioHardwarePlugInInterface5ID)
							HP_IOProc(HP_Device* inDevice, dispatch_queue_t inDispatchQueue, AudioDeviceIOBlock inIOBlock, UInt32 inIOBufferSetID, bool inAllocateBuffers, bool inUseIOBuffers);
#endif
							HP_IOProc(const HP_IOProc& inIOProc);
							~HP_IOProc();
	HP_IOProc&				operator=(const HP_IOProc& inIOProc);

//  Attributes
public:
	bool					IsSameIOProcID(AudioDeviceIOProcID inIOProcID) const	{ return mClientDataIsRelevant ? (inIOProcID == reinterpret_cast<AudioDeviceIOProcID>(this)) : (inIOProcID == mIOProc); }
	AudioDeviceIOProc		GetIOProc() const { return mIOProc; }
	void*					GetClientData() const { return mClientData; }
	bool					IsClientDataRelevant() const { return mClientDataIsRelevant; }
#if	defined(kAudioHardwarePlugInInterface5ID)
	dispatch_queue_t		GetDispatchQueue() const { return mDispatchQueue; }
	AudioDeviceIOBlock		GetIOBlock() const	{ return mIOBlock; }
#endif
	bool					IsEnabled() const { return mIsEnabled; }
	bool					IsCallable(const AudioTimeStamp& inInputTime, const AudioTimeStamp& inOutputTime) const;
	
	const IOBufferList*		GetIOBufferList(bool inIsInput) const { return inIsInput ? &mInputIOBufferList : &mOutputIOBufferList; }
	IOBufferList*			GetIOBufferList(bool inIsInput) { return inIsInput ? &mInputIOBufferList : &mOutputIOBufferList; }
	const AudioBufferList*	GetAudioBufferList(bool inIsInput) const { return inIsInput ? mInputAudioBufferList : mOutputAudioBufferList; }
	AudioBufferList*		GetAudioBufferList(bool inIsInput) { return inIsInput ? mInputAudioBufferList : mOutputAudioBufferList; }
	
	void					GetIOBufferActualDataSize(bool inIsInput, UInt32 inStreamIndex, UInt32& outActualDataByteSize, UInt32& outActualDataFrameSize) const;
	void					SetIOBufferActualDataSize(bool inIsInput, UInt32 inStreamIndex, UInt32 inActualDataByteSize, UInt32 inActualDataFrameSize);

	bool					IsAnyStreamEnabled(bool inIsInput) const;
	bool					IsStreamEnabled(bool inIsInput, UInt32 inStreamIndex) const;
	const HP_StreamUsage&   GetStreamUsage(bool inIsInput) const	{ return inIsInput ? mInputStreamUsage : mOutputStreamUsage; }
	void					GetStreamUsage(bool inIsInput, UInt32 inNumberStreams, bool outStreamUsage[]) const;
	void					SetStreamUsage(bool inIsInput, UInt32 inNumberStreams, const bool inStreamUsage[]);

//  Operations
public:
	void					AllocateBufferLists();
	void					DeallocateBufferLists();
	void					ReallocateBufferLists();
	void					AllocateBufferList(bool inIsInput);
	void					RefreshBufferList(bool inIsInput);
	void					FreeBufferList(bool inIsInput);
	bool					BufferListHasData(bool inIsInput);
	
	void					Start();
	void					StartAtTime(const AudioTimeStamp& inStartTime, UInt32 inStartTimeFlags);
	void					Stop();

	void					Call(const AudioTimeStamp& inCurrentTime, const AudioTimeStamp& inInputTime, const AudioBufferList* inInputData, const AudioTimeStamp& inOutputTime, AudioBufferList* outOutputData);
	
//  Implementation
public:
	bool					IsLessThan(const HP_IOProc& inIOProc) const { return mIOProc < inIOProc.mIOProc; }
	bool					IsEqualTo(const HP_IOProc& inIOProc) const  { return mIOProc == inIOProc.mIOProc; }
	
	static AudioBufferList* AllocateBufferList(const HP_Device& inDevice, bool inIsInput, const HP_StreamUsage& inStreamUsage, UInt32 inIOBufferSetID, IOBufferList& outIOBufferList, bool inUseIOBuffers, bool inAllocateBuffers);
	static void				RefreshBufferList(const HP_Device& inDevice, bool inIsInput, AudioBufferList* ioBufferList, const IOBufferList& inIOBufferList);
	static void				FreeBufferList(const HP_Device& inDevice, bool inIsInput, UInt32 inIOBufferSetID, AudioBufferList* inBufferList, IOBufferList& ioIOBufferList, bool inUseIOBuffers);
	
private:
	HP_Device*				mDevice;
	AudioDeviceIOProc		mIOProc;
	void*					mClientData;
	bool					mClientDataIsRelevant;
#if	defined(kAudioHardwarePlugInInterface5ID)
	dispatch_queue_t		mDispatchQueue;
	AudioDeviceIOBlock		mIOBlock;
#endif
	UInt32					mIOBufferSetID;
	bool					mAllocateBuffers;
	bool					mUseIOBuffers;
	bool					mIsEnabled;
	AudioTimeStamp			mStartTime;
	UInt32					mStartTimeFlags;
	HP_StreamUsage			mInputStreamUsage;
	HP_StreamUsage			mOutputStreamUsage;
	IOBufferList			mInputIOBufferList;
	AudioBufferList*		mInputAudioBufferList;
	IOBufferList			mOutputIOBufferList;
	AudioBufferList*		mOutputAudioBufferList;

};

inline bool	operator<(const HP_IOProc& x, const HP_IOProc& y)   { return x.IsLessThan(y); }
inline bool	operator==(const HP_IOProc& x, const HP_IOProc& y)  { return x.IsEqualTo(y); }
inline bool	operator!=(const HP_IOProc& x, const HP_IOProc& y)  { return !(x == y); }
inline bool	operator<=(const HP_IOProc& x, const HP_IOProc& y)  { return (x < y) || (x == y); }
inline bool	operator>=(const HP_IOProc& x, const HP_IOProc& y)  { return !(x < y); }
inline bool	operator>(const HP_IOProc& x, const HP_IOProc& y)   { return !((x < y) || (x == y)); }

//==================================================================================================
//	HP_IOProcList
//==================================================================================================
class HP_IOProcList
{

//  Constants
public:
	enum
	{
							kBufferListAllocationBehaviorNone   = 0,
							kBufferListAllocationBehaviorShared = 1,
							kBufferListAllocationBehaviorAll	= 2
	};

//	Construction/Destruction
public:
							HP_IOProcList(HP_Device* inDevice, UInt32 inIOBufferSetID, bool inUseIOBuffers);
							~HP_IOProcList();

private:
	HP_Device*				mDevice;

//  IOProc Management
public:
	UInt32					GetNumberIOProcs() const;
	HP_IOProc*				GetIOProcByIndex(UInt32 inIndex) const;
	HP_IOProc*				GetIOProcByIOProcID(AudioDeviceIOProcID inIOProcID) const;
	HP_IOProc*				GetEnabledIOProcByIndex(UInt32 inIndex) const;

	bool					HasIOProcID(AudioDeviceIOProcID inIOProcID) const;
	bool					HasIOProc(AudioDeviceIOProc inIOProc, void* inClientData, bool inClientDataIsRelevant) const;
#if	defined(kAudioHardwarePlugInInterface5ID)
	bool					HasIOBlock(dispatch_queue_t inDispatchQueue, AudioDeviceIOBlock inIOBlock) const;
#endif
	AudioDeviceIOProcID		AddIOProc(AudioDeviceIOProc inIOProc, void* inClientData, bool inClientDataIsRelevant);
#if	defined(kAudioHardwarePlugInInterface5ID)
	AudioDeviceIOProcID		AddIOBlock(dispatch_queue_t inDispatchQueue, AudioDeviceIOBlock inIOBlock);
#endif
	void					RemoveIOProc(AudioDeviceIOProcID inIOProcID);
	void					RemoveAllIOProcs();

private:
	typedef std::vector<HP_IOProc*>	IOProcList;
	
	IOProcList				mIOProcList;

//  IOProc Attributes
public:
	void					GetIOProcStreamUsage(AudioDeviceIOProcID inIOProcID, bool inIsInput, UInt32 inNumberStreams, bool outStreamUsage[]) const;
	void					SetIOProcStreamUsage(AudioDeviceIOProcID inIOProcID, bool inIsInput, UInt32 inNumberStreams, const bool inStreamUsage[]);
	void					GetIOProcStreamUsageUnion(bool inIsInput, HP_StreamUsage& outStreamUsage) const;
	
//  IO Buffer List Management
public:
	UInt32					GetBufferListAllocationBehavior(bool inIsInput) const { return inIsInput ? mInputBufferListAllocationBehavior : mOutputBufferListAllocationBehavior; }
	void					SetBufferListAllocationBehavior(bool inIsInput, UInt32 inBehavior) { if(inIsInput) { mInputBufferListAllocationBehavior = inBehavior; } else { mOutputBufferListAllocationBehavior = inBehavior; } }
	const IOBufferList*		GetSharedIOBufferList(bool inIsInput) const { return inIsInput ? &mSharedInputIOBufferList : &mSharedOutputIOBufferList; }
	IOBufferList*			GetSharedIOBufferList(bool inIsInput) { return inIsInput ? &mSharedInputIOBufferList : &mSharedOutputIOBufferList; }
	const AudioBufferList*	GetSharedAudioBufferList(bool inIsInput) const { return inIsInput ? mSharedInputAudioBufferList : mSharedOutputAudioBufferList; }
	AudioBufferList*		GetSharedAudioBufferList(bool inIsInput) { return inIsInput ? mSharedInputAudioBufferList : mSharedOutputAudioBufferList; }
	
	void					AllocateAllIOProcBufferLists();
	void					DeallocateAllIOProcBufferLists();
	void					ReallocateAllIOProcBufferLists();
	
	void					RefreshIOProcBufferLists(bool inIsInput);
	void					RefreshAllIOProcBufferLists();
	
	void					GetIOBufferActualDataSize(bool inIsInput, UInt32 inStreamIndex, UInt32& outActualDataByteSize, UInt32& outActualDataFrameSize) const;
	void					SetIOBufferActualDataSize(bool inIsInput, UInt32 inStreamIndex, UInt32 inActualDataByteSize, UInt32 inActualDataFrameSize);

private:
	void					AllocateIOProcBufferLists(HP_IOProc* ioIOProc);
	void					DeallocateIOProcBufferLists(HP_IOProc* ioIOProc);
	void					ReallocateIOProcBufferLists(HP_IOProc* ioIOProc);

	void					AllocateSharedBufferLists();
	void					DeallocateSharedBufferLists();
	void					ReallocateSharedBufferLists();
	
	UInt32					mIOBufferSetID;
	bool					mUseIOBuffers;
	UInt32					mInputBufferListAllocationBehavior;
	UInt32					mOutputBufferListAllocationBehavior;
	IOBufferList			mSharedInputIOBufferList;
	AudioBufferList*		mSharedInputAudioBufferList;
	IOBufferList			mSharedOutputIOBufferList;
	AudioBufferList*		mSharedOutputAudioBufferList;
	
//  IO Transport Operations
public:
	void					StartIOProc(AudioDeviceIOProcID inIOProcID);
	void					StartIOProcAtTime(AudioDeviceIOProcID inIOProcID, const AudioTimeStamp& inStartTime, UInt32 inStartTimeFlags);
	void					StopIOProc(AudioDeviceIOProcID inIOProcID);
	void					StopIOProc(HP_IOProc* inIOProc);
	void					StopAllIOProcs();
	

	UInt32					GetNumberEnabledIOProcs() const		{ return mNumberEnabledIOProcs; }
	UInt32					GetNumberNULLStarts() const			{ return mNULLStarts; }
	bool					IsNothingEnabled() const			{ return ((mNumberEnabledIOProcs == 0) && (mNULLStarts == 0)); }
	bool					IsOnlyOneThingEnabled() const		{ return ((mNumberEnabledIOProcs == 1) && (mNULLStarts == 0)) || ((mNumberEnabledIOProcs == 0) && (mNULLStarts == 1)); }
	bool					IsOnlyOneIOProcEnabled() const		{ return mNumberEnabledIOProcs == 1; }
	bool					IsOnlyOneNULLEnabled() const		{ return mNULLStarts == 1; }
	bool					IsOnlyNULLEnabled() const			{ return (mNumberEnabledIOProcs == 0) && (mNULLStarts > 0); }
	bool					IsAnyIOProcEnabled() const			{ return mNumberEnabledIOProcs > 0; }
	bool					IsAnythingEnabled() const			{ return (mNumberEnabledIOProcs > 0) || (mNULLStarts > 0); }

private:
	UInt32					mNumberEnabledIOProcs;
	UInt32					mNULLStarts;
	
};

#endif

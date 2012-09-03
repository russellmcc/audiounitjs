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
#if !defined(__HP_Device_h__)
#define __HP_Device_h__

//==================================================================================================
//	Includes
//==================================================================================================

//	Super Class Includes
#include "HP_Object.h"

//  PublicUtility Includes
#include "CAMutex.h"

//  Standard Library Includes
#include <vector>

//==================================================================================================
//	Types
//==================================================================================================

class	CAGuard;
class   CAPropertyAddressList;
class	HP_Command;
class	HP_Control;
class	HP_DeviceFormatList;
#if Use_HAL_Telemetry
	class   HP_IOCycleTelemetry;
#endif
class	HP_IOProcList;
class	HP_PreferredChannels;
class   HP_Stream;

//==================================================================================================
//	HP_Device
//==================================================================================================

class HP_Device
:
	public HP_Object
{

//	Construction/Destruction
public:
							HP_Device(AudioDeviceID inAudioDeviceID, AudioClassID inClassID, HP_HardwarePlugIn* inPlugIn, UInt32 inIOBufferSetID, bool inUseIOBuffers);
	virtual					~HP_Device();
	
	virtual void			Initialize();
	virtual void			Teardown();

//  Basic Attributes
public:
	virtual bool			IsAlive() const;
	virtual bool			IsHidden() const;
	virtual CFStringRef		CopyDeviceName() const;
	virtual CFStringRef		CopyDeviceManufacturerName() const;
	virtual CFURLRef		CopyDeviceIcon() const;
	virtual CFStringRef		CopyElementFullName(const AudioObjectPropertyAddress& inAddress) const;
	virtual CFStringRef		CopyElementCategoryName(const AudioObjectPropertyAddress& inAddress) const;
	virtual CFStringRef		CopyElementNumberName(const AudioObjectPropertyAddress& inAddress) const;
	virtual CFStringRef		CopyConfigurationApplicationBundleID() const;
	virtual CFStringRef		CopyDeviceUID() const;
	virtual CFStringRef		CopyModelUID() const;
	virtual UInt32			GetTransportType() const;
	virtual bool			IsConstantRateClock() const;
	virtual bool			CanBeDefaultDevice(bool inIsInput, bool inIsSystem) const;
	virtual bool			HogModeIsOwnedBySelf() const;
	virtual bool			HogModeIsOwnedBySelfOrIsFree() const;
	virtual void			HogModeStateChanged();
	virtual void			GetDefaultChannelLayout(bool inIsInput, AudioChannelLayout& outLayout) const;
	const char*				GetDebugDeviceName() const	{ return mDebugDeviceName; }

private:
	char					mDebugDeviceName[256];

//  Basic Operations
public:
	virtual CAMutex*		GetObjectStateMutex();
	CAMutex&				GetDeviceStateMutex() { return *mDeviceStateMutex; }
	virtual void			Show() const;

protected:
	virtual void			CreateDeviceStateMutex();
	virtual void			DestroyDeviceStateMutex();
	
	CAMutex*				mDeviceStateMutex;

//	Property Access
public:
	virtual bool			HasProperty(const AudioObjectPropertyAddress& inAddress) const;
	virtual bool			IsPropertySettable(const AudioObjectPropertyAddress& inAddress) const;
	virtual UInt32			GetPropertyDataSize(const AudioObjectPropertyAddress& inAddress, UInt32 inQualifierDataSize, const void* inQualifierData) const;
	virtual void			GetPropertyData(const AudioObjectPropertyAddress& inAddress, UInt32 inQualifierDataSize, const void* inQualifierData, UInt32& ioDataSize, void* outData) const;
	virtual void			SetPropertyData(const AudioObjectPropertyAddress& inAddress, UInt32 inQualifierDataSize, const void* inQualifierData, UInt32 inDataSize, const void* inData, const AudioTimeStamp* inWhen);
#if	(MAC_OS_X_VERSION_MAX_ALLOWED < MAC_OS_X_VERSION_10_4)
	virtual void			PropertiesChanged(UInt32 inNumberAddresses, const AudioObjectPropertyAddress inAddresses[]) const;
#endif

	virtual void			ClearPrefs();
	
protected:
	HP_PreferredChannels*	mPreferredChannels;
	HP_DeviceFormatList*	mDeviceFormatList;

//	Command Management
public:
	virtual void			ExecuteCommand(HP_Command* inCommand);
	virtual void			ExecuteAllCommands();
	virtual void			ClearAllCommands();

protected:
	virtual bool			IsSafeToExecuteCommand();
	virtual bool			IsPermittedToExecuteCommand(HP_Command* inCommand);
	virtual bool			StartCommandExecution(void** outSavedCommandState);
	virtual void			FinishCommandExecution(void* inSavedCommandState);

	typedef std::vector<HP_Command*>	CommandList;
	
	CommandList				mCommandList;
	CommandList				mShadowCommandList;

//	IOProc Management
public:
	virtual	AudioDeviceIOProcID	Do_CreateIOProcID(AudioDeviceIOProc inProc, void* inClientData);
#if	defined(kAudioHardwarePlugInInterface5ID)
	virtual	AudioDeviceIOProcID	Do_CreateIOProcIDWithBlock(dispatch_queue_t inDispatchQueue, AudioDeviceIOBlock inBlock);
#endif
	virtual void				Do_DestroyIOProcID(AudioDeviceIOProcID inIOProcID);
	
	virtual void			Do_AddIOProc(AudioDeviceIOProc inProc, void* inClientData);
	virtual void			AddIOProc(AudioDeviceIOProc inProc, void* inClientData);
	
	virtual void			Do_RemoveIOProc(AudioDeviceIOProc inProc);
	virtual void			RemoveIOProc(AudioDeviceIOProc inProc);

	virtual void			Do_StartIOProc(AudioDeviceIOProcID inProcID);
	virtual void			StartIOProc(AudioDeviceIOProcID inProcID);
	
	virtual void			Do_StartIOProcAtTime(AudioDeviceIOProcID inProcID, AudioTimeStamp& ioStartTime, UInt32 inStartTimeFlags);
	virtual void			StartIOProcAtTime(AudioDeviceIOProcID inProcID, const AudioTimeStamp& inStartTime, UInt32 inStartTimeFlags);
	
	virtual void			Do_StopIOProc(AudioDeviceIOProcID inProcID);
	virtual void			StopIOProc(AudioDeviceIOProcID inProcID);
	
	virtual void			Do_StopAllIOProcs();
	virtual void			StopAllIOProcs();
	
	virtual void			Do_SetIOProcStreamUsage(AudioDeviceIOProcID inProcID, bool inIsInput, UInt32 inNumberStreams, const UInt32 inStreamUsage[]);
	virtual void			SetIOProcStreamUsage(AudioDeviceIOProcID inProcID, bool inIsInput, UInt32 inNumberStreams, const bool inStreamUsage[]);

protected:
	virtual void			CreateIOProcList();
	virtual void			DestroyIOProcList();

	HP_IOProcList*			mIOProcList;

//  IO Management
public:
	virtual UInt32			GetLatency(bool inIsInput) const;
	virtual UInt32			GetSafetyOffset(bool inIsInput) const;
	
	virtual bool			IsValidIOBufferFrameSize(UInt32 inIOBufferFrameSize) const;
	virtual UInt32			GetMinimumIOBufferFrameSize() const;
	virtual UInt32			GetMaximumIOBufferFrameSize() const;
	
	UInt32					GetIOBufferSetID() const { return mIOBufferSetID; }
	UInt32					GetIOBufferFrameSize() const	{ return mIOBufferFrameSize; }
	virtual UInt32			GetIOBufferFrameSizePadding() const;
	UInt32					DetermineIOBufferFrameSize() const;
	virtual void			Do_SetIOBufferFrameSize(UInt32 inIOBufferFrameSize);
	virtual void			Do_SetQuietIOBufferFrameSize(UInt32 inIOBufferFrameSize);
	void					SetIOBufferFrameSize(UInt32 inIOBufferFrameSize, bool inSendNotifications);
	virtual void			CalculateIOThreadTimeConstraints(UInt64& outPeriod, UInt32& outQuanta);
	
	bool					IsIOEngineRunning() const   { return mIOEngineIsRunning; }
	void					IOEngineStarted()			{ mIOEngineIsRunning = true; }
	void					IOEngineStopped()			{ mIOEngineIsRunning = false; }
	virtual bool			IsIOEngineRunningSomewhere() const;
	
	virtual CAGuard*		GetIOGuard();
	
	virtual void			Read(const AudioTimeStamp& inStartTime, AudioBufferList* outData);
	virtual bool			CallIOProcs(const AudioTimeStamp& inCurrentTime, const AudioTimeStamp& inInputTime, const AudioTimeStamp& inOutputTime);

protected:
	virtual void			IOBufferFrameSizeChanged(bool inSendNotifications, CAPropertyAddressList* outChangedProperties);
	virtual void			StartIOEngine();
	virtual void			StartIOEngineAtTime(const AudioTimeStamp& inStartTime, UInt32 inStartTimeFlags);
	virtual void			StopIOEngine();
	virtual UInt32			GetDefaultIOBufferSizeForSampleRate(Float64 inSampleRate);

	UInt32					mIOBufferSetID;
	bool					mUseIOBuffers;
	UInt32					mIOBufferFrameSize;
	bool					mIOEngineIsRunning;

//	IO Cycle Telemetry Support
public:
#if Use_HAL_Telemetry
	HP_IOCycleTelemetry&	GetIOCycleTelemetry() const { return *mIOCycleTelemetry; }
#endif
	virtual UInt32			GetIOCycleNumber() const;
	
#if Use_HAL_Telemetry
protected:
	HP_IOCycleTelemetry*	mIOCycleTelemetry;
#endif
	
//	Time Management
public:
	virtual void			GetCurrentTime(AudioTimeStamp& outTime);
	virtual void			SafeGetCurrentTime(AudioTimeStamp& outTime);
	virtual void			TranslateTime(const AudioTimeStamp& inTime, AudioTimeStamp& outTime);
	virtual void			GetNearestStartTime(AudioTimeStamp& ioRequestedStartTime, UInt32 inFlags);
	
	virtual Float64			GetCurrentNominalSampleRate() const;
	virtual Float64			GetCurrentActualSampleRate() const;

	virtual void			StartIOCycleTimingServices();
	virtual bool			EstablishIOCycleAnchorTime(AudioTimeStamp& outAnchorTime);
	virtual bool			UpdateIOCycleTimingServices();
	virtual void			StopIOCycleTimingServices();

//  Stream Management
public:
	bool					HasAnyStreams(bool inIsInput) const	{ return GetNumberStreams(inIsInput) > 0; }
	bool					HasAnyNonLinearPCMStreams() const { return HasAnyNonLinearPCMStreams(true) || HasAnyNonLinearPCMStreams(false); }
	bool					HasAnyNonLinearPCMStreams(bool inIsInput) const;
	bool					HasAnyNonMixableStreams() const { return HasAnyNonMixableStreams(true) || HasAnyNonMixableStreams(false); }
	bool					HasAnyNonMixableStreams(bool inIsInput) const;
	bool					CanSetIsMixable(bool inIsInput) const;
	bool					HasInputStreams() const { return HasAnyStreams(true); }
	bool					HasOutputStreams() const { return HasAnyStreams(false); }
	UInt32					GetNumberStreams(bool inIsInput) const { return inIsInput ? ToUInt32(mInputStreamList.size()) : ToUInt32(mOutputStreamList.size()); }
	HP_Stream*				GetStreamByIndex(bool inIsInput, UInt32 inIndex) const;
	HP_Stream*				GetStreamByDeviceChannel(bool inIsInput, UInt32 inDeviceChannel) const;
	HP_Stream*				GetStreamByPropertyAddress(const AudioObjectPropertyAddress& inAddress, bool inTryRealHard) const;
	UInt32					GetTotalNumberChannels(bool inIsInput) const;

protected:
	void					AddStream(HP_Stream* inStream);
	void					RemoveStream(HP_Stream* inStream);

	typedef std::vector<HP_Stream*>	StreamList;
	
	StreamList				mInputStreamList;
	StreamList				mOutputStreamList;

//	Control Management
public:
	virtual HP_Control*					GetControlByAddress(const AudioObjectPropertyAddress& inDeviceAddress) const;
	virtual void						ConvertDeviceAddressToControlAddress(const AudioObjectPropertyAddress& inDeviceAddress, AudioClassID& outControlClassID, AudioObjectPropertyScope& outControlScope, AudioObjectPropertyElement& outControlElement) const;
	virtual AudioObjectPropertySelector	ConvertDeviceSelectorToControlSelector(AudioObjectPropertySelector inDeviceSelector) const;
	
	virtual AudioObjectPropertySelector	GetPrimaryValueChangedPropertySelectorForControl(HP_Control* inControl) const;
	virtual AudioObjectPropertySelector	GetSecondaryValueChangedPropertySelectorForControl(HP_Control* inControl) const;
	virtual AudioObjectPropertySelector	GetThirdValueChangedPropertySelectorForControl(HP_Control* inControl) const;
	virtual AudioObjectPropertySelector	GetFourthValueChangedPropertySelectorForControl(HP_Control* inControl) const;
	
	virtual AudioObjectPropertyScope	GetPrimaryValueChangedPropertyScopeForControl(HP_Control* inControl) const;
	virtual AudioObjectPropertyScope	GetSecondaryValueChangedPropertyScopeForControl(HP_Control* inControl) const;
	virtual AudioObjectPropertyScope	GetThirdValueChangedPropertyScopeForControl(HP_Control* inControl) const;
	virtual AudioObjectPropertyScope	GetFourthValueChangedPropertyScopeForControl(HP_Control* inControl) const;
	
	virtual AudioObjectPropertySelector	GetPrimaryRangeChangedPropertySelectorForControl(HP_Control* inControl) const;
	virtual AudioObjectPropertySelector	GetSecondaryRangeChangedPropertySelectorForControl(HP_Control* inControl) const;
	
	virtual AudioObjectPropertyScope	GetPrimaryRangeChangedPropertyScopeForControl(HP_Control* inControl) const;
	virtual AudioObjectPropertyScope	GetSecondaryRangeChangedPropertyScopeForControl(HP_Control* inControl) const;

protected:
	void								AddControl(HP_Control* inControl);
	void								RemoveControl(HP_Control* inControl);
	void								ClearControlMarks();
	
	typedef std::vector<HP_Control*>	ControlList;
	
	ControlList							mControlList;

};

#endif

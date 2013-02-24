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
#if !defined(__HP_Stream_h__)
#define __HP_Stream_h__

//==================================================================================================
//	Includes
//==================================================================================================

//	Super Class Includes
#include "HP_Object.h"

//	Local Includes
#include "HP_FormatList.h"

//==================================================================================================
//	Types
//==================================================================================================

class   HP_Device;

//==================================================================================================
//	HP_Stream
//==================================================================================================

class HP_Stream
:
	public HP_Object
{

//	Construction/Destruction
public:
								HP_Stream(AudioStreamID inAudioStreamID, HP_HardwarePlugIn* inPlugIn, HP_Device* inOwningDevice, bool inIsInput, UInt32 inStartingDeviceChannelNumber);
								HP_Stream(AudioStreamID inAudioStreamID, HP_HardwarePlugIn* inPlugIn, HP_Device* inOwningDevice, bool inIsInput, UInt32 inStartingDeviceChannelNumber, UInt32 inStartingDeviceChannelNumberOffset);
	virtual						~HP_Stream();
	
	virtual void				Initialize();
	virtual void				Teardown();

//  Attributes
public:
	virtual CAMutex*			GetObjectStateMutex();
	virtual CFStringRef			CopyStreamName() const;
	virtual CFStringRef			CopyStreamManufacturerName() const;
	virtual CFStringRef			CopyElementFullName(const AudioObjectPropertyAddress& inAddress) const;
	virtual CFStringRef			CopyElementCategoryName(const AudioObjectPropertyAddress& inAddress) const;
	virtual CFStringRef			CopyElementNumberName(const AudioObjectPropertyAddress& inAddress) const;
	HP_Device*					GetOwningDevice() const					{ return mOwningDevice; }
	bool						IsInput() const							{ return mIsInput; }
	bool						IsOutput() const						{ return !mIsInput; }
	AudioObjectPropertyScope	GetDevicePropertyScope() const			{ return mIsInput ? kAudioDevicePropertyScopeInput : kAudioDevicePropertyScopeOutput; }
	AudioObjectPropertyScope	GetOppositeDevicePropertyScope() const	{ return mIsInput ? kAudioDevicePropertyScopeOutput : kAudioDevicePropertyScopeInput; }
	virtual UInt32				GetTerminalType() const;
	UInt32						GetStartingDeviceChannelNumber() const  { return mStartingDeviceChannelNumber + mStartingDeviceChannelNumberOffset; }
	void						SetStartingDeviceChannelNumberOffset(UInt32 inStartingDeviceChannelNumberOffset) { mStartingDeviceChannelNumberOffset = inStartingDeviceChannelNumberOffset; }
	virtual UInt32				GetLatency() const;

protected:
	HP_Device*					mOwningDevice;
	bool						mIsInput;
	UInt32						mStartingDeviceChannelNumber;
	UInt32						mStartingDeviceChannelNumberOffset;

//	Property Access
public:
	void						ChangeDevicePropertyAddressIntoStreamPropertyAddress(AudioObjectPropertyAddress& ioAddress) const;
	void						ChangeStreamPropertyAddressIntoDevicePropertyAddress(AudioObjectPropertyAddress& ioAddress) const;
	
	virtual bool				HasProperty(const AudioObjectPropertyAddress& inAddress) const;
	virtual bool				IsPropertySettable(const AudioObjectPropertyAddress& inAddress) const;
	virtual UInt32				GetPropertyDataSize(const AudioObjectPropertyAddress& inAddress, UInt32 inQualifierDataSize, const void* inQualifierData) const;
	virtual void				GetPropertyData(const AudioObjectPropertyAddress& inAddress, UInt32 inQualifierDataSize, const void* inQualifierData, UInt32& ioDataSize, void* outData) const;
	virtual void				SetPropertyData(const AudioObjectPropertyAddress& inAddress, UInt32 inQualifierDataSize, const void* inQualifierData, UInt32 inDataSize, const void* inData, const AudioTimeStamp* inWhen);
#if	(MAC_OS_X_VERSION_MAX_ALLOWED < MAC_OS_X_VERSION_10_4)
	virtual void				PropertiesChanged(UInt32 inNumberAddresses, const AudioObjectPropertyAddress inAddresses[]) const;
#endif

//  Basic Operations
public:
	virtual void				Show() const;
	virtual void				RegisterIOBuffer(UInt32 inBufferSetID, UInt32 inBufferByteSize, void* inBuffer);
	virtual void				UnregisterIOBuffer(UInt32 inBufferSetID, UInt32 inBufferByteSize, void* inBuffer);

//  Format Operations
public:
	UInt32						GetCurrentNumberChannels() const															{ return mFormatList->GetCurrentNumberChannels(); }
	void						GetCurrentPhysicalFormat(AudioStreamBasicDescription& outFormat) const						{ return mFormatList->GetCurrentPhysicalFormat(outFormat); }
	void						GetCurrentVirtualFormat(AudioStreamBasicDescription& outFormat) const						{ return mFormatList->GetCurrentVirtualFormat(outFormat); }	
	Float64						GetCurrentNominalSampleRate() const															{ return mFormatList->GetCurrentNominalSampleRate(); }
	void						SetCurrentNominalSampleRate(Float64 inNewSampleRate, bool inTellHardware)					{ return mFormatList->SetCurrentNominalSampleRate(inNewSampleRate, inTellHardware); }
	UInt32						GetNumberAvailableNominalSampleRateRanges() const											{ return mFormatList->GetNumberAvailableNominalSampleRateRanges(); }
	void						GetAvailableNominalSampleRateRangeByIndex(UInt32 inIndex, AudioValueRange& outRange) const	{ return mFormatList->GetAvailableNominalSampleRateRangeByIndex(inIndex, outRange); }
	bool						SupportsNominalSampleRate(Float64 inNewSampleRate) const									{ return mFormatList->SupportsNominalSampleRate(inNewSampleRate); }
	bool						IsLinearPCM() const																			{ return mFormatList->IsLinearPCM(); }
	bool						IsMixable() const																			{ return mFormatList->IsMixable(); }
	void						SetIsMixable(bool inIsMixable, bool inTellHardware = true)									{ mFormatList->SetIsMixable(inIsMixable, inTellHardware); }
	bool						CanSetIsMixable()const																		{ return mFormatList->CanSetIsMixable(); }
	UInt32						CalculateIOBufferByteSize(UInt32 inIOBufferFrameSize) const									{ return mFormatList->CalculateIOBufferByteSize(inIOBufferFrameSize); }
	UInt32						CalculateIOBufferFrameSize(UInt32 inIOBufferByteSize) const									{ return mFormatList->CalculateIOBufferFrameSize(inIOBufferByteSize); }

	virtual bool				TellHardwareToSetPhysicalFormat(const AudioStreamBasicDescription& inFormat);

protected:
	HP_FormatList*				mFormatList;

};

#endif

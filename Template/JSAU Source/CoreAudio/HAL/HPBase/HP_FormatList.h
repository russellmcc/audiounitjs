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
#if !defined(__HP_FormatList_h__)
#define __HP_FormatList_h__

//==================================================================================================
//	Includes
//==================================================================================================

//	Super Class Includes
#include "HP_Property.h"

//	PublicUtility Includes
#include "CAAudioValueRange.h"
#include "CAStreamBasicDescription.h"
#include "CAStreamRangedDescription.h"

//	Standard Library Includes
#include <vector>

//==================================================================================================
//	Types
//==================================================================================================

class   HP_Device;
class   HP_Stream;

class   CAPropertyAddressList;
typedef std::vector<CAAudioValueRange>  HP_SampleRateRangeList;

//==================================================================================================
//	HP_FormatList
//
//  This class encapsulates the necessary smarts for implementing the format related properties in
//  the HAL's API for AudioStream objects.
//==================================================================================================

class HP_FormatList
:
	public HP_Property
{

//	Construction/Destruction
public:
						HP_FormatList(HP_Stream* inOwningStream);
	virtual				~HP_FormatList();

//	Physical Format Operations
public:
	UInt32				GetCurrentNumberChannels() const { return mCurrentPhysicalFormat.mChannelsPerFrame; }
	void				GetCurrentPhysicalFormat(AudioStreamBasicDescription& outFormat) const { outFormat = mCurrentPhysicalFormat; }
	void				SetCurrentPhysicalFormat(const AudioStreamBasicDescription& inFormat, bool inTellHardware);
	
	UInt32				GetNumberAvailablePhysicalFormats() const;
	void				GetAvailablePhysicalFormatByIndex(UInt32 inIndex, AudioStreamRangedDescription& outFormat) const;
	bool				SupportsPhysicalFormat(const AudioStreamBasicDescription& inFormat) const;
	void				BestMatchForPhysicalFormat(AudioStreamBasicDescription& ioFormat) const;
	bool				SanityCheckPhysicalFormat(const AudioStreamBasicDescription& inFormat) const;
	
	void				AddPhysicalFormat(const AudioStreamRangedDescription& inPhysicalFormat);
	void				RemoveAllFormats();

//	Virtual Format Operations
public:
	void				GetCurrentVirtualFormat(AudioStreamBasicDescription& outFormat) const;
	void				SetCurrentVirtualFormat(const AudioStreamBasicDescription& inFormat, bool inTellHardware);
	
	UInt32				GetNumberAvailableVirtualFormats() const;
	void				GetAvailableVirtualFormatByIndex(UInt32 inIndex, AudioStreamRangedDescription& outFormat) const;
	bool				SupportsVirtualFormat(const AudioStreamBasicDescription& inFormat) const;
	void				BestMatchForVirtualFormat(AudioStreamBasicDescription& ioFormat) const;
	bool				SanityCheckVirtualFormat(const AudioStreamBasicDescription& inFormat) const;
	
	UInt32				CalculateIOBufferByteSize(UInt32 inIOBufferFrameSize) const;
	UInt32				CalculateIOBufferFrameSize(UInt32 inIOBufferByteSize) const;

	static UInt32		CalculateIOBufferByteSize(const AudioStreamBasicDescription& inFormat, UInt32 inIOBufferFrameSize);
	static UInt32		CalculateIOBufferFrameSize(const AudioStreamBasicDescription& inFormat, UInt32 inIOBufferByteSize);

//	Nominal Sample Rate Operations
public:
	Float64				GetCurrentNominalSampleRate() const { return mCurrentPhysicalFormat.mSampleRate; }
	void				SetCurrentNominalSampleRate(Float64 inNewSampleRate, bool inTellHardware);
	
	UInt32				GetNumberAvailableNominalSampleRateRanges() const;
	void				GetAvailableNominalSampleRateRangeByIndex(UInt32 inIndex, AudioValueRange& outRange) const;
	bool				SupportsNominalSampleRate(Float64 inNewSampleRate) const;

private:
	void				GatherAvailableNominalSampleRateRanges(HP_SampleRateRangeList& outRanges) const;

//  Mixability Operations
public:
	bool				IsLinearPCM() const;
	bool				IsMixable() const;
	void				SetIsMixable(bool inIsMixable, bool inTellHardware);
	bool				CanSetIsMixable() const;

//	Property Operations
public:
	virtual bool		IsActive(const AudioObjectPropertyAddress& inAddress) const;
	virtual bool		IsPropertySettable(const AudioObjectPropertyAddress& inAddress) const;
	virtual UInt32		GetPropertyDataSize(const AudioObjectPropertyAddress& inAddress, UInt32 inQualifierDataSize, const void* inQualifierData) const;
	virtual void		GetPropertyData(const AudioObjectPropertyAddress& inAddress, UInt32 inQualifierDataSize, const void* inQualifierData, UInt32& ioDataSize, void* outData) const;
	virtual void		SetPropertyData(const AudioObjectPropertyAddress& inAddress, UInt32 inQualifierDataSize, const void* inQualifierData, UInt32 inDataSize, const void* inData, const AudioTimeStamp* inWhen);

	virtual UInt32		GetNumberAddressesImplemented() const;
	virtual void		GetImplementedAddressByIndex(UInt32 inIndex, AudioObjectPropertyAddress& outAddress) const;
	
	static void			DetermineNotifications(const HP_Device& inDevice, const HP_Stream& inStream, const AudioStreamBasicDescription& inOldPhysicalFormat, const AudioStreamBasicDescription& inNewPhysicalFormat, CAPropertyAddressList& outDeviceNotifications, CAPropertyAddressList& outStreamNotifications);

//  Implementation
private:
	struct  AvailableFormat
	{
		CAStreamBasicDescription	mSampleFormat;
		HP_SampleRateRangeList		mSampleRateRanges;
		
		AvailableFormat() : mSampleFormat(), mSampleRateRanges() {}
		AvailableFormat(const AudioStreamBasicDescription& inSampleFormat, const AudioValueRange& inSampleRateRange) : mSampleFormat(inSampleFormat), mSampleRateRanges() { mSampleRateRanges.push_back(inSampleRateRange); }
	};
	
	typedef std::vector<AvailableFormat>	AvailableFormatList;

	static UInt32				AFL_GetNumberFormats(const AvailableFormatList& inAvailableFormatList);
	static void					AFL_GetFormatByIndex(const AvailableFormatList& inAvailableFormatList, UInt32 inIndex, AudioStreamRangedDescription& outFormat);
	static bool					AFL_FindSampleFormat(const AvailableFormatList& inAvailableFormatList, const AudioStreamBasicDescription& inFormat, HP_FormatList::AvailableFormatList::const_iterator& outIterator);
	static bool					AFL_SupportsFormat(const AvailableFormatList& inAvailableFormatList, const AudioStreamBasicDescription& inFormat);
	static void					AFL_BestMatchForFormat(const AvailableFormatList& inAvailableFormatList, const AudioStreamBasicDescription& inCurrentFormat, AudioStreamBasicDescription& ioFormat);
	static void					AFL_AddFormat(AvailableFormatList& ioAvailableFormatList, const AudioStreamRangedDescription& inFormat);
	
	static void					ConvertPhysicalFormatToVirtualFormat(AudioStreamBasicDescription& ioFormat);
	bool						ConvertVirtualFormatToPhysicalFormat(AudioStreamBasicDescription& ioFormat) const;

	HP_Stream*					mOwningStream;
	CAStreamBasicDescription	mCurrentPhysicalFormat;
	AvailableFormatList			mAvailablePhysicalFormatList;
	AvailableFormatList			mAvailableVirtualFormatList;

};

//==================================================================================================
//	HP_DeviceFormatList
//
//  This class encapsulates the necessary smarts for implementing the format related properties in
//  the HAL's API for AudioDevice objects.
//==================================================================================================

class HP_DeviceFormatList
:
	public HP_Property
{

//	Construction/Destruction
public:
						HP_DeviceFormatList(HP_Device* inOwningDevice);
	virtual				~HP_DeviceFormatList();

//	Property Operations
public:
	virtual bool		IsActive(const AudioObjectPropertyAddress& inAddress) const;
	virtual bool		IsPropertySettable(const AudioObjectPropertyAddress& inAddress) const;
	virtual UInt32		GetPropertyDataSize(const AudioObjectPropertyAddress& inAddress, UInt32 inQualifierDataSize, const void* inQualifierData) const;
	virtual void		GetPropertyData(const AudioObjectPropertyAddress& inAddress, UInt32 inQualifierDataSize, const void* inQualifierData, UInt32& ioDataSize, void* outData) const;
	virtual void		SetPropertyData(const AudioObjectPropertyAddress& inAddress, UInt32 inQualifierDataSize, const void* inQualifierData, UInt32 inDataSize, const void* inData, const AudioTimeStamp* inWhen);

	virtual UInt32		GetNumberAddressesImplemented() const;
	virtual void		GetImplementedAddressByIndex(UInt32 inIndex, AudioObjectPropertyAddress& outAddress) const;

//  Implementation
public:
	void				SetUseIntersectionForMasterRates(bool inUseIntersectionForMasterRates) { mUseIntersectionForMasterRates = inUseIntersectionForMasterRates; }
	
private:
	void				GatherAvailableNominalSampleRateRanges(HP_SampleRateRangeList& outRanges) const;
	UInt32				GetNumberAvailableMasterNominalSampleRateRanges() const;
	void				GetAvailableMasterNominalSampleRateRangeByIndex(UInt32 inIndex, AudioValueRange& outRange) const;
	bool				SupportsMasterNominalSampleRate(Float64 inNewSampleRate) const;
	
	HP_Device*			mOwningDevice;
	bool				mUseIntersectionForMasterRates;
	
};

#endif

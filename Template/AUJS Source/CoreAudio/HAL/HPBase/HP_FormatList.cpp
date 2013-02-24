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
#include "HP_FormatList.h"

//  Local Includes
#include "HP_Device.h"
#include "HP_Stream.h"

//  PublicUtility Includes
#include "CADebugMacros.h"
#include "CAException.h"

//==================================================================================================
//	HP_FormatList
//==================================================================================================

static void	AFL_ComputeUnion(const AudioValueRange& inRange, const HP_SampleRateRangeList& inRangeList, HP_SampleRateRangeList& outUnion)
{
	//	this method assumes that the ranges in inRangeList are disjoint and that they are sorted from low to high and
	
	//	start at the beginning of inRangeList
	HP_SampleRateRangeList::const_iterator theIterator = inRangeList.begin();
	
	//	iterate through inRangeList and add all the ranges that are strictly less than inRange
	while((theIterator != inRangeList.end()) && CAAudioValueRange::IsStrictlyLessThan(*theIterator, inRange))
	{
		//	put this range in the union
		outUnion.push_back(*theIterator);
		
		//	go to the next one
		std::advance(theIterator, 1);
	}
	
	if(theIterator != inRangeList.end())
	{
		if(!CAAudioValueRange::IsStrictlyGreaterThan(*theIterator, inRange))
		{
			//	inRange intersects the range that theIterator points at, but might actually intersect several contiguous ranges
			
			//	initialize the starting point, noting that we can skip the current one since we already know it's in the intersection
			HP_SampleRateRangeList::const_iterator theGreaterIterator = theIterator;
			std::advance(theGreaterIterator, 1);
			
			//	iterate until we find a range that is strictly greater than inRange
			while((theGreaterIterator != inRangeList.end()) && !CAAudioValueRange::IsStrictlyGreaterThan(*theGreaterIterator, inRange))
			{
				//	go to the next one
				std::advance(theGreaterIterator, 1);
			}
			
			//	theGreaterIterator now points at either one past the highest range in the intersection or the end of the vector
			//	Either way, we have to adjust it to point at the true highest range in the intersection
			std::advance(theGreaterIterator, -1);
			
			//	now theIterator points at the lowest range in the intersection and theGreaterIterator points at the highest
			//	so we can compute the coagulated range
			AudioValueRange theCoagulation;
			theCoagulation.mMinimum = std::min(theIterator->mMinimum, inRange.mMinimum);
			theCoagulation.mMaximum = std::max(theGreaterIterator->mMaximum, inRange.mMaximum);
			
			//	add the coagulation to the union
			outUnion.push_back(theCoagulation);
			
			//	adjust theIterator to point at the next range for processing
			theIterator = theGreaterIterator;
			std::advance(theIterator, 1);
		}
		else
		{
			//	the range theIterator points at is strictly greater than inRange, so insert inRange in front of it and we're done
			outUnion.push_back(inRange);
		}
			
		//	we need to now copy the remaining higher ranges in inRangeList into the union
		while(theIterator != inRangeList.end())
		{
			//	put this range in the union
			outUnion.push_back(*theIterator);
			
			//	go to the next one
			std::advance(theIterator, 1);
		}
	}
	else
	{
		//	inRange is larger than all of the ranges in inRangeList, so just add it onto the end of the union and we're done
		//	This is also the case if inRangeList is empty
		outUnion.push_back(inRange);
	}
}

static void	AFL_ComputeIntersection(const AudioValueRange& inRange, const HP_SampleRateRangeList& inRangeList, HP_SampleRateRangeList& outIntersections)
{
	//	iterate through the list and compute the intersections
	HP_SampleRateRangeList::const_iterator theIterator = inRangeList.begin();
	while(theIterator != inRangeList.end())
	{
		//	figure out if the range intersects
		AudioValueRange theIntersection;
		if(CAAudioValueRange::Intersection(inRange, *theIterator, theIntersection))
		{
			//	it does, so add the intersection to the return list
			outIntersections.push_back(theIntersection);
		}
		
		//	go to the next one
		std::advance(theIterator, 1);
	}
}

HP_FormatList::HP_FormatList(HP_Stream* inOwningStream)
:
	HP_Property(),
	mOwningStream(inOwningStream),
	mCurrentPhysicalFormat(),
	mAvailablePhysicalFormatList(),
	mAvailableVirtualFormatList()
{
}

HP_FormatList::~HP_FormatList()
{
}

void	HP_FormatList::SetCurrentPhysicalFormat(const AudioStreamBasicDescription& inFormat, bool inTellHardware)
{
	if(!inTellHardware || mOwningStream->TellHardwareToSetPhysicalFormat(inFormat))
	{
		mCurrentPhysicalFormat = inFormat;
	}
}

UInt32	HP_FormatList::GetNumberAvailablePhysicalFormats() const
{
	return AFL_GetNumberFormats(mAvailablePhysicalFormatList);
}

void	HP_FormatList::GetAvailablePhysicalFormatByIndex(UInt32 inIndex, AudioStreamRangedDescription& outFormat) const
{
	AFL_GetFormatByIndex(mAvailablePhysicalFormatList, inIndex, outFormat);
}

bool	HP_FormatList::SupportsPhysicalFormat(const AudioStreamBasicDescription& inFormat) const
{
	return AFL_SupportsFormat(mAvailablePhysicalFormatList, inFormat);
}

void	HP_FormatList::BestMatchForPhysicalFormat(AudioStreamBasicDescription& ioFormat) const
{
	AFL_BestMatchForFormat(mAvailablePhysicalFormatList, mCurrentPhysicalFormat, ioFormat);
}

bool	HP_FormatList::SanityCheckPhysicalFormat(const AudioStreamBasicDescription& inFormat) const
{
	//	This method performs a check on inFormat to see if it is likely to be supported by this stream.
	bool theAnswer = true;
	AudioStreamBasicDescription theTestFormat;
	
	//	check if the sample rate is supported
	if(theAnswer && (inFormat.mSampleRate != 0.0))
	{
		theAnswer = SupportsNominalSampleRate(inFormat.mSampleRate);
	}
	
	//	check if the format ID is supported
	if(theAnswer && (inFormat.mFormatID != 0))
	{
		memset(&theTestFormat, 0, sizeof(AudioStreamBasicDescription));
		theTestFormat.mFormatID = inFormat.mFormatID;
		theAnswer = SupportsPhysicalFormat(theTestFormat);
	}
	
	//	check if the number of channels is supported
	if(theAnswer && (inFormat.mChannelsPerFrame != 0))
	{
		memset(&theTestFormat, 0, sizeof(AudioStreamBasicDescription));
		theTestFormat.mChannelsPerFrame = inFormat.mChannelsPerFrame;
		theAnswer = SupportsPhysicalFormat(theTestFormat);
	}
	
	//	if the format is linear PCM, check if it's parts are supported
	if(theAnswer && (inFormat.mFormatID == kAudioFormatLinearPCM))
	{
		memset(&theTestFormat, 0, sizeof(AudioStreamBasicDescription));
		theTestFormat.mFormatID = inFormat.mFormatID;
		theTestFormat.mFormatFlags = inFormat.mFormatFlags;
		theTestFormat.mBitsPerChannel = inFormat.mBitsPerChannel;
		theAnswer = SupportsPhysicalFormat(theTestFormat);
	}
	
	return theAnswer;
}

void	HP_FormatList::AddPhysicalFormat(const AudioStreamRangedDescription& inPhysicalFormat)
{
	//	reject formats with 0 channels
	if(inPhysicalFormat.mFormat.mChannelsPerFrame > 0)
	{
		//  add it to the physical format list
		AFL_AddFormat(mAvailablePhysicalFormatList, inPhysicalFormat);
		
		//  make a virtual format out of it
		AudioStreamRangedDescription theVirtualFormat = inPhysicalFormat;
#if !HAL_Embedded_Build
		CAStreamBasicDescription::NormalizeLinearPCMFormat(theVirtualFormat.mFormat);
#endif
		
		//  add it to the virtual format list
		AFL_AddFormat(mAvailableVirtualFormatList, theVirtualFormat);
	}
}

void	HP_FormatList::RemoveAllFormats()
{
	mAvailablePhysicalFormatList.clear();
	mAvailableVirtualFormatList.clear();
}

void	HP_FormatList::GetCurrentVirtualFormat(AudioStreamBasicDescription& outFormat) const
{
	outFormat = mCurrentPhysicalFormat;
#if !HAL_Embedded_Build
	CAStreamBasicDescription::NormalizeLinearPCMFormat(outFormat);
#endif
}

void	HP_FormatList::SetCurrentVirtualFormat(const AudioStreamBasicDescription& inFormat, bool inTellHardware)
{
	AudioStreamBasicDescription thePhysicalFormat = inFormat;
	if(ConvertVirtualFormatToPhysicalFormat(thePhysicalFormat))
	{
		SetCurrentPhysicalFormat(thePhysicalFormat, inTellHardware);
	}
	else
	{
		DebugMessage("HP_FormatList::SetCurrentVirtualFormat: no reasonable format could be found");
		throw CAException(kAudioDeviceUnsupportedFormatError);
	}
}

UInt32	HP_FormatList::GetNumberAvailableVirtualFormats() const
{
	return AFL_GetNumberFormats(mAvailableVirtualFormatList);
}

void	HP_FormatList::GetAvailableVirtualFormatByIndex(UInt32 inIndex, AudioStreamRangedDescription& outFormat) const
{
	AFL_GetFormatByIndex(mAvailableVirtualFormatList, inIndex, outFormat);
}

bool	HP_FormatList::SupportsVirtualFormat(const AudioStreamBasicDescription& inFormat) const
{
	return AFL_SupportsFormat(mAvailableVirtualFormatList, inFormat);
}

void	HP_FormatList::BestMatchForVirtualFormat(AudioStreamBasicDescription& ioFormat) const
{
	AudioStreamBasicDescription theVirtualFormat;
	GetCurrentVirtualFormat(theVirtualFormat);
	AFL_BestMatchForFormat(mAvailableVirtualFormatList, theVirtualFormat, ioFormat);
}

bool	HP_FormatList::SanityCheckVirtualFormat(const AudioStreamBasicDescription& inFormat) const
{
	//	This method performs a check on inFormat to see if it is likely to be supported by this stream.
	bool theAnswer = true;
	AudioStreamBasicDescription theTestFormat;
	
	//	check if the sample rate is supported
	if(theAnswer && (inFormat.mSampleRate != 0.0))
	{
		theAnswer = SupportsNominalSampleRate(inFormat.mSampleRate);
	}
	
	//	check if the format ID is supported
	if(theAnswer && (inFormat.mFormatID != 0))
	{
		memset(&theTestFormat, 0, sizeof(AudioStreamBasicDescription));
		theTestFormat.mFormatID = inFormat.mFormatID;
		theAnswer = SupportsVirtualFormat(theTestFormat);
	}
	
	//	check if the number of channels is supported
	if(theAnswer && (inFormat.mChannelsPerFrame != 0))
	{
		memset(&theTestFormat, 0, sizeof(AudioStreamBasicDescription));
		theTestFormat.mChannelsPerFrame = inFormat.mChannelsPerFrame;
		theAnswer = SupportsVirtualFormat(theTestFormat);
	}
	
	//	if the format is linear PCM, check if it is canonical
	if(theAnswer && (inFormat.mFormatID == kAudioFormatLinearPCM))
	{
		//	check the format flags
		if(theAnswer && (inFormat.mFormatFlags != 0))
		{
			theAnswer = inFormat.mFormatFlags == kAudioFormatFlagsNativeFloatPacked;
		}
		
		//	check the bit depth
		if(theAnswer && (inFormat.mBitsPerChannel != 0))
		{
			theAnswer = inFormat.mBitsPerChannel == 32;
		}
	}
	
	return theAnswer;
}

UInt32	HP_FormatList::CalculateIOBufferByteSize(UInt32 inIOBufferFrameSize) const
{
	AudioStreamBasicDescription theVirtualFormat;
	GetCurrentVirtualFormat(theVirtualFormat);
	return CalculateIOBufferByteSize(theVirtualFormat, inIOBufferFrameSize);
}

UInt32	HP_FormatList::CalculateIOBufferFrameSize(UInt32 inIOBufferByteSize) const
{
	AudioStreamBasicDescription theVirtualFormat;
	GetCurrentVirtualFormat(theVirtualFormat);
	return CalculateIOBufferFrameSize(theVirtualFormat, inIOBufferByteSize);
}

UInt32  HP_FormatList::CalculateIOBufferByteSize(const AudioStreamBasicDescription& inFormat, UInt32 inIOBufferFrameSize)
{
	UInt32 theAnswer = 0;
	
	//  figure out how big the IO buffer needs to be for the given amount of frames
	switch(inFormat.mFormatID)
	{
		case kAudioFormatLinearPCM:
			//	linear PCM is simple, note that the virtual format will be
			//	the right thing even if the format isn't mixable
			theAnswer = inFormat.mBytesPerFrame * inIOBufferFrameSize;
			break;
		
		case kAudioFormat60958AC3:
			//	IEC 60958 formats work like 16 bit stereo linear PCM
			theAnswer = 2 * SizeOf32(SInt16) * inIOBufferFrameSize;
			break;
			
		case kAudioFormatAC3:
		case kAudioFormatMPEGLayer1:
		case kAudioFormatMPEGLayer2:
		case kAudioFormatMPEGLayer3:
		default:
			//	everything else is required to work one packet at a time
			Assert(inIOBufferFrameSize == inFormat.mFramesPerPacket, "HP_FormatList::CalculateIOBufferByteSize: illegal buffer size");
			theAnswer = inFormat.mBytesPerPacket;
			break;
	};
	
	return theAnswer;
}

UInt32  HP_FormatList::CalculateIOBufferFrameSize(const AudioStreamBasicDescription& inFormat, UInt32 inIOBufferByteSize)
{
	UInt32 theAnswer = 0;
	
	//  figure out how big the IO buffer needs to be for the given amount of frames
	switch(inFormat.mFormatID)
	{
		case kAudioFormatLinearPCM:
			//	linear PCM is simple, note that the virtual format will be
			//	the right thing even if the format isn't mixable
			theAnswer = inIOBufferByteSize / inFormat.mBytesPerFrame;
			break;
		
		case kAudioFormat60958AC3:
			//	IEC 60958 formats work like 16 bit stereo linear PCM
			theAnswer = inIOBufferByteSize / (2 * SizeOf32(SInt16));
			break;
			
		case kAudioFormatAC3:
		case kAudioFormatMPEGLayer1:
		case kAudioFormatMPEGLayer2:
		case kAudioFormatMPEGLayer3:
		default:
			//	everything else is required to work one packet at a time
			Assert(inIOBufferByteSize == inFormat.mBytesPerPacket, "HP_FormatList::CalculateIOBufferFrameSize: illegal buffer size");
			theAnswer = inFormat.mFramesPerPacket;
			break;
	};
	
	return theAnswer;
}

void	HP_FormatList::SetCurrentNominalSampleRate(Float64 inNewSampleRate, bool inTellHardware)
{
	if(SupportsNominalSampleRate(inNewSampleRate))
	{
		//	we know there's a format in there, that supports it, we just need to find it
		
		//	try to change just the sample rate
		AudioStreamBasicDescription thePhysicalFormat = mCurrentPhysicalFormat;
		thePhysicalFormat.mSampleRate = inNewSampleRate;
		
		//	make sure this new format is supported
		if(!SupportsPhysicalFormat(thePhysicalFormat) && (thePhysicalFormat.mFormatID == kAudioFormatLinearPCM))
		{
			//	it isn't, so 0 out a few fields and look for a best match
			thePhysicalFormat = mCurrentPhysicalFormat;
			thePhysicalFormat.mSampleRate = inNewSampleRate;
			thePhysicalFormat.mBytesPerPacket = 0;
			thePhysicalFormat.mBytesPerFrame = 0;
			thePhysicalFormat.mChannelsPerFrame = 0;
			BestMatchForPhysicalFormat(thePhysicalFormat);
			
			if(thePhysicalFormat.mSampleRate != inNewSampleRate)
			{
				//	still couldn't find the format, so widen the search a bit more
				thePhysicalFormat = mCurrentPhysicalFormat;
				thePhysicalFormat.mSampleRate = inNewSampleRate;
				thePhysicalFormat.mFormatFlags = 0;
				thePhysicalFormat.mBytesPerPacket = 0;
				thePhysicalFormat.mFramesPerPacket = 0;
				thePhysicalFormat.mBytesPerFrame = 0;
				thePhysicalFormat.mChannelsPerFrame = 0;
				thePhysicalFormat.mBitsPerChannel = 0;
				BestMatchForPhysicalFormat(thePhysicalFormat);
			}
		}
		
		//	set the format
		SetCurrentPhysicalFormat(thePhysicalFormat, inTellHardware);
	}
}

UInt32	HP_FormatList::GetNumberAvailableNominalSampleRateRanges() const
{
	HP_SampleRateRangeList theAvailableRates;
	GatherAvailableNominalSampleRateRanges(theAvailableRates);
	return ToUInt32(theAvailableRates.size());
}

void	HP_FormatList::GetAvailableNominalSampleRateRangeByIndex(UInt32 inIndex, AudioValueRange& outRange) const
{
	HP_SampleRateRangeList theAvailableRates;
	GatherAvailableNominalSampleRateRanges(theAvailableRates);
	if(inIndex < theAvailableRates.size())
	{
		outRange = theAvailableRates.at(inIndex);
	}
}

bool	HP_FormatList::SupportsNominalSampleRate(Float64 inNewSampleRate) const
{
	bool theAnswer = false;
	
	//	get the available rates
	HP_SampleRateRangeList theAvailableRates;
	GatherAvailableNominalSampleRateRanges(theAvailableRates);
	
	//	iterate through the available rates
	HP_SampleRateRangeList::const_iterator theRateRangeIterator = theAvailableRates.begin();
	while(!theAnswer && (theRateRangeIterator != theAvailableRates.end()))
	{
		if(CAAudioValueRange::ContainsValue(*theRateRangeIterator, inNewSampleRate))
		{
			theAnswer = true;
		}
		else
		{
			std::advance(theRateRangeIterator, 1);
		}
	}
	
	return theAnswer;
}

void	HP_FormatList::GatherAvailableNominalSampleRateRanges(HP_SampleRateRangeList& outRanges) const
{
	//	clear the return value
	outRanges.clear();
	
//#define	Use_Intersection	1
#if	!Use_Intersection

	//	iterate through the physical formats
	HP_SampleRateRangeList theUnion;
	UInt32 theNumberFormats = AFL_GetNumberFormats(mAvailablePhysicalFormatList);
	for(UInt32 theFormatIndex = 0; theFormatIndex < theNumberFormats; ++theFormatIndex)
	{
		//	get the format
		AudioStreamRangedDescription theFormat;
		AFL_GetFormatByIndex(mAvailablePhysicalFormatList, theFormatIndex, theFormat);
		
		//	clear the union
		theUnion.clear();
		
		//	compute the union
		AFL_ComputeUnion(theFormat.mSampleRateRange, outRanges, theUnion);
		
		//	copy the union to the return value
		outRanges = theUnion;
	}

#else

	//  get the current sample format
	AudioStreamBasicDescription theFormat = mCurrentPhysicalFormat;
	theFormat.mSampleRate = 0;
	
	//  find it in the list
	AvailableFormatList::const_iterator theIterator;
	if(AFL_FindSampleFormat(mAvailablePhysicalFormatList, theFormat, theIterator))
	{
		outRanges = theIterator->mSampleRateRanges;
	}

#endif

}

bool	HP_FormatList::IsLinearPCM() const
{
	return mCurrentPhysicalFormat.mFormatID == kAudioFormatLinearPCM;
}

bool	HP_FormatList::IsMixable() const
{
	bool theAnswer = mCurrentPhysicalFormat.mFormatID == kAudioFormatLinearPCM;
	if(theAnswer)
	{
		theAnswer = (mCurrentPhysicalFormat.mFormatFlags & kAudioFormatFlagIsNonMixable) == 0;
	}
	return theAnswer;
}

void	HP_FormatList::SetIsMixable(bool inIsMixable, bool inTellHardware)
{
	if(inIsMixable != IsMixable())
	{
		AudioStreamBasicDescription theNewPhysicalFormat = mCurrentPhysicalFormat;
		if(inIsMixable)
		{
			//  turn off the flag
			theNewPhysicalFormat.mFormatFlags &= ~kAudioFormatFlagIsNonMixable;
		}
		else
		{
			//  turn on the flag
			theNewPhysicalFormat.mFormatFlags |= kAudioFormatFlagIsNonMixable;
		}
		SetCurrentPhysicalFormat(theNewPhysicalFormat, inTellHardware);
	}
}

bool	HP_FormatList::CanSetIsMixable() const
{
	//  non-linear PCM is not mixable by definition
	bool theAnswer = mCurrentPhysicalFormat.mFormatID == kAudioFormatLinearPCM;
	
	//  but linear PCM data might be able to switch
	if(theAnswer)
	{
		//  have to look to see if there is a mixable/non-mixable counter part to the current format
		AudioStreamBasicDescription thePhysicalFormat = mCurrentPhysicalFormat;
		if((thePhysicalFormat.mFormatFlags & kAudioFormatFlagIsNonMixable) != 0)
		{
			//  turn off the flag
			thePhysicalFormat.mFormatFlags &= ~kAudioFormatFlagIsNonMixable;
		}
		else
		{
			//  turn on the flag
			thePhysicalFormat.mFormatFlags |= kAudioFormatFlagIsNonMixable;
		}
		
		//  ignore the sample rate, since we're looking just for the sample format
		thePhysicalFormat.mSampleRate = 0;
		
		//  look for it in the list
		theAnswer = SupportsPhysicalFormat(thePhysicalFormat);
	}
	
	return theAnswer;
}

bool	HP_FormatList::IsActive(const AudioObjectPropertyAddress& /*inAddress*/) const
{
	return true;
}

bool	HP_FormatList::IsPropertySettable(const AudioObjectPropertyAddress& inAddress) const
{
	bool theAnswer = false;
	
	switch(inAddress.mSelector)
	{
		//  device properties
		case kAudioDevicePropertySupportsMixing:
			theAnswer = CanSetIsMixable();
			break;
			
		case kAudioDevicePropertyNominalSampleRate:
			theAnswer = true;
			break;
			
		case kAudioDevicePropertyAvailableNominalSampleRates:
			theAnswer = false;
			break;
		
		//  stream properties
		case kAudioStreamPropertyVirtualFormat:
			//  aka kAudioDevicePropertyStreamFormat
			theAnswer = true;
			break;
			
		case kAudioStreamPropertyPhysicalFormat:
			theAnswer = true;
			break;
			
		case kAudioStreamPropertyAvailableVirtualFormats:
		case kAudioStreamPropertyAvailablePhysicalFormats:
			theAnswer = false;
			break;
		
		//  obsolete device properties
		case kAudioDevicePropertyStreamFormats:
		case kAudioDevicePropertyStreamFormatSupported:
		case kAudioDevicePropertyStreamFormatMatch:
			theAnswer = false;
			break;
		
		//  obsolete stream properties
		case kAudioStreamPropertyPhysicalFormats:
		case kAudioStreamPropertyPhysicalFormatSupported:
		case kAudioStreamPropertyPhysicalFormatMatch:
			theAnswer = false;
			break;
	};
	
	return theAnswer;
}

UInt32	HP_FormatList::GetPropertyDataSize(const AudioObjectPropertyAddress& inAddress, UInt32 /*inQualifierDataSize*/, const void* /*inQualifierData*/) const
{
	UInt32 theAnswer = 0;
	
	switch(inAddress.mSelector)
	{
		//  device properties
		case kAudioDevicePropertySupportsMixing:
			theAnswer = SizeOf32(UInt32);
			break;
			
		case kAudioDevicePropertyNominalSampleRate:
			theAnswer = SizeOf32(Float64);
			break;
			
		case kAudioDevicePropertyAvailableNominalSampleRates:
			theAnswer = GetNumberAvailableNominalSampleRateRanges() * SizeOf32(AudioValueRange);
			break;
		
		//  stream properties
		case kAudioStreamPropertyVirtualFormat:
			//  aka kAudioDevicePropertyStreamFormat
			theAnswer = SizeOf32(AudioStreamBasicDescription);
			break;
			
		case kAudioStreamPropertyAvailableVirtualFormats:
			theAnswer = GetNumberAvailableVirtualFormats() * SizeOf32(AudioStreamRangedDescription);
			break;
			
		case kAudioStreamPropertyPhysicalFormat:
			theAnswer = SizeOf32(AudioStreamBasicDescription);
			break;
			
		case kAudioStreamPropertyAvailablePhysicalFormats:
			theAnswer = GetNumberAvailablePhysicalFormats() * SizeOf32(AudioStreamRangedDescription);
			break;
			
		//  obsolete device properties
		case kAudioDevicePropertyStreamFormats:
			theAnswer = GetNumberAvailableVirtualFormats() * SizeOf32(AudioStreamBasicDescription);
			break;
			
		case kAudioDevicePropertyStreamFormatSupported:
			theAnswer = SizeOf32(AudioStreamBasicDescription);
			break;
			
		case kAudioDevicePropertyStreamFormatMatch:
			theAnswer = SizeOf32(AudioStreamBasicDescription);
			break;
			
		//  obsolete stream properties
		case kAudioStreamPropertyPhysicalFormats:
			theAnswer = GetNumberAvailablePhysicalFormats() * SizeOf32(AudioStreamBasicDescription);
			break;
			
		case kAudioStreamPropertyPhysicalFormatSupported:
			theAnswer = SizeOf32(AudioStreamBasicDescription);
			break;
			
		case kAudioStreamPropertyPhysicalFormatMatch:
			theAnswer = SizeOf32(AudioStreamBasicDescription);
			break;
	};
	
	return theAnswer;
}

void	HP_FormatList::GetPropertyData(const AudioObjectPropertyAddress& inAddress, UInt32 /*inQualifierDataSize*/, const void* /*inQualifierData*/, UInt32& ioDataSize, void* outData) const
{
	AudioValueRange*				theRanges;
	UInt32							theNumberFormats;
	UInt32							theIndex;
	AudioStreamRangedDescription	theAvailableFormat;
	AudioStreamRangedDescription*	theAvailableFormatPtr = static_cast<AudioStreamRangedDescription*>(outData);
	AudioStreamBasicDescription*	theFormatDataPtr = static_cast<AudioStreamBasicDescription*>(outData);
	
	switch(inAddress.mSelector)
	{
		//  device properties
		case kAudioDevicePropertySupportsMixing:
			ThrowIf(ioDataSize != sizeof(UInt32), CAException(kAudioHardwareBadPropertySizeError), "HP_FormatList::GetPropertyData: wrong data size for kAudioDevicePropertySupportsMixing");
			*(static_cast<UInt32*>(outData)) = IsMixable() ? 1 : 0;
			break;
			
		case kAudioDevicePropertyNominalSampleRate:
			ThrowIf(ioDataSize != sizeof(Float64), CAException(kAudioHardwareBadPropertySizeError), "HP_FormatList::GetPropertyData: wrong data size for kAudioDevicePropertyNominalSampleRate");
			*(static_cast<Float64*>(outData)) = GetCurrentNominalSampleRate();
			break;
			
		case kAudioDevicePropertyAvailableNominalSampleRates:
			theNumberFormats = std::min((UInt32)(ioDataSize / SizeOf32(AudioValueRange)), GetNumberAvailableNominalSampleRateRanges());
			theRanges = static_cast<AudioValueRange*>(outData);
			for(theIndex = 0; theIndex < theNumberFormats; ++theIndex)
			{
				GetAvailableNominalSampleRateRangeByIndex(theIndex, theRanges[theIndex]);
			}
			std::sort(&theRanges[0], &theRanges[theNumberFormats], CAAudioValueRange::LessThan());
			ioDataSize = theNumberFormats * SizeOf32(AudioValueRange);
			break;
		
		//  stream properties
		case kAudioStreamPropertyVirtualFormat:
			//  aka kAudioDevicePropertyStreamFormat
			ThrowIf(ioDataSize != sizeof(AudioStreamBasicDescription), CAException(kAudioHardwareBadPropertySizeError), "HP_FormatList::GetPropertyData: wrong data size for kAudioStreamPropertyVirtualFormat");
			GetCurrentVirtualFormat(*theFormatDataPtr);
			break;
			
		case kAudioStreamPropertyAvailableVirtualFormats:
			theNumberFormats = std::min((UInt32)(ioDataSize / SizeOf32(AudioStreamRangedDescription)), GetNumberAvailableVirtualFormats());
			for(theIndex = 0; theIndex < theNumberFormats; ++theIndex)
			{
				GetAvailableVirtualFormatByIndex(theIndex, theAvailableFormatPtr[theIndex]);
			}
			ioDataSize = theNumberFormats * SizeOf32(AudioStreamRangedDescription);
			break;
			
		case kAudioStreamPropertyPhysicalFormat:
			ThrowIf(ioDataSize != sizeof(AudioStreamBasicDescription), CAException(kAudioHardwareBadPropertySizeError), "HP_FormatList::GetPropertyData: wrong data size for kAudioStreamPropertyPhysicalFormat");
			GetCurrentPhysicalFormat(*theFormatDataPtr);
			break;
			
		case kAudioStreamPropertyAvailablePhysicalFormats:
			theNumberFormats = std::min((UInt32)(ioDataSize / SizeOf32(AudioStreamRangedDescription)), GetNumberAvailablePhysicalFormats());
			for(theIndex = 0; theIndex < theNumberFormats; ++theIndex)
			{
				GetAvailablePhysicalFormatByIndex(theIndex, theAvailableFormatPtr[theIndex]);
			}
			ioDataSize = theNumberFormats * SizeOf32(AudioStreamRangedDescription);
			break;
			
		//  obsolete device properties
		case kAudioDevicePropertyStreamFormats:
			theNumberFormats = std::min((UInt32)(ioDataSize / SizeOf32(AudioStreamBasicDescription)), GetNumberAvailableVirtualFormats());
			for(theIndex = 0; theIndex < theNumberFormats; ++theIndex)
			{
				GetAvailableVirtualFormatByIndex(theIndex, theAvailableFormat);
				theFormatDataPtr[theIndex] = theAvailableFormat.mFormat;
			}
			ioDataSize = theNumberFormats * SizeOf32(AudioStreamBasicDescription);
			break;
			
		case kAudioDevicePropertyStreamFormatSupported:
			ThrowIf(ioDataSize != sizeof(AudioStreamBasicDescription), CAException(kAudioHardwareBadPropertySizeError), "HP_FormatList::GetPropertyData: wrong data size for kAudioDevicePropertyStreamFormatSupported");
			ThrowIf(!SupportsVirtualFormat(*theFormatDataPtr), CAException(kAudioDeviceUnsupportedFormatError), "HP_FormatList::GetPropertyData: kAudioDevicePropertyStreamFormatSupported: format not supported");
			break;
			
		case kAudioDevicePropertyStreamFormatMatch:
			ThrowIf(ioDataSize != sizeof(AudioStreamBasicDescription), CAException(kAudioHardwareBadPropertySizeError), "HP_FormatList::GetPropertyData: wrong data size for kAudioDevicePropertyStreamFormatMatch");
			BestMatchForVirtualFormat(*theFormatDataPtr);
			break;
			
		//  obsolete stream properties
		case kAudioStreamPropertyPhysicalFormats:
			theNumberFormats = std::min((UInt32)(ioDataSize / SizeOf32(AudioStreamBasicDescription)), GetNumberAvailablePhysicalFormats());
			for(theIndex = 0; theIndex < theNumberFormats; ++theIndex)
			{
				GetAvailablePhysicalFormatByIndex(theIndex, theAvailableFormat);
				theFormatDataPtr[theIndex] = theAvailableFormat.mFormat;
			}
			ioDataSize = theNumberFormats * SizeOf32(AudioStreamBasicDescription);
			break;
			
		case kAudioStreamPropertyPhysicalFormatSupported:
			ThrowIf(ioDataSize != sizeof(AudioStreamBasicDescription), CAException(kAudioHardwareBadPropertySizeError), "HP_FormatList::GetPropertyData: wrong data size for kAudioStreamPropertyPhysicalFormatSupported");
			ThrowIf(!SupportsPhysicalFormat(*theFormatDataPtr), CAException(kAudioDeviceUnsupportedFormatError), "HP_FormatList::GetPropertyData: kAudioStreamPropertyPhysicalFormatSupported: format not supported");
			break;
			
		case kAudioStreamPropertyPhysicalFormatMatch:
			ThrowIf(ioDataSize != sizeof(AudioStreamBasicDescription), CAException(kAudioHardwareBadPropertySizeError), "HP_FormatList::GetPropertyData: wrong data size for kAudioStreamPropertyPhysicalFormatMatch");
			BestMatchForPhysicalFormat(*theFormatDataPtr);
			break;
	};
}

void	HP_FormatList::SetPropertyData(const AudioObjectPropertyAddress& inAddress, UInt32 /*inQualifierDataSize*/, const void* /*inQualifierData*/, UInt32 inDataSize, const void* inData, const AudioTimeStamp* /*inWhen*/)
{
	AudioStreamBasicDescription theNewFormat;
	const AudioStreamBasicDescription* theFormatDataPtr = static_cast<const AudioStreamBasicDescription*>(inData);
	
	switch(inAddress.mSelector)
	{
		//  device properties
		case kAudioDevicePropertySupportsMixing:
			ThrowIf(inDataSize != sizeof(UInt32), CAException(kAudioHardwareBadPropertySizeError), "HP_FormatList::SetPropertyData: wrong data size for kAudioDevicePropertySupportsMixing");
			SetIsMixable(*(static_cast<const UInt32*>(inData)) != 0, true);
			break;
		
		case kAudioDevicePropertyNominalSampleRate:
			{
				ThrowIf(inDataSize != sizeof(Float64), CAException(kAudioHardwareBadPropertySizeError), "HP_FormatList::SetPropertyData: wrong data size for kAudioDevicePropertyNominalSampleRate");
				
				//	get the new sample rate
				Float64 theSampleRate = *(static_cast<const Float64*>(inData));
				
				//	screen the sample rate
				ThrowIf((theSampleRate != 0.0) && !SupportsNominalSampleRate(theSampleRate), CAException(kAudioDeviceUnsupportedFormatError), "HP_FormatList::SetPropertyData: given sample rate is not supported for kAudioDevicePropertyNominalSampleRate");
					
				SetCurrentNominalSampleRate(theSampleRate, true);
			}
			break;
			
		//  stream properties
		case kAudioStreamPropertyVirtualFormat:
			//  aka kAudioDevicePropertyStreamFormat
			ThrowIf(inDataSize != sizeof(AudioStreamBasicDescription), CAException(kAudioHardwareBadPropertySizeError), "HP_FormatList::SetPropertyData: wrong data size for kAudioStreamPropertyVirtualFormat");
			
			//	make a mutable copy of the new format
			theNewFormat = *theFormatDataPtr;
			
			//	screen the format
			ThrowIf(!SanityCheckVirtualFormat(theNewFormat), CAException(kAudioDeviceUnsupportedFormatError), "HP_FormatList::SetPropertyData: given format is not supported for kAudioStreamPropertyVirtualFormat");
				
			//	look for a best match to what was asked for
			BestMatchForVirtualFormat(theNewFormat);
			
			//	set the new format
			SetCurrentVirtualFormat(theNewFormat, true);
			break;
			
		case kAudioStreamPropertyPhysicalFormat:
			ThrowIf(inDataSize != sizeof(AudioStreamBasicDescription), CAException(kAudioHardwareBadPropertySizeError), "HP_FormatList::SetPropertyData: wrong data size for kAudioStreamPropertyPhysicalFormat");
			
			//	make a mutable copy of the new format
			theNewFormat = *theFormatDataPtr;
			
			//	screen the format
			ThrowIf(!SanityCheckPhysicalFormat(theNewFormat), CAException(kAudioDeviceUnsupportedFormatError), "HP_FormatList::SetPropertyData: given format is not supported for kAudioStreamPropertyPhysicalFormat");
				
			//	look for a best match to what was asked for
			BestMatchForPhysicalFormat(theNewFormat);
			
			//	set the new format
			SetCurrentPhysicalFormat(theNewFormat, true);
			break;
	};
}

UInt32	HP_FormatList::GetNumberAddressesImplemented() const
{
	return 13;
}

void	HP_FormatList::GetImplementedAddressByIndex(UInt32 inIndex, AudioObjectPropertyAddress& outAddress) const
{
	switch(inIndex)
	{
		//  device properties
		case 0:
			outAddress.mSelector = kAudioDevicePropertyNominalSampleRate;
			break;
			
		case 1:
			outAddress.mSelector = kAudioDevicePropertyAvailableNominalSampleRates;
			break;
		
		//  stream properties
		case 2:
			outAddress.mSelector = kAudioStreamPropertyVirtualFormat;
			break;
			
		case 3:
			outAddress.mSelector = kAudioStreamPropertyAvailableVirtualFormats;
			break;
			
		case 4:
			outAddress.mSelector = kAudioStreamPropertyPhysicalFormat;
			break;
			
		case 5:
			outAddress.mSelector = kAudioStreamPropertyAvailablePhysicalFormats;
			break;
			
		//  obsolete device properties
		case 6:
			outAddress.mSelector = kAudioDevicePropertyStreamFormats;
			break;
			
		case 7:
			outAddress.mSelector = kAudioDevicePropertyStreamFormatSupported;
			break;
			
		case 8:
			outAddress.mSelector = kAudioDevicePropertyStreamFormatMatch;
			break;
			
		//  obsolete stream properties
		case 9:
			outAddress.mSelector = kAudioStreamPropertyPhysicalFormats;
			break;
			
		case 10:
			outAddress.mSelector = kAudioStreamPropertyPhysicalFormatSupported;
			break;
			
		case 11:
			outAddress.mSelector = kAudioStreamPropertyPhysicalFormatMatch;
			break;
		
		case 12:
			outAddress.mSelector = kAudioDevicePropertySupportsMixing;
			break;
	};
	outAddress.mScope = kAudioObjectPropertyScopeWildcard;
	outAddress.mElement = kAudioObjectPropertyElementWildcard;
}

void	HP_FormatList::DetermineNotifications(const HP_Device& inDevice, const HP_Stream& inStream, const AudioStreamBasicDescription& inOldPhysicalFormat, const AudioStreamBasicDescription& inNewPhysicalFormat, CAPropertyAddressList& outDeviceNotifications, CAPropertyAddressList& outStreamNotifications)
{
	if(inOldPhysicalFormat != inNewPhysicalFormat)
	{
		//  figure out the corresponding old virtual format
		AudioStreamBasicDescription theOldVirtualFormat = inOldPhysicalFormat;
#if !HAL_Embedded_Build
		CAStreamBasicDescription::NormalizeLinearPCMFormat(theOldVirtualFormat);
#endif
		
		//  figure out the corresponding old physical sample format
		AudioStreamBasicDescription theOldPhysicalSampleFormat = inOldPhysicalFormat;
		theOldPhysicalSampleFormat.mSampleRate = 0;
		
		//  figure out the corresponding new virtual format
		AudioStreamBasicDescription theNewVirtualFormat = inNewPhysicalFormat;
#if !HAL_Embedded_Build
		CAStreamBasicDescription::NormalizeLinearPCMFormat(theNewVirtualFormat);
#endif
		
		//  figure out the corresponding new physical sample format
		AudioStreamBasicDescription theNewPhysicalSampleFormat = inNewPhysicalFormat;
		theNewPhysicalSampleFormat.mSampleRate = 0;
		
		//  figure out what aspects of the format have changed
		bool thePhysicalFormatChanged = true;
		bool thePhysicalSampleFormatChanged = theOldPhysicalSampleFormat != theNewPhysicalSampleFormat;
		bool theVirtualFormatChanged = theOldVirtualFormat != theNewVirtualFormat;
		bool theSampleRateChanged = inOldPhysicalFormat.mSampleRate != inNewPhysicalFormat.mSampleRate;
		bool theNumberChannelsChanged = inOldPhysicalFormat.mChannelsPerFrame != inNewPhysicalFormat.mChannelsPerFrame;
		bool theMixabilityChanged = CAStreamBasicDescription::IsMixable(inOldPhysicalFormat) != CAStreamBasicDescription::IsMixable(inNewPhysicalFormat);
		
		//  figure out the addresses of the various properties that change
		AudioObjectPropertyAddress theAddress;

		//	for backward compatibility, we also have to send device notifications for the master element if the stream is the first
		bool isFirstStream = inStream.GetStartingDeviceChannelNumber() == 1;
		
		if(thePhysicalFormatChanged)
		{
			//  When the physical format changes, the following properties change:
			//		kAudioStreamPropertyPhysicalFormat is a stream property that is reflected in the
			//		stream's owning device as a property of that stream's starting device channel number.
			
			//  the device notifications
			theAddress.mSelector = kAudioStreamPropertyPhysicalFormat;
			theAddress.mScope = inStream.GetDevicePropertyScope();
			theAddress.mElement = inStream.GetStartingDeviceChannelNumber();
			outDeviceNotifications.AppendUniqueItem(theAddress);
			
			if(isFirstStream)
			{
				theAddress.mElement = kAudioObjectPropertyElementMaster;
				outDeviceNotifications.AppendUniqueItem(theAddress);
			}
			
			//  the stream notifications
			theAddress.mSelector = kAudioStreamPropertyPhysicalFormat;
			theAddress.mScope = kAudioObjectPropertyScopeGlobal;
			theAddress.mElement = kAudioObjectPropertyElementMaster;
			outStreamNotifications.AppendUniqueItem(theAddress);
		}
		
		if(thePhysicalSampleFormatChanged)
		{
			//  The physical sample format is the format with the sample rate factored out. If it changes
			//  it means that there are a new set of sample rate ranges and the following properties change:
			//		kAudioDevicePropertyAvailableNominalSampleRates is a device global property that gets
			//		reflected in the input and output scopes, presuming the device currently has those sections.
			
			//  the device notifications
			theAddress.mSelector = kAudioDevicePropertyAvailableNominalSampleRates;
			theAddress.mScope = kAudioObjectPropertyScopeGlobal;
			theAddress.mElement = kAudioObjectPropertyElementMaster;
			outDeviceNotifications.AppendUniqueItem(theAddress);
		}
		
		if(theVirtualFormatChanged)
		{
			//  When the virtual format changes, the following properties change:
			//		kAudioStreamPropertyVirtualFormat is a stream property that is reflected in the
			//		stream's owning device as a property of that stream's starting device channel number.
			
			//  the device notifications
			theAddress.mSelector = kAudioStreamPropertyVirtualFormat;
			theAddress.mScope = inStream.GetDevicePropertyScope();
			theAddress.mElement = inStream.GetStartingDeviceChannelNumber();
			outDeviceNotifications.AppendUniqueItem(theAddress);
			
			if(isFirstStream)
			{
				theAddress.mElement = kAudioObjectPropertyElementMaster;
				outDeviceNotifications.AppendUniqueItem(theAddress);
			}
			
			//  the stream notifications
			theAddress.mSelector = kAudioStreamPropertyVirtualFormat;
			theAddress.mScope = kAudioObjectPropertyScopeGlobal;
			theAddress.mElement = kAudioObjectPropertyElementMaster;
			outStreamNotifications.AppendUniqueItem(theAddress);
		}
		
		if(theSampleRateChanged)
		{
			//  When the sample rate changes, the following properties change:
			//		kAudioDevicePropertyNominalSampleRate is a device global property that gets refelcted
			//		in the input and output scopes, presuming the device currently has those sections.
			//		kAudioDevicePropertyLatency is an input or output property.
			//		kAudioDevicePropertySafetyOffset is an input or output property.
			//		kAudioStreamPropertyPhysicalFormat and kAudioStreamPropertyVirtualFormat on each stream
			
			//  the device notifications
			theAddress.mSelector = kAudioDevicePropertyNominalSampleRate;
			theAddress.mScope = kAudioObjectPropertyScopeGlobal;
			theAddress.mElement = kAudioObjectPropertyElementMaster;
			outDeviceNotifications.AppendUniqueItem(theAddress);
			
			UInt32 theNumberStreams;
			UInt32 theStreamIndex;
			HP_Stream* theStream;
			if(inDevice.HasAnyStreams(inStream.IsInput()))
			{
				theAddress.mSelector = kAudioDevicePropertyLatency;
				theAddress.mScope = kAudioDevicePropertyScopeInput;
				theAddress.mElement = kAudioObjectPropertyElementMaster;
				outDeviceNotifications.AppendUniqueItem(theAddress);
				
				theAddress.mSelector = kAudioDevicePropertySafetyOffset;
				theAddress.mScope = kAudioDevicePropertyScopeInput;
				theAddress.mElement = kAudioObjectPropertyElementMaster;
				outDeviceNotifications.AppendUniqueItem(theAddress);
				
				theNumberStreams = inDevice.GetNumberStreams(inStream.IsInput());
				for(theStreamIndex = 0; theStreamIndex < theNumberStreams; ++theStreamIndex)
				{
					theStream = inDevice.GetStreamByIndex(inStream.IsInput(), theStreamIndex);
					
					theAddress.mSelector = kAudioStreamPropertyPhysicalFormat;
					theAddress.mScope = theStream->GetDevicePropertyScope();
					theAddress.mElement = theStream->GetStartingDeviceChannelNumber();
					outDeviceNotifications.AppendUniqueItem(theAddress);
					
					if(theAddress.mElement == 1)
					{
						theAddress.mElement = 0;
						outDeviceNotifications.AppendUniqueItem(theAddress);
					}
					
					theAddress.mSelector = kAudioStreamPropertyVirtualFormat;
					theAddress.mScope = theStream->GetDevicePropertyScope();
					theAddress.mElement = theStream->GetStartingDeviceChannelNumber();
					outDeviceNotifications.AppendUniqueItem(theAddress);
					
					if(theAddress.mElement == 1)
					{
						theAddress.mElement = 0;
						outDeviceNotifications.AppendUniqueItem(theAddress);
					}
				}
			}
			
			if(inDevice.HasAnyStreams(!inStream.IsInput()))
			{
				theAddress.mSelector = kAudioDevicePropertyLatency;
				theAddress.mScope = kAudioDevicePropertyScopeOutput;
				theAddress.mElement = kAudioObjectPropertyElementMaster;
				outDeviceNotifications.AppendUniqueItem(theAddress);
				
				theAddress.mSelector = kAudioDevicePropertySafetyOffset;
				theAddress.mScope = kAudioDevicePropertyScopeOutput;
				theAddress.mElement = kAudioObjectPropertyElementMaster;
				outDeviceNotifications.AppendUniqueItem(theAddress);
				
				theNumberStreams = inDevice.GetNumberStreams(!inStream.IsInput());
				for(theStreamIndex = 0; theStreamIndex < theNumberStreams; ++theStreamIndex)
				{
					theStream = inDevice.GetStreamByIndex(!inStream.IsInput(), theStreamIndex);
					
					theAddress.mSelector = kAudioStreamPropertyPhysicalFormat;
					theAddress.mScope = theStream->GetDevicePropertyScope();
					theAddress.mElement = theStream->GetStartingDeviceChannelNumber();
					outDeviceNotifications.AppendUniqueItem(theAddress);
					
					if(theAddress.mElement == 1)
					{
						theAddress.mElement = 0;
						outDeviceNotifications.AppendUniqueItem(theAddress);
					}
					
					theAddress.mSelector = kAudioStreamPropertyVirtualFormat;
					theAddress.mScope = theStream->GetDevicePropertyScope();
					theAddress.mElement = theStream->GetStartingDeviceChannelNumber();
					outDeviceNotifications.AppendUniqueItem(theAddress);
					
					if(theAddress.mElement == 1)
					{
						theAddress.mElement = 0;
						outDeviceNotifications.AppendUniqueItem(theAddress);
					}
				}
			}
			
			//  the stream notifications
			theAddress.mSelector = kAudioDevicePropertyNominalSampleRate;
			theAddress.mScope = kAudioObjectPropertyScopeGlobal;
			theAddress.mElement = kAudioObjectPropertyElementMaster;
			outStreamNotifications.AppendUniqueItem(theAddress);
				
			theAddress.mSelector = kAudioDevicePropertyLatency;
			theAddress.mScope = kAudioObjectPropertyScopeGlobal;
			theAddress.mElement = kAudioObjectPropertyElementMaster;
			outStreamNotifications.AppendUniqueItem(theAddress);
		}
		
		if(theNumberChannelsChanged)
		{
			//  When the number of channels changes, it means that the starting channel number for
			//  streams after the given stream will be renumbered, but that is tracked elsewhere.
			//  Here we just concern ourselves about the following properties:
			//		kAudioDevicePropertyStreamConfiguration is a device property that tracks the
			//		channel distribution among streams in a given direction.
			
			//  the device notifications
			theAddress.mSelector = kAudioDevicePropertyStreamConfiguration;
			theAddress.mScope = inStream.GetDevicePropertyScope();
			theAddress.mElement = kAudioObjectPropertyElementMaster;
			outDeviceNotifications.AppendUniqueItem(theAddress);
		}
		
		if(theMixabilityChanged)
		{
			//	When the mixability changes, it means the following properties change:
			//		kAudioDevicePropertySupportsMixing is a stream property that only gets reflected
			//		in the global scope, master element on the device

			//  the device notifications
			theAddress.mSelector = kAudioDevicePropertySupportsMixing;
			theAddress.mScope = kAudioObjectPropertyScopeGlobal;
			theAddress.mElement = kAudioObjectPropertyElementMaster;
			outDeviceNotifications.AppendUniqueItem(theAddress);
			
			theAddress.mSelector = kAudioDevicePropertySupportsMixing;
			theAddress.mScope = kAudioObjectPropertyScopeGlobal;
			theAddress.mElement = kAudioObjectPropertyElementMaster;
			outStreamNotifications.AppendUniqueItem(theAddress);
		}
	}
}

UInt32  HP_FormatList::AFL_GetNumberFormats(const HP_FormatList::AvailableFormatList& inAvailableFormatList)
{
	UInt32 theAnswer = 0;
	
	//  go through the available format list
	AvailableFormatList::const_iterator theAvailableFormatIterator = inAvailableFormatList.begin();
	while(theAvailableFormatIterator != inAvailableFormatList.end())
	{
		//  count all the ranges for the current sample format (there is one available format per range)
		theAnswer += ToUInt32(theAvailableFormatIterator->mSampleRateRanges.size());
		
		//  go to the next one
		std::advance(theAvailableFormatIterator, 1);
	}
	
	return theAnswer;
}

void	HP_FormatList::AFL_GetFormatByIndex(const HP_FormatList::AvailableFormatList& inAvailableFormatList, UInt32 inIndex, AudioStreamRangedDescription& outFormat)
{
	//  the items in the available format list are enumerated by iterating through the items in the list
	bool wasFound = false;
	UInt32 theCurrentIndex = 0;
	AvailableFormatList::const_iterator theAvailableFormatIterator = inAvailableFormatList.begin();
	while(!wasFound && (theAvailableFormatIterator != inAvailableFormatList.end()))
	{
		//  get the number of sample rate ranges in the current format
		UInt32 theNumberSampleRateRanges = ToUInt32(theAvailableFormatIterator->mSampleRateRanges.size());
		
		//  see if the requested index falls into the current range of indexes
		if((inIndex >= theCurrentIndex) && (inIndex < (theCurrentIndex + theNumberSampleRateRanges)))
		{
			//  it does, so this is it
			wasFound = true;
			
			//  copy the sample format
			outFormat.mFormat = theAvailableFormatIterator->mSampleFormat;
			
			//  copy the sample rate range
			outFormat.mSampleRateRange = theAvailableFormatIterator->mSampleRateRanges.at(inIndex - theCurrentIndex);
			
			//  make sure the sample rate is set right in the sample format
			if(outFormat.mSampleRateRange.mMinimum == outFormat.mSampleRateRange.mMaximum)
			{
				outFormat.mFormat.mSampleRate = outFormat.mSampleRateRange.mMinimum;
			}
		}
		else
		{
			//  it doesn't, so go to the next item
			theCurrentIndex += theNumberSampleRateRanges;
			std::advance(theAvailableFormatIterator, 1);
		}
	}
}

bool	HP_FormatList::AFL_FindSampleFormat(const HP_FormatList::AvailableFormatList& inAvailableFormatList, const AudioStreamBasicDescription& inFormat, HP_FormatList::AvailableFormatList::const_iterator& outIterator)
{
	bool wasSampleFormatFound = false;
	AvailableFormatList::const_iterator theAvailableFormatIterator = inAvailableFormatList.begin();
	while(!wasSampleFormatFound && (theAvailableFormatIterator != inAvailableFormatList.end()))
	{
		if(inFormat == theAvailableFormatIterator->mSampleFormat)
		{
			//  this is it
			wasSampleFormatFound = true;
			outIterator = theAvailableFormatIterator;
		}
		else
		{
			//  this isn't it, so keep looking
			std::advance(theAvailableFormatIterator, 1);
		}
	}
	return wasSampleFormatFound;
}

bool	HP_FormatList::AFL_SupportsFormat(const HP_FormatList::AvailableFormatList& inAvailableFormatList, const AudioStreamBasicDescription& inFormat)
{
	//  initialize the return value
	bool theAnswer = false;
	
	//  declare some commonly used variables
	bool wasSampleFormatFound;
	bool wasSampleRateFound;
	AvailableFormatList::const_iterator theAvailableFormatIterator;
	HP_SampleRateRangeList::const_iterator theSampleRateRangeIterator;
	
	//  break the sample rate away from the sample format
	AudioStreamBasicDescription theSampleFormat = inFormat;
	theSampleFormat.mSampleRate = 0.0;
	Float64 theSampleRate = inFormat.mSampleRate;
	
	//  look for the sample format in the available format list, if there is anything in the sample format
	if(memcmp(&theSampleFormat, &CAStreamBasicDescription::sEmpty, sizeof(AudioStreamBasicDescription) - sizeof(UInt32)) != 0)
	{
		wasSampleFormatFound = false;
		theAvailableFormatIterator = inAvailableFormatList.begin();
		while(!wasSampleFormatFound && (theAvailableFormatIterator != inAvailableFormatList.end()))
		{
			if(theSampleFormat == theAvailableFormatIterator->mSampleFormat)
			{
				//  this is it
				wasSampleFormatFound = true;
				
				//  look to see if the sample rate is supported
				if(theSampleRate != 0.0)
				{
					wasSampleRateFound = false;
					theSampleRateRangeIterator = theAvailableFormatIterator->mSampleRateRanges.begin();
					while(!wasSampleRateFound && (theSampleRateRangeIterator != theAvailableFormatIterator->mSampleRateRanges.end()))
					{
						if(CAAudioValueRange::ContainsValue(*theSampleRateRangeIterator, theSampleRate))
						{
							//  the rate is in this range
							wasSampleRateFound = true;
							theAnswer = true;
						}
						else
						{
							//  go to the next range
							std::advance(theSampleRateRangeIterator, 1);
						}
					}
				}
				else
				{
					//  0 means any rate, which is always supported
					theAnswer = true;
				}
			}
			else
			{
				//  this isn't it, so keep looking
				std::advance(theAvailableFormatIterator, 1);
			}
		}
	}
	else if(theSampleRate != 0.0)
	{
		//  the query is only about the sample rate, so look for a format somewhere that suppports it
		wasSampleFormatFound = false;
		theAvailableFormatIterator = inAvailableFormatList.begin();
		while(!wasSampleFormatFound && (theAvailableFormatIterator != inAvailableFormatList.end()))
		{
			//  look to see if the sample rate is supported
			wasSampleRateFound = false;
			theSampleRateRangeIterator = theAvailableFormatIterator->mSampleRateRanges.begin();
			while(!wasSampleRateFound && (theSampleRateRangeIterator != theAvailableFormatIterator->mSampleRateRanges.end()))
			{
				if(CAAudioValueRange::ContainsValue(*theSampleRateRangeIterator, theSampleRate))
				{
					//  the rate is in this range
					wasSampleRateFound = true;
					theAnswer = true;
				}
				else
				{
					//  go to the next range
					std::advance(theSampleRateRangeIterator, 1);
				}
			}
			
			//  go to the next format
			std::advance(theAvailableFormatIterator, 1);
		}
	}
	else
	{
		//  the query is about a totally empty ASBD, which matches everything
		theAnswer = true;
	}
	
	return theAnswer;
}

void	HP_FormatList::AFL_BestMatchForFormat(const HP_FormatList::AvailableFormatList& inAvailableFormatList, const AudioStreamBasicDescription& inCurrentFormat, AudioStreamBasicDescription& ioFormat)
{
	//  break the sample rate away from the sample format
	AudioStreamBasicDescription theSampleFormat = ioFormat;
	theSampleFormat.mSampleRate = 0.0;
	Float64 theSampleRate = ioFormat.mSampleRate;
	
	//  look for the sample format in the available format list
	bool wasSampleFormatFound = false;
	AvailableFormatList::const_iterator theAvailableFormatIterator = inAvailableFormatList.begin();
	while(!wasSampleFormatFound && (theAvailableFormatIterator != inAvailableFormatList.end()))
	{
		if(theSampleFormat == theAvailableFormatIterator->mSampleFormat)
		{
			//  this is it
			wasSampleFormatFound = true;
			
			//	first account for a sample rate of 0, which means "use the current sample rate if it's available, otherwise use the best"
			if(theSampleRate == 0.0)
			{
				theSampleRate = inCurrentFormat.mSampleRate;
			}
			
			bool wasSampleRateFound = false;
			HP_SampleRateRangeList::const_iterator theSampleRateRangeIterator = theAvailableFormatIterator->mSampleRateRanges.begin();
			while(!wasSampleRateFound && (theSampleRateRangeIterator != theAvailableFormatIterator->mSampleRateRanges.end()))
			{
				if(CAAudioValueRange::ContainsValue(*theSampleRateRangeIterator, theSampleRate))
				{
					//  the rate is in this range
					wasSampleRateFound = true;
					
					//  set the return values
					ioFormat = theAvailableFormatIterator->mSampleFormat;
					ioFormat.mSampleRate = theSampleRate;
				}
				else
				{
					//  go to the next range
					std::advance(theSampleRateRangeIterator, 1);
				}
			}
			
			if(!wasSampleRateFound)
			{
				//  the sample formats match, but the sample rates don't, so pick a rate
				ioFormat = theAvailableFormatIterator->mSampleFormat;
				ioFormat.mSampleRate = CAAudioValueRange::PickCommonSampleRate(theAvailableFormatIterator->mSampleRateRanges.front());
			}
		}
		else
		{
			//  this isn't it, so keep looking
			std::advance(theAvailableFormatIterator, 1);
		}
	}
	
	if(!wasSampleFormatFound)
	{
		//  nothing matched, so pick something
		ioFormat = inAvailableFormatList.front().mSampleFormat;
		ioFormat.mSampleRate = CAAudioValueRange::PickCommonSampleRate(inAvailableFormatList.front().mSampleRateRanges.front());
	}
}

void	HP_FormatList::AFL_AddFormat(HP_FormatList::AvailableFormatList& ioAvailableFormatList, const AudioStreamRangedDescription& inFormat)
{
	//  This routine assumes that inFormat has mSampleRate set to 0 if the range of sample rates
	//  contains more than just one rate. It also assumes that if mSampleRate is not 0, then the
	//  range of samples rates is set to have the minimum and the maximum equal to the sample rate.
	
	//  formats are stored by divorcing the sample rate from the sample format
	AudioStreamBasicDescription theSampleFormat = inFormat.mFormat;
	theSampleFormat.mSampleRate = 0.0;
	AudioValueRange theSampleRateRange = inFormat.mSampleRateRange;
	
	//  look in the list of available formats for a sample format that is the same or the first
	//  format for which operator < return false.
	bool wasSampleFormatFound = false;
	AvailableFormatList::iterator theAvailableFormatIterator = ioAvailableFormatList.begin();
	while(!wasSampleFormatFound && (theAvailableFormatIterator != ioAvailableFormatList.end()))
	{
		if(theAvailableFormatIterator->mSampleFormat < theSampleFormat)
		{
			//  this means the current availabe format is better than the new format, so keep looking
			std::advance(theAvailableFormatIterator, 1);
		}
		else if(theAvailableFormatIterator->mSampleFormat == theSampleFormat)
		{
			//  this means that there is already an entry in the list for this sample format, so
			//  we need to extend it's sample rate range list with the new rate range
			wasSampleFormatFound = true;
			
			//  iterate through the rate ranges until we find one that isn't strictly less than the new one
			bool wasSampleRateRangeFound = false;
			HP_SampleRateRangeList::iterator theSampleRateRangeIterator = theAvailableFormatIterator->mSampleRateRanges.begin();
			while(!wasSampleRateRangeFound && (theSampleRateRangeIterator != theAvailableFormatIterator->mSampleRateRanges.end()))
			{
				if(CAAudioValueRange::IsStrictlyLessThan(theSampleRateRange, *theSampleRateRangeIterator))
				{
					//  the new range is entirely disjoint and before the current range, so go to the next range
					std::advance(theSampleRateRangeIterator, 1);
				}
				else if(CAAudioValueRange::OverlapsLow(theSampleRateRange, *theSampleRateRangeIterator))
				{
					//  the new range's intersects the current range, but the new range's minimum is smaller than the current
					//  range's minimum, which is kind of wierd, but we'll adjust the current minimum to reflect it and be done
					wasSampleRateRangeFound = true;
					theSampleRateRangeIterator->mMinimum = theSampleRateRange.mMinimum;
				}
				else if(CAAudioValueRange::IsStrictlyContainedBy(theSampleRateRange, *theSampleRateRangeIterator))
				{
					//  the new range is entirely contained by the current range, so we can ignore the new one
					wasSampleRateRangeFound = true;
				}
				else if(CAAudioValueRange::OverlapsHigh(theSampleRateRange, *theSampleRateRangeIterator))
				{
					//  the new range's intersects the current range, but the new range's maximum is larger than the current
					//  range's maximum which is kind of wierd, but we'll adjust the current maximum to reflect it and be done
					wasSampleRateRangeFound = true;
					theSampleRateRangeIterator->mMaximum = theSampleRateRange.mMaximum;
				}
				else
				{
					//  the new range is entirely disjoint and after the current range, so insert it before the current one
					wasSampleRateRangeFound = true;
					theAvailableFormatIterator->mSampleRateRanges.insert(theSampleRateRangeIterator, theSampleRateRange);
				}
			}
			
			if(!wasSampleRateRangeFound)
			{
				//  the new sample rate did get placed, which means it's disjoint and larger than any of the current ones
				//  so put it at the end of the list
				theAvailableFormatIterator->mSampleRateRanges.push_back(theSampleRateRange);
			}
		}
		else
		{
			//  the sample format isn't in the list, so make a new entry in front of the current iterator
			wasSampleFormatFound = true;
			ioAvailableFormatList.insert(theAvailableFormatIterator, AvailableFormat(theSampleFormat, theSampleRateRange));
		}
	}
	
	if(!wasSampleFormatFound)
	{
		//  the sample format isn't in the list, so make a new entry at the end of the list
		ioAvailableFormatList.push_back(AvailableFormat(theSampleFormat, theSampleRateRange));
	}
}

bool	HP_FormatList::ConvertVirtualFormatToPhysicalFormat(AudioStreamBasicDescription& ioFormat) const
{
	//  this method returns whether or not something reasonable can be found for the given format
	bool theAnswer = false;
	
	//  the only thing that needs to change is to take things in the canonical format and find a matching physical format
	if((ioFormat.mFormatID == kAudioFormatLinearPCM) && ((ioFormat.mFormatFlags & kAudioFormatFlagsNativeFloatPacked) == kAudioFormatFlagsNativeFloatPacked) && (ioFormat.mBitsPerChannel = (8 * SizeOf32(Float32))))
	{
		//  the only thing that can really change is sample rate and the number of channels,
		//  so set up a physical format with all wildcards
		AudioStreamBasicDescription thePhysicalFormat = ioFormat;
		thePhysicalFormat.mFormatFlags = 0;
		thePhysicalFormat.mBytesPerPacket = 0;
		thePhysicalFormat.mFramesPerPacket = 1;
		thePhysicalFormat.mBytesPerFrame = 0;
		thePhysicalFormat.mBitsPerChannel = 0;
		
		//  try to find a match for the format in the physical format list
		AFL_BestMatchForFormat(mAvailablePhysicalFormatList, mCurrentPhysicalFormat, thePhysicalFormat);
		
		//  make sure we got something that isn't too far from what was requested
		if((thePhysicalFormat.mFormatID == ioFormat.mFormatID) && (thePhysicalFormat.mSampleRate == ioFormat.mSampleRate) && (thePhysicalFormat.mChannelsPerFrame == ioFormat.mChannelsPerFrame))
		{
			theAnswer = true;
			ioFormat = thePhysicalFormat;
		}
	}
	else
	{
		//  all other formats get passed through untouched
		theAnswer = true;
	}
	
	return theAnswer;
}

//==================================================================================================
//	HP_DeviceFormatList
//==================================================================================================

HP_DeviceFormatList::HP_DeviceFormatList(HP_Device* inOwningDevice)
:
	HP_Property(),
	mOwningDevice(inOwningDevice),
	mUseIntersectionForMasterRates(false)
{
}

HP_DeviceFormatList::~HP_DeviceFormatList()
{
}

bool	HP_DeviceFormatList::IsActive(const AudioObjectPropertyAddress& inAddress) const
{
	bool theAnswer = false;
	switch(inAddress.mSelector)
	{
		//  device properties
		case kAudioDevicePropertySupportsMixing:
		case kAudioDevicePropertyNominalSampleRate:
		case kAudioDevicePropertyAvailableNominalSampleRates:
			theAnswer = true;
			break;
		
		//	everything else is a stream property
		default:
			switch(inAddress.mScope)
			{
				case kAudioDevicePropertyScopeInput:
					theAnswer = mOwningDevice->HasAnyStreams(true);
					break;
					
				case kAudioDevicePropertyScopeOutput:
					theAnswer = mOwningDevice->HasAnyStreams(false);
					break;
			};
			break;
		
	};
	
	return theAnswer;
}

bool	HP_DeviceFormatList::IsPropertySettable(const AudioObjectPropertyAddress& inAddress) const
{
	bool theAnswer = false;
	
	//	find the stream that goes with the given address
	AudioObjectPropertyAddress theStreamAddress = inAddress;
	HP_Stream* theStream = mOwningDevice->GetStreamByPropertyAddress(inAddress, false);
	
	//	filter out properties that are different for devices
	switch(inAddress.mSelector)
	{
		case kAudioDevicePropertySupportsMixing:
			if(inAddress.mElement == 0)
			{
				theAnswer = mOwningDevice->CanSetIsMixable(true) || mOwningDevice->CanSetIsMixable(false);
			}
			else
			{
				ThrowIfNULL(theStream, CAException(kAudioHardwareUnknownPropertyError), "HP_DeviceFormatList::IsPropertySettable: no stream for given scope and element");
				theStream->ChangeDevicePropertyAddressIntoStreamPropertyAddress(theStreamAddress);
				theAnswer = theStream->IsPropertySettable(theStreamAddress);
			}
			break;
		
		case kAudioDevicePropertyNominalSampleRate:
			theAnswer = true;
			break;
			
		case kAudioDevicePropertyAvailableNominalSampleRates:
			theAnswer = false;
			break;
			
		default:
			//	let the stream handle it
			ThrowIfNULL(theStream, CAException(kAudioHardwareUnknownPropertyError), "HP_DeviceFormatList::IsPropertySettable: no stream for given scope and element");
			theStream->ChangeDevicePropertyAddressIntoStreamPropertyAddress(theStreamAddress);
			theAnswer = theStream->IsPropertySettable(theStreamAddress);
			break;
	};
	
	return theAnswer;
}

UInt32	HP_DeviceFormatList::GetPropertyDataSize(const AudioObjectPropertyAddress& inAddress, UInt32 inQualifierDataSize, const void* inQualifierData) const
{
	UInt32 theAnswer = 0;
	
	//	find the stream that goes with the given address
	AudioObjectPropertyAddress theStreamAddress = inAddress;
	HP_Stream* theStream = mOwningDevice->GetStreamByPropertyAddress(inAddress, false);
	
	//	filter out properties that are different for devices
	switch(inAddress.mSelector)
	{
		case kAudioDevicePropertySupportsMixing:
			theAnswer = SizeOf32(UInt32);
			break;
		
		case kAudioDevicePropertyNominalSampleRate:
			theAnswer = SizeOf32(Float64);
			break;
			
		case kAudioDevicePropertyAvailableNominalSampleRates:
			if(inAddress.mElement == 0)
			{
				theAnswer = GetNumberAvailableMasterNominalSampleRateRanges() * SizeOf32(AudioValueRange);
			}
			else
			{
				ThrowIfNULL(theStream, CAException(kAudioHardwareUnknownPropertyError), "HP_DeviceFormatList::GetPropertyDataSize: no stream for given scope and element");
				theStream->ChangeDevicePropertyAddressIntoStreamPropertyAddress(theStreamAddress);
				theAnswer = theStream->GetPropertyDataSize(theStreamAddress, inQualifierDataSize, inQualifierData);
			}
			break;
		
		default:
			//	let the stream handle it
			ThrowIfNULL(theStream, CAException(kAudioHardwareUnknownPropertyError), "HP_DeviceFormatList::GetPropertyDataSize: no stream for given scope and element");
			theStream->ChangeDevicePropertyAddressIntoStreamPropertyAddress(theStreamAddress);
			theAnswer = theStream->GetPropertyDataSize(theStreamAddress, inQualifierDataSize, inQualifierData);
			break;
	};
	
	return theAnswer;
}

void	HP_DeviceFormatList::GetPropertyData(const AudioObjectPropertyAddress& inAddress, UInt32 inQualifierDataSize, const void* inQualifierData, UInt32& ioDataSize, void* outData) const
{
	//	find the stream that goes with the given address
	AudioValueRange*	theRanges;
	UInt32				theNumberItems;
	UInt32				theIndex;
	AudioObjectPropertyAddress theStreamAddress = inAddress;
	HP_Stream*			theStream = mOwningDevice->GetStreamByPropertyAddress(inAddress, false);
	
	//	filter out properties that are different for devices
	switch(inAddress.mSelector)
	{
		case kAudioDevicePropertySupportsMixing:
			if(inAddress.mElement == 0)
			{
				ThrowIf(ioDataSize != sizeof(UInt32), CAException(kAudioHardwareBadPropertySizeError), "HP_DeviceFormatList::GetPropertyData: wrong data size for kAudioDevicePropertySupportsMixing");
				
				//	this value is 0 only if all the streams are mixable
				bool isMixable = true;
				for(UInt32 theSectionIndex = 0; isMixable && (theSectionIndex < 2); ++theSectionIndex)
				{
					bool theSection = theSectionIndex == 0;
					theNumberItems = mOwningDevice->GetNumberStreams(theSection);
					for(theIndex = 0; isMixable && (theIndex < theNumberItems); ++theIndex)
					{
						theStream = mOwningDevice->GetStreamByIndex(theSection, theIndex);
						isMixable = theStream->IsMixable();
					}
				}
				
				*(static_cast<UInt32*>(outData)) = isMixable ? 1 : 0;
			}
			else
			{
				ThrowIfNULL(theStream, CAException(kAudioHardwareUnknownPropertyError), "HP_DeviceFormatList::GetPropertyData: no stream for given scope and element");
				theStream->ChangeDevicePropertyAddressIntoStreamPropertyAddress(theStreamAddress);
				theStream->GetPropertyData(theStreamAddress, inQualifierDataSize, inQualifierData, ioDataSize, outData);
			}
			break;
		
		case kAudioDevicePropertyNominalSampleRate:
			ThrowIf(ioDataSize != sizeof(Float64), CAException(kAudioHardwareBadPropertySizeError), "HP_DeviceFormatList::GetPropertyData: wrong data size for kAudioDevicePropertyNominalSampleRate");
			*(static_cast<Float64*>(outData)) = mOwningDevice->GetCurrentNominalSampleRate();
			break;
						
		case kAudioDevicePropertyAvailableNominalSampleRates:
			if(inAddress.mElement == 0)
			{
				//	get the available sample rates
				HP_SampleRateRangeList theMasterRanges;
				GatherAvailableNominalSampleRateRanges(theMasterRanges);
				
				//	stuff them into the return value
				theNumberItems = std::min(ioDataSize / SizeOf32(AudioValueRange), ToUInt32(theMasterRanges.size()));
				theRanges = static_cast<AudioValueRange*>(outData);
				for(theIndex = 0; theIndex < theNumberItems; ++theIndex)
				{
					theRanges[theIndex] = theMasterRanges.at(theIndex);
				}
				
				//	sort the list
				std::sort(&theRanges[0], &theRanges[theNumberItems], CAAudioValueRange::LessThan());
				
				//	set the returned size
				ioDataSize = theNumberItems * SizeOf32(AudioValueRange);
			}
			else
			{
				ThrowIfNULL(theStream, CAException(kAudioHardwareUnknownPropertyError), "HP_DeviceFormatList::GetPropertyData: no stream for given scope and element");
				theStream->ChangeDevicePropertyAddressIntoStreamPropertyAddress(theStreamAddress);
				theStream->GetPropertyData(theStreamAddress, inQualifierDataSize, inQualifierData, ioDataSize, outData);
			}
			break;
		
		default:
			//	let the stream handle it
			ThrowIfNULL(theStream, CAException(kAudioHardwareUnknownPropertyError), "HP_DeviceFormatList::GetPropertyData: no stream for given scope and element");
			theStream->ChangeDevicePropertyAddressIntoStreamPropertyAddress(theStreamAddress);
			theStream->GetPropertyData(theStreamAddress, inQualifierDataSize, inQualifierData, ioDataSize, outData);
			break;
	};
}

void	HP_DeviceFormatList::SetPropertyData(const AudioObjectPropertyAddress& inAddress, UInt32 inQualifierDataSize, const void* inQualifierData, UInt32 inDataSize, const void* inData, const AudioTimeStamp* inWhen)
{
	//	find the stream that goes with the given address
	UInt32 theNumberItems;
	UInt32 theIndex;
	UInt32 theSectionIndex;
	bool theSection;
	bool isDone;
	AudioObjectPropertyAddress theStreamAddress = inAddress;
	HP_Stream* theStream = mOwningDevice->GetStreamByPropertyAddress(inAddress, false);
	
	//	filter out properties that are different for devices
	switch(inAddress.mSelector)
	{
		case kAudioDevicePropertySupportsMixing:
			if(inAddress.mElement == 0)
			{
				ThrowIf(inDataSize != sizeof(UInt32), CAException(kAudioHardwareBadPropertySizeError), "HP_DeviceFormatList::SetPropertyData: wrong data size for kAudioDevicePropertySupportsMixing");
				
				//	figure out what they want to set
				bool isMixable = *(static_cast<const UInt32*>(inData)) != 0;
				
				//	iterate through all the streams and set it
				for(theSectionIndex = 0; theSectionIndex < 2; ++theSectionIndex)
				{
					theSection = theSectionIndex == 0;
					theNumberItems = mOwningDevice->GetNumberStreams(theSection);
					for(theIndex = 0; theIndex < theNumberItems; ++theIndex)
					{
						try
						{
							theStream = mOwningDevice->GetStreamByIndex(theSection, theIndex);
							theStream->SetIsMixable(isMixable, true);
						}
						catch(...)
						{
							//	don't bother with excetions here
						}
					}
				}
			}
			else
			{
				ThrowIfNULL(theStream, CAException(kAudioHardwareUnknownPropertyError), "HP_DeviceFormatList::SetPropertyData: no stream for given scope and element");
				theStream->ChangeDevicePropertyAddressIntoStreamPropertyAddress(theStreamAddress);
				theStream->SetPropertyData(inAddress, inQualifierDataSize, inQualifierData, inDataSize, inData, inWhen);
			}
			break;
		
		case kAudioDevicePropertyNominalSampleRate:
			if(inAddress.mElement == 0)
			{
				ThrowIf(inDataSize != sizeof(Float64), CAException(kAudioHardwareBadPropertySizeError), "HP_DeviceFormatList::SetPropertyData: wrong data size for kAudioDevicePropertyNominalSampleRate");
				Float64 theNewSampleRate = *(static_cast<const Float64*>(inData));
				
				//	iterate through all the streams to find one that supports the given sample rate
				isDone = false;
				for(theSectionIndex = 0; !isDone && (theSectionIndex < 2); ++theSectionIndex)
				{
					//	calculate the section (output first)
					theSection = theSectionIndex != 0;
					
					//	iterate through the streams
					theNumberItems = mOwningDevice->GetNumberStreams(theSection);
					for(theIndex = 0; !isDone && (theIndex < theNumberItems); ++theIndex)
					{
						try
						{
							theStream = mOwningDevice->GetStreamByIndex(theSection, theIndex);
							if((theStream != NULL) && theStream->SupportsNominalSampleRate(theNewSampleRate))
							{
								isDone = true;
								theStream->SetCurrentNominalSampleRate(theNewSampleRate, true);
							}
						}
						catch(...)
						{
							//	don't bother with exceptions here
						}
					}
				}
				
				ThrowIf(!isDone, CAException(kAudioDeviceUnsupportedFormatError), "HP_DeviceFormatList::SetPropertyData: no stream supports the given sample rate");
			}
			else
			{
				ThrowIfNULL(theStream, CAException(kAudioHardwareUnknownPropertyError), "HP_DeviceFormatList::SetPropertyData: no stream for given scope and element");
				theStream->ChangeDevicePropertyAddressIntoStreamPropertyAddress(theStreamAddress);
				theStream->SetPropertyData(inAddress, inQualifierDataSize, inQualifierData, inDataSize, inData, inWhen);
			}
			break;
						
		default:
			//	let the stream handle it
			ThrowIfNULL(theStream, CAException(kAudioHardwareUnknownPropertyError), "HP_DeviceFormatList::SetPropertyData: no stream for given scope and element");
			theStream->ChangeDevicePropertyAddressIntoStreamPropertyAddress(theStreamAddress);
			theStream->SetPropertyData(inAddress, inQualifierDataSize, inQualifierData, inDataSize, inData, inWhen);
			break;
	};
}

UInt32	HP_DeviceFormatList::GetNumberAddressesImplemented() const
{
	return 13;
}

void	HP_DeviceFormatList::GetImplementedAddressByIndex(UInt32 inIndex, AudioObjectPropertyAddress& outAddress) const
{
	switch(inIndex)
	{
		//  device properties
		case 0:
			outAddress.mSelector = kAudioDevicePropertyNominalSampleRate;
			break;
			
		case 1:
			outAddress.mSelector = kAudioDevicePropertyAvailableNominalSampleRates;
			break;
		
		//  stream properties
		case 2:
			outAddress.mSelector = kAudioStreamPropertyVirtualFormat;
			break;
			
		case 3:
			outAddress.mSelector = kAudioStreamPropertyAvailableVirtualFormats;
			break;
			
		case 4:
			outAddress.mSelector = kAudioStreamPropertyPhysicalFormat;
			break;
			
		case 5:
			outAddress.mSelector = kAudioStreamPropertyAvailablePhysicalFormats;
			break;
			
		//  obsolete device properties
		case 6:
			outAddress.mSelector = kAudioDevicePropertyStreamFormats;
			break;
			
		case 7:
			outAddress.mSelector = kAudioDevicePropertyStreamFormatSupported;
			break;
			
		case 8:
			outAddress.mSelector = kAudioDevicePropertyStreamFormatMatch;
			break;
			
		//  obsolete stream properties
		case 9:
			outAddress.mSelector = kAudioStreamPropertyPhysicalFormats;
			break;
			
		case 10:
			outAddress.mSelector = kAudioStreamPropertyPhysicalFormatSupported;
			break;
			
		case 11:
			outAddress.mSelector = kAudioStreamPropertyPhysicalFormatMatch;
			break;
		
		case 12:
			outAddress.mSelector = kAudioDevicePropertySupportsMixing;
			break;
	};
	outAddress.mScope = kAudioObjectPropertyScopeWildcard;
	outAddress.mElement = kAudioObjectPropertyElementWildcard;
}

void	HP_DeviceFormatList::GatherAvailableNominalSampleRateRanges(HP_SampleRateRangeList& outRanges) const
{
	//	intialize the return value
	outRanges.clear();

	//	declare some common locals
	bool					isFirstStream = true;
	HP_Stream*				theStream = NULL;
	UInt32					theStreamIndex = 0;
	UInt32					theNumberStreams = 0;
	AudioValueRange			theRange = { 0, 0 };
	UInt32					theNumberRanges = 0;
	UInt32					theRangeIndex = 0;
	HP_SampleRateRangeList	theTempList;
	
	if(!mUseIntersectionForMasterRates)
	{
		//	iterate through the input streams
		theNumberStreams = mOwningDevice->GetNumberStreams(true);
		for(theStreamIndex = 0; theStreamIndex < theNumberStreams; ++theStreamIndex)
		{
			//	get the stream
			theStream = mOwningDevice->GetStreamByIndex(true, theStreamIndex);
			
			//	iterate through it's available sample rates
			theNumberRanges = theStream->GetNumberAvailableNominalSampleRateRanges();
			for(theRangeIndex = 0; theRangeIndex < theNumberRanges; ++theRangeIndex)
			{
				//	clear the recepticle for the union
				theTempList.clear();
				
				//	get the range
				theStream->GetAvailableNominalSampleRateRangeByIndex(theRangeIndex, theRange);
				
				//	compute the union
				AFL_ComputeUnion(theRange, outRanges, theTempList);
				
				//	put the union into the return value
				outRanges = theTempList;
			}
		}
		
		//	iterate through the output streams
		theNumberStreams = mOwningDevice->GetNumberStreams(false);
		for(theStreamIndex = 0; theStreamIndex < theNumberStreams; ++theStreamIndex)
		{
			//	get the stream
			theStream = mOwningDevice->GetStreamByIndex(false, theStreamIndex);
			
			//	iterate through it's available sample rates
			theNumberRanges = theStream->GetNumberAvailableNominalSampleRateRanges();
			for(theRangeIndex = 0; theRangeIndex < theNumberRanges; ++theRangeIndex)
			{
				//	clear the recepticle for the union
				theTempList.clear();
				
				//	get the range
				theStream->GetAvailableNominalSampleRateRangeByIndex(theRangeIndex, theRange);
				
				//	compute the union
				AFL_ComputeUnion(theRange, outRanges, theTempList);
				
				//	put the union into the return value
				outRanges = theTempList;
			}
		}
	}
	else
	{
		//	iterate through the input streams
		theNumberStreams = mOwningDevice->GetNumberStreams(true);
		for(theStreamIndex = 0; theStreamIndex < theNumberStreams; ++theStreamIndex)
		{
			//	get the stream
			theStream = mOwningDevice->GetStreamByIndex(true, theStreamIndex);
			
			//	clear the intersections
			theTempList.clear();
			
			//	iterate through it's available sample rates
			theNumberRanges = theStream->GetNumberAvailableNominalSampleRateRanges();
			for(theRangeIndex = 0; theRangeIndex < theNumberRanges; ++theRangeIndex)
			{
				//	get the range
				theStream->GetAvailableNominalSampleRateRangeByIndex(theRangeIndex, theRange);
				
				//	put it in the return vector
				if(!isFirstStream)
				{
					//	calculate the intersections
					AFL_ComputeIntersection(theRange, outRanges, theTempList);
				}
				else
				{
					//	the set of ranges for the first stream go straight in 
					theTempList.push_back(theRange);
				}
			}
			
			//	set the return value from the intersections
			outRanges = theTempList;
			
			//	clear the first stream marker
			isFirstStream = false;
		}
		
		//	iterate through the output streams
		theNumberStreams = mOwningDevice->GetNumberStreams(false);
		for(theStreamIndex = 0; theStreamIndex < theNumberStreams; ++theStreamIndex)
		{
			//	get the stream
			theStream = mOwningDevice->GetStreamByIndex(false, theStreamIndex);
			
			//	clear the intersections
			theTempList.clear();
			
			//	iterate through it's available sample rates
			theNumberRanges = theStream->GetNumberAvailableNominalSampleRateRanges();
			for(theRangeIndex = 0; theRangeIndex < theNumberRanges; ++theRangeIndex)
			{
				//	get the range
				theStream->GetAvailableNominalSampleRateRangeByIndex(theRangeIndex, theRange);
				
				//	put it in the return vector
				if(!isFirstStream)
				{
					//	calculate the intersections
					AFL_ComputeIntersection(theRange, outRanges, theTempList);
				}
				else
				{
					//	the set of ranges for the first stream go straight in 
					theTempList.push_back(theRange);
				}
			}
			
			//	set the return value from the intersections
			outRanges = theTempList;
			
			//	clear the first stream marker
			isFirstStream = false;
		}
	}
}

UInt32	HP_DeviceFormatList::GetNumberAvailableMasterNominalSampleRateRanges() const
{
	HP_SampleRateRangeList theRanges;
	GatherAvailableNominalSampleRateRanges(theRanges);
	return ToUInt32(theRanges.size());
}

void	HP_DeviceFormatList::GetAvailableMasterNominalSampleRateRangeByIndex(UInt32 inIndex, AudioValueRange& outRange) const
{
	HP_SampleRateRangeList theRanges;
	GatherAvailableNominalSampleRateRanges(theRanges);
	if(inIndex < theRanges.size())
	{
		outRange = theRanges.at(inIndex);
	}
	else
	{
		outRange.mMinimum = 0;
		outRange.mMaximum = 0;
	}
}

bool	HP_DeviceFormatList::SupportsMasterNominalSampleRate(Float64 inNewSampleRate) const
{
	bool theAnswer = false;
	
	HP_SampleRateRangeList theRanges;
	GatherAvailableNominalSampleRateRanges(theRanges);
	
	HP_SampleRateRangeList::const_iterator theRateRangeIterator = theRanges.begin();
	while(!theAnswer && (theRateRangeIterator != theRanges.end()))
	{
		if(CAAudioValueRange::ContainsValue(*theRateRangeIterator, inNewSampleRate))
		{
			theAnswer = true;
		}
		else
		{
			std::advance(theRateRangeIterator, 1);
		}
	}
	
	return theAnswer;
}

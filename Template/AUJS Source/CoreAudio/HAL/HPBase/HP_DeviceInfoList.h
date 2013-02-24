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
#if !defined(__HP_DeviceInfoList_h__)
#define __HP_DeviceInfoList_h__

//==================================================================================================
//	Includes
//==================================================================================================

//	Local Includes
#include "AudioHardware.h"

//	PublicUtility Includes
#include "CAStreamBasicDescription.h"
#include "CADebugMacros.h"

//	Standard Library Includes
#include <vector>

//==================================================================================================
//	HP_StreamInfoListItem
//==================================================================================================

template <class StreamHWObjectType>
class	HP_StreamInfoListItem
{
public:
	AudioObjectID				mAudioStreamID;
	UInt32						mStartingChannel;
	AudioStreamBasicDescription	mPhysicalFormat;
	StreamHWObjectType			mHWObject;
	
	HP_StreamInfoListItem() : mAudioStreamID(0), mStartingChannel(0), mPhysicalFormat(CAStreamBasicDescription::sEmpty), mHWObject(0) {}
	HP_StreamInfoListItem(const HP_StreamInfoListItem& inItem) : mAudioStreamID(inItem.mAudioStreamID), mStartingChannel(inItem.mStartingChannel), mPhysicalFormat(inItem.mPhysicalFormat), mHWObject(inItem.mHWObject) {}
	HP_StreamInfoListItem& operator=(const HP_StreamInfoListItem& inItem) { mAudioStreamID = inItem.mAudioStreamID; mStartingChannel = inItem.mStartingChannel; mPhysicalFormat = inItem.mPhysicalFormat; mHWObject = inItem.mHWObject;  return *this; }
	~HP_StreamInfoListItem() {}
};

template <class StreamHWObjectType>
inline bool	operator<(const HP_StreamInfoListItem<StreamHWObjectType>& x, const HP_StreamInfoListItem<StreamHWObjectType>& y)	{ return x.mStartingChannel < y.mStartingChannel; }
template <class StreamHWObjectType>
inline bool	operator==(const HP_StreamInfoListItem<StreamHWObjectType>& x, const HP_StreamInfoListItem<StreamHWObjectType>& y)	{ return x.mStartingChannel == y.mStartingChannel; }
template <class StreamHWObjectType>
inline bool	operator!=(const HP_StreamInfoListItem<StreamHWObjectType>& x, const HP_StreamInfoListItem<StreamHWObjectType>& y)	{ return !(x == y); }
template <class StreamHWObjectType>
inline bool	operator<=(const HP_StreamInfoListItem<StreamHWObjectType>& x, const HP_StreamInfoListItem<StreamHWObjectType>& y)	{ return (x < y) || (x == y); }
template <class StreamHWObjectType>
inline bool	operator>=(const HP_StreamInfoListItem<StreamHWObjectType>& x, const HP_StreamInfoListItem<StreamHWObjectType>& y)	{ return !(x < y); }
template <class StreamHWObjectType>
inline bool	operator>(const HP_StreamInfoListItem<StreamHWObjectType>& x, const HP_StreamInfoListItem<StreamHWObjectType>& y)	{ return !((x < y) || (x == y)); }

//==================================================================================================
//	HP_DeviceInfoList
//
//	This class provides a means to save the current state of the device so that it can be compared
//	with other states.
//==================================================================================================

template <class StreamHWObjectType>
class HP_DeviceInfoList
{

//	Types
private:
	typedef std::vector<HP_StreamInfoListItem<StreamHWObjectType> >	StreamInfoList;
	
//	Construction/Destruction
public:
	HP_DeviceInfoList()
	:
		mInputLatency(0),
		mOutputLatency(0),
		mInputSafetyOffset(0),
		mOutputSafetyOffset(0),
		mInputStreamInfoList(),
		mOutputStreamInfoList()
	{
	}

	virtual	~HP_DeviceInfoList()
	{
	}

//	Device Operations
public:
	UInt32					GetLatency(bool inIsInput) const
	{
		return inIsInput ? mInputLatency : mOutputLatency;
	}
	
	UInt32					GetSafetyOffset(bool inIsInput) const
	{
		return inIsInput ? mInputSafetyOffset : mOutputSafetyOffset;
	}

	UInt32					GetNumberStreams(bool inIsInput) const
	{
		const StreamInfoList& theStreamInfoList = GetStreamInfoList(inIsInput);
		return ToUInt32(theStreamInfoList.size());
	}
	
	UInt32					GetTotalNumberChannels(bool inIsInput) const
	{
		UInt32 theAnswer = 0;
		
		const StreamInfoList& theStreamInfoList = inIsInput ? mInputStreamInfoList : mOutputStreamInfoList;
		typename StreamInfoList::const_iterator theIterator = theStreamInfoList.begin();
		while(theIterator != theStreamInfoList.end())
		{
			theAnswer += theIterator->mPhysicalFormat.mChannelsPerFrame;
			std::advance(theIterator, 1);
		}
		
		return theAnswer;
	}

//	Stream Operations
public:
	AudioObjectID			GetStreamAudioObjectIDByIndex(bool inIsInput, UInt32 inStreamIndex) const
	{
		AudioObjectID theAnswer = 0;
		const StreamInfoList& theStreamInfoList = GetStreamInfoList(inIsInput);
		if(inStreamIndex < theStreamInfoList.size())
		{
			typename StreamInfoList::const_iterator theIterator = theStreamInfoList.begin();
			std::advance(theIterator, inStreamIndex);
			theAnswer = theIterator->mAudioStreamID;
		}
		return theAnswer;
	}

	void					SetStreamAudioObjectIDByIndex(bool inIsInput, UInt32 inStreamIndex, AudioObjectID inAudioStreamID)
	{
		StreamInfoList& theStreamInfoList = GetStreamInfoList(inIsInput);
		if(inStreamIndex < theStreamInfoList.size())
		{
			typename StreamInfoList::iterator theIterator = theStreamInfoList.begin();
			std::advance(theIterator, inStreamIndex);
			theIterator->mAudioStreamID = inAudioStreamID;
		}
	}

	UInt32					GetStreamStartingChannelByIndex(bool inIsInput, UInt32 inStreamIndex) const
	{
		UInt32 theAnswer = 0;
		const StreamInfoList& theStreamInfoList = GetStreamInfoList(inIsInput);
		if(inStreamIndex < theStreamInfoList.size())
		{
			typename StreamInfoList::const_iterator theIterator = theStreamInfoList.begin();
			std::advance(theIterator, inStreamIndex);
			theAnswer = theIterator->mStartingChannel;
		}
		return theAnswer;
	}

	void					GetStreamPhysicalFormatByIndex(bool inIsInput, UInt32 inStreamIndex, AudioStreamBasicDescription& outFormat) const
	{
		const StreamInfoList& theStreamInfoList = GetStreamInfoList(inIsInput);
		if(inStreamIndex < theStreamInfoList.size())
		{
			typename StreamInfoList::const_iterator theIterator = theStreamInfoList.begin();
			std::advance(theIterator, inStreamIndex);
			outFormat = theIterator->mPhysicalFormat;
		}
	}

	StreamHWObjectType		GetStreamHWObjectByIndex(bool inIsInput, UInt32 inStreamIndex) const
	{
		StreamHWObjectType theAnswer = (StreamHWObjectType)0;
		const StreamInfoList& theStreamInfoList = GetStreamInfoList(inIsInput);
		if(inStreamIndex < theStreamInfoList.size())
		{
			typename StreamInfoList::const_iterator theIterator = theStreamInfoList.begin();
			std::advance(theIterator, inStreamIndex);
			theAnswer = theIterator->mHWObject;
		}
		return theAnswer;
	}
	
	AudioObjectID			GetStreamAudioObjectIDByDeviceChannel(bool inIsInput, UInt32 inDeviceChannel) const
	{
		AudioObjectID theAnswer = 0;
		HP_StreamInfoListItem<StreamHWObjectType> theItem;
		if(GetItemByDeviceChannel(inIsInput, inDeviceChannel, theItem))
		{
			theAnswer = theItem.mAudioStreamID;
		}
		return theAnswer;
	}

	UInt32					GetStreamStartingChannelByDeviceChannel(bool inIsInput, UInt32 inDeviceChannel) const
	{
		UInt32 theAnswer = 0;
		HP_StreamInfoListItem<StreamHWObjectType> theItem;
		if(GetItemByDeviceChannel(inIsInput, inDeviceChannel, theItem))
		{
			theAnswer = theItem.mStartingChannel;
		}
		return theAnswer;
	}

	void					GetStreamPhysicalFormatByDeviceChannel(bool inIsInput, UInt32 inDeviceChannel, AudioStreamBasicDescription& outFormat) const
	{
		HP_StreamInfoListItem<StreamHWObjectType> theItem;
		if(GetItemByDeviceChannel(inIsInput, inDeviceChannel, theItem))
		{
			outFormat = theItem.mPhysicalFormat;
		}
	}

	StreamHWObjectType		GetStreamHWObjectByDeviceChannel(bool inIsInput, UInt32 inDeviceChannel) const
	{
		StreamHWObjectType theAnswer = NULL;
		HP_StreamInfoListItem<StreamHWObjectType> theItem;
		if(GetItemByDeviceChannel(inIsInput, inDeviceChannel, theItem))
		{
			theAnswer = theItem.mHWObject;
		}
		return theAnswer;
	}

//	Implementation
protected:
	const StreamInfoList&	GetStreamInfoList(bool inIsInput) const { return (inIsInput ? mInputStreamInfoList: mOutputStreamInfoList); }
	StreamInfoList&			GetStreamInfoList(bool inIsInput) { return (inIsInput ? mInputStreamInfoList: mOutputStreamInfoList); }
	bool					GetItemByDeviceChannel(bool inIsInput, UInt32 inDeviceChannel, HP_StreamInfoListItem<StreamHWObjectType>& outItem) const
	{
		const StreamInfoList& theStreamInfoList = GetStreamInfoList(inIsInput);
		typename StreamInfoList::const_iterator theIterator = theStreamInfoList.begin();
		bool wasFound = false;
		while((theIterator != theStreamInfoList.end()) && !wasFound)
		{
			if((inDeviceChannel >= theIterator->mStartingChannel) && (inDeviceChannel < (theIterator->mStartingChannel + theIterator->mPhysicalFormat.mChannelsPerFrame)))
			{
				wasFound = true;
				outItem = *theIterator;
			}
		}
		
		return wasFound;
	}
	
	UInt32					mInputLatency;
	UInt32					mOutputLatency;
	UInt32					mInputSafetyOffset;
	UInt32					mOutputSafetyOffset;
	StreamInfoList			mInputStreamInfoList;
	StreamInfoList			mOutputStreamInfoList;

};

#endif

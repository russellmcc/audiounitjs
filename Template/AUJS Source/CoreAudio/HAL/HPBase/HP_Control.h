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
#if !defined(__HP_Control_h__)
#define __HP_Control_h__

//==================================================================================================
//	Includes
//==================================================================================================

//	Super Class Includes
#include "HP_Object.h"
#include "HP_Property.h"

//==================================================================================================
//	Types
//==================================================================================================

class	HP_Device;

//==================================================================================================
//	HP_Control
//==================================================================================================

class HP_Control
:
	public HP_Object
{

//	Construction/Destruction
public:
										HP_Control(AudioObjectID inObjectID, AudioClassID inClassID, HP_HardwarePlugIn* inPlugIn, HP_Device* inOwningDevice);
	virtual								~HP_Control();

//	Attributes
public:
	virtual AudioClassID				GetBaseClassID() const;
	virtual CAMutex*					GetObjectStateMutex();
	virtual void						Show() const;
	bool								GetMark() const { return mMark; }
	void								SetMark(bool inMark) { mMark = inMark; }
	virtual CFStringRef					CopyName() const;
	virtual CFStringRef					CopyManufacturerName() const;
	virtual AudioObjectPropertyScope	GetPropertyScope() const = 0;
	virtual AudioObjectPropertyElement	GetPropertyElement() const = 0;
	virtual void*						GetImplementationObject() const;
	virtual UInt32						GetVariant() const;
	virtual bool						IsReadOnly() const;
	
//	Property Access
public:
	virtual bool						HasProperty(const AudioObjectPropertyAddress& inAddress) const;
	virtual bool						IsPropertySettable(const AudioObjectPropertyAddress& inAddress) const;
	virtual UInt32						GetPropertyDataSize(const AudioObjectPropertyAddress& inAddress, UInt32 inQualifierDataSize, const void* inQualifierData) const;
	virtual void						GetPropertyData(const AudioObjectPropertyAddress& inAddress, UInt32 inQualifierDataSize, const void* inQualifierData, UInt32& ioDataSize, void* outData) const;
	virtual void						SetPropertyData(const AudioObjectPropertyAddress& inAddress, UInt32 inQualifierDataSize, const void* inQualifierData, UInt32 inDataSize, const void* inData, const AudioTimeStamp* inWhen);

	virtual void						ValueChanged();
	virtual void						RangeChanged();

protected:
	virtual void						GetValueChangedDeviceNotifications(CAPropertyAddressList& outDeviceNotifications);
	virtual void						GetRangeChangedDeviceNotifications(CAPropertyAddressList& outDeviceNotifications);
	virtual void						SendNotifications(const CAPropertyAddressList& inDeviceNotifications);

//	Implementation
protected:
	HP_Device*							mOwningDevice;
	bool								mMark;

};

//==================================================================================================
//	HP_LevelControl
//==================================================================================================

class HP_LevelControl
:
	public HP_Control
{

//	Construction/Destruction
public:
						HP_LevelControl(AudioObjectID inObjectID, AudioClassID inClassID, HP_HardwarePlugIn* inPlugIn, HP_Device* inOwningDevice);
	virtual				~HP_LevelControl();

//	Attributes
public:
	virtual AudioClassID	GetBaseClassID() const;
	virtual Float32		GetMinimumDBValue() const = 0;
	virtual Float32		GetMaximumDBValue() const = 0;

	virtual Float32		GetDBValue() const = 0;
	virtual void		SetDBValue(Float32 inDBValue) = 0;

	virtual Float32		GetScalarValue() const = 0;
	virtual void		SetScalarValue(Float32 inScalarValue) = 0;

	virtual Float32		ConverScalarValueToDBValue(Float32 inScalarValue) const = 0;
	virtual Float32		ConverDBValueToScalarValue(Float32 inDBValue) const = 0;

//	Property Access
public:
	virtual bool		HasProperty(const AudioObjectPropertyAddress& inAddress) const;
	virtual bool		IsPropertySettable(const AudioObjectPropertyAddress& inAddress) const;
	virtual UInt32		GetPropertyDataSize(const AudioObjectPropertyAddress& inAddress, UInt32 inQualifierDataSize, const void* inQualifierData) const;
	virtual void		GetPropertyData(const AudioObjectPropertyAddress& inAddress, UInt32 inQualifierDataSize, const void* inQualifierData, UInt32& ioDataSize, void* outData) const;
	virtual void		SetPropertyData(const AudioObjectPropertyAddress& inAddress, UInt32 inQualifierDataSize, const void* inQualifierData, UInt32 inDataSize, const void* inData, const AudioTimeStamp* inWhen);

};

//==================================================================================================
//	HP_BooleanControl
//==================================================================================================

class HP_BooleanControl
:
	public HP_Control
{

//	Construction/Destruction
public:
						HP_BooleanControl(AudioObjectID inObjectID, AudioClassID inClassID, HP_HardwarePlugIn* inPlugIn, HP_Device* inOwningDevice);
	virtual				~HP_BooleanControl();

//	Attributes
public:
	virtual AudioClassID	GetBaseClassID() const;
	virtual bool		GetValue() const = 0;
	virtual void		SetValue(bool inValue) = 0;

//	Property Access
public:
	virtual bool		HasProperty(const AudioObjectPropertyAddress& inAddress) const;
	virtual bool		IsPropertySettable(const AudioObjectPropertyAddress& inAddress) const;
	virtual UInt32		GetPropertyDataSize(const AudioObjectPropertyAddress& inAddress, UInt32 inQualifierDataSize, const void* inQualifierData) const;
	virtual void		GetPropertyData(const AudioObjectPropertyAddress& inAddress, UInt32 inQualifierDataSize, const void* inQualifierData, UInt32& ioDataSize, void* outData) const;
	virtual void		SetPropertyData(const AudioObjectPropertyAddress& inAddress, UInt32 inQualifierDataSize, const void* inQualifierData, UInt32 inDataSize, const void* inData, const AudioTimeStamp* inWhen);

};

//==================================================================================================
//	HP_SelectorControl
//==================================================================================================

class HP_SelectorControl
:
	public HP_Control
{

//	Construction/Destruction
public:
						HP_SelectorControl(AudioObjectID inObjectID, AudioClassID inClassID, HP_HardwarePlugIn* inPlugIn, HP_Device* inOwningDevice);
	virtual				~HP_SelectorControl();

//	Attributes
public:
	virtual AudioClassID	GetBaseClassID() const;
	virtual UInt32		GetNumberItems() const = 0;

	virtual UInt32		GetCurrentItemID() const = 0;
	virtual UInt32		GetCurrentItemIndex() const = 0;
	
	virtual void		SetCurrentItemByID(UInt32 inItemID) = 0;
	virtual void		SetCurrentItemByIndex(UInt32 inItemIndex) = 0;
	
	virtual UInt32		GetItemIDForIndex(UInt32 inItemIndex) const = 0;
	virtual UInt32		GetItemIndexForID(UInt32 inItemID) const = 0;
	
	virtual CFStringRef	CopyItemNameByID(UInt32 inItemID) const = 0;
	virtual CFStringRef	CopyItemNameByIndex(UInt32 inItemIndex) const = 0;
	
	virtual CFStringRef	CopyItemNameByIDWithoutLocalizing(UInt32 inItemID) const = 0;
	virtual CFStringRef	CopyItemNameByIndexWithoutLocalizing(UInt32 inItemIndex) const = 0;
	
	virtual UInt32		GetItemKindByID(UInt32 inItemID) const = 0;
	virtual UInt32		GetItemKindByIndex(UInt32 inItemIndex) const = 0;

//	Property Access
public:
	virtual bool		HasProperty(const AudioObjectPropertyAddress& inAddress) const;
	virtual bool		IsPropertySettable(const AudioObjectPropertyAddress& inAddress) const;
	virtual UInt32		GetPropertyDataSize(const AudioObjectPropertyAddress& inAddress, UInt32 inQualifierDataSize, const void* inQualifierData) const;
	virtual void		GetPropertyData(const AudioObjectPropertyAddress& inAddress, UInt32 inQualifierDataSize, const void* inQualifierData, UInt32& ioDataSize, void* outData) const;
	virtual void		SetPropertyData(const AudioObjectPropertyAddress& inAddress, UInt32 inQualifierDataSize, const void* inQualifierData, UInt32 inDataSize, const void* inData, const AudioTimeStamp* inWhen);

};

//==================================================================================================
//	HP_StereoPanControl
//==================================================================================================

class HP_StereoPanControl
:
	public HP_Control
{

//	Construction/Destruction
public:
						HP_StereoPanControl(AudioObjectID inObjectID, AudioClassID inClassID, HP_HardwarePlugIn* inPlugIn, HP_Device* inOwningDevice);
	virtual				~HP_StereoPanControl();

//	Attributes
public:
	virtual AudioClassID	GetBaseClassID() const;
	virtual Float32		GetValue() const = 0;
	virtual void		SetValue(Float32 inValue) = 0;
	virtual void		GetChannels(UInt32& outLeftChannel, UInt32& outRightChannel) const = 0;

//	Property Access
public:
	virtual bool		HasProperty(const AudioObjectPropertyAddress& inAddress) const;
	virtual bool		IsPropertySettable(const AudioObjectPropertyAddress& inAddress) const;
	virtual UInt32		GetPropertyDataSize(const AudioObjectPropertyAddress& inAddress, UInt32 inQualifierDataSize, const void* inQualifierData) const;
	virtual void		GetPropertyData(const AudioObjectPropertyAddress& inAddress, UInt32 inQualifierDataSize, const void* inQualifierData, UInt32& ioDataSize, void* outData) const;
	virtual void		SetPropertyData(const AudioObjectPropertyAddress& inAddress, UInt32 inQualifierDataSize, const void* inQualifierData, UInt32 inDataSize, const void* inData, const AudioTimeStamp* inWhen);

};

//==================================================================================================
//	HP_DeviceControlProperty
//==================================================================================================

class HP_DeviceControlProperty
:
	public HP_Property
{

//	Construction/Destruction
public:
						HP_DeviceControlProperty(HP_Device* inDevice);
	virtual				~HP_DeviceControlProperty();

//	Operations
public:
	virtual bool		IsActive(const AudioObjectPropertyAddress& inAddress) const;
	virtual bool		IsPropertySettable(const AudioObjectPropertyAddress& inAddress) const;
	virtual UInt32		GetPropertyDataSize(const AudioObjectPropertyAddress& inAddress, UInt32 inQualifierDataSize, const void* inQualifierData) const;
	virtual void		GetPropertyData(const AudioObjectPropertyAddress& inAddress, UInt32 inQualifierDataSize, const void* inQualifierData, UInt32& ioDataSize, void* outData) const;
	virtual void		SetPropertyData(const AudioObjectPropertyAddress& inAddress, UInt32 inQualifierDataSize, const void* inQualifierData, UInt32 inDataSize, const void* inData, const AudioTimeStamp* inWhen);

	virtual UInt32		GetNumberAddressesImplemented() const;
	virtual void		GetImplementedAddressByIndex(UInt32 inIndex, AudioObjectPropertyAddress& outAddress) const;

//	Implementation
protected:
	HP_Device*			mDevice;

};

#endif

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
#if !defined(__HP_Object_h__)
#define __HP_Object_h__

//==================================================================================================
//	Includes
//==================================================================================================

//	System Includes
#include <CoreAudio/AudioHardwarePlugIn.h>

//	PublicUtility Includes
#include "CAPropertyAddress.h"

//	Standard Library Includes
#include <vector>

//==================================================================================================
//	Types
//==================================================================================================

class	CAMutex;
class	HP_Device;
class	HP_HardwarePlugIn;
class   HP_Property;
class	HP_Stream;

//==================================================================================================
//	HP_Object
//==================================================================================================

class HP_Object
{

//	Construction/Destruction
public:
							HP_Object(AudioObjectID inObjectID, AudioClassID inClassID, HP_HardwarePlugIn* inPlugIn);
	virtual					~HP_Object();
	
	virtual void			Initialize();
	virtual void			Teardown();

private:
							HP_Object(const HP_Object&);
	HP_Object&				operator=(const HP_Object&);

//  Attributes
public:
	AudioObjectID			GetObjectID() const						{ return mObjectID; }
	void					SetObjectID(AudioObjectID inObjectID);
	AudioClassID			GetClassID() const						{ return mClassID; }
	HP_HardwarePlugIn*		GetPlugIn() const						{ return mPlugIn; }

	static bool				IsSubClass(AudioClassID inClassID, AudioClassID inBaseClassID);

protected:
	AudioObjectID			mObjectID;
	AudioClassID			mClassID;
	HP_HardwarePlugIn*		mPlugIn;
	
//  Operations
public:
	virtual CAMutex*		GetObjectStateMutex();
	virtual void			Show() const;
	
	static HP_Object*		GetObjectByID(AudioObjectID inObjectID);
	static HP_Device*		GetDeviceByID(AudioObjectID inObjectID);
	static HP_Stream*		GetStreamByID(AudioObjectID inObjectID);

	static CAMutex*			GetObjectStateMutexByID(AudioObjectID inObjectID);
	static void				SetObjectStateMutexForID(AudioObjectID inObjectID, CAMutex* inMutex);
	
//	Property Access
public:
	virtual bool			HasProperty(const AudioObjectPropertyAddress& inAddress) const;
	virtual bool			IsPropertySettable(const AudioObjectPropertyAddress& inAddress) const;
	virtual UInt32			GetPropertyDataSize(const AudioObjectPropertyAddress& inAddress, UInt32 inQualifierDataSize, const void* inQualifierData) const;
	virtual void			GetPropertyData(const AudioObjectPropertyAddress& inAddress, UInt32 inQualifierDataSize, const void* inQualifierData, UInt32& ioDataSize, void* outData) const;
	virtual void			SetPropertyData(const AudioObjectPropertyAddress& inAddress, UInt32 inQualifierDataSize, const void* inQualifierData, UInt32 inDataSize, const void* inData, const AudioTimeStamp* inWhen);
	virtual void			PropertiesChanged(UInt32 inNumberAddresses, const AudioObjectPropertyAddress inAddresses[]) const;

protected:
	virtual void			PropertyListenerAdded(const AudioObjectPropertyAddress& inAddress);
	virtual void			PropertyListenerRemoved(const AudioObjectPropertyAddress& inAddress);

//	Property Management
public:
	void					AddProperty(HP_Property* inProperty);
	void					RemoveProperty(HP_Property* inProperty);

private:
	typedef std::pair<CAPropertyAddress, HP_Property*>  PropertyMapItem;
	typedef std::vector<PropertyMapItem>				PropertyMap;
	
	PropertyMap::iterator   FindPropertyByAddress(const AudioObjectPropertyAddress& inAddress);
	HP_Property*			FindActivePropertyByAddress(const AudioObjectPropertyAddress& inAddress) const;
	
	PropertyMap				mPropertyMap;

};

#endif

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

//	Internal Includes
#include "HP_Device.h"
#include "HP_Object.h"
#include "HP_HardwarePlugIn.h"
#include "HP_Stream.h"

//	System Includes
#include <CoreAudio/AudioHardwarePlugIn.h>

//	PublicUtility Includes
#include "CACFObject.h"
#include "CADebugMacros.h"
#include "CAException.h"
#include "CAMutex.h"

//==================================================================================================
//	HP_HardwarePlugInInterface
//==================================================================================================

#pragma mark	Plug-In Operations

static ULONG	HP_HardwarePlugIn_AddRef(AudioHardwarePlugInRef inSelf)
{
	ULONG theAnswer = 0;
	OSStatus theError = kAudioHardwareNoError;
	
	try
	{
		//  check the function arguments
		ThrowIfNULL(inSelf, CAException(kAudioHardwareIllegalOperationError), "HP_HardwarePlugIn_AddRef: no plug-in");

		//	get the object out of the interface reference
		HP_HardwarePlugIn* thePlugIn = HP_HardwarePlugIn::GetObject(inSelf);
		
		//	retain it
		theAnswer = thePlugIn->Retain();
	}
	catch(const CAException& inException)
	{
		theError = inException.GetError();
	}
	catch(...)
	{
		theError = kAudioHardwareUnspecifiedError;
	}
	
	return theAnswer;
}

static ULONG	HP_HardwarePlugIn_Release(AudioHardwarePlugInRef inSelf)
{
	ULONG theAnswer = 0;
	OSStatus theError = kAudioHardwareNoError;
	
	try
	{
		//  check the function arguments
		ThrowIfNULL(inSelf, CAException(kAudioHardwareIllegalOperationError), "HP_HardwarePlugIn_Release: no plug-in");

		//	get the object out of the interface reference
		HP_HardwarePlugIn* thePlugIn = HP_HardwarePlugIn::GetObject(inSelf);
		
		//	release it
		theAnswer = thePlugIn->Release();
		
		//	note that thePlugIn is invalid now, so don't use it!
	}
	catch(const CAException& inException)
	{
		theError = inException.GetError();
	}
	catch(...)
	{
		theError = kAudioHardwareUnspecifiedError;
	}
	
	return theAnswer;
}

static HRESULT	HP_HardwarePlugIn_QueryInterface(AudioHardwarePlugInRef inSelf, REFIID inUUID, LPVOID* outInterface)
{
	OSStatus theError = kAudioHardwareNoError;
	
	try
	{
		//  check the function arguments
		ThrowIfNULL(inSelf, CAException(kAudioHardwareIllegalOperationError), "HP_HardwarePlugIn_QueryInterface: no plug-in");
		ThrowIfNULL(outInterface, CAException(kAudioHardwareIllegalOperationError), "HP_HardwarePlugIn_QueryInterface: no place to store the return value");

		//	set the returned interface to NULL
		*outInterface = NULL;
		
		//	get the object out of the interface reference
		HP_HardwarePlugIn* thePlugIn = HP_HardwarePlugIn::GetObject(inSelf);
		
		// create a CoreFoundation UUIDRef for the requested interface.
		CACFUUID theInterfaceUUID(CFUUIDCreateFromUUIDBytes(NULL, inUUID));

		// test the requested ID against the valid interfaces.
#if	defined(kAudioHardwarePlugInInterface5ID)
		if(theInterfaceUUID.IsEqual(kAudioHardwarePlugInInterface5ID) || theInterfaceUUID.IsEqual(kAudioHardwarePlugInInterface4ID) || theInterfaceUUID.IsEqual(kAudioHardwarePlugInInterface3ID) || theInterfaceUUID.IsEqual(kAudioHardwarePlugInInterface2ID) || theInterfaceUUID.IsEqual(kAudioHardwarePlugInInterfaceID) || theInterfaceUUID.IsEqual(IUnknownUUID))
#else
		if(theInterfaceUUID.IsEqual(kAudioHardwarePlugInInterface4ID) || theInterfaceUUID.IsEqual(kAudioHardwarePlugInInterface3ID) || theInterfaceUUID.IsEqual(kAudioHardwarePlugInInterface2ID) || theInterfaceUUID.IsEqual(kAudioHardwarePlugInInterfaceID) || theInterfaceUUID.IsEqual(IUnknownUUID))
#endif
		{
			//	 it's one of the interfaces we understand
			
			//	retain the object on behalf of the caller
			thePlugIn->Retain();
			
			//	return the interface;
			*outInterface = thePlugIn->GetInterface();
		}
		else
		{
			//	not anything we understand
			theError = E_NOINTERFACE;
		}
	}
	catch(const CAException& inException)
	{
		theError = inException.GetError();
	}
	catch(...)
	{
		theError = kAudioHardwareUnspecifiedError;
	}
	
	return theError;
}

static OSStatus	HP_HardwarePlugIn_Initialize(AudioHardwarePlugInRef inSelf)
{
	OSStatus theError = kAudioHardwareNoError;
	
	try
	{
		//  check the function arguments
		ThrowIfNULL(inSelf, CAException(kAudioHardwareIllegalOperationError), "HP_HardwarePlugIn_Initialize: no plug-in");

		//	get the object out of the interface reference
		HP_HardwarePlugIn* thePlugIn = HP_HardwarePlugIn::GetObject(inSelf);
		
		//	do the work
		thePlugIn->Initialize();
	}
	catch(const CAException& inException)
	{
		theError = inException.GetError();
	}
	catch(...)
	{
		theError = kAudioHardwareUnspecifiedError;
	}
	
	return theError;
}

static OSStatus	HP_HardwarePlugIn_InitializeWithObjectID(AudioHardwarePlugInRef inSelf, AudioObjectID inObjectID)
{
	OSStatus theError = kAudioHardwareNoError;
	
	try
	{
		//  check the function arguments
		ThrowIfNULL(inSelf, CAException(kAudioHardwareIllegalOperationError), "HP_HardwarePlugIn_Initialize: no plug-in");

		//	get the object out of the interface reference
		HP_HardwarePlugIn* thePlugIn = HP_HardwarePlugIn::GetObject(inSelf);
		
		//	do the work
		thePlugIn->InitializeWithObjectID(inObjectID);
	}
	catch(const CAException& inException)
	{
		theError = inException.GetError();
	}
	catch(...)
	{
		theError = kAudioHardwareUnspecifiedError;
	}
	
	return theError;
}

static OSStatus	HP_HardwarePlugIn_Teardown(AudioHardwarePlugInRef inSelf)
{
	OSStatus theError = kAudioHardwareNoError;
	
	try
	{
		//  check the function arguments
		ThrowIfNULL(inSelf, CAException(kAudioHardwareIllegalOperationError), "HP_HardwarePlugIn_Teardown: no plug-in");

		//	get the object out of the interface reference
		HP_HardwarePlugIn* thePlugIn = HP_HardwarePlugIn::GetObject(inSelf);
		
		//	do the work
		thePlugIn->Teardown();
	}
	catch(const CAException& inException)
	{
		theError = inException.GetError();
	}
	catch(...)
	{
		theError = kAudioHardwareUnspecifiedError;
	}
	
	return theError;
}

#pragma mark	AudioObject Operations

static void HP_HardwarePlugIn_ObjectShow(AudioHardwarePlugInRef inSelf, AudioObjectID inObjectID)
{
	try
	{
		//  check the function arguments
		ThrowIfNULL(inSelf, CAException(kAudioHardwareIllegalOperationError), "HP_HardwarePlugIn_ObjectShow: no plug-in");

		//  find the object for the given ID
		HP_Object* theObject = HP_Object::GetObjectByID(inObjectID);
		ThrowIfNULL(theObject, CAException(kAudioHardwareBadObjectError), "HP_HardwarePlugIn_ObjectShow: no object with given ID");
		
		//	do the work
		theObject->Show();
	}
	catch(...)
	{
	}
}

static Boolean  HP_HardwarePlugIn_ObjectHasProperty(AudioHardwarePlugInRef inSelf, AudioObjectID inObjectID, const AudioObjectPropertyAddress* inAddress)
{
	Boolean		theAnswer = false;
	CAMutex*	theObjectStateMutex = NULL;
	bool		theObjectStateMutexNeedsUnlocking = false;
	
	try
	{
		//  check the function arguments
		ThrowIfNULL(inSelf, CAException(kAudioHardwareIllegalOperationError), "HP_HardwarePlugIn_ObjectHasProperty: no plug-in");
		ThrowIfNULL(inAddress, CAException(kAudioHardwareIllegalOperationError), "HP_HardwarePlugIn_ObjectHasProperty: no address");
		
		//	get the object state mutex
		theObjectStateMutex = HP_Object::GetObjectStateMutexByID(inObjectID);
		
		//	lock the mutex
		if(theObjectStateMutex != NULL)
		{
			theObjectStateMutexNeedsUnlocking = theObjectStateMutex->Lock();
		}
		
		//  find the object for the given ID
		HP_Object* theObject = HP_Object::GetObjectByID(inObjectID);
		ThrowIfNULL(theObject, CAException(kAudioHardwareBadObjectError), "HP_HardwarePlugIn_ObjectHasProperty: no object with given ID after locking");
		
		//	do the work
		theAnswer = theObject->HasProperty(*inAddress);
	}
	catch(const CAException& inException)
	{
		theAnswer = false;
	}
	catch(...)
	{
		theAnswer = false;
	}
	
	//	unlock the object state mutex if we need to
	if((theObjectStateMutex != NULL) && theObjectStateMutexNeedsUnlocking)
	{
		theObjectStateMutex->Unlock();
	}
	
	return theAnswer;
}

static OSStatus HP_HardwarePlugIn_ObjectIsPropertySettable(AudioHardwarePlugInRef inSelf, AudioObjectID inObjectID, const AudioObjectPropertyAddress* inAddress, Boolean* outIsSettable)
{
	OSStatus	theError = kAudioHardwareNoError;
	CAMutex*	theObjectStateMutex = NULL;
	bool		theObjectStateMutexNeedsUnlocking = false;
	
	try
	{
		//  check the function arguments
		ThrowIfNULL(inSelf, CAException(kAudioHardwareIllegalOperationError), "HP_HardwarePlugIn_ObjectIsPropertySettable: no plug-in");
		ThrowIfNULL(inAddress, CAException(kAudioHardwareIllegalOperationError), "HP_HardwarePlugIn_ObjectIsPropertySettable: no address");
		ThrowIfNULL(outIsSettable, CAException(kAudioHardwareIllegalOperationError), "HP_HardwarePlugIn_ObjectIsPropertySettable: no place to store return value");
		
		//	get the object state mutex
		theObjectStateMutex = HP_Object::GetObjectStateMutexByID(inObjectID);
		
		//	lock the mutex
		if(theObjectStateMutex != NULL)
		{
			theObjectStateMutexNeedsUnlocking = theObjectStateMutex->Lock();
		}
		
		//  find the object for the given ID
		HP_Object* theObject = HP_Object::GetObjectByID(inObjectID);
		ThrowIfNULL(theObject, CAException(kAudioHardwareBadObjectError), "HP_HardwarePlugIn_ObjectIsPropertySettable: no object with given ID after locking");
		
		//	do the work
		*outIsSettable = theObject->IsPropertySettable(*inAddress);
	}
	catch(const CAException& inException)
	{
		theError = inException.GetError();
	}
	catch(...)
	{
		theError = kAudioHardwareUnspecifiedError;
	}
	
	//	unlock the object state mutex if we need to
	if((theObjectStateMutex != NULL) && theObjectStateMutexNeedsUnlocking)
	{
		theObjectStateMutex->Unlock();
	}
	
	return theError;
}

static OSStatus HP_HardwarePlugIn_ObjectGetPropertyDataSize(AudioHardwarePlugInRef inSelf, AudioObjectID inObjectID, const AudioObjectPropertyAddress* inAddress, UInt32 inQualifierDataSize, const void* inQualifierData, UInt32* outDataSize)
{
	OSStatus	theError = kAudioHardwareNoError;
	CAMutex*	theObjectStateMutex = NULL;
	bool		theObjectStateMutexNeedsUnlocking = false;
	
	try
	{
		//  check the function arguments
		ThrowIfNULL(inSelf, CAException(kAudioHardwareIllegalOperationError), "HP_HardwarePlugIn_ObjectGetPropertyDataSize: no plug-in");
		ThrowIfNULL(inAddress, CAException(kAudioHardwareIllegalOperationError), "HP_HardwarePlugIn_ObjectGetPropertyDataSize: no address");
		ThrowIfNULL(outDataSize, CAException(kAudioHardwareIllegalOperationError), "HP_HardwarePlugIn_ObjectGetPropertyDataSize: no place to store return value");
		
		//	get the object state mutex
		theObjectStateMutex = HP_Object::GetObjectStateMutexByID(inObjectID);
		
		//	lock the mutex
		if(theObjectStateMutex != NULL)
		{
			theObjectStateMutexNeedsUnlocking = theObjectStateMutex->Lock();
		}
		
		//  find the object for the given ID
		HP_Object* theObject = HP_Object::GetObjectByID(inObjectID);
		ThrowIfNULL(theObject, CAException(kAudioHardwareBadObjectError), "HP_HardwarePlugIn_ObjectGetPropertyDataSize: no object with given ID after locking");
		
		//	do the work
		*outDataSize = theObject->GetPropertyDataSize(*inAddress, inQualifierDataSize, inQualifierData);
	}
	catch(const CAException& inException)
	{
		theError = inException.GetError();
	}
	catch(...)
	{
		theError = kAudioHardwareUnspecifiedError;
	}
	
	//	unlock the object state mutex if we need to
	if((theObjectStateMutex != NULL) && theObjectStateMutexNeedsUnlocking)
	{
		theObjectStateMutex->Unlock();
	}
	
	return theError;
}

static OSStatus HP_HardwarePlugIn_ObjectGetPropertyData(AudioHardwarePlugInRef inSelf, AudioObjectID inObjectID, const AudioObjectPropertyAddress* inAddress, UInt32 inQualifierDataSize, const void* inQualifierData, UInt32* ioDataSize, void* outData)
{
	OSStatus	theError = kAudioHardwareNoError;
	CAMutex*	theObjectStateMutex = NULL;
	bool		theObjectStateMutexNeedsUnlocking = false;
	
	try
	{
		//  check the function arguments
		ThrowIfNULL(inSelf, CAException(kAudioHardwareIllegalOperationError), "HP_HardwarePlugIn_ObjectGetPropertyData: no plug-in");
		ThrowIfNULL(inAddress, CAException(kAudioHardwareIllegalOperationError), "HP_HardwarePlugIn_ObjectGetPropertyData: no address");
		ThrowIfNULL(ioDataSize, CAException(kAudioHardwareIllegalOperationError), "HP_HardwarePlugIn_ObjectGetPropertyData: no info about the size of the property data");
		
		//	get the object state mutex
		theObjectStateMutex = HP_Object::GetObjectStateMutexByID(inObjectID);
		
		//	lock the mutex
		if(theObjectStateMutex != NULL)
		{
			theObjectStateMutexNeedsUnlocking = theObjectStateMutex->Lock();
		}
		
		//  find the object for the given ID
		HP_Object* theObject = HP_Object::GetObjectByID(inObjectID);
		ThrowIfNULL(theObject, CAException(kAudioHardwareBadObjectError), "HP_HardwarePlugIn_ObjectGetPropertyData: no object with given ID after locking");
		
		//	do the work
		theObject->GetPropertyData(*inAddress, inQualifierDataSize, inQualifierData, *ioDataSize, outData);
	}
	catch(const CAException& inException)
	{
		theError = inException.GetError();
	}
	catch(...)
	{
		theError = kAudioHardwareUnspecifiedError;
	}
	
	//	unlock the object state mutex if we need to
	if((theObjectStateMutex != NULL) && theObjectStateMutexNeedsUnlocking)
	{
		theObjectStateMutex->Unlock();
	}
	
	return theError;
}

static OSStatus HP_HardwarePlugIn_ObjectSetPropertyData(AudioHardwarePlugInRef inSelf, AudioObjectID inObjectID, const AudioObjectPropertyAddress* inAddress, UInt32 inQualifierDataSize, const void* inQualifierData, UInt32 inDataSize, const void* inData)
{
	OSStatus	theError = kAudioHardwareNoError;
	CAMutex*	theObjectStateMutex = NULL;
	bool		theObjectStateMutexNeedsUnlocking = false;
	
	try
	{
		//  check the function arguments
		ThrowIfNULL(inSelf, CAException(kAudioHardwareIllegalOperationError), "HP_HardwarePlugIn_ObjectSetPropertyData: no plug-in");
		ThrowIfNULL(inAddress, CAException(kAudioHardwareIllegalOperationError), "HP_HardwarePlugIn_ObjectSetPropertyData: no address");
		
		//	get the object state mutex
		theObjectStateMutex = HP_Object::GetObjectStateMutexByID(inObjectID);
		
		//	lock the mutex
		if(theObjectStateMutex != NULL)
		{
			theObjectStateMutexNeedsUnlocking = theObjectStateMutex->Lock();
		}
		
		//  find the object for the given ID
		HP_Object* theObject = HP_Object::GetObjectByID(inObjectID);
		ThrowIfNULL(theObject, CAException(kAudioHardwareBadObjectError), "HP_HardwarePlugIn_ObjectSetPropertyData: no object with given ID after locking");
		
		//	do the work
		theObject->SetPropertyData(*inAddress, inQualifierDataSize, inQualifierData, inDataSize, inData, NULL);
	}
	catch(const CAException& inException)
	{
		theError = inException.GetError();
	}
	catch(...)
	{
		theError = kAudioHardwareUnspecifiedError;
	}
	
	//	unlock the object state mutex if we need to
	if((theObjectStateMutex != NULL) && theObjectStateMutexNeedsUnlocking)
	{
		theObjectStateMutex->Unlock();
	}
	
	return theError;
}

#pragma mark	AudioDevice Operations

static OSStatus	HP_HardwarePlugIn_DeviceCreateIOProcID(AudioHardwarePlugInRef inSelf, AudioDeviceID inDeviceID, AudioDeviceIOProc inProc, void* inClientData, AudioDeviceIOProcID* outIOProcID)
{
	OSStatus theError = kAudioHardwareNoError;
	
	try
	{
		//  check the function arguments
		ThrowIfNULL(inSelf, CAException(kAudioHardwareIllegalOperationError), "HP_HardwarePlugIn_DeviceCreateIOProcID: no plug-in");
		ThrowIfNULL(inProc, CAException(kAudioHardwareIllegalOperationError), "HP_HardwarePlugIn_DeviceCreateIOProcID: no IOProc to add");
		ThrowIfNULL(outIOProcID, CAException(kAudioHardwareIllegalOperationError), "HP_HardwarePlugIn_DeviceCreateIOProcID: nowhere to put the return value");
		
		//  find the device for the given ID
		HP_Device* theDevice = HP_Object::GetDeviceByID(inDeviceID);
		ThrowIfNULL(theDevice, CAException(kAudioHardwareBadDeviceError), "HP_HardwarePlugIn_DeviceCreateIOProcID: no device with given ID");
		
		//	do the work
		*outIOProcID = theDevice->Do_CreateIOProcID(inProc, inClientData);
	}
	catch(const CAException& inException)
	{
		theError = inException.GetError();
	}
	catch(...)
	{
		theError = kAudioHardwareUnspecifiedError;
	}
	
	return theError;
}

#if	defined(kAudioHardwarePlugInInterface5ID)

static OSStatus	HP_HardwarePlugIn_DeviceCreateIOProcIDWithBlock(AudioHardwarePlugInRef inSelf, AudioDeviceIOProcID* outIOProcID, AudioDeviceID inDeviceID, dispatch_queue_t inDispatchQueue, AudioDeviceIOBlock inBlock)
{
	OSStatus theError = kAudioHardwareNoError;
	
	try
	{
		//  check the function arguments
		ThrowIfNULL(inSelf, CAException(kAudioHardwareIllegalOperationError), "HP_HardwarePlugIn_DeviceCreateIOProcIDWithBlock: no plug-in");
		ThrowIf(inBlock == 0, CAException(kAudioHardwareIllegalOperationError), "HP_HardwarePlugIn_DeviceCreateIOProcIDWithBlock: no IOBlock to add");
		ThrowIfNULL(outIOProcID, CAException(kAudioHardwareIllegalOperationError), "HP_HardwarePlugIn_DeviceCreateIOProcIDWithBlock: nowhere to put the return value");
		
		//  find the device for the given ID
		HP_Device* theDevice = HP_Object::GetDeviceByID(inDeviceID);
		ThrowIfNULL(theDevice, CAException(kAudioHardwareBadDeviceError), "HP_HardwarePlugIn_DeviceCreateIOProcIDWithBlock: no device with given ID");
		
		//	do the work
		*outIOProcID = theDevice->Do_CreateIOProcIDWithBlock(inDispatchQueue, inBlock);
	}
	catch(const CAException& inException)
	{
		theError = inException.GetError();
	}
	catch(...)
	{
		theError = kAudioHardwareUnspecifiedError;
	}
	
	return theError;
}

#endif

static OSStatus	HP_HardwarePlugIn_DeviceDestroyIOProcID(AudioHardwarePlugInRef inSelf, AudioDeviceID inDeviceID, AudioDeviceIOProcID inIOProcID)
{
	OSStatus theError = kAudioHardwareNoError;
	
	try
	{
		//  check the function arguments
		ThrowIfNULL(inSelf, CAException(kAudioHardwareIllegalOperationError), "HP_HardwarePlugIn_DeviceDestroyIOProcID: no plug-in");
		
		//  find the device for the given ID
		HP_Device* theDevice = HP_Object::GetDeviceByID(inDeviceID);
		ThrowIfNULL(theDevice, CAException(kAudioHardwareBadDeviceError), "HP_HardwarePlugIn_DeviceAddIOProc: no device with given ID");
		
		//	do the work
		theDevice->Do_DestroyIOProcID(inIOProcID);
	}
	catch(const CAException& inException)
	{
		theError = inException.GetError();
	}
	catch(...)
	{
		theError = kAudioHardwareUnspecifiedError;
	}
	
	return theError;
}

static OSStatus	HP_HardwarePlugIn_DeviceAddIOProc(AudioHardwarePlugInRef inSelf, AudioDeviceID inDeviceID, AudioDeviceIOProc inProc, void* inClientData)
{
	OSStatus theError = kAudioHardwareNoError;
	
	try
	{
		//  check the function arguments
		ThrowIfNULL(inSelf, CAException(kAudioHardwareIllegalOperationError), "HP_HardwarePlugIn_DeviceAddIOProc: no plug-in");
		ThrowIfNULL(inProc, CAException(kAudioHardwareIllegalOperationError), "HP_HardwarePlugIn_DeviceAddIOProc: no IOProc to add");
		
		//  find the device for the given ID
		HP_Device* theDevice = HP_Object::GetDeviceByID(inDeviceID);
		ThrowIfNULL(theDevice, CAException(kAudioHardwareBadDeviceError), "HP_HardwarePlugIn_DeviceAddIOProc: no device with given ID");
		
		//	do the work
		theDevice->Do_AddIOProc(inProc, inClientData);
	}
	catch(const CAException& inException)
	{
		theError = inException.GetError();
	}
	catch(...)
	{
		theError = kAudioHardwareUnspecifiedError;
	}
	
	return theError;
}

static OSStatus	HP_HardwarePlugIn_DeviceRemoveIOProc(AudioHardwarePlugInRef inSelf, AudioDeviceID inDeviceID, AudioDeviceIOProc inProc)
{
	OSStatus theError = kAudioHardwareNoError;
	
	try
	{
		//  check the function arguments
		ThrowIfNULL(inSelf, CAException(kAudioHardwareIllegalOperationError), "HP_HardwarePlugIn_DeviceRemoveIOProc: no plug-in");
		ThrowIfNULL(inProc, CAException(kAudioHardwareIllegalOperationError), "HP_HardwarePlugIn_DeviceRemoveIOProc: no IOProc to remove");
		
		//  find the device for the given ID
		HP_Device* theDevice = HP_Object::GetDeviceByID(inDeviceID);
		ThrowIfNULL(theDevice, CAException(kAudioHardwareBadDeviceError), "HP_HardwarePlugIn_DeviceRemoveIOProc: no device with given ID");
		
		//	do the work
		theDevice->Do_RemoveIOProc(inProc);
	}
	catch(const CAException& inException)
	{
		theError = inException.GetError();
	}
	catch(...)
	{
		theError = kAudioHardwareUnspecifiedError;
	}
	
	return theError;
}

static OSStatus	HP_HardwarePlugIn_DeviceStart(AudioHardwarePlugInRef inSelf, AudioDeviceID inDeviceID, AudioDeviceIOProc inProc)
{
	OSStatus theError = kAudioHardwareNoError;
	
	try
	{
		//  check the function arguments
		ThrowIfNULL(inSelf, CAException(kAudioHardwareIllegalOperationError), "HP_HardwarePlugIn_DeviceStart: no plug-in");
		
		//  find the device for the given ID
		HP_Device* theDevice = HP_Object::GetDeviceByID(inDeviceID);
		ThrowIfNULL(theDevice, CAException(kAudioHardwareBadDeviceError), "HP_HardwarePlugIn_DeviceStart: no device with given ID");
		
		//	do the work
		theDevice->Do_StartIOProc(inProc);
	}
	catch(const CAException& inException)
	{
		theError = inException.GetError();
	}
	catch(...)
	{
		theError = kAudioHardwareUnspecifiedError;
	}
	
	return theError;
}

static OSStatus	HP_HardwarePlugIn_DeviceStartAtTime(AudioHardwarePlugInRef inSelf, AudioDeviceID inDeviceID, AudioDeviceIOProc inProc, AudioTimeStamp* ioRequestedStartTime, UInt32 inFlags)
{
	OSStatus theError = kAudioHardwareNoError;
	
	try
	{
		//  check the function arguments
		ThrowIfNULL(inSelf, CAException(kAudioHardwareIllegalOperationError), "HP_HardwarePlugIn_DeviceStartAtTime: no plug-in");
		
		//  find the device for the given ID
		HP_Device* theDevice = HP_Object::GetDeviceByID(inDeviceID);
		ThrowIfNULL(theDevice, CAException(kAudioHardwareBadDeviceError), "HP_HardwarePlugIn_DeviceStartAtTime: no device with given ID");
		
		//	do the work
		if((inProc != NULL) && (ioRequestedStartTime != NULL))
		{
			theDevice->Do_StartIOProcAtTime(inProc, *ioRequestedStartTime, inFlags);
		}
		else
		{
			theDevice->Do_StartIOProc(inProc);
		}
	}
	catch(const CAException& inException)
	{
		theError = inException.GetError();
	}
	catch(...)
	{
		theError = kAudioHardwareUnspecifiedError;
	}
	
	return theError;
}

static OSStatus	HP_HardwarePlugIn_DeviceStop(AudioHardwarePlugInRef inSelf, AudioDeviceID inDeviceID, AudioDeviceIOProc inProc)
{
	OSStatus theError = kAudioHardwareNoError;
	
	try
	{
		//  check the function arguments
		ThrowIfNULL(inSelf, CAException(kAudioHardwareIllegalOperationError), "HP_HardwarePlugIn_DeviceStop: no plug-in");
		
		//  find the device for the given ID
		HP_Device* theDevice = HP_Object::GetDeviceByID(inDeviceID);
		ThrowIfNULL(theDevice, CAException(kAudioHardwareBadDeviceError), "HP_HardwarePlugIn_DeviceStop: no device with given ID");
		
		//	do the work
		theDevice->Do_StopIOProc(inProc);
	}
	catch(const CAException& inException)
	{
		theError = inException.GetError();
	}
	catch(...)
	{
		theError = kAudioHardwareUnspecifiedError;
	}
	
	return theError;
}

static OSStatus	HP_HardwarePlugIn_DeviceRead(AudioHardwarePlugInRef inSelf, AudioDeviceID inDeviceID, const AudioTimeStamp* inStartTime, AudioBufferList* outData)
{
	OSStatus theError = kAudioHardwareNoError;
	
	try
	{
		//  check the function arguments
		ThrowIfNULL(inSelf, CAException(kAudioHardwareIllegalOperationError), "HP_HardwarePlugIn_DeviceRead: no plug-in");
		ThrowIfNULL(inStartTime, CAException(kAudioHardwareIllegalOperationError), "HP_HardwarePlugIn_DeviceRead: no start time");
		ThrowIfNULL(outData, CAException(kAudioHardwareIllegalOperationError), "HP_HardwarePlugIn_DeviceRead: no place for the data");
		
		//  find the device for the given ID
		HP_Device* theDevice = HP_Object::GetDeviceByID(inDeviceID);
		ThrowIfNULL(theDevice, CAException(kAudioHardwareBadDeviceError), "HP_HardwarePlugIn_DeviceRead: no device with given ID");
		
		//	do the work
		theDevice->Read(*inStartTime, outData);
	}
	catch(const CAException& inException)
	{
		theError = inException.GetError();
	}
	catch(...)
	{
		theError = kAudioHardwareUnspecifiedError;
	}
	
	return theError;
}

static OSStatus	HP_HardwarePlugIn_DeviceGetCurrentTime(AudioHardwarePlugInRef inSelf, AudioDeviceID inDeviceID, AudioTimeStamp* outTime)
{
	OSStatus theError = kAudioHardwareNoError;
	
	try
	{
		//  check the function arguments
		ThrowIfNULL(inSelf, CAException(kAudioHardwareIllegalOperationError), "HP_HardwarePlugIn_DeviceGetCurrentTime: no plug-in");
		ThrowIfNULL(outTime, CAException(kAudioHardwareIllegalOperationError), "HP_HardwarePlugIn_DeviceGetCurrentTime: no place for the return data");
		
		//  find the device for the given ID
		HP_Device* theDevice = HP_Object::GetDeviceByID(inDeviceID);
		ThrowIfNULL(theDevice, CAException(kAudioHardwareBadDeviceError), "HP_HardwarePlugIn_DeviceGetCurrentTime: no device with given ID");
		
		//	do the work
		theDevice->SafeGetCurrentTime(*outTime);
	}
	catch(const CAException& inException)
	{
		theError = inException.GetError();
	}
	catch(...)
	{
		theError = kAudioHardwareUnspecifiedError;
	}
	
	return theError;
}

static OSStatus	HP_HardwarePlugIn_DeviceTranslateTime(AudioHardwarePlugInRef inSelf, AudioDeviceID inDeviceID, const AudioTimeStamp* inTime, AudioTimeStamp* outTime)
{
	OSStatus theError = kAudioHardwareNoError;
	
	try
	{
		//  check the function arguments
		ThrowIfNULL(inSelf, CAException(kAudioHardwareIllegalOperationError), "HP_HardwarePlugIn_DeviceTranslateTime: no plug-in");
		ThrowIfNULL(inTime, CAException(kAudioHardwareIllegalOperationError), "HP_HardwarePlugIn_DeviceTranslateTime: no input time stamp");
		ThrowIfNULL(outTime, CAException(kAudioHardwareIllegalOperationError), "HP_HardwarePlugIn_DeviceTranslateTime: no place for the return data");
		
		//  find the device for the given ID
		HP_Device* theDevice = HP_Object::GetDeviceByID(inDeviceID);
		ThrowIfNULL(theDevice, CAException(kAudioHardwareBadDeviceError), "HP_HardwarePlugIn_DeviceTranslateTime: no device with given ID");
		
		//	do the work
		theDevice->TranslateTime(*inTime, *outTime);
	}
	catch(const CAException& inException)
	{
		theError = inException.GetError();
	}
	catch(...)
	{
		theError = kAudioHardwareUnspecifiedError;
	}
	
	return theError;
}

static OSStatus	HP_HardwarePlugIn_DeviceGetNearestStartTime(AudioHardwarePlugInRef inSelf, AudioDeviceID inDeviceID, AudioTimeStamp* ioRequestedStartTime, UInt32 inFlags)
{
	OSStatus theError = kAudioHardwareNoError;
	
	try
	{
		//  check the function arguments
		ThrowIfNULL(inSelf, CAException(kAudioHardwareIllegalOperationError), "HP_HardwarePlugIn_DeviceGetNearestStartTime: no plug-in");
		ThrowIfNULL(ioRequestedStartTime, CAException(kAudioHardwareIllegalOperationError), "HP_HardwarePlugIn_DeviceGetNearestStartTime: no time stamp");
		
		//  find the device for the given ID
		HP_Device* theDevice = HP_Object::GetDeviceByID(inDeviceID);
		ThrowIfNULL(theDevice, CAException(kAudioHardwareBadDeviceError), "HP_HardwarePlugIn_DeviceGetNearestStartTime: no device with given ID");
		
		//	do the work
		theDevice->GetNearestStartTime(*ioRequestedStartTime, inFlags);
	}
	catch(const CAException& inException)
	{
		theError = inException.GetError();
	}
	catch(...)
	{
		theError = kAudioHardwareUnspecifiedError;
	}
	
	return theError;
}

static OSStatus	HP_HardwarePlugIn_DeviceGetPropertyInfo(AudioHardwarePlugInRef inSelf, AudioDeviceID inDeviceID, UInt32 inChannel, Boolean isInput, AudioDevicePropertyID inPropertyID, UInt32* outSize, Boolean* outWritable)
{
	OSStatus	theError = kAudioHardwareNoError;
	CAMutex*	theObjectStateMutex = NULL;
	bool		theObjectStateMutexNeedsUnlocking = false;
	
	try
	{
		//  check the function arguments
		ThrowIfNULL(inSelf, CAException(kAudioHardwareIllegalOperationError), "HP_HardwarePlugIn_DeviceGetPropertyInfo: no plug-in");
		
		//	get the object state mutex
		theObjectStateMutex = HP_Object::GetObjectStateMutexByID(inDeviceID);
		
		//	lock the mutex
		if(theObjectStateMutex != NULL)
		{
			theObjectStateMutexNeedsUnlocking = theObjectStateMutex->Lock();
		}
		
		//  find the device for the given ID
		HP_Device* theDevice = HP_Object::GetDeviceByID(inDeviceID);
		ThrowIfNULL(theDevice, CAException(kAudioHardwareBadObjectError), "HP_HardwarePlugIn_DeviceGetPropertyInfo: no device with given ID after locking");
		
		//  construct a property address
		CAPropertyAddress theAddress(inPropertyID, isInput == 0 ? kAudioDevicePropertyScopeOutput : kAudioDevicePropertyScopeInput, inChannel);
		
		//	do the work
		ThrowIf(!theDevice->HasProperty(theAddress), CAException(kAudioHardwareUnknownPropertyError), "HP_HardwarePlugIn_DeviceGetPropertyInfo: no such property");
		if(outSize != NULL)
		{
			*outSize = theDevice->GetPropertyDataSize(theAddress, 0, NULL);
		}
		if(outWritable != NULL)
		{
			*outWritable = theDevice->IsPropertySettable(theAddress) ? 1 : 0;
		}
	}
	catch(const CAException& inException)
	{
		theError = inException.GetError();
	}
	catch(...)
	{
		theError = kAudioHardwareUnspecifiedError;
	}
	
	//	unlock the object state mutex if we need to
	if((theObjectStateMutex != NULL) && theObjectStateMutexNeedsUnlocking)
	{
		theObjectStateMutex->Unlock();
	}
	
	return theError;
}

static OSStatus	HP_HardwarePlugIn_DeviceGetProperty(AudioHardwarePlugInRef inSelf, AudioDeviceID inDeviceID, UInt32 inChannel, Boolean isInput, AudioDevicePropertyID inPropertyID, UInt32* ioPropertyDataSize, void* outPropertyData)
{
	OSStatus	theError = kAudioHardwareNoError;
	CAMutex*	theObjectStateMutex = NULL;
	bool		theObjectStateMutexNeedsUnlocking = false;
	
	try
	{
		//  check the function arguments
		ThrowIfNULL(inSelf, CAException(kAudioHardwareIllegalOperationError), "HP_HardwarePlugIn_DeviceGetProperty: no plug-in");
		ThrowIfNULL(ioPropertyDataSize, CAException(kAudioHardwareIllegalOperationError), "HP_HardwarePlugIn_DeviceGetProperty: no data size");
		
		//	get the object state mutex
		theObjectStateMutex = HP_Object::GetObjectStateMutexByID(inDeviceID);
		
		//	lock the mutex
		if(theObjectStateMutex != NULL)
		{
			theObjectStateMutexNeedsUnlocking = theObjectStateMutex->Lock();
		}
		
		//  find the device for the given ID
		HP_Device* theDevice = HP_Object::GetDeviceByID(inDeviceID);
		ThrowIfNULL(theDevice, CAException(kAudioHardwareBadObjectError), "HP_HardwarePlugIn_DeviceGetProperty: no device with given ID after locking");
		
		//  construct a property address
		CAPropertyAddress theAddress(inPropertyID, isInput == 0 ? kAudioDevicePropertyScopeOutput : kAudioDevicePropertyScopeInput, inChannel);
		
		//	do the work
		theDevice->GetPropertyData(theAddress, 0, NULL, *ioPropertyDataSize, outPropertyData);
	}
	catch(const CAException& inException)
	{
		theError = inException.GetError();
	}
	catch(...)
	{
		theError = kAudioHardwareUnspecifiedError;
	}
	
	//	unlock the object state mutex if we need to
	if((theObjectStateMutex != NULL) && theObjectStateMutexNeedsUnlocking)
	{
		theObjectStateMutex->Unlock();
	}
	
	return theError;
}

static OSStatus	HP_HardwarePlugIn_DeviceSetProperty(AudioHardwarePlugInRef inSelf, AudioDeviceID inDeviceID, const AudioTimeStamp* inWhen, UInt32 inChannel, Boolean isInput, AudioDevicePropertyID inPropertyID, UInt32 inPropertyDataSize, const void* inPropertyData)
{
	OSStatus	theError = kAudioHardwareNoError;
	CAMutex*	theObjectStateMutex = NULL;
	bool		theObjectStateMutexNeedsUnlocking = false;
	
	try
	{
		//  check the function arguments
		ThrowIfNULL(inSelf, CAException(kAudioHardwareIllegalOperationError), "HP_HardwarePlugIn_DeviceSetProperty: no plug-in");
		
		//	get the object state mutex
		theObjectStateMutex = HP_Object::GetObjectStateMutexByID(inDeviceID);
		
		//	lock the mutex
		if(theObjectStateMutex != NULL)
		{
			theObjectStateMutexNeedsUnlocking = theObjectStateMutex->Lock();
		}
		
		//  find the device for the given ID
		HP_Device* theDevice = HP_Object::GetDeviceByID(inDeviceID);
		ThrowIfNULL(theDevice, CAException(kAudioHardwareBadObjectError), "HP_HardwarePlugIn_DeviceSetProperty: no device with given ID after locking");
		
		//  construct a property address
		CAPropertyAddress theAddress(inPropertyID, isInput == 0 ? kAudioDevicePropertyScopeOutput : kAudioDevicePropertyScopeInput, inChannel);
		
		//	do the work
		theDevice->SetPropertyData(theAddress, 0, NULL, inPropertyDataSize, inPropertyData, inWhen);
	}
	catch(const CAException& inException)
	{
		theError = inException.GetError();
	}
	catch(...)
	{
		theError = kAudioHardwareUnspecifiedError;
	}
	
	//	unlock the object state mutex if we need to
	if((theObjectStateMutex != NULL) && theObjectStateMutexNeedsUnlocking)
	{
		theObjectStateMutex->Unlock();
	}
	
	return theError;
}

#pragma mark	AudioStream Operations

static OSStatus	HP_HardwarePlugIn_StreamGetPropertyInfo(AudioHardwarePlugInRef inSelf, AudioStreamID inStreamID, UInt32 inChannel, AudioDevicePropertyID inPropertyID, UInt32* outSize, Boolean* outWritable)
{
	OSStatus	theError = kAudioHardwareNoError;
	CAMutex*	theObjectStateMutex = NULL;
	bool		theObjectStateMutexNeedsUnlocking = false;
	
	try
	{
		//  check the function arguments
		ThrowIfNULL(inSelf, CAException(kAudioHardwareIllegalOperationError), "HP_HardwarePlugIn_StreamGetPropertyInfo: no plug-in");
		
		//	get the object state mutex
		theObjectStateMutex = HP_Object::GetObjectStateMutexByID(inStreamID);
		
		//	lock the mutex
		if(theObjectStateMutex != NULL)
		{
			theObjectStateMutexNeedsUnlocking = theObjectStateMutex->Lock();
		}
		
		//  find the stream for the given ID
		HP_Stream* theStream = HP_Object::GetStreamByID(inStreamID);
		ThrowIfNULL(theStream, CAException(kAudioHardwareBadObjectError), "HP_HardwarePlugIn_StreamGetPropertyInfo: no device with given ID after locking");
		
		//  construct a property address
		CAPropertyAddress theAddress(inPropertyID, kAudioObjectPropertyScopeGlobal, inChannel);
		
		//	do the work
		ThrowIf(!theStream->HasProperty(theAddress), CAException(kAudioHardwareUnknownPropertyError), "HP_HardwarePlugIn_StreamGetPropertyInfo: no such property");
		if(outSize != NULL)
		{
			*outSize = theStream->GetPropertyDataSize(theAddress, 0, NULL);
		}
		if(outWritable != NULL)
		{
			*outWritable = theStream->IsPropertySettable(theAddress) ? 1 : 0;
		}
	}
	catch(const CAException& inException)
	{
		theError = inException.GetError();
	}
	catch(...)
	{
		theError = kAudioHardwareUnspecifiedError;
	}
	
	//	unlock the object state mutex if we need to
	if((theObjectStateMutex != NULL) && theObjectStateMutexNeedsUnlocking)
	{
		theObjectStateMutex->Unlock();
	}
	
	return theError;
}

static OSStatus	HP_HardwarePlugIn_StreamGetProperty(AudioHardwarePlugInRef inSelf, AudioStreamID inStreamID, UInt32 inChannel, AudioDevicePropertyID inPropertyID, UInt32* ioPropertyDataSize, void* outPropertyData)
{
	OSStatus	theError = kAudioHardwareNoError;
	CAMutex*	theObjectStateMutex = NULL;
	bool		theObjectStateMutexNeedsUnlocking = false;
	
	try
	{
		//  check the function arguments
		ThrowIfNULL(inSelf, CAException(kAudioHardwareIllegalOperationError), "HP_HardwarePlugIn_StreamGetProperty: no plug-in");
		ThrowIfNULL(ioPropertyDataSize, CAException(kAudioHardwareIllegalOperationError), "HP_HardwarePlugIn_StreamGetProperty: no data size");
		
		//	get the object state mutex
		theObjectStateMutex = HP_Object::GetObjectStateMutexByID(inStreamID);
		
		//	lock the mutex
		if(theObjectStateMutex != NULL)
		{
			theObjectStateMutexNeedsUnlocking = theObjectStateMutex->Lock();
		}
		
		//  find the stream for the given ID
		HP_Stream* theStream = HP_Object::GetStreamByID(inStreamID);
		ThrowIfNULL(theStream, CAException(kAudioHardwareBadObjectError), "HP_HardwarePlugIn_StreamGetProperty: no device with given ID after locking");
		
		//  construct a property address
		CAPropertyAddress theAddress(inPropertyID, kAudioObjectPropertyScopeGlobal, inChannel);
		
		//	do the work
		theStream->GetPropertyData(theAddress, 0, NULL, *ioPropertyDataSize, outPropertyData);
	}
	catch(const CAException& inException)
	{
		theError = inException.GetError();
	}
	catch(...)
	{
		theError = kAudioHardwareUnspecifiedError;
	}
	
	//	unlock the object state mutex if we need to
	if((theObjectStateMutex != NULL) && theObjectStateMutexNeedsUnlocking)
	{
		theObjectStateMutex->Unlock();
	}
	
	return theError;
}

static OSStatus	HP_HardwarePlugIn_StreamSetProperty(AudioHardwarePlugInRef inSelf, AudioStreamID inStreamID, const AudioTimeStamp* inWhen, UInt32 inChannel, AudioDevicePropertyID inPropertyID, UInt32 inPropertyDataSize, const void* inPropertyData)
{
	OSStatus theError = kAudioHardwareNoError;
	CAMutex*	theObjectStateMutex = NULL;
	bool		theObjectStateMutexNeedsUnlocking = false;
	
	try
	{
		//  check the function arguments
		ThrowIfNULL(inSelf, CAException(kAudioHardwareIllegalOperationError), "HP_HardwarePlugIn_StreamSetProperty: no plug-in");
		
		//	get the object state mutex
		theObjectStateMutex = HP_Object::GetObjectStateMutexByID(inStreamID);
		
		//	lock the mutex
		if(theObjectStateMutex != NULL)
		{
			theObjectStateMutexNeedsUnlocking = theObjectStateMutex->Lock();
		}
		
		//  find the stream for the given ID
		HP_Stream* theStream = HP_Object::GetStreamByID(inStreamID);
		ThrowIfNULL(theStream, CAException(kAudioHardwareBadObjectError), "HP_HardwarePlugIn_StreamSetProperty: no device with given ID after locking");
		
		//  construct a property address
		CAPropertyAddress theAddress(inPropertyID, kAudioObjectPropertyScopeGlobal, inChannel);
		
		//	do the work
		theStream->SetPropertyData(theAddress, 0, NULL, inPropertyDataSize, inPropertyData, inWhen);
	}
	catch(const CAException& inException)
	{
		theError = inException.GetError();
	}
	catch(...)
	{
		theError = kAudioHardwareUnspecifiedError;
	}
	
	//	unlock the object state mutex if we need to
	if((theObjectStateMutex != NULL) && theObjectStateMutexNeedsUnlocking)
	{
		theObjectStateMutex->Unlock();
	}
	
	return theError;
}

AudioHardwarePlugInInterface	HP_HardwarePlugIn::sInterface = 
{
	//	Padding for COM
	NULL,
	
	//	IUnknown Routines
	(HRESULT (*)(void*, CFUUIDBytes, void**))HP_HardwarePlugIn_QueryInterface,
	(ULONG (*)(void*))HP_HardwarePlugIn_AddRef,
	(ULONG (*)(void*))HP_HardwarePlugIn_Release,
	
	//	HAL Plug-In Routines
	HP_HardwarePlugIn_Initialize,
	HP_HardwarePlugIn_Teardown,
	HP_HardwarePlugIn_DeviceAddIOProc,
	HP_HardwarePlugIn_DeviceRemoveIOProc,
	HP_HardwarePlugIn_DeviceStart,
	HP_HardwarePlugIn_DeviceStop,
	HP_HardwarePlugIn_DeviceRead,
	HP_HardwarePlugIn_DeviceGetCurrentTime,
	HP_HardwarePlugIn_DeviceTranslateTime,
	HP_HardwarePlugIn_DeviceGetPropertyInfo,
	HP_HardwarePlugIn_DeviceGetProperty,
	HP_HardwarePlugIn_DeviceSetProperty,
	HP_HardwarePlugIn_StreamGetPropertyInfo,
	HP_HardwarePlugIn_StreamGetProperty,
	HP_HardwarePlugIn_StreamSetProperty,
	HP_HardwarePlugIn_DeviceStartAtTime,
	HP_HardwarePlugIn_DeviceGetNearestStartTime,
	HP_HardwarePlugIn_InitializeWithObjectID,
	HP_HardwarePlugIn_ObjectShow,
	HP_HardwarePlugIn_ObjectHasProperty,
	HP_HardwarePlugIn_ObjectIsPropertySettable,
	HP_HardwarePlugIn_ObjectGetPropertyDataSize,
	HP_HardwarePlugIn_ObjectGetPropertyData,
	HP_HardwarePlugIn_ObjectSetPropertyData,
	HP_HardwarePlugIn_DeviceCreateIOProcID,
	HP_HardwarePlugIn_DeviceDestroyIOProcID
#if	defined(kAudioHardwarePlugInInterface5ID)
	,HP_HardwarePlugIn_DeviceCreateIOProcIDWithBlock
#endif
};

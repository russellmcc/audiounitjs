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
#if !defined(__HP_DeviceCommands_h__)
#define __HP_DeviceCommands_h__

//==================================================================================================
//	Includes
//==================================================================================================

//	Super Class Includes
#include "HP_Command.h"

//	System Includes
#include <CoreAudio/AudioHardware.h>

//==================================================================================================
//	HP_DeviceCommands
//==================================================================================================

class	HP_AddIOProcCommand
:
	public	HP_Command
{

//	Constants
public:
	enum
	{
						kID	= '+iop'
	};

//	Construction/Destruction
public:
						HP_AddIOProcCommand(AudioDeviceIOProc inProc, void* inClientData);
	virtual				~HP_AddIOProcCommand();

//	Operations
public:
	virtual void		Execute(HP_Device* inDevice);

private:
	AudioDeviceIOProc	mProc;
	void*				mClientData;

};

class	HP_RemoveIOProcCommand
:
	public	HP_Command
{

//	Constants
public:
	enum
	{
						kID	= '-iop'
	};

//	Construction/Destruction
public:
						HP_RemoveIOProcCommand(AudioDeviceIOProc inProc);
	virtual				~HP_RemoveIOProcCommand();

//	Operations
public:
	virtual void		Execute(HP_Device* inDevice);

private:
	AudioDeviceIOProc	mProc;

};

class	HP_ChangeIOProcStreamUsageCommand
:
	public	HP_Command
{

//	Constants
public:
	enum
	{
						kID	= 'suse'
	};

//	Construction/Destruction
public:
						HP_ChangeIOProcStreamUsageCommand(AudioDeviceIOProc inProc, bool inIsInput, UInt32 inNumberStreams, const UInt32 inStreamUsage[]);
	virtual				~HP_ChangeIOProcStreamUsageCommand();

//	Operations
public:
	virtual void		Execute(HP_Device* inDevice);

private:
	AudioDeviceIOProc	mProc;
	bool				mIsInput;
	UInt32				mNumberStreams;
	bool*				mStreamUsage;

};

class	HP_StartIOProcCommand
:
	public	HP_Command
{

//	Constants
public:
	enum
	{
						kID	= '>iop'
	};

//	Construction/Destruction
public:
						HP_StartIOProcCommand(AudioDeviceIOProc inProc);
						HP_StartIOProcCommand(AudioDeviceIOProc inProc, const AudioTimeStamp& inRequestedStartTime, UInt32 inFlags);
	virtual				~HP_StartIOProcCommand();

//	Operations
public:
	virtual void		Execute(HP_Device* inDevice);

private:
	AudioDeviceIOProc	mProc;
	AudioTimeStamp		mStartTime;
	UInt32				mStartTimeFlags;
	bool				mUseStartTime;

};

class	HP_StopIOProcCommand
:
	public	HP_Command
{

//	Constants
public:
	enum
	{
						kID	= '<iop'
	};

//	Construction/Destruction
public:
						HP_StopIOProcCommand(AudioDeviceIOProc inProc);
	virtual				~HP_StopIOProcCommand();

//	Operations
public:
	virtual void		Execute(HP_Device* inDevice);

private:
	AudioDeviceIOProc	mProc;

};

class	HP_StopAllIOProcsCommand
:
	public	HP_Command
{

//	Constants
public:
	enum
	{
						kID	= 'saio'
	};

//	Construction/Destruction
public:
						HP_StopAllIOProcsCommand();
	virtual				~HP_StopAllIOProcsCommand();

//	Operations
public:
	virtual void		Execute(HP_Device* inDevice);

};

class	HP_ChangeBufferSizeCommand
:
	public	HP_Command
{

//	Constants
public:
	enum
	{
						kID	= 'bufs'
	};

//	Construction/Destruction
public:
						HP_ChangeBufferSizeCommand(UInt32 inNewBufferFrameSize, bool inSendNotifications);
	virtual				~HP_ChangeBufferSizeCommand();

//	Operations
public:
	virtual void		Execute(HP_Device* inDevice);
	UInt32				GetNewBufferFrameSize() const { return mNewBufferFrameSize; }

private:
	UInt32				mNewBufferFrameSize;
	bool				mSendNotifications;

};

#endif

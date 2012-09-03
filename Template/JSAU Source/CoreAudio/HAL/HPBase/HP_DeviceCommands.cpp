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
#include "HP_DeviceCommands.h"

//  Local Includes
#include "HP_Device.h"

//  PublicUtility Includes
#include "CAAudioTimeStamp.h"

//==================================================================================================
//	HP_DeviceCommands
//==================================================================================================

HP_AddIOProcCommand::HP_AddIOProcCommand(AudioDeviceIOProc inProc, void* inClientData)
:
	HP_Command(kID),
	mProc(inProc),
	mClientData(inClientData)
{
}

HP_AddIOProcCommand::~HP_AddIOProcCommand()
{
}

void	HP_AddIOProcCommand::Execute(HP_Device* inDevice)
{
	if(inDevice != NULL)
	{
		inDevice->AddIOProc(mProc, mClientData);
	}
}

HP_RemoveIOProcCommand::HP_RemoveIOProcCommand(AudioDeviceIOProc inProc)
:
	HP_Command(kID),
	mProc(inProc)
{
}

HP_RemoveIOProcCommand::~HP_RemoveIOProcCommand()
{
}

void	HP_RemoveIOProcCommand::Execute(HP_Device* inDevice)
{
	if(inDevice != NULL)
	{
		inDevice->RemoveIOProc(mProc);
	}
}

HP_ChangeIOProcStreamUsageCommand::HP_ChangeIOProcStreamUsageCommand(AudioDeviceIOProc inProc, bool inIsInput, UInt32 inNumberStreams, const UInt32 inStreamUsage[])
:
	HP_Command(kID),
	mProc(inProc),
	mIsInput(inIsInput),
	mNumberStreams(inNumberStreams),
	mStreamUsage(NULL)
{
	mStreamUsage = new bool[mNumberStreams];
	for(UInt32 theStreamIndex = 0; theStreamIndex < mNumberStreams; ++theStreamIndex)
	{
		mStreamUsage[theStreamIndex] = (inStreamUsage[theStreamIndex] != 0);
	}
}

HP_ChangeIOProcStreamUsageCommand::~HP_ChangeIOProcStreamUsageCommand()
{
	delete[] mStreamUsage;
}

void	HP_ChangeIOProcStreamUsageCommand::Execute(HP_Device* inDevice)
{
	if(inDevice != NULL)
	{
		inDevice->SetIOProcStreamUsage(mProc, mIsInput, mNumberStreams, mStreamUsage);
	}
}

HP_StartIOProcCommand::HP_StartIOProcCommand(AudioDeviceIOProc inProc)
:
	HP_Command(kID),
	mProc(inProc),
	mStartTime(CAAudioTimeStamp::kZero),
	mStartTimeFlags(0),
	mUseStartTime(false)
{
}

HP_StartIOProcCommand::HP_StartIOProcCommand(AudioDeviceIOProc inProc, const AudioTimeStamp& inRequestedStartTime, UInt32 inStartTimeFlags)
:
	HP_Command(kID),
	mProc(inProc),
	mStartTime(inRequestedStartTime),
	mStartTimeFlags(inStartTimeFlags),
	mUseStartTime(true)
{
}

HP_StartIOProcCommand::~HP_StartIOProcCommand()
{
}

void	HP_StartIOProcCommand::Execute(HP_Device* inDevice)
{
	if(inDevice != NULL)
	{
		if(mUseStartTime)
		{
			inDevice->StartIOProcAtTime(mProc, mStartTime, mStartTimeFlags);
		}
		else
		{
			inDevice->StartIOProc(mProc);
		}
	}
}

HP_StopIOProcCommand::HP_StopIOProcCommand(AudioDeviceIOProc inProc)
:
	HP_Command(kID),
	mProc(inProc)
{
}

HP_StopIOProcCommand::~HP_StopIOProcCommand()
{
}

void	HP_StopIOProcCommand::Execute(HP_Device* inDevice)
{
	if(inDevice != NULL)
	{
		inDevice->StopIOProc(mProc);
	}
}

HP_StopAllIOProcsCommand::HP_StopAllIOProcsCommand()
:
	HP_Command(kID)
{
}

HP_StopAllIOProcsCommand::~HP_StopAllIOProcsCommand()
{
}

void	HP_StopAllIOProcsCommand::Execute(HP_Device* inDevice)
{
	if(inDevice != NULL)
	{
		inDevice->StopAllIOProcs();
	}
}

HP_ChangeBufferSizeCommand::HP_ChangeBufferSizeCommand(UInt32 inNewBufferFrameSize, bool inSendNotifications)
:
	HP_Command(kID),
	mNewBufferFrameSize(inNewBufferFrameSize),
	mSendNotifications(inSendNotifications)
{
}

HP_ChangeBufferSizeCommand::~HP_ChangeBufferSizeCommand()
{
}

void	HP_ChangeBufferSizeCommand::Execute(HP_Device* inDevice)
{
	if(inDevice != NULL)
	{
		inDevice->SetIOBufferFrameSize(mNewBufferFrameSize, mSendNotifications);
	}
}

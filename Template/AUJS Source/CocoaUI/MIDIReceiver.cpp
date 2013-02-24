#include "MIDIReceiver.h"
#include <cmath>

// this uses basic coreMIDI stuff to grab all incoming MIDI
// and send it to the AU.

namespace
{
    void throwOnErr(OSStatus a)
    {
        if(a != noErr)
            throw a;
    }
    
    template<typename t>
    void throwIfNull(t tt)
    {
        if(not tt)
            throw static_cast<OSStatus>(-9);
    }

    void ReceiverNotifyProc(const MIDINotification* message, void* refCon)
    {
        MIDIReceiver* that = reinterpret_cast<MIDIReceiver*>(refCon);
        const MIDIObjectAddRemoveNotification* addRemove = reinterpret_cast<const MIDIObjectAddRemoveNotification*>(message);
        switch (message->messageID)
        {
            case kMIDIMsgObjectAdded:
                if (addRemove->childType == kMIDIObjectType_Source)
                    MIDIPortConnectSource(that->getPort(), reinterpret_cast<MIDIEndpointRef>(addRemove->child), 0);
                break;
            case kMIDIMsgObjectRemoved:
                if (addRemove->childType == kMIDIObjectType_Source)
                    MIDIPortDisconnectSource(that->getPort(), reinterpret_cast<MIDIEndpointRef>(addRemove->child));
                break;
        }
    }
    
    void ReceiverReadProc(const MIDIPacketList* pktlist, void* readProcRefCon, void* srcConnRefCon)
    {
        AudioUnit au = reinterpret_cast<MIDIReceiver*>(readProcRefCon)->getAudioUnit();
        for(UInt32 i = 0; i < pktlist->numPackets; ++i)
        {
            // skip all packets longer than 3.
            if(pktlist->packet[i].length <= 3)
            {
                Byte data[3] = {0};
                memcpy(data, pktlist->packet[i].data, std::min(static_cast<UInt16>(3), pktlist->packet[i].length));
                MusicDeviceMIDIEvent(au, data[0], data[1], data[2], 0);
            }
        }
    }
};

MIDIReceiver::MIDIReceiver(AudioUnit au) : mAU(au), mClient(0), mPort(0)
{
    throwOnErr(MIDIClientCreate(CFSTR("#NAME MIDI Client"), ReceiverNotifyProc, this, &mClient));
    throwIfNull(mClient);
    
    throwOnErr(MIDIInputPortCreate(mClient, CFSTR("#NAME MIDI Port"), ReceiverReadProc, this, &mPort));
    throwIfNull(mPort);
    
    // hook up everything!
    for(ItemCount i = 0; i < MIDIGetNumberOfSources(); ++i)
    {
        MIDIEndpointRef endpoint = MIDIGetSource(i);
        MIDIPortConnectSource(mPort, endpoint, 0);
    }
};

MIDIReceiver::~MIDIReceiver()
{
    if(mPort)
      MIDIPortDispose(mPort);
    
    if(mClient)
      MIDIClientDispose(mClient);
};

#ifndef __#PROJNAME__MIDIReceiver__
#define __#PROJNAME__MIDIReceiver__

#import <CoreMIDI/CoreMIDI.h>

// forwards all CoreMIDI messages to
// the given audio unit.
class MIDIReceiver
{
public:
  MIDIReceiver(AudioUnit u);
  ~MIDIReceiver();
  
  AudioUnit getAudioUnit() {return mAU;}
  MIDIPortRef getPort() {return mPort;}
private:
  // can't copy this.
  MIDIReceiver(const MIDIReceiver& rhs);
  MIDIReceiver& operator=(const MIDIReceiver& rhs);

  AudioUnit mAU;
  MIDIClientRef mClient;
  MIDIPortRef mPort;
  
};

#endif

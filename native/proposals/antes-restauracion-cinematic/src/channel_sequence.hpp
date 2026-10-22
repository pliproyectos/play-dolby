#pragma once
#include <cstdint>
namespace bridge {
// UI-thread scheduler: never opens streams, and cannot schedule after cancel().
class ChannelSequence {
 unsigned count_=0,next_=0;
 bool active_=false,playing_=false;
 uint64_t due_=0;
public:
 bool active()const{return active_;}
 void cancel(){active_=false;playing_=false;count_=next_=0;due_=0;}
 void begin(unsigned count){cancel();if(count==2||count==6||count==8){count_=count;active_=true;}}
 int takeReady(uint64_t now){
  if(!active_||playing_||now<due_)return -1;
  playing_=true;return int(next_++);
 }
 void toneFinished(uint64_t now){
  if(!active_||!playing_)return;
  playing_=false;
  if(next_==count_)cancel();else due_=now+700;
 }
};
}

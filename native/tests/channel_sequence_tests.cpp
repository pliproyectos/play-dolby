#include "../src/channel_sequence.hpp"
#include <cassert>
#include <iostream>
int main(){
 for(unsigned count:{2u,6u,8u}){
  bridge::ChannelSequence sequence;sequence.begin(count);
  uint64_t time=0;
  for(unsigned channel=0;channel<count;++channel){
   assert(sequence.takeReady(time)==int(channel));
   assert(sequence.takeReady(time+100)==-1);
   time+=2000;sequence.toneFinished(time);
   assert(sequence.takeReady(time+699)==-1);
   time+=700;
  }
  assert(!sequence.active());assert(sequence.takeReady(time+10000)==-1);
 }
 bridge::ChannelSequence sequence;
 sequence.begin(8);assert(sequence.takeReady(0)==0);sequence.cancel();
 sequence.toneFinished(2000);assert(sequence.takeReady(5000)==-1);
 sequence.begin(6);assert(sequence.takeReady(0)==0);sequence.toneFinished(2000);
 sequence.cancel();assert(sequence.takeReady(5000)==-1);
 sequence.begin(3);assert(!sequence.active());
 std::cout<<"PASS: 2/6/8-channel order, inter-tone delay, cancel during tone/gap, no restart.\n";
}

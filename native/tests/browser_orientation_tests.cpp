#include <winsock2.h>
#include <windows.h>
#include <iostream>
#include <cassert>
#include "../src/browser_audio.hpp"
#include "../src/orientation.hpp"
int main(int argc,char**){
 if(argc>1){browserAudio::Receiver receiver;receiver.start();std::cout<<"READY"<<std::endl;Sleep(1500);float output[4096*8]{};receiver.mix(output,4096,8,96000);bool signal=false;for(float value:output)signal|=std::abs(value)>.01f;assert(signal);std::cout<<"PASS PCM48k -> mix96k"<<std::endl;receiver.stop();return 0;}
 // Distinct channel signals catch duplicated voice and accidental changes to other speakers.
 for(unsigned source=0;source<8;++source){
  bridge::Orientation correction;
  for(bool enabled:{true,false}){
   correction.swapCenterRight=enabled;
   float frame[8]{};
   for(int n=0;n<1000;++n){std::fill_n(frame,8,0.f);frame[source]=.1f;correction.process(frame,1,8,48000);}
   for(unsigned dest=0;dest<8;++dest){
    unsigned original=dest;
    if(enabled){if(dest==1)original=2;else if(dest==2)original=1;else if(dest==7)original=6;}
    assert(std::abs(frame[dest]-(original==source?.1f:0.f))<1e-6f);
   }
  }
 }
 for(int degrees:{0,90,-90,180}){
  bridge::Orientation actual,expected;
  actual.degrees=degrees;expected.degrees=degrees;
  for(bool active:{true,false}){
   actual.swapCenterRight=active;
   for(int frame=0;frame<1024;++frame){
    float a[]={.01f,.12f,.23f,.34f,.45f,.56f,.67f,.78f},b[8];
    std::copy_n(a,8,b);if(active){std::swap(b[1],b[2]);b[7]=b[6];}
    actual.process(a,1,8,48000);expected.process(b,1,8,48000);
    if(frame==1023)for(unsigned c=0;c<8;++c)assert(std::abs(a[c]-b[c])<1e-5f);
   }
  }
 }
 for(int degrees:{0,90,-90,180}){
  for(unsigned channel=0;channel<8;++channel){bridge::Orientation rotation;rotation.degrees=degrees;float buffer[8]{};for(int i=0;i<1000;++i){std::fill_n(buffer,8,0.f);buffer[channel]=.1f;rotation.process(buffer,1,8,48000);}float power=0;for(float value:buffer){assert(std::isfinite(value));power+=value*value;}assert(std::abs(power-.01f)<.00001f);if(channel==3)assert(buffer[3]==.1f);if(degrees==0)assert(std::abs(buffer[channel]-.1f)<.000001f);if(channel==7 && degrees==90)assert(buffer[4]>.06f && buffer[5]>.06f);if(channel==2 && degrees==180)assert(buffer[4]>.06f && buffer[5]>.06f);}
 }
 std::cout<<"PASS identity, +/-90/180, channel energy, LFE unchanged, rear interpolation"<<std::endl;
}

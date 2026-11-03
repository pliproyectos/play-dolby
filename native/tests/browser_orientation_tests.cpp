#include <winsock2.h>
#include <windows.h>
#include <iostream>
#include <cassert>
#include "../src/browser_audio.hpp"
#include "../src/orientation.hpp"
int main(int argc,char**){
 if(argc>1){browserAudio::Receiver receiver;receiver.start();std::cout<<"READY"<<std::endl;Sleep(1500);float output[4096*8]{};receiver.mix(output,4096,8,96000);bool signal=false;for(float value:output)signal|=std::abs(value)>.01f;assert(signal);std::cout<<"PASS PCM48k -> mix96k"<<std::endl;receiver.stop();return 0;}
 for(int degrees:{0,90,-90,180}){
  for(unsigned channel=0;channel<8;++channel){bridge::Orientation rotation;rotation.degrees=degrees;float buffer[8]{};for(int i=0;i<1000;++i){std::fill_n(buffer,8,0.f);buffer[channel]=.1f;rotation.process(buffer,1,8,48000);}float power=0;for(float value:buffer){assert(std::isfinite(value));power+=value*value;}assert(std::abs(power-.01f)<.00001f);if(channel==3)assert(buffer[3]==.1f);if(degrees==0)assert(std::abs(buffer[channel]-.1f)<.000001f);if(channel==7 && degrees==90)assert(buffer[4]>.06f && buffer[5]>.06f);if(channel==2 && degrees==180)assert(buffer[4]>.06f && buffer[5]>.06f);}
 }
 std::cout<<"PASS identity, +/-90/180, channel energy, LFE unchanged, rear interpolation"<<std::endl;
}

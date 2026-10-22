#include "../src/bridge_core.hpp"
#include <iostream>
#include <stdexcept>
#define REQUIRE(x) if(!(x))throw std::runtime_error(#x)
int main(){
    bridge::Core c;
    std::vector<float> input(4096*8),output(256*8);
    for(unsigned i=0;i<4096;++i)for(unsigned j=0;j<8;++j)input[i*8+j]=float(j+1)/10;
    c.gain=1;c.route[0]=7;c.route[7]=-1;
    for(unsigned b=0;b<16;++b)c.capture(input.data()+b*256*8,256,8);c.render(output.data(),256,8);
    REQUIRE(std::abs(output[0]-.8f)<1e-6);REQUIRE(output[7]==0);
    for(unsigned j=1;j<7;++j)REQUIRE(std::abs(output[j]-float(j+1)/10)<1e-6);
    c.reset();c.render(output.data(),256,8);for(auto f:output)REQUIRE(f==0);
    c.route[0]=0;input[0]=std::nanf("");for(unsigned b=0;b<16;++b)c.capture(input.data()+b*256*8,256,8);REQUIRE(c.invalid==1);
    c.reset();for(unsigned i=0;i<bridge::Capacity+10;++i)c.ring.push({});REQUIRE(c.ring.available()==bridge::Capacity);
    c.ring.consume(100);for(unsigned i=0;i<100;++i)REQUIRE(c.ring.push({}));REQUIRE(c.ring.available()==bridge::Capacity);
    for(double drift:{-0.001,0.001}){
        c.reset();std::fill(input.begin(),input.end(),.1f);for(unsigned b=0;b<4;++b)c.capture(input.data(),256,8);
        double accumulated=0;
        for(int block=0;block<20000;++block){accumulated+=256*(1+drift);unsigned n=unsigned(accumulated);accumulated-=n;c.capture(input.data(),n,8);c.render(output.data(),256,8);}
        REQUIRE(c.underflows==0);REQUIRE(c.overflows==0);REQUIRE(c.ring.available()>128);REQUIRE(c.ring.available()<5000);
    }
    // Small device blocks must start well before the previous 50 ms prefill.
    c.reset();c.gain=1;
    for(unsigned j=0;j<8;++j)c.route[j]=j;
    std::fill(input.begin(),input.end(),.1f);
    for(unsigned n=0;n<4;++n)c.capture(input.data(),128,8);
    c.render(output.data(),128,8);
    REQUIRE(c.primed);REQUIRE(c.target==128+128+c.margin);REQUIRE(output[0]>.09f);
    // Realistic callback sizes and clock drift, including ring wraparound.
    for(unsigned block:{128u,256u,480u,1024u})for(double drift:{-.001,.001}) {
        c.reset();output.resize(block*8);
        unsigned warm=std::max(512u,2*block+2);
        for(unsigned n=0;n<warm;n+=block)c.capture(input.data(),block,8);
        double fraction=0;
        for(unsigned n=0;n<12000;++n) {
            fraction+=block*(1+drift);unsigned captured=unsigned(fraction);fraction-=captured;
            c.capture(input.data(),captured,8);c.render(output.data(),block,8);
        }
        REQUIRE(c.underflows==0);REQUIRE(c.overflows==0);
        REQUIRE(c.ring.available()<4*block+512);
    }
    // Asymmetric blocks measured on hardware: Qu ASIO 64 in, HDMI exclusive 960 out (96 kHz).
    // Queue must settle at out + in + margin, not 2 x the larger block.
    for(double drift:{-.001,.001}) {
        c.reset();output.resize(960*8);
        for(unsigned n=0;n<1300;n+=64)c.capture(input.data(),64,8);
        double fraction=0;
        for(unsigned n=0;n<10000;++n) {   // 100 s at 96 kHz
            fraction+=960*(1+drift);unsigned captured=unsigned(fraction);fraction-=captured;
            for(unsigned done=0;done<captured;done+=64)c.capture(input.data(),std::min(64u,captured-done),8);
            c.render(output.data(),960,8);
        }
        REQUIRE(c.underflows==0);REQUIRE(c.overflows==0);
        REQUIRE(c.target==960+64+c.margin);
        REQUIRE(c.ring.available()<c.target+960);
    }
    // Starvation produces silence and re-primes safely.
    c.reset();output.resize(256*8);
    for(unsigned n=0;n<5;++n)c.capture(input.data(),128,8);
    for(unsigned n=0;n<5;++n)c.render(output.data(),256,8);
    REQUIRE(c.underflows==1);REQUIRE(!c.primed);
    for(float value:output)REQUIRE(value==0);
    std::cout<<"PASS: low-latency startup, variable blocks, asymmetric 64/960 blocks, starvation recovery, channel routing, mute, silence, finite samples, bounded ring, +/-1000 ppm drift (106 s simulated each).\n";
}


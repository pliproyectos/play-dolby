#pragma once
#include <array>
#include <atomic>
#include <cmath>
#include <algorithm>
namespace bridge {
class Orientation {
    using Matrix=std::array<std::array<float,8>,8>;
    Matrix current{};
    int applied=0;
    unsigned remaining=0;
    static Matrix matrix(int degrees) {
        Matrix result{};
        const unsigned order[]={2,1,7,5,4,6,0};
        const float angles[]={0,30,90,150,210,270,330,360};
        result[3][3]=1;
        for(unsigned source:order){
            unsigned index=0;while(order[index]!=source)++index;
            float az=std::fmod(angles[index]+degrees+360.f,360.f);
            unsigned segment=0;while(segment<6 && az>=angles[segment+1])++segment;
            float t=(az-angles[segment])/(angles[segment+1]-angles[segment]);
            result[order[segment]][source]=std::cos(t*1.57079632679f);
            result[order[(segment+1)%7]][source]=std::sin(t*1.57079632679f);
        }
        return result;
    }
public:
    std::atomic<int> degrees{0};
    Orientation():current(matrix(0)){}
    void process(float* output,unsigned frames,unsigned channels,double rate) {
        int selected=degrees.load();auto target=matrix(selected);
        if(selected!=applied){applied=selected;remaining=std::max(1u,unsigned(rate*.02));}
        for(unsigned i=0;i<frames;++i){
            if(remaining){for(unsigned d=0;d<8;++d)for(unsigned s=0;s<8;++s)current[d][s]+=(target[d][s]-current[d][s])/remaining;--remaining;}
            std::array<float,8> input{},result{};
            for(unsigned c=0;c<channels;++c)input[c]=output[i*channels+c];
            for(unsigned d=0;d<channels;++d)for(unsigned s=0;s<channels;++s)result[d]+=current[d][s]*input[s];
            float peak=1;for(unsigned c=0;c<channels;++c)peak=std::max(peak,std::abs(result[c]));
            for(unsigned c=0;c<channels;++c)output[i*channels+c]=result[c]/peak;
        }
    }
};
}

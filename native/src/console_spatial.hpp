#pragma once
#include "bridge_core.hpp"

namespace bridge {

// The front pair of the routed console mix feeds the spatial effect. The other
// six channels retain their discrete routing. Call before adding Chrome.
class ConsoleSpatial {
public:
    enum Mode { Off, Classical, Rhythm, Continuous, LiveVoice };
    std::atomic<Mode> mode{Off};

    void prepare(double rate, bool console) {
        enabled = console;
        sampleRate = rate;
        interval = std::max(1u, unsigned(rate * .04));
        smoothing = float(1 - std::exp(-1 / (rate * .03)));
        lowCoeff = coefficient(220);
        midCoeff = coefficient(2200);
        highPassCoeff = coefficient(140);
        ambientCoeff = coefficient(5800);
        const double times[] = {.012,.015,.021,.025};
        for(unsigned i=0;i<4;++i) delays[i]=std::max(1u,unsigned(rate*times[i]));
        history.assign(delays.back()+1,0.f);
        position = samples = untilUpdate = 0;
        low = mid = sideLow = ambient = 0;
        previousEnergy = meanFlux = lowPower = midPower = sidePower = monoPower = 0;
        lastBeat = lastOrbit = -100.;
        beatPeriod = .6;
        orbitStart = -100.;
        left = targetLeft = Frame{1,0,0,0,0,0,0,0};
        right = targetRight = Frame{0,1,0,0,0,0,0,0};
        moving = targetMoving = {};
        ambientGain = targetAmbient = 0;
        active = Off;
    }

    void process(float* audio, unsigned frames, unsigned channels) {
        if(!enabled || channels != 8) return;
        const auto selected = mode.load(std::memory_order_relaxed);
        if(selected == Off && active == Off && !untilUpdate) return;
        if(selected != active) {
            active = selected;
            untilUpdate = selected == Off ? unsigned(sampleRate * .18) : interval;
            update();
        }
        for(unsigned i=0;i<frames;++i) {
            if(!untilUpdate) {
                if(active == Off) break;
                update();
                untilUpdate = interval;
            }
            auto* frame = audio+i*channels;
            const float l=frame[0], r=frame[1], mono=(l+r)*.5f, side=(l-r)*.5f;
            low += lowCoeff*(mono-low);
            mid += midCoeff*(mono-mid);
            sideLow += highPassCoeff*(side-sideLow);
            ambient += ambientCoeff*(side-sideLow-ambient);
            history[position]=ambient;
            lowPower+=low*low;
            midPower+=(mid-low)*(mid-low);
            sidePower+=side*side;
            monoPower+=mono*mono;
            ambientGain+=(targetAmbient-ambientGain)*smoothing;
            frame[0]=frame[1]=0;
            for(unsigned c=0;c<8;++c) {
                left[c]+=(targetLeft[c]-left[c])*smoothing;
                right[c]+=(targetRight[c]-right[c])*smoothing;
                moving[c]+=(targetMoving[c]-moving[c])*smoothing;
                frame[c]+=l*left[c]+r*right[c]+ambient*moving[c];
            }
            const unsigned surround[]={6,7,4,5};
            for(unsigned j=0;j<4;++j) {
                const auto delayed=(position+history.size()-delays[j])%history.size();
                frame[surround[j]]+=history[delayed]*ambientGain*(j%2 ? -.7071f : .7071f);
            }
            position=(position+1)%history.size();
            ++samples;
            if(--untilUpdate==0 && active==Off) {
                left=targetLeft;
                right=targetRight;
                moving={};
                ambientGain=0;
            }
        }
    }

private:
    const pli::Disposicion room{{
        {"FC",2,0},{"FR",1,30},{"SR",7,100},{"BR",5,145},
        {"BL",4,-145},{"SL",6,-100},{"FL",0,-30}
    }};
    bool enabled=false;
    Mode active=Off;
    double sampleRate=48000;
    unsigned interval=1920,untilUpdate=0;
    uint64_t samples=0;
    size_t position=0;
    std::array<unsigned,4> delays{};
    std::vector<float> history;
    float smoothing=0,lowCoeff=0,midCoeff=0,highPassCoeff=0,ambientCoeff=0;
    float low=0,mid=0,sideLow=0,ambient=0;
    double previousEnergy=0,meanFlux=0,lowPower=0,midPower=0,sidePower=0,monoPower=0;
    double lastBeat=-100,lastOrbit=-100,beatPeriod=.6,orbitStart=-100,orbitDuration=4.8;
    Frame left{},right{},moving{},targetLeft{},targetRight{},targetMoving{};
    float ambientGain=0,targetAmbient=0;

    float coefficient(double hz) const {
        return float(1-std::exp(-2*pli::Pi*hz/sampleRate));
    }
    void continuous(double angle,double width,Frame& l,Frame& r) const {
        l=room.ganancias(angle-width);
        r=room.ganancias(angle+width);
        const Frame baseL{.72f,0,.22f,0,.42f,0,.48f,0};
        const Frame baseR{0,.72f,.22f,0,0,.42f,0,.48f};
        for(unsigned c=0;c<8;++c) {
            l[c]=baseL[c]+.65f*l[c];
            r[c]=baseR[c]+.65f*r[c];
            const float scale=std::max(1.f,l[c]+r[c]);
            l[c]/=scale;
            r[c]/=scale;
        }
    }
    void update() {
        const double time=samples/sampleRate;
        const double energy=std::sqrt((lowPower+.4*midPower)/interval);
        const double flux=std::max(0.,energy-previousEnergy);
        previousEnergy=energy;
        meanFlux=.94*meanFlux+.06*flux;
        const double sideRms=std::sqrt(sidePower), monoRms=std::sqrt(monoPower);
        if(active==Rhythm && flux>std::max(.002,2.2*meanFlux) && time-lastBeat>.28) {
            const double period=time-lastBeat;
            if(period<=1.3) beatPeriod=.75*beatPeriod+.25*period;
            lastBeat=time;
            if(sideRms>.25*monoRms && time-lastOrbit>12.) {
                orbitStart=lastOrbit=time;
                orbitDuration=std::clamp(8*beatPeriod,3.,8.);
            }
        }
        lowPower=midPower=sidePower=monoPower=0;
        targetLeft={1,0,0,0,0,0,0,0};
        targetRight={0,1,0,0,0,0,0,0};
        targetMoving={};
        targetAmbient=0;
        const double angle=std::fmod(time*18,360.);
        if(active==Continuous) {
            continuous(angle,30,targetLeft,targetRight);
        } else if(active==Classical) {
            const double phase=angle/360.;
            const double progress=std::fmod(phase,.5)*2;
            const float sweep=float(std::pow(std::sin(pli::Pi*progress),4));
            const double direction=(phase<.5?1:-1)*(155-130*progress);
            Frame sweepLeft,sweepRight;
            continuous(0,30,targetLeft,targetRight);
            continuous(direction,12,sweepLeft,sweepRight);
            for(unsigned c=0;c<8;++c) {
                targetLeft[c]+=(sweepLeft[c]-targetLeft[c])*sweep;
                targetRight[c]+=(sweepRight[c]-targetRight[c])*sweep;
            }
        } else if(active==Rhythm) {
            targetLeft[0]=targetRight[1]=.82f;
            targetLeft[2]=targetRight[2]=.11f;
            targetAmbient=.28f;
            const double progress=(time-orbitStart)/orbitDuration;
            if(progress<1.) {
                targetMoving=room.ganancias(360*progress);
                for(auto& gain:targetMoving) gain*=.16f;
            }
        }
    }
};
}

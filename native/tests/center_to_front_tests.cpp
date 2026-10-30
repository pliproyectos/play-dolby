#include "../src/center_to_front.hpp"
#include <iostream>
#include <limits>
#include <stdexcept>
#define CHECK(x) if (!(x)) throw std::runtime_error(#x)
bool near(float a, float b) { return std::abs(a-b) < 1e-6f; }
int main() {
    const float g = bridge::CenterToFrontMinus12Db;
    std::array<float,8> input{.1f,-.1f,.4f,.2f,.3f,-.3f,.15f,-.15f};
    auto f=input;
    bridge::centerToFront(f,g);
    CHECK(near(f[0],input[0]+.4f*g)); CHECK(near(f[1],input[1]+.4f*g));
    for(int c=2;c<8;++c) CHECK(f[c]==input[c]);
    // Disable and invalid settings leave all channels identical.
    for(float amount : {0.f, -1.f, std::numeric_limits<float>::quiet_NaN()}) {
        f=input; bridge::centerToFront(f,amount); CHECK(f==input);
    }
    // Negative center polarity is retained on both fronts; no rear leakage.
    f={0,0,-.5f,0,0,0,0,0}; bridge::centerToFront(f,g);
    CHECK(near(f[0],-.5f*g)); CHECK(near(f[1],-.5f*g)); CHECK(f[2]==-.5f);
    for(int c=3;c<8;++c) CHECK(f[c]==0);
    // Correlated full-scale content must not overload. Linked reduction preserves ratios.
    f={1,1,1,.2f,.3f,.4f,.5f,.6f}; bridge::centerToFront(f,g);
    for(float sample:f) CHECK(std::isfinite(sample) && std::abs(sample)<=1.f);
    CHECK(near(f[0],1)); CHECK(near(f[1],1)); CHECK(near(f[2],1/(1+g)));
    CHECK(near(f[4]/f[2],.3f));
    f=input; f[2]=0; auto noCenter=f; bridge::centerToFront(f,g); CHECK(f==noCenter);
    std::cout << "PASS: center copy at -12 dB, original center, other channels, bypass, polarity, linked peak protection.\n";
}

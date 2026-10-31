// Two silent shared streams on the same endpoint, modelling bridge + Chrome.
#define main hardware_test_main
#include "hw_bridge_test.cpp"
#undef main
int silence(const void*, void* out, unsigned long n, const PaStreamCallbackTimeInfo*, PaStreamCallbackFlags, void* channels) {
    std::fill_n(static_cast<float*>(out), n * *static_cast<int*>(channels), 0.f);
    return paContinue;
}
int main() {
    PaStream *a=nullptr, *b=nullptr;
    try {
        api.load();
        int out=findOutput();
        if(out<0) for(int i=0;i<api.Pa_GetDeviceCount();++i)
            if(hostType(i)==paWASAPI && api.Pa_GetDeviceInfo(i)->maxOutputChannels>=2){out=i;break;}
        if(out<0){std::puts("SKIP: no WASAPI output connected");return 77;}
        auto d=api.Pa_GetDeviceInfo(out);
        int channels=d->maxOutputChannels>=8?8:2;
        auto info=wasapi::streamInfo(false,channels,true);
        PaStreamParameters op{out,channels,paFloat32,d->defaultLowOutputLatency,&info};
        auto check=[](PaError e){if(e!=paNoError)throw std::runtime_error(api.Pa_GetErrorText(e));};
        check(api.Pa_OpenStream(&a,nullptr,&op,96000,paFramesPerBufferUnspecified,paNoFlag,silence,&channels));
        check(api.Pa_StartStream(a));
        check(api.Pa_OpenStream(&b,nullptr,&op,d->defaultSampleRate,paFramesPerBufferUnspecified,paNoFlag,silence,&channels));
        check(api.Pa_StartStream(b));
        std::this_thread::sleep_for(std::chrono::seconds(2));
        if(api.Pa_IsStreamActive(a)!=1 || api.Pa_IsStreamActive(b)!=1)throw std::runtime_error("stream stopped");
        std::printf("PASS: two simultaneous silent shared streams, %s, %d channels, 96000 + %.0f Hz\n",d->name,channels,d->defaultSampleRate);
        api.Pa_AbortStream(b);api.Pa_CloseStream(b);b=nullptr;
        api.Pa_AbortStream(a);api.Pa_CloseStream(a);a=nullptr;
        api.Pa_Terminate();return 0;
    } catch(const std::exception& e) {
        if(b){api.Pa_AbortStream(b);api.Pa_CloseStream(b);}
        if(a){api.Pa_AbortStream(a);api.Pa_CloseStream(a);}
        std::printf("FAIL: %s\n",e.what());return 1;
    }
}

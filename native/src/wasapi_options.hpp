#pragma once
// Réplica mínima de PaWasapiStreamInfo (pa_win_wasapi.h, PortAudio v19.7.0, versión 1 de la
// estructura). PortAudio rechaza la estructura si size o version no coinciden, así que un
// desajuste de ABI falla al abrir, no en silencio.
#include "../vendor/portaudio.h"

namespace wasapi {
enum : unsigned long { Exclusive = 1u << 0, UseChannelMask = 1u << 2 };
constexpr unsigned long Mask71 = 0x63F;  // FL FR FC LFE BL BR SL SR, igual que Windows 7.1
constexpr unsigned long Mask51 = 0x3F;   // FL FR FC LFE BL BR
constexpr unsigned long Mask20 = 0x3;

struct StreamInfo {
    unsigned long size;
    PaHostApiTypeId hostApiType;
    unsigned long version;
    unsigned long flags;
    unsigned long channelMask;
    void* hostProcessorOutput;
    void* hostProcessorInput;
    int threadPriority;
    int streamCategory;
    int streamOption;
};

inline StreamInfo streamInfo(bool exclusive, unsigned channels = 8) {
    StreamInfo s{};
    s.size = sizeof(StreamInfo);
    s.hostApiType = paWASAPI;
    s.version = 1;
    s.flags = UseChannelMask | (exclusive ? Exclusive : 0);
    s.channelMask = channels == 8 ? Mask71 : channels == 6 ? Mask51 : Mask20;
    return s;
}
}

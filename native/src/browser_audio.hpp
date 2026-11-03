#pragma once
#include <winsock2.h>
#include <ws2tcpip.h>
#include <wincrypt.h>
#include <thread>
#include <memory>
#include <vector>
#include <array>
#include <atomic>
#include <string>
#include <sstream>
#include <stdexcept>
#include <cctype>
#include <cstring>
#include <cmath>
#include <algorithm>

namespace browserAudio {
using Frame = std::array<float,8>;
class Receiver {
    std::thread worker;
    std::atomic<bool> stopping{false};
    SOCKET listener=INVALID_SOCKET;
    // Each generation has one producer (network) and one consumer (HDMI).
    struct Buffer {
        std::vector<Frame> queue{32768};
        std::atomic<uint64_t> read{0},write{0};
        const unsigned sampleRate;
        double phase=0;
        bool primed=false;
        explicit Buffer(unsigned rate):sampleRate(rate){}
    };
    std::shared_ptr<Buffer> buffer=std::make_shared<Buffer>(48000);
    bool receive(SOCKET socket,void* data,size_t bytes) {
        auto p=static_cast<char*>(data);
        while(bytes && !stopping) {
            int n=recv(socket,p,int(bytes),0);
            if(n<=0)return false;
            p+=n;bytes-=size_t(n);
        }
        return bytes==0;
    }
    bool sendAll(SOCKET socket,const std::string& data) {
        size_t position=0;
        while(position<data.size() && !stopping) {
            int n=send(socket,data.data()+position,int(data.size()-position),0);
            if(n<=0)return false;
            position+=size_t(n);
        }
        return position==data.size();
    }
    static std::string acceptKey(const std::string& key) {
        auto text=key+"258EAFA5-E914-47DA-95CA-C5AB0DC85B11";
        HCRYPTPROV provider=0;HCRYPTHASH hash=0;
        if(!CryptAcquireContextW(&provider,nullptr,nullptr,PROV_RSA_FULL,CRYPT_VERIFYCONTEXT))return {};
        BYTE digest[20];DWORD length=sizeof(digest);std::string result;
        if(CryptCreateHash(provider,CALG_SHA1,0,0,&hash)) {
            if(CryptHashData(hash,reinterpret_cast<const BYTE*>(text.data()),DWORD(text.size()),0) && CryptGetHashParam(hash,HP_HASHVAL,digest,&length,0)) {
                DWORD size=0;
                CryptBinaryToStringA(digest,length,CRYPT_STRING_BASE64|CRYPT_STRING_NOCRLF,nullptr,&size);
                result.resize(size);
                if(CryptBinaryToStringA(digest,length,CRYPT_STRING_BASE64|CRYPT_STRING_NOCRLF,result.data(),&size))result.resize(size);
                else result.clear();
            }
            CryptDestroyHash(hash);
        }
        CryptReleaseContext(provider,0);return result;
    }
    bool handshake(SOCKET socket) {
        std::string request;char c;
        while(request.size()<8192 && receive(socket,&c,1)) {
            request+=c;if(request.size()>=4 && request.substr(request.size()-4)=="\r\n\r\n")break;
        }
        std::istringstream input(request);std::string line;
        std::getline(input,line);
        if(line!="GET /pli-audio HTTP/1.1\r")return false;
        std::string origin,key,upgrade,connection,version;
        while(std::getline(input,line)) {
            auto colon=line.find(':');if(colon==std::string::npos)continue;
            std::string name=line.substr(0,colon),value=line.substr(colon+1);
            for(auto& ch:name)ch=char(std::tolower(static_cast<unsigned char>(ch)));
            auto first=value.find_first_not_of(" \t"),last=value.find_last_not_of(" \t\r");
            value=first==std::string::npos?"":value.substr(first,last-first+1);
            if(name=="origin")origin=value;
            if(name=="sec-websocket-key")key=value;
            if(name=="upgrade")upgrade=value;
            if(name=="connection")connection=value;
            if(name=="sec-websocket-version")version=value;
        }
        if(origin!="chrome-extension://ebpjbolnlhmbebbhiooonakajihpknco") {
            sendAll(socket,"HTTP/1.1 403 Forbidden\r\nContent-Length: 0\r\n\r\n");return false;
        }
        for(auto& ch:upgrade)ch=char(std::tolower(static_cast<unsigned char>(ch)));
        for(auto& ch:connection)ch=char(std::tolower(static_cast<unsigned char>(ch)));
        BYTE decoded[32];DWORD size=sizeof(decoded);
        if(version!="13" || upgrade!="websocket" || connection.find("upgrade")==std::string::npos || !CryptStringToBinaryA(key.c_str(),DWORD(key.size()),CRYPT_STRING_BASE64,decoded,&size,nullptr,nullptr) || size!=16)return false;
        auto accepted=acceptKey(key);if(accepted.empty())return false;
        return sendAll(socket,"HTTP/1.1 101 Switching Protocols\r\nUpgrade: websocket\r\nConnection: Upgrade\r\nSec-WebSocket-Accept: "+accepted+"\r\n\r\n");
    }
    void session(SOCKET socket) {
        if(!handshake(socket))return;
        connected=true;clear();
        bool haveSequence=false;uint32_t previous=0;
        while(!stopping) {
            unsigned char header[2];if(!receive(socket,header,2))break;
            unsigned opcode=header[0]&15;
            if((header[0]&0x70) || !(header[0]&0x80) || !(header[1]&0x80))break;
            uint64_t length=header[1]&127;
            if(length==126){unsigned char bytes[2];if(!receive(socket,bytes,2))break;length=unsigned(bytes[0])*256+bytes[1];}
            else if(length==127){unsigned char bytes[8];if(!receive(socket,bytes,8))break;length=0;for(auto byte:bytes)length=(length<<8)|byte;}
            if(length>65552 || (opcode>=8 && length>125))break;
            unsigned char mask[4];if(!receive(socket,mask,4))break;
            std::vector<unsigned char> data(size_t(length),0);
            if(!receive(socket,data.data(),data.size()))break;
            for(size_t i=0;i<data.size();++i)data[i]^=mask[i%4];
            if(opcode==8)break;
            if(opcode==9){std::string pong; pong+=char(0x8a);pong+=char(length);pong.append(reinterpret_cast<const char*>(data.data()),data.size());if(!sendAll(socket,pong))break;continue;}
            if(opcode==10)continue;
            if(opcode!=2 || data.size()<16 || std::memcmp(data.data(),"PLI1",4))break;
            uint32_t rate,frames,sequence;
            std::memcpy(&rate,data.data()+4,4);std::memcpy(&frames,data.data()+8,4);std::memcpy(&sequence,data.data()+12,4);
            if(rate<8000 || rate>192000 || frames<128 || frames>2048 || data.size()!=16+size_t(frames)*32)break;
            if(haveSequence && (uint32_t(sequence-previous)==0 || uint32_t(sequence-previous)>0x7fffffff))break;
            previous=sequence;haveSequence=true;
            auto block=std::atomic_load(&buffer);
            auto write=block->write.load();
            if(block->sampleRate!=rate || write-block->read.load()+frames>block->queue.size()){
                block=std::make_shared<Buffer>(rate);
                std::atomic_store(&buffer,block);
                write=0;
            }
            for(unsigned i=0;i<frames;++i) {
                Frame frame;
                std::memcpy(frame.data(),data.data()+16+i*32,32);
                for(auto& sample:frame)if(!std::isfinite(sample))sample=0;
                block->queue[write++%block->queue.size()]=frame;
            }
            block->write.store(write);
            packets++;
        }
        connected=false;clear();
    }
    void run() {
        while(!stopping) {
            fd_set ready;FD_ZERO(&ready);FD_SET(listener,&ready);timeval timeout{0,200000};
            if(select(0,&ready,nullptr,nullptr,&timeout)<=0)continue;
            SOCKET client=accept(listener,nullptr,nullptr);if(client==INVALID_SOCKET)continue;
            DWORD wait=500;setsockopt(client,SOL_SOCKET,SO_RCVTIMEO,reinterpret_cast<char*>(&wait),sizeof(wait));
            setsockopt(client,SOL_SOCKET,SO_SNDTIMEO,reinterpret_cast<char*>(&wait),sizeof(wait));
            try{session(client);}catch(...){connected=false;clear();}
            shutdown(client,SD_BOTH);closesocket(client);
        }
    }
public:
    std::atomic<bool> connected{false};
    std::atomic<uint64_t> packets{0};
    std::atomic<float> gain{1.f};
    void clear(){std::atomic_store(&buffer,std::make_shared<Buffer>(48000));}
    void start() {
        WSADATA data;if(WSAStartup(MAKEWORD(2,2),&data))throw std::runtime_error("No se pudo iniciar la conexión Chrome.");
        listener=socket(AF_INET,SOCK_STREAM,IPPROTO_TCP);
        BOOL exclusive=TRUE;setsockopt(listener,SOL_SOCKET,SO_EXCLUSIVEADDRUSE,reinterpret_cast<char*>(&exclusive),sizeof(exclusive));
        sockaddr_in address{};address.sin_family=AF_INET;address.sin_port=htons(19771);address.sin_addr.s_addr=htonl(INADDR_LOOPBACK);
        if(listener==INVALID_SOCKET || bind(listener,reinterpret_cast<sockaddr*>(&address),sizeof(address)) || listen(listener,1)) {
            if(listener!=INVALID_SOCKET)closesocket(listener);listener=INVALID_SOCKET;WSACleanup();
            throw std::runtime_error("Puerto Chrome 19771 ocupado. Cierra la otra copia de Play Dolby.");
        }
        stopping=false;worker=std::thread([this]{run();});
    }
    void stop(){stopping=true;if(worker.joinable())worker.join();if(listener!=INVALID_SOCKET){closesocket(listener);listener=INVALID_SOCKET;WSACleanup();}}
    ~Receiver(){stop();}
    void mix(float* output,unsigned frames,unsigned channels,double rate) {
        auto block=std::atomic_load(&buffer);
        const auto write=block->write.load();
        auto read=block->read.load();
        auto& queue=block->queue;
        auto& phase=block->phase;
        auto& primed=block->primed;
        const unsigned sampleRate=block->sampleRate;
        const double target=sampleRate*.096;
        if(!primed){if(write-read<unsigned(target))return;primed=true;}
        const double ratio=sampleRate/rate*(1.+std::clamp((double(write-read)-target)/target*.002,-.003,.003));
        const float level=gain.load();
        for(unsigned i=0;i<frames;++i){
            if(write-read<2){primed=false;phase=0;break;}
            const auto& a=queue[read%queue.size()];const auto& b=queue[(read+1)%queue.size()];
            for(unsigned c=0;c<channels;++c)output[i*channels+c]+=float(a[c]+phase*(b[c]-a[c]))*level;
            phase+=ratio;auto consumed=uint64_t(phase);phase-=consumed;read+=std::min(consumed,write-read);
        }
        block->read.store(read);
    }
};
}

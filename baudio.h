#include <future>
#include <atomic>
#include <Windows.h>
#include <audioclient.h>
#include <AudioClient.h>
#include <AudioPolicy.h>
#include <MMDeviceApi.h>
#include <iostream>
#include <algorithm>
#include <stdexcept>
#include <fstream>
#include "encodeAudio.h"
extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
#include <libavutil/imgutils.h>
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include "libavutil/imgutils.h"
#include "libavutil/log.h"
#include "libswresample/swresample.h"

#include "libavutil/avutil.h"
#include "libavutil/opt.h"
}
const CLSID CLSID_MMDeviceEnumerator = __uuidof(MMDeviceEnumerator);
const IID IID_IMMDeviceEnumerator = __uuidof(IMMDeviceEnumerator);
const IID IID_IAudioClient = __uuidof(IAudioClient);
const IID IID_IAudioCaptureClient = __uuidof(IAudioCaptureClient);

class MyAudioSink {
public:
    int channel;
    int sample;
    int len = 0;
    //std::ofstream outfile;
    std::vector<BYTE> audioBuffer;
    MyAudioSink() {
        //outfile.open("tes.pcm", std::ios::out | std::ios::binary);
    }
    int SetFormat(WAVEFORMATEX* pwfx) {
        channel = pwfx->nChannels;
        sample = pwfx->wBitsPerSample / 8;
        //std::cout << channel << sample << std::endl;
        return 0;
    }
    int CopyData(BYTE* buff, int length, BOOL* done) {
        audioBuffer.insert(audioBuffer.end(), buff, buff + length * channel * sample);
        //outfile.write((const char*)buff, length * channel * sample);
        len += length;
        return 1;
        //return fwrite(buff, 1, length * channel * sample, fOutput);
    }

};
class rec_au {
public:

	int rec_duration = 100;
    audio_record* rs;
    std::atomic<bool>& audio_stop;
    MyAudioSink* pMySink;
    HANDLE hCaptureEvent;
    HRESULT hr;
    IMMDeviceEnumerator* pEnumerator = NULL;
    IMMDevice* pDevice = NULL;
    IAudioClient* pAudioClient = NULL;
    IAudioCaptureClient* pCaptureClient = NULL;
    WAVEFORMATEX* pwfx = NULL;
    UINT32 packetLength = 0;
    REFERENCE_TIME hnsRequestedDuration = 10000000;//1s
    void exits(HRESULT hr) {
        if (hr < 0) {
            throw std::runtime_error("fail to create!");
        }
    }
	rec_au(std::atomic<bool> &audio_stop,AVFormatContext* format_ctx, AVStream* audio_stream) :audio_stop(audio_stop){
        rs = new audio_record(48000, format_ctx, audio_stream);
	}
	int init_device() {
        CoInitialize(NULL);
        pMySink = new MyAudioSink;
        hr = CoCreateInstance(
            CLSID_MMDeviceEnumerator, NULL,
            CLSCTX_ALL, IID_IMMDeviceEnumerator,
            (void**)&pEnumerator);
        exits(hr);

        hr = pEnumerator->GetDefaultAudioEndpoint(
            eRender, eConsole, &pDevice);
        exits(hr);

        hr = pDevice->Activate(
            IID_IAudioClient, CLSCTX_ALL,
            NULL, (void**)&pAudioClient);
        exits(hr);

        hr = pAudioClient->GetMixFormat(&pwfx);
        exits(hr);

        hr = pAudioClient->Initialize(
            AUDCLNT_SHAREMODE_SHARED,
            AUDCLNT_STREAMFLAGS_LOOPBACK | AUDCLNT_STREAMFLAGS_EVENTCALLBACK,
            hnsRequestedDuration,
            0,
            pwfx,
            NULL);
        exits(hr);
        hCaptureEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
        if (hCaptureEvent == nullptr) {
            printf("Failed to create event.\n");
            return hr;
        }
        pAudioClient->SetEventHandle(hCaptureEvent);
        if (FAILED(hr)) {
            // 错误处理
            return -1;
        }
        // Get the size of the allocated buffer.
        //hr = pAudioClient->GetBufferSize(&bufferFrameCount);
        //exits(hr);

        hr = pAudioClient->GetService(
            IID_IAudioCaptureClient,
            (void**)&pCaptureClient);
        exits(hr);

        // Notify the audio sink which format to use.
        hr = pMySink->SetFormat(pwfx);
        exits(hr);

	}
	~rec_au(){}


    void loop() {
        pAudioClient->Start();
        int sss = 0;
        while (audio_stop.load()) {
            DWORD waitResult = WaitForSingleObject(hCaptureEvent, 500);  // 等待事件触发或超时
            //std::cout << "---" << std::endl;
            if (waitResult == WAIT_OBJECT_0) {
                // 获取可用的音频帧数
                UINT32 packetSize = 0;
                
                do {
                    hr = pCaptureClient->GetNextPacketSize(&packetSize);
                    if (SUCCEEDED(hr) && packetSize > 0) {
                        // 缓冲区有数据，读取并处理数据
                        BYTE* pData = nullptr;
                        UINT32 numFramesAvailable = 0;
                        DWORD flags = 0;

                        hr = pCaptureClient->GetBuffer(&pData, &numFramesAvailable, &flags, nullptr, nullptr);
                        pMySink->CopyData(pData, numFramesAvailable, FALSE);
                        sss += numFramesAvailable;
                        //std::cout << pMySink->audioBuffer.size() << std::endl;
                        //std::cout << packetSize << std::endl;
                        
                        if (SUCCEEDED(hr)) {
                            if (flags & AUDCLNT_BUFFERFLAGS_SILENT) {
                                // 如果是静音数据，填充静音帧
                                //FillSilentFrames(numFramesAvailable);
                            }
                            else {
                                // 处理实际音频数据
                                //ProcessAudioData(pData, numFramesAvailable);
                            }

                            // 释放已使用的缓冲区
                            pCaptureClient->ReleaseBuffer(numFramesAvailable);
                        }
                    }
                    else {
                        // 如果没有足够的数据，选择等待更多数据或处理不足部分
                        //printf("Not enough data available in buffer, waiting for more data...\n");
                    }
                } while (packetSize>0);
                
                int len = rs->start_record(pMySink->audioBuffer.data(), pMySink->audioBuffer.size(), FALSE);
                if (len < pMySink->audioBuffer.size()) {
                    pMySink->audioBuffer.erase(pMySink->audioBuffer.begin(), pMySink->audioBuffer.begin() + pMySink->audioBuffer.size() - len);
                }
            }
            else if (waitResult == WAIT_TIMEOUT) {
                // 超时处理（没有数据或没有事件触发）
                //FillSilentFrames();  // 填充静音或处理其他操作
                std::vector<char> silenceData(0.5*48000*2*4, 0);
                pMySink->audioBuffer.insert(pMySink->audioBuffer.end(), silenceData.begin(), silenceData.end());
                int len = rs->start_record(pMySink->audioBuffer.data(), pMySink->audioBuffer.size(), FALSE);
                sss += 24000;
                if (len < pMySink->audioBuffer.size()) {
                    pMySink->audioBuffer.erase(pMySink->audioBuffer.begin(), pMySink->audioBuffer.begin() + pMySink->audioBuffer.size() - len);
                }
            }

        }
        rs->end();
        audio_stop.store(true);
    }
};


//int main() {
//    CoInitialize(NULL);
//    /*rec_au *s = new rec_au();
//    s->init_device();
//    s->loop();*/
//}
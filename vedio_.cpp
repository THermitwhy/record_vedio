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

#include "bvedio.h"
#include "threadPool.hpp"
#include "baudio.h"
struct vedio_config {
    AVFormatContext* vedio_format_ctx;
    AVStream* vedio_stream;
    AVStream* audio_stream;
    int vedio_fps;
    std::string vedio_name;
    static vedio_config init_vedio_config(std::string file_name,int vedio_fps) {
        AVFormatContext* format_ctx = nullptr;
        avformat_alloc_output_context2(&format_ctx, nullptr, nullptr, file_name.c_str());
        AVStream* vedio_stream = avformat_new_stream(format_ctx, NULL);
        AVStream* audio_stream = avformat_new_stream(format_ctx, NULL);
        return vedio_config{ format_ctx,vedio_stream,audio_stream,vedio_fps, file_name };
    }
};

int main() {
    av_log_set_level(AV_LOG_DEBUG);
    const char* output_filename = "out.mp4";
	AVFormatContext* format_ctx = nullptr;
	avformat_alloc_output_context2(&format_ctx, nullptr, nullptr, output_filename);
    AVStream* vedio_stream = avformat_new_stream(format_ctx, NULL);
	AVStream *audio_stream = avformat_new_stream(format_ctx, NULL);
    AVDictionary* opt = nullptr;
    if (!(format_ctx->oformat->flags & AVFMT_NOFILE)) {
        if (avio_open(&format_ctx->pb, output_filename, AVIO_FLAG_WRITE) < 0) {
            //std::cerr << "无法打开输出文件" << std::endl;
            //avcodec_free_context(&codec_ctx);
            //avcodec_parameters_free(&codec_params);
            avformat_free_context(format_ctx);
            throw std::runtime_error("无法打开输出文件");
        }
    }

    std::atomic<bool> vedio_stop = true;
    std::atomic<bool> audio_stop = true;
    int fps = 30;
    //这里sleep一下，明天写
    bvedio* ve = new bvedio(fps, vedio_stop, vedio_stream, format_ctx);
    rec_au* ae = new rec_au(audio_stop,format_ctx,audio_stream);
    ae->init_device();
    //写入文件头
    av_dict_set_int(&opt, "video_track_timescale", fps, 0);
    int ret = avformat_write_header(format_ctx, &opt);
    if (ret < 0) {
        //std::cerr << "无法写入文件头" << std::endl;
        //avcodec_free_context(&codec_ctx);
        //avcodec_parameters_free(&codec_params);
        avformat_free_context(format_ctx);
        throw std::runtime_error("无法写入文件头");
    }
    ThreadPool pool(2);
    pool.submit([&]() {
        ve->loop();
        });
    pool.submit([&]() {
        
        ae->loop();
        });
    Sleep(10000);
    //std::string a;
    //std::cin >> a;
    vedio_stop.store(false);
    audio_stop.store(false);
    while(!vedio_stop.load()&&!audio_stop.load()){}
    if (av_write_trailer(format_ctx) < 0) {
        //std::cerr << "无法写入文件尾" << std::endl;
        throw std::runtime_error("无法写入文件尾");
    }
}
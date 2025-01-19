#pragma once
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
#include <chrono>
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
class audio_record {
public:
    audio_record(int sample_rate,AVFormatContext* format_ctx,AVStream* audio_stream) :sample_rate(sample_rate),audio_format(format_ctx), audio_stream(audio_stream){
        init_audio();
    }

    int start_record(BYTE* buff, int len, BOOL done) {
        int frame_count = audio_frame->nb_samples * av_get_bytes_per_sample(AV_SAMPLE_FMT_FLTP) * audio_frame->channels;
        memset(sample_data.data(), 0, sample_data.size());
        memset(sample_data2.data(), 0, sample_data2.size());
        int i = 0;
        while (len > frame_count) {
            memcpy(sample_data.data(), buff + i, frame_count);
            f32le_convert_to_fltp((float*)sample_data.data(), (float*)sample_data2.data(), audio_frame->nb_samples);
            i = i + frame_count;
            av_samples_fill_arrays(audio_frame->data, audio_frame->linesize,
                sample_data2.data(), audio_frame->channels,
                audio_frame->nb_samples, audio_codec_context->sample_fmt, 0);


            audio_frame->pts = pts;
            pts += audio_frame->nb_samples;
            len = len - frame_count;
            encode();
            memset(sample_data.data(), 0, sample_data.size());
            memset(sample_data2.data(), 0, sample_data2.size());
        }
        return len;
    }

    void end() {
        end_encode();
        //av_write_trailer(audio_format);
        av_frame_free(&audio_frame);
        av_packet_free(&audio_packet);
        avcodec_free_context(&audio_codec_context);
    }
private:
    void init_audio() {
        audio_par = avcodec_parameters_alloc();
        audio_par->codec_id = AV_CODEC_ID_AAC;
        audio_par->codec_type = AVMEDIA_TYPE_AUDIO;
        audio_par->channel_layout = AV_CH_LAYOUT_STEREO;
        audio_par->channels = av_get_channel_layout_nb_channels(audio_par->channel_layout);
        audio_par->profile = FF_PROFILE_AAC_LOW;    //
        audio_par->bit_rate = 128 * 1024;
        audio_par->sample_rate = sample_rate;
        audio_par->format = AV_SAMPLE_FMT_FLTP;

        audio_codec = avcodec_find_encoder(audio_par->codec_id);
        audio_codec_context = avcodec_alloc_context3(audio_codec);
        avcodec_parameters_to_context(audio_codec_context, audio_par);
        audio_codec_context->time_base = { 1,sample_rate };
        int re = avcodec_open2(audio_codec_context, audio_codec, nullptr);
        if (re < 0) {
            //std::cerr << "无法打开编码器" << std::endl;
            avcodec_free_context(&audio_codec_context);
            avcodec_parameters_free(&audio_par);
            avformat_free_context(audio_format);
            return;
            //throw std::runtime_error("无法打开编码器");
        }

        // 设置流的编解码器参数
        if (avcodec_parameters_from_context(audio_stream->codecpar, audio_codec_context) < 0) {
            //std::cerr << "无法设置流的编解码器参数" << std::endl;
            avcodec_free_context(&audio_codec_context);
            avcodec_parameters_free(&audio_par);
            avformat_free_context(audio_format);
            return;
            //throw std::runtime_error("无法设置流的编解码器参数");
        }




        audio_frame = av_frame_alloc();
        audio_packet = av_packet_alloc();


        audio_frame->nb_samples = audio_codec_context->frame_size;
        audio_frame->format = audio_codec_context->sample_fmt;
        audio_frame->channel_layout = audio_codec_context->channel_layout;
        audio_frame->channels = av_get_channel_layout_nb_channels(audio_frame->channel_layout);

        int ret = av_frame_get_buffer(audio_frame, 0);
        if (ret < 0) {
            fprintf(stderr, "Could not allocate audio data buffers\n");
            //exit(1);
        }
        sample_data.resize(audio_frame->nb_samples * audio_frame->channels * av_get_bytes_per_sample(AV_SAMPLE_FMT_FLTP));
        sample_data2.resize(audio_frame->nb_samples * audio_frame->channels * av_get_bytes_per_sample(AV_SAMPLE_FMT_FLTP));

    }
    void f32le_convert_to_fltp(float* f32le, float* fltp, int nb_samples) {
        float* fltp_l = fltp;   // 左通道
        float* fltp_r = fltp + nb_samples;   // 右通道
        for (int i = 0; i < nb_samples; i++) {
            fltp_l[i] = f32le[i * 2];     // 0 1   - 2 3
            fltp_r[i] = f32le[i * 2 + 1];   // 可以尝试注释左声道或者右声道听听声音
        }
    }



    int encode() {
        int ret = avcodec_send_frame(audio_codec_context, audio_frame);
        while (ret >= 0) {
            ret = avcodec_receive_packet(audio_codec_context, audio_packet);
            if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
                return 1;
            }
            if (ret < 0) {
                return -1;
            }
            audio_packet->stream_index = this->audio_stream->index;
            av_interleaved_write_frame(audio_format, audio_packet);
        }
        return 1;
    }

    int end_encode() {
        int ret = avcodec_send_frame(audio_codec_context, NULL);
        while (ret >= 0) {
            ret = avcodec_receive_packet(audio_codec_context, audio_packet);
            if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
                return -1;
            }
            if (ret < 0) {
                return -1;
            }
            audio_packet->stream_index = this->audio_stream->index;
            av_interleaved_write_frame(audio_format, audio_packet);
        }
        return 1;
    }
    AVStream* audio_stream;
    AVFormatContext* audio_format;
    const AVCodec* audio_codec;
    AVCodecParameters* audio_par;
    AVCodecContext* audio_codec_context;
    AVFrame* audio_frame;
    AVPacket* audio_packet;
    int sample_rate;
    std::vector<uint8_t> sample_data;
    std::vector<uint8_t> sample_data2;
    int pts = 0;

};

#pragma once
#ifndef B_VEDIO
#define B_VEDIO
#include <windows.h>
#include <stdio.h>
#include <assert.h>
#include <iostream>
#include <vector>
#include "baudio.hpp"
#include "utils.hpp"
#include "dx_capture.hpp"
extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
#include <libavutil/imgutils.h>

}


class bvedio {
public:
	bvedio(int fps,std::atomic<bool>& stop_flag, AVStream* stream, AVFormatContext* format_ctx);
	~bvedio();
	HWND hWnd ;
	//capture_windows_instance *captureinstance;
	ScreenCapture *sc;
	int vedio_fps;
	std::atomic<bool>& stop_flag;
	//编码器上下文
	AVCodecContext* codec_ctx;
	const AVCodec* codec;

	//音频
	// 编解码器参数
	AVCodecParameters* codec_params;
	// 视频流
	AVStream* video_stream;
	// 输出格式
	AVFormatContext* format_ctx = nullptr;
	// 文件头
	AVDictionary* opt = nullptr;
	//即时帧
	AVFrame* current_frame = nullptr;


	int count = 0;
	struct plane_param {
		int width;
		int height;
		int bitrate;
		int frame_rate;
		int time_count;
	};
	plane_param params;
	void initHeader();
	void initRect();
	int loop();
	void recordLoop();
	void captureFrame(int frame_count);
	//void release
	void endLoop();
};


#endif
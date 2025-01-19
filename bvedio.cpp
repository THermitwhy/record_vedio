#include "bvedio.h"
#include <fstream>
#include <exception>
#include <chrono>
bvedio::bvedio(int fps, std::atomic<bool>& stop_flag, AVStream* stream, AVFormatContext* format_ctx) :
    vedio_fps(fps), stop_flag(stop_flag), video_stream(stream), format_ctx(format_ctx)
{
    hWnd = GetDesktopWindow();
    initRect();
    initHeader();
    //captureinstance = new capture_windows_instance(params.width, params.height, hWnd);
    sc = new ScreenCapture(params.width, params.height);
}

bvedio::~bvedio()
{
    endLoop();

}

void bvedio::initHeader()
{
    //const char* output_filename = "output.mp4";
    //// 设置输出格式
    ////AVFormatContext* format_ctx = nullptr;
    //avformat_alloc_output_context2(&format_ctx, nullptr, nullptr, output_filename);
    //if (!format_ctx) {
    //    throw std::runtime_error("无法创建输出格式上下文");
    //    //std::cerr << "无法创建输出格式上下文" << std::endl;
    //}

    //// 设置视频流
    //video_stream = avformat_new_stream(format_ctx, nullptr);
    ////AVStream* s2 = avformat_new_stream(format_ctx, nullptr);;
    //if (!video_stream) {
    //    avformat_free_context(format_ctx);
    //    throw std::runtime_error("无法创建视频流");
    //    //std::cerr << "无法创建视频流" << std::endl;
    //    
    //}
    video_stream->time_base = { 1, params.frame_rate };

    // 设置编解码器参数
    codec_params = avcodec_parameters_alloc();
    codec_params->codec_id = AV_CODEC_ID_H264;
    codec_params->codec_type = AVMEDIA_TYPE_VIDEO;
    codec_params->width = params.width;
    codec_params->height = params.height;
    codec_params->format = AV_PIX_FMT_YUV420P;
    codec_params->bit_rate = params.bitrate;
    codec_params->profile = FF_PROFILE_H264_BASELINE;
    codec_params->level = 31;
    //codec_params->time_base
   
    



    codec = avcodec_find_encoder(codec_params->codec_id);
    //codec = avcodec_find_encoder_by_name("h264_nvenc");//硬件编码
    if (!codec) {

        //std::cerr << "无法找到编码器" << std::endl;
        avcodec_parameters_free(&codec_params);
        avformat_free_context(format_ctx);
        throw std::runtime_error("无法找到编码器");
    }

    codec_ctx = avcodec_alloc_context3(codec);
    if (!codec_ctx) {
        //std::cerr << "无法分配编码器上下文" << std::endl;
        avcodec_parameters_free(&codec_params);
        avformat_free_context(format_ctx);
        throw std::runtime_error("无法分配编码器上下文");
    }

    if (avcodec_parameters_to_context(codec_ctx, codec_params) < 0) {
        //std::cerr << "无法将编码器参数复制到编码器上下文" << std::endl;
        avcodec_free_context(&codec_ctx);
        avcodec_parameters_free(&codec_params);
        avformat_free_context(format_ctx);
        throw std::runtime_error("无法将编码器参数复制到编码器上下文");

    }
    codec_ctx->time_base = { 1, params.frame_rate };
    //codec_ctx->bit_rate = 4000000;
    
    /*codec_ctx->framerate = { 30,1 };
    codec_ctx->time_base = { 1, params.frame_rate };
    codec_ctx->framerate = { params.frame_rate, 1 };
    codec_ctx->gop_size = 10;
    codec_ctx->max_b_frames = 1;*/
    if (avcodec_open2(codec_ctx, codec, nullptr) < 0) {
        //std::cerr << "无法打开编码器" << std::endl;
        avcodec_free_context(&codec_ctx);
        avcodec_parameters_free(&codec_params);
        avformat_free_context(format_ctx);
        throw std::runtime_error("无法打开编码器");
    }
    //video_stream = avformat_new_stream(format_ctx, codec);
    // 设置流的编解码器参数
    if (avcodec_parameters_from_context(video_stream->codecpar, codec_ctx) < 0) {
        //std::cerr << "无法设置流的编解码器参数" << std::endl;
        avcodec_free_context(&codec_ctx);
        avcodec_parameters_free(&codec_params);
        avformat_free_context(format_ctx);
        throw std::runtime_error("无法设置流的编解码器参数");
    }

    // 打开输出文件
    //if (!(format_ctx->oformat->flags & AVFMT_NOFILE)) {
    //    if (avio_open(&format_ctx->pb, "out.mp4", AVIO_FLAG_WRITE) < 0) {
    //        //std::cerr << "无法打开输出文件" << std::endl;
    //        avcodec_free_context(&codec_ctx);
    //        avcodec_parameters_free(&codec_params);
    //        avformat_free_context(format_ctx);
    //        throw std::runtime_error("无法打开输出文件");
    //    }
    //}
    //// 写入文件头
    //format_ctx->streams[0]->codecpar->codec_type = AVMEDIA_TYPE_VIDEO;
    //av_dict_set_int(&opt, "video_track_timescale", 30, 0);
    ////format_ctx->video_codec_id = codec->id;
    ////format_ctx->video_codec = codec;
    //int ret = avformat_write_header(format_ctx, &opt);
    //if (ret < 0) {
    //    //std::cerr << "无法写入文件头" << std::endl;
    //    avcodec_free_context(&codec_ctx);
    //    avcodec_parameters_free(&codec_params);
    //    avformat_free_context(format_ctx);
    //    throw std::runtime_error("无法写入文件头");
    //}

    //initAudioStream();

}

void bvedio::initRect()
{
    params.width = 2560;
    params.height = 1440;
    params.bitrate = 4000000;
    params.frame_rate = vedio_fps;
    //params.time_count = times;
}
void writeYUVToFile(const char* filename, uint8_t* yuv420, int width, int height) {
    std::ofstream outfile(filename, std::ios::out | std::ios::binary);
    if (!outfile) {
        std::cerr << "无法打开文件以写入YUV数据" << std::endl;
        return;
    }

    int y_size = width * height;
    int u_size = y_size / 4;
    int v_size = y_size / 4;

    // 写入 Y 平面数据
    outfile.write(reinterpret_cast<char*>(yuv420), y_size);

    // 写入 U 平面数据
    outfile.write(reinterpret_cast<char*>(yuv420 + y_size), u_size);

    // 写入 V 平面数据
    outfile.write(reinterpret_cast<char*>(yuv420 + y_size + u_size), v_size);

    outfile.close();
}
int bvedio::loop()
{
    //counts = counts * params.frame_rate;
    int frame_count = 0;
    int y_stride = params.width;
    int u_stride = (params.width + 1) / 2;
    int v_stride = (params.width + 1) / 2;
    uint8_t* yuv420 = new uint8_t[y_stride * params.height + u_stride * (params.height / 2) + v_stride * (params.height / 2)];
    std::vector<BYTE> buffer(params.width * params.height * 4);
    //int delay_time = 1000 / params.frame_rate;
    std::chrono::milliseconds delay_time(1000 / params.frame_rate);
    AVPacket* pkt;
    pkt = av_packet_alloc();
    //pkt->stream_index = video_stream->index;
    while (stop_flag.load()) {

        auto start = std::chrono::high_resolution_clock::now();

        

        //std::cout << y_stride * params.height + u_stride * (params.height / 2) + v_stride * (params.height / 2) << std::endl;
        //captureinstance->grab_windows();
        sc->Capture(buffer.data());
        libyuv::ARGBToI420(
            (const uint8_t*)buffer.data(), params.width*4,
            yuv420, y_stride,
            yuv420 + (y_stride * params.height), u_stride,
            yuv420 + (y_stride * params.height) + (u_stride * (params.height / 2)), v_stride,
            params.width, params.height
        );

        auto t1 = std::chrono::high_resolution_clock::now();
        auto duration1 = std::chrono::duration_cast<std::chrono::milliseconds>(t1 - start);
        //std::cout << duration1.count() << std::endl;
        convertYUVToAVFrame(yuv420, params.width, params.height, y_stride, u_stride, v_stride,current_frame);
        //std::cout << sizeof(yuv420) << std::endl;
        
        if (!current_frame) {
            fprintf(stderr, "Error converting YUV to AVFrame\n");
           
            return -1;
        }

        auto t2 = std::chrono::high_resolution_clock::now();
        auto duration2 = std::chrono::duration_cast<std::chrono::milliseconds>(t2 - t1);
        // 设置帧的时间戳
        
        //frame_count++;
        current_frame->pts = frame_count;

        // 发送帧给编码器
        if (avcodec_send_frame(codec_ctx, current_frame) < 0) {
            std::cerr << "无法发送帧给编码器" << std::endl;
            av_frame_free(&current_frame);
            /*avcodec_free_context(&codec_ctx);
            avcodec_parameters_free(&codec_params);
            avformat_free_context(format_ctx);*/
            return -1;
        }

        pkt->data = nullptr;
        pkt->size = 0;
        pkt->time_base = codec_ctx->time_base;
        while (avcodec_receive_packet(codec_ctx, pkt) == 0) {
            //std::cout << pkt->pts << std::endl;
            int ret = av_interleaved_write_frame(format_ctx, pkt);
            if (ret  < 0) {
                //throw std::runtime_error("无法写入编码数据到文件");
                av_packet_unref(pkt);
                av_frame_free(&current_frame);
                /*avcodec_free_context(&codec_ctx);
                avcodec_parameters_free(&codec_params);
                avformat_free_context(format_ctx);*/
                return -1;
            }
            av_packet_unref(pkt);
        }


        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

        if (duration < delay_time) {
            while (std::chrono::high_resolution_clock::now() < start + delay_time) {

            }
            //std::cout << std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now()-start) << std::endl;
        }
        ++frame_count;
    }
    avcodec_send_frame(codec_ctx, nullptr);

    // 从编码器接收编码数据并写入文件
    //AVPacket pkt;
    //av_init_packet(&pkt);
    pkt->data = nullptr;
    pkt->size = 0;
    pkt->time_base = codec_ctx->time_base;
    while (avcodec_receive_packet(codec_ctx, pkt) == 0) {
        if (av_interleaved_write_frame(format_ctx, pkt) < 0) {
            //std::cerr << "无法写入编码数据到文件" << std::endl;
            throw std::runtime_error("无法写入编码数据到文件");
            av_packet_unref(pkt);
            av_frame_free(&current_frame);
            /*avcodec_free_context(&codec_ctx);
            avcodec_parameters_free(&codec_params);
            avformat_free_context(format_ctx);*/
            return -1;
        }
        av_packet_unref(pkt);
    }
    av_frame_free(&current_frame);
    //avcodec_flush_buffers(codec_ctx);
    //if (av_write_trailer(format_ctx) < 0) {
    //    //std::cerr << "无法写入文件尾" << std::endl;
    //    throw std::runtime_error("无法写入文件尾");
    //}
    delete[] yuv420;
    stop_flag.store(true);
    return 1;
}






//void bvedio::recordLoop() {
//    int frame_count = 0;
//    int audio_frame_index = 0;
//    while (frame_count < times * params.frame_rate) {
//        captureFrame(frame_count);
//        frame_count++;
//        // 更新音频帧索引
//    }
//}


//
//void bvedio::captureFrame(int frame_count)
//{
//    // 填充 YUV 数据（这里假设你已经有了 YUV 数据）
//        // 替换下面的代码以填充实际的 YUV 数据
//        //std::vector<uint8_t> yuv_data(width * height * 3 / 2, 0); // 假设这里创建了 YUV 数据
//
//        //// 将 YUV 数据填充到帧中
//        //int y_size = width * height;
//        //frame->data[0] = yuv_data.data();
//        //frame->data[1] = yuv_data.data() + y_size;
//        //frame->data[2] = yuv_data.data() + y_size * 5 / 4;
//        //frame->linesize[0] = width;
//        //frame->linesize[1] = width / 2;
//        //frame->linesize[2] = width / 2;
//    Sleep(1000 / params.frame_rate);
//
//    int y_stride = params.width;
//    int u_stride = (params.width + 1) / 2;
//    int v_stride = (params.width + 1) / 2;
//    uint8_t* yuv420 = new uint8_t[y_stride * params.height + u_stride * (params.height / 2) + v_stride * (params.height / 2)];
//
//    //std::cout << y_stride * params.height + u_stride * (params.height / 2) + v_stride * (params.height / 2) << std::endl;
//    //CaptureScreenOrWindow(hWnd, yuv420, y_stride, u_stride, v_stride);
//    //if (frame_count == 5) {
//    //    writeYUVToFile("output.yuv", yuv420, params.width, params.height);
//    //}
//    convertYUVToAVFrame(yuv420, params.width, params.height, y_stride, u_stride, v_stride, current_frame);
//    //std::cout << sizeof(yuv420) << std::endl;
//    delete[] yuv420;
//    if (!current_frame) {
//        fprintf(stderr, "Error converting YUV to AVFrame\n");
//
//        return ;
//    }
//
//
//
//    //ShootScreen(hWnd, frame);
//    // 设置帧的时间戳
//    current_frame->pts = frame_count;
//
//    // 发送帧给编码器
//    if (avcodec_send_frame(codec_ctx, current_frame) < 0) {
//        std::cerr << "无法发送帧给编码器" << std::endl;
//        av_frame_free(&current_frame);
//        /*avcodec_free_context(&codec_ctx);
//        avcodec_parameters_free(&codec_params);
//        avformat_free_context(format_ctx);*/
//        return ;
//    }
//
//    // 从编码器接收编码数据并写入文件
//    AVPacket pkt;
//    av_init_packet(&pkt);
//    pkt.data = nullptr;
//    pkt.size = 0;
//    if (avcodec_receive_packet(codec_ctx, &pkt) == 0) {
//        if (av_interleaved_write_frame(format_ctx, &pkt) < 0) {
//            //std::cerr << "无法写入编码数据到文件" << std::endl;
//            throw std::runtime_error("无法写入编码数据到文件");
//            av_packet_unref(&pkt);
//            av_frame_free(&current_frame);
//            /*avcodec_free_context(&codec_ctx);
//            avcodec_parameters_free(&codec_params);
//            avformat_free_context(format_ctx);*/
//            return ;
//        }
//        av_packet_unref(&pkt);
//    }
//    //av_packet_unref(&pkt);
//    
//    av_frame_free(&current_frame);
//}

void bvedio::endLoop()
{
    //int ret = av_write_trailer(format_ctx);
    //if(ret < 0) {
    //    //std::cerr << "无法写入文件尾" << std::endl;
    //    throw std::runtime_error("无法写入文件尾");
    //}

    // 释放资源
    //av_frame_free(&frame);
    avcodec_free_context(&codec_ctx);
    avcodec_parameters_free(&this->codec_params);
    avformat_free_context(format_ctx);
}

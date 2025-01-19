#ifndef B_UTILS
#define B_UTILS
#include <windows.h>
#include <stdio.h>
#include <assert.h>
#include <iostream>
#include <vector>
#include <chrono>
#include <libyuv.h> // 包含 libyuv 库
extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
#include <libavutil/imgutils.h>

}
static double getScale() {//获取屏幕缩放
    HWND hWnd = GetDesktopWindow();
    HMONITOR hMonitor = MonitorFromWindow(hWnd, MONITOR_DEFAULTTONEAREST);

    // 获取监视器逻辑宽度与高度
    MONITORINFOEX miex;
    miex.cbSize = sizeof(miex);
    GetMonitorInfo(hMonitor, &miex);
    int cxLogical = (miex.rcMonitor.right - miex.rcMonitor.left);
    int cyLogical = (miex.rcMonitor.bottom - miex.rcMonitor.top);

    // 获取监视器物理宽度与高度
    DEVMODE dm;
    dm.dmSize = sizeof(dm);
    dm.dmDriverExtra = 0;
    EnumDisplaySettings(miex.szDevice, ENUM_CURRENT_SETTINGS, &dm);
    int cxPhysical = dm.dmPelsWidth;
    int cyPhysical = dm.dmPelsHeight;

    // 缩放比例计算  实际上使用任何一个即可
    double horzScale = ((double)cxPhysical / (double)cxLogical);
    double vertScale = ((double)cyPhysical / (double)cyLogical);
    assert(horzScale - vertScale < 0.1); // 宽或高这个缩放值应该是相等的
    return vertScale;
}

//static void CaptureScreenOrWindow(HWND hWnd, uint8_t* yuv420, int y_stride, int u_stride, int v_stride) {
//    double scale = 1.5;
//    HDC hdc = CreateDC("DISPLAY", NULL, NULL, NULL);
//    if (!hdc) return;
//
//    uint32_t ScrWidth = 0, ScrHeight = 0;
//    RECT rect = { 0 };
//
//    if (hWnd == NULL) {
//        ScrWidth = GetDeviceCaps(hdc, HORZRES);
//        ScrHeight = GetDeviceCaps(hdc, VERTRES);
//    }
//    else {
//        GetWindowRect(hWnd, &rect);
//        ScrWidth = static_cast<uint32_t>((rect.right - rect.left) * scale);
//        ScrHeight = static_cast<uint32_t>((rect.bottom - rect.top) * scale);
//    }
//
//    HDC hmdc = CreateCompatibleDC(hdc);
//    if (!hmdc) {
//        DeleteDC(hdc);
//        return;
//    }
//
//    HBITMAP hBmpScreen = CreateCompatibleBitmap(hdc, ScrWidth, ScrHeight);
//    if (!hBmpScreen) {
//        DeleteDC(hmdc);
//        DeleteDC(hdc);
//        return;
//    }
//
//    HBITMAP holdbmp = (HBITMAP)SelectObject(hmdc, hBmpScreen);
//
//    BITMAP bm;
//    GetObject(hBmpScreen, sizeof(bm), &bm);
//
//    BITMAPINFOHEADER bi = { 0 };
//    bi.biSize = sizeof(BITMAPINFOHEADER);
//    bi.biWidth = bm.bmWidth;
//    bi.biHeight = bm.bmHeight;
//    bi.biPlanes = bm.bmPlanes;
//    bi.biBitCount = bm.bmBitsPixel;
//    bi.biCompression = BI_RGB;
//    bi.biSizeImage = bm.bmHeight * bm.bmWidthBytes;
//
//    char* buf = new char[bi.biSizeImage];
//    if (buf) {
//        BitBlt(hmdc, 0, 0, ScrWidth, ScrHeight, hdc, rect.left, rect.top, SRCCOPY);
//        GetDIBits(hmdc, hBmpScreen, 0, (DWORD)ScrHeight, buf, (LPBITMAPINFO)&bi, DIB_RGB_COLORS);
//        int rgb_stride = ScrWidth * 4;
//        libyuv::ARGBToI420(
//            (const uint8_t*)buf, rgb_stride,
//            yuv420, y_stride,
//            yuv420 + (y_stride * ScrHeight), u_stride,
//            yuv420 + (y_stride * ScrHeight) + (u_stride * (ScrHeight / 2)), v_stride,
//            ScrWidth, ScrHeight
//        );
//        delete[] buf;
//    }
//
//    SelectObject(hmdc, holdbmp);
//    DeleteObject(hBmpScreen);
//    DeleteDC(hmdc);
//    DeleteDC(hdc);
//}

static int createAVFrame(int width, int height, AVFrame* &frame) {
    if (frame == nullptr) {
        frame = av_frame_alloc();
    }
    //AVFrame* frame = av_frame_alloc();
    if (!frame) {
        fprintf(stderr, "Could not allocate AVFrame\n");
        return -1;
    }

    frame->format = AV_PIX_FMT_YUV420P;
    frame->width = width;
    frame->height = height;
    frame->time_base = { 1,30 };
    int ret = av_image_alloc(frame->data, frame->linesize, width, height, AV_PIX_FMT_YUV420P, 32);
    if (ret < 0) {
        fprintf(stderr, "Could not allocate raw picture buffer\n");
        av_frame_free(&frame);
        return -1;
    }

    return 0;
}
static int convertYUVToAVFrame(uint8_t* yuv420, int width, int height, int y_stride, int u_stride, int v_stride, AVFrame* &frame) {
    if (frame == nullptr) {
        createAVFrame(width, height, frame);
    }
    if (!frame) {
        return -1;
    }
        

    // Copy Y plane
    for (int i = 0; i < height; ++i) {
        //memcpy(frame->data[0] + i * frame->linesize[0], yuv420 + (height - 1 - i) * y_stride, width);
        memcpy(frame->data[0] + i * frame->linesize[0], yuv420 + i * y_stride, width);
    }

    // Copy U plane
    for (int i = 0; i < height / 2; ++i) {
        //memcpy(frame->data[1] + i * frame->linesize[1], yuv420 + y_stride * height + (height / 2 - 1 - i) * u_stride, width / 2);
        memcpy(frame->data[1] + i * frame->linesize[1], yuv420 + y_stride * height + i * u_stride, width / 2);
    }

    // Copy V plane
    for (int i = 0; i < height / 2; ++i) {
        //memcpy(frame->data[2] + i * frame->linesize[2], yuv420 + y_stride * height + u_stride * (height / 2) + (height / 2 - 1 - i) * v_stride, width / 2);
        memcpy(frame->data[2] + i * frame->linesize[2], yuv420 + y_stride * height + u_stride * (height / 2) +  i * v_stride, width / 2);
    }

    return 0;
}
//
//class capture_windows_instance {
//public:
//    capture_windows_instance(int p_width,int p_height, HWND &hWnd):hWnd(hWnd) {
//        hdc = CreateDC("DISPLAY", NULL, NULL, NULL);
//        hmdc = CreateCompatibleDC(hdc);
//        get_srcrect();
//        hBmpScreen = CreateCompatibleBitmap(hdc, ScrWidth, ScrHeight);
//        y_stride = p_width;
//        u_stride = (p_width + 1) / 2;
//        v_stride = (p_width + 1) / 2;
//        yuv420 = new uint8_t[y_stride * p_height + u_stride * (p_height / 2) + v_stride * (p_height / 2)];
//    };
//    int ruin() {
//        DeleteObject(hBmpScreen);
//        DeleteDC(hmdc);
//        DeleteDC(hdc);
//        delete[] buf;
//    };
//    double scale = 1.5;
//    HWND hWnd;
//    HDC hdc;
//    HDC hmdc;
//    HBITMAP hBmpScreen;
//    RECT rect = { 0 };
//    int y_stride, u_stride, v_stride;
//    uint32_t ScrWidth = 0, ScrHeight = 0;
//    uint8_t* yuv420 = nullptr;
//    char* buf = nullptr;
//    int get_srcrect() {
//        
//        if (hWnd == NULL) {
//            ScrWidth = GetDeviceCaps(hdc, HORZRES);
//            ScrHeight = GetDeviceCaps(hdc, VERTRES);
//        }
//        else {
//            GetWindowRect(hWnd, &rect);
//            ScrWidth = static_cast<uint32_t>((rect.right - rect.left) * scale);
//            ScrHeight = static_cast<uint32_t>((rect.bottom - rect.top) * scale);
//        }
//        return 1;
//    }
//    int grab_windows() {
//        auto start = std::chrono::high_resolution_clock::now();
//        double scale = 1.5;
//        //HDC hdc = CreateDC("DISPLAY", NULL, NULL, NULL);
//        if (!hdc) return -1;
//        if (!hmdc) {
//            DeleteDC(hdc);
//            return -1;
//        }
//
//        //HBITMAP hBmpScreen = CreateCompatibleBitmap(hdc, ScrWidth, ScrHeight);
//        if (!hBmpScreen) {
//            DeleteDC(hmdc);
//            DeleteDC(hdc);
//            return -1;
//        }
//
//        HBITMAP holdbmp = (HBITMAP)SelectObject(hmdc, hBmpScreen);
//
//        BITMAP bm;
//        GetObject(hBmpScreen, sizeof(bm), &bm);
//
//        BITMAPINFOHEADER bi = { 0 };
//        bi.biSize = sizeof(BITMAPINFOHEADER);
//        bi.biWidth = bm.bmWidth;
//        bi.biHeight = bm.bmHeight;
//        bi.biPlanes = bm.bmPlanes;
//        bi.biBitCount = bm.bmBitsPixel;
//        bi.biCompression = BI_RGB;
//        bi.biSizeImage = bm.bmHeight * bm.bmWidthBytes;
//
//        if (buf == nullptr) {
//            buf = new char[bi.biSizeImage];
//        }
//
//        if (buf) {
//            BitBlt(hmdc, 0, 0, ScrWidth, ScrHeight, hdc, rect.left, rect.top, SRCCOPY);
//            GetDIBits(hmdc, hBmpScreen, 0, (DWORD)ScrHeight, buf, (LPBITMAPINFO)&bi, DIB_RGB_COLORS);
//            int rgb_stride = ScrWidth * 4;
//            auto t1 = std::chrono::high_resolution_clock::now();
//            auto duration1 = std::chrono::duration_cast<std::chrono::milliseconds>(t1 - start);
//            std::cout << duration1.count() << std::endl;
//            
//            libyuv::ARGBToI420(
//                (const uint8_t*)buf, rgb_stride,
//                yuv420, y_stride,
//                yuv420 + (y_stride * ScrHeight), u_stride,
//                yuv420 + (y_stride * ScrHeight) + (u_stride * (ScrHeight / 2)), v_stride,
//                ScrWidth, ScrHeight
//            );
//
//        }
//
//        SelectObject(hmdc, holdbmp);
//        //DeleteObject(hBmpScreen);
//        //DeleteDC(hmdc);
//        //DeleteDC(hdc);
//    };
//};
#endif
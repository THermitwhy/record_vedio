#pragma once
#ifndef DX_CAPTURE
#define DX_CAPTURE
#include <d3d11.h>
#include <dxgi1_2.h>
#include <wrl.h>
#include <vector>
#include <iostream>
#pragma comment ( lib, "D3D11.lib")
using Microsoft::WRL::ComPtr;

class ScreenCapture {
public:
    ScreenCapture(int screenWidth, int screenHeight)
        : screenWidth(screenWidth), screenHeight(screenHeight) {
        Initialize();
    }

    ~ScreenCapture() {
        if (outputDuplication) {
            outputDuplication->ReleaseFrame();
        }
    }

    bool Capture(BYTE* buffer) {
        ComPtr<IDXGIResource> desktopResource;
        DXGI_OUTDUPL_FRAME_INFO frameInfo;

        HRESULT hr = outputDuplication->AcquireNextFrame(10, &frameInfo, &desktopResource);
        if (FAILED(hr)) {
            if (hr == DXGI_ERROR_WAIT_TIMEOUT) {
                return false;  // 超时，没有新帧
            }
            std::cerr << "Failed to acquire next frame: " << std::hex << hr << std::endl;
            return false;
        }

        ComPtr<ID3D11Texture2D> desktopImage;
        hr = desktopResource.As(&desktopImage);
        if (FAILED(hr)) {
            std::cerr << "Failed to cast resource to texture: " << std::hex << hr << std::endl;
            outputDuplication->ReleaseFrame();
            return false;
        }

        context->CopyResource(copyTexture.Get(), desktopImage.Get());

        D3D11_MAPPED_SUBRESOURCE mappedResource;
        hr = context->Map(copyTexture.Get(), 0, D3D11_MAP_READ, 0, &mappedResource);
        if (FAILED(hr)) {
            std::cerr << "Failed to map texture: " << std::hex << hr << std::endl;
            outputDuplication->ReleaseFrame();
            return false;
        }

        const BYTE* srcData = static_cast<const BYTE*>(mappedResource.pData);
        const int rowPitch = mappedResource.RowPitch;

        for (int y = 0; y < screenHeight; ++y) {
            memcpy(buffer + y * screenWidth * 4, srcData + y * rowPitch, screenWidth * 4);
        }

        context->Unmap(copyTexture.Get(), 0);
        outputDuplication->ReleaseFrame();

        return true;
    }

private:
    int screenWidth;
    int screenHeight;
    ComPtr<ID3D11Device> device;
    ComPtr<ID3D11DeviceContext> context;
    ComPtr<IDXGIOutputDuplication> outputDuplication;
    ComPtr<ID3D11Texture2D> copyTexture;

    void Initialize() {
        // 创建 D3D11 设备和上下文
        D3D_FEATURE_LEVEL featureLevel;
        HRESULT hr = D3D11CreateDevice(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, 0, nullptr, 0,
            D3D11_SDK_VERSION, &device, &featureLevel, &context);
        if (FAILED(hr)) {
            std::cerr << "Failed to create D3D11 device: " << std::hex << hr << std::endl;
            throw std::runtime_error("D3D11 device creation failed");
        }

        ComPtr<IDXGIDevice> dxgiDevice;
        device.As(&dxgiDevice);

        ComPtr<IDXGIAdapter> adapter;
        dxgiDevice->GetAdapter(&adapter);

        ComPtr<IDXGIOutput> output;
        adapter->EnumOutputs(0, &output);

        ComPtr<IDXGIOutput1> output1;
        output.As(&output1);

        hr = output1->DuplicateOutput(device.Get(), &outputDuplication);
        if (FAILED(hr)) {
            std::cerr << "Failed to duplicate output: " << std::hex << hr << std::endl;
            throw std::runtime_error("Output duplication failed");
        }

        // 创建 CPU 可访问的纹理
        D3D11_TEXTURE2D_DESC copyDesc = {};
        copyDesc.Width = screenWidth;
        copyDesc.Height = screenHeight;
        copyDesc.MipLevels = 1;
        copyDesc.ArraySize = 1;
        copyDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
        copyDesc.SampleDesc.Count = 1;
        copyDesc.Usage = D3D11_USAGE_STAGING;
        copyDesc.BindFlags = 0;
        copyDesc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
        copyDesc.MiscFlags = 0;

        hr = device->CreateTexture2D(&copyDesc, nullptr, &copyTexture);
        if (FAILED(hr)) {
            std::cerr << "Failed to create copy texture: " << std::hex << hr << std::endl;
            throw std::runtime_error("Texture creation failed");
        }
    }
};
//
//int main() {
//    int screenWidth = 2560;
//    int screenHeight = 1440;
//    ScreenCapture screenCapture(screenWidth, screenHeight);
//
//    std::vector<BYTE> buffer(screenWidth * screenHeight * 4);
//
//    if (screenCapture.Capture(buffer.data())) {
//        std::cout << "Screen captured successfully!" << buffer.size()<< std::endl;
//    }
//    else {
//        std::cerr << "Failed to capture screen." << std::endl;
//    }
//
//    return 0;
//}

#endif // !DX_CAPTURE

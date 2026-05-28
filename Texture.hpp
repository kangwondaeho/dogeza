#pragma once
#include "Framework.hpp"
#include <wincodec.h>

#pragma comment(lib, "windowscodecs.lib")
#pragma comment(lib, "ole32.lib")

class Texture
{
public:
    ID3D11ShaderResourceView* pSRV = nullptr;
    ID3D11SamplerState* pSampler = nullptr;
    UINT width = 0;
    UINT height = 0;

    Texture() = default;
    Texture(const Texture&) = delete;
    Texture& operator=(const Texture&) = delete;

    ~Texture()
    {
        Release();
    }

    void Release()
    {
        if (pSRV) { pSRV->Release(); pSRV = nullptr; }
        if (pSampler) { pSampler->Release(); pSampler = nullptr; }
    }

    bool Load(ID3D11Device* device, const wchar_t* path)
    {
        if (!device || !path) return false;

        Release();

        HRESULT hrInit = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
        if (FAILED(hrInit) && hrInit != RPC_E_CHANGED_MODE)
            return false;

        IWICImagingFactory* factory = nullptr;
        IWICBitmapDecoder* decoder = nullptr;
        IWICBitmapFrameDecode* frame = nullptr;
        IWICFormatConverter* converter = nullptr;
        ID3D11Texture2D* tex2D = nullptr;
        bool success = false;

        do
        {
            HRESULT hr = CoCreateInstance(
                CLSID_WICImagingFactory,
                nullptr,
                CLSCTX_INPROC_SERVER,
                IID_PPV_ARGS(&factory));
            if (FAILED(hr)) break;

            hr = factory->CreateDecoderFromFilename(
                path,
                nullptr,
                GENERIC_READ,
                WICDecodeMetadataCacheOnDemand,
                &decoder);
            if (FAILED(hr)) break;

            hr = decoder->GetFrame(0, &frame);
            if (FAILED(hr)) break;

            hr = frame->GetSize(&width, &height);
            if (FAILED(hr)) break;

            hr = factory->CreateFormatConverter(&converter);
            if (FAILED(hr)) break;

            hr = converter->Initialize(
                frame,
                GUID_WICPixelFormat32bppRGBA,
                WICBitmapDitherTypeNone,
                nullptr,
                0.0f,
                WICBitmapPaletteTypeCustom);
            if (FAILED(hr)) break;

            std::vector<unsigned char> pixels((size_t)width * (size_t)height * 4);
            hr = converter->CopyPixels(nullptr, width * 4, (UINT)pixels.size(), pixels.data());
            if (FAILED(hr)) break;

            D3D11_TEXTURE2D_DESC td = {};
            td.Width = width;
            td.Height = height;
            td.MipLevels = 1;
            td.ArraySize = 1;
            td.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
            td.SampleDesc.Count = 1;
            td.Usage = D3D11_USAGE_DEFAULT;
            td.BindFlags = D3D11_BIND_SHADER_RESOURCE;

            D3D11_SUBRESOURCE_DATA init = {};
            init.pSysMem = pixels.data();
            init.SysMemPitch = width * 4;

            hr = device->CreateTexture2D(&td, &init, &tex2D);
            if (FAILED(hr)) break;

            D3D11_SHADER_RESOURCE_VIEW_DESC srvd = {};
            srvd.Format = td.Format;
            srvd.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
            srvd.Texture2D.MipLevels = 1;

            hr = device->CreateShaderResourceView(tex2D, &srvd, &pSRV);
            if (FAILED(hr)) break;

            success = true;
        } while (false);

        if (tex2D) tex2D->Release();
        if (converter) converter->Release();
        if (frame) frame->Release();
        if (decoder) decoder->Release();
        if (factory) factory->Release();

        if (!success) Release();
        return success;
    }

    bool CreateSampler(ID3D11Device* device)
    {
        if (!device) return false;
        if (pSampler) { pSampler->Release(); pSampler = nullptr; }

        D3D11_SAMPLER_DESC desc = {};
        desc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
        desc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
        desc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
        desc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
        desc.ComparisonFunc = D3D11_COMPARISON_NEVER;
        desc.MinLOD = 0.0f;
        desc.MaxLOD = D3D11_FLOAT32_MAX;

        return SUCCEEDED(device->CreateSamplerState(&desc, &pSampler));
    }
};

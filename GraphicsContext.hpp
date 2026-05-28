#pragma once
#include "Framework.hpp"

class GraphicsContext {
public:
    ID3D11Device* Device = nullptr;
    ID3D11DeviceContext* ImmediateContext = nullptr;
    IDXGISwapChain* SwapChain = nullptr;
    ID3D11RenderTargetView* RTV = nullptr;
    bool IsFullscreen = false;
    int VSync = 1;
    ID3D11BlendState* AlphaBlendState = nullptr;
    ID3D11BlendState* AdditiveBlendState = nullptr;

    // Post-processing bloom resources.
    ID3D11Texture2D* SceneTexture = nullptr;
    ID3D11RenderTargetView* SceneRTV = nullptr;
    ID3D11ShaderResourceView* SceneSRV = nullptr;
    ID3D11SamplerState* PostSampler = nullptr;
    ID3D11Buffer* PostVertexBuffer = nullptr;
    ID3D11Buffer* BloomConstantBuffer = nullptr;
    ShaderSet PostCopyShaders;
    ShaderSet BloomShaders;
    bool BloomEnabled = true;
    float BloomThreshold = 0.72f;
    float BloomIntensity = 0.38f;
    float BloomRadius = 1.75f;
    int BackBufferWidth = 1280;
    int BackBufferHeight = 720;

    ~GraphicsContext() {
        ReleasePostProcess();
        if (AdditiveBlendState) AdditiveBlendState->Release();
        if (AlphaBlendState) AlphaBlendState->Release();
        if (RTV) RTV->Release();
        if (SwapChain) SwapChain->Release();
        if (ImmediateContext) ImmediateContext->Release();
        if (Device) Device->Release();
    }

    bool InitDX(HWND hWnd, int w, int h) {
        DXGI_SWAP_CHAIN_DESC sd = {};
        sd.BufferCount = 1;
        sd.BufferDesc.Width = w;
        sd.BufferDesc.Height = h;
        sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
        sd.OutputWindow = hWnd;
        sd.SampleDesc.Count = 1;
        sd.Windowed = TRUE;

        HRESULT hr = D3D11CreateDeviceAndSwapChain(NULL, D3D_DRIVER_TYPE_HARDWARE, NULL, 0, NULL, 0,
            D3D11_SDK_VERSION, &sd, &SwapChain, &Device, NULL, &ImmediateContext);

        if (FAILED(hr)) return false;

        D3D11_BLEND_DESC blendDesc = {};
        blendDesc.RenderTarget[0].BlendEnable = TRUE;
        blendDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
        blendDesc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
        blendDesc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
        blendDesc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
        blendDesc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ZERO;
        blendDesc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
        blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
        Device->CreateBlendState(&blendDesc, &AlphaBlendState);

        D3D11_BLEND_DESC addDesc = {};
        addDesc.RenderTarget[0].BlendEnable = TRUE;
        addDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_ONE;
        addDesc.RenderTarget[0].DestBlend = D3D11_BLEND_ONE;
        addDesc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
        addDesc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
        addDesc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ONE;
        addDesc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
        addDesc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
        Device->CreateBlendState(&addDesc, &AdditiveBlendState);

        BackBufferWidth = w;
        BackBufferHeight = h;

        bool ok = CreateRTV(w, h);
        InitPostProcess(w, h);
        return ok;
    }

    bool CreateRTV(int w, int h) {
        if (RTV) RTV->Release();
        ID3D11Texture2D* pBB = nullptr;
        SwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&pBB);
        Device->CreateRenderTargetView(pBB, NULL, &RTV);
        if (pBB) pBB->Release();
        return true;
    }

    void Resize(int w, int h) {
        ImmediateContext->OMSetRenderTargets(0, 0, 0);
        if (RTV) { RTV->Release(); RTV = nullptr; }
        SwapChain->ResizeBuffers(0, w, h, DXGI_FORMAT_UNKNOWN, 0);
        BackBufferWidth = w;
        BackBufferHeight = h;
        CreateRTV(w, h);
        InitPostProcess(w, h);
    }


    void ReleasePostProcessTargets()
    {
        if (SceneSRV) { SceneSRV->Release(); SceneSRV = nullptr; }
        if (SceneRTV) { SceneRTV->Release(); SceneRTV = nullptr; }
        if (SceneTexture) { SceneTexture->Release(); SceneTexture = nullptr; }
    }

    void ReleasePostProcess()
    {
        ReleasePostProcessTargets();
        if (PostSampler) { PostSampler->Release(); PostSampler = nullptr; }
        if (PostVertexBuffer) { PostVertexBuffer->Release(); PostVertexBuffer = nullptr; }
        if (BloomConstantBuffer) { BloomConstantBuffer->Release(); BloomConstantBuffer = nullptr; }
        PostCopyShaders.Release();
        BloomShaders.Release();
    }

    bool CreateSceneTarget(int w, int h)
    {
        ReleasePostProcessTargets();

        D3D11_TEXTURE2D_DESC td = {};
        td.Width = w;
        td.Height = h;
        td.MipLevels = 1;
        td.ArraySize = 1;
        td.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        td.SampleDesc.Count = 1;
        td.Usage = D3D11_USAGE_DEFAULT;
        td.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;

        if (FAILED(Device->CreateTexture2D(&td, nullptr, &SceneTexture))) return false;
        if (FAILED(Device->CreateRenderTargetView(SceneTexture, nullptr, &SceneRTV))) return false;
        if (FAILED(Device->CreateShaderResourceView(SceneTexture, nullptr, &SceneSRV))) return false;
        return true;
    }

    bool InitPostProcess(int w, int h)
    {
        if (!Device) return false;

        CreateSceneTarget(w, h);

        if (!PostSampler)
        {
            D3D11_SAMPLER_DESC sd = {};
            sd.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
            sd.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
            sd.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
            sd.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
            sd.MaxLOD = D3D11_FLOAT32_MAX;
            Device->CreateSamplerState(&sd, &PostSampler);
        }

        if (!PostVertexBuffer)
        {
            Vertex verts[6] = {
                { { -1.0f,  1.0f, 0.0f }, { 0.0f, 0.0f, 0.0f, 1.0f } },
                { {  1.0f,  1.0f, 0.0f }, { 1.0f, 0.0f, 0.0f, 1.0f } },
                { { -1.0f, -1.0f, 0.0f }, { 0.0f, 1.0f, 0.0f, 1.0f } },
                { {  1.0f, -1.0f, 0.0f }, { 1.0f, 1.0f, 0.0f, 1.0f } },
                { { -1.0f, -1.0f, 0.0f }, { 0.0f, 1.0f, 0.0f, 1.0f } },
                { {  1.0f,  1.0f, 0.0f }, { 1.0f, 0.0f, 0.0f, 1.0f } }
            };

            D3D11_BUFFER_DESC bd = {};
            bd.Usage = D3D11_USAGE_DEFAULT;
            bd.ByteWidth = sizeof(verts);
            bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;

            D3D11_SUBRESOURCE_DATA sd = {};
            sd.pSysMem = verts;
            Device->CreateBuffer(&bd, &sd, &PostVertexBuffer);
        }

        if (!BloomConstantBuffer)
        {
            D3D11_BUFFER_DESC bd = {};
            bd.Usage = D3D11_USAGE_DEFAULT;
            bd.ByteWidth = sizeof(BloomBuffer);
            bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
            Device->CreateBuffer(&bd, nullptr, &BloomConstantBuffer);
        }

        if (!PostCopyShaders.vs || !PostCopyShaders.ps || !PostCopyShaders.layout)
        {
            D3D11_INPUT_ELEMENT_DESC ied[] =
            {
                { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT,    0,  0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
                { "COLOR",    0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 }
            };
            PostCopyShaders = CompileAndCreate(L"postcopy.hlsl", 0, true, ied, ARRAYSIZE(ied));
        }

        if (!BloomShaders.vs || !BloomShaders.ps || !BloomShaders.layout)
        {
            D3D11_INPUT_ELEMENT_DESC ied[] =
            {
                { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT,    0,  0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
                { "COLOR",    0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 }
            };
            BloomShaders = CompileAndCreate(L"bloom.hlsl", 0, true, ied, ARRAYSIZE(ied));
        }

        return true;
    }

    void BeginSceneRender(float clearColor[4], int w, int h)
    {
        if (!SceneRTV) InitPostProcess(w, h);

        ID3D11ShaderResourceView* nullSRV[1] = { nullptr };
        ImmediateContext->PSSetShaderResources(0, 1, nullSRV);

        ImmediateContext->ClearRenderTargetView(SceneRTV, clearColor);
        ImmediateContext->OMSetRenderTargets(1, &SceneRTV, nullptr);
    }

    void DrawPostQuad(ShaderSet& shaders)
    {
        if (!PostVertexBuffer || !SceneSRV || !PostSampler) return;

        ImmediateContext->IASetInputLayout(shaders.layout);
        ImmediateContext->VSSetShader(shaders.vs, nullptr, 0);
        ImmediateContext->PSSetShader(shaders.ps, nullptr, 0);

        ID3D11ShaderResourceView* srv = SceneSRV;
        ImmediateContext->PSSetShaderResources(0, 1, &srv);
        ImmediateContext->PSSetSamplers(0, 1, &PostSampler);

        UINT stride = sizeof(Vertex);
        UINT offset = 0;
        ImmediateContext->IASetVertexBuffers(0, 1, &PostVertexBuffer, &stride, &offset);
        ImmediateContext->Draw(6, 0);
    }

    void EndSceneRenderWithBloom(int w, int h)
    {
        if (!RTV || !SceneSRV) return;

        float black[] = { 0, 0, 0, 1 };
        ImmediateContext->ClearRenderTargetView(RTV, black);
        ImmediateContext->OMSetRenderTargets(1, &RTV, nullptr);

        float blendFactor[4] = { 0, 0, 0, 0 };

        // Base scene copy.
        // Important: copy the original scene opaquely. If alpha blending is used here,
        // transparent UI/effect pixels can blend with the black backbuffer and the whole
        // frame looks darker. Bloom must be added after this, not replace/dim the base.
        ImmediateContext->OMSetBlendState(nullptr, blendFactor, 0xffffffff);
        DrawPostQuad(PostCopyShaders);

        if (BloomEnabled && BloomShaders.ps && BloomShaders.vs)
        {
            BloomBuffer cb = {};
            cb.texelThreshold = XMFLOAT4(1.0f / (float)w, 1.0f / (float)h, BloomThreshold, BloomIntensity);
            cb.params = XMFLOAT4(BloomRadius, 0.0f, 0.0f, 0.0f);
            ImmediateContext->UpdateSubresource(BloomConstantBuffer, 0, nullptr, &cb, 0, 0);
            ImmediateContext->PSSetConstantBuffers(1, 1, &BloomConstantBuffer);

            ImmediateContext->OMSetBlendState(AdditiveBlendState, blendFactor, 0xffffffff);
            DrawPostQuad(BloomShaders);
        }

        ImmediateContext->OMSetBlendState(AlphaBlendState, blendFactor, 0xffffffff);

        ID3D11ShaderResourceView* nullSRV[1] = { nullptr };
        ImmediateContext->PSSetShaderResources(0, 1, nullSRV);
    }

    void SetFullscreen(bool goFull) {
        IsFullscreen = goFull;
        SwapChain->SetFullscreenState(goFull, NULL);
    }

    ShaderSet CompileAndCreate(const void* source, size_t length, bool isFile, D3D11_INPUT_ELEMENT_DESC* ied, UINT iedCount)
    {
        ShaderSet res;
        ID3DBlob* vsBlob = nullptr;
        ID3DBlob* psBlob = nullptr;
        ID3DBlob* errBlob = nullptr;
        HRESULT hr;

        if (isFile)
        {
            hr = D3DCompileFromFile((LPCWSTR)source, nullptr, nullptr, "VS", "vs_5_0", 0, 0, &vsBlob, &errBlob);
            if (FAILED(hr))
            {
                if (errBlob)
                {
                    OutputDebugStringA((char*)errBlob->GetBufferPointer());
                    errBlob->Release();
                }
                else
                {
                    if (hr == HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND))
                    {
                        MessageBoxA(NULL, "shader file not found", "Critical Error", MB_ICONERROR);
                    }
                    else
                    {
                        char msg[256];
                        sprintf_s(msg, "Shader VS error. HRESULT: 0x%08X", hr);
                        MessageBoxA(NULL, msg, "System Error", MB_ICONERROR);
                    }
                }
                if (vsBlob) vsBlob->Release();
                return res;
            }

            hr = D3DCompileFromFile((LPCWSTR)source, nullptr, nullptr, "PS", "ps_5_0", 0, 0, &psBlob, &errBlob);
            if (FAILED(hr))
            {
                if (errBlob)
                {
                    OutputDebugStringA((char*)errBlob->GetBufferPointer());
                    errBlob->Release();
                }
                else
                {
                    if (hr == HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND))
                    {
                        MessageBoxA(NULL, "shader file not found", "Critical Error", MB_ICONERROR);
                    }
                    else
                    {
                        char msg[256];
                        sprintf_s(msg, "Shader PS error. HRESULT: 0x%08X", hr);
                        MessageBoxA(NULL, msg, "System Error", MB_ICONERROR);
                    }
                }
                if (psBlob) psBlob->Release();
                return res;
            }
        }
        else
        {
            hr = D3DCompile(source, length, nullptr, nullptr, nullptr, "VS", "vs_5_0", 0, 0, &vsBlob, &errBlob);
            if (FAILED(hr))
            {
                if (errBlob)
                {
                    printf("[Shader Error] VS Compile Error:\n%s\n", (char*)errBlob->GetBufferPointer());
                    errBlob->Release();
                }
                return res;
            }

            hr = D3DCompile(source, length, nullptr, nullptr, nullptr, "PS", "ps_5_0", 0, 0, &psBlob, &errBlob);
            if (FAILED(hr))
            {
                if (errBlob)
                {
                    printf("[Shader Error] PS Compile Error:\n%s\n", (char*)errBlob->GetBufferPointer());
                    errBlob->Release();
                }
                if (vsBlob) vsBlob->Release();
                return res;
            }
        }

        Device->CreateVertexShader(vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), nullptr, &res.vs);
        Device->CreatePixelShader(psBlob->GetBufferPointer(), psBlob->GetBufferSize(), nullptr, &res.ps);

        if (vsBlob && ied)
        {
            Device->CreateInputLayout(ied, iedCount, vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), &res.layout);
        }

        if (vsBlob) vsBlob->Release();
        if (psBlob) psBlob->Release();

        return res;
    }
};

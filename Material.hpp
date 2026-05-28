#pragma once
#include "Framework.hpp"
#include "Texture.hpp"

class Material {
public:
    ShaderSet shaders;
    Material(ShaderSet s) : shaders(s) {}
    virtual ~Material() {}
    virtual void Bind(ID3D11DeviceContext* context) = 0;
};

class ColorMaterial : public Material {
public:
    XMFLOAT4 color;
    ID3D11Buffer* pColorBuffer = nullptr;

    ColorMaterial(ShaderSet s, XMFLOAT4 col, ID3D11Device* device)
        : Material(s), color(col) {
        D3D11_BUFFER_DESC cbd = { 0 };
        cbd.Usage = D3D11_USAGE_DEFAULT;
        cbd.ByteWidth = sizeof(ColorBuffer);
        cbd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
        device->CreateBuffer(&cbd, nullptr, &pColorBuffer);
    }

    virtual ~ColorMaterial() {
        if (pColorBuffer) pColorBuffer->Release();
    }

    void SetColor(XMFLOAT4 col) { color = col; }

    void Bind(ID3D11DeviceContext* context) override {
        context->IASetInputLayout(shaders.layout);
        context->VSSetShader(shaders.vs, nullptr, 0);
        context->PSSetShader(shaders.ps, nullptr, 0);

        ColorBuffer cb = { color };
        context->UpdateSubresource(pColorBuffer, 0, nullptr, &cb, 0, 0);
        context->PSSetConstantBuffers(1, 1, &pColorBuffer);
    }
};

class TextureMaterial : public Material
{
private:
    Texture* texture = nullptr;

public:
    TextureMaterial(ShaderSet s, Texture* tex)
        : Material(s), texture(tex) {}

    void SetTexture(Texture* tex)
    {
        texture = tex;
    }

    Texture* GetTexture() const
    {
        return texture;
    }

    void Bind(ID3D11DeviceContext* context) override
    {
        context->IASetInputLayout(shaders.layout);
        context->VSSetShader(shaders.vs, nullptr, 0);
        context->PSSetShader(shaders.ps, nullptr, 0);

        if (texture)
        {
            ID3D11ShaderResourceView* srv = texture->pSRV;
            ID3D11SamplerState* sampler = texture->pSampler;
            context->PSSetShaderResources(0, 1, &srv);
            context->PSSetSamplers(0, 1, &sampler);
        }
    }
};

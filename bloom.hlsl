Texture2D sceneTexture : register(t0);
SamplerState sceneSampler : register(s0);

cbuffer cbBloom : register(b1)
{
    float4 texelThreshold; // xy = texel size, z = threshold, w = intensity
    float4 params;         // x = radius
};

struct VS_IN
{
    float3 pos : POSITION;
    float4 uvData : COLOR;
};

struct PS_IN
{
    float4 pos : SV_POSITION;
    float2 uv : TEXCOORD0;
};

PS_IN VS(VS_IN input)
{
    PS_IN output;
    output.pos = float4(input.pos, 1.0f);
    output.uv = input.uvData.xy;
    return output;
}

float3 Bright(float2 uv)
{
    float3 c = sceneTexture.Sample(sceneSampler, uv).rgb;
    float lum = dot(c, float3(0.2126f, 0.7152f, 0.0722f));
    float k = saturate((lum - texelThreshold.z) / max(0.0001f, 1.0f - texelThreshold.z));
    return c * k;
}

float4 PS(PS_IN input) : SV_Target
{
    float2 texel = texelThreshold.xy * params.x;
    float2 uv = input.uv;

    float3 sum = 0.0f;
    float weight = 0.0f;

    sum += Bright(uv) * 0.22f; weight += 0.22f;

    sum += Bright(uv + texel * float2( 1.0f,  0.0f)) * 0.13f; weight += 0.13f;
    sum += Bright(uv + texel * float2(-1.0f,  0.0f)) * 0.13f; weight += 0.13f;
    sum += Bright(uv + texel * float2( 0.0f,  1.0f)) * 0.13f; weight += 0.13f;
    sum += Bright(uv + texel * float2( 0.0f, -1.0f)) * 0.13f; weight += 0.13f;

    sum += Bright(uv + texel * float2( 1.0f,  1.0f)) * 0.08f; weight += 0.08f;
    sum += Bright(uv + texel * float2(-1.0f,  1.0f)) * 0.08f; weight += 0.08f;
    sum += Bright(uv + texel * float2( 1.0f, -1.0f)) * 0.08f; weight += 0.08f;
    sum += Bright(uv + texel * float2(-1.0f, -1.0f)) * 0.08f; weight += 0.08f;

    sum += Bright(uv + texel * float2( 2.0f,  0.0f)) * 0.045f; weight += 0.045f;
    sum += Bright(uv + texel * float2(-2.0f,  0.0f)) * 0.045f; weight += 0.045f;
    sum += Bright(uv + texel * float2( 0.0f,  2.0f)) * 0.045f; weight += 0.045f;
    sum += Bright(uv + texel * float2( 0.0f, -2.0f)) * 0.045f; weight += 0.045f;

    float3 bloom = (sum / max(0.0001f, weight)) * texelThreshold.w;
    return float4(bloom, 0.0f);
}

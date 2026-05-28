cbuffer cbWorld : register(b0)
{
    matrix matWorld;
};

Texture2D mainTexture : register(t0);
SamplerState mainSampler : register(s0);

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
    output.pos = mul(float4(input.pos, 1.0f), matWorld);
    output.uv = input.uvData.xy;
    return output;
}

float4 PS(PS_IN input) : SV_Target
{
    return mainTexture.Sample(mainSampler, input.uv);
}

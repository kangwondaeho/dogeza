Texture2D sceneTexture : register(t0);
SamplerState sceneSampler : register(s0);

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

float4 PS(PS_IN input) : SV_Target
{
    float4 c = sceneTexture.Sample(sceneSampler, input.uv);
    return float4(c.rgb, 1.0f);
}

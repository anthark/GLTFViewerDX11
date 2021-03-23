Texture2D<float4> envTexture : register(t0);

SamplerState samState : register(s0);

static const float PI = 3.14159265358979323846f;

cbuffer Transformation : register(b0)
{
    matrix World;
    matrix View;
    matrix Projection;
    float4 CameraPos;
}

struct VS_INPUT
{
    float3 Pos : POSITION;
};

struct PS_INPUT
{
    float4 Pos : SV_POSITION;
    float3 Tex : TEXCOORD;
};

PS_INPUT vs_main(VS_INPUT input)
{
    PS_INPUT output = (PS_INPUT) 0;
    output.Pos = float4(input.Pos, 1.0f);
    output.Pos = mul(output.Pos, World);
    output.Pos = mul(output.Pos, View);
    output.Pos = mul(output.Pos, Projection);
    output.Tex = input.Pos;
    return output;
}

float4 ps_main(PS_INPUT input) : SV_TARGET
{
    float3 pos = normalize(input.Tex);
    float2 invAtan = float2(0.1591, 0.3183);
    float2 uv = float2(atan(pos.z / pos.x), asin(pos.y)) * invAtan + 0.5;
    float u = 1.0f - atan(pos.z / pos.x) / (2 * PI);
    float v = asin(pos.y) / PI + 0.5f;
    float u1 = 0.5 + atan(pos.x / pos.z) / (2 * PI);
    float v1 = 0.5 - asin(pos.y) / PI;

    // return float4(1, 0, 0, 1);
    return float4(envTexture.Sample(samState, float2(u, v)).xyz, 1.0f);
}

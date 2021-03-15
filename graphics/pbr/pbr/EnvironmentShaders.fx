Texture2D<float4> sourceTexture : register(t0);

SamplerState samState : register(s0);

cbuffer Transformation: register(b0)
{
    matrix World;
    matrix View;
    matrix Projection;
	float4 CameraPos;
}

struct VS_INPUT
{
    float4 Pos : POSITION;
    float2 Tex : TEXCOORD0;
    float3 Normal : NORMAL;
};

struct PS_INPUT
{
    float4 Pos : SV_POSITION;
    float2 Tex : TEXCOORD0;
};

PS_INPUT vs_main(VS_INPUT input)
{
    PS_INPUT output = (PS_INPUT)0;
    output.Pos = mul(input.Pos, World);
    output.Pos = mul(output.Pos, View);
    output.Pos = mul(output.Pos, Projection);
    output.Tex = input.Tex;
    return output;
}

float4 ps_main(PS_INPUT input) : SV_TARGET
{
    return sourceTexture.Sample(samState, input.Tex) * 16;
}

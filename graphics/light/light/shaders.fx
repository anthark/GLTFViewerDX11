Texture2D shaderTexture : register(t0);

SamplerState samLinear : register(s0);

cbuffer Transformation: register(b0)
{
    matrix World;
    matrix View;
    matrix Projection;
}

struct VS_INPUT
{
    float4 Pos : POS;
    float2 Tex : TEX;
};

struct PS_INPUT
{
    float4 Pos : SV_POSITION;
    float2 Tex : TEX;
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
    return shaderTexture.Sample(samLinear, input.Tex);
}

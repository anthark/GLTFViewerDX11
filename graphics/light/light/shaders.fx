#define NUM_LIGHTS 3

Texture2D shaderTexture : register(t0);

SamplerState samLinear : register(s0);

cbuffer Transformation: register(b0)
{
    matrix World;
    matrix View;
    matrix Projection;
}

cbuffer LightsPosition: register(b1)
{
    float3 LightPositions[NUM_LIGHTS];
}

cbuffer LightsColor : register(b2)
{
    float4 LightColors[NUM_LIGHTS];
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
    float3 LightDir1 : TEXCOORD1;
    float3 LightDir2 : TEXCOORD2;
    float3 LightDir3 : TEXCOORD3;
    float3 Normal : NORMAL;
};

PS_INPUT vs_main(VS_INPUT input)
{
    PS_INPUT output = (PS_INPUT)0;
    output.Pos = mul(input.Pos, World);

    output.LightDir1 = normalize(LightPositions[0] - output.Pos.xyz);
    output.LightDir2 = normalize(LightPositions[1] - output.Pos.xyz);
    output.LightDir3 = normalize(LightPositions[2] - output.Pos.xyz);

    output.Pos = mul(output.Pos, View);
    output.Pos = mul(output.Pos, Projection);
    output.Tex = input.Tex;
    output.Normal = normalize(mul(input.Normal, (float3x3)World));
    return output;
}

float4 ps_main(PS_INPUT input) : SV_TARGET
{
    float4 color1, color2, color3;
    color1 = LightColors[0] * saturate(dot(input.Normal, input.LightDir1));
    color2 = LightColors[1] * saturate(dot(input.Normal, input.LightDir2));
    color3 = LightColors[2] * saturate(dot(input.Normal, input.LightDir3));
    return shaderTexture.Sample(samLinear, input.Tex) * (color1 + color2 + color3);
}

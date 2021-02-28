#define NUM_LIGHTS 3

Texture2D shaderTexture : register(t0);

SamplerState samLinear : register(s0);

cbuffer Transformation: register(b0)
{
    matrix World;
    matrix View;
    matrix Projection;
    float4 Eye;
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
    float3 ViewDir : TEXCOORD4;
    float3 LightDir1 : TEXCOORD1;
    float3 LightDir2 : TEXCOORD2;
    float3 LightDir3 : TEXCOORD3;
    float3 Normal : NORMAL;
};

PS_INPUT vs_main(VS_INPUT input)
{
    PS_INPUT output = (PS_INPUT)0;
    output.Pos = mul(input.Pos, World);

    output.ViewDir = normalize(Eye.xyz - output.Pos.xyz);

    output.LightDir1 = output.Pos.xyz - LightPositions[0];
    output.LightDir2 = output.Pos.xyz - LightPositions[1];
    output.LightDir3 = output.Pos.xyz - LightPositions[2];

    output.Pos = mul(output.Pos, View);
    output.Pos = mul(output.Pos, Projection);
    output.Tex = input.Tex;
    output.Normal = normalize(mul(input.Normal, (float3x3)World));
    return output;
}

float Attenuation(float3 lightDir)
{
    float d = length(lightDir);
    return 1 / (1.0f + 10.0f * d + 100.0f * d * d);
}

float4 ps_main(PS_INPUT input) : SV_TARGET
{
    float4 color1, color2, color3;
    float atten1, atten2, atten3;
    float spot1, spot2, spot3;
    float3 lightDir1, lightDir2, lightDir3;
    float3 h1, h2, h3;
    float p1, p2, p3;

    atten1 = Attenuation(input.LightDir1);
    atten2 = Attenuation(input.LightDir2);
    atten3 = Attenuation(input.LightDir3);

    p1 = 2.0f;
    p2 = 2.0f;
    p3 = 2.0f;

    spot1 = 0.2;
    spot2 = 0.2;
    spot3 = 0.2;

    lightDir1 = normalize(input.LightDir1);
    lightDir2 = normalize(input.LightDir2);
    lightDir3 = normalize(input.LightDir3);

    h1 = normalize(input.ViewDir + input.LightDir1);
    h2 = normalize(input.ViewDir + input.LightDir2);
    h3 = normalize(input.ViewDir + input.LightDir3);

    color1 = LightColors[0] * atten1 * spot1 * (1 + dot(input.Normal, lightDir1) + pow(dot(input.Normal, h1), p1));
    color2 = LightColors[1] * atten2 * spot2 * (1 + dot(input.Normal, lightDir2) + pow(dot(input.Normal, h2), p2));
    color3 = LightColors[2] * atten3 * spot3 * (1 + dot(input.Normal, lightDir3) + pow(dot(input.Normal, h3), p3));

    return shaderTexture.Sample(samLinear, input.Tex) * (color1 + color2 + color3);
}

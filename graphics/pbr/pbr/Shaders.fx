#define NUM_LIGHTS 3

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

    output.LightDir1 = LightPositions[0] - output.Pos.xyz;
    output.LightDir2 = LightPositions[1] - output.Pos.xyz;
    output.LightDir3 = LightPositions[2] - output.Pos.xyz;

    output.Pos = mul(output.Pos, View);
    output.Pos = mul(output.Pos, Projection);
    output.Tex = input.Tex;
    output.Normal = normalize(mul(input.Normal, (float3x3)World));
    return output;
}

float Attenuation(float3 lightDir)
{
    float d = length(lightDir);
    return 1 / (1.0f + 0.1f * d + 0.01f * d * d);
}

float4 ps_main(PS_INPUT input) : SV_TARGET
{
    float4 color1, color2, color3;
    float atten1, atten2, atten3;
    float3 lightDir1, lightDir2, lightDir3;

    atten1 = Attenuation(input.LightDir1);
    atten2 = Attenuation(input.LightDir2);
    atten3 = Attenuation(input.LightDir3);

    lightDir1 = normalize(input.LightDir1);
    lightDir2 = normalize(input.LightDir2);
    lightDir3 = normalize(input.LightDir3);

    color1 = LightColors[0] * atten1 * dot(lightDir1, input.Normal);
    color2 = LightColors[1] * atten2 * dot(lightDir2, input.Normal);
    color3 = LightColors[2] * atten3 * dot(lightDir3, input.Normal);

    return float4(1.0f, 1.0f, 1.0f, 1.0f) * (color1 + color2 + color3);
}

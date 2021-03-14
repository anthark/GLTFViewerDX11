#define NUM_LIGHTS 3

static const float PI = 3.14159265358979323846f;

cbuffer Transformation: register(b0)
{
    matrix World;
    matrix View;
    matrix Projection;
	float4 CameraPos;
}

cbuffer LightsPosition: register(b1)
{
    float3 LightPositions[NUM_LIGHTS];
}

cbuffer LightsColor : register(b2)
{
    float4 LightColors[NUM_LIGHTS];
}

cbuffer Material : register(b3)
{
	float3 Albedo;
	float Roughness;
	float Metalness;
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
    float3 Normal : NORMAL;
	float4 WorldPos : POSITION;
};

PS_INPUT vs_main(VS_INPUT input)
{
    PS_INPUT output = (PS_INPUT)0;
    output.Pos = mul(input.Pos, World);
	output.WorldPos = output.Pos;
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
    
	lightDir1 = LightPositions[0] - input.WorldPos.xyz;
	lightDir2 = LightPositions[1] - input.WorldPos.xyz;
	lightDir3 = LightPositions[2] - input.WorldPos.xyz;
    
    atten1 = Attenuation(lightDir1);
    atten2 = Attenuation(lightDir2);
	atten3 = Attenuation(lightDir3);

    lightDir1 = normalize(lightDir1);
    lightDir2 = normalize(lightDir2);
    lightDir3 = normalize(lightDir3);

    color1 = LightColors[0] * atten1 * dot(lightDir1, input.Normal);
    color2 = LightColors[1] * atten2 * dot(lightDir2, input.Normal);
    color3 = LightColors[2] * atten3 * dot(lightDir3, input.Normal);

    return float4(1.0f, 1.0f, 1.0f, 1.0f) * (color1 + color2 + color3);
}

float3 h(float3 v, float3 l) 
{
    return normalize((v + l) / 2);
}

float normalDistribution(float3 n, float3 v, float3 l)
{
    float roughnessSqr = pow(max(Roughness, 0.0001f), 2);
	return roughnessSqr / (PI * pow(pow(max(dot(n, h(v, l)), 0), 2) * (roughnessSqr - 1) + 1, 2));
}

float4 ndps_main(PS_INPUT input) : SV_TARGET
{
	return normalDistribution(input.Normal, normalize(CameraPos.xyz - input.WorldPos.xyz), normalize(LightPositions[0] - input.WorldPos.xyz));
}

float SchlickGGX(float3 n, float3 v, float k)
{
	float value = max(dot(n, v), 0);
	return value / (value * (1 - k) + k);
}

float geometry(float3 n, float3 v, float3 l)
{
	float k = pow(Roughness + 1, 2) / 8;
	return SchlickGGX(n, v, k) * SchlickGGX(n, l, k);
}

float4 gps_main(PS_INPUT input) : SV_TARGET
{
	return geometry(input.Normal, normalize(CameraPos.xyz - input.WorldPos.xyz), normalize(LightPositions[0] - input.WorldPos.xyz));
}

float3 fresnel(float3 n, float3 v, float3 l)
{
	float3 F0 = lerp(float3(0.04f, 0.04f, 0.04f), Albedo, Metalness);
	return (F0 + (1 - F0) * pow(1 - max(dot(h(v, l), v), 0), 5)) * sign(max(dot(l, n), 0));
}

float4 fps_main(PS_INPUT input) : SV_TARGET
{
	return float4(fresnel(input.Normal, normalize(CameraPos.xyz - input.WorldPos.xyz), normalize(LightPositions[0] - input.WorldPos.xyz)), 1.0f);
}
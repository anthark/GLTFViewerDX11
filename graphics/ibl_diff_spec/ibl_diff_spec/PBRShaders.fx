#define NUM_LIGHTS 3

TextureCube irradianceTexture : register(t0);
TextureCube prefilteredColorTexture : register(t1);
Texture2D<float4> preintegratedBRDFTexture : register(t2);

SamplerState MinMagMipLinear : register(s0);
SamplerState MinMagLinearMipPointClamp : register(s1);

static const float PI = 3.14159265358979323846f;
static const float MAX_REFLECTION_LOD = 4.0;

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
    float3 MetalF0;
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

float3 h(float3 v, float3 l) 
{
    return normalize(v + l);
}

float normalDistribution(float3 n, float3 v, float3 l)
{
    float roughnessSqr = pow(max(Roughness, 0.01f), 2);
	return roughnessSqr / (PI * pow(pow(max(dot(n, h(v, l)), 0), 2) * (roughnessSqr - 1) + 1, 2));
}

float4 ndps_main(PS_INPUT input) : SV_TARGET
{
    return normalDistribution(normalize(input.Normal), normalize(CameraPos.xyz - input.WorldPos.xyz), normalize(LightPositions[0] - input.WorldPos.xyz));
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
    return geometry(normalize(input.Normal), normalize(CameraPos.xyz - input.WorldPos.xyz), normalize(LightPositions[0] - input.WorldPos.xyz));
}

float3 fresnel(float3 n, float3 v, float3 l)
{
	float3 F0 = lerp(float3(0.04f, 0.04f, 0.04f), Albedo, Metalness);
    return (F0 + (1 - F0) * pow(1 - max(dot(h(v, l), v), 0), 5)) * sign(max(dot(l, n), 0));
}

float4 fps_main(PS_INPUT input) : SV_TARGET
{
    return float4(fresnel(normalize(input.Normal), normalize(CameraPos.xyz - input.WorldPos.xyz), normalize(LightPositions[0] - input.WorldPos.xyz)), 1.0f);
}

float3 BRDF(float3 p, float3 n, float3 v, float3 l)
{
	float D = normalDistribution(n, v, l);
	float G = geometry(n, v, l);
	float3 F = fresnel(n, v, l);

    return (1 - F) * Albedo / PI * (1 - Metalness) + D * F * G / (0.001f + 4 * (max(dot(l, n), 0) * max(dot(v, n), 0)));
}

float Attenuation(float3 lightDir)
{
	float d = length(lightDir);
	return 1 / (1.0f + 0.1f * d + 0.01f * d * d);
}

float3 LO_i(float3 p, float3 n, float3 v, float3 lightDir, float4 lightColor)
{
	float atten = Attenuation(lightDir);
	float3 l = normalize(lightDir);
    return BRDF(p, n, v, l) * lightColor.rgb * atten * max(dot(l, n), 0) * lightColor.a;
}

float3 FresnelSchlickRoughnessFunction(float3 F0, float3 n, float3 v, float roughness)
{
    return (F0 + (max(1 - roughness, F0) - F0) * pow(1 - max(dot(n, v), 0), 5));
}

float3 Ambient(float3 n, float3 v)
{
    float3 r = normalize(reflect(-v, n));
    float3 prefilteredColor = prefilteredColorTexture.SampleLevel(MinMagMipLinear, r, Roughness * MAX_REFLECTION_LOD).xyz;
    float3 F0 = lerp(float3(0.04, 0.04, 0.04), MetalF0, Metalness);
    float3 F = FresnelSchlickRoughnessFunction(F0, n, v, Roughness);
    float2 envBRDF = preintegratedBRDFTexture.Sample(MinMagLinearMipPointClamp, float2(max(dot(n, v), 0.0), Roughness)).xy;
    float3 specular = prefilteredColor * (F0 * envBRDF.x + envBRDF.y);

    float3 irradiance = irradianceTexture.Sample(MinMagMipLinear, n).xyz;
    return (1 - F) * irradiance * Albedo * (1 - Metalness) + specular;
}

float4 ps_main(PS_INPUT input) : SV_TARGET
{
	float3 color1, color2, color3;
	float3 v = normalize(CameraPos.xyz - input.WorldPos.xyz);
    float3 n = normalize(input.Normal);

	color1 = LO_i(input.WorldPos.xyz, n, v, LightPositions[0] - input.WorldPos.xyz, LightColors[0]);
	color2 = LO_i(input.WorldPos.xyz, n, v, LightPositions[1] - input.WorldPos.xyz, LightColors[1]);
	color3 = LO_i(input.WorldPos.xyz, n, v, LightPositions[2] - input.WorldPos.xyz, LightColors[2]);

    float3 ambient = Ambient(n, v);

    return float4(color1 + color2 + color3 + ambient, 1.0f);
}

#define NUM_LIGHTS 1

TextureCube irradianceTexture : register(t0);
TextureCube prefilteredColorTexture : register(t1);
Texture2D<float4> preintegratedBRDFTexture : register(t2);

Texture2D<float4> diffuseTexture : register(t3);
Texture2D<float4> metallicRoughnessTexture : register(t4);
Texture2D<float4> normalTexture : register(t5);

SamplerState MinMagMipLinear : register(s0);
SamplerState MinMagLinearMipPointClamp : register(s1);
SamplerState ModelSampler : register(s2);

static const float PI = 3.14159265358979323846f;
static const float MAX_REFLECTION_LOD = 4.0;

cbuffer Transformation: register(b0)
{
    matrix World;
    matrix View;
    matrix Projection;
	float4 CameraPos;
}

cbuffer Lights : register(b1)
{
    float4 LightPositions[NUM_LIGHTS];
    float4 LightColors[NUM_LIGHTS];
    float4 LightAttenuations[NUM_LIGHTS];
}

cbuffer Material : register(b2)
{
    float4 Albedo;
	float Roughness;
	float Metalness;
}

struct VS_INPUT
{
    float3 Normal : NORMAL;
    float3 Pos : POSITION;
#ifdef HAS_TANGENT
    float4 Tangent : TANGENT;
#endif
    float2 Tex : TEXCOORD_0;
};

struct PS_INPUT
{
    float4 Pos : SV_POSITION;
    float2 Tex : TEXCOORD_0;
    float3 Normal : NORMAL;
	float4 WorldPos : POSITION;
#ifdef HAS_TANGENT
    float3 Tangent : TANGENT;
#endif
};

PS_INPUT vs_main(VS_INPUT input)
{
    PS_INPUT output = (PS_INPUT)0;
    output.Pos = mul(float4(input.Pos, 1.0f), World);
	output.WorldPos = output.Pos;
    output.Pos = mul(output.Pos, View);
    output.Pos = mul(output.Pos, Projection);
    output.Tex = input.Tex;
    output.Normal = normalize(mul(input.Normal, (float3x3)World));
#ifdef HAS_TANGENT
    output.Tangent = normalize(mul(input.Tangent, World).xyz);
#endif
    return output;
}

float3 h(float3 v, float3 l) 
{
    return normalize(v + l);
}

float normalDistribution(float3 n, float3 v, float3 l, float roughness)
{
    float roughnessSqr = pow(max(roughness, 0.01f), 2);
	return roughnessSqr / (PI * pow(pow(max(dot(n, h(v, l)), 0), 2) * (roughnessSqr - 1) + 1, 2));
}

float4 ndps_main(PS_INPUT input) : SV_TARGET
{
    return normalDistribution(normalize(input.Normal), normalize(CameraPos.xyz - input.WorldPos.xyz), normalize(LightPositions[0].xyz - input.WorldPos.xyz), Roughness);
}

float SchlickGGX(float3 n, float3 v, float k)
{
	float value = max(dot(n, v), 0);
	return value / (value * (1 - k) + k);
}

float geometry(float3 n, float3 v, float3 l, float roughness)
{
	float k = pow(roughness + 1, 2) / 8;
	return SchlickGGX(n, v, k) * SchlickGGX(n, l, k);
}

float4 gps_main(PS_INPUT input) : SV_TARGET
{
    return geometry(normalize(input.Normal), normalize(CameraPos.xyz - input.WorldPos.xyz), normalize(LightPositions[0].xyz - input.WorldPos.xyz), Roughness);
}

float3 fresnel(float3 n, float3 v, float3 l, float3 albedo, float metalness)
{
	float3 F0 = lerp(float3(0.04f, 0.04f, 0.04f), albedo, metalness);
    return (F0 + (1 - F0) * pow(1 - max(dot(h(v, l), v), 0), 5)) * sign(max(dot(l, n), 0));
}

float4 fps_main(PS_INPUT input) : SV_TARGET
{
    return float4(fresnel(normalize(input.Normal), normalize(CameraPos.xyz - input.WorldPos.xyz), normalize(LightPositions[0].xyz - input.WorldPos.xyz), Albedo.xyz, Metalness), 1.0f);
}

float3 BRDF(float3 p, float3 n, float3 v, float3 l, float3 albedo, float metalness, float roughness)
{
	float D = normalDistribution(n, v, l, roughness);
	float G = geometry(n, v, l, roughness);
	float3 F = fresnel(n, v, l, albedo, metalness);

    return (1 - F) * albedo / PI * (1 - metalness) + D * F * G / (0.001f + 4 * (max(dot(l, n), 0) * max(dot(v, n), 0)));
}

float Attenuation(float3 lightDir, float3 attenuation)
{
	float d = length(lightDir);
    float factor = attenuation[0] + attenuation[1] * d + attenuation[2] * d * d;
    return 1 / max(factor, 1e-9);
}

float3 LO_i(float3 p, float3 n, float3 v, uint lightIndex, float3 pos, float3 albedo, float metalness, float roughness)
{
    float3 lightDir = LightPositions[lightIndex].xyz - pos;
    float4 lightColor = LightColors[lightIndex];
    float atten = Attenuation(lightDir, LightAttenuations[lightIndex].xyz);
	float3 l = normalize(lightDir);
    return BRDF(p, n, v, l, albedo, metalness, roughness) * lightColor.rgb * atten * max(dot(l, n), 0) * lightColor.a;
}

float3 FresnelSchlickRoughnessFunction(float3 F0, float3 n, float3 v, float roughness)
{
    return (F0 + (max(1 - roughness, F0) - F0) * pow(1 - max(dot(n, v), 0), 5));
}

float3 Ambient(float3 n, float3 v, float3 albedo, float metalness, float roughness)
{
    float3 r = normalize(reflect(-v, n));
    float3 prefilteredColor = prefilteredColorTexture.SampleLevel(MinMagMipLinear, r, roughness * MAX_REFLECTION_LOD).xyz;
    float3 F0 = lerp(float3(0.04, 0.04, 0.04), albedo, metalness);
    float3 F = FresnelSchlickRoughnessFunction(F0, n, v, roughness);
    float2 envBRDF = preintegratedBRDFTexture.Sample(MinMagLinearMipPointClamp, float2(max(dot(n, v), 0.0), roughness)).xy;
    float3 specular = prefilteredColor * (F0 * envBRDF.x + envBRDF.y);

    float3 irradiance = irradianceTexture.Sample(MinMagMipLinear, n).xyz;
    return (1 - F) * irradiance * albedo * (1 - metalness) + specular;
}

float4 GetAlbedo(float2 uv)
{
    float4 albedo = Albedo;
#ifdef HAS_COLOR_TEXTURE
    albedo *= pow(diffuseTexture.Sample(ModelSampler, uv), 2.2f);
#endif
#ifdef HAS_OCCLUSION_TEXTURE
    albedo.xyz *= metallicRoughnessTexture.Sample(ModelSampler, uv).r;
#endif
    return albedo;
}

float2 GetMetalnessRoughness(float2 uv)
{
    float2 material = float2(Metalness, Roughness);
#ifdef HAS_METAL_ROUGH_TEXTURE
    material *= metallicRoughnessTexture.Sample(ModelSampler, uv).bg;
#endif
    return material.xy;
}

float4 ps_main(PS_INPUT input) : SV_TARGET
{
	float3 color1, color2, color3;
	float3 v = normalize(CameraPos.xyz - input.WorldPos.xyz);
    float3 n = normalize(input.Normal);

#ifdef HAS_EMISSIVE
    return diffuseTexture.Sample(ModelSampler, input.Tex);
#else
#ifdef HAS_NORMAL_TEXTURE
    float3 nm = (normalTexture.Sample(ModelSampler, input.Tex) * 2.0f - 1.0f).xyz;
    float3 tangent = normalize(input.Tangent);
    float3 binormal = cross(n, tangent);
    n = normalize(nm.x * tangent + nm.y * binormal + input.Normal);
#endif

#ifdef DOUBLE_SIDED
    if (dot(-v, n) > 0.0f)
        n = -n;
#endif

    float2 material = GetMetalnessRoughness(input.Tex);
    float metalness = material.x;
    float roughness = material.y;
    float4 albedo = GetAlbedo(input.Tex);

    float3 color = 0.0f;
    for (uint i = 0; i < NUM_LIGHTS; ++i)
        color += LO_i(input.WorldPos.xyz, n, v, i, input.WorldPos.xyz, albedo.rgb, metalness, roughness);

    float3 ambient = Ambient(n, v, albedo.rgb, metalness, roughness);

    return float4(color + ambient, albedo.a);
#endif
}

TextureCube cubeTexture : register(t0);

SamplerState samState : register(s0);

static const float PI = 3.14159265358979323846f;
static const int N1 = 200;
static const int N2 = 50;

cbuffer Transformation : register(b0)
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
    float3 Tex : TEXCOORD;
};

float3 Irradiance(float3 normal)
{
    float3 dir = abs(normal.z) < 0.999 ? float3(0.0, 0.0, 1.0) : float3(1.0, 0.0, 0.0);
    float3 tangent = normalize(cross(dir, normal));
    float3 bitangent = cross(normal, tangent);

    float3 irradiance = float3(0.0, 0.0, 0.0);
    for (int i = 0; i < N1; i++)
    {
        for (int j = 0; j < N2; j++)
        {
            float phi = i * (2 * PI / N1);
            float theta = j * (PI / 2 / N2);
            float3 tangentSample = float3(sin(theta) * cos(phi), sin(theta) * sin(phi), cos(theta));
            float3 sampleVec = tangentSample.x * tangent + tangentSample.y * bitangent + tangentSample.z * normal;
            irradiance += cubeTexture.Sample(samState, sampleVec).xyz * cos(theta) * sin(theta);
        }
    }
    irradiance = PI * irradiance / (N1 * N2);
    return irradiance;
}

PS_INPUT vs_main(VS_INPUT input)
{
    PS_INPUT output = (PS_INPUT) 0;
    output.Pos = mul(input.Pos, World);
    output.Pos = mul(output.Pos, View);
    output.Pos = mul(output.Pos, Projection);
    output.Tex = input.Pos.xyz;
    return output;
}

float4 ps_main(PS_INPUT input) : SV_TARGET
{
    return float4(Irradiance(input.Tex), 1.0f);
}
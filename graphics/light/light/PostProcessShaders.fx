Texture2D<float4> Texture : register(t0);

SamplerState samState : register(s0);

struct PS_INPUT
{
    float4 Pos : SV_POSITION;
    float2 Tex : TEXCOORD;
};

PS_INPUT vs_main(uint input : SV_VERTEXID)
{
    PS_INPUT output = (PS_INPUT)0;
    output.Tex = float2((input << 1) & 2, input & 2);
    output.Pos = float4(output.Tex.x * 2 - 1, -output.Tex.y * 2 + 1, 0, 1);
    return output;
}

float AverageLightness()
{
    return 0.01;
}

float Exposure()
{
    float l = AverageLightness();
    float keyValue = 1.03 - 2 / (2 + log10(l + 1));
    return keyValue / clamp(l, 1e-7, 1.0);
}

float3 Uncharted2Tonemap(float3 x)
{
    static const float a = 0.1;  // Shoulder Strength
    static const float b = 0.50; // Linear Strength
    static const float c = 0.1;  // Linear Angle
    static const float d = 0.20; // Toe Strength
    static const float e = 0.02; // Toe Numerator
    static const float f = 0.30; // Toe Denominator
                                 // Note: e/f = Toe Angle

    return ((x * (a * x + c * b) + d * e) / (x * (a * x + b) + d * f)) - e / f;
}

float3 TonemapFilmic(float3 color)
{
    static const float W = 11.2; // Linear White Point Value

    float e = Exposure();
    float3 curr = Uncharted2Tonemap(e * color);
    float3 whiteScale = 1.0f / Uncharted2Tonemap(W);
    return curr * whiteScale;
}


float4 ps_main(PS_INPUT input) : SV_TARGET
{
    float4 color = Texture.Sample(samState, input.Tex);
    return float4(TonemapFilmic(color.xyz), color.a);
}

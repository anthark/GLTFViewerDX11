cbuffer Transformation: register(b0)
{
    matrix World;
    matrix View;
    matrix Projection;
}

float4 main(float4 pos : POS) : SV_POSITION
{
    float4 output = float4(pos.xyz, 1.0f);
    output = mul(output, World);
    output = mul(output, View);
    output = mul(output, Projection);
    return output;
}

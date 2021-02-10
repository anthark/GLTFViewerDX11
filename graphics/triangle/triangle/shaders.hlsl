float4 vs_main(float4 pos : POS) : SV_POSITION
{
    return float4(pos.xyz, 1.0);
}

float4 ps_main(float4 pos : SV_POSITION) : SV_TARGET
{
  return float4(1.0, 0.0, 1.0, 1.0);
}

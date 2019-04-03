#include "default_signature.hlsli"

Texture2D<float4> tex : register(t1);
SamplerState sam;

[RootSignature(MyRS1)]
float4 main(float2 coord : TEXCOORD0) : SV_TARGET
{
	float4 val =  tex.Sample(sam, coord);
    float3 color = val.xyz * (1.0f - val.w);
    return float4(color, 1.0f);
}

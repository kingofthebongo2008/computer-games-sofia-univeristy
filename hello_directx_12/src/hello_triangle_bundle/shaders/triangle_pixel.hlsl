#include "default_signature.hlsli"

struct interpolated_value
{
    float4 m_position : SV_POSITION;
    float3 m_color    : TEXCOORD0;
};

//36 DWORDS, must match the root signature
cbuffer Constants : register(b9)
{
	float m_w;
	float m_h;
	float m_pad[2];
};

[RootSignature( MyRS3 ) ]
float4 main(interpolated_value v) : SV_TARGET0
{
	return float4(v.m_color, 0.0f);
}

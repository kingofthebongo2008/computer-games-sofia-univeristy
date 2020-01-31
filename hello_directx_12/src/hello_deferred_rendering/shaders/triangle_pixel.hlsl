#include "default_signature.hlsli"

struct interpolated_value
{
    float4 m_position               : SV_POSITION;
    float3 m_color                  : TEXCOORD0;
    nointerpolation uint4 m_domain	: TEXCOORD1;
};

[RootSignature( MyRS1 ) ]
float4 main(interpolated_value v) : SV_TARGET0
{
    float4 c = v.m_domain.xyzw / 255.0f;
	return float4(c.xyz, 1.0f);
}

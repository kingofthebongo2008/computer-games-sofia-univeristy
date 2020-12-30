#include "default_signature.hlsli"

struct interpolated_value
{
	float4 m_position  : SV_POSITION;
	float2 m_uv        : TEXCOORD0;
};


[RootSignature( MyRS3 ) ]
float4 main(interpolated_value v) : SV_TARGET0
{
	return float4(v.m_uv.xy, 0.0f, 0.0f);
}

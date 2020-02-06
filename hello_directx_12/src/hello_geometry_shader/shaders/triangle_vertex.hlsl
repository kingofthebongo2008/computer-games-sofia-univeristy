#include "default_signature.hlsli"

struct vertex_attributes
{
	float3 m_position				: POSITION0;
	float3 m_color					: TEXCOORD0;
	nointerpolation uint4 m_domain	: TEXCOORD1;
};

struct interpolated_value
{
	float4 m_position				: SV_POSITION;
    float3 m_color					: TEXCOORD0;
	nointerpolation uint4 m_domain	: TEXCOORD1;
};

[RootSignature( MyRS1 ) ]
interpolated_value main(in vertex_attributes v)
{
	interpolated_value r	= (interpolated_value)0;
	r.m_position			= float4(v.m_position, 1.0);
	r.m_color				= v.m_color;
	r.m_domain				= v.m_domain;

	return r;
}
#include "default_signature.hlsli"

struct vertex_attributes
{
	float4 m_position : POSITION0;
	float3 m_color	  : TEXCOORD0;
};

struct interpolated_value
{
	float4 m_position : SV_POSITION;
    float3 m_color    : TEXCOORD0;
};

[RootSignature( MyRS1 ) ]
interpolated_value main(in vertex_attributes v)
{
	interpolated_value r = (interpolated_value)0;
	r.m_position = v.m_position;
	r.m_color = v.m_color;

	return r;
}
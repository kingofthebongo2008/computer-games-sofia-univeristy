#include "default_signature.hlsli"

struct vertex_input
{
	float4 m_position : POSITION;
};

struct interpolated_value
{
	float4 m_position		 : SV_POSITION;
	float4 m_position_os     : POSITION;
};

cbuffer g_constants : register(b0)
{
	float4x4 m_view;
	float4x4 m_projection;
};


[RootSignature( MyRS1 ) ]
interpolated_value main(in vertex_input v)
{
	interpolated_value r = (interpolated_value)0;
	r.m_position	= mul(mul(v.m_position, m_view), m_projection);
	r.m_position_os	= v.m_position;

	return r;
}
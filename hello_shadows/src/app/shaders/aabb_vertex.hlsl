#include "default_signature.hlsli"

struct vertex_input
{
	float4 m_position : POSITION;
};

struct interpolated_value
{
	float4 m_position : SV_POSITION;
};

[RootSignature( MyRS1 ) ]
interpolated_value main(in vertex_input v)
{
	interpolated_value r = (interpolated_value)0;
	r.m_position		 = v.m_position;

	return r;
}
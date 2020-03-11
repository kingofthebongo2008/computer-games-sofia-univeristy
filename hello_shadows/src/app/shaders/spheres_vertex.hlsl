#include "default_signature.hlsli"
#include "constants.fxh"

struct vertex_input
{
	float4 m_position : POSITION;
};

struct interpolated_value
{
	float4 m_position     : SV_POSITION;
	float3 m_position_os  : POSITION;
	uint   m_vertex_id	  : ID;
};

[RootSignature( MyRS1 ) ]
interpolated_value main(in vertex_input v, in uint vid : SV_VERTEXID)
{
	interpolated_value r = (interpolated_value)0;
	r.m_position		 = mul(mul(v.m_position, m_view), m_projection);
	r.m_position_os		 = float3(2, 2, 2);// v.m_position;
	r.m_vertex_id		 = vid;

	return r;
}
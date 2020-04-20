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

ByteAddressBuffer Geometry : register(t0);


[RootSignature( MyRS1 ) ]
interpolated_value main(in uint vid : SV_VERTEXID)
{
	interpolated_value r = (interpolated_value)0;
	r.m_position		 = 0;
	r.m_position_os		 = asfloat(Geometry.Load3(m_draw_argument0));
	r.m_vertex_id		 = vid;

	return r;
}
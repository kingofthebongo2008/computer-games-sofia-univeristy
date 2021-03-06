#include "default_signature.hlsli"

struct vertex_attributes
{
	float4 m_position				: SV_POSITION;
    float3 m_color					: TEXCOORD0;
	nointerpolation uint4 m_domain	: TEXCOORD1;
	nointerpolation uint  m_startid : TEXCOORD2;
};

struct vertex_input
{

};

[maxvertexcount(3)]
void    main(triangle vertex_input input[3], inout TriangleStream<vertex_attributes> outputStream )
{
	vertex_attributes o0;
	vertex_attributes o1;
	vertex_attributes o2;

	o0.m_position	= float4(0.0f, 0.5f, 0.5f, 1);
	o0.m_color		= float3(1.0f, 0.0f, 0.0f);
	o1.m_position	= float4(-0.5f, 0.0f, 0.5f, 1);
	o1.m_color		= float3(0.0f, 1.0f, 0.0f);
	o2.m_position	= float4(0.5f, 0.0f, 0.5f, 1);
	o2.m_color		= float3(0.0f, 0.0f, 1.0f);

	o0.m_domain		= 255;
	o1.m_domain		= 255;
	o2.m_domain		= 255;

	o0.m_startid	= 0;
	o1.m_startid	= 0;
	o2.m_startid	= 0;

	outputStream.Append(o0);
	outputStream.Append(o1);
	outputStream.Append(o2);
}


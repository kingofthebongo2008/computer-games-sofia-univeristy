#include "default_signature.hlsli"

struct vertex_attributes
{
	float4 m_position : SV_POSITION;
    float3 m_color	  : TEXCOORD0;
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

	o0.m_position	= float4(0.0f, 0.5f, 0.5f, 1.0f);
	o0.m_color		= float3(1.0f, 0.0f, 0.0f);
	o1.m_position	= float4(-0.5f, 0.0f, 0.5f, 1.0f);
	o1.m_color		= float3(0.0f, 1.0f, 0.0f);
	o2.m_position	= float4(0.5f, 0.0f, 0.5f, 1.0f);
	o2.m_color		= float3(0.0f, 0.0f, 1.0f);

	outputStream.Append(o0);
	outputStream.Append(o1);
	outputStream.Append(o2);
}


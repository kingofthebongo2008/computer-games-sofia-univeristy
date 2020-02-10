#include "default_signature.hlsli"

struct vertex_attributes
{
	float4 m_position                   : SV_POSITION;
	float3 m_color                      : TEXCOORD0;
	uint  m_view_port					: SV_ViewportArrayIndex;
};

struct vertex_input
{
	float4 m_position                   : SV_POSITION;
	float3 m_color                      : TEXCOORD0;
};

static const float4x4 transforms[2] =
{
	{ float4(1,0,0,0),	float4(0,1,0,0), float4(0,0,1,0), float4(0,0,0,1) },
	{ float4(0,-1,0,0), float4(1,0,0,0), float4(0,0,1,0), float4(0,0,0,1) }
};

[maxvertexcount( 3 * 4) ]
void    main(triangle vertex_input input[3], inout TriangleStream<vertex_attributes> outputStream )
{
	vertex_attributes o0;
	vertex_attributes o1;
	vertex_attributes o2;

	o0.m_position	= mul(input[0].m_position, transforms[0]);
	o1.m_position   = mul(input[1].m_position, transforms[0]);
	o2.m_position	= mul(input[2].m_position, transforms[0]);

	o0.m_view_port  = 0;
	o1.m_view_port	= 0;
	o2.m_view_port	= 0;

	o0.m_color		= input[0].m_color;
	o1.m_color		= input[1].m_color;
	o2.m_color		= input[2].m_color;

	outputStream.Append(o0);
	outputStream.Append(o1);
	outputStream.Append(o2);

	outputStream.RestartStrip();

	o0.m_position  = mul(input[0].m_position, transforms[1]);
	o1.m_position  = mul(input[1].m_position, transforms[1]);
	o2.m_position  = mul(input[2].m_position, transforms[1]);

	o0.m_view_port = 1;
	o1.m_view_port = 1;
	o2.m_view_port = 1;

	outputStream.Append(o0);
	outputStream.Append(o1);
	outputStream.Append(o2);

	outputStream.RestartStrip();

	o0.m_position = mul(input[0].m_position, transforms[0]);
	o1.m_position = mul(input[1].m_position, transforms[0]);
	o2.m_position = mul(input[2].m_position, transforms[0]);

	o0.m_view_port = 2;
	o1.m_view_port = 2;
	o2.m_view_port = 2;

	outputStream.Append(o0);
	outputStream.Append(o1);
	outputStream.Append(o2);

	outputStream.RestartStrip();

	o0.m_position = mul(input[0].m_position, transforms[1]);
	o1.m_position = mul(input[1].m_position, transforms[1]);
	o2.m_position = mul(input[2].m_position, transforms[1]);

	o0.m_view_port = 3;
	o1.m_view_port = 3;
	o2.m_view_port = 3;

	outputStream.Append(o0);
	outputStream.Append(o1);
	outputStream.Append(o2);

	outputStream.RestartStrip();
}


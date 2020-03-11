#include "default_signature.hlsli"
#include "constants.fxh"

struct interpolated_value
{
    float4 m_position     : SV_POSITION;
	float2 m_clip_space	  : TEXCOORD0;
};

#include "default_signature.hlsli"
struct vertex_input
{
	float4 m_position     : SV_POSITION;
	float4 m_position_os  : POSITION;
};

[maxvertexcount(4)]
void    main(point vertex_input input[1], inout TriangleStream<interpolated_value> outputStream )
{
	float3	camera_position = m_camera_position.xyz;// -float3 (m_view[0].w, m_view[1].w, m_view[2].w);
	float halfWidth			= 0.5;

	/*

	float3 forward			= normalize(input[0].m_position_os.xyz - camera_position);
	float3 up				= float3(0.0f, 1.0f, 0.0f);
	float3 temp				= normalize(cross(up, forward));
	float3 right			= normalize(cross(up, temp));

	right					= right * halfWidth;
	up						= normalize( cross(right, forward) ) * halfWidth;
	*/

	float3 up				= float3(0.0f, 1.0f, 0.0f);
	float3 eye_direction	= normalize(input[0].m_position_os.xyz - camera_position);
	float3 r2				= normalize(eye_direction);
	float3 r0				= normalize(cross(up, eye_direction));
	float3 r1				= cross(r0, r2);
	float3 negate			= -camera_position;
	
	float3 right2			= r0	* halfWidth;
	float3 up2				= r1	* halfWidth;

	float3 vert[4];

	vert[0]					= input[0].m_position_os.xyz - right2 - up2; // Get bottom left vertex
	vert[1]					= input[0].m_position_os.xyz + right2 - up2; // Get bottom right vertex
	vert[2]					= input[0].m_position_os.xyz - right2 + up2; // Get top left vertex
	vert[3]					= input[0].m_position_os.xyz + right2 + up2; // Get top right vertex

	float2 texCoord[4];
	texCoord[0] = float2(-1, -1);
	texCoord[1] = float2(1, -1);
	texCoord[2] = float2(-1, 1);
	texCoord[3] = float2(1, 1);

	interpolated_value v0;
	interpolated_value v1;
	interpolated_value v2;
	interpolated_value v3;

	v0.m_position	= mul(mul(float4(vert[0], 1.0f), m_view), m_projection);
	v1.m_position	= mul(mul(float4(vert[1], 1.0f), m_view), m_projection);
	v2.m_position	= mul(mul(float4(vert[2], 1.0f), m_view), m_projection);
	v3.m_position	= mul(mul(float4(vert[3], 1.0f), m_view), m_projection);

	v0.m_clip_space = texCoord[0];
	v1.m_clip_space = texCoord[1];
	v2.m_clip_space = texCoord[2];
	v3.m_clip_space = texCoord[3];

	outputStream.Append(v1);
	outputStream.Append(v0);
	outputStream.Append(v3);
	outputStream.Append(v2);
	//outputStream.RestartStrip();
	//outputStream.Append(v3);
	//outputStream.Append(v0);
	//outputStream.Append(v2);

	/*
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

	outputStream.Append(o1);
	outputStream.Append(o2);
	outputStream.Append(o3);
	*/
}



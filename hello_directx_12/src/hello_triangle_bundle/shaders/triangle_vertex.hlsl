#include "default_signature.hlsli"

struct interpolated_value
{
	float4 m_position : SV_POSITION;
    float2 m_uv		  : TEXCOORD0;
};

[RootSignature( MyRS3 ) ]
interpolated_value main(uint v : SV_VERTEXID)
{
	interpolated_value r = (interpolated_value)0;

	float2 tmp = float2((v << 1) & 2, v & 2);
	float2 pos	= tmp;
	float2 uv	= tmp;

	r.m_position = (float4(pos * float2(2, -2) - float2(1, -1), 0, 1));
	r.m_uv		 = uv;

	return	r;
}
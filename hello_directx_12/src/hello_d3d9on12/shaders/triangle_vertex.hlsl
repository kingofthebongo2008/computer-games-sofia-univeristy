#include "default_signature.hlsli"

struct interpolated_value
{
	float4 m_position  : SV_POSITION;
        float2 m_uv        : TEXCOORD0;
};

ByteAddressBuffer geom : register(t2);

float4x4 g_mvp		   : register(b0);



[RootSignature( MyRS3 ) ]
interpolated_value main(uint v : SV_VERTEXID)
{
	interpolated_value r = (interpolated_value)0;

	uint4 vtx			 = geom.Load4(v * 16);
	r.m_position		 = mul(float4(asfloat(vtx.xy), 0, 1), g_mvp);
	r.m_uv			     = asfloat(vtx.zw);

	return r;
}
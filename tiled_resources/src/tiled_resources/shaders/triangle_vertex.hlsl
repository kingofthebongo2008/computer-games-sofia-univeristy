#include "default_signature.hlsli"

struct interpolated_value
{
	float4 m_position : SV_POSITION;
    float3 m_color    : TEXCOORD0;
};

struct value
{
    float3 m_position : POSITION;
};


[RootSignature( MyRS1 ) ]
interpolated_value main(in value v)
{
	interpolated_value r = (interpolated_value)0;
    r.m_position = float4(v.m_position, 1.0f);
    r.m_color    = float3(0.0f, 1.0f, 0.0f);
	return r;
}
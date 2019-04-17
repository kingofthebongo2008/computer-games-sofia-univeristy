#include "default_signature.hlsli"

struct interpolated_value
{
	float4 m_position : SV_POSITION;
    float3 m_color    : TEXCOORD0;
};

[RootSignature( MyRS1 ) ]
interpolated_value main(uint v : SV_VERTEXID)
{
	interpolated_value r = (interpolated_value)0;
	r.m_position = float4(0.0f, 0.0f, 0.0f, 1.0f);

	if (v == 0)
	{
		r.m_position    = float4(0.0f, 0.5f, 0.5f, 1.0f);
        r.m_color       = float3(1.0f, 0.0f, 0.0f);
		return r;
	}

	if (v == 1)
	{
		r.m_position    = float4(-0.5f, 0.0f, 0.5f, 1.0f);
        r.m_color       = float3(0.0f, 1.0f, 0.0f);
		return r;
	}

	if (v == 2)
	{
		r.m_position    = float4(0.5f, 0.0f, 0.5f, 1.0f);
        r.m_color       = float3(0.0f, 0.0f, 1.0f);
		return r;
	}

	return r;
}
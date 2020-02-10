#include "default_signature.hlsli"

struct interpolated_value
{
    float4 m_position                   : SV_POSITION;
    float3 m_color                      : TEXCOORD0;
    uint  m_view_port					: SV_ViewportArrayIndex;
};

[RootSignature( MyRS1 ) ]
float4 main(interpolated_value v) : SV_TARGET0
{
    //float4 c = v.m_domain.xyzw / 255.0f;
    float4 c;

    if (v.m_view_port == 0)
    {
        c = float4(1, 0, 0, 1);// v.m_domain.xyzw / 255.0f;
    }
    else if (v.m_view_port == 1)
    {
        c = float4(1, 1, 0, 1);// v.m_domain.xyzw / 255.0f;
    }
    else if (v.m_view_port == 2)
    {
        c = float4(1, 1, 1, 1);// v.m_domain.xyzw / 255.0f;
    }
    else
    {
        c = float4(0, 1, 0, 1);// v.m_domain.xyzw / 255.0f;
    }

	return float4(c.xyz, 1.0f);
}

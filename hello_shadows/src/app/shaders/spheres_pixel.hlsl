#include "default_signature.hlsli"

struct interpolated_value
{
    float4 m_position     : SV_POSITION;
    float2 m_clip_space	  : TEXCOORD0;
};

[RootSignature( MyRS1 ) ]
float4 main(interpolated_value v) : SV_TARGET0
{
    float   alpha = 1;
    float3  color = float3(0, 1, 0) * alpha;

    float3 n      = float3(v.m_clip_space.x, v.m_clip_space.y, 0);
    float r2      = dot(n.xy, n.xy);

    // if the texel is not inside the sphere
    if (r2 > 1.0f)
        discard;

    return float4(color, alpha);
}

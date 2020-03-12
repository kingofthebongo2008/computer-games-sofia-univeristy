#include "default_signature.hlsli"
#include "constants.fxh"

struct interpolated_value
{
    float4 m_position     : SV_POSITION;
    float2 m_clip_space	  : TEXCOORD0;
};

float4 argb_to_float4(uint v)
{
    uint a = (v >> 24) & 0xff;
    uint r = (v >> 16) & 0xff;
    uint g = (v >> 8) & 0xff;
    uint b = (v >> 0) & 0xff;

    uint4   rgba_i  = uint4(r, g, b, a);
    float4  rgba    = rgba_i / 255.0f;

    return rgba;
}

[RootSignature( MyRS1 ) ]
float4 main(interpolated_value v) : SV_TARGET0
{
    float4  color = argb_to_float4(m_draw_argument1);

    //float   alpha = 1;
    //float3  color = float3(0, 1, 0) * alpha;

    return color;
}

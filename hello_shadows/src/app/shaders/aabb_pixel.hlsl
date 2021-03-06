#include "default_signature.hlsli"

struct interpolated_value
{
    float4 m_position     : SV_POSITION;
    float4 m_position_    : TEXCOORD0;
};

[RootSignature( MyRS1 ) ]
float4 main(interpolated_value v) : SV_TARGET0
{
    float   alpha = 1;
    float3  color = float3(1, 0, 0) * alpha;
    return float4(color, alpha);
}

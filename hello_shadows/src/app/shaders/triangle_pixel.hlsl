#include "default_signature.hlsli"
#include "constants.fxh"

struct interpolated_value
{
    float4 m_position     : SV_POSITION;
    float4 m_position_    : TEXCOORD0;
};

[RootSignature( MyRS1 ) ]
float4 main(interpolated_value v) : SV_TARGET0
{
	return float4(v.m_position_.xyz / v.m_position_.w, v.m_position_.w);
}

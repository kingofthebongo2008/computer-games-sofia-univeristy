#include "default_signature.hlsli"
#include "default_samplers.hlsli"

struct interpolated_value
{
	float4 m_position  : SV_POSITION;
	float2 m_uv        : TEXCOORD0;
};

Texture2D<float> g_font_texture : register(t1);

[RootSignature( MyRS3 ) ]
float4 main(interpolated_value v) : SV_TARGET0
{
	float sample	      = g_font_texture.Sample(g_linear_clamp, v.m_uv).r;
	float scale			  = 1.0 / fwidth(sample); 
	float signedDistance = (sample - 0.5) * scale; 

	float color = clamp(signedDistance + 0.5, 0.0, 1.0); 
	float alpha = clamp(signedDistance + 0.5 + scale * 0.125, 0.0, 1.0);

	return float4(color.x, 0.0, 0.0f, alpha);
}

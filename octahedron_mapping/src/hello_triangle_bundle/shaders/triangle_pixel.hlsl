#include "default_signature.hlsli"
#include "default_samplers.hlsli"

struct interpolated_value
{
	float4 m_position : SV_POSITION;
	float2 m_uv		  : TEXCOORD0;
};

//36 DWORDS, must match the root signature
cbuffer Constants : register(b9)
{
	float m_w;
	float m_h;
	float m_pad[2];
};

float signNotZero(float value)
{
	return value >= 0.0 ? 1.0 : -1.0;
}

float2 signNotZero(float2 value)
{
	return value >= 0.0 ? 1.0 : -1.0;
}

float3 signNotZero(float3 value)
{
	return value >= 0.0 ? 1.0 : -1.0;
}

/** Assumes that v is a unit vector. The result is an octahedral vector on the [-1, +1] square. */
float2 octEncode(in float3 v) 
{
	float l1norm = abs(v.x) + abs(v.y) + abs(v.z);
	float2 result = v.xy / l1norm;
	if (v.z < 0.0)
	{
		result = (1.0 - abs(result.yx)) * signNotZero(result.xy);
	}
	return result;
}

/** Returns a unit vector. Argument o is an octahedral vector packed via octEncode,
	on the [-1, +1] square*/
float3 octDecode(float2 o)
{
	float3 v = float3(o.x, o.y, 1.0 - abs(o.x) - abs(o.y));
	if (v.z < 0.0)
	{
		v.xy = (1.0 - abs(v.yx)) * signNotZero(v.xy);
	}
	return normalize(v);
}

bool octFlipped(float2 r) {
	float t = max(abs(r.x), abs(r.y));
	return t >= 1.0;
}

// Example of computing mirrored repeat sampling 
// of an octahedron map with a small texel offset.
// Note this is not designed to solve the double wrap case.
// The "base" is as computed by Oct3To2() above.
//float2 coord = base + float2(-2.0, 2.0);    // 2 VALU
//coord = OctFlipped(coord) ? -coord : coord; // 4 VALU
//coord = coord * 0.5 + 0.5;

TextureCube<float4> g_texture : register(t0);
[RootSignature( MyRS3 ) ]
float4 main(interpolated_value v) : SV_TARGET0
{
	float2 uv			= v.m_uv * 2.0 - 1.0f;
	float3 co			= octDecode(uv);
	float4 fragColor	= float4(g_texture.Sample(g_linear_clamp, co).rgb, 1.0);
	return fragColor;
}

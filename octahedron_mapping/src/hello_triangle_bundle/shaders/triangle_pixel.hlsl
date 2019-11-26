#include "default_signature.hlsli"

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

/*
float2 octahedral_mapping(float3 co)
{
	// projection onto octahedron
	co /= dot(vec3(1), abs(co));

	// out-folding of the downward faces
	if (co.y < 0.0) {
		co.xy = (1.0 - abs(co.zx)) * sign(co.xz);
	}

	// mapping to [0;1]ˆ2 texture space
	return co.xy * 0.5 + 0.5;
}

float3 octahedral_unmapping(vec2 co)
{
	co = co * 2.0 - 1.0;

	vec2 abs_co = abs(co);
	vec3 v = vec3(co, 1.0 - (abs_co.x + abs_co.y));

	if (abs_co.x + abs_co.y > 1.0) {
		v.xy = (abs(co.yx) - 1.0) * -sign(co.xy);
	}

	return v;
}

void mainImage(out vec4 fragColor, in vec2 fragCoord)
{
	vec2 uv = fragCoord.xy / iResolution.xy;
	//uv = 0.5 + vec2(uv - 0.5)*(1.6+0.6*sin(iTime));

	//edge mirroring
	vec2 m = abs(uv - 0.5) + 0.5;
	vec2 f = floor(m);
	float x = f.x - f.y;
	bool mirror = (x != 0.0);

	if (mirror) {
		uv.xy = 1.0 - uv.xy;
	}

	uv = fract(uv);

	vec3 co = octahedral_unmapping(uv);

	fragColor = vec4(texture(iChannel0, co).rgb, 1.0);
}
*/

[RootSignature( MyRS3 ) ]
float4 main(interpolated_value v) : SV_TARGET0
{
   float  value			= 1;
   float4 fragColor		= float4(1,0,0, 1.0);
   return fragColor;
}

#include "default_signature.hlsli"

cbuffer CB0
{
    float2 scale;
    float2 offset;
};

struct VS_IN
{
    float2 pos : POSITION;
    float2 tex : TEXCOORD0;
};

struct VS_OUT
{
    float2 tex : TEXCOORD0;
    float4 pos : SV_POSITION;
};

[RootSignature(MyRS1)]
VS_OUT main(VS_IN input)
{
    VS_OUT output;
    output.tex = input.tex;
    output.pos.xy = input.pos * scale + offset;
    output.pos.z = 0.5f;
    output.pos.w = 1.0f;
	return output;
}

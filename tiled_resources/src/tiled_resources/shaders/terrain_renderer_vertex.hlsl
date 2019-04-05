#include "default_signature.hlsli"
#include "pass_constants.hlsli"

struct VS_IN
{
    float3 pos : POSITION;
};

struct VS_OUT
{
    float3 tex : TEXCOORD0;
    float3 utan : TANGENT0;
    float3 vtan : TANGENT1;
    float4 pos : SV_POSITION;
};

[RootSignature(MyRS1)]
VS_OUT main(VS_IN input)
{
    float dataScaleFactor = 10.0f;

    VS_OUT output;
    output.tex = normalize(input.pos);
    output.utan = normalize(float3(-input.pos.y, input.pos.x, 0.0f));
    output.vtan = normalize(cross(input.pos, output.utan));

    float4 pos = float4(input.pos, 1.0f);

    float offset = length(pos.xyz) - 1.0f;

    offset /= dataScaleFactor;
    offset *= m_ScaleFactor;
    
    pos.xyz = normalize(pos.xyz) * (1.0f + offset);
    pos = mul(pos, ViewMatrix);
    pos = mul(pos, ProjectionMatrix);
    output.pos = pos;

    return output;
}

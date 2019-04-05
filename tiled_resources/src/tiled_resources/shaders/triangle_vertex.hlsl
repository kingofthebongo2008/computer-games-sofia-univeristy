#include "default_signature.hlsli"
#include "pass_constants.hlsli"

struct interpolated_value
{
	float4 m_position   : SV_POSITION;
    float3 m_uv         : TEXCOORD0;
};

struct value
{
    float3 m_position : POSITION;
};



float4x4 makeTestView()
{
    float4x4 view;

    view._11 = -0.943035305;
    view._12 = -0.301380903;
    view._13 = -0.140902951;
    view._14 = 0.000000000;
    view._21 = -0.175059080;
    view._22 = 0.0893634558;
    view._23 = 0.980493903;
    view._24 = 0.000000000;
    view._31 = -0.282910585;
    view._32 = 0.949306846;
    view._33 = -0.137032211;
    view._34 = 0.000000000;
    view._41 = 5.96046448e-08;
    view._42 = -3.27825546e-07;
    view._43 = -2.12156677;
    view._44 = 1.00000000;

    return view;
}

float4x4 makeTestProjection()
{
    float4x4 view;

    view._11 = -0.943035305;
    view._12 = -0.301380903;
    view._13 = -0.140902951;
    view._14 = 0.000000000;
    view._21 = -0.175059080;
    view._22 = 0.0893634558;
    view._23 = 0.980493903;
    view._24 = 0.000000000;
    view._31 = -0.282910585;
    view._32 = 0.949306846;
    view._33 = -0.137032211;
    view._34 = 0.000000000;
    view._41 = 5.96046448e-08;
    view._42 = -3.27825546e-07;
    view._43 = -2.12156677;
    view._44 = 1.00000000;

    return view;
}

[RootSignature( MyRS1 ) ]
interpolated_value main(in value input)
{
    float4x4 projection = ProjectionMatrix;
    float4x4 view       = ViewMatrix;

	
	interpolated_value r = (interpolated_value)0;

	float dataScaleFactor = 10.0f;
    float scaleFactor     = m_ScaleFactor;

	float4 pos	 = float4(input.m_position, 1.0f);
	float offset = length(pos.xyz) - 1.0f;

	offset /= dataScaleFactor;
	offset *= scaleFactor;

	pos.xyz = normalize(pos.xyz) * (1.0f + offset);
	pos = mul(pos, view);           //column major
	pos = mul(pos, projection);     //row major

    r.m_position    = pos;
    r.m_uv          = normalize(input.m_position);
 

	return r;
}
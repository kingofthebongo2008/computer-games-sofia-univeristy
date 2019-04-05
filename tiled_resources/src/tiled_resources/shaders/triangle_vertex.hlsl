#include "default_signature.hlsli"

struct interpolated_value
{
	float4 m_position : SV_POSITION;
    float3 m_color    : TEXCOORD0;
};

struct value
{
    float3 m_position : POSITION;
};

[RootSignature( MyRS1 ) ]
interpolated_value main(in value input)
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


	float4x4 projection;

	projection._11 = 1.11935902;
	projection._12 = 0.000000000;
	projection._13 = 0.000000000;
	projection._14 = 0.000000000;
	projection._21 = 0.000000000;
	projection._22 = 1.42814779;
	projection._23 = 0.000000000;
	projection._24 = 0.000000000;
	projection._31 = 0.000000000;
	projection._32 = 0.000000000;
	projection._33 = - 1.00001526;
	projection._34 = - 1.00000000;
	projection._41 = 0.000000000;
	projection._42 = 0.000000000;
	projection._43 = - 0.00390630960;
	projection._44 = 0.000000000;

	interpolated_value r = (interpolated_value)0;

	float dataScaleFactor = 10.0f;
	float scaleFactor	  = 10.0f;

	float4 pos	 = float4(input.m_position, 1.0f);
	float offset = length(pos.xyz) - 1.0f;

	offset /= dataScaleFactor;
	offset *= scaleFactor;

	pos.xyz = normalize(pos.xyz) * (1.0f + offset);
	pos = mul(pos, view);
	pos = mul(pos, projection);


    r.m_position = pos;
    r.m_color    = float3(0.0f, 1.0f, 0.0f);
	return r;
}
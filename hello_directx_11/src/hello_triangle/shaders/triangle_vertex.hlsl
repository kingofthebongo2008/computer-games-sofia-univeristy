struct interpolated_value
{
	float4 m_position : SV_POSITION;
    float3 m_color    : TEXCOORD0;
};

static const float3 red    = float3(1, 0, 0);
static const float3 green  = float3(0, 1, 0);
static const float3 blue   = float3(0, 0, 1);

interpolated_value main(uint v : SV_VERTEXID)
{
	interpolated_value r;
	r.m_position = float4(0.0f, 0.0f, 0.0f, 1.0f);

	if (v == 0)
	{
		r.m_position = float4(0.0f, 0.5f, 0.5f, 1.0f);
        r.m_color    = red;
		return r;
	}

	if (v == 1)
	{
		r.m_position = float4(-0.5f, 0.0f, 0.5f, 1.0f);
        r.m_color    = green;
		return r;
	}

	if (v == 2)
	{
		r.m_position = float4(0.5f, 0.0f, 0.5f, 1.0f);
        r.m_color   = blue;
		return r;
	}

	return r;
}
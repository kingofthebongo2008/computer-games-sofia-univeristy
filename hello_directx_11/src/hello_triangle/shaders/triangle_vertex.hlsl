struct interpolated_value
{
	float4 m_position : SV_POSITION;
};

interpolated_value main(uint v : SV_VERTEXID)
{
	interpolated_value r;
	r.m_position = float4(0.0f, 0.0f, 0.0f, 1.0f);

	if (v == 0)
	{
		r.m_position = float4(0.0f, 0.5f, 0.0f, 1.0f);
		return r;
	}

	if (v == 1)
	{
		r.m_position = float4(-0.5f, 0.0f, 0.0f, 1.0f);
		return r;
	}

	if (v == 2)
	{
		r.m_position = float4(0.5f, 0.0f, 0.0f, 1.0f);
		return r;
	}

	return r;
}
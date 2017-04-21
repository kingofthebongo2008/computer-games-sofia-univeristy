struct interpolated_value
{
	float4 m_position : SV_POSITION;
};

struct vector4
{
	float4 value;
};

float4x4 make_float4x4_translation(float3 v)
{
	float4x4 m = { 
					{ 1.0f, 0.0f, 0.0f, 0.0f},
					{ 0.0f, 1.0f, 0.0f, 0.0f},
					{ 0.0f, 0.0f, 1.0f, 0.0f},
					{ v.x, v.y, v.z, 1.0f }
	};
	return m;
}

float4x4 make_float4x4_rotation_y(float radians)
{
	float s = sin(radians);
	float c = cos(radians);

	float4x4 m = {
		{ c,	0.0f, s,	0.0f },
		{ 0.0f, 1.0f, 0.0f, 0.0f },
		{ -s,	0.0f, c,	0.0f },
		{ 0.0f, 0.0f, 0.0f, 1.0f }
	};
	return m;
}

float4x4 make_float4x4_rotation_z(float radians)
{
	float s = sin(radians);
	float c = cos(radians);

	float4x4 m = {
		{ c,	s, 1.0f,	0.0f },
		{ -s,	c, 0.0f,	0.0f },
		{ 0.0f,	0.0f, 1.0f,	0.0f },
		{ 0.0f, 0.0f, 0.0f, 1.0f }
	};
	return m;
}

float4x4 make_float4x4_scale(float scale)
{
	float s = scale;
	float4x4 m = {
		{ s,	0.0f, 0.0f,	0.0f },
		{ 0.0f,	s, 0.0f,	0.0f },
		{ 0.0f,	0.0f, s,	0.0f },
		{ 0.0f, 0.0f, 0.0f, 1.0f }
	};
	return m;
}

interpolated_value main(uint v : SV_VERTEXID)
{
	interpolated_value r;

	const float		pi			 = 3.1415f;
	float4			position	 = float4(0.0f, 0.0f, 0.0f, 1.0f);
	float4x4		translation  = make_float4x4_translation(float3(0.0, -0.5f, 0.0f));
	float4x4		rotation	 = make_float4x4_rotation_z( pi / 2.0f);
	float4x4		scale		 = make_float4x4_scale(0.5f);
	

	if (v == 0)
	{
		position = float4(0.0f, 0.5f, 0.5f, 1.0f);
	}
	else if (v == 1)
	{
		position = float4(-0.5f, 0.0f, 0.5f, 1.0f);
	}
	else if (v == 2)
	{
		position = float4(0.5f, 0.0f, 0.5f, 1.0f);
	}

	position		= mul(position, scale);
	position		= mul(position, rotation);
	position		= mul(position, translation);

	r.m_position	= position;
	return r;
}
struct interpolated_value
{
	float4 m_position : SV_POSITION;
	uint   instanceid : SV_INSTANCEID;
};

static const float4 colors[4]=
{	
	{ 0.0f, 0.0f, 1.0f, 0.0f},
	{ 0.0f, 1.0f, 0.0f, 0.0f },
	{ 0.3f, 0.2f, 0.0f, 0.0f },
	{ 0.2f, 0.0f, 0.3f, 0.0f }
};

float4 main(interpolated_value v) : SV_TARGET0
{
	return colors[v.instanceid];
}

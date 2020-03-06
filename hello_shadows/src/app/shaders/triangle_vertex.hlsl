#include "default_signature.hlsli"

struct interpolated_value
{
	float4 m_position	: SV_POSITION;
    float4 m_position_  : TEXCOORD0;
};

namespace lispsm
{
	struct view_transform
	{
		float3 m_position;
		float3 m_direction;
		float3 m_up;
	};

	struct perspective_transform
	{
		float  m_aspect;
		float  m_fov_y;
		float  m_near;
		float  m_far;
	};

	float4x4 perspective_matrix(const perspective_transform c)
	{
		float4x4 r;

		float sinFov = sin(c.m_fov_y / 2.0f);
		float cosFov = cos(c.m_fov_y / 2.0f);

		float height = cosFov / sinFov;
		float width = height / c.m_aspect;
		float nearz = c.m_near;
		float farz = c.m_far;
		float range = farz / (farz - nearz);

		r[0] = float4(width, 0, 0, 0);
		r[1] = float4(0, height, 0, 0);
		r[2] = float4(0, 0, range, 1);
		r[3] = float4(0, 0, -range * nearz, 0);

		return r;
	}

	float Pi()
	{
		return 3.14159265358979323846f;
	}

	float radians(float degrees)
	{
		return ((degrees) / 180.0f) * Pi();
	}

	perspective_transform make_perspective_transform()
	{
		perspective_transform r;

		r.m_aspect = 16.0f / 9.0f;
		r.m_fov_y  = radians(75.0f);
		r.m_near   = 64000.0f;	// 1.0f;
		r.m_far    = 1.0f;		// 64000.0f;

		return r;
	}
}


[RootSignature( MyRS1 ) ]
interpolated_value main(uint v : SV_VERTEXID)
{
	interpolated_value r = (interpolated_value)0;
	r.m_position = float4(0.0f, 0.0f, 0.0f, 1.0f);

	float z = 0.5f;
	if (v == 0)
	{
		r.m_position    = float4(-16000, -16000, z, 1.0f);
	}

	if (v == 1)
	{
		r.m_position    = float4(-16000, 16000, z, 1.0f);
	}

	if (v == 2)
	{
		r.m_position    = float4(16000, 16000.0, z, 1.0f);
	}

	float4x4 perspective = lispsm::perspective_matrix(lispsm::make_perspective_transform());
	r.m_position		 = mul(r.m_position, perspective);
	r.m_position_		 = r.m_position;
	return r;
}
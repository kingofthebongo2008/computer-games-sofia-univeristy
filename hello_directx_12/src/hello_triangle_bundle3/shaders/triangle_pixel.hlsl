#include "default_signature.hlsli"
#include "default_samplers.hlsli"

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

static const float3 CameraCenter	= float3(0.0, 0.0, 0.0);

float3 opU(float3 d, float iResult, float v)
{
	return (iResult < d.y) ? float3(d.x, iResult, v) : d;
}

static const float MaxDistance = 1e10;

// Triangle:        https://www.shadertoy.com/view/MlGcDz
float iTriangle(in float3 ro, in float3 rd, in float2 distBound, inout float u_, inout float v_, in float3 v0, in float3 v1, in float3 v2)
{
	float3 v1v0 = v1 - v0;
	float3 v2v0 = v2 - v0;
	float3 rov0 = ro - v0;

	float3  n	= cross(v1v0, v2v0);
	float3  q	= cross(rov0, rd);
	float d		= 1.0 / dot(rd, n);
	float u		= d * dot(-q, v2v0);
	float v		= d * dot(q, v1v0);
	float t		= d * dot(-n, rov0);

	if ( d < 0 || ( u  < 0. ) || (v  < 0.) || (u + v)> 1. || t < distBound.x || t > distBound.y )
	{
		return MaxDistance;
	}
	else
	{
        u_      = saturate(u);
        v_      = saturate(v);
		return t;
	}
}

static const float attributes0_u[3] =
{
    float(0),
    float(1),
    float(1),
};

static const float attributes0_v[3] =
{
    float(0),
    float(0),
    float(1),
};

static const float attributes1_u[3] =
{
    float(0),
    float(1),
    float(0),
};

static const float attributes1_v[3] =
{
    float(0),
    float(1),
    float(1),
};

float3 translate(float3 v, float3 t)
{
    return t + v;
}


Texture2D<float4> g_texture : register(t0);

[RootSignature( MyRS3 ) ]
float4 main(interpolated_value v) : SV_TARGET0
{
   // Normalized pixel coordinates (from 0 to 1)
   float2 uv			      = float2(v.m_uv.x, v.m_uv.y);// f* float2(1.0f, -1.0f);
   uv						  = (uv  * 2 ) - 1;         //scale distances by two

   float3                      dimensions               = float3(m_w, m_h, 1);
   float3                      position_green           = float3(300, 300, 0);      //coordinate system upper left corner
   float3                      dimensions_green         = float3(160, 160, 0);
   float3                      dimensions_normalized    = dimensions_green / dimensions;
   float3                     position_normalized       = float3(-1, -1, 0) + 2 * position_green / dimensions + dimensions_green / dimensions;  // + position_green / dimensions_green;
   float3                      translation              = float3(-0.5, -0.5, 0 );

   //orthographic projection, many cameras with 1 ray
   float3 rayOrigin			 = float3(uv.x, uv.y, 0);
   float3 rayDir			 = float3(0, 0, 1);


   //triangles
   float3 t0				 = float3(-1, -1,  0.5);
   float3 t1				 = float3(1,  -1,  0.5);
   float3 t2				 = float3(1,  1,   0.5);

   float3 t3				 = float3(-1,-1,   0.5);
   float3 t4				 = float3(1, 1,    0.5);
   float3 t5				 = float3(-1, 1,   0.5);

   float3 distBound			 = float3(0.0, MaxDistance, 0 );

   float  bary_x             = 1;
   float  bary_y             = 1;

   //transforms for the triangles
   t0 *= dimensions_normalized;
   t1 *= dimensions_normalized;
   t2 *= dimensions_normalized;
   t3 *= dimensions_normalized;
   t4 *= dimensions_normalized;
   t5 *= dimensions_normalized;

   t0 = translate(t0, position_normalized);
   t1 = translate(t1, position_normalized);
   t2 = translate(t2, position_normalized);

   t3 = translate(t3, position_normalized);
   t4 = translate(t4, position_normalized);
   t5 = translate(t5, position_normalized);


   //ray tracing against two triangles
   {
	   distBound			= opU(distBound, iTriangle(rayOrigin, rayDir, distBound.xy, bary_x, bary_y, t0, t1, t2), 1);
	   distBound			= opU(distBound, iTriangle(rayOrigin, rayDir, distBound.xy, bary_x, bary_y, t3, t4, t5), 2);
   }

   float4 fragColor = float4(0.5,0.5,0.5,1);

   int triangleHit          = (int)distBound.z;

   bary_x = saturate(bary_x);
   bary_y = saturate(bary_y);

   if ( triangleHit == 1 )
   {
       float  u_x           = lerp(attributes0_u[0], attributes0_u[1], bary_x);
       u_x                  = lerp(u_x, attributes0_u[2], bary_y);

       float  u_y           = lerp(attributes0_v[0], attributes0_v[1], bary_x);
       u_y                  = lerp(u_y, attributes0_v[2], bary_y);

       float2 co            = float2(u_x, u_y);
       fragColor            = float4(g_texture.Sample(g_linear_clamp, co).rgb, 1.0);
   }
   else if ( triangleHit == 2 )
   {
       float  u_x = lerp(attributes1_u[0], attributes1_u[1], bary_x);
       u_x = lerp(u_x, attributes1_u[2], bary_y);

       float  u_y = lerp(attributes1_v[0], attributes1_v[1], bary_x);
       u_y = lerp(u_y, attributes1_v[2], bary_y);
       
       float2 co = float2(u_x, u_y);
       fragColor = float4(g_texture.Sample(g_linear_clamp, co).rgb, 1.0);

   }
   else
   {
       fragColor = float4(0, 0, 0, 1);
   }

   return fragColor;
}

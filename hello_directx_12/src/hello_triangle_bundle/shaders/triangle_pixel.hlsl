#include "default_signature.hlsli"

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

static const float3 CameraCenter	= float3(0.0, 0.0, -12.0);
static const float  imagePlaneDist	= 1.5;
static const float4 purp			= float4(0.5, 0.0, 1.0, 1.0);

float2 opU(float2 d, float iResult)
{
	return (iResult < d.y) ? float2(d.x, iResult) : d;
}

static const float MaxDistance = 1e10;

// Triangle:        https://www.shadertoy.com/view/MlGcDz
float iTriangle(in float3 ro, in float3 rd, in float2 distBound, inout float3 normal, in float3 v0, in float3 v1, in float3 v2)
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
		normal = normalize(-n);
		return t;
	}
}

[RootSignature( MyRS3 ) ]
float4 main(interpolated_value v) : SV_TARGET0
{
   // Normalized pixel coordinates (from 0 to 1)
   float2 uv				= float2(v.m_uv.x, 1.0f - v.m_uv.y);// f* float2(1.0f, -1.0f);
   uv						= (uv * 2.0) - 1.0;
   uv.x					   *= m_w / m_h;

   //SPHERE RENDER
	//ray origin
   float3 rayOrigin			= CameraCenter;

   //image plane pixel
   float3 imagePlanePixel	= float3(uv.x, uv.y, CameraCenter.z + imagePlaneDist);

   //ray direction
   float3 rayDir			= normalize(imagePlanePixel - rayOrigin);

   float3 t0				= float3(-1, 0, -5);
   float3 t1				= float3(1, 0, -5);
   float3 t2				= float3(0, 1, -5);

   float3 t3				= float3(0, 2, -5);
   float3 t4				= float3(-2, 2, -5);
   float3 t5				= float3(0, 0.5, -5);

   float3 normal			= float3(0, 0, 0);
   float2 distBound			= float2(0.001f, 100);

   for (int i = 0; i < 3200; i++)
   {
	   distBound				= opU(distBound, iTriangle(rayOrigin, rayDir, distBound, normal, t0, t1, t2));
	   distBound				= opU(distBound, iTriangle(rayOrigin, rayDir, distBound, normal, t3, t4, t5));
   }

   float value;

   if (distBound.y != 100.0)
   {
	   value = 1.0f;
   }
   else
   {
	   value = 0.0f;
   }

   value				   = clamp(value, 0.0, 1.0);
   float4 fragColor		   = float4(value * purp.x, value * purp.y, value * purp.z, 1.0);

   return fragColor;
}

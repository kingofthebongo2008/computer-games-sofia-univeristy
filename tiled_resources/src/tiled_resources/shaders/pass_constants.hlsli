#ifndef __pass_constants_hlsli__
#define __pass_constants_hlsli__

//36 DWORDS, must match the root signature
cbuffer VertexShaderConstants : register(b9)
{
	row_major float4x4 ViewMatrix;
	row_major float4x4 ProjectionMatrix; // Also may include device orientation rotation transform.
	float3             m_SunPosition;    // Used in the pixel shader for lighting
	float              m_ScaleFactor;    // Scale factor
};

#endif
 





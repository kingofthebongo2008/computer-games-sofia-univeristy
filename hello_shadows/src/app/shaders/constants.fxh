#ifndef __constants_h__
#define __constants_h__

cbuffer g_constants		  : register(b1)
{
	float4x4			  m_view;
	float4x4			  m_projection;
	float4				  m_camera_position;
};

cbuffer g_constants		  : register(b0)
{
	uint m_draw_argument0;
	uint m_draw_argument1;
	uint m_draw_argument2;
	uint m_draw_argument3;
};

#endif
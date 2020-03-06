#ifndef __default_samplers_hlsli__
#define __default_samplers_hlsli__

SamplerState g_point_clamp         : register(s2);
SamplerState g_linear_clamp        : register(s3);
SamplerState g_point_wrap          : register(s4);
SamplerState g_linear_wrap         : register(s5);
SamplerState g_linear_border_black : register(s6);
SamplerState g_linear_border_white : register(s7);


#endif


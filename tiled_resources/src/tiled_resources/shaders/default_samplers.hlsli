#ifndef __default_samplers_hlsli__
#define __default_samplers_hlsli__

SamplerState samplerWrapPoint               : register(s0);
SamplerState samplerClampLinear             : register(s1);
SamplerState samplerWrapAnisotropic         : register(s2);
SamplerState samplerWrapAnisotropicLimited  : register(s3);
SamplerState samplerWrapMaximum             : register(s4);

#endif
 





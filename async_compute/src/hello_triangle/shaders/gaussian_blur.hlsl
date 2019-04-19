#include "default_signature.hlsli"

RWTexture2D<float4>   lightingBuffer : register( u0 );

[numthreads(16, 16, 1)]
[RootSignature( MyRS2 ) ]
void main( uint3 DTid    : SV_DispatchThreadID )
{
	lightingBuffer[DTid.xy] = float4(0,0.5,0.5,1.0);
}



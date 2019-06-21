#include "default_signature.hlsli"

Texture2DMS<float4>   lightingBufferMSAA : register(t0);
RWByteAddressBuffer   lightingBuffer     : register(u0);

[numthreads(8, 8, 1)]
[RootSignature( MyRS2 ) ]
void main( uint3 DTid    : SV_DispatchThreadID, uint3 gtid : SV_GroupThreadID )
{
	/*
	uint2 size;
	lightingBuffer.GetDimensions(size.x, size.y);

	if ( gtid.xy == uint2( 0, 0 ) )
	GroupMemoryBarrierWithGroupSync();
	*/	

    lightingBuffer.Store(0, 0);
}



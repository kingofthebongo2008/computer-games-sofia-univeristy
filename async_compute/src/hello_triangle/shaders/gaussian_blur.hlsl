#include "default_signature.hlsli"

globallycoherent RWTexture2D<float4>   lightingBuffer : register( u0 );

groupshared uint4 pixels[ 18 ];

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
	lightingBuffer[DTid.xy] = float4(0,0.5,0.5,1.0);
}



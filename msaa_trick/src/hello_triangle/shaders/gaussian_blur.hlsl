#include "default_signature.hlsli"

Texture2DMS<float4>   lightingBufferMSAA : register(t0);
RWTexture2D<float4>   lightingBuffer     : register(u0);

[numthreads(8, 8, 1)]
[RootSignature( MyRS2 ) ]
void main( uint3 DTid    : SV_DispatchThreadID, uint3 gtid : SV_GroupThreadID )
{
    uint sample_x       = DTid.x % 2;
    uint sample_y       = DTid.y % 2;

    uint x              = DTid.x / 2;
    uint y              = DTid.y / 2;

    uint fetch_sample   = sample_y * 2 + sample_x;
    float4 r            = lightingBufferMSAA.Load( uint2(x,y), fetch_sample);


    lightingBuffer[ DTid.xy ] = r;
    /*
    uint2 size;
    lightingBuffer.GetDimensions(size.x, size.y);
    if ( gtid.xy == uint2( 0, 0 ) )
    GroupMemoryBarrierWithGroupSync();
    */ 
}



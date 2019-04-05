#include "default_signature.hlsli"

struct interpolated_value
{
    float4 m_position   : SV_POSITION;
    float3 m_uv         : TEXCOORD0;
};
// Render Target contains:
//   R : U texture coordinate at this pixel.
//   G : V texture coordinate at this pixel.
//   B : W texture coordinate at this pixel.
//   A : Encodes calculated texture level of detail at this pixel.

[RootSignature(MyRS1)]
float4 main(interpolated_value input) : SV_TARGET
{
    // For simplicity, assume all textures are maximally sized and have full MIP chains.
    // Extracted sample values will be transformed as necessary to account for this.
    const  float SamplingRatio = 8.0f; // Must match  SampleSettings::Sampling::Ratio
    float2 EncodeConstants = { 16384.0f / SamplingRatio, 15.0f };
    float4 ret;

    // Save the interpolated texcoords.
    ret.rgb = (float3(1.0f, 1.0f, 1.0f) + input.m_uv) / 2.0f;

    // Get the texcoord derivatives in target space.
    float3 dtdx = ddx(input.m_uv);
    float3 dtdy = ddy(input.m_uv);

    float2 duvdx = float2(0.0f, 0.0f);
    float2 duvdy = float2(0.0f, 0.0f);

    float3 absTex = abs(input.m_uv);
    if (absTex.x > absTex.y && absTex.x > absTex.z)
    {
        // Major Axis = X.
        float2 texp = -input.m_uv.yz;
        float2 dtdxp = -dtdx.yz;
        float2 dtdyp = -dtdy.yz;
        duvdx = (input.m_uv.x * dtdxp - texp * dtdx.x) / (input.m_uv.x * input.m_uv.x);
        duvdy = (input.m_uv.x * dtdyp - texp * dtdy.x) / (input.m_uv.x * input.m_uv.x);
    }
    else if (absTex.y > absTex.x && absTex.y > absTex.z)
    {
        // Major Axis = Y.
        float2 texp = -input.m_uv.xz;
        float2 dtdxp = -dtdx.xz;
        float2 dtdyp = -dtdy.xz;
        duvdx = (input.m_uv.y * dtdxp - texp * dtdx.y) / (input.m_uv.y * input.m_uv.y);
        duvdy = (input.m_uv.y * dtdyp - texp * dtdy.y) / (input.m_uv.y * input.m_uv.y);
    }
    else
    {
        // Major Axis = Z.
        float2 texp = -input.m_uv.xy;
        float2 dtdxp = -dtdx.xy;
        float2 dtdyp = -dtdy.xy;
        duvdx = (input.m_uv.z * dtdxp - texp * dtdx.z) / (input.m_uv.z * input.m_uv.z);
        duvdy = (input.m_uv.z * dtdyp - texp * dtdy.z) / (input.m_uv.z * input.m_uv.z);
    }

    // Calculate the maximum magnitude of the scaled derivative in texel space.
    float dldx = sqrt(duvdx.x * duvdx.x + duvdy.x * duvdy.x);
    float dldy = sqrt(duvdx.y * duvdx.y + duvdy.y * duvdy.y);
    float derivative = max(dldx, dldy) * 0.5f; // Multiply by 0.5 due to texcube faces spanning -1 to 1.

    // Useful derivative values will range from TargetRatio/ResourceDimension, corresponding to the
    // most detailed mip, to TargetRatio, corresponding to the least detailed MIP. Roughly,
    // derivative = TargetRatio * 2 ^ MipLevel / ResourceDimension. Since the resulting MipLevel is
    // the interesting linear value to extract, we want to encode it as
    // MipLevel = log2(derivative * ResourceDimension / TargetRatio). To encode this in the UNORM
    // range of 0..1, we divide by the total MIP count of the resource.
    // EncodedValue = log2(derivative * ResourceDimension / TargetRatio) / MipCount.
    // The constants (ResourceDimension / TargetRatio) and (MipCount) are stored in
    // EncodeConstants.x and EncodeConstants.y, respectively.
    float encodedLevelOfDetail = log2(derivative * EncodeConstants.x) / EncodeConstants.y;
    ret.a = encodedLevelOfDetail;

    return ret;
}

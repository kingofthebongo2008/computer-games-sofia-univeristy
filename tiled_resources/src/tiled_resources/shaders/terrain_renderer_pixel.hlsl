#include "default_signature.hlsli"
#include "default_samplers.hlsli"
#include "pass_constants.hlsli"

TextureCube<float3> ColorTexture	: register(t1);
TextureCube<float>	ColorResidency	: register(t2);
TextureCube<float2> NormalTexture	: register(t3);
TextureCube<float>	NormalResidency : register(t4);


struct PS_IN
{
    float3 tex : TEXCOORD0;
    float3 utan : TANGENT0;
    float3 vtan : TANGENT1;
};

[RootSignature(MyRS1)]
float4 main(PS_IN input) : SV_TARGET
{
    float3 tex = normalize(input.tex);

#if RESTRICT_TIER_1
    // Gather can be used to emulate the MAXIMUM filter variants when on Tier 1.
    float4 normalSampleValues = NormalResidency.Gather(samplerWrapMaximum, tex) * 16.0f;
    float normalMinLod = normalSampleValues.x;
    normalMinLod = max(normalMinLod, normalSampleValues.y);
    normalMinLod = max(normalMinLod, normalSampleValues.z);
    normalMinLod = max(normalMinLod, normalSampleValues.w);
    float4 diffuseSampleValues = ColorResidency.Gather(samplerWrapMaximum, tex) * 16.0f;
    float diffuseMinLod = diffuseSampleValues.x;
    diffuseMinLod = max(diffuseMinLod, diffuseSampleValues.y);
    diffuseMinLod = max(diffuseMinLod, diffuseSampleValues.z);
    diffuseMinLod = max(diffuseMinLod, diffuseSampleValues.w);
    // SampleLevel in conjunction with CalculateLevelOfDetail can be used to emulate LOD Clamp behavior when on Tier 1.
    float diffuseCalculatedLod = ColorTexture.CalculateLevelOfDetail(samplerWrapAnisotropic, tex);
    float3 diffuse = diffuseCalculatedLod < diffuseMinLod ? ColorTexture.SampleLevel(samplerWrapAnisotropic, tex, diffuseMinLod) : ColorTexture.Sample(samplerWrapAnisotropic, tex);
    float normalCalculatedLod = NormalTexture.CalculateLevelOfDetail(samplerWrapAnisotropic, tex);
    float2 tangent = normalCalculatedLod < normalMinLod ? NormalTexture.SampleLevel(samplerWrapAnisotropic, tex, normalMinLod) : NormalTexture.Sample(samplerWrapAnisotropic, tex);
#else
    float normalMinLod = NormalResidency.Sample(samplerWrapMaximum, tex) * 16.0f;
    float diffuseMinLod = ColorResidency.Sample(samplerWrapMaximum, tex) * 16.0f;
    float3 diffuse = ColorTexture.Sample(samplerWrapAnisotropic, tex, diffuseMinLod);
    float2 tangent = NormalTexture.Sample(samplerWrapAnisotropic, tex, normalMinLod);
#endif

    float dataScaleFactor = 10.0f;
    float scaleFactor	  = m_ScaleFactor;

    float3 normal = tangent.x * input.utan + tangent.y * input.vtan;

    float arg = 1.0f - tangent.x * tangent.x - tangent.y * tangent.y;
    if (arg > 0.0f)
    {
        normal += (dataScaleFactor / scaleFactor) * sqrt(arg) * cross(input.utan, input.vtan);
    }

    normal = normalize(normal);

	//simulate simple lighting
    float ambient		= 0.2f;
    float lighting		= ambient + (1.0f - ambient) * saturate(dot(normal,m_SunPosition.xyz));
    float3 dustColor	= float3(0.92f, 0.65f, 0.41f);

    float3 litColor		= diffuse * lighting;

    float3 finalColor	= lerp(litColor, dustColor, 0.5f);
    finalColor			= litColor;

    return float4(finalColor, 1.0f);
}

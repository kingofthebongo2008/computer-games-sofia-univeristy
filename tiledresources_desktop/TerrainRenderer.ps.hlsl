TextureCube<float3> ColorTexture : register(t0);
TextureCube<float> ColorResidency : register(t1);
TextureCube<float2> NormalTexture : register(t2);
TextureCube<float> NormalResidency : register(t3);
SamplerState Trilinear : register(s0);
SamplerState MaxFilter : register(s1);

cbuffer PixelShaderConstants : register(b0)
{
    float4 SunPosition;
};

struct PS_IN
{
    float3 tex : TEXCOORD0;
    float3 utan : TANGENT0;
    float3 vtan : TANGENT1;
};

float4 main(PS_IN input) : SV_TARGET
{
    float3 tex = normalize(input.tex);

#if RESTRICT_TIER_1
    // Gather can be used to emulate the MAXIMUM filter variants when on Tier 1.
    float4 normalSampleValues = NormalResidency.Gather(MaxFilter, tex) * 16.0f;
    float normalMinLod = normalSampleValues.x;
    normalMinLod = max(normalMinLod, normalSampleValues.y);
    normalMinLod = max(normalMinLod, normalSampleValues.z);
    normalMinLod = max(normalMinLod, normalSampleValues.w);
    float4 diffuseSampleValues = ColorResidency.Gather(MaxFilter, tex) * 16.0f;
    float diffuseMinLod = diffuseSampleValues.x;
    diffuseMinLod = max(diffuseMinLod, diffuseSampleValues.y);
    diffuseMinLod = max(diffuseMinLod, diffuseSampleValues.z);
    diffuseMinLod = max(diffuseMinLod, diffuseSampleValues.w);
    // SampleLevel in conjunction with CalculateLevelOfDetail can be used to emulate LOD Clamp behavior when on Tier 1.
    float diffuseCalculatedLod = ColorTexture.CalculateLevelOfDetail(Trilinear, tex);
    float3 diffuse = diffuseCalculatedLod < diffuseMinLod ? ColorTexture.SampleLevel(Trilinear, tex, diffuseMinLod) : ColorTexture.Sample(Trilinear, tex);
    float normalCalculatedLod = NormalTexture.CalculateLevelOfDetail(Trilinear, tex);
    float2 tangent = normalCalculatedLod < normalMinLod ? NormalTexture.SampleLevel(Trilinear, tex, normalMinLod) : NormalTexture.Sample(Trilinear, tex);
#else
    float normalMinLod = NormalResidency.Sample(MaxFilter, tex) * 16.0f;
    float diffuseMinLod = ColorResidency.Sample(MaxFilter, tex) * 16.0f;
    float3 diffuse = ColorTexture.Sample(Trilinear, tex, diffuseMinLod);
    float2 tangent = NormalTexture.Sample(Trilinear, tex, normalMinLod);
#endif

    float dataScaleFactor = 10.0f;
    float scaleFactor = SunPosition.w;

    float3 normal = tangent.x * input.utan + tangent.y * input.vtan;
    float arg = 1.0f - tangent.x * tangent.x - tangent.y * tangent.y;
    if (arg > 0.0f)
    {
        normal += (dataScaleFactor / scaleFactor) * sqrt(arg) * cross(input.utan, input.vtan);
    }
    normal = normalize(normal);
    float ambient = 0.2f;
    float lighting = ambient + (1.0f - ambient) * saturate(dot(normal,SunPosition.xyz));

    float3 dustColor = float3(0.92f, 0.65f, 0.41f);

    float3 litColor = diffuse * lighting;

    float3 finalColor = lerp(litColor, dustColor, 0.5f);
    finalColor = litColor;

    return float4(finalColor, 1.0f);
}

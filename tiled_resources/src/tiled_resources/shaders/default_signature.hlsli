#ifndef __default_signature_hlsli__
#define __default_signature_hlsli__
//CBV 2 dwords
//SRV 2 dwords
//UAV 2 dwords
//Descriptor Table 1 dword

//Sum must be up 60 64

#define MyRS1 "RootFlags( ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT ), " \
			  "RootConstants(num32BitConstants=36, b9), " \
              "DescriptorTable( SRV(t0, numDescriptors = 4))," \
              "StaticSampler(s0, addressU = TEXTURE_ADDRESS_WRAP,   addressV = TEXTURE_ADDRESS_WRAP, addressW = TEXTURE_ADDRESS_WRAP,  filter = FILTER_MIN_MAG_MIP_POINT )," \
              "StaticSampler(s1, addressU = TEXTURE_ADDRESS_WRAP,   addressV = TEXTURE_ADDRESS_WRAP, addressW = TEXTURE_ADDRESS_WRAP,  filter = FILTER_MIN_MAG_MIP_LINEAR )," \
              "StaticSampler(s2, addressU = TEXTURE_ADDRESS_WRAP,   addressV = TEXTURE_ADDRESS_WRAP, addressW = TEXTURE_ADDRESS_WRAP,  filter = FILTER_ANISOTROPIC, MaxAnisotropy = 16 )," \
              "StaticSampler(s3, addressU = TEXTURE_ADDRESS_WRAP,   addressV = TEXTURE_ADDRESS_WRAP, addressW = TEXTURE_ADDRESS_WRAP,  filter = FILTER_ANISOTROPIC, MaxAnisotropy = 4  )," \
              "StaticSampler(s4, addressU = TEXTURE_ADDRESS_WRAP,   addressV = TEXTURE_ADDRESS_WRAP, addressW = TEXTURE_ADDRESS_WRAP,  filter = FILTER_MAXIMUM_MIN_MAG_MIP_LINEAR )" 
#endif


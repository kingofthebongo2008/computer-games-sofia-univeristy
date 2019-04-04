#ifndef __default_signature_hlsli__
#define __default_signature_hlsli__

#define MyRS1 "RootFlags( ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT ), " \
              "CBV(b0, space = 0), " \
              "SRV(t0), " \
              "UAV(u0), " \
              "DescriptorTable( CBV(b1, numDescriptors = 5))," \
              "DescriptorTable( UAV(u1, numDescriptors = 2))," \
              "DescriptorTable( SRV(t1, numDescriptors = 8))," \
              "DescriptorTable( SRV(t9, numDescriptors = 2) ), " \
              "RootConstants(num32BitConstants=1, b9), " \
              "DescriptorTable( UAV(u3), UAV(u4), UAV(u5)), " \
              "StaticSampler(s0, addressU = TEXTURE_ADDRESS_WRAP,   addressV = TEXTURE_ADDRESS_WRAP, addressW = TEXTURE_ADDRESS_WRAP,  filter = FILTER_MIN_MAG_MIP_POINT )," \
              "StaticSampler(s1, addressU = TEXTURE_ADDRESS_WRAP,   addressV = TEXTURE_ADDRESS_WRAP, addressW = TEXTURE_ADDRESS_WRAP,  filter = FILTER_MIN_MAG_MIP_LINEAR )," \
              "StaticSampler(s2, addressU = TEXTURE_ADDRESS_WRAP,   addressV = TEXTURE_ADDRESS_WRAP, addressW = TEXTURE_ADDRESS_WRAP,  filter = FILTER_ANISOTROPIC, MaxAnisotropy = 16 )," \
              "StaticSampler(s3, addressU = TEXTURE_ADDRESS_WRAP,   addressV = TEXTURE_ADDRESS_WRAP, addressW = TEXTURE_ADDRESS_WRAP,  filter = FILTER_ANISOTROPIC, MaxAnisotropy = 4  )," \
              "StaticSampler(s4, addressU = TEXTURE_ADDRESS_WRAP,   addressV = TEXTURE_ADDRESS_WRAP, addressW = TEXTURE_ADDRESS_WRAP,  filter = FILTER_MAXIMUM_MIN_MAG_MIP_LINEAR )" 
#endif


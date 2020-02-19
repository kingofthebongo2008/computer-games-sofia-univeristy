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
              "StaticSampler(s0)," \
              "StaticSampler(s1)," \
              "StaticSampler(s2, addressU = TEXTURE_ADDRESS_CLAMP,  addressV = TEXTURE_ADDRESS_CLAMP, addressW = TEXTURE_ADDRESS_CLAMP, filter = FILTER_MIN_MAG_MIP_POINT )," \
              "StaticSampler(s3, addressU = TEXTURE_ADDRESS_CLAMP,  addressV = TEXTURE_ADDRESS_CLAMP, addressW = TEXTURE_ADDRESS_CLAMP, filter = FILTER_MIN_MAG_MIP_LINEAR )," \
              "StaticSampler(s4, addressU = TEXTURE_ADDRESS_WRAP,   addressV = TEXTURE_ADDRESS_WRAP, addressW = TEXTURE_ADDRESS_WRAP,  filter = FILTER_MIN_MAG_MIP_POINT )," \
              "StaticSampler(s5, addressU = TEXTURE_ADDRESS_WRAP,   addressV = TEXTURE_ADDRESS_WRAP, addressW = TEXTURE_ADDRESS_WRAP,  filter = FILTER_MIN_MAG_MIP_LINEAR )," \
              "StaticSampler(s6, addressU = TEXTURE_ADDRESS_BORDER, addressV = TEXTURE_ADDRESS_BORDER, addressW = TEXTURE_ADDRESS_BORDER, filter = FILTER_MIN_MAG_MIP_LINEAR, borderColor = STATIC_BORDER_COLOR_OPAQUE_BLACK )," \
              "StaticSampler(s7, addressU = TEXTURE_ADDRESS_BORDER, addressV = TEXTURE_ADDRESS_BORDER, addressW = TEXTURE_ADDRESS_BORDER, filter = FILTER_MIN_MAG_MIP_LINEAR, borderColor = STATIC_BORDER_COLOR_OPAQUE_WHITE )"

#define MyRS2 "RootFlags( 0 ), " \
              "CBV(b0, space = 0), " \
              "SRV(t0), " \
              "UAV(u0), " \
              "DescriptorTable( CBV(b1, numDescriptors = 5))," \
              "DescriptorTable( UAV(u1, numDescriptors = 2))," \
              "DescriptorTable( SRV(t1, numDescriptors = 8))," \
              "DescriptorTable( SRV(t9, numDescriptors = 2) ), " \
              "RootConstants(num32BitConstants=1, b9), " \
              "DescriptorTable( UAV(u3), UAV(u4), UAV(u5)), " \
              "StaticSampler(s0)," \
              "StaticSampler(s1)," \
              "StaticSampler(s2, addressU = TEXTURE_ADDRESS_CLAMP,  addressV = TEXTURE_ADDRESS_CLAMP, addressW = TEXTURE_ADDRESS_CLAMP, filter = FILTER_MIN_MAG_MIP_POINT )," \
              "StaticSampler(s3, addressU = TEXTURE_ADDRESS_CLAMP,  addressV = TEXTURE_ADDRESS_CLAMP, addressW = TEXTURE_ADDRESS_CLAMP, filter = FILTER_MIN_MAG_MIP_LINEAR )," \
              "StaticSampler(s4, addressU = TEXTURE_ADDRESS_WRAP,   addressV = TEXTURE_ADDRESS_WRAP, addressW = TEXTURE_ADDRESS_WRAP,  filter = FILTER_MIN_MAG_MIP_POINT )," \
              "StaticSampler(s5, addressU = TEXTURE_ADDRESS_WRAP,   addressV = TEXTURE_ADDRESS_WRAP, addressW = TEXTURE_ADDRESS_WRAP,  filter = FILTER_MIN_MAG_MIP_LINEAR )," \
              "StaticSampler(s6, addressU = TEXTURE_ADDRESS_BORDER, addressV = TEXTURE_ADDRESS_BORDER, addressW = TEXTURE_ADDRESS_BORDER, filter = FILTER_MIN_MAG_MIP_LINEAR, borderColor = STATIC_BORDER_COLOR_OPAQUE_BLACK )," \
              "StaticSampler(s7, addressU = TEXTURE_ADDRESS_BORDER, addressV = TEXTURE_ADDRESS_BORDER, addressW = TEXTURE_ADDRESS_BORDER, filter = FILTER_MIN_MAG_MIP_LINEAR, borderColor = STATIC_BORDER_COLOR_OPAQUE_WHITE )"

#endif


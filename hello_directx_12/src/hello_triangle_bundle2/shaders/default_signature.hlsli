#ifndef __default_signature_hlsli__
#define __default_signature_hlsli__

#define MyRS3 "RootFlags( ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT ), " \
              "RootConstants(num32BitConstants=4, b9), " \
              "StaticSampler(s0)," \
              "StaticSampler(s1)," \
              "StaticSampler(s2, addressU = TEXTURE_ADDRESS_CLAMP,  addressV = TEXTURE_ADDRESS_CLAMP, addressW = TEXTURE_ADDRESS_CLAMP, filter = FILTER_MIN_MAG_MIP_POINT )," \
              "StaticSampler(s3, addressU = TEXTURE_ADDRESS_CLAMP,  addressV = TEXTURE_ADDRESS_CLAMP, addressW = TEXTURE_ADDRESS_CLAMP, filter = FILTER_MIN_MAG_MIP_LINEAR )," \
              "StaticSampler(s4, addressU = TEXTURE_ADDRESS_WRAP,   addressV = TEXTURE_ADDRESS_WRAP, addressW = TEXTURE_ADDRESS_WRAP,  filter = FILTER_MIN_MAG_MIP_POINT )," \
              "StaticSampler(s5, addressU = TEXTURE_ADDRESS_WRAP,   addressV = TEXTURE_ADDRESS_WRAP, addressW = TEXTURE_ADDRESS_WRAP,  filter = FILTER_MIN_MAG_MIP_LINEAR )," \
              "StaticSampler(s6, addressU = TEXTURE_ADDRESS_BORDER, addressV = TEXTURE_ADDRESS_BORDER, addressW = TEXTURE_ADDRESS_BORDER, filter = FILTER_MIN_MAG_MIP_LINEAR, borderColor = STATIC_BORDER_COLOR_OPAQUE_BLACK )," \
              "StaticSampler(s7, addressU = TEXTURE_ADDRESS_BORDER, addressV = TEXTURE_ADDRESS_BORDER, addressW = TEXTURE_ADDRESS_BORDER, filter = FILTER_MIN_MAG_MIP_LINEAR, borderColor = STATIC_BORDER_COLOR_OPAQUE_WHITE )"

#define MyRS4 "RootFlags( 0 ), " \
              "RootConstants(num32BitConstants=4, b9), " \
              "StaticSampler(s0)," \
              "StaticSampler(s1)," \
              "StaticSampler(s2, addressU = TEXTURE_ADDRESS_CLAMP,  addressV = TEXTURE_ADDRESS_CLAMP, addressW = TEXTURE_ADDRESS_CLAMP, filter = FILTER_MIN_MAG_MIP_POINT )," \
              "StaticSampler(s3, addressU = TEXTURE_ADDRESS_CLAMP,  addressV = TEXTURE_ADDRESS_CLAMP, addressW = TEXTURE_ADDRESS_CLAMP, filter = FILTER_MIN_MAG_MIP_LINEAR )," \
              "StaticSampler(s4, addressU = TEXTURE_ADDRESS_WRAP,   addressV = TEXTURE_ADDRESS_WRAP, addressW = TEXTURE_ADDRESS_WRAP,  filter = FILTER_MIN_MAG_MIP_POINT )," \
              "StaticSampler(s5, addressU = TEXTURE_ADDRESS_WRAP,   addressV = TEXTURE_ADDRESS_WRAP, addressW = TEXTURE_ADDRESS_WRAP,  filter = FILTER_MIN_MAG_MIP_LINEAR )," \
              "StaticSampler(s6, addressU = TEXTURE_ADDRESS_BORDER, addressV = TEXTURE_ADDRESS_BORDER, addressW = TEXTURE_ADDRESS_BORDER, filter = FILTER_MIN_MAG_MIP_LINEAR, borderColor = STATIC_BORDER_COLOR_OPAQUE_BLACK )," \
              "StaticSampler(s7, addressU = TEXTURE_ADDRESS_BORDER, addressV = TEXTURE_ADDRESS_BORDER, addressW = TEXTURE_ADDRESS_BORDER, filter = FILTER_MIN_MAG_MIP_LINEAR, borderColor = STATIC_BORDER_COLOR_OPAQUE_WHITE )"


#endif


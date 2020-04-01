#pragma once

#include "vector.h"

#include <assert.h>

namespace uc
{
    namespace math
    {
        typedef uint16_t        half;
        typedef __m128i         half4;
        typedef __m128i         half4_2;            // two half4 vectors in a compact form
        typedef std::uint64_t   compact_half4;      // half4 vector in a compact form, suitable for storage

        struct half2
        {
            half    r[2];
        };

        struct half3
        {
            half    r[2];
        };

        namespace details
        {
            template <uint32_t gather_group> inline __m128i UC_MATH_CALL extract_hi_16(__m128i v_1)
            {
                const uint32_t shuffle_k_0 = _MM_SHUFFLE(0, 2, 1, 3);
                const uint32_t shuffle_k_1 = _MM_SHUFFLE(1, 3, 0, 2);

                __m128i v_13 = _mm_shuffle_epi32(v_1, shuffle_k_0);
                __m128i v_02 = _mm_shuffle_epi32(v_1, shuffle_k_1);
                __m128i v_0_2 = _mm_unpacklo_epi16(v_02, v_13);

                __m128i result = _mm_shuffle_epi32(v_0_2, gather_group);

                return result;
            }

            inline __m128i UC_MATH_CALL extract_hi16(__m128i v1, __m128i v2)
            {
                alignas(16) static const uint32_t clear_mask[4] = { 0xffff0000, 0xffff0000, 0xffff0000, 0xffff0000 };

                const uint32_t shuffle_k_1 = _MM_SHUFFLE(0, 2, 1, 3);
                const uint32_t shuffle_k_2 = _MM_SHUFFLE(1, 3, 2, 0);

                __m128i v_1 = _mm_and_si128(*reinterpret_cast<const __m128i*> (&clear_mask[0]), v1);
                __m128i v_2 = _mm_and_si128(*reinterpret_cast<const __m128i*> (&clear_mask[0]), v2);

                __m128i result_1 = extract_hi_16 <shuffle_k_1>(v_1);
                __m128i result_2 = extract_hi_16 <shuffle_k_2>(v_2);

                __m128i result = _mm_or_si128(result_1, result_2);

                return result;
            }
        }

        namespace details1
        {
            struct convert_float_uint32
            {
                explicit convert_float_uint32(float v)
                {
                    m_value.f = v;
                }

                explicit convert_float_uint32(uint32_t v)
                {
                    m_value.i = v;
                }

                operator uint32_t() const
                {
                    return m_value.i;
                }

                operator float() const
                {
                    return m_value.f;
                }

            private:

                union
                {
                    float f;
                    uint32_t i;
                } m_value;

                convert_float_uint32();
            };

            inline half convert_f32_f16(float value)
            {
                details1::convert_float_uint32 convertor(value);

                uint32_t    IValue;
                uint32_t    Sign;
                half            Result;

                IValue = static_cast<uint32_t> (convertor) & 0x7FFFFFFFU;
                Sign = (static_cast<uint32_t> (convertor) & 0x80000000U) >> 16U;


                if (IValue > 0x47FFEFFFU)
                {
                    // The number is too large to be represented as a half.  Saturate to infinity.
                    Result = static_cast<half> ((Sign | 0x7FFFU));
                }
                else
                {
                    if (IValue < 0x38800000U)
                    {
                        // The number is too small to be represented as a normalized half.
                        // Convert it to a denormalized value.
                        uint32_t Shift = 113U - (IValue >> 23U); //exponent
                        uint32_t v = (IValue & 0x7FFFFFU); // mantissa

                        v = 0x800000U | v;
                        v = v >> Shift;

                        //v = 0x00800000 >> 0x00000071;
                        //IValue = (0x800000U | (IValue & 0x7FFFFFU) ) >> Shift;
                        IValue = v;
                        //IValue = 0;
                    }
                    else
                    {
                        // Rebias the exponent to represent the value as a normalized half.
                        IValue += 0xC8000000U;
                    }

                    Result = static_cast<half> ((Sign | ((IValue + 0x0FFFU + ((IValue >> 13U) & 1U)) >> 13U)));
                }

                return static_cast<half> (Result);
            }

            inline float convert_f16_f32(half Value)
            {
                uint32_t Mantissa;
                uint32_t Exponent;
                uint32_t Result;

                Mantissa = (uint32_t)(Value & 0x03FF);

                if ((Value & 0x7C00) != 0)  // The value is normalized
                {
                    Exponent = (uint32_t)((Value >> 10) & 0x1F);
                }
                else if (Mantissa != 0)     // The value is denormalized
                {
                    // Normalize the value in the resulting float
                    Exponent = 1;

                    do
                    {
                        Exponent--;
                        Mantissa <<= 1;
                    } while ((Mantissa & 0x0400) == 0);

                    Mantissa &= 0x03FF;
                }
                else                        // The value is zero
                {
                    Exponent = (UINT)-112;
                }

                Result = ((Value & 0x8000) << 16) | // Sign
                    ((Exponent + 112) << 23) | // Exponent
                    (Mantissa << 13);          // Mantissa

                union
                {
                    uint32_t u;
                    float         f;
                } c = { Result };

                return c.f;
            }

            inline half4_2 UC_MATH_CALL convert_f32_f16(afloat4 v1, afloat4 v2)
            {
                alignas(16) static const uint32_t sign_mask[4] = { 0x80008000,  0x80008000, 0x80008000, 0x80008000 };
                alignas(16) static const uint32_t exponent_offset[4] = { 0x38003800,    0x38003800, 0x38003800, 0x38003800 };   //112
                alignas(16) static const uint32_t exponent_mask[4] = { 0x7c007c00, 0x7c007c00, 0x7c007c00, 0x7c007c00 };
                alignas(16) static const uint32_t mantissa_mask[4] = { 0x03ff03ff, 0x03ff03ff, 0x03ff03ff, 0x03ff03ff };

                __m128i v_1 = _mm_castps_si128(v1);
                __m128i v_2 = _mm_castps_si128(v2);

                //sign exponent
                __m128i se = details::extract_hi16(v_1, v_2);   //  | seeeeeee emmmmmmm |

                __m128i s = _mm_and_si128(*reinterpret_cast<const __m128i*> (&sign_mask[0]), se);       //  | s000000 00000000 |
                __m128i e = _mm_andnot_si128(*reinterpret_cast<const __m128i*> (&sign_mask[0]), se);    //  | 0eeeeeee emmmmmmm |

                __m128i underflow_mask = _mm_cmpgt_epi16(e, *reinterpret_cast<const __m128i*>(&exponent_offset[0]));

                e = _mm_subs_epi16(e, *reinterpret_cast<const __m128i*>(&exponent_offset[0])); // (e = ((e – 127) + 15))
                e = _mm_slli_epi16(e, 3);
                e = _mm_and_si128(e, *reinterpret_cast<const __m128i*>(&exponent_mask[0]));

                __m128i v_1_1 = _mm_slli_epi32(v_1, 8);
                __m128i v_2_1 = _mm_slli_epi32(v_2, 8);

                __m128i m = details::extract_hi16(v_1_1, v_2_1);    //  | emmmmmmm mmmmmmmm |
                m = _mm_srli_epi16(m, 5);                           //  | 0000emm mmmmmmmm |

                m = _mm_and_si128(m, *reinterpret_cast<const __m128i*>(&mantissa_mask[0])); //  | 00000mm mmmmmmmm |

                __m128i result = _mm_or_si128(s, m);
                result = _mm_or_si128(result, e);
                result = _mm_and_si128(underflow_mask, result); //map to zero all numbers that are small. most gpus do not have denormal(subnormal) halves.

                return result;
            }

        }

        namespace details2
        {
            inline half convert_f32_f16(float value)
            {
                extern math::half   base_table[512];
                extern uint8_t  shift_table[512];

                union
                {
                    float f;
                    uint32_t u;
                    int32_t  i;
                } c = { value };

                uint32_t mask_1 = c.i >> 23;        //remove mantissa
                uint32_t mask_2 = mask_1 & 0x1ff;
                uint32_t mask_3 = c.i & 0x007fffff;

                uint8_t  shift = shift_table[mask_2];
                half          base = base_table[mask_2];

                return static_cast<half> (base + (mask_3 >> shift));
            }

            inline float convert_f16_f32(half h)
            {
                extern uint32_t mantissa_table[2048];
                extern uint32_t exponent_table[64];
                extern uint16_t offset_table[64];

                uint32_t result = mantissa_table[offset_table[h >> 10] + (h & 0x3ff)] + exponent_table[h >> 10];

                union
                {
                    uint32_t u;
                    int32_t  i;
                    float f;
                } c = { result };

                return c.f;
            }

            void generate_tables();

            inline __m128i select(__m128i value1, __m128i value2, __m128i control)
            {
                // (((b ^ a) & mask)^a)
                /*
                __m128i v1 = _mm_xor_si128(value2, value1);
                __m128i v2 = _mm_andnot_si128(control, v1);
                return _mm_or_si128(v1, v2);
                */

                __m128i v1 = _mm_andnot_si128(control, value1);
                __m128i v2 = _mm_and_si128(value2, control);
                return _mm_or_si128(v1, v2);
            }

            inline math::half4_2 UC_MATH_CALL convert_f32_f16(afloat4 v1, afloat4 v2)
            {
                alignas(16) static const uint32_t sign_mask[4] = { 0x80008000,  0x80008000, 0x80008000, 0x80008000 };
                alignas(16) static const uint32_t exponent_offset[4] = { 0x38003800,    0x38003800, 0x38003800, 0x38003800 };   // greater than 112
                alignas(16) static const uint32_t sub_112[4] = { 0x40004000, 0x40004000, 0x40004000, 0x40004000 };

                __m128i v_1 = _mm_castps_si128(v1);
                __m128i v_2 = _mm_castps_si128(v2);

                __m128i v_1_1 = _mm_slli_epi32(v_1, 3);
                __m128i v_2_1 = _mm_slli_epi32(v_2, 3);

                __m128i e_m = details::extract_hi16(v_1_1, v_2_1);  //  | eeeeeemm mmmmmmmm |   // extract 10 bit mantissa + exponent
                __m128i e_m_x = _mm_xor_si128(e_m, *reinterpret_cast<const __m128i*> (&sub_112[0]));    // (e = ((e – 127) + 15)) ( relies on the fact that 1111 1111 - 01110000 flips a bit (great trick by Mike Day from Insomniac) )

                __m128i se = details::extract_hi16(v_1, v_2);           //  | seeeeeee emmmmmmm |   //extract the sign + exponent
                __m128i result = select(e_m_x, se, *reinterpret_cast<const __m128i*> (&sign_mask[0]));

                __m128i e = _mm_andnot_si128(*reinterpret_cast<const __m128i*> (&sign_mask[0]), se);    //  | 0eeeeeee emmmmmmm |
                __m128i underflow_mask = _mm_cmpgt_epi16(e, *reinterpret_cast<const __m128i*>(&exponent_offset[0]));

                result = _mm_and_si128(underflow_mask, result); //map to zero all numbers that are small. most gpus do not have denormal(subnormal) halves.

                return result;
            }
        }

        inline half convert_f32_f16(float value)
        {
            return details1::convert_f32_f16(value);
        }

        inline float convert_f16_f32(half h)
        {
            return details1::convert_f16_f32(h);
        }

        inline half4 UC_MATH_CALL convert_f32_f16(afloat4 value)
        {
            alignas(16) static const uint32_t exponent_mask[4] = { 0xff,    0xff,   0xff,   0xff };
            alignas(16) static const uint32_t sign_mask[4] = { 0x8000,  0x8000, 0x8000, 0x8000 };
            alignas(16) static const uint32_t manitssa_mask[4] = { 0x07ff,  0x07ff, 0x07ff, 0x07ff };
            alignas(16) static const uint32_t e_denorm_mask[4] = { 112, 112, 112, 112 };
            alignas(16) static const uint32_t rounddmask[4] = { 0x1,    0x1, 0x1, 0x1 };

            __m128i v_1 = _mm_castps_si128(value);

            __m128i xmm1 = _mm_load_si128(reinterpret_cast<const __m128i*> (&exponent_mask[0]));
            __m128i xmm2 = _mm_load_si128(reinterpret_cast<const __m128i*> (&sign_mask[0]));
            __m128i xmm3 = _mm_load_si128(reinterpret_cast<const __m128i*> (&manitssa_mask[0]));

            //extract exponent
            __m128i exponent = _mm_srli_epi32(v_1, 23);
            exponent = _mm_and_si128(exponent, xmm1);

            //extract sign
            __m128i sign = _mm_srli_epi32(v_1, 16);
            sign = _mm_and_si128(sign, xmm2);

            //extract mantissa
            __m128i mantissa = _mm_srli_epi32(v_1, 12);
            mantissa = _mm_and_si128(mantissa, xmm3);

            __m128i xmm4 = _mm_load_si128(reinterpret_cast<const __m128i*> (&e_denorm_mask[0]));
            __m128i map_zero = _mm_cmpgt_epi32(exponent, xmm4); // do not use denormal(subnormal) numbers, most gpu's do not have denormal multiplication

            // bits |= ((e - 112) << 10) | (m >> 1);
            mantissa = _mm_srli_epi32(mantissa, 1); // m << 1
            exponent = _mm_sub_epi32(exponent, xmm4);   // e - 112
            exponent = _mm_slli_epi32(exponent, 10);    // e << 10

            __m128i result = _mm_or_si128(exponent, mantissa); // e | m
            result = _mm_or_si128(result, sign);                // sign | (e | m)

            __m128i xmm5 = _mm_load_si128(reinterpret_cast<const __m128i*> (&rounddmask[0]));   // m & 1
            mantissa = _mm_and_si128(mantissa, xmm5);

            /* Extra rounding. An overflow will set mantissa to 0 and increment
             * the exponent, which is OK. */
             //bits += m & 1;
            result = _mm_add_epi32(result, mantissa);
            result = _mm_and_si128(result, map_zero);   // map to zero underflow numbers

            return result;
        }

        inline half4_2 UC_MATH_CALL convert_f32_f16(afloat4 v1, afloat4 v2)
        {
            return details2::convert_f32_f16(v1, v2);
        }

        inline compact_half4 compact(half4 value)
        {
            const uint32_t shuffle_k = _MM_SHUFFLE(3, 0, 2, 1);
            const uint32_t shuffle_k_1 = _MM_SHUFFLE(0, 3, 1, 2);

            __m128i h1_v = _mm_shufflelo_epi16(value, shuffle_k);
            __m128i h2_v = _mm_shufflehi_epi16(value, shuffle_k);
            __m128i h3_v = _mm_shuffle_epi32(h2_v, shuffle_k_1);

            __m128i h4_v = _mm_unpacklo_epi32(h1_v, h3_v);
            
            //todo: 

            assert(false);
            /*
            __int64 s = _mm_cvtsi128_si64x(h4_v);

            return s;
            */
            compact_half4 r;
            return r;
        }

        inline void store1(half* __restrict address, half4 value)
        {
            compact_half4 s = compact(value);
            half* __restrict h = reinterpret_cast<half* __restrict> (&s);
            *address++ = *h++;
        }

        inline void store2(half* __restrict address, half4 value)
        {
            compact_half4 s = compact(value);
            half* __restrict h = reinterpret_cast<half* __restrict> (&s);
            *address++ = *h++;
            *address++ = *h++;
        }

        inline void store3(half* __restrict address, half4 value)
        {
            compact_half4 s = compact(value);
            half* __restrict h = reinterpret_cast<half* __restrict> (&s);
            *address++ = *h++;
            *address++ = *h++;
            *address++ = *h++;
        }

        inline void store4(half* __restrict address, half4 value)
        {
            compact_half4 s = compact(value);
            *(reinterpret_cast<compact_half4*> (address)) = s;
        }

        inline void stream(half* __restrict address, half4_2 value)
        {
            _mm_stream_si128(reinterpret_cast<__m128i* __restrict>(address), value);
        }

        inline void convert_f32_f16_stream(const float* in_buffer, uint32_t count, math::half* out_buffer)
        {
            for (uint32_t i = 0; i < count; i += 8)
            {
                float4 v_1 = math::load4(&in_buffer[i]);
                float4 v_2 = math::load4(&in_buffer[i + 4]);
                half4_2  k = math::convert_f32_f16(v_1, v_2);
                math::stream(&out_buffer[i], k);
            }
        }

        inline void convert_3_x_f32_f16_stream(const float* in_buffer, size_t count, float pad, math::half* out_buffer)
        {
            float4 one = math::load1(&pad);
            one = swizzle<x, x, x, x>(one);

            alignas(16) const uint32_t mask_w[4] = { 0, 0, 0, 0xFFFFFFFF };
            size_t j = 0;

            for (size_t i = 0; i < count; i += 6, j += 8)
            {
                float4 v_1 = math::load3u(&in_buffer[i]);
                float4 v_2 = math::load3u(&in_buffer[i + 3]);

                float4 v_3 = select(v_1, one, reinterpret_cast<const float4*> (&mask_w)[0]);
                float4 v_4 = select(v_2, one, reinterpret_cast<const float4*> (&mask_w)[0]);

                half4_2  k = math::convert_f32_f16(v_3, v_4);
                math::stream(&out_buffer[j], k);
            }
        }

        #if defined( _X64 )
        inline void convert_f32_f16_stream(const float* in_buffer, size_t count, math::half* out_buffer)
        {
            convert_f32_f16_stream(in_buffer, static_cast<uint32_t>(count), out_buffer);
        }
        #endif
    }
}



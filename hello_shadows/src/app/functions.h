#pragma once

#include <tuple>

#include "vector.h"
#include "defines.h"

namespace uc
{
    namespace math
    {
        namespace details1
        {
            inline float4 UC_MATH_CALL int_part(afloat4 v)
            {
                alignas(16) static const uint32_t sign_mask[4] = { 0x80000000,  0x80000000, 0x80000000, 0x80000000 };

                float4 mask = load4(reinterpret_cast<const float*> (&sign_mask[0]));
                float4 sign = simd_and(v, mask);
                float4 abs  = simd_and_not(mask, v);

                //this number is 2<<23 as a floating point number.
                //23 is the bits in the mantissa
                float4 two_shift_23 = splat(8388608.0f);

                float4 result = add(abs, two_shift_23); //eliminate the mantissa in v
                result = sub(result, two_shift_23);             //reconstruct the number

                //1.6 returns 2, instead of 1
                float4 le = compare_le(abs, result);
                float4 result_1 = sub(result, one());
                float4 s = select(result, result_1, le);

                //mirror the result for negatives
                return simd_or(s, sign);
            }

            inline float4 UC_MATH_CALL frac_part(afloat4 v)
            {
                return sub(v, int_part(v));
            }
        }

        namespace details2
        {
            inline float4 UC_MATH_CALL int_part(afloat4 v)
            {
                // truncate value
                __m128i  trunc = _mm_cvttps_epi32(v);
                __m128  mask = _mm_castsi128_ps(_mm_cmpeq_epi32(trunc, _mm_set1_epi32(0x80000000u)));

                // convert back to float and mask out invalid values
                __m128  x = _mm_cvtepi32_ps(trunc);
                x = _mm_andnot_ps(mask, x);
                return _mm_add_ps(x, _mm_and_ps(mask, v));
            }

            inline float4 UC_MATH_CALL frac_part(afloat4 v)
            {
                return sub(v, int_part(v));
            }
        }

        namespace details
        {
            inline float4 UC_MATH_CALL int_part(afloat4 v)
            {
                return details2::int_part(v);
            }

            inline float4 UC_MATH_CALL frac_part(afloat4 v)
            {
                return details2::frac_part(v);
            }
        }

        inline float4 UC_MATH_CALL truncate(afloat4 value)
        {
            return details::int_part(value);
        }

        inline float4 UC_MATH_CALL floor(afloat4 value)
        {
            float4 int_part = details::int_part(value);
            float4 frac_part = sub(value, int_part);
            float4 int_part_minus_one = sub(int_part, one());

            float4 mask = compare_ge(frac_part, zero());

            float4 result = select(int_part_minus_one, int_part, mask);

            return result;
        }

        inline float4 UC_MATH_CALL ceiling(afloat4 value)
        {
            float4 int_part = details::int_part(value);
            float4 frac_part = sub(value, int_part);
            float4 int_part_plus_one = add(int_part, one());

            float4 mask = compare_ge(frac_part, zero());
            float4 result = select(int_part, int_part_plus_one, mask);

            return result;
        }

        inline float4 UC_MATH_CALL round(afloat4 value)
        {
            float4 one_half = splat(0.5f);
            float4 minus_one_half = splat(-0.5f);

            float4 mask = compare_lt(value, zero());

            float4 int_part_1 = details::int_part(add(value, one_half));
            float4 int_part_2 = details::int_part(add(value, minus_one_half));

            return select(int_part_1, int_part_2, mask);
        }

        namespace details
        {
            inline float4 UC_MATH_CALL negative_multiply_subtract(afloat4 v_1, afloat4 v_2, afloat4 v_3)
            {
                float4 v = mul(v_1, v_2);
                return sub(v_3, v);
            }

            inline float4 UC_MATH_CALL mod_angles(afloat4 value)
            {
                float4 v;
                float4 result;

                float4 reciprocal_two_pi = { 0.159154943f, 0.159154943f, 0.159154943f, 0.159154943f };
                float4 two_pi = { 6.283185307f, 6.283185307f, 6.283185307f, 6.283185307f };

                // Modulo the range of the given angles such that -XM_PI <= Angles < XM_PI
                v = mul(value, reciprocal_two_pi);
                v = round(v);

                result = details::negative_multiply_subtract(two_pi, v, value);

                return result;
            }
        }

        //does not perform parameter check. expects input parameters in -pi<=value<pi
        float4  sin_1(float4 value);
        float4  cos_1(float4 value);

        inline std::tuple<float4, float4> UC_MATH_CALL sin_cos(afloat4 value)
        {
            float4 v = details::mod_angles(value);
            float4 v_1 = sin_1(v);
            float4 v_2 = cos_1(v);

            return std::make_tuple(v_1, v_2);
        }

        inline std::tuple<float4, float4> UC_MATH_CALL sin_cos_1(afloat4 value)
        {
            float4 v_1 = sin_1(value);
            float4 v_2 = cos_1(value);

            return std::make_tuple(v_1, v_2);
        }

        inline float4   UC_MATH_CALL sin(afloat4 value)
        {
            float4 v = details::mod_angles(value);
            return sin_1(v);
        }

        inline float4   UC_MATH_CALL cos(afloat4 value)
        {
            float4 v = details::mod_angles(value);
            return cos_1(v);
        }

        //compile time log2
        template<uint32_t x> struct log2_c
        {
            static const uint32_t value = 1 + log2_c< x / 2>::value;
        };

        template<> struct log2_c<1>
        {
            static const uint32_t value = 0;
        };

        template<> struct log2_c<0>
        {

        };
    }
}


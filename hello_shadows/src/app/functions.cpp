#include "pch.h"

#include "functions.h"

namespace uc
{
    namespace math
    {
        float4	sin_1(float4 value)
        {
            float4 sin_coefficients0 = { 1.0f, -0.166666667f, 8.333333333e-3f, -1.984126984e-4f };
            float4 sin_coefficients1 = { 2.755731922e-6f, -2.505210839e-8f, 1.605904384e-10f, -7.647163732e-13f };
            float4 sin_coefficients2 = { 2.811457254e-15f, -8.220635247e-18f, 1.957294106e-20f, -3.868170171e-23f };


            float4 v_1, v_2, v_3, v_5, v_7, v_9, v_11, v_13, v_15, v_17, v_19, v_21, v_23;
            float4 s_1, s_2, s_3, s_4, s_5, s_6, s_7, s_8, s_9, s_10, s_11;
            float4 result;

            v_1 = value;

            // sin(v) ~= v - v^3 / 3! + v^5 / 5! - v^7 / 7! + v^9 / 9! - v^11 / 11! + v^13 / 13! - 
            //           v^15 / 15! + v^17 / 17! - v^19 / 19! + v^21 / 21! - v^23 / 23! (for -PI <= v < PI)

            v_2 = mul(v_1, v_1);
            v_3 = mul(v_2, v_1);
            v_5 = mul(v_3, v_2);
            v_7 = mul(v_5, v_2);
            v_9 = mul(v_7, v_2);
            v_11 = mul(v_9, v_2);
            v_13 = mul(v_11, v_2);
            v_15 = mul(v_13, v_2);
            v_17 = mul(v_15, v_2);
            v_19 = mul(v_17, v_2);
            v_21 = mul(v_19, v_2);
            v_23 = mul(v_21, v_2);

            s_1 = splat_y(sin_coefficients0);
            s_2 = splat_z(sin_coefficients0);
            s_3 = splat_w(sin_coefficients0);
            s_4 = splat_x(sin_coefficients1);
            s_5 = splat_y(sin_coefficients1);
            s_6 = splat_z(sin_coefficients1);
            s_7 = splat_w(sin_coefficients1);
            s_8 = splat_x(sin_coefficients2);
            s_9 = splat_y(sin_coefficients2);
            s_10 = splat_z(sin_coefficients2);
            s_11 = splat_w(sin_coefficients2);

            result = mad(s_1, v_3, v_1);
            result = mad(s_2, v_5, result);
            result = mad(s_3, v_7, result);
            result = mad(s_4, v_9, result);
            result = mad(s_5, v_11, result);
            result = mad(s_6, v_13, result);
            result = mad(s_7, v_15, result);
            result = mad(s_8, v_17, result);
            result = mad(s_9, v_19, result);
            result = mad(s_10, v_21, result);
            result = mad(s_11, v_23, result);

            return result;
        }

        float4	cos_1(float4 value)
        {
            float4 cos_coefficients0 = { 1.0f, -0.5f, 4.166666667e-2f, -1.388888889e-3f };
            float4 cos_coefficients1 = { 2.480158730e-5f, -2.755731922e-7f, 2.087675699e-9f, -1.147074560e-11f };
            float4 cos_coefficients2 = { 4.779477332e-14f, -1.561920697e-16f, 4.110317623e-19f, -8.896791392e-22f };

            float4 v_1, v_2, v_4, v_6, v_8, v_10, v_12, v_14, v_16, v_18, v_20, v_22;
            float4 c_1, c_2, c_3, c_4, c_5, c_6, c_7, c_8, c_9, c_10, c_11;
            float4 result;

            v_1 = value;

            // cos(v) ~= 1 - v^2 / 2! + v^4 / 4! - v^6 / 6! + v^8 / 8! - v^10 / 10! + v^12 / 12! - 
            //           v^14 / 14! + v^16 / 16! - v^18 / 18! + v^20 / 20! - v^22 / 22! (for -PI <= V < PI)

            v_2 = mul(v_1, v_1);
            v_4 = mul(v_2, v_2);
            v_6 = mul(v_4, v_2);
            v_8 = mul(v_4, v_4);
            v_10 = mul(v_6, v_4);
            v_12 = mul(v_6, v_6);
            v_14 = mul(v_8, v_6);
            v_16 = mul(v_8, v_8);
            v_18 = mul(v_10, v_8);
            v_20 = mul(v_10, v_10);
            v_22 = mul(v_12, v_10);

            c_1 = splat_y(cos_coefficients0);
            c_2 = splat_z(cos_coefficients0);
            c_3 = splat_w(cos_coefficients0);
            c_4 = splat_x(cos_coefficients1);
            c_5 = splat_y(cos_coefficients1);
            c_6 = splat_z(cos_coefficients1);
            c_7 = splat_w(cos_coefficients1);
            c_8 = splat_x(cos_coefficients2);
            c_9 = splat_y(cos_coefficients2);
            c_10 = splat_z(cos_coefficients2);
            c_11 = splat_w(cos_coefficients2);

            result = mad(c_1, v_2, one());
            result = mad(c_2, v_4, result);
            result = mad(c_3, v_6, result);
            result = mad(c_4, v_8, result);
            result = mad(c_5, v_10, result);
            result = mad(c_6, v_12, result);
            result = mad(c_7, v_14, result);
            result = mad(c_8, v_16, result);
            result = mad(c_9, v_18, result);
            result = mad(c_10, v_20, result);
            result = mad(c_11, v_22, result);

            return result;
        }
    }
}


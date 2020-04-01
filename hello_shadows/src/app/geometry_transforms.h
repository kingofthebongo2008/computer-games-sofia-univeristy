#pragma once

#include <limits>
#include <tuple>

#include "geometry.h"
#include "vector.h"
#include "matrix.h"

namespace uc
{
    namespace math
    {
        //translation and rotation or reflection, preserve length and angles
        struct euclidean_transform_3d
        {
            float4x4 m_value;   
        };

        //euclidean + scale, shear,  does not preserve length and angles
        struct affine_transform_3d
        {
            float4x4 m_value;   
        };

        //affine + does not preserve parallel lines
        struct projective_transform_3d
        {
            float4x4 m_value;
        };

        inline euclidean_transform_3d UC_MATH_CALL load_euclidean_transform_3d(const float* __restrict address)
        {
            euclidean_transform_3d r;
            r.m_value = load44(address);
        }

        inline euclidean_transform_3d UC_MATH_CALL make_euclidean_transform_3d(afloat4x4 m)
        {
            euclidean_transform_3d r;
            r.m_value = m;
            return r;
        }

        inline euclidean_transform_3d UC_MATH_CALL loadu_euclidean_transform_3d(const float* __restrict address)
        {
            euclidean_transform_3d r;
            r.m_value = load44u(address);
        }

        inline affine_transform_3d UC_MATH_CALL load_affine_transform_3d(const float* __restrict address)
        {
            affine_transform_3d r;
            r.m_value = load44(address);
        }

        inline affine_transform_3d UC_MATH_CALL loadu_affine_transform_3d(const float* __restrict address)
        {
            affine_transform_3d r;
            r.m_value = load44u(address);
        }

        inline projective_transform_3d UC_MATH_CALL load_projective_transform_3d(const float* __restrict address)
        {
            projective_transform_3d r;
            r.m_value = load44(address);
        }

        inline projective_transform_3d UC_MATH_CALL loadu_projective_transform_3d(const float* __restrict address)
        {
            projective_transform_3d r;
            r.m_value = load44u(address);
        }
    }
}



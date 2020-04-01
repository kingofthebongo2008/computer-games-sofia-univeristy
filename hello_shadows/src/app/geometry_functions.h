#pragma once

#include <limits>
#include <tuple>

#include "geometry.h"
#include "geometry_transforms.h"
#include "graphics.h"

namespace uc
{
    namespace math
    {
        struct aabb_center_radius
        {
            float4 m_center;   
            float4 m_extents;  //half width extents
        };

        inline float4 bounds_min(const aabb_center_radius& b)
        {
            float4 offset = math::set(-1.0f, -1.0f, -1.0f, 0.0f);
            return mad(b.m_extents, offset, b.m_center);
        }

        inline float4 bounds_max(const aabb_center_radius& b)
        {
            float4 offset = math::set(1.0f, 1.0f, 1.0f, 0.0f);
            return mad(b.m_extents, offset, b.m_center);
        }

        using aabb1 = aabb_center_radius;

        inline aabb1 transform( const aabb1& b, const euclidean_transform_3d& t)
        {
            static const float4 box_offset[8] =
            {
                { -1.0f, -1.0f,  1.0f, 0.0f },
                {  1.0f, -1.0f,  1.0f, 0.0f },
                {  1.0f,  1.0f,  1.0f, 0.0f },
                { -1.0f,  1.0f,  1.0f, 0.0f },
                { -1.0f, -1.0f, -1.0f, 0.0f },
                {  1.0f, -1.0f, -1.0f, 0.0f },
                {  1.0f,  1.0f, -1.0f, 0.0f },
                { -1.0f,  1.0f, -1.0f, 0.0f },
            };

            float4 aabb_point = transform3(mad(b.m_extents, box_offset[0], b.m_center), t.m_value);

            float4 min_point = aabb_point;
            float4 max_point = aabb_point;

            for (auto i = 1U; i < 8; ++i)
            {
                aabb_point = transform3(mad(b.m_extents, box_offset[i], b.m_center), t.m_value);
                min_point = min(aabb_point, min_point);
                max_point = max(aabb_point, max_point);
            }


            aabb1 r;
            r.m_center  = mul(add(min_point, max_point), 0.5f);
            r.m_extents = mul(sub(max_point, min_point), 0.5f);
            return r;
        }
    }
}


